from pathlib import Path
import concurrent.futures,hashlib,json,subprocess,shutil
root=Path(__file__).resolve().parents[3];out=Path(__file__).resolve().parent
names=['DemoCastGroup','DemoCastGroupHolder','DemoCastSubGroup','DemoSimpleCastHolder','DemoSubPartKeeper','DemoActionKeeper','DemoPlayerKeeper','DemoCameraKeeper','DemoCameraFunction','DemoTalkAnimCtrl','DemoPositionController']
staged=out/'native-staged';(staged/'Game/Demo').mkdir(parents=True,exist_ok=True);(staged/'Game/Util').mkdir(parents=True,exist_ok=True)
for n in names:
 shutil.copyfile(root/'decomp/include/Game/Demo'/(n+'.hpp'),staged/'Game/Demo'/(n+'.hpp'))
 shutil.copyfile(root/'decomp/src/Game/Demo'/(n+'.cpp'),staged/'Game/Demo'/(n+'.cpp'))
objutil=(root/'src/Game/Util/ObjUtil.hpp').read_text()
if 'bool isName(' not in objutil:objutil+='\nnamespace MR {\n    bool isName(const NameObj*, const char*);\n    bool isSame(const NameObj*, const NameObj*);\n}\n'
(staged/'Game/Util/ObjUtil.hpp').write_text(objutil)
prior=json.loads((root/'notes/original-talk-demo-owners-20260910/demo/time-native-proof.json').read_text())['command'];base=prior[1:];base=[x for x in base if not x.startswith('-I'+str(root/'notes'))];base=base[:base.index('-c')];base.insert(0,'-I'+str(staged))
helper=root/'build/.tools/game-execution-charset/9f8b08a6c256ed1e771d13b564d146aa334180070727ffb5717e5c56f101a17a/game-literal-preprocessor'
def run(n):
 source=staged/'Game/Demo'/(n+'.cpp');obj=out/(n+'.native.o');cmd=['python3',str(root/'script/game_execution_charset.py'),'--compiler',prior[0],'--helper',str(helper),'--game-root',str(staged/'Game'),'--game-root',str(root/'src/Game'),'--']+base+['-c',str(source),'-o',str(obj)]
 p=subprocess.run(cmd,cwd=root,capture_output=True,text=True);(out/(n+'.native.log')).write_text(p.stdout+p.stderr)
 r={'name':n,'command':cmd,'exit_code':p.returncode,'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest()}
 if p.returncode==0:
  nm=subprocess.run(['/opt/homebrew/opt/llvm/bin/llvm-nm','--undefined-only','--demangle',str(obj)],capture_output=True,text=True)
  r['undefined']=[s.strip().removeprefix('U ').strip() for s in nm.stdout.splitlines() if s.strip()]
 else:r['errors']=[s for s in p.stderr.splitlines() if 'error:' in s]
 return r
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:rs=list(pool.map(run,names))
(out/'native-probe.json').write_text(json.dumps(rs,indent=2)+'\n')
for r in rs:print(r['name'],r['exit_code'],r.get('errors',len(r.get('undefined',[]))))
raise SystemExit(any(r['exit_code'] for r in rs))
