from pathlib import Path
import json,subprocess
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-particle-draw-executor-20260903'
base=json.loads((N/'native-compiles.json').read_text())[0]['command'][:-4]
flags=['-fsanitize=address,undefined','-fno-omit-frame-pointer'];evidence=[]
sources=[B/'staged/Game/NameObj/NameObjAdaptor.cpp',N/'callback-probe.cpp']
objects=[]
for src in sources:
 out=B/(src.stem+'-asan.o');objects.append(str(out));cmd=base+flags+['-c',str(src),'-o',str(out)]
 p=subprocess.run(cmd,cwd=R/'pc-port',capture_output=True,text=True);(B/(src.stem+'-asan-compile.log')).write_text(p.stdout+p.stderr)
 if p.returncode:print(p.stdout,p.stderr);p.check_returncode()
 evidence.append({'command':cmd,'returncode':0})
old=json.loads((R/'pc-port/notes/original-jpa-resource-loader-20260903/probe-link.json').read_text())['command']
cmd=[old[0]]+flags+objects+old[old.index('-Wl,-dead_strip'):]
cmd[cmd.index('-o')+1]=str(B/'probe-asan')
p=subprocess.run(cmd,cwd=R/'pc-port',capture_output=True,text=True);(B/'probe-asan-link.log').write_text(p.stdout+p.stderr)
if p.returncode:print(p.stdout,p.stderr);p.check_returncode()
evidence.append({'command':cmd,'returncode':0})
cmd=[str(B/'probe-asan')]
p=subprocess.run(cmd,cwd=R,capture_output=True,text=True);(N/'probe-asan-runtime.log').write_text(p.stdout+p.stderr);print(p.stdout,p.stderr);p.check_returncode()
evidence.append({'command':cmd,'returncode':0,'stdout':p.stdout,'stderr':p.stderr})
(N/'native-verification-asan.json').write_text(json.dumps(evidence,indent=2)+'\n')
