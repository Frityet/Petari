#!/usr/bin/env python3
from pathlib import Path
import difflib,json,re,subprocess,concurrent.futures
ROOT=Path(__file__).resolve().parents[3];NOTES=Path(__file__).resolve().parent;BUILD=ROOT/'build/original-mario-raster-effects-20260903';STAGE=BUILD/'staged';PC=ROOT/'pc-port';changes={}
def write(rel,text):
 p=STAGE/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text);native=PC/'src'/rel;changes[rel]=(native.read_text() if native.exists() else '',text)
write('Game/Player/MarioActorSpecialDraw.cpp',(ROOT/'src/Game/Player/MarioActorSpecialDraw.cpp').read_text())
rel='Game/Player/MarioActor.hpp';s=(PC/'src'/rel).read_text().replace('    /* 0x1D8 */ FBO* _1D8;\n    /* 0x1DC */ FBO* _1DC;','    /* 0x1D8 */ FBO* mRasterBuffers[2];');write(rel,s)
for rel in ['Game/Player/MarioActorInit.cpp','Game/Player/MarioActor.cpp']:
 s=(PC/'src'/rel).read_text().replace('_1D8','mRasterBuffers[0]').replace('_1DC','mRasterBuffers[1]');write(rel,s)
patch=[]
for rel,(old,new) in changes.items():patch.extend(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='a/pc-port/src/'+rel,tofile='b/pc-port/src/'+rel))
rootpatch=[]
for rel in ['src/Game/Player/MarioActorSpecialDraw.cpp','include/Game/Player/MarioActor.hpp','src/Game/Player/MarioActorInit.cpp','src/Game/Player/MarioActor.cpp']:
 old=(BUILD/'baseline'/rel).read_text();new=(ROOT/rel).read_text()
 if rel=='src/Game/Player/MarioActorSpecialDraw.cpp' and '// void MarioActor::drawWallShade' in old:
  # Parent integrated the independently recovered wall-shade body during this task.
  start=new.index('void MarioActor::drawWallShade(');end=new.index('void MarioActor::drawSpinInhibit()',start)
  a=old.index('// void MarioActor::drawWallShade');b=old.index('void MarioActor::drawSpinInhibit()',a);old=old[:a]+new[start:end]+old[b:]
 rootpatch.extend(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel))
for out in (BUILD,NOTES):
 (out/'native.patch').write_text(''.join(patch));(out/'root.patch').write_text(''.join(rootpatch));(out/'native-files.json').write_text(json.dumps(list(changes),indent=2)+'\n')
deps=(PC/'build/.deps/smg-pc-game/macosx/arm64/debug/src/Game/System/ResourceHolder.cpp.o.d').read_text().split('values = {',1)[1];flags=[json.loads(x) for x in re.findall(r'"(?:\\.|[^"\\])*"',deps)]
command=[flags[0],'-I'+str(STAGE),'-I'+str(ROOT/'build/audio-animation-boundary-20260903/staged')]+flags[1:]+['-I'+str(ROOT/'include'),'-I'+str(ROOT/'libs/JSystem/include')]
def work(rel):
 source=STAGE/rel;cmd=command+['-c',str(source),'-o',str(BUILD/(Path(rel).stem+'.native.o'))]
 result=subprocess.run(cmd,cwd=PC,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(BUILD/(Path(rel).stem+'.native.log')).write_text(result.stdout)
 print(rel,result.returncode,result.stdout[-2400:] if result.returncode else '')
 return {'source':rel,'exit':result.returncode,'command':cmd}
with concurrent.futures.ThreadPoolExecutor() as pool:rows=list(pool.map(work,['Game/Player/MarioActorSpecialDraw.cpp','Game/Player/MarioActorInit.cpp','Game/Player/MarioActor.cpp']))
for p in (BUILD,NOTES):(p/'native-compile.json').write_text(json.dumps(rows,indent=2)+'\n')
assert all(r['exit']==0 for r in rows)
