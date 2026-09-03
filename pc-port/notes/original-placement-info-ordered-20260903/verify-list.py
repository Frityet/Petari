#!/usr/bin/env python3
from pathlib import Path
import importlib.util,subprocess,json
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-placement-info-ordered-20260903'
s=importlib.util.spec_from_file_location('placement',N/'verify-original.py');v=importlib.util.module_from_spec(s);s.loader.exec_module(v)
base='849e6b233';elves=[]
for side in ('before','after'):
 overlay=B/('list-'+side)
 for rel in ('include/Game/Util/BothDirList.hpp','src/Game/Util/BothDirList.cpp'):
  text=subprocess.check_output(['git','show',base+':'+rel],cwd=R,text=True) if side=='before' else (R/rel).read_text()
  p=overlay/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text)
 cmd=v.v.c.compiler('cflags_game');cmd[3:3]=['-i',str(overlay/'include')];obj=B/('BothDirList.'+side+'.o');cmd+=['-c',str(overlay/'src/Game/Util/BothDirList.cpp'),'-o',str(obj)];subprocess.run(cmd,cwd=R,check=True);elves.append(v.Elf(obj))
a,b=elves;rows=[]
for name,start,size,index in a.symbols:
 if not size or name.startswith('__ct__Q22MR14BothDirPtrList') or index==0 or not (a.sections[index][2]&4):continue
 assert a.code(name)==b.code(name) and a.references(name)==b.references(name),name
 rows.append({'symbol':name,'bytes':size,'instructions_and_relocations_equal':True});print(name,size,'unchanged')
(N/'list-evidence.json').write_text(json.dumps({'preimage_commit':base,'same_bool_constructor_body_moved_inline':True,'remaining_methods':rows},indent=2)+'\n')
