from pathlib import Path
import concurrent.futures, hashlib, json, re, shutil, subprocess
root=Path(__file__).resolve().parents[3]
out=Path(__file__).resolve().parent
include=out/'native-headers'; (include/'Game/Demo').mkdir(parents=True,exist_ok=True)
for h in (root/'decomp/include/Game/Demo').glob('*.hpp'): shutil.copyfile(h,include/'Game/Demo'/h.name)
entries=json.loads((root/'compile_commands.json').read_text()); entry=next(e for e in entries if e['file'].endswith('NPCActor.cpp'))
base=[];skip=False
for x in entry['arguments']:
 if skip:skip=False;continue
 if x in ['-o','-MF','-MT','-MQ']:skip=True;continue
 if x in ['-c','-MMD','-MD','-MP'] or x.endswith('NPCActor.cpp'):continue
 base.append(x)
base.insert(1,'-I'+str(include))
names=['DemoCastGroup','DemoCastGroupHolder','DemoCastSubGroup','DemoSimpleCastHolder','DemoExecutor','DemoSubPartKeeper','DemoPositionController','DemoActionKeeper','DemoPlayerKeeper','DemoCameraKeeper','DemoCameraFunction','DemoTalkAnimCtrl']
def probe(name):
 source=root/'decomp/src/Game/Demo'/(name+'.cpp'); obj=out/(name+'.native.o'); cmd=base+['-c',str(source),'-o',str(obj)]
 p=subprocess.run(cmd,cwd=root,capture_output=True,text=True);(out/(name+'.native.log')).write_text(p.stdout+p.stderr)
 result={'name':name,'command':cmd,'exit_code':p.returncode,'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest()}
 if p.returncode==0:
  n=subprocess.run(['/opt/homebrew/opt/llvm/bin/llvm-nm','--undefined-only','--demangle',str(obj)],capture_output=True,text=True)
  result['undefined']=[line.strip().removeprefix('U ').strip() for line in n.stdout.splitlines() if line.strip()]
 else:result['errors']=[line for line in p.stderr.splitlines() if 'error:' in line][:8]
 return result
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool: results=list(pool.map(probe,names))
(out/'existing-native-probe.json').write_text(json.dumps(results,indent=2)+'\n')
for r in results:print(r['name'],r['exit_code'],r.get('errors',len(r.get('undefined',[]))))
