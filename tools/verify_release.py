#!/usr/bin/env python3
"""Bounded, read-only checks for the Windows Steam public release wrapper.

No extraction, installation, network access or execution of packaged code.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import stat
import struct
import sys
import zipfile
import zlib

MIB = 1024 * 1024
MAX_ZIP = 128 * MIB
MAX_MEMBER = 128 * MIB
MAX_TOTAL = 256 * MIB
MAX_JSON = MIB
MAX_ENTRIES = 128
MAX_PE_WORK = 512 * MIB  # Charge decompression revisits caused by backward PE seeks.
CHUNK = 64 * 1024
VERSION = re.compile(r"(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)", re.ASCII)
DIGEST = re.compile(r"[0-9a-f]{64}", re.ASCII)
PREFIX = "VAGON/TsabsPassengerWagon/"
DLL_NAME = "FactoryGameSteam-TsabsPassengerWagon-Win64-Shipping.dll"
DLL = PREFIX + "Binaries/Win64/" + DLL_NAME
MODULES = PREFIX + "Binaries/Win64/FactoryGameSteam-Win64-Shipping.modules"
DESCRIPTOR = PREFIX + "TsabsPassengerWagon.uplugin"
DEPENDENCY = b"FactoryGameSteam-FactoryDedicatedClient-Win64-Shipping.dll"
REQUIRED = {"MANIFEST.json", "LEEME.txt", "LICENSE", DLL, MODULES, DESCRIPTOR} | {
    PREFIX + "Content/Paks/Windows/TsabsPassengerWagonFactoryGame-Windows." + ext
    for ext in ("pak", "ucas", "utoc")}
FAILURE = "FAIL: release package verification failed."
SUCCESS = ("PASS: offline release package checks only; not gameplay, compatibility, "
           "malware or private-data certification.")


class VerificationError(Exception):
    """A fixed-message refusal, never containing untrusted archive data."""

    def __init__(self):
        super().__init__(FAILURE)


def require(condition):
    if not condition:
        raise VerificationError()


def strict_version(value):
    require(type(value) is str and VERSION.fullmatch(value) is not None)
    return value


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result)
        result[key] = value
    return result


def reject_constant(_value):
    raise VerificationError()


def read_json(archive, info):
    require(0 < info.file_size <= MAX_JSON)
    with archive.open(info) as stream:
        data = stream.read(MAX_JSON + 1)
    require(len(data) == info.file_size and len(data) <= MAX_JSON)
    result = json.loads(data.decode("utf-8"), object_pairs_hook=unique_object,
                        parse_constant=reject_constant)
    require(type(result) is dict)
    return result


def safe_name(info):
    name = info.orig_filename  # ZipInfo.filename truncates at NUL: validate original too.
    require(name == info.filename and name and not name.startswith("/"))
    require(not any(ord(char) < 32 or ord(char) == 127 or char in "\\:<>\"|?*"
                    for char in name))
    parts = name[:-1].split("/") if info.is_dir() else name.split("/")
    for part in parts:
        require(part not in ("", ".", "..") and not part.endswith((".", " ")))
        stem = part.split(".")[0].upper()
        require(stem not in {"CON", "PRN", "AUX", "NUL", "CONIN$", "CONOUT$"})
        require(re.fullmatch(r"(?:COM|LPT)[1-9¹²³]", stem) is None)
    # APPNOTE 4.4.2/4.4.15: attributes depend on the made-by host.
    require(info.create_system in (0, 3, 10))  # Supported wrapper encodings only.
    attributes = info.external_attr & 0xFFFF
    if info.create_system == 10:
        # Restricted NTFS native flags: readonly/hidden/system/directory/archive,
        # normal/temporary/compressed/not-content-indexed. No POSIX high word.
        require(not (info.external_attr & ~0x29B7))
    else:
        # DOS low byte (also supported for Unix), with Python's upper-word mode.
        require(not (attributes & ~0xB7))
        mode = stat.S_IFMT(info.external_attr >> 16)
        require(mode in ((0, stat.S_IFDIR) if info.is_dir() else (0, stat.S_IFREG)))
    # Unsupported reparse/device/EFS metadata is refused above; EFS is not the
    # ZIP encryption flag. NORMAL must stand alone and must not mark a directory.
    require(not (attributes & 0x10) or info.is_dir())
    require(not (attributes & 0x80) or (attributes == 0x80 and not info.is_dir()))
    require(not (info.flag_bits & (1 | 0x40)))
    require(info.compress_type in (zipfile.ZIP_STORED, zipfile.ZIP_DEFLATED))
    require(0 <= info.file_size <= MAX_MEMBER and 0 <= info.compress_size <= MAX_ZIP)
    return name


def inventory(archive):
    entries = archive.infolist()
    require(0 < len(entries) <= MAX_ENTRIES)
    names, folded, regular, directories = set(), set(), {}, []
    for info in entries:
        name = safe_name(info)
        require(name not in names and name.casefold() not in folded)
        names.add(name)
        folded.add(name.casefold())
        if info.is_dir():
            require(info.file_size == 0)
            directories.append(name)
        else:
            require(info.file_size > 0)
            regular[name] = info
    require(sum(info.file_size for info in entries) <= MAX_TOTAL)
    profiles = [name for name in regular if re.fullmatch(r"Solo-SML-(.+)\.smmprofile", name)]
    require(len(profiles) <= 1)
    for name in profiles:
        strict_version(name[len("Solo-SML-"):-len(".smmprofile")])
    require(set(regular) == REQUIRED | set(profiles))
    json_names = {"MANIFEST.json", DESCRIPTOR, MODULES} | set(profiles)
    require(all(regular[name].file_size <= MAX_JSON for name in json_names))
    ancestors = {name[:index + 1] for name in regular
                 for index, char in enumerate(name) if char == "/"}
    require(all(name in ancestors for name in directories))
    return regular, profiles


def check_descriptor(value, version):
    require(value.get("VersionName") == version and value.get("SemVersion") == version)
    modules = value.get("Modules")
    require(type(modules) is list and len(modules) == 1)
    module = modules[0]
    require(type(module) is dict and module.get("Name") == "TsabsPassengerWagon"
            and module.get("Type") == "Runtime")
    plugins = value.get("Plugins")
    require(type(plugins) is list)
    sml = []
    for item in plugins:
        require(type(item) is dict)
        if item.get("Name") == "SML":
            sml.append(item)
    require(len(sml) == 1 and sml[0].get("Enabled") is True
            and sml[0].get("Optional", False) is False)
    require(type(sml[0].get("SemVersion")) is str and bool(sml[0]["SemVersion"].strip()))


def check_profile(value, name):
    # Only the observed SML-only profile schema; no arbitrary paths/settings/mods.
    sml_version = name[len("Solo-SML-"):-len(".smmprofile")]
    expected = {
        "profile": {"name": "TsabsRailTest-SML312", "mods": {
            "SML": {"version": "=" + sml_version, "enabled": True}},
            "required_targets": ["Windows"]},
        "lockfile": {"version": 1, "mods": {}},
        "metadata": {"gameVersion": 513885}}
    # Encoded equality retains JSON types (True must not compare equal to 1).
    require(json.dumps(value, sort_keys=True) == json.dumps(expected, sort_keys=True))


class PEReader:
    """Read small ranges directly from a ZipExtFile; do not load the DLL into RAM."""

    def __init__(self, stream, size):
        self.stream, self.size = stream, size
        self.sections = []
        self.headers = 0
        self.position = 0
        self.work = 0

    def bounds(self, offset, length):
        require(0 <= offset <= self.size and 0 <= length <= self.size - offset)

    def read(self, offset, length):
        self.bounds(offset, length)
        self.work += (offset if offset < self.position else offset - self.position) + length
        require(self.work <= MAX_PE_WORK)
        self.stream.seek(offset)
        data = self.stream.read(length)
        self.position = offset + length
        require(len(data) == length)
        return data

    def unpack(self, fmt, offset):
        return struct.unpack(fmt, self.read(offset, struct.calcsize(fmt)))

    def rva(self, address, length=1):
        require(address > 0)
        if address < self.headers:
            require(length <= self.headers - address)
            return address
        candidates = [(raw + address - start, raw_size - (address - start))
                      for start, virtual_size, raw, raw_size in self.sections
                      if start <= address < start + max(virtual_size, raw_size)]
        require(len(candidates) == 1)
        offset, remaining = candidates[0]
        require(0 <= length <= remaining)
        self.bounds(offset, length)
        return offset

    def string(self, address, maximum=256):
        result = bytearray()
        for index in range(maximum):
            char = self.read(self.rva(address + index), 1)[0]
            if char == 0:
                require(bool(result))
                return bytes(result)
            require(32 <= char < 127)
            result.append(char)
        raise VerificationError()

    def thunks(self, address, iat):
        for index in range(4096):
            value, = self.unpack("<Q", self.rva(address + 8 * index, 8))
            iat_value, = self.unpack("<Q", self.rva(iat + 8 * index, 8))
            if value == 0:
                require(index > 0 and iat_value == 0)
                return
            require(iat_value != 0)
            if value & (1 << 63):
                require(value & ~((1 << 63) | 0xFFFF) == 0)
            else:
                self.rva(value, 2)  # import-by-name hint then NUL-terminated symbol
                self.string(value + 2, 4096)
        raise VerificationError()

    def check(self):
        require(self.read(0, 2) == b"MZ")
        pe, = self.unpack("<I", 0x3C)
        require(pe >= 64 and self.read(pe, 4) == b"PE\0\0")
        machine, count, _time, _symbols, _number, optional_size, flags = self.unpack("<HHIIIHH", pe + 4)
        require(machine == 0x8664 and flags & 0x2000 and 0 < count <= 96)
        optional = pe + 24
        require(optional_size >= 128 and self.unpack("<H", optional)[0] == 0x20B)
        self.read(optional, optional_size)
        self.headers, = self.unpack("<I", optional + 60)
        directory_count, = self.unpack("<I", optional + 108)
        require(2 <= directory_count <= 16 and optional_size >= 112 + directory_count * 8)
        section_table = optional + optional_size
        require(section_table + count * 40 <= self.headers <= self.size)
        for index in range(count):
            virtual_size, start, raw_size, raw = self.unpack("<IIII", section_table + index * 40 + 8)
            require(start >= self.headers and start + max(virtual_size, raw_size) <= 0x100000000)
            require(raw_size == 0 or raw >= self.headers)
            self.bounds(raw, raw_size)
            for previous, previous_size, previous_raw, previous_raw_size in self.sections:
                require(start + max(virtual_size, raw_size) <= previous or
                        previous + max(previous_size, previous_raw_size) <= start)
                require(not raw_size or not previous_raw_size or raw + raw_size <= previous_raw or
                        previous_raw + previous_raw_size <= raw)
            self.sections.append((start, virtual_size, raw, raw_size))
        imports, import_size = self.unpack("<II", optional + 120)
        require(40 <= import_size <= MAX_JSON and import_size % 20 == 0)
        self.rva(imports, import_size)
        found, terminated = False, False
        for index in range(import_size // 20):
            original, timestamp, forwarder, name, first = self.unpack("<IIIII", self.rva(imports + index * 20, 20))
            if not any((original, timestamp, forwarder, name, first)):
                terminated = True
                break
            require(first != 0)
            dependency = self.string(name)
            # Bound IAT values need not be import-by-name RVAs, but its complete
            # array must still be present and terminate alongside the lookup table.
            self.thunks(original or first, first)
            found = found or dependency.lower() == DEPENDENCY.lower()
        require(terminated and found)


def _verify(path, expected_version, sha256):
    version = strict_version(expected_version)
    require(sha256 is None or (type(sha256) is str and DIGEST.fullmatch(sha256) is not None))
    path = Path(path)
    require(path.name == "satisfactory-passenger-railcar-" + version + "-windows-steam.zip")
    flags = os.O_RDONLY | getattr(os, "O_BINARY", 0) | getattr(os, "O_NONBLOCK", 0) | getattr(os, "O_NOFOLLOW", 0)
    descriptor = os.open(path, flags)
    with os.fdopen(descriptor, "rb") as stream:
        size = os.fstat(stream.fileno())
        require(stat.S_ISREG(size.st_mode) and 0 < size.st_size <= MAX_ZIP)
        if sha256 is not None:
            digest = hashlib.sha256()
            while chunk := stream.read(CHUNK):
                digest.update(chunk)
            require(digest.hexdigest() == sha256)
            stream.seek(0)
        with zipfile.ZipFile(stream) as archive:
            regular, profiles = inventory(archive)
            manifest = read_json(archive, regular["MANIFEST.json"])
            require(manifest.get("version") == version)
            hashes = manifest.get("files")
            require(type(hashes) is dict and set(hashes) == set(regular) - {"MANIFEST.json"})
            require(all(type(value) is str and DIGEST.fullmatch(value) is not None for value in hashes.values()))
            for name, expected in hashes.items():
                digest, total = hashlib.sha256(), 0
                with archive.open(regular[name]) as member:
                    while chunk := member.read(CHUNK):
                        total += len(chunk)
                        require(total <= regular[name].file_size and total <= MAX_MEMBER)
                        digest.update(chunk)
                require(total == regular[name].file_size and digest.hexdigest() == expected)
            check_descriptor(read_json(archive, regular[DESCRIPTOR]), version)
            modules = read_json(archive, regular[MODULES])
            require(modules.get("BuildId") == "SML" and modules.get("Modules") == {"TsabsPassengerWagon": DLL_NAME})
            for name in profiles:
                check_profile(read_json(archive, regular[name]), name)
            with archive.open(regular[DLL]) as member:
                PEReader(member, regular[DLL].file_size).check()


def verify_release(path, expected_version, sha256=None):
    """Return None on success, or raise a sanitized VerificationError."""
    try:
        _verify(path, expected_version, sha256)
    except VerificationError:
        raise
    except (OSError, ValueError, TypeError, KeyError, OverflowError, RecursionError,
            zipfile.BadZipFile, RuntimeError, NotImplementedError, EOFError,
            struct.error, zlib.error):
        raise VerificationError() from None


class SafeParser(argparse.ArgumentParser):
    def error(self, message):
        self.exit(2, FAILURE + "\n")


def main(argv=None):
    parser = SafeParser(description="Offline Windows Steam wrapper checks; never extracts or executes payloads.")
    parser.add_argument("zip", metavar="ZIP")
    parser.add_argument("--expected-version", required=True, metavar="VERSION")
    parser.add_argument("--sha256", metavar="EXPECTED_HEX")
    args = parser.parse_args(argv)
    try:
        verify_release(args.zip, args.expected_version, args.sha256)
    except VerificationError:
        print(FAILURE, file=sys.stderr)
        return 1
    print(SUCCESS)
    return 0


if __name__ == "__main__":
    sys.exit(main())
