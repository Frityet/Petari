#!/usr/bin/env python3
from pathlib import Path
import json,subprocess,sys
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-jpa-resource-loader-20260903'
base=json.loads((R/'pc-port/notes/audio-animation-boundary-20260903/link-evidence.json').read_text())['command']
cmd=[];skip=False
for a in base:
 if skip:skip=False
 elif a=='-o':skip=True
 elif a.endswith('.o') or a.startswith('-Wl,-u,') or a.startswith('-Wl,-map,'):continue
 else:cmd.append(a)
full='--full' in sys.argv
objects=[B/'probe.o',B/'JpcResource.o']
if full:objects=[B/'JpcResource.o']+[B/(p.stem+'.o') for p in (B/'staged/JSystem/JParticle').glob('*.cpp')]+[B/'OriginalJPAEmitterInit.o',B/'full-probe.o']
cmd[1:1]=[str(p) for p in objects]
name='full-probe' if full else 'probe';cmd+=['-o',str(B/name)]
r=subprocess.run(cmd,cwd=R/'pc-port',stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(B/(name+'-link.log')).write_text(r.stdout);(N/(name+'-link.json')).write_text(json.dumps({'command':cmd,'returncode':r.returncode},indent=2)+'\n');print(r.stdout[-15000:]);print('link',r.returncode)
if r.returncode==0:
 p=subprocess.run([str(B/name),str(B/'Effect.arc')],cwd=R,capture_output=True,text=True);print(p.stdout,p.stderr);(B/(name+'-runtime.log')).write_text(p.stdout+p.stderr);print('runtime',p.returncode)
