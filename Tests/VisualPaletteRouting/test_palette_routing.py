"""Test exact production color forwarding at instrumented routing boundaries only."""
from pathlib import Path
import hashlib, importlib.util, json, re, subprocess, sys
ROOT=Path('C:/Modding/Satisfactory')
PLUGIN=ROOT/'StarterProject/Mods/TsabsPassengerWagon'
BASE=ROOT/'Research/Visual/PrivateIntegration01/PaletteRouting'
OLD=PLUGIN/'Tests/DerailmentRouting/run_routing.py'
SOURCE=PLUGIN/'Source/TsabsPassengerWagon/Private/TsabsPassengerRailcar.cpp'
TEMPLATE=Path(__file__).with_name('PaletteHarness.cpp.in')
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    label=sys.argv[1]
    if not re.fullmatch(r'(RED|GREEN)[0-9]{2}',label):raise ValueError('unique RED/GREEN label required')
    out=BASE/label;out.mkdir(parents=True,exist_ok=False)
    report={'passed':False,'scope':'Exact production callback routing only; mock native function does not verify game palette, materials or network', 'source_sha256':sha(SOURCE),'template_sha256':sha(TEMPLATE),'test_sha256':sha(Path(__file__))}
    try:
        text=SOURCE.read_bytes().decode('utf-8-sig')
        match=re.search(r'^void ATsabsPassengerRailcar::OnCustomizationDataApplied\(',text,re.M)
        assert match is not None, 'Expected production customization callback is absent'
        spec=importlib.util.spec_from_file_location('existing_extractor',OLD)
        assert spec is not None and spec.loader is not None, 'Production extractor could not load'
        module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);setattr(module,'METHODS',('OnCustomizationDataApplied',))
        body=getattr(module,'definitions')(text)['OnCustomizationDataApplied'];(out/'production-definition.cpp').write_bytes(body.encode('utf-8'))
        code=TEMPLATE.read_text(encoding='utf-8').replace('@@PRODUCTION_DEFINITION@@',body);cpp=out/'palette-test.cpp';cpp.write_text(code,encoding='utf-8');exe=out/'palette-test.exe'
        vc=Path('C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat');bat=out/'compile.cmd';bat.write_text('@call "'+str(vc)+'"\n@if errorlevel 1 exit /b %errorlevel%\ncl.exe /nologo /std:c++20 /EHsc /utf-8 /Fe:"'+str(exe)+'" /Fo:"'+str(out/'palette-test.obj')+'" "'+str(cpp)+'"\n@exit /b %errorlevel%\n')
        build=subprocess.run(['cmd.exe','/d','/c',str(bat)],capture_output=True,text=True,errors='replace',timeout=180);(out/'build.log').write_text(build.stdout+build.stderr,encoding='utf-8');report['build_exit']=build.returncode;assert build.returncode==0,'Harness compilation failed'
        run=subprocess.run([str(exe)],capture_output=True,text=True,errors='replace',timeout=60);(out/'test.log').write_text(run.stdout+run.stderr,encoding='utf-8');cases=re.findall(r'^CASE PASS flags=(\d+)$',run.stdout,re.M);report.update(run_exit=run.returncode,cases=cases);assert run.returncode==0 and cases==['0','1','2','3'],'Routing cases failed';report['passed']=True
    except Exception as e:report['failure']=str(e)
    (out/'result.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(report,indent=2));return 0 if report['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
