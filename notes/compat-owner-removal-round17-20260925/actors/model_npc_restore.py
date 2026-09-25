from ownership_edits import *
p='src/Game/LiveActor/ModelObj.hpp';s=Path(p).read_text();donor=Path('decomp/include/Game/LiveActor/ModelObj.hpp').read_text()
s=s.replace('class ModelObj : public LiveActor {','class ActorJointCtrl;\nclass LodCtrl;\n\nclass ModelObj : public LiveActor {')
s+='\n'+donor[donor.index('class ModelObjNpc :'):]
write(p,s)
p='src/Game/LiveActor/ModelObj.cpp';s=Path(p).read_text();donor=Path('decomp/src/Game/LiveActor/ModelObj.cpp').read_text()
s=s.replace('#include "Game/LiveActor/ModelObj.hpp"','#include "Game/LiveActor/ModelObj.hpp"\n#include "Game/LiveActor/ActorJointCtrl.hpp"\n#include "Game/LiveActor/LodCtrl.hpp"')
s=s.replace('#include "Game/Util/LiveActorUtil.hpp"','#include "Game/Util/ActorShadowUtil.hpp"\n#include "Game/Util/LiveActorUtil.hpp"\n#include "Game/Util/ObjUtil.hpp"')
s+='\n'+donor[donor.index('void ModelObjNpc::init('):]
write(p,s)
p='src/Game/NPC/KinopioAstro.cpp';s=Path(p).read_text();s=s.replace('u32 JKRArchive::getExpandedResSize(const void* pResource) const {\n    return getResSize(pResource);\n}\n\n','');write(p,s)
save()
