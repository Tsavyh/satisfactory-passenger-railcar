"""Offline packager tests; synthetic fixtures are not a compiled mod."""
import contextlib
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch
import zipfile

# Explicit reuse; unittest discovery with -s Tests exposes this module.
from test_release_verifier import payloads, PREFIX, DESCRIPTOR, MODULES, DLL, VERSION

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "tools" / "package_release.py"
SIX = sorted(name for name in payloads() if name.startswith(PREFIX))


class ReleasePackagerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.api = None
        if SCRIPT.is_file():
            spec = importlib.util.spec_from_file_location("release_packager", SCRIPT)
            cls.api = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(cls.api)

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="railcar packager ")
        self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name)
        self.plugin = self.base / "cooked plugin"
        self.plugin.mkdir()
        for name, data in payloads().items():
            if name in SIX:
                path = self.selected(name)
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(data)
        self.output = self.base / "new release"

    def selected(self, name):
        return self.plugin / name.removeprefix(PREFIX)

    def require_api(self):
        if self.api is None:
            self.skipTest("API absent: initial RED establishes absence only")

    def build(self, output=None):
        self.require_api()
        return self.api.build_release(self.plugin, output or self.output)

    def refuse(self, output=None):
        self.require_api()
        with self.assertRaises(self.api.PackagingError) as caught:
            self.build(output)
        self.assertEqual(str(caught.exception), self.api.FAILURE)
        self.assertNotIn("CANARY", str(caught.exception))

    def clean_parent(self):
        self.assertEqual({p.name for p in self.base.iterdir()}, {self.plugin.name})

    def link(self, path, target, directory=False):
        try:
            path.symlink_to(target, target_is_directory=directory)
        except (OSError, NotImplementedError):
            self.skipTest("symlink creation not available on this platform/account")

    def test_api_present(self):
        self.assertTrue(SCRIPT.is_file(), "missing release packager API")
        self.assertTrue(callable(getattr(self.api, "build_release", None)))

    def test_real_verifier_exact_members_hashes_and_immutable_inputs(self):
        self.require_api()
        extra = self.plugin / "Source" / "private CANARY.pdb"
        extra.parent.mkdir()
        extra.write_bytes(b"not packaged")
        before = {str(p.relative_to(self.plugin)): p.read_bytes()
                  for p in self.plugin.rglob("*") if p.is_file()}
        result = self.build()
        self.assertIsInstance(result, Path)
        self.assertEqual(result.parent, self.output)
        self.assertEqual(result.name, f"satisfactory-passenger-railcar-{VERSION}-windows-steam.zip")
        self.assertEqual({p.name for p in self.output.iterdir()}, {result.name, "SHA256SUMS.txt"})
        digest = hashlib.sha256(result.read_bytes()).hexdigest()
        self.assertEqual((self.output / "SHA256SUMS.txt").read_bytes(),
                         f"{digest}  {result.name}\n".encode("ascii"))
        self.api.verifier.verify_release(result, VERSION, digest)  # Real, unmocked oracle.
        with zipfile.ZipFile(result) as archive:
            self.assertEqual(archive.namelist(), sorted([*SIX, "LICENSE", "LEEME.txt", "MANIFEST.json"]))
            manifest = json.loads(archive.read("MANIFEST.json"))
            self.assertEqual(manifest["version"], VERSION)
            self.assertEqual(manifest["license_own_code"], "MIT")
            self.assertEqual(set(manifest["files"]), set(archive.namelist()) - {"MANIFEST.json"})
            for name, expected in manifest["files"].items():
                self.assertEqual(hashlib.sha256(archive.read(name)).hexdigest(), expected)
            for name in SIX:
                self.assertEqual(archive.read(name), payloads()[name])
            self.assertEqual(archive.read("LICENSE"), (ROOT / "LICENSE").read_bytes())
            for info in archive.infolist():
                self.assertEqual(info.date_time, (1980, 1, 1, 0, 0, 0))
                self.assertEqual(info.external_attr >> 16, 0o100644)
            readme = archive.read("LEEME.txt").decode("utf-8")
            self.assertIn("SML", readme)
            self.assertNotIn("CANARY", readme)
            self.assertNotIn(str(self.base), readme)
            self.assertFalse(any("smmprofile" in name for name in archive.namelist()))
        after = {str(p.relative_to(self.plugin)): p.read_bytes()
                 for p in self.plugin.rglob("*") if p.is_file()}
        self.assertEqual(before, after)

    def test_windows_stat_ctime_disagreement_accepts_unchanged_payloads(self):
        self.require_api()
        metadata = self.api._metadata
        fstat = self.api.os.fstat

        def windows_info(info, descriptor=False):
            # Model CPython 3.12's path/fd ctime mismatch; birthtime is stable.
            return SimpleNamespace(
                st_mode=info.st_mode, st_dev=info.st_dev, st_ino=info.st_ino,
                st_size=info.st_size, st_mtime_ns=info.st_mtime_ns,
                st_ctime_ns=info.st_ctime_ns + (100 if descriptor else 0),
                st_birthtime_ns=getattr(info, "st_birthtime_ns", info.st_ctime_ns),
                st_file_attributes=getattr(info, "st_file_attributes", 0))

        with patch.object(self.api.sys, "platform", "win32"), \
             patch.object(self.api, "_metadata", side_effect=lambda *a, **k: windows_info(metadata(*a, **k))), \
             patch.object(self.api.os, "fstat", side_effect=lambda fd: windows_info(fstat(fd), True)):
            result = self.build()
        self.api.verifier.verify_release(result, VERSION)
        with zipfile.ZipFile(result) as archive:
            for name in SIX:
                self.assertEqual(archive.read(name), payloads()[name])

    def test_platform_stamp_retains_identity_and_change_guards(self):
        self.require_api()
        fields = dict(st_dev=1, st_ino=2, st_size=3, st_mtime_ns=4,
                      st_ctime_ns=5, st_birthtime_ns=6)
        before = SimpleNamespace(**fields)
        ctime_only = SimpleNamespace(**(fields | {"st_ctime_ns": 7}))
        with patch.object(self.api.sys, "platform", "win32"):
            expected = self.api._stamp(before)
            self.assertEqual(expected, self.api._stamp(ctime_only))
            for field in ("st_dev", "st_ino", "st_size", "st_mtime_ns", "st_birthtime_ns"):
                with self.subTest(field=field):
                    changed = SimpleNamespace(**(fields | {field: fields[field] + 1}))
                    self.assertNotEqual(expected, self.api._stamp(changed))
        with patch.object(self.api.sys, "platform", "linux"):
            self.assertNotEqual(self.api._stamp(before), self.api._stamp(ctime_only))

    def test_deterministic_same_toolchain(self):
        self.require_api()
        first = self.build()
        second = self.build(self.base / "another release")
        self.assertEqual(first.read_bytes(), second.read_bytes())
        self.assertEqual((first.parent / "SHA256SUMS.txt").read_bytes(),
                         (second.parent / "SHA256SUMS.txt").read_bytes())

    def test_version_and_requirements_propagate_without_runtime_claims(self):
        self.require_api()
        path = self.selected(DESCRIPTOR)
        value = json.loads(path.read_bytes())
        value.update(VersionName="2.3.4", SemVersion="2.3.4", GameVersion=">=999999")
        value["Plugins"][0]["SemVersion"] = "^4.5.6"
        path.write_text(json.dumps(value), encoding="utf-8")
        result = self.build()
        self.assertIn("2.3.4", result.name)
        self.api.verifier.verify_release(result, "2.3.4")
        with zipfile.ZipFile(result) as archive:
            readme = archive.read("LEEME.txt").decode("utf-8")
            for text in ("2.3.4", ">=999999", "^4.5.6"):
                self.assertIn(text, readme)
            for text in ("0.1.9", "startup passed", "tested"):
                self.assertNotIn(text, readme)

    def test_existing_directory_empty_or_populated_and_file_unchanged(self):
        self.require_api()
        for kind in ("empty", "populated", "file"):
            with self.subTest(kind=kind):
                destination = self.base / kind
                if kind == "file":
                    destination.write_bytes(b"old")
                else:
                    destination.mkdir()
                    if kind == "populated":
                        (destination / "old").write_bytes(b"old")
                self.refuse(destination)
                if kind == "file":
                    self.assertEqual(destination.read_bytes(), b"old")
                else:
                    self.assertEqual({p.name for p in destination.iterdir()},
                                     {"old"} if kind == "populated" else set())

    def test_existing_symlink_and_dangling_link_unchanged(self):
        self.require_api()
        for dangling in (False, True):
            with self.subTest(dangling=dangling):
                link = self.base / ("dangling" if dangling else "link")
                target = self.base / "absent" if dangling else self.plugin
                self.link(link, target, not dangling)
                self.refuse(link)
                self.assertTrue(link.is_symlink())

    def test_missing_parent_inside_input_and_linked_parent_refused(self):
        self.require_api()
        self.refuse(self.base / "missing" / "release")
        self.refuse(self.plugin / "release")
        self.assertFalse((self.plugin / "release").exists())
        alias = self.base / "alias"
        self.link(alias, self.base, True)
        self.refuse(alias / "release")
        self.assertFalse((self.base / "release").exists())

    def test_missing_empty_nonregular_selected_files(self):
        self.require_api()
        for name in SIX:
            for kind in ("missing", "empty", "directory"):
                with self.subTest(name=name, kind=kind):
                    path = self.selected(name)
                    original = path.read_bytes()
                    path.unlink()
                    if kind == "empty":
                        path.write_bytes(b"")
                    elif kind == "directory":
                        path.mkdir()
                    self.refuse()
                    self.clean_parent()
                    if kind == "directory":
                        path.rmdir()
                    path.write_bytes(original)

    def test_selected_symlink_and_ancestor_symlink_refused(self):
        self.require_api()
        path = self.selected(DLL)
        data = path.read_bytes()
        target = self.base / "external DLL"
        target.write_bytes(data)
        path.unlink()
        self.link(path, target)
        self.refuse()
        self.assertEqual(target.read_bytes(), data)
        path.unlink()
        path.write_bytes(data)
        binaries = self.plugin / "Binaries"
        moved = self.base / "external Binaries"
        binaries.rename(moved)
        self.link(binaries, moved, True)
        self.refuse()
        self.assertFalse(self.output.exists())

    @unittest.skipUnless(hasattr(os, "mkfifo"), "FIFO creation unavailable on this platform")
    def test_selected_fifo_refused_without_blocking(self):
        self.require_api()
        path = self.selected(DLL)
        path.unlink()
        os.mkfifo(path)
        self.refuse()
        self.clean_parent()

    def test_descriptor_strict_version_json_and_module_contracts(self):
        self.require_api()
        path = self.selected(DESCRIPTOR)
        original = path.read_bytes()
        bad_values = [b"{", b"[]", b"{\"x\":NaN}", b"{\"x\":1,\"x\":2}"]
        value = json.loads(original)
        for key, bad in (("VersionName", "01.1.9"), ("SemVersion", "0.1.8"),
                         ("VersionName", "CANARY"), ("VersionName", True), ("Modules", []),
                         ("Plugins", []), ("GameVersion", "CANARY/private")):
            changed = dict(value)
            changed[key] = bad
            bad_values.append(json.dumps(changed).encode())
        for index, data in enumerate(bad_values):
            with self.subTest(index=index):
                path.write_bytes(data)
                self.refuse()
                self.clean_parent()
        path.write_bytes(original)

    def test_real_verifier_rejects_bad_modules_and_pe_import(self):
        self.require_api()
        for name, bad in ((MODULES, b"{}"), (MODULES, b"{"), (DLL, b"not PE"),
                          (DLL, payloads()[DLL].replace(b"FactoryDedicatedClient", b"XactoryDedicatedClient"))):
            with self.subTest(name=name, size=len(bad)):
                path = self.selected(name)
                original = path.read_bytes()
                path.write_bytes(bad)
                self.refuse()
                self.clean_parent()
                path.write_bytes(original)

    def test_declared_limits_rejected_before_copy(self):
        self.require_api()
        for name, limit in ((DESCRIPTOR, self.api.verifier.MAX_JSON),
                            (MODULES, self.api.verifier.MAX_JSON),
                            (DLL, self.api.verifier.MAX_MEMBER)):
            with self.subTest(name=name):
                path = self.selected(name)
                original = path.read_bytes()
                with path.open("r+b") as stream:
                    stream.truncate(limit + 1)
                self.refuse()
                self.clean_parent()
                path.write_bytes(original)
        with patch.object(self.api.verifier, "MAX_TOTAL", 1):
            self.refuse()
        self.clean_parent()

    def test_mutation_or_truncation_after_inventory_refused(self):
        self.require_api()
        original = self.api._copy_input
        for data in (b"short", b"X" * len(payloads()[DLL])):
            with self.subTest(size=len(data)):
                def mutate(source, *args, **kwargs):
                    if source == self.selected(DLL):
                        source.write_bytes(data)
                    return original(source, *args, **kwargs)
                with patch.object(self.api, "_copy_input", mutate):
                    self.refuse()
                self.clean_parent()
                self.selected(DLL).write_bytes(payloads()[DLL])

    def test_verification_failure_never_publishes_and_cleans_stage(self):
        self.require_api()
        with patch.object(self.api.verifier, "verify_release",
                          side_effect=self.api.verifier.VerificationError()):
            self.refuse()
        self.clean_parent()

    def test_publication_write_failure_cleans_only_owned_output(self):
        self.require_api()
        original = Path.open
        def failing_open(path, *args, **kwargs):
            if path == self.output / "SHA256SUMS.txt":
                raise OSError("CANARY/private")
            return original(path, *args, **kwargs)
        with patch.object(Path, "open", failing_open):
            self.refuse()
        self.clean_parent()

    def test_publication_creation_race_preserves_winner(self):
        self.require_api()
        original = Path.mkdir
        def raced_mkdir(path, *args, **kwargs):
            if path == self.output:
                original(path)
                (path / "winner").write_bytes(b"old")
            return original(path, *args, **kwargs)
        with patch.object(Path, "mkdir", raced_mkdir):
            self.refuse()
        self.assertEqual({p.name for p in self.output.iterdir()}, {"winner"})
        self.assertEqual((self.output / "winner").read_bytes(), b"old")
        self.assertEqual({p.name for p in self.base.iterdir()}, {self.plugin.name, self.output.name})

    def test_requirement_prose_is_not_copied_into_generated_instructions(self):
        self.require_api()
        path = self.selected(DESCRIPTOR)
        original = json.loads(path.read_bytes())
        for index, (field, text) in enumerate((("GameVersion", "CANARY"), ("GameVersion", "   "),
                                                ("SML", "CANARY"), ("SML", "3.12.0 private name"))):
            with self.subTest(field=field, kind=len(text)):
                value = json.loads(json.dumps(original))
                if field == "SML":
                    value["Plugins"][0]["SemVersion"] = text
                else:
                    value[field] = text
                path.write_text(json.dumps(value), encoding="utf-8")
                output = self.base / f"prose-{index}"
                self.refuse(output)
                self.assertFalse(output.exists())

    def test_source_copy_and_actual_publication_write_failure_cleanup(self):
        self.require_api()
        original_copy = self.api._copy_input
        calls = []
        def failed_copy(*args, **kwargs):
            calls.append(1)
            if len(calls) == 2:
                raise OSError("CANARY/private")
            return original_copy(*args, **kwargs)
        with patch.object(self.api, "_copy_input", failed_copy):
            self.refuse()
        self.clean_parent()
        original_open = Path.open
        class BrokenWriter:
            def __init__(self, stream):
                self.stream = stream
            def __enter__(self):
                return self
            def __exit__(self, *exc):
                self.stream.close()
            def write(self, data):
                self.stream.write(data[:1])
                raise OSError("CANARY/private")
        def failed_open(path, *args, **kwargs):
            stream = original_open(path, *args, **kwargs)
            if path.parent == self.output and args and args[0] == "xb":
                return BrokenWriter(stream)
            return stream
        with patch.object(Path, "open", failed_open):
            self.refuse()
        self.clean_parent()

    def test_unrelated_entries_are_not_enumerated_or_opened(self):
        self.require_api()
        extra = self.plugin / "private.pdb"
        extra.write_bytes(b"CANARY")
        original = self.api.os.open
        opened = []
        def recording_open(path, *args, **kwargs):
            opened.append(Path(path))
            return original(path, *args, **kwargs)
        # Path iteration is never needed to select plugin input.
        with patch.object(Path, "iterdir", side_effect=RuntimeError("no enumeration")), \
             patch.object(self.api.os, "open", recording_open):
            result = self.build()
        self.assertNotIn(extra, opened)
        self.assertTrue(result.is_file())
        self.assertEqual(extra.read_bytes(), b"CANARY")

    def test_input_root_symlink_refused(self):
        self.require_api()
        alias = self.base / "cooked alias"
        self.link(alias, self.plugin, True)
        with self.assertRaises(self.api.PackagingError):
            self.api.build_release(alias, self.output)
        self.assertFalse(self.output.exists())

    def test_cli_spaces_unrelated_cwd_optimized_success_and_refusal(self):
        self.require_api()
        for flags in ([], ["-O"]):
            output = self.base / ("optimized" if flags else "normal")
            command = [sys.executable, "-B", *flags, str(SCRIPT), str(self.plugin), "--output", str(output)]
            result = subprocess.run(command, cwd=self.base, capture_output=True, text=True, timeout=15)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn("package checks only", result.stdout)
            refusal = subprocess.run(command, cwd=self.base, capture_output=True, text=True, timeout=15)
            self.assertNotEqual(refusal.returncode, 0)
            self.assertNotIn("PASS", refusal.stdout + refusal.stderr)

    def test_cli_sanitized_failures_help_and_import_safe(self):
        self.require_api()
        base = [sys.executable, "-B", str(SCRIPT)]
        for args in ([], ["--CANARY"], ["CANARY/private", "--output", str(self.output)],
                     [str(self.plugin), "--output", "CANARY/private/new"]):
            result = subprocess.run(base + args, cwd=self.base, capture_output=True, text=True, timeout=15)
            self.assertNotEqual(result.returncode, 0)
            for text in ("CANARY", "Traceback", "PASS"):
                self.assertNotIn(text, result.stdout + result.stderr)
        help_result = subprocess.run(base + ["--help"], capture_output=True, text=True, timeout=15)
        self.assertEqual(help_result.returncode, 0)
        self.assertIn("PLUGIN_DIR", help_result.stdout)
        out, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            spec = importlib.util.spec_from_file_location("packager_import_only", SCRIPT)
            spec.loader.exec_module(importlib.util.module_from_spec(spec))
        self.assertEqual(out.getvalue() + err.getvalue(), "")


if __name__ == "__main__":
    unittest.main()
