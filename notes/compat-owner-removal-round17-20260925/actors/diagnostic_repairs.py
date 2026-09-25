from ownership_edits import *
for e in json.loads(Path('notes/compat-owner-removal-round17-20260925/missing-game-source-imports.json').read_text()):
 status[e['destination']]='new central donor import before this lane'
def edit(path, old, new):
 s=Path(path).read_text()
 assert old in s, (path,old)
 write(path,s.replace(old,new))
def include(path,header):
 s=Path(path).read_text(); line='#include "'+header+'"\n'
 if line not in s:
  lines=s.splitlines(keepends=True);idx=max(i for i,l in enumerate(lines) if l.startswith('#include "Game/'))+1
  lines.insert(idx,line);write(path,''.join(lines))
for p,h in {
 'Boss/OtaKing.cpp':['Game/Util/FixedPosition.hpp'],
 'Enemy/CocoSambo.cpp':['Game/Util/FixedPosition.hpp'],
 'Enemy/OtaRock.cpp':['Game/Util/FixedPosition.hpp'],
 'Enemy/FireBall.cpp':['Game/Util/Color.hpp'],
 'Enemy/EyeBeamer.cpp':['Game/AreaObj/MercatorTransformCube.hpp'],
 'NPC/TrickRabbitSnow.cpp':['Game/Util/FootPrint.hpp'],
 'NPC/TrickRabbitFreeRun.cpp':['Game/Util/FootPrint.hpp'],
 'NPC/Kinopio.cpp':['Game/Util/NPCUtil.hpp'],
 'NPC/KinopioAstro.cpp':['Game/Util/NPCUtil.hpp','Game/Util/LayoutUtil.hpp'],
 'NPC/CareTaker.cpp':['Game/Util/NPCUtil.hpp'],
 'NPC/Syati.cpp':['Game/Util/LayoutUtil.hpp'],
}.items():
 for header in h:include('src/Game/'+p,header)
for p,old,new in [
 ('Boss/BossKameckStateBattle.cpp','mBattlePattarn->_10 == nullptr','!mBattlePattarn->_10'),
 ('Enemy/BallBeamer.cpp','initModelManagerWithAnm("BallBeamer", nullptr, nullptr)','initModelManagerWithAnm("BallBeamer", nullptr, false)'),
 ('Enemy/BallBeamer.cpp','initEffectKeeper(3, nullptr, nullptr)','initEffectKeeper(3, nullptr, false)'),
 ('Enemy/BallBeamer.cpp','initSound(2, nullptr)','initSound(2, false)'),
 ('Enemy/HammerHeadPackun.cpp','jmap->mData->mNumEntries : nullptr','jmap->mData->mNumEntries : 0'),
 ('Enemy/OtaRock.cpp','mThrowCocoNutCounter(nullptr)','mThrowCocoNutCounter(0)'),
 ('Enemy/BegomanBase.cpp','MR::getWaterAreaObj(&info, mPosition) != nullptr','MR::getWaterAreaObj(&info, mPosition)'),
 ('Enemy/KirairaChain.cpp','MR::getRandom(0L, mPointCount - 1)','MR::getRandom(0, mPointCount - 1)'),
 ('NPC/TicoRail.cpp','MR::getRandom(0, 2l)','MR::getRandom(0, 2)'),
 ('NPC/Syati.cpp','initEffectKeeper(1, nullptr, nullptr)','initEffectKeeper(1, nullptr, false)'),
 ('Enemy/KoopaJrShip.cpp','std::not1(std::ptr_fun(&MR::isDead))','[](const LiveActor* actor) { return !MR::isDead(actor); }'),
 ('NPC/NPCActor.hpp','template < typename T >\nclass JointControlDelegator;','class JointController;'),
 ('NPC/NPCActor.hpp','JointControlDelegator< NPCActor >* mDelegator','JointController* mDelegator'),
]:edit('src/Game/'+p,old,new)
p='src/Game/Boss/BossStinkBug.hpp'
s=Path(p).read_text();s=s[:s.index('\nclass BossStinkBugFollowValidater :')]
s=s.replace('#include "Game/Util/BaseMatrixFollowTargetHolder.hpp"','#include <JSystem/JGeometry/TMatrix.hpp>')
write(p,s)
save()
