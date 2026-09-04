from pathlib import Path
import json,subprocess
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-predraw-scheduler-20260903';S=B/'staged'
base=json.loads((R/'pc-port/notes/original-jpa-resource-loader-20260903/native-compiles.json').read_text())[0]['command'][:-4]
base=[x.replace(str(R/'build/original-jpa-resource-loader-20260903/staged'),str(S)) for x in base]
rows=[];objects=[]
for name in ['Game/NameObj/NameObjCategoryList.cpp','Game/NameObj/NameObjListExecutor.cpp','scene/SceneDrawBufferService.cpp','runtime/SceneScheduler.cpp','Game/NameObj/NameObj.cpp','compat/OriginalPreDrawRegistration.cpp']:
 out=B/(Path(name).stem+'.o');objects.append(str(out));cmd=base+['-fsanitize=address,undefined','-fno-omit-frame-pointer','-c',str(S/name),'-o',str(out)]
 p=subprocess.run(cmd,cwd=R/'pc-port',capture_output=True,text=True);(B/(Path(name).stem+'-compile.log')).write_text(p.stdout+p.stderr);print(name,p.returncode)
 if p.returncode:print(p.stdout,p.stderr);p.check_returncode()
 rows.append({'command':cmd,'returncode':p.returncode})
(N/'native-compiles.json').write_text(json.dumps(rows,indent=2)+'\n')

cmd=base+['-fsanitize=address,undefined','-fno-omit-frame-pointer','-c',str(N/'probe.cpp'),'-o',str(B/'probe.o')]
p=subprocess.run(cmd,cwd=R/'pc-port',capture_output=True,text=True);(B/'probe-compile.log').write_text(p.stdout+p.stderr)
if p.returncode:print(p.stdout,p.stderr);p.check_returncode()
objects.append(str(B/'probe.o'));rows.append({'command':cmd,'returncode':0})
old=json.loads((R/'pc-port/notes/original-jpa-resource-loader-20260903/probe-link.json').read_text())['command']
cmd=[old[0],'-fsanitize=address,undefined','-fno-omit-frame-pointer']+objects+old[old.index('-Wl,-dead_strip'):]
cmd[cmd.index('-o')+1]=str(B/'probe')
p=subprocess.run(cmd,cwd=R/'pc-port',capture_output=True,text=True);(B/'probe-link.log').write_text(p.stdout+p.stderr)
if p.returncode:print(p.stdout,p.stderr);p.check_returncode()
rows.append({'command':cmd,'returncode':0})
p=subprocess.run([str(B/'probe')],cwd=R,capture_output=True,text=True,timeout=30);(N/'probe-runtime.log').write_text(p.stdout+p.stderr);print(p.stdout,p.stderr);p.check_returncode()
rows.append({'command':[str(B/'probe')],'returncode':0,'stdout':p.stdout,'stderr':p.stderr})
(N/'native-verification.json').write_text(json.dumps(rows,indent=2)+'\n')
