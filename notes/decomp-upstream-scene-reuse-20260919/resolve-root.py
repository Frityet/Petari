"""Documented file-specific reconciliation of the Util/library merge lane."""
from pathlib import Path
import subprocess,re,json
root=Path(__file__).resolve().parents[2]; repo=root/'decomp'
def version(path,rev): return subprocess.check_output(['git','show',f'{rev}:{path}'],cwd=repo,text=True)
def write(path,text): (repo/path).write_text(text)
def choose(path,side):
 p=repo/path; text=p.read_text()
 pattern=r'^<<<<<<< HEAD\n(.*?)^=======\n(.*?)^>>>>>>> upstream/master\n'
 text,n=re.subn(pattern,lambda m:m.group(side),text,flags=re.M|re.S)
 assert n,path
 write(path,text)
def body(text,name):
 # Function blocks only; brace scanning is sufficient for these reviewed functions.
 pattern=rf'^    [^\n]*\b{re.escape(name)}\([^;{{}}]*\)[^;{{}}]*\{{'
 m=re.search(pattern,text,re.M); assert m,name
 i=m.end(); depth=1
 while depth:
  depth+=(text[i]=='{')-(text[i]=='}'); i+=1
 return text[m.start():i]
# These incoming recoveries contain all local method implementations, improve names
# and match the corresponding incoming headers. Keep each dependency family together.
full=['include/Game/Util/'+x+'.hpp' for x in ['FurCtrl','FurDrawer','FurMulti','FurParam','FurShader','DirectDraw','AreaObjUtil','MapPartsUtil']]
full+=['src/Game/Util/'+x+'.cpp' for x in ['FurCtrl','FurDrawer','FurMulti','FurShader','AreaObjUtil','DirectDraw','MapPartsUtil','JointController','SceneUtil','StringUtil','TalkUtil','LayoutUtil']]
full+=['libs/JSystem/include/JSystem/J3DGraphAnimator/J3DModel.hpp','libs/nw4r/include/nw4r/db/assert.h','libs/nw4r/include/nw4r/lyt/material.h','libs/nw4r/include/nw4r/ut/LinkList.h','libs/nw4r/include/nw4r/ut/TagProcessorBase.h']
for path in full: write(path,version(path,'upstream/master'))
# Upstream has the cursor/font accessors too, but not the individual setters.
p='libs/nw4r/include/nw4r/ut/CharWriter.h'; t=version(p,'upstream/master')
t=t.replace('            void SetScale(f32 hScale, f32 vScale) {','            void SetCursorX(f32 x) {\n                mCursorPos.x = x;\n            }\n\n            void SetCursorY(f32 y) {\n                mCursorPos.y = y;\n            }\n\n            void SetScale(f32 hScale, f32 vScale) {')
write(p,t)
# Preserve completed generic template bodies/extra const functor overloads.
for p in ['include/Game/Util/Array.hpp','include/Game/Util/Functor.hpp','libs/JSystem/include/JSystem/JGeometry/TPartition3.hpp']:
 choose(p,1)
for p in ['libs/MSL_C++/include/algorithm','libs/MSL_C++/include/functional.hpp']:
 choose(p,2)
# Keep both constructors. The local bool overload is recovered, upstream adds default.
p='include/Game/Util/BothDirList.hpp'; choose(p,1)
t=(repo/p).read_text().replace('        BothDirPtrList(bool doInit) {','        BothDirPtrList() {\n            initiate();\n        }\n\n        BothDirPtrList(bool doInit) {'); write(p,t)
# Keep both original joint lookup overloads rather than discarding either.
p='include/Game/Util/JointController.hpp'; local=version(p,'HEAD'); t=version(p,'upstream/master')
func=body(local,'createJointController'); marker='    template < class T >\n    JointController* createJointController('
t=t.replace(marker,'    template < class T >\n'+func+'\n\n'+marker,1); write(p,t)
# Preserve local scope around goto target in isBindRoof, retaining other auto-merged upstream changes.
choose('src/Game/Util/LiveActorUtil.cpp',1)
# Upstream's MapUtil has completed ground helpers; retain the existing sound-code helper.
p='src/Game/Util/MapUtil.cpp'; local=version(p,'HEAD'); t=version(p,'upstream/master')
func=body(local,'isSoundCodeSand'); t=t.replace('    bool isCodeSand(',func+'\n\n    bool isCodeSand(',1); write(p,t)
# ObjUtil incoming qualified namespace functions still leave five recovered bodies as declarations.
p='src/Game/Util/ObjUtil.cpp'; local=version(p,'HEAD'); t=version(p,'upstream/master')
for name in ['getCsvDataBool','getCsvDataVec','getCsvDataColor','findNamePosOnGround','tryFindLinkNamePos']:
 func=body(local,name); func='\n'.join(s[4:] if s.startswith('    ') else s for s in func.splitlines())
 func=re.sub(r'^(\w+\s+)(\w+)(\()',r'\1MR::\2\3',func)
 t,n=re.subn(r'^\w+ MR::'+name+r'\([^;\n]*\);$',lambda m:func,t,count=1,flags=re.M); assert n,name
for inc in ['Game/Util/MtxUtil.hpp','Game/Util/GravityUtil.hpp','Game/Util/MapUtil.hpp','Game/Map/HitInfo.hpp']:
 t=t.replace('#include "Game/Util/ObjUtil.hpp"','#include "Game/Util/ObjUtil.hpp"\n#include "'+inc+'"',1)
write(p,t)
print(json.dumps({'incoming_complete_files':full,'special_union_files':13},indent=2))
