#!/usr/bin/env python3
from pathlib import Path
import json,subprocess,shutil,hashlib
R=Path(__file__).resolve().parents[3]; N=Path(__file__).resolve().parent; B=R/'build/original-jpa-resource-loader-20260903';S=B/'staged'
e=next(e for e in json.loads((R/'pc-port/compile_commands.json').read_text()) if e['file'].endswith('/MarioMove.cpp'))
cmd=[];skip=False
for a in e['arguments']:
 if skip:skip=False
 elif a=='-o':skip=True
 elif a not in ('-c',e['file']):cmd.append(a)
cmd[1:1]=['-I'+str(S)]
sources=['resource/JpcResource.cpp','JSystem/JParticle/JPAResourceLoader.cpp','JSystem/JParticle/JPAResourceManager.cpp']
for p in sorted((R/'src/JSystem/JParticle').glob('*.cpp')):
 name='JSystem/JParticle/'+p.name
 if name not in sources:
  text=p.read_text()
  if p.name=='JPABaseShape.cpp':text=text.replace('if (i == i_data[j].index)', 'if (j < param_2 && i == i_data[j].index)')
  (S/name).write_text(text);sources.append(name)
for name in ['probe.cpp','block-probe.cpp','full-probe.cpp']:
 probe=B/name
 if probe.exists():sources.append(str(probe))
sources.append(str(R/'pc-port/notes/original-effect-construction-20260903/native/OriginalJPAEmitterInit.cpp'))
results=[]
for name in sources:
 p=S/name if not Path(name).is_absolute() else Path(name);o=B/(p.stem+'.o');c=cmd+['-c',str(p),'-o',str(o)]
 r=subprocess.run(c,cwd=e['directory'],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(B/(p.stem+'.log')).write_text(r.stdout)
 results.append({'source':name,'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'command':c,'returncode':r.returncode}); print(p.stem,r.returncode)
 if r.returncode:print(r.stdout[-2000:])
(N/'native-compiles.json').write_text(json.dumps(results,indent=2)+'\n')
