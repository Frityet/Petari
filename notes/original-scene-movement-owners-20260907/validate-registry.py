from pathlib import Path
import hashlib,json,re,shlex,subprocess
root=Path(__file__).resolve().parents[2];note=Path(__file__).resolve().parent
base=json.loads((root/'notes/original-mario-owner-activation-20260907/Mario-compile.json').read_text())['command'];base=base[:base.index('-o')]
results=[];objects=[]
for source in ['src/Game/NameObj/NameObjHolder.cpp','src/scene/SceneNameObjRegistry.cpp','src/compat/NameObjLifetimeCompat.cpp','src/compat/ActorRuntimeRegistry.cpp','tests/SceneNameObjRegistryTests.cpp']:
 p=root/source;obj=note/(p.stem+'.o');cmd=base+['-o',str(obj),str(p)];log=note/(p.stem+'-registry.log')
 with log.open('w') as f:r=subprocess.run(cmd,cwd=root,stdout=f,stderr=subprocess.STDOUT)
 results.append({'source':source,'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'exit':r.returncode,'command':cmd});print(source,r.returncode,flush=True)
 if r.returncode:break
 objects.append(str(obj))
else:
 text=(root/'notes/original-stage-initialization-20260907/smg-pc-original-player-status-storage-tests-build.log').read_text();lines=[re.sub(r'\x1b\[[0-9;]*m','',x) for x in text.splitlines()];cmd=shlex.split(next(x for x in lines if 'clang++ -o' in x));cmd[2]=str(note/'scene-registry-tests');cmd[3:4]=objects
 with (note/'registry-link.log').open('w') as f:r=subprocess.run(cmd,cwd=root,stdout=f,stderr=subprocess.STDOUT)
 results.append({'phase':'link','exit':r.returncode,'command':cmd});print('link',r.returncode,flush=True)
 if r.returncode==0:
  with (note/'registry-run.log').open('w') as f:r=subprocess.run([str(note/'scene-registry-tests')],cwd=root,stdout=f,stderr=subprocess.STDOUT,timeout=60)
  results.append({'phase':'run','exit':r.returncode});print('run',r.returncode,(note/'registry-run.log').read_text(),flush=True)
 else:print('\n'.join(x for x in (note/'registry-link.log').read_text().splitlines() if 'undefined' in x.lower() or 'referenced from:' in x),flush=True)
(note/'registry-result.json').write_text(json.dumps(results,indent=2)+'\n')
