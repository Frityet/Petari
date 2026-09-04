from pathlib import Path
import json,subprocess,os
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-effect-system-native-20260903'
rows=json.loads((N/'native-compiles.json').read_text());assert all(r['returncode']==0 for r in rows)
cmd=rows[0]['command'];base=cmd[:cmd.index('-MMD')]+['-I'+str(R),'-ivfsoverlay',str(B/'aurora-overlay.json')]
cmd=base+['-c',str(N/'probe.cpp'),'-o',str(B/'probe.o')];p=subprocess.run(cmd,cwd=R/'pc-port',text=True,capture_output=True);(B/'probe-compile.log').write_text(p.stdout+p.stderr)
if p.returncode:print(p.stdout,p.stderr);p.check_returncode()
old=json.loads((R/'pc-port/notes/original-jpa-resource-loader-20260903/probe-link.json').read_text())['command'];objects=[r['command'][r['command'].index('-o')+1] for r in rows]+[str(B/'probe.o')]+[str(B/(n+'.o')) for n in ['GXManage','GXDispList','GXVert']]
cmd=[old[0],'-fsanitize=address,undefined','-fno-omit-frame-pointer']+objects+old[old.index('-Wl,-dead_strip'):];cmd[cmd.index('-o')+1]=str(B/'probe')
p=subprocess.run(cmd,cwd=R/'pc-port',text=True,capture_output=True);(N/'probe-link.log').write_text(p.stdout+p.stderr);(N/'probe-link.json').write_text(json.dumps({'command':cmd,'returncode':p.returncode},indent=2)+'\n')
if p.returncode:print(p.stdout,p.stderr);p.check_returncode()
env=os.environ.copy();env['SMGPC_REAL_DISC']=str(next(R.glob('*.rvz')))
p=subprocess.run([str(B/'probe')],cwd=R,env=env,text=True,capture_output=True,timeout=30);(N/'probe-runtime.log').write_text(p.stdout+p.stderr);print(p.stdout,p.stderr);p.check_returncode()
