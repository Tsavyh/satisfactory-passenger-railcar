"""Compile exact production method definitions against catalogued ROUTING-ONLY boundaries.
No Unreal/SDK patching; no physics/RCO/RPC emulation or native-success assertion.
Every label is immutable. Outputs and source archives live only in Setup/PassengerDerailment0114.
"""
from pathlib import Path
import datetime
import hashlib
import json
import re
import shutil
import subprocess
import sys
import zipfile

ROOT = Path('C:/Modding/Satisfactory')
PLUGIN = ROOT / 'StarterProject/Mods/TsabsPassengerWagon'
EVIDENCE = ROOT / 'Setup/PassengerDerailment0114'
METHODS = ('OnUse_Implementation', 'UpdateUseState_Implementation', 'GetLookAtDecription_Implementation')

def sha(data):
    return hashlib.sha256(data).hexdigest()

def definitions(text):
    """Lex braces outside strings/comments; retain COMPLETE definitions byte-for-byte as UTF-8 text."""
    extracted = {}
    for method in METHODS:
        match = re.search(r'^(?:void|FText) ATsabsPassengerRailcar::' + method + r'\(', text, re.M)
        if not match:
            raise RuntimeError('Missing unique production method ' + method)
        if len(re.findall(r'ATsabsPassengerRailcar::' + method + r'\(', text)) != 1:
            raise RuntimeError('Ambiguous production method ' + method)
        start = text.index('{', match.start())
        depth, state, i = 0, 'code', start
        while i < len(text):
            c, pair = text[i], text[i:i+2]
            if state == 'line':
                if c == '\n': state = 'code'
            elif state == 'block':
                if pair == '*/': state = 'code'; i += 1
            elif state in ('"', "'"):
                if c == '\\': i += 1
                elif c == state: state = 'code'
            elif pair == '//': state = 'line'; i += 1
            elif pair == '/*': state = 'block'; i += 1
            elif c in ('"', "'"): state = c
            elif c == '{': depth += 1
            elif c == '}':
                depth -= 1
                if depth == 0:
                    extracted[method] = text[match.start():i+1]
                    break
            i += 1
        else:
            raise RuntimeError('Unbalanced production method ' + method)
    return extracted

def main():
    label = sys.argv[1]
    if not re.fullmatch(r'passenger-derail0114-routing-(?:red|green)\d{2}', label):
        raise SystemExit('Use the agreed unique RED/GREEN label')
    output = EVIDENCE / label
    output.mkdir(parents=True, exist_ok=False)
    # Snapshot before compile/run. Only source inputs, never binaries/intermediates.
    inputs = sorted([PLUGIN / 'TsabsPassengerWagon.uplugin'] +
                    [p for directory in ('Source', 'Tests') for p in (PLUGIN / directory).rglob('*') if p.is_file()])
    hashes = {p.relative_to(PLUGIN).as_posix(): sha(p.read_bytes()) for p in inputs}
    (output / 'SourceHashes.json').write_text(json.dumps(hashes, indent=2), encoding='utf-8')
    archive = output / ('red-source.zip' if '-red' in label else 'green-source.zip')
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
        for p in inputs: z.write(p, p.relative_to(PLUGIN).as_posix())
    with zipfile.ZipFile(archive) as z:
        crc = z.testzip()
        assert crc is None
        assert {n: sha(z.read(n)) for n in z.namelist()} == hashes
    source = PLUGIN / 'Source/TsabsPassengerWagon/Private/TsabsPassengerRailcar.cpp'
    # Decode bytes directly to retain original line endings within definitions.
    bodies = definitions(source.read_bytes().decode('utf-8-sig'))
    for method, body in bodies.items():
        (output / (method + '.cpp')).write_bytes(body.encode('utf-8'))
    template = Path(__file__).with_name('RoutingHarness.cpp.in')
    code = template.read_text(encoding='utf-8').replace('@@PRODUCTION_DEFINITIONS@@', '\n\n'.join(bodies.values()))
    generated = output / 'RoutingHarness.cpp'
    generated.write_text(code, encoding='utf-8')
    vcvars = Path('C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat')
    exe = output / 'routing.exe'
    bat = output / 'compile.cmd'
    bat.write_text('@call "' + str(vcvars) + '"\n@if errorlevel 1 exit /b %errorlevel%\n'
                   'cl.exe /nologo /std:c++20 /EHsc /utf-8 /W3 /Fe:"' + str(exe) + '" /Fo:"'
                   + str(output / 'routing.obj') + '" "' + str(generated) + '"\n@exit /b %errorlevel%\n', encoding='utf-8')
    build = subprocess.run(['cmd.exe', '/d', '/c', str(bat)], capture_output=True, encoding='utf-8', errors='replace', timeout=180)
    (output / 'compile.log').write_text(build.stdout + build.stderr, encoding='utf-8')
    result = {
        'label': label, 'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'scope': 'ROUTING ONLY: exact three complete production definitions, instrumented native/feedback/boarding boundaries; NOT native RPC/rerail success',
        'source_sha256': sha(source.read_bytes()), 'template_sha256': sha(template.read_bytes()),
        'runner_sha256': sha(Path(__file__).read_bytes()),
        'definition_sha256': {k: sha(v.encode('utf-8')) for k, v in bodies.items()},
        'source_count': len(hashes), 'archive': str(archive), 'archive_sha256': sha(archive.read_bytes()),
        'archive_crc_ok': crc is None, 'archive_members_match_manifest': True,
        'compile_exit': build.returncode, 'command': ['cmd.exe', '/d', '/c', str(bat)],
    }
    if build.returncode == 0:
        run = subprocess.run([str(exe)], capture_output=True, encoding='utf-8', errors='replace', timeout=60)
        (output / 'routing.log').write_text(run.stdout + run.stderr, encoding='utf-8')
        cases = re.findall(r'^CASE (PASS|FAIL) (.+)$', run.stdout, re.M)
        failures = re.findall(r'^ASSERT_FAIL (.+)$', run.stdout, re.M)
        result.update(run_exit=run.returncode, cases=[{'name': n, 'state': s} for s, n in cases],
                      case_count=len(cases), passed=sum(s == 'PASS' for s, _ in cases),
                      failed=sum(s == 'FAIL' for s, _ in cases), assertion_failures=failures)
    (output / 'result.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({k: v for k, v in result.items() if k not in ('cases', 'assertion_failures')}, indent=2))
    if result.get('assertion_failures'):
        print('\n'.join(result['assertion_failures'][:8]))
    return build.returncode or result.get('run_exit', 2)

if __name__ == '__main__':
    raise SystemExit(main())
