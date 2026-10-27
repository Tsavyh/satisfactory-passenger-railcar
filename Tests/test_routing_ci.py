"""Portable contract for generating the existing routing-only fixtures."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class RoutingCIContract(unittest.TestCase):
    def test_generation_uses_real_sources_without_claiming_execution(self):
        with tempfile.TemporaryDirectory(dir=os.environ.get("TMPDIR")) as scratch:
            output = Path(scratch) / "routing"
            result = subprocess.run(
                [sys.executable, "-B", str(ROOT / "tools/run_routing_checks.py"),
                 "--output", str(output), "--generate-only"],
                capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            report = json.loads((output / "result.json").read_text())
            self.assertEqual(report["status"], "GENERATED_NOT_EXECUTED")
            self.assertEqual(set(report["suites"]), {"derailment", "availability-runtime", "availability-editor-branch"})
            self.assertEqual([report["suites"][name]["expected_cases"] for name in
                              ("derailment", "availability-runtime", "availability-editor-branch")], [150, 23, 3])
            derailment = (output / "derailment.cpp").read_text()
            self.assertIn("ATsabsPassengerRailcar::OnUse_Implementation(", derailment)
            self.assertNotIn("@@PRODUCTION_DEFINITIONS@@", derailment)
            availability = (output / "availability-runtime.cpp").read_text()
            self.assertIn("UTsabsPassengerWorldModule::", availability)
            self.assertNotIn("@@PRODUCTION_", availability)
            self.assertEqual(availability, (output / "availability-editor-branch.cpp").read_text())
            self.assertFalse(list(output.glob("*.exe")))

    def test_routing_result_requires_complete_unique_success(self):
        import importlib.util
        spec = importlib.util.spec_from_file_location("routing_ci", ROOT / "tools/run_routing_checks.py")
        if spec is None or spec.loader is None:
            self.fail("Cannot load the CI runner")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        check = getattr(module, "check_run", None)
        if not callable(check):
            self.fail("Execution results must be validated, not just generated")
        suite = {"expected_cases": 2, "expected_names": ["one", "two"]}
        valid = "CASE PASS one\nCASE PASS two\n"
        self.assertTrue(check(suite, valid, 0))
        for text, exit_code in ((valid, 1), ("", 0), ("CASE PASS one\n", 0),
                                (valid + "CASE PASS extra\n", 0),
                                ("CASE PASS one\nCASE PASS one\n", 0),
                                ("CASE PASS two\nCASE PASS one\n", 0),
                                (valid.replace("PASS two", "FAIL two"), 0),
                                (valid + "ASSERT_FAIL unsafe\n", 0),
                                (valid + "ACTIVE_LIST_UNSUBSCRIBE unsafe\n", 0)):
            with self.subTest(text=text, exit_code=exit_code):
                self.assertFalse(check(suite, text, exit_code))


if __name__ == "__main__":
    unittest.main()
