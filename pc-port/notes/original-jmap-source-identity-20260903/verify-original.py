#!/usr/bin/env python3
"""Verify root pointer-width architecture and the complete SDK index/fetch bodies."""
from pathlib import Path
import importlib.util,json,subprocess,hashlib
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-jmap-source-identity-20260903'
spec=importlib.util.spec_from_file_location('stageproof',R/'pc-port/notes/original-stage-data-holder-20260903/verify-original.py');p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
 rows=[]
 groups=[('Game/Scene/StageDataHolder','cflags_game',['findPlacedStageDataHolder__15StageDataHolderCFRC12JMapInfoIter','getStartJMapInfoIterFromStartDataIndex__15StageDataHolderCFi','calcDataAddress__15StageDataHolderFv','updateDataAddress__15StageDataHolderFPCQ22MR26AssignableArray<8JMapInfo>']),('JSystem/JKernel/JKRArchivePub','cflags_jsys',['getIdxResource__10JKRArchiveFUl']),('JSystem/JKernel/JKRMemArchive','cflags_jsys',['fetchResource__13JKRMemArchiveFPQ210JKRArchive12SDIFileEntryPUl'])]
 dol=p.c.DOL.read_bytes();assert hashlib.sha1(dol).hexdigest()=='25c5959534b3c21246c6c7e42021b916b41fb578'
 for src,flags,names in groups:
  obj=B/(Path(src).name+'.o');source=R/'src'/f'{src}.cpp';cmd=p.c.compiler(flags)+['-c',str(source),'-o',str(obj)];subprocess.run(cmd,cwd=R,check=True)
  retail=R/'build/original-collision-owner-20260903/retail/obj'/f'{src}.o';a=p.Elf(retail);b=p.Elf(obj)
  diff=B/(Path(src).name+'-diff.json');subprocess.run([R/'build/tools/objdiff-cli','diff','-1',retail,'-2',obj,'-o',diff,'--format','json-pretty'],check=True,stdout=subprocess.DEVNULL);j=json.loads(diff.read_text())
  for name in names:
   refs=p.relocate(b,a,name,p.addresses[name],dol);match=next(s['match_percent'] for s in j['left']['symbols'] if s['name']==name)
   rows.append(dict(source=str(source.relative_to(R)),source_sha256=sha(source),symbol=name,retail_address=hex(p.addresses[name]),bytes=len(b.code(name)),match_percent=match,all_instructions_identical_after_verified_relocations=True,relocations=refs,compiler_command=cmd));print(name,match,len(b.code(name)))
 (N/'compiler-evidence.json').write_text(json.dumps(dict(functions=rows,headers={str(f):sha(R/f) for f in ['include/Game/Scene/StageDataHolder.hpp','include/Game/Util/JMapInfo.hpp']}),indent=2)+'\n')
if __name__=='__main__':main()
