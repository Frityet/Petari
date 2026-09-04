from pathlib import Path
import shutil,re,json,subprocess,hashlib
R=Path(__file__).resolve().parents[3];N=Path(__file__).resolve().parent;B=R/'build/original-predraw-scheduler-20260903';S=B/'staged'
def write(rel,text):
 p=S/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text)
for name in ['NameObjCategoryList','NameObjListExecutor']:
 write('Game/NameObj/'+name+'.cpp',(R/'src/Game/NameObj'/(name+'.cpp')).read_text())
 write('Game/NameObj/'+name+'.hpp',(R/'include/Game/NameObj'/(name+'.hpp')).read_text())
header=(S/'Game/NameObj/NameObjCategoryList.hpp').read_text()
header=header.replace('namespace {','class NameObjDelegator {\npublic:\n    virtual ~NameObjDelegator() = default;\n    virtual void operator()(NameObj*) = 0;\n};\n\nnamespace {',1)
header=header.replace('class NameObjRealDelegator {','class NameObjRealDelegator : public NameObjDelegator {')
header=header.replace('NameObjRealDelegator< NameObjMethod >* mDelegator;', 'NameObjDelegator* mDelegator;').replace('NameObjRealDelegator< NameObjMethodConst >* mDelegatorConst;', 'NameObjDelegator* mDelegatorConst;')
write('Game/NameObj/NameObjCategoryList.hpp',header)
for rel in ['Game/Util/Functor.hpp','Game/Util/ObjUtil.hpp']:
 write(rel,(R/'pc-port/notes/original-particle-draw-executor-20260903/native'/rel).read_text())
scene=(R/'pc-port/src/Game/Scene/SceneFunction.hpp').read_text()
if 'enum CameraType' not in scene:
 camera=re.search(r'    enum CameraType \{.*?    \};', (R/'include/Game/Scene/SceneFunction.hpp').read_text(),re.S).group()
 scene=scene.replace('namespace MR {','namespace MR {\n'+camera,1)
write('Game/Scene/SceneFunction.hpp',scene)
source=(R/'src/Game/Scene/SceneNameObjListExecutor.cpp').read_text()
table=re.search(r'    const CategoryListInitialTable cDrawListInitTable\[\] = \{.*?\n    \};',source,re.S).group()
table=table.replace('-1,  // TODO "None" DrawType name?', 'static_cast<u32>(-1),  // Original unsigned sentinel.')
write('scene/DrawCategoryInitialTable.inc',table+'\n')
write('scene/DrawBufferInitialTable.inc',(R/'pc-port/src/scene/DrawBufferInitialTable.inc').read_text())
# Further staged service/scheduler files are maintained as reviewable native copies.
for path in (N/'native').rglob('*') if (N/'native').exists() else []:
 if path.is_file():write(str(path.relative_to(N/'native')),path.read_text())
