"""Full-definition standalone routing; Engine/SML boundaries are NOT gameplay."""
from pathlib import Path
import hashlib,json,re,subprocess,sys,zipfile,os,argparse
PLUGIN=Path(__file__).resolve().parents[2]
FILES={'POLICY':'Source/TsabsPassengerWagon/Public/PassengerContentPolicy.h','HEADER':'Source/TsabsPassengerWagon/Public/TsabsPassengerWorldModule.h','PRODUCTION':'Source/TsabsPassengerWagon/Private/TsabsPassengerWorldModule.cpp','WAGON':'Source/TsabsPassengerWagon/Private/TsabsPassengerWagonModule.cpp'}
def sha(b):return hashlib.sha256(b).hexdigest()
def main():
 parser=argparse.ArgumentParser();parser.add_argument('label');parser.add_argument('--editor',action='store_true');parser.add_argument('--archive',type=Path);parser.add_argument('--output-root',type=Path,required=True);parser.add_argument('--scratch-root',type=Path,required=True)
 args=parser.parse_args();assert re.fullmatch('[a-z0-9-]+',args.label)
 target=args.output_root/args.label;target.mkdir(parents=True,exist_ok=False)
 scratch=args.scratch_root/args.label;scratch.mkdir(parents=True,exist_ok=False)
 exact={k:(PLUGIN/n).read_bytes() for k,n in FILES.items()}
 if args.archive:
  with zipfile.ZipFile(args.archive) as z:
   assert z.testzip() is None
   exact={k:z.read(n) for k,n in FILES.items()}
 template=Path(__file__).with_name('AvailabilityHarness.cpp.in').read_bytes();code=template.decode('utf-8');bodies={}
 for k,b in exact.items():
  body=re.sub(r'^#include[^\r\n]*(?:\r?\n|$)','',b.decode('utf-8-sig'),flags=re.M)
  bodies[k]=body;assert code.count('@@'+k+'@@')==1;code=code.replace('@@'+k+'@@',body)
  (target/(k+'.exact')).write_bytes(b)
 assert '@@' not in code
 cpp=scratch/'harness.cpp';cpp.write_bytes(code.encode('utf-8'));exe=scratch/'routing.exe'
 bat=scratch/'compile.cmd';bat.write_text('@call "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" -vcvars_ver=14.38\n@if errorlevel 1 exit /b %errorlevel%\ncl.exe /nologo /std:c++20 /EHsc /utf-8 /W3 /DWITH_EDITOR='+str(int(args.editor))+' /Fe:"'+str(exe)+'" /Fo:"'+str(scratch/'routing.obj')+'" "'+str(cpp)+'"\n@exit /b %errorlevel%\n')
 env=os.environ.copy();env.update(TEMP=str(args.scratch_root),TMP=str(args.scratch_root),TMPDIR=str(args.scratch_root))
 build=subprocess.run(['cmd.exe','/d','/c',str(bat)],cwd=scratch,env=env,capture_output=True,encoding='utf-8',errors='replace',timeout=180)
 (target/'compile.log').write_text(build.stdout+build.stderr,encoding='utf-8')
 main=template.decode('utf-8').split('int main(){',1)[1];common,branches=main.split('#if WITH_EDITOR',1);editor,runtime=branches.split('#else',1);runtime=runtime.split('#endif',1)[0]
 expected=re.findall(r' Case\("([^"]+)"',common+(editor if args.editor else runtime));assert len(expected)==len(set(expected))
 result={'compile_exit':build.returncode,'WITH_EDITOR':args.editor,'expected_cases':expected,'expected_case_count':len(expected),'compiler_toolset_requested':'14.38','template_sha256':sha(template),'runner_sha256':sha(Path(__file__).read_bytes()),'exact_source_sha256':{k:sha(b) for k,b in exact.items()},'body_text_sha256':{k:sha(b.encode()) for k,b in bodies.items()},'extraction':'Only #include directives removed, ALL complete body definitions retained; no production conditionals rewritten.','archive':str(args.archive) if args.archive else None,'archive_sha256':sha(args.archive.read_bytes()) if args.archive else None,'scope':'Standalone production routing with catalogued Engine/SML boundaries; not Unreal, detours, gameplay, replication or runtime shutdown acceptance.'}
 if not build.returncode:
  run=subprocess.run([str(exe)],cwd=scratch,env=env,capture_output=True,encoding='utf-8',errors='replace',timeout=30)
  text=run.stdout+run.stderr;(target/'run.log').write_text(text,encoding='utf-8');cases=re.findall(r'^CASE (PASS|FAIL) (.+)$',text,re.M)
  result.update(run_exit=run.returncode,case_count=len(cases),passed=sum(s=='PASS' for s,n in cases),failed=sum(s=='FAIL' for s,n in cases),cases=[{'name':n,'state':s} for s,n in cases],assertion_failures=re.findall('^ASSERT_FAIL (.+)$',text,re.M),active_unsubscriptions=re.findall('^ACTIVE_LIST_UNSUBSCRIBE (.+)$',text,re.M))
  result.update(metrics={k:int(v) for k,v in re.findall(r'^METRIC (\w+) (\d+)$',text,re.M)},complete_case_payload=[n for s,n in cases]==expected)
 result['standalone_passed']=build.returncode==0 and result.get('run_exit')==0 and result.get('complete_case_payload',False) and result.get('failed')==0 and not result.get('assertion_failures') and not result.get('active_unsubscriptions')
 for p in (cpp,bat):(target/p.name).write_bytes(p.read_bytes())
 (target/'result.json').write_text(json.dumps(result,indent=2))
 print(json.dumps(result,indent=2))
 if build.returncode:print(build.stdout+build.stderr)
 return build.returncode or result.get('run_exit',2) or (0 if result['standalone_passed'] else 2)
if __name__=='__main__':raise SystemExit(main())
