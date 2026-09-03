#!/usr/bin/env python3
from pathlib import Path
import json,subprocess,hashlib,sys
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-jpa-resource-loader-20260903'
e=json.loads((N/'native-compiles.json').read_text())[0]['command'][:-4]
sanitize='--sanitize' in sys.argv;suffix='-asan' if sanitize else '';flags=['-fsanitize=address,undefined','-fno-omit-frame-pointer'] if sanitize else []
results=[]
names=['resource/JpcResource.cpp','JSystem/JParticle/JPABaseShape.cpp','JSystem/JParticle/JPADynamicsBlock.cpp','JSystem/JParticle/JPAExtraShape.cpp','JSystem/JParticle/JPAChildShape.cpp','JSystem/JParticle/JPAExTexShape.cpp','JSystem/JParticle/JPAKeyBlock.cpp','JSystem/JParticle/JPAMath.cpp','JSystem/JParticle/JPATexture.cpp']
if not sanitize:names=['resource/JpcResource.cpp']
for name in names+['probe.cpp','block-probe.cpp']:
 src=B/name if name in ['probe.cpp','block-probe.cpp'] else B/'staged'/name
 obj=B/(src.stem+suffix+'.o');cmd=e+flags+['-c',str(src),'-o',str(obj)]
 p=subprocess.run(cmd,cwd=R/'pc-port',capture_output=True,text=True);(B/(src.stem+suffix+'-compile.log')).write_text(p.stdout+p.stderr);p.check_returncode();results.append({'command':cmd,'returncode':p.returncode})
for name in ['probe','block-probe']:
 cmd=json.loads((N/(name+'-link.json')).read_text())['command']
 if sanitize:cmd=[str(B/(Path(a).stem+suffix+'.o')) if a.endswith('.o') and str(B) in a else a for a in cmd]
 cmd[cmd.index('-o')+1]=str(B/(name+suffix));cmd[1:1]=flags
 p=subprocess.run(cmd,cwd=R/'pc-port',capture_output=True,text=True);(B/(name+suffix+'-link.log')).write_text(p.stdout+p.stderr);p.check_returncode();results.append({'command':cmd,'returncode':0})
 cmd=[str(B/(name+suffix)),str(B/'Effect.arc')]+([str(B/'retail-colors.bin')] if name=='block-probe' else [])
 p=subprocess.run(cmd,cwd=R,capture_output=True,text=True);(N/(name+suffix+'-runtime.log')).write_text(p.stdout+p.stderr);print(p.stdout,p.stderr);p.check_returncode();results.append({'command':cmd,'returncode':0,'stdout':p.stdout,'stderr':p.stderr})
(N/('native-verification'+suffix+'.json')).write_text(json.dumps(results,indent=2)+'\n')
