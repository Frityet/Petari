#!/usr/bin/env python3
from pathlib import Path
import importlib.util,json,subprocess,hashlib,difflib
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-jpa-overwrite-20260903'
spec=importlib.util.spec_from_file_location('helper',R/'pc-port/notes/j3d-vertex-buffer-lifecycle-20260903/verify-object.py');h=importlib.util.module_from_spec(spec);spec.loader.exec_module(h)
src=R/'src/Game/System/Overwrite.cpp';cmd=h.compiler('cflags_game')+['-c',str(src),'-o',str(B/'Overwrite.o')]
r=subprocess.run(cmd,cwd=R,capture_output=True,text=True);(B/'Overwrite.log').write_text(r.stdout+r.stderr)
if r.returncode:print(r.stdout,r.stderr);r.check_returncode()
cmd2=[str(R/'build/tools/objdiff-cli'),'diff','-1',str(R/'build/mario-update-restoration-20260903/retail/obj/Game/System/Overwrite.o'),'-2',str(B/'Overwrite.o'),'-o',str(B/'Overwrite.json'),'--format','json-pretty'];subprocess.run(cmd2,check=True,capture_output=True)
d=json.loads((B/'Overwrite.json').read_text());syms=[]
for s in d['left']['symbols']:
 if 'JPAField' in s['name'] and s.get('match_percent') is not None and 'F' in s['name'] and not s['name'].startswith('__'):
  syms.append({k:s[k] for k in ['name','size','match_percent']});print(s['name'],s['match_percent'])
(N/'root-evidence.json').write_text(json.dumps({'command':cmd,'methods':syms,'source_sha256':hashlib.sha256(src.read_bytes()).hexdigest()},indent=2)+'\n')
old=(B/'Overwrite-before.cpp').read_text();new=src.read_text();(N/'root.patch').write_text(''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),'a/src/Game/System/Overwrite.cpp','b/src/Game/System/Overwrite.cpp')))
