#!/usr/bin/env python3
"""Run existing production-routing fixtures without Unreal or a local SDK.

Routing boundaries are instrumented: this does NOT test native RPCs, physics,
real detours, multiplayer, gameplay, or actual Unreal Editor execution.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def generate(output):
    output.mkdir(parents=True, exist_ok=False)
    spec = importlib.util.spec_from_file_location(
        "derailment_routing", ROOT / "Tests/DerailmentRouting/run_routing.py")
    if spec is None or spec.loader is None:
        raise RuntimeError("Cannot load the existing production-definition extractor")
    extractor = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(extractor)
    source = ROOT / "Source/TsabsPassengerWagon/Private/TsabsPassengerRailcar.cpp"
    template = (ROOT / "Tests/DerailmentRouting/RoutingHarness.cpp.in").read_text(encoding="utf-8")
    definitions = extractor.definitions(source.read_bytes().decode("utf-8-sig"))
    code = template.replace("@@PRODUCTION_DEFINITIONS@@", "\n\n".join(definitions.values()))
    (output / "derailment.cpp").write_text(code, encoding="utf-8")
    report = {"status": "GENERATED_NOT_EXECUTED", "scope": __doc__, "suites": {
        "derailment": {"expected_cases": 150, "editor": 0}}}
    template = (ROOT / "Tests/AvailabilityRouting/AvailabilityHarness.cpp.in").read_text(encoding="utf-8")
    files = {
        "POLICY": "Source/TsabsPassengerWagon/Public/PassengerContentPolicy.h",
        "HEADER": "Source/TsabsPassengerWagon/Public/TsabsPassengerWorldModule.h",
        "PRODUCTION": "Source/TsabsPassengerWagon/Private/TsabsPassengerWorldModule.cpp",
        "WAGON": "Source/TsabsPassengerWagon/Private/TsabsPassengerWagonModule.cpp",
    }
    code = template
    for marker, name in files.items():
        body = re.sub(r"^#include[^\r\n]*(?:\r?\n|$)", "",
                      (ROOT / name).read_bytes().decode("utf-8-sig"), flags=re.M)
        if code.count("@@" + marker + "@@") != 1:
            raise ValueError("Missing or repeated source marker: " + marker)
        code = code.replace("@@" + marker + "@@", body)
    if "@@" in code:
        raise ValueError("Unresolved source marker")
    main = template.split("int main(){", 1)[1]
    common, branches = main.split("#if WITH_EDITOR", 1)
    editor, runtime = branches.split("#else", 1)
    runtime = runtime.split("#endif", 1)[0]
    for name, enabled, branch, count in (
            ("availability-runtime", 0, runtime, 23),
            ("availability-editor-branch", 1, editor, 3)):
        expected = re.findall(r' Case\("([^\"]+)"', common + branch)
        if len(expected) != count or len(expected) != len(set(expected)):
            raise ValueError("Unexpected or duplicated fixture cases: " + name)
        (output / (name + ".cpp")).write_text(code, encoding="utf-8")
        report["suites"][name] = {"expected_cases": count, "expected_names": expected, "editor": enabled}
    for name, suite in report["suites"].items():
        suite["generated_sha256"] = hashlib.sha256((output / (name + ".cpp")).read_bytes()).hexdigest()
    return report


def check_run(suite, text, exit_code):
    cases = re.findall(r"^CASE (PASS|FAIL) (.+)$", text, re.M)
    names = [name for state, name in cases]
    return (exit_code == 0 and len(cases) == suite["expected_cases"]
            and len(names) == len(set(names))
            and all(state == "PASS" for state, name in cases)
            and ("expected_names" not in suite or names == suite["expected_names"])
            and not re.search(r"^(ASSERT_FAIL|ACTIVE_LIST_UNSUBSCRIBE) ", text, re.M))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--generate-only", action="store_true")
    args = parser.parse_args()
    report = generate(args.output)
    if not args.generate_only:
        for name, suite in report["suites"].items():
            exe = args.output / (name + ".exe")
            command = ["cl.exe", "/nologo", "/std:c++20", "/EHsc", "/utf-8", "/W3",
                       "/DWITH_EDITOR=" + str(suite["editor"]), "/Fe:" + str(exe),
                       "/Fo:" + str(args.output / (name + ".obj")),
                       str(args.output / (name + ".cpp"))]
            suite["passed"] = False
            try:
                build = subprocess.run(command, capture_output=True, text=True,
                                       encoding="utf-8", errors="replace", timeout=180)
                (args.output / (name + ".compile.log")).write_text(build.stdout + build.stderr, encoding="utf-8")
                suite["compile_exit"] = build.returncode
                if build.returncode == 0:
                    run = subprocess.run([str(exe)], capture_output=True, text=True,
                                         encoding="utf-8", errors="replace", timeout=60)
                    text = run.stdout + run.stderr
                    (args.output / (name + ".run.log")).write_text(text, encoding="utf-8")
                    suite.update(run_exit=run.returncode,
                                 observed_cases=len(re.findall(r"^CASE (?:PASS|FAIL) ", text, re.M)),
                                 passed=check_run(suite, text, run.returncode))
            except (OSError, subprocess.TimeoutExpired) as error:
                suite["execution_error"] = type(error).__name__
        report["status"] = "PASS" if all(suite["passed"] for suite in report["suites"].values()) else "FAIL"
    (args.output / "result.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(json.dumps(report, indent=2))
    return 1 if report["status"] == "FAIL" else 0


if __name__ == "__main__":
    raise SystemExit(main())
