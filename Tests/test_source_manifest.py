"""Verify the exact published plugin inputs on every portable CI run."""
import hashlib
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class PublishedSourceManifest(unittest.TestCase):
    def test_reviewed_source_hashes_and_complete_source_tree(self):
        manifest = json.loads((ROOT / "SOURCE-MANIFEST.json").read_text(encoding="utf-8"))
        files = manifest["files"]
        self.assertEqual(manifest["tested_plugin_source_files"], len(files))
        for name, expected in files.items():
            with self.subTest(name=name):
                self.assertFalse(Path(name).is_absolute())
                self.assertNotIn("..", Path(name).parts)
                self.assertEqual(hashlib.sha256((ROOT / name).read_bytes()).hexdigest(), expected)
        source_files = {p.relative_to(ROOT).as_posix() for p in (ROOT / "Source").rglob("*") if p.is_file()}
        self.assertEqual(source_files, {name for name in files if name.startswith("Source/")})

    def test_descriptor_and_binary_release_identity(self):
        manifest = json.loads((ROOT / "SOURCE-MANIFEST.json").read_text(encoding="utf-8"))
        descriptor = json.loads((ROOT / "TsabsPassengerWagon.uplugin").read_text(encoding="utf-8"))
        self.assertEqual(manifest["version"], descriptor["VersionName"])
        self.assertEqual(manifest["version"], descriptor["SemVersion"])
        self.assertEqual(manifest["public_binary"]["asset"],
                         "satisfactory-passenger-railcar-" + manifest["version"] + "-windows-steam.zip")


if __name__ == "__main__":
    unittest.main()
