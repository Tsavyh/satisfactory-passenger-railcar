"""Offline adversarial fixtures: synthetic PE/ZIP bytes, NOT a rebuilt mod."""
import contextlib
import copy
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import stat
import struct
import subprocess
import sys
import tempfile
import unittest
import warnings
import zipfile

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "tools" / "verify_release.py"
PREFIX = "VAGON/TsabsPassengerWagon/"
DLL_NAME = "FactoryGameSteam-TsabsPassengerWagon-Win64-Shipping.dll"
DLL = PREFIX + "Binaries/Win64/" + DLL_NAME
MODULES = PREFIX + "Binaries/Win64/FactoryGameSteam-Win64-Shipping.modules"
DESCRIPTOR = PREFIX + "TsabsPassengerWagon.uplugin"
DEPENDENCY = b"FactoryGameSteam-FactoryDedicatedClient-Win64-Shipping.dll"
VERSION = "0.1.9"
NAME = "satisfactory-passenger-railcar-0.1.9-windows-steam.zip"


def encoded(value):
    return json.dumps(value).encode("utf-8")


def synthetic_pe():
    """One x64 PE32+ section, one normal import and one named thunk."""
    data = bytearray(1536)
    data[:2] = b"MZ"
    struct.pack_into("<I", data, 0x3C, 0x80)
    data[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<HHIIIHH", data, 0x84, 0x8664, 1, 0, 0, 0, 240, 0x2022)
    optional = 0x98
    struct.pack_into("<H", data, optional, 0x20B)
    struct.pack_into("<I", data, optional + 60, 512)
    struct.pack_into("<I", data, optional + 108, 16)
    struct.pack_into("<II", data, optional + 120, 0x1000, 40)
    section = optional + 240
    data[section:section + 8] = b".rdata\0\0"
    struct.pack_into("<IIII", data, section + 8, 1024, 0x1000, 1024, 512)
    struct.pack_into("<IIIII", data, 512, 0x1100, 0, 0, 0x1080, 0x1120)
    data[640:640 + len(DEPENDENCY) + 1] = DEPENDENCY + b"\0"
    struct.pack_into("<Q", data, 768, 0x1140)
    struct.pack_into("<Q", data, 800, 0x1140)
    data[834:841] = b"symbol\0"
    return bytes(data)


def payloads():
    return {
        "LEEME.txt": b"Synthetic fixture only.\n",
        "LICENSE": b"Synthetic license fixture.\n",
        "Solo-SML-3.12.0.smmprofile": encoded({
            "profile": {"name": "TsabsRailTest-SML312", "mods": {
                "SML": {"version": "=3.12.0", "enabled": True}},
                "required_targets": ["Windows"]},
            "lockfile": {"version": 1, "mods": {}},
            "metadata": {"gameVersion": 513885}}),
        DESCRIPTOR: encoded({"VersionName": VERSION, "SemVersion": VERSION,
            "Modules": [{"Name": "TsabsPassengerWagon", "Type": "Runtime"}],
            "Plugins": [{"Name": "SML", "Enabled": True, "SemVersion": "^3.12.0"}]}),
        MODULES: encoded({"BuildId": "SML", "Modules": {"TsabsPassengerWagon": DLL_NAME}}),
        DLL: synthetic_pe(),
        **{PREFIX + "Content/Paks/Windows/TsabsPassengerWagonFactoryGame-Windows." + ext:
           b"synthetic cooked payload" for ext in ("pak", "ucas", "utoc")},
    }


def archive(path, files=None, manifest=None, extra=(), directories=()):
    files = payloads() if files is None else files
    manifest = ({"version": VERSION, "files": {
        name: hashlib.sha256(data).hexdigest() for name, data in files.items()}}
        if manifest is None else manifest)
    with warnings.catch_warnings():
        warnings.simplefilter("ignore", UserWarning)  # deliberate duplicate ZIP fixtures
        with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED) as output:
            for name in directories:
                output.writestr(name, b"")
            for name, data in files.items():
                output.writestr(name, data)
            output.writestr("MANIFEST.json", encoded(manifest) if isinstance(manifest, dict) else manifest)
            for name, data in extra:
                output.writestr(name, data)
    return path


class ReleaseVerifierTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.api = None
        if SCRIPT.is_file():
            spec = importlib.util.spec_from_file_location("release_verifier", SCRIPT)
            cls.api = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(cls.api)

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.directory = Path(self.temp.name)
        self.path = self.directory / NAME

    def require_api(self):
        if self.api is None:
            self.skipTest("API absent; initial RED establishes absence only")

    def verify(self, path=None, version=VERSION, sha=None):
        self.require_api()
        return self.api.verify_release(path or self.path, version, sha)

    def refuse(self, files=None, manifest=None, extra=(), directories=()):
        self.require_api()
        archive(self.path, files, manifest, extra, directories)
        with self.assertRaises(self.api.VerificationError) as caught:
            self.verify()
        self.assertNotIn("CANARY", str(caught.exception))

    def test_api_present(self):
        self.assertTrue(SCRIPT.is_file(), "missing offline release verifier API")
        self.assertTrue(callable(getattr(self.api, "verify_release", None)))

    def test_success_no_writes_and_optional_profile(self):
        self.require_api()
        files = payloads()
        for profile in (True, False):
            with self.subTest(profile=profile):
                if not profile:
                    del files["Solo-SML-3.12.0.smmprofile"]
                archive(self.path, files, directories=("VAGON/", PREFIX))
                before = {p.name: p.read_bytes() for p in self.directory.iterdir()}
                self.verify()
                self.assertEqual(before, {p.name: p.read_bytes() for p in self.directory.iterdir()})

    def test_outer_checksum(self):
        self.require_api()
        archive(self.path)
        digest = hashlib.sha256(self.path.read_bytes()).hexdigest()
        self.verify(sha=digest)
        for value in ("0" * 64, "x" * 64, "A" * 64, "CANARY"):
            with self.subTest(kind=len(value)):
                with self.assertRaises(self.api.VerificationError):
                    self.verify(sha=value)

    def test_versions_and_filename(self):
        self.require_api()
        archive(self.path)
        for version in ("0.1.8", "v0.1.9", "0.1", "01.1.9", "0.1.9\n", 1, True, None):
            with self.subTest(kind=type(version).__name__):
                with self.assertRaises(self.api.VerificationError):
                    self.verify(version=version)
        wrong = self.directory / "CANARY.zip"
        self.path.rename(wrong)
        with self.assertRaises(self.api.VerificationError):
            self.verify(wrong)

    def test_descriptor_contract(self):
        self.require_api()
        for key, value in (("VersionName", "0.1.8"), ("SemVersion", 19),
                           ("Modules", []), ("Modules", [{"Name": "Other", "Type": "Runtime"}]),
                           ("Modules", [{"Name": "TsabsPassengerWagon", "Type": "Editor"}]),
                           ("Plugins", []), ("Plugins", [{"Name": "SML", "Enabled": False}]),
                           ("Plugins", [{"Name": "SML", "Enabled": 1}]),
                           ("Plugins", [{"Name": "SML", "Enabled": True, "Optional": True}])):
            with self.subTest(key=key, kind=type(value).__name__):
                files = payloads()
                descriptor = json.loads(files[DESCRIPTOR])
                descriptor[key] = value
                files[DESCRIPTOR] = encoded(descriptor)
                self.refuse(files)

    def test_modules_contract(self):
        self.require_api()
        for value in ({"BuildId": "Other", "Modules": {"TsabsPassengerWagon": DLL_NAME}},
                      {"BuildId": "SML", "Modules": {}},
                      {"BuildId": "SML", "Modules": {"TsabsPassengerWagon": "CANARY.dll"}},
                      {"BuildId": "SML", "Modules": {"TsabsPassengerWagon": DLL_NAME, "SML": "SML.dll"}}):
            files = payloads()
            files[MODULES] = encoded(value)
            self.refuse(files)

    def test_manifest_coverage_version_digest_and_altered_bytes(self):
        self.require_api()
        files = payloads()
        valid = {"version": VERSION, "files": {n: hashlib.sha256(b).hexdigest() for n, b in files.items()}}
        variants = []
        for version in (None, 19, True, "0.1.8", [VERSION]):
            item = copy.deepcopy(valid)
            item["version"] = version
            variants.append(item)
        for digest in ("A" * 64, "x" * 64, 1, None, "0" * 64):
            item = copy.deepcopy(valid)
            item["files"]["LICENSE"] = digest
            variants.append(item)
        item = copy.deepcopy(valid)
        del item["files"]["LICENSE"]
        variants.append(item)
        item = copy.deepcopy(valid)
        item["files"]["CANARY"] = "0" * 64
        variants.append(item)
        for index, item in enumerate(variants):
            with self.subTest(index=index):
                self.refuse(files, item)
        files["LICENSE"] += b"altered"
        self.refuse(files, valid)

    def test_json_malformed_duplicate_keys_and_types(self):
        self.require_api()
        for name in (DESCRIPTOR, MODULES, "Solo-SML-3.12.0.smmprofile", "MANIFEST.json"):
            for bad in (b"{", b"[]", b"null", b"1", b"{\"version\":1,\"version\":2}",
                        b"{\"x\":{\"a\":1,\"a\":2}}", b"{\"x\":NaN}", b"\xff"):
                with self.subTest(name=name, kind=bad[:1]):
                    files = payloads()
                    if name == "MANIFEST.json":
                        self.refuse(files, bad)
                    else:
                        files[name] = bad
                        self.refuse(files)

    def test_profile_only_known_sml_shape(self):
        self.require_api()
        name = "Solo-SML-3.12.0.smmprofile"
        original = json.loads(payloads()[name])
        variants = []
        for field, value in (("mods", {"Other": {"version": "=3.12.0", "enabled": True}}),
                             ("required_targets", ["Linux"]), ("name", "CANARY")):
            item = copy.deepcopy(original)
            item["profile"][field] = value
            variants.append(item)
        item = copy.deepcopy(original)
        item["profile"]["mods"]["SML"]["enabled"] = 1
        variants.append(item)
        item = copy.deepcopy(original)
        item["metadata"]["CANARY"] = "private"
        variants.append(item)
        for item in variants:
            files = payloads()
            files[name] = encoded(item)
            self.refuse(files)

    def test_missing_empty_extra_and_duplicate_members(self):
        self.require_api()
        for name in payloads():
            with self.subTest(name=name):
                files = payloads()
                del files[name]
                if name.endswith("smmprofile"):
                    continue
                self.refuse(files)
                files = payloads()
                files[name] = b""
                self.refuse(files)
        for name in ("CANARY.txt", "VAGON/SML/Binaries/Win64/SML.dll", "LICENSE", "license", "MANIFEST.json"):
            with self.subTest(name=name):
                self.refuse(extra=((name, b"CANARY"),))

    def test_unsafe_paths_and_unrelated_directories(self):
        self.require_api()
        names = ("/CANARY", "../CANARY", "a/../CANARY", "a/./CANARY", "a//CANARY",
                 "a\\CANARY", "C:/CANARY", "a:CANARY", "CANARY\x01", "CANARY\x00hidden",
                 "a./CANARY", "a /CANARY", "CON", "aux.txt", "LPT1/CANARY", "com9.txt",
                 "unused/", "vagon/")
        for index, name in enumerate(names):
            with self.subTest(index=index):
                self.refuse(extra=((name, b"" if name.endswith("/") else b"CANARY"),))

    def test_symlinks_and_special_files(self):
        self.require_api()
        for mode in (stat.S_IFLNK, stat.S_IFIFO, stat.S_IFCHR, stat.S_IFSOCK):
            info = zipfile.ZipInfo("LICENSE")
            info.create_system = 3
            info.external_attr = (mode | 0o600) << 16
            self.refuse(extra=((info, b"CANARY"),))
        files = payloads()
        del files["LICENSE"]
        info = zipfile.ZipInfo("LICENSE")
        info.create_system = 3
        info.external_attr = (stat.S_IFLNK | 0o600) << 16
        self.refuse(files, extra=((info, b"CANARY"),))

    def patch_member_metadata(self, host, attributes, name="LICENSE"):
        """Mutate the real central directory, not a mocked ZipInfo/API."""
        data = bytearray(self.path.read_bytes())
        with zipfile.ZipFile(self.path) as source:
            at = source.start_dir
            for info in source.infolist():
                if info.filename == name:
                    data[at + 5] = host
                    struct.pack_into("<I", data, at + 38, attributes)
                    break
                at += 46 + sum(struct.unpack_from("<HHH", data, at + 28))
            else:
                self.fail("fixture member missing")
        self.path.write_bytes(data)

    def check_metadata_refusal(self):
        from unittest.mock import patch
        before = {p.name: p.read_bytes() for p in self.directory.iterdir()}
        with self.subTest(interface="api"):
            with patch.object(zipfile.ZipFile, "open", autospec=True,
                              side_effect=zipfile.ZipFile.open) as opened:
                with self.assertRaises(self.api.VerificationError) as caught:
                    self.verify()
                self.assertEqual(str(caught.exception), self.api.FAILURE)
                self.assertEqual(opened.call_count, 0, "must refuse before opening payloads")
        for flags in ([], ["-O"]):
            with self.subTest(interface="cli", flags=flags):
                result = subprocess.run(
                    [sys.executable, "-B", *flags, str(SCRIPT), str(self.path),
                     "--expected-version", VERSION], capture_output=True, text=True, timeout=10)
                self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
                self.assertEqual(result.stdout, "")
                self.assertEqual(result.stderr, self.api.FAILURE + "\n")
        self.assertEqual(before, {p.name: p.read_bytes() for p in self.directory.iterdir()})

    def test_ntfs_reparse_metadata_refused_api_and_cli(self):
        self.require_api()
        archive(self.path)
        self.patch_member_metadata(10, 0x400)
        self.check_metadata_refusal()

    def test_windows_special_and_unsupported_metadata_refused(self):
        self.require_api()
        # EFS is unsupported filesystem metadata, not a ZIP encryption flag.
        for host in (0, 3, 10):
            for attributes in (0x8, 0x40, 0x200, 0x400, 0x1000, 0x4000, 0x8000):
                for name in ("LICENSE", PREFIX):
                    with self.subTest(host=host, attributes=attributes, name=name):
                        archive(self.path, directories=(PREFIX,))
                        self.patch_member_metadata(host, attributes, name)
                        self.check_metadata_refusal()
        for host, attributes in ((10, 0x80000000), (10, 0o600 << 16),
                                 (0, stat.S_IFIFO << 16), (3, stat.S_IFIFO << 16),
                                 (0, 0x100), (0, 0x800), (0, 0x2000),
                                 (3, 0x100), (3, 0x800), (3, 0x2000), (255, 0)):
            with self.subTest(host=host, attributes=attributes):
                archive(self.path)
                self.patch_member_metadata(host, attributes)
                self.check_metadata_refusal()

    def test_windows_contradictory_directory_and_normal_metadata_refused(self):
        self.require_api()
        for host in (0, 3, 10):
            for name, attributes in (("LICENSE", 0x10), ("LICENSE", 0x30),
                                     ("LICENSE", 0xA0), (PREFIX, 0x80), (PREFIX, 0x90)):
                with self.subTest(host=host, attributes=attributes, name=name):
                    archive(self.path, directories=(PREFIX,))
                    self.patch_member_metadata(host, attributes, name)
                    self.check_metadata_refusal()

    def test_supported_unix_dos_and_ntfs_metadata_remain_accepted(self):
        self.require_api()
        for host in (0, 3, 10):
            modes = (0,) if host == 10 else (0, 0o600, stat.S_IFREG, stat.S_IFREG | 0o644)
            # DOS low byte, plus a supported NTFS native-attribute subset.
            flags = (0, 0x1, 0x2, 0x4, 0x20, 0x80, 0x27)
            if host == 10:
                flags += (0x100, 0x800, 0x2000, 0x2827)
            for mode in modes:
                for attributes in flags:
                    with self.subTest(host=host, mode=mode, attributes=attributes):
                        archive(self.path, directories=(PREFIX,))
                        self.patch_member_metadata(host, (mode << 16) | attributes)
                        directory_mode = 0 if host == 10 else (stat.S_IFDIR | 0o755)
                        self.patch_member_metadata(host, (directory_mode << 16) | 0x10, PREFIX)
                        self.verify()
            directory_flags = (0, 0x10, 0x11, 0x30)
            if host == 10:
                directory_flags += (0x810, 0x2010)
            for attributes in directory_flags:
                with self.subTest(host=host, directory_attributes=attributes):
                    archive(self.path, directories=(PREFIX,))
                    self.patch_member_metadata(host, attributes, PREFIX)
                    self.verify()

    def test_compression_encryption_size_and_count_metadata(self):
        self.require_api()
        # Patch only central-directory metadata to exercise pre-decompression guards.
        for offset, fmt, value in ((8, "H", 1), (10, "H", 99), (24, "I", 128 * 1024 * 1024 + 1)):
            archive(self.path)
            data = bytearray(self.path.read_bytes())
            at = data.index(b"PK\x01\x02")
            struct.pack_into("<" + fmt, data, at + offset, value)
            self.path.write_bytes(data)
            with self.assertRaises(self.api.VerificationError):
                self.verify()
        self.refuse(extra=tuple(("extra" + str(n), b"x") for n in range(129)))
        files = payloads()
        files[DESCRIPTOR] = b" " * (1024 * 1024 + 1)
        self.refuse(files)
        archive(self.path)
        with self.path.open("r+b") as stream:
            stream.truncate(128 * 1024 * 1024 + 1)
        with self.assertRaises(self.api.VerificationError):
            self.verify()

    def test_malformed_pe_headers_architecture_and_dll_flag(self):
        self.require_api()
        variants = [b"", b"MZ", synthetic_pe()[:200], b"x" * 1536]
        for offset, fmt, value in ((0x3C, "I", 0xFFFFFFF0), (0x84, "H", 0x14C),
                                   (0x94, "H", 12), (0x96, "H", 0x22), (0x98, "H", 0x10B),
                                   (0x98 + 120, "I", 0xFFFFFFF0), (0x98 + 124, "I", 19),
                                   (0x98 + 240 + 20, "I", 0xFFFFFF00)):
            data = bytearray(synthetic_pe())
            struct.pack_into("<" + fmt, data, offset, value)
            variants.append(bytes(data))
        for index, value in enumerate(variants):
            with self.subTest(index=index):
                files = payloads()
                files[DLL] = value
                self.refuse(files)

    def test_pe_import_required_not_string_scan_and_malformed_imports(self):
        self.require_api()
        variants = []
        data = bytearray(synthetic_pe())
        data[512:552] = b"\0" * 40
        variants.append(bytes(data))  # dependency string remains, but no import
        data = bytearray(synthetic_pe())
        data[640:640 + len(DEPENDENCY)] = b"X" * len(DEPENDENCY)
        variants.append(bytes(data))
        for offset, value in ((524, 0xFFFFFF00), (512, 0xFFFFFF00), (528, 0), (532, 1)):
            data = bytearray(synthetic_pe())
            struct.pack_into("<I", data, offset, value)
            variants.append(bytes(data))
        data = bytearray(synthetic_pe())
        data[640:] = b"X" * (len(data) - 640)
        variants.append(bytes(data))
        for index, value in enumerate(variants):
            with self.subTest(index=index):
                files = payloads()
                files[DLL] = value
                self.refuse(files)

    def test_pe_reads_stay_bounded(self):
        self.require_api()
        from unittest.mock import patch
        files = payloads()
        data = bytearray(synthetic_pe())
        data.extend(b"\0" * (2 * 1024 * 1024))
        struct.pack_into("<I", data, 0x98 + 240 + 8, len(data) - 512)
        struct.pack_into("<I", data, 0x98 + 240 + 16, len(data) - 512)
        files[DLL] = bytes(data)
        archive(self.path, files)
        requested = []
        original = zipfile.ZipExtFile.read
        def recording_read(stream, size=-1):
            requested.append(size)
            return original(stream, size)
        with patch.object(zipfile.ZipExtFile, "read", recording_read):
            self.verify()
        self.assertTrue(all(0 <= size <= 1024 * 1024 + 1 for size in requested))

    def test_pe_iat_must_hold_complete_terminated_import_array(self):
        self.require_api()
        files = payloads()
        data = bytearray(synthetic_pe())
        struct.pack_into("<I", data, 528, 0x13F8)
        struct.pack_into("<Q", data, len(data) - 8, 0x1140)
        files[DLL] = bytes(data)
        self.refuse(files)

    def test_json_size_refused_before_any_decompression(self):
        self.require_api()
        from unittest.mock import patch
        files = payloads()
        files[DESCRIPTOR] = b" " * (1024 * 1024 + 1)
        archive(self.path, files)
        reads = []
        original = zipfile.ZipExtFile.read
        def recording_read(stream, size=-1):
            reads.append(size)
            return original(stream, size)
        with patch.object(zipfile.ZipExtFile, "read", recording_read):
            with self.assertRaises(self.api.VerificationError):
                self.verify()
        self.assertEqual(len(reads), 0)

    def test_pe_decompression_work_is_bounded(self):
        self.require_api()
        from unittest.mock import patch
        # Deliberately tiny parse budget, independent of inventory size budget.
        archive(self.path)
        with patch.object(self.api, "MAX_PE_WORK", 1, create=True):
            with self.assertRaises(self.api.VerificationError):
                self.verify()

    def test_cli_sanitized_failures_optimization_and_import_safe(self):
        self.require_api()
        archive(self.path)
        command = [sys.executable, "-B", str(SCRIPT)]
        for flags in ([], ["-O"]):
            base = [sys.executable, "-B", *flags, str(SCRIPT)]
            good = subprocess.run(base + [str(self.path), "--expected-version", VERSION],
                                  capture_output=True, text=True, timeout=10)
            self.assertEqual(good.returncode, 0, good.stderr)
            self.assertIn("package checks only", good.stdout)
            bad = subprocess.run(base + [str(self.path), "--expected-version", "0.1.8"],
                                 capture_output=True, text=True, timeout=10)
            self.assertNotEqual(bad.returncode, 0)
            self.assertNotIn("PASS", bad.stdout + bad.stderr)
        for args in (["CANARY/private.zip", "--expected-version", VERSION],
                     [str(self.path), "--expected-version", "CANARY"],
                     ["--CANARY"], [], [str(self.path), "--expected-version", VERSION, "--sha256", "CANARY"]):
            result = subprocess.run(command + args, capture_output=True, text=True, timeout=10)
            self.assertNotEqual(result.returncode, 0)
            self.assertNotIn("CANARY", result.stdout + result.stderr)
            self.assertNotIn("Traceback", result.stdout + result.stderr)
            self.assertNotIn("PASS", result.stdout + result.stderr)
        out, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            spec = importlib.util.spec_from_file_location("import_only", SCRIPT)
            spec.loader.exec_module(importlib.util.module_from_spec(spec))
        self.assertEqual(out.getvalue() + err.getvalue(), "")

    def test_invalid_archive_and_io_errors(self):
        self.require_api()
        self.path.write_bytes(b"CANARY not a zip")
        with self.assertRaises(self.api.VerificationError):
            self.verify()
        self.path.unlink()
        with self.assertRaises(self.api.VerificationError):
            self.verify()
        self.path.mkdir()
        with self.assertRaises(self.api.VerificationError):
            self.verify()


if __name__ == "__main__":
    unittest.main()
