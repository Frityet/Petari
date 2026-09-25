from ownership_edits import *
for e in json.loads(Path('notes/compat-owner-removal-round17-20260925/missing-game-source-imports.json').read_text()):status[e['destination']]='new central donor import before this lane'
# Restore missing utilities without replacing previously ported bodies.
p='src/Game/Util/NPCUtil.cpp';s=Path(p).read_text();donor=Path('decomp/src/Game/Util/NPCUtil.cpp').read_text()
missing=['initDefaultPose','turnPlayerToActor','setNPCActorPose','setDefaultPose','convertPosOnGround','isActionContinuous','invalidateLodCtrl','tryTalkNearPlayerAndStartMoveTalkAction','tryTalkForceAtEndAndStartTalkAction','tryChangeTalkActionRandom']
blocks=[]
for match in re.finditer(r'^    (?:void|bool) (\w+)\([^\n]*\) \{',donor,re.M):
 name=match.group(1)
 if name not in missing and not (name=='setNPCActorPos' and 'const TVec3f&' in match.group(0)):continue
 start=match.start();end=donor.index('\n    }',match.end())+len('\n    }')
 blocks.append(donor[start:end])
assert len(blocks)==11
pos=s.index('namespace MR {')+len('namespace MR {')
s=s[:pos]+'\n'+ '\n\n'.join(blocks)+'\n'+s[pos:]
s=s.replace('#include "Game/LiveActor/ModelObj.hpp"','#include "Game/LiveActor/ModelObj.hpp"\n#include "Game/LiveActor/LodCtrl.hpp"\n#include "Game/Map/HitInfo.hpp"')
write(p,s)
# Complete original physics/control implementation; only native parser/string lifetime differs.
p='src/Game/LiveActor/DynamicJointCtrl.hpp';s=Path('decomp/include/Game/LiveActor/DynamicJointCtrl.hpp').read_text()
s=s.replace('#include <revolution.h>','#include <revolution.h>\n#include <memory>')
s=s.replace('    /* 0x14 */ JointCtrlRate* mControlRate;','    /* 0x14 */ JointCtrlRate* mControlRate;\n\nprivate:\n    std::unique_ptr< char[] > mNativeName;')
write(p,s)
p='src/Game/LiveActor/DynamicJointCtrl.cpp';s=Path('decomp/src/Game/LiveActor/DynamicJointCtrl.cpp').read_text()
s=s.replace('#include "Game/Util.hpp"','#include "Game/Util.hpp"\n#include "Game/Util/JMapInfo.hpp"')
s=s.replace('#include <JSystem/J3DGraphAnimator/J3DJoint.hpp>','#include <JSystem/J3DGraphAnimator/J3DJoint.hpp>\n#include <cstring>\n#include <memory>')
s=s.replace('template const bool JMapInfo::getValue< f32 >(int, const char*, f32*) const;\n\n','')
s=s.replace(': mActor(pActor), mName(pName), _8(), mCtrlNodes(), mParams(pParam), mControlRate(new JointCtrlRate) {\n}',': mActor(pActor), mName(pName), _8(), mCtrlNodes(), mParams(pParam), mControlRate(new JointCtrlRate) {\n    if (pName != nullptr) {\n        const std::size_t length = std::strlen(pName) + 1;\n        mNativeName = std::make_unique< char[] >(length);\n        std::memcpy(mNativeName.get(), pName, length);\n        mName = mNativeName.get();\n    }\n}')
s=s.replace('JMapInfo* csv = MR::tryCreateCsvParser(resourceHolder, "%s.bcsv", ::sTextOutFileName);','std::unique_ptr< JMapInfo > csv(MR::tryCreateCsvParser(resourceHolder, "%s.bcsv", ::sTextOutFileName));')
write(p,s)
for name in ['TripodBoss','SkeletalFishGuard']:
 p='src/Game/Boss/'+name+'.cpp';s=Path(p).read_text();assert name+'::~'+name not in s
 s+='\n'+name+'::~'+name+'() = default;\n';write(p,s)
save()
