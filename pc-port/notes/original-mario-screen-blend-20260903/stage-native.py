#!/usr/bin/env python3
from pathlib import Path
import difflib,json,re,subprocess,concurrent.futures
ROOT=Path(__file__).resolve().parents[3];NOTES=Path(__file__).resolve().parent;BUILD=ROOT/'build/original-mario-screen-blend-20260903';STAGE=BUILD/'staged';PC=ROOT/'pc-port';changes={}
def write(rel,text):
 p=STAGE/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text);native=PC/'src'/rel;changes[rel]=(native.read_text() if native.exists() else '',text)
write('Game/Player/MarioActorSpecialDraw.cpp',(ROOT/'src/Game/Player/MarioActorSpecialDraw.cpp').read_text())
rel='Game/Player/MarioActor.hpp';s=(PC/'src'/rel).read_text().replace('    /* 0xB34 */ f32 _B34;\n    /* 0xB38 */ f32 _B38;\n    /* 0xB3C */ f32 _B3C;\n    /* 0xB40 */ f32 _B40;','    /* 0xB34 */ TVec2f mScreenBoxPosition;\n    /* 0xB3C */ TVec2f mScreenBoxSize;');write(rel,s)
rel='Game/Player/MarioActorInit.cpp';s=(PC/'src'/rel).read_text()
for a,b in [('_B34','mScreenBoxPosition.x'),('_B38','mScreenBoxPosition.y'),('_B3C','mScreenBoxSize.x'),('_B40','mScreenBoxSize.y')]:s=s.replace(a,b)
write(rel,s)
for name in ['GXTexture','GXGeometry','GXManage']:write('revolution/gx/'+name+'.h','#pragma once\n#include <dolphin/gx/'+name+'.h>\n')
patch=[]
for rel,(old,new) in changes.items():patch.extend(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='a/pc-port/src/'+rel,tofile='b/pc-port/src/'+rel))
rootpatch=[]
for rel in ['src/Game/Player/MarioActorSpecialDraw.cpp','include/Game/Player/MarioActor.hpp','src/Game/Player/MarioActorInit.cpp','libs/RVL_SDK/include/revolution/gx/GXManage.h']:
 baseline=BUILD/'baseline'/rel
 if rel=='src/Game/Player/MarioActorSpecialDraw.cpp':baseline=BUILD/'baseline.cpp'
 old=baseline.read_text();new=(ROOT/rel).read_text();rootpatch.extend(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel))
for out in (BUILD,NOTES):
 (out/'native.patch').write_text(''.join(patch));(out/'root.patch').write_text(''.join(rootpatch));(out/'native-files.json').write_text(json.dumps(list(changes),indent=2)+'\n')
deps=(PC/'build/.deps/smg-pc-game/macosx/arm64/debug/src/Game/System/ResourceHolder.cpp.o.d').read_text().split('values = {',1)[1];flags=[json.loads(x) for x in re.findall(r'"(?:\\.|[^"\\])*"',deps)]
command=[flags[0],'-I'+str(STAGE),'-I'+str(ROOT/'build/audio-animation-boundary-20260903/staged')]+flags[1:]+['-I'+str(ROOT/'include'),'-I'+str(ROOT/'libs/JSystem/include')]
def work(rel):
 source=STAGE/rel;cmd=command+['-c',str(source),'-o',str(BUILD/(Path(rel).stem+'.native.o'))]
 result=subprocess.run(cmd,cwd=PC,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(BUILD/(Path(rel).stem+'.native.log')).write_text(result.stdout)
 print(rel,result.returncode,result.stdout[-2400:] if result.returncode else '')
 return {'source':rel,'exit':result.returncode,'command':cmd}
with concurrent.futures.ThreadPoolExecutor() as pool:rows=list(pool.map(work,['Game/Player/MarioActorSpecialDraw.cpp','Game/Player/MarioActorInit.cpp']))
for p in (BUILD,NOTES):(p/'native-compile.json').write_text(json.dumps(rows,indent=2)+'\n')
assert all(r['exit']==0 for r in rows)
