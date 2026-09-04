#!/usr/bin/env python3
from pathlib import Path
import json,subprocess,os,sys,shutil
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-particle-resource-owner-20260903';S=B/'staged'
for p in (N/'native').rglob('*'):
 if p.is_file():
  out=S/p.relative_to(N/'native');out.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,out)
base=json.loads((R/'pc-port/notes/original-jpa-resource-loader-20260903/native-compiles.json').read_text())[0]['command'][:-4]
base=[x.replace(str(R/'build/original-jpa-resource-loader-20260903/staged'),str(S)) for x in base]
rows=[];objects=[]
names=[S/'runtime/ParticleResourceOwnership.cpp',S/'compat/OriginalParticleResourceLookup.cpp',S/'compat/OriginalParticleResourceQueries.cpp',R/'pc-port/src/Game/Effect/ParticleResourceHolder.cpp',N/'probe.cpp']
for src in names:
 out=B/(src.stem+'.o');objects.append(str(out));cmd=base+['-fsanitize=address,undefined','-fno-omit-frame-pointer','-c',str(src),'-o',str(out)]
 p=subprocess.run(cmd,cwd=R/'pc-port',capture_output=True,text=True);(B/(src.stem+'-compile.log')).write_text(p.stdout+p.stderr)
 if p.returncode:print(p.stdout,p.stderr);p.check_returncode()
 rows.append({'command':cmd,'returncode':p.returncode})
old=json.loads((R/'pc-port/notes/original-jpa-resource-loader-20260903/probe-link.json').read_text())['command']
cmd=[old[0],'-fsanitize=address,undefined','-fno-omit-frame-pointer']+objects+old[old.index('-Wl,-dead_strip'):]
cmd[cmd.index('-o')+1]=str(B/'probe');p=subprocess.run(cmd,cwd=R/'pc-port',capture_output=True,text=True);(B/'probe-link.log').write_text(p.stdout+p.stderr)
if p.returncode:print(p.stdout,p.stderr);p.check_returncode()
rows.append({'command':cmd,'returncode':p.returncode})
(N/'native-compiles.json').write_text(json.dumps(rows,indent=2)+'\n')
env=os.environ.copy();env['SMGPC_REAL_DISC']=str(next(R.glob('*.rvz')))
cmd=[str(B/'probe')]+sys.argv[1:];p=subprocess.run(cmd,cwd=R,env=env,capture_output=True,text=True,timeout=60)
(N/'probe-runtime.log').write_text(p.stdout+p.stderr);print(p.stdout,p.stderr)
rows.append({'command':cmd,'returncode':p.returncode,'stdout':p.stdout,'stderr':p.stderr});(N/'native-verification.json').write_text(json.dumps(rows,indent=2)+'\n')
p.check_returncode()
