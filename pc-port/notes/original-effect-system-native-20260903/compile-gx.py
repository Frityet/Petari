from pathlib import Path
import json,subprocess
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-effect-system-native-20260903';A=R/'pc-port/aurora'
overlay={'version':0,'use-external-names':False,'roots':[{'type':'file','name':str(A/p.relative_to(N/'aurora')),'external-contents':str(p)} for p in (N/'aurora').rglob('*') if p.is_file()]}
(B/'aurora-overlay.json').write_text(json.dumps(overlay,indent=2)+'\n')
entries=json.loads((R/'pc-port/compile_commands.json').read_text());rows=[]
for name in ['GXManage','GXDispList','GXVert']:
 entry=next(e for e in entries if e['file'].endswith('/'+name+'.cpp'));old=entry['arguments'];cmd=old[:-3];cmd.remove('-c')
 cmd+=['-ivfsoverlay',str(B/'aurora-overlay.json'),'-fsanitize=address,undefined','-fno-omit-frame-pointer','-c',str(A/'lib/dolphin/gx'/f'{name}.cpp'),'-o',str(B/f'{name}.o')]
 p=subprocess.run(cmd,cwd=R/'pc-port',text=True,capture_output=True);(B/f'{name}-compile.log').write_text(p.stdout+p.stderr);rows.append({'command':cmd,'returncode':p.returncode});print(name,p.returncode,p.stderr if p.returncode else '')
(N/'gx-compiles.json').write_text(json.dumps(rows,indent=2)+'\n')
