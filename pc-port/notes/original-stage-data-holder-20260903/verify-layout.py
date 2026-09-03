#!/usr/bin/env python3
from pathlib import Path
import importlib.util,subprocess,json,hashlib
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-stage-data-holder-20260903'
def load(name,p):
 s=importlib.util.spec_from_file_location(name,p);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
v=load('verify_stage',N/'verify-original.py');rows=[]
# Same current source on both sides isolates only the field's actual matrix
# type and its two equivalent return expressions. Constructor is excluded
# from the old-header source because its newly recovered typed identity call
# cannot exist with the former raw-array declaration.
for unit,rel in [('SceneUtil','Game/Util/SceneUtil.cpp'),('StageDataHolder','Game/Scene/StageDataHolder.cpp')]:
 source=(R/'src'/rel).read_text();old=source
 if unit=='SceneUtil':old=old.replace('return const_cast< TPos3f* >(&holder->mPlacementMtx);','return (TPos3f*)holder->mPlacementMtx;')
 else:
  start=old.index('StageDataHolder::StageDataHolder(');end=old.index('void StageDataHolder::init(',start);old=old[:start]+old[end:];source=old
 elves=[]
 for side,text in [('before',old),('after',source)]:
  overlay=B/side;header=overlay/'Game/Scene/StageDataHolder.hpp';header.parent.mkdir(parents=True,exist_ok=True)
  body=(R/'include/Game/Scene/StageDataHolder.hpp').read_text()
  if side=='before':body=body.replace('TPos3f mPlacementMtx;','Mtx mPlacementMtx;')
  header.write_text(body);p=overlay/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text);obj=B/(unit+'.'+side+'.o');cmd=v.c.compiler('cflags_game');cmd[3:3]=['-i',str(overlay)];cmd+=['-c',str(p),'-o',str(obj)];subprocess.run(cmd,cwd=R,check=True);elves.append(v.Elf(obj))
 for name,start,size,index in elves[0].symbols:
  if not size or not (name.startswith('getZonePlacementMtx__2MR') if unit=='SceneUtil' else name.startswith(('calcPlacementMtx__15','__dt__15StageDataHolder'))):continue
  a,b=elves;assert a.code(name)==b.code(name);assert a.references(name)==b.references(name)
  rows.append({'symbol':name,'bytes':size,'instructions_and_relocations_equal':True});print(name,size,'unchanged')
(N/'layout-evidence.json').write_text(json.dumps({'typed_member':'StageDataHolder::mPlacementMtx, Mtx -> TPos3f','root_header_sha256':hashlib.sha256((R/'include/Game/Scene/StageDataHolder.hpp').read_bytes()).hexdigest(),'functions':rows},indent=2)+'\n')
