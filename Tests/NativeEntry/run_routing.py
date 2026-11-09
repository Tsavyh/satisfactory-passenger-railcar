"""Exact own production entry definitions; catalogued data/native routing boundaries.
Not Unreal, save deserialization, network, possession or collision acceptance.
"""
from pathlib import Path
import argparse,datetime,hashlib,json,os,re,shutil,subprocess,tempfile,zipfile

PLUGIN=Path(__file__).resolve().parents[2]
PUBLIC=PLUGIN/'Source/TsabsPassengerWagon/Public'
PRIVATE=PLUGIN/'Source/TsabsPassengerWagon/Private'
DEFINITIONS=[('TsabsPassengerAnchor.cpp','ATsabsPassengerRailcar* ATsabsPassengerAnchor::GetPassengerRailcar() const'),
 ('TsabsPassengerAnchor.cpp','bool ATsabsPassengerAnchor::CanDriverEnter(AFGCharacterPlayer* Character)'),
 ('TsabsPassengerAnchor.cpp','bool ATsabsPassengerAnchor::DriverEnter(AFGCharacterPlayer* Character)'),
 ('TsabsPassengerRailcar.cpp','bool ATsabsPassengerRailcar::IsPassengerAnchor(const ATsabsPassengerAnchor* Anchor) const'),
 ('TsabsPassengerRailcar.cpp','bool ATsabsPassengerRailcar::IsBoardingCharacterValid(AFGCharacterPlayer* Character) const'),
 ('TsabsPassengerRailcar.cpp','bool ATsabsPassengerRailcar::CanReachBoardingBody(AFGCharacterPlayer* Character) const'),
 ('TsabsPassengerRailcar.cpp','bool ATsabsPassengerRailcar::IsBoardingReservationValid(AFGCharacterPlayer* Character, const ATsabsPassengerAnchor* Anchor) const')]
def sha(data): return hashlib.sha256(data).hexdigest()
def complete_definition(text,signature):
    start=text.find(signature+'\n{')
    if start<0 or text.find(signature+'\n{',start+1)>=0: raise RuntimeError('Missing/ambiguous signature: '+signature)
    i=text.index('{',start);depth=0;state='code'
    while i<len(text):
        c=text[i];pair=text[i:i+2]
        if state=='line':
            if c=='\n': state='code'
        elif state=='block':
            if pair=='*/': state='code';i+=1
        elif state in ('"',"'"):
            if c=='\\':i+=1
            elif c==state:state='code'
        elif pair=='//':state='line';i+=1
        elif pair=='/*':state='block';i+=1
        elif c in ('"',"'"):state=c
        elif c=='{':depth+=1
        elif c=='}':
            depth-=1
            if depth==0:return text[start:i+1]
        i+=1
    raise RuntimeError('Unbalanced definition: '+signature)
