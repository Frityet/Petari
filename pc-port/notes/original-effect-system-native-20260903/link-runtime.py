from pathlib import Path
import json,subprocess
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-effect-system-native-20260903'
rows=json.loads((N/'native-compiles.json').read_text());cmd=rows[0]['command'];cmd=cmd[:cmd.index('-MMD')]+['-c',str(N/'runtime-probe.cpp'),'-o',str(B/'runtime-probe.o')]
p=subprocess.run(cmd,cwd=R/'pc-port',text=True,capture_output=True);(N/'runtime-probe-compile.log').write_text(p.stdout+p.stderr)
if p.returncode: print(p.stderr);raise SystemExit(p.returncode)
cmd=json.loads((N/'probe-link.json').read_text())['command'];cmd=[x.replace(str(B/'probe.o'),str(B/'runtime-probe.o')) for x in cmd];cmd[cmd.index('-o')+1]=str(B/'runtime-probe');cmd += [str(next((Path.home()/'.xmake/packages/l/libpng').glob('*/f8ae13cb78784172986d714555ab04d9/lib/libpng.a'))),'-lz']
p=subprocess.run(cmd,cwd=R/'pc-port',text=True,capture_output=True);(N/'runtime-probe-link.log').write_text(p.stdout+p.stderr);(N/'runtime-probe-link.json').write_text(json.dumps({'command':cmd,'returncode':p.returncode},indent=2)+'\n');print('runtime fixture compile=0 link='+str(p.returncode));print(p.stderr if p.returncode else 'Not run: actual Aurora window/renderer lifecycle is parent-owned.');raise SystemExit(p.returncode)
