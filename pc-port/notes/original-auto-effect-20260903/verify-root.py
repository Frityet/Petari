#!/usr/bin/env python3
from pathlib import Path
import importlib.util,json,subprocess,hashlib,difflib
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-auto-effect-20260903'
s=importlib.util.spec_from_file_location('h',R/'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py');h=importlib.util.module_from_spec(s);s.loader.exec_module(h)
results=[]
for name in ['AutoEffectGroup','AutoEffectInfo']:
 src=R/'src/Game/Effect'/(name+'.cpp');cmd=h.compiler('cflags_game')+['-c',str(src),'-o',str(B/(name+'.o'))]
 p=subprocess.run(cmd,cwd=R,capture_output=True,text=True);(B/(name+'-compile.log')).write_text(p.stdout+p.stderr)
 if p.returncode:print(p.stdout,p.stderr);p.check_returncode()
 retail=R/'build/mario-update-restoration-20260903/retail/obj/Game/Effect'/(name+'.o')
 c=[str(R/'build/tools/objdiff-cli'),'diff','-1',str(retail),'-2',str(B/(name+'.o')),'-o',str(B/(name+'.json')),'--format','json-pretty'];subprocess.run(c,check=True,capture_output=True)
 data=json.loads((B/(name+'.json')).read_text());rows=[]
 for row in data['left']['symbols']:
  if row.get('match_percent') is not None:
   out={k:row[k] for k in ['name','size','match_percent']};rows.append(out);print(name,out)
 results.append({'source':str(src.relative_to(R)),'source_sha256':hashlib.sha256(src.read_bytes()).hexdigest(),'command':cmd,'methods':rows})
(N/'root-evidence.json').write_text(json.dumps(results,indent=2)+'\n')