def inputs():
    files=[PLUGIN/'TsabsPassengerWagon.uplugin']+[f for d in ('Source','Tests','Config','Content','Resources') for f in (PLUGIN/d).rglob('*') if f.is_file() and '__pycache__' not in f.parts and f.suffix!='.pyc']
    return {f.relative_to(PLUGIN).as_posix():f.read_bytes() for f in sorted(files)}
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--out',required=True);parser.add_argument('--expect',choices=('red','green'),default='green');args=parser.parse_args();out=Path(args.out);out.mkdir(parents=True,exist_ok=False)
    source=inputs();hashes={n:sha(b) for n,b in source.items()}
    with zipfile.ZipFile(out/'source.zip','x',zipfile.ZIP_DEFLATED) as z:
        for n,b in source.items():z.writestr(n,b)
    with zipfile.ZipFile(out/'source.zip') as z:
        assert z.testzip() is None and {n:sha(z.read(n)) for n in z.namelist()}==hashes
    bodies={signature:complete_definition((PRIVATE/file).read_text(encoding='utf-8-sig'),signature) for file,signature in DEFINITIONS}
    header=(PUBLIC/'TsabsPassengerRailcar.h').read_text(encoding='utf-8-sig');inline=[l.strip() for l in header.splitlines() if 'bool IsBoardingTransactionActive() const' in l];assert len(inline)==1
    template=Path(__file__).with_name('EntryHarness.cpp.in');code=template.read_text().replace('@@PRODUCTION_DEFINITIONS@@','\n\n'.join(bodies.values())).replace('@@TRANSACTION_INLINE@@',inline[0]);(out/'EntryHarness.cpp').write_text(code,encoding='utf-8')
    record={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'scope':__doc__,'expectation':args.expect,'source_sha256':hashes,'definition_sha256':{n:sha(b.encode()) for n,b in bodies.items()},'transaction_inline_sha256':sha(inline[0].encode()),'template_sha256':sha(template.read_bytes()),'runner_sha256':sha(Path(__file__).read_bytes()),'reservation_header_sha256':sha((PUBLIC/'PassengerBoardingReservation.h').read_bytes()),'boundaries':['UObject validity/weak pointer representation and native Actor/Character/Controller getters use labelled data','native Super CanDriverEnter/DriverEnter permission/return/counters only, NO player-state mutation','unchanged preflight exit helper is refusal/count boundary; real safe exit tested separately in Engine/game','complete7own definitions + own actual reservation header + actual inline transaction query are compiled unchanged']}
    scratch=os.environ.get('TMPDIR') or os.environ.get('TEMP')
    with tempfile.TemporaryDirectory(prefix='passenger-native-entry-',dir=scratch) as tmp:
        tmp=Path(tmp);(tmp/'CoreMinimal.h').write_text('// Boundary: UObject and IsValid are declared before actual own header.\n',encoding='utf-8');exe=tmp/('entry.exe' if os.name=='nt' else 'entry');cxx=os.environ.get('CXX','cl' if os.name=='nt' else 'c++')
        if os.name=='nt' and Path(cxx).stem.lower() in ('cl','cl.exe'):
            vs=Path('C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat');bat=tmp/'compile.cmd';bat.write_text('@call "'+str(vs)+'"\n@if errorlevel 1 exit /b %errorlevel%\ncl /nologo /std:c++20 /EHsc /utf-8 /W3 /Fe:"'+str(exe)+'" /Fo:"'+str(tmp/'entry.obj')+'" /I"'+str(PUBLIC)+'" /I"'+str(tmp)+'" "'+str(out/'EntryHarness.cpp')+'"\n@exit /b %errorlevel%\n',encoding='utf-8');command=['cmd.exe','/d','/c',str(bat)]
        else: command=[cxx,'-std=c++20','-O2','-Wall','-Wextra','-Werror','-I'+str(PUBLIC),'-I'+str(tmp),str(out/'EntryHarness.cpp'),'-o',str(exe)]
        build=subprocess.run(command,capture_output=True,encoding='utf-8',errors='replace',timeout=180);(out/'compile.log').write_text(build.stdout+build.stderr,encoding='utf-8');record.update(build_exit=build.returncode,command=command)
        if build.returncode==0:
            run=subprocess.run([str(exe)],capture_output=True,encoding='utf-8',errors='replace',timeout=30);(out/'run.log').write_text(run.stdout+run.stderr,encoding='utf-8');cases=re.findall(r'^CASE (PASS|FAIL) (.+)$',run.stdout,re.M);assertions=re.findall(r'^ASSERT_FAIL (.+)$',run.stdout,re.M);summary=re.search(r'^SUMMARY cases=(\d+) assertions=(\d+) failures=(\d+)$',run.stdout,re.M);record.update(run_exit=run.returncode,cases=[{'state':s,'name':n} for s,n in cases],case_count=len(cases),passed_cases=sum(s=='PASS' for s,n in cases),failed_cases=sum(s=='FAIL' for s,n in cases),assertion_failures=assertions);record['summary_consistent']=bool(summary and int(summary[1])==len(cases) and int(summary[3])==len(assertions));record['expected_observed']=record['summary_consistent'] and len(cases)>0 and ((run.returncode!=0 and len(assertions)>0) if args.expect=='red' else (run.returncode==0 and not assertions and all(s=='PASS' for s,n in cases)))
    record['source_unchanged']={n:sha(b) for n,b in inputs().items()}==hashes;record['passed']=bool(record.get('expected_observed') and record['source_unchanged']);(out/'result.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8');print(json.dumps({k:v for k,v in record.items() if k not in ('source_sha256','cases','assertion_failures','definition_sha256')},indent=2));return 0 if record['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
