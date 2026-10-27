#!/usr/bin/env python3
"""Create a verified Windows Steam wrapper, without installing or executing it."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import stat
import sys
import tempfile
import zipfile
import zlib

# Resolve the sibling verifier independently of the caller's working directory.
ROOT = Path(__file__).resolve().parents[1]
_spec = importlib.util.spec_from_file_location("_packager_verifier", ROOT / "tools" / "verify_release.py")
verifier = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(verifier)

FAILURE = "FAIL: release packaging failed; use --help for requirements."
SUCCESS = "PASS: verified release created; offline package checks only, not gameplay validation."
PAYLOADS = sorted(name for name in verifier.REQUIRED if name.startswith(verifier.PREFIX))


class PackagingError(Exception):
    """A fixed-message refusal without input paths or contents."""

    def __init__(self):
        super().__init__(FAILURE)


def require(condition):
    if not condition:
        raise PackagingError()


def _path(value):
    path = Path(value)
    # Do not normalize away a potentially linked ancestor followed by '..'.
    require(".." not in path.parts)
    return Path(os.path.abspath(path))


def _metadata(path, directory=False):
    """Check only selected paths and ancestor metadata, never enumerate input."""
    for parent in reversed(path.parents):
        info = parent.lstat()
        require(stat.S_ISDIR(info.st_mode) and not _linked(info))
    info = path.lstat()
    require(not _linked(info))
    require(stat.S_ISDIR(info.st_mode) if directory else stat.S_ISREG(info.st_mode))
    return info


def _linked(info):
    # Includes Windows junctions and other reparse points, not just symlinks.
    return stat.S_ISLNK(info.st_mode) or bool(
        getattr(info, "st_file_attributes", 0) & getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0x400))


def _stamp(info):
    # CPython 3.12 on Windows reports creation time via lstat's ctime but
    # change time via fstat's ctime. Birth time agrees across both APIs.
    timestamp = info.st_birthtime_ns if sys.platform == "win32" else info.st_ctime_ns
    return (info.st_dev, info.st_ino, info.st_size, info.st_mtime_ns, timestamp)


def _open_input(path, expected):
    require(_stamp(_metadata(path)) == _stamp(expected))
    flags = os.O_RDONLY | getattr(os, "O_BINARY", 0) | getattr(os, "O_NONBLOCK", 0) | getattr(os, "O_NOFOLLOW", 0)
    fd = os.open(path, flags)
    try:
        current = os.fstat(fd)
        require(stat.S_ISREG(current.st_mode) and not _linked(current)
                and _stamp(current) == _stamp(expected))
        return os.fdopen(fd, "rb")
    except BaseException:
        os.close(fd)
        raise


def _unchanged(path, stream, expected):
    require(_stamp(os.fstat(stream.fileno())) == _stamp(expected)
            and _stamp(_metadata(path)) == _stamp(expected))


def _small_json(path, expected):
    with _open_input(path, expected) as stream:
        data = stream.read(verifier.MAX_JSON + 1)
        require(len(data) == expected.st_size and len(data) <= verifier.MAX_JSON)
        _unchanged(path, stream, expected)
    value = json.loads(data.decode("utf-8"), object_pairs_hook=verifier.unique_object,
                       parse_constant=verifier.reject_constant)
    require(type(value) is dict)
    return value


def _copy_input(source, target, expected):
    """Hash exactly the bounded bytes copied; notice cooperative source changes."""
    total, digest = 0, hashlib.sha256()
    with _open_input(source, expected) as incoming, target.open("xb") as outgoing:
        while chunk := incoming.read(verifier.CHUNK):
            total += len(chunk)
            require(total <= expected.st_size and total <= verifier.MAX_MEMBER)
            outgoing.write(chunk)
            digest.update(chunk)
        require(total == expected.st_size)
        _unchanged(source, incoming, expected)
    return digest.hexdigest()


def _encoded(value):
    return (json.dumps(value, sort_keys=True, ensure_ascii=True, indent=2) + "\n").encode("utf-8")


def _readme(descriptor, version):
    sml = next(item["SemVersion"] for item in descriptor["Plugins"] if item["Name"] == "SML")
    game = descriptor.get("GameVersion")
    # Requirements are short version expressions, never arbitrary descriptor prose.
    expression = re.compile(r"[0-9 .<>=~^*+|,-]{1,128}", re.ASCII)
    require(expression.fullmatch(sml) is not None and re.search(r"[0-9]", sml) is not None)
    require(game is None or (type(game) is str and expression.fullmatch(game) is not None
                             and re.search(r"[0-9]", game) is not None))
    game = game if game is not None else "not specified / no indicada"
    return (
        f"Satisfactory Passenger Railcar {version} - Windows Steam\n"
        f"Descriptor requirements / Requisitos del descriptor: game/juego {game}; SML {sml}.\n\n"
        "EN: Close the game and back up saves and the existing plugin before installation.\n"
        "Install compatible SML separately; it is not included. Copy VAGON/TsabsPassengerWagon\n"
        "to FactoryGame/Mods/TsabsPassengerWagon, without an extra nested directory.\n"
        "Host and clients must use the same plugin version and compatible dependencies.\n"
        "This wrapper passed offline package checks only, not game startup, gameplay,\n"
        "multiplayer, compatibility, malware or private-data certification.\n\n"
        "ES: Cierra el juego y guarda una copia de partidas y del plugin antes de instalar.\n"
        "Instala SML compatible por separado; no se incluye. Copia VAGON/TsabsPassengerWagon\n"
        "a FactoryGame/Mods/TsabsPassengerWagon, sin anidar otra carpeta.\n"
        "Host y clientes deben usar la misma version del plugin y dependencias compatibles.\n"
        "Solo se comprueba el paquete offline, no arranque, juego, multijugador,\n"
        "compatibilidad, ausencia de malware ni de datos privados.\n"
    ).encode("utf-8")


def _zip(path, members):
    with zipfile.ZipFile(path, "x", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name in sorted(members):
            info = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
            info.create_system = 3
            info.external_attr = 0o100644 << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            info._compresslevel = 9
            source = members[name]
            size = source.stat().st_size
            require(0 < size <= verifier.MAX_MEMBER)
            total = 0
            with source.open("rb") as incoming, archive.open(info, "w") as outgoing:
                while chunk := incoming.read(verifier.CHUNK):
                    total += len(chunk)
                    require(total <= size)
                    outgoing.write(chunk)
                require(total == size)
            require(path.stat().st_size <= verifier.MAX_ZIP)
    require(path.stat().st_size <= verifier.MAX_ZIP)


def _checksum(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        total = 0
        while chunk := stream.read(verifier.CHUNK):
            total += len(chunk)
            require(total <= verifier.MAX_ZIP)
            digest.update(chunk)
    return digest.hexdigest()


def _publish(stage, output, names):
    # mkdir is the ownership boundary: a preexisting/race-winning dir is untouched.
    output.mkdir(exist_ok=False)
    owned = []
    try:
        for name in names:
            destination = output / name
            with destination.open("xb") as outgoing:
                owned.append(destination)
                with (stage / name).open("rb") as incoming:
                    while chunk := incoming.read(verifier.CHUNK):
                        outgoing.write(chunk)
    except Exception:
        for destination in reversed(owned):
            destination.unlink()
        output.rmdir()
        raise


def _build(plugin_dir, output):
    plugin_dir, output = _path(plugin_dir), _path(output)
    _metadata(plugin_dir, directory=True)
    _metadata(output.parent, directory=True)
    require(not os.path.lexists(output))
    require(not output.is_relative_to(plugin_dir) and not plugin_dir.is_relative_to(output))
    require(not output.is_relative_to(ROOT) and not ROOT.is_relative_to(output))

    inputs = {name: plugin_dir / name.removeprefix(verifier.PREFIX) for name in PAYLOADS}
    inputs["LICENSE"] = ROOT / "LICENSE"
    metadata = {}
    for name, path in inputs.items():
        info = _metadata(path)
        limit = verifier.MAX_JSON if name in (verifier.DESCRIPTOR, verifier.MODULES, "LICENSE") else verifier.MAX_MEMBER
        require(0 < info.st_size <= limit)
        metadata[name] = info
    require(sum(info.st_size for info in metadata.values()) <= verifier.MAX_TOTAL)
    descriptor = _small_json(inputs[verifier.DESCRIPTOR], metadata[verifier.DESCRIPTOR])
    version = verifier.strict_version(descriptor.get("VersionName"))
    verifier.check_descriptor(descriptor, version)
    readme = _readme(descriptor, version)
    basename = f"satisfactory-passenger-railcar-{version}-windows-steam.zip"

    with tempfile.TemporaryDirectory(prefix=".railcar-package-", dir=output.parent) as temporary:
        stage = Path(temporary)
        members, hashes = {}, {}
        for index, name in enumerate(sorted(inputs)):
            target = stage / f"input-{index}"
            hashes[name] = _copy_input(inputs[name], target, metadata[name])
            members[name] = target
        members["LEEME.txt"] = stage / "readme"
        members["LEEME.txt"].write_bytes(readme)
        hashes["LEEME.txt"] = hashlib.sha256(readme).hexdigest()
        manifest = _encoded({"version": version, "license_own_code": "MIT", "files": hashes})
        require(len(manifest) <= verifier.MAX_JSON)
        members["MANIFEST.json"] = stage / "manifest"
        members["MANIFEST.json"].write_bytes(manifest)
        require(sum(path.stat().st_size for path in members.values()) <= verifier.MAX_TOTAL)
        archive = stage / basename
        _zip(archive, members)
        digest = _checksum(archive)
        verifier.verify_release(archive, version, digest)
        (stage / "SHA256SUMS.txt").write_bytes(f"{digest}  {basename}\n".encode("ascii"))
        _publish(stage, output, (basename, "SHA256SUMS.txt"))
    return output / basename


def build_release(plugin_dir, output):
    """Return the ZIP Path in a NEW output directory, or raise PackagingError.

    Reads only six allowlisted cooked plugin files and this repository's LICENSE.
    Ordinary failures clean owned output; interruption is not crash-atomic.
    """
    try:
        return _build(plugin_dir, output)
    except PackagingError:
        raise
    except (verifier.VerificationError, OSError, ValueError, TypeError, KeyError,
            OverflowError, RecursionError, RuntimeError, EOFError, zipfile.BadZipFile,
            zlib.error):
        raise PackagingError() from None


class SafeParser(argparse.ArgumentParser):
    def error(self, message):
        self.exit(2, FAILURE + "\n")


def main(argv=None):
    parser = SafeParser(description=(
        "Package an unpacked cooked Windows Steam plugin (not a source checkout). "
        "Python 3.12; fixed allowlist, offline verification, no installation. / "
        "Empaqueta un plugin cocinado, sin instalarlo."))
    parser.add_argument("plugin_dir", metavar="PLUGIN_DIR", help="root containing TsabsPassengerWagon.uplugin")
    parser.add_argument("--output", required=True, metavar="NEW_DIRECTORY",
                        help="new directory only; real existing parent; never overwrite / no sobrescribir")
    args = parser.parse_args(argv)
    try:
        build_release(args.plugin_dir, args.output)
    except PackagingError:
        print(FAILURE, file=sys.stderr)
        return 1
    print(SUCCESS)
    return 0


if __name__ == "__main__":
    sys.exit(main())
