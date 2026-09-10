from pathlib import Path
import concurrent.futures,json,subprocess,hashlib
root=Path(__file__).resolve().parents[2];out=Path(__file__).resolve().parent
entries=json.loads((root/'compile_commands.json').read_text());entry=next(e for e in entries if e['file'].endswith('NPCActor.cpp'))
base=[];skip=False
for x in entry['arguments']:
 if skip:skip=False;continue
 if x in ['-o','-MF','-MT','-MQ']:skip=True;continue
 if x in ['-c','-MMD','-MD','-MP'] or x.endswith('NPCActor.cpp'):continue
 base.append(x)
files=['Game/Demo/DemoDirector.cpp','Game/Demo/DemoFunction.cpp','Game/Demo/DemoExecutorFunction.cpp','Game/Util/DemoUtil.cpp','compat/OriginalActorBckControl.cpp','Game/Util/ObjUtil.cpp']
def run(f):
 source=root/'src'/f;obj=out/(source.stem+'.native.o');cmd=base+['-c',str(source),'-o',str(obj)];p=subprocess.run(cmd,cwd=root,capture_output=True,text=True);(out/(source.stem+'.native.log')).write_text(p.stdout+p.stderr)
 r={'file':f,'code':p.returncode,'command':cmd,'sha256':hashlib.sha256(source.read_bytes()).hexdigest()}
 if p.returncode:r['errors']=[x for x in p.stderr.splitlines() if 'error:' in x]
 else:r['undefined']=subprocess.run(['/opt/homebrew/opt/llvm/bin/llvm-nm','--undefined-only','--demangle',str(obj)],capture_output=True,text=True).stdout
 return r
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:r=list(pool.map(run,files))
(out/'director-compile.json').write_text(json.dumps(r,indent=2)+'\n')
for x in r:print(x['file'],x['code'],x.get('errors',''))
