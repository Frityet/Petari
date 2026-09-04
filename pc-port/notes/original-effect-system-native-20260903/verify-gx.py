from pathlib import Path
import json,subprocess
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-effect-system-native-20260903'
cmd=json.loads((N/'gx-compiles.json').read_text())[0]['command'];cmd=cmd[:cmd.index('-c')]+['-I'+str(R),'-c',str(N/'gx-probe.cpp'),'-o',str(B/'gx-probe.o')]
p=subprocess.run(cmd,cwd=R/'pc-port',capture_output=True,text=True);(N/'gx-probe-compile.log').write_text(p.stdout+p.stderr)
if p.returncode:print(p.stderr);raise SystemExit(p.returncode)
cmd=json.loads((N/'probe-link.json').read_text())['command'];prefix=cmd[:cmd.index('-Wl,-dead_strip')];cmd=[cmd[0],'-fsanitize=address,undefined','-fno-omit-frame-pointer']+[str(B/(name+'.o')) for name in ['gx-probe','GXManage','GXDispList','GXVert']]+cmd[cmd.index('-Wl,-dead_strip'):];cmd[cmd.index('-o')+1]=str(B/'gx-probe')
p=subprocess.run(cmd,cwd=R/'pc-port',capture_output=True,text=True);(N/'gx-probe-link.log').write_text(p.stdout+p.stderr);(N/'gx-probe-link.json').write_text(json.dumps({'command':cmd,'returncode':p.returncode},indent=2)+'\n')
if p.returncode: print(p.stderr);raise SystemExit(p.returncode)
p=subprocess.run([str(B/'gx-probe')],cwd=R,capture_output=True,text=True,timeout=30);(N/'gx-probe-runtime.log').write_text(p.stdout+p.stderr);print(p.stdout,p.stderr);raise SystemExit(p.returncode)
