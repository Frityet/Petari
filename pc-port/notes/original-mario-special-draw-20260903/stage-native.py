#!/usr/bin/env python3
from pathlib import Path
import difflib,json
ROOT=Path(__file__).resolve().parents[3];BUILD=ROOT/'build/original-mario-special-draw-20260903';STAGE=BUILD/'staged';NOTES=Path(__file__).resolve().parent
changes={}
def write(rel,text):
 p=STAGE/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text)
 changes[rel]=((ROOT/'pc-port/src'/rel).read_text(),text)
def extract(text,start,end):return text[text.index(start):text.index(end,text.index(start))]
write('Game/Player/MarioActorSpecialDraw.cpp',(ROOT/'src/Game/Player/MarioActorSpecialDraw.cpp').read_text())
s=(ROOT/'pc-port/src/Game/Player/MarioActor.hpp').read_text().replace('''    /* 0xB24 */ f32 _B24;
    /* 0xB28 */ f32 _B28;
    /* 0xB2C */ f32 _B2C;
    /* 0xB30 */ f32 _B30;''','''    /* 0xB24 */ TVec2f mScreenBoxMin;
    /* 0xB2C */ TVec2f mScreenBoxMax;''')
write('Game/Player/MarioActor.hpp',s)
s=(ROOT/'pc-port/src/Game/Player/MarioActorInit.cpp').read_text()
for a,c in [('_B24','mScreenBoxMin.x'),('_B28','mScreenBoxMin.y'),('_B2C','mScreenBoxMax.x'),('_B30','mScreenBoxMax.y')]:s=s.replace(a,c)
write('Game/Player/MarioActorInit.cpp',s)
s=(ROOT/'pc-port/src/Game/Player/DLchanger.hpp').read_text().replace('    void swap();','    DLholder* swap();')
write('Game/Player/DLchanger.hpp',s)
rootvec=(ROOT/'libs/JSystem/include/JSystem/JGeometry/TVec.hpp').read_text();nativevec=(ROOT/'pc-port/src/JSystem/JGeometry/TVec.hpp').read_text()
extra=extract(rootvec,'        void setMin(const TVec2< f32 >& min)','        inline bool isZero()')
if 'void setMin(const TVec2<' not in nativevec:nativevec=nativevec.replace('        void add(const TVec2& value)',extra+'        void add(const TVec2& value)',1)
write('JSystem/JGeometry/TVec.hpp',nativevec)
rootbox=(ROOT/'libs/JSystem/include/JSystem/JGeometry/TBox.hpp').read_text();nativebox=(ROOT/'pc-port/src/JSystem/JGeometry/TBox.hpp').read_text()
extra=extract(rootbox,'    template <>\n    struct TBox< TVec2< f32 > >','    template <>\n    struct TBox< TVec3< f32 > >')
nativebox=nativebox.replace('    template <>\n    struct TBox<TVec3<f32>>',extra+'    template <>\n    struct TBox<TVec3<f32>>')
extra=extract(rootbox,'    template < typename T >\n    struct TBox2','    template < typename T >\n    class TBox3')
nativebox=nativebox.replace('    template <typename T>\n    struct TBox3',extra+'    template <typename T>\n    struct TBox3')
nativebox+='\nusing TBox2f = JGeometry::TBox2<f32>;\nusing TBox2s = JGeometry::TBox2<s16>;\n'
write('JSystem/JGeometry/TBox.hpp',nativebox)
write('revolution/gx/GXFrameBuf.h', '#pragma once\n#include <dolphin/gx/GXFrameBuffer.h>\n')
patch=[]
for rel,(old,new) in changes.items():patch.extend(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='a/pc-port/src/'+rel,tofile='b/pc-port/src/'+rel))
for out in (BUILD,NOTES):
 (out/'native.patch').write_text(''.join(patch));(out/'native-files.json').write_text(json.dumps(list(changes),indent=2)+'\n')
print('staged',len(changes),'files')
