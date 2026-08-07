#include "Game/NPC/NPCActor.hpp"
#include "Game/Enemy/AnimScaleController.hpp"
#include "Game/LiveActor/LodCtrl.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/LiveActor/PartsModel.hpp"
#include "Game/LiveActor/Spine.hpp"
#include "Game/NPC/NPCActorItem.hpp"
#include "Game/NameObj/NameObjArchiveListCollector.hpp"
#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/ActorShadowUtil.hpp"
#include "Game/Util/ActorSwitchUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/JointController.hpp"
#include "Game/Util/JointUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/NPCUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/RailUtil.hpp"
#include "Game/Util/StarPointerUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include "JSystem/JGeometry/TUtil.hpp"
#include "JSystem/JMath/JMATrigonometric.hpp"

namespace MR {
    bool isNullOrEmptyString(const char*);
    void makeQuatRotateRadian(TQuat4f*, const TVec3f&);
    void makeQuatRotateDegree(TQuat4f*, const TVec3f&);
    void setBaseTRMtx(LiveActor*, const TQuat4f&);
    void calcGravity(LiveActor*);
    bool isExistBck(const LiveActor*, const char*);
    void startBckNoInterpole(const LiveActor*, const char*);
    void setBckFrameAtRandom(const LiveActor*);
    void calcAnimDirect(LiveActor*);
    void connectToSceneIndirectNpc(LiveActor*);
    void addToAttributeGroupSearchTurtle(const LiveActor*);
    void initStarPointerTargetAtJoint(LiveActor*, const char*, f32, const TVec3f&);
    bool isStarPointerPointing2POnPressButton(const LiveActor*, const char*, bool, bool);
    void makeAxisFrontUp(TVec3f*, TVec3f*, const TVec3f&, const TVec3f&);
    void clampVecAngleDeg(TVec3f*, const TVec3f&, f32);
    bool turnQuatYDirRad(TQuat4f*, const TQuat4f&, const TVec3f&, f32);
    bool isSameDirection(const TVec3f&, const TVec3f&, f32);
    bool isOppositeDirection(const TVec3f&, const TVec3f&, f32);
    inline f32 acos(f32 value) {
        return JGeometry::TUtil<f32>::acos(value);
    }

    JointControlDelegator<NPCActor>* createNPCActorJointDelegator(NPCActor*, const char*);
}  // namespace MR

const void* smgpcNPCActorModelPresence(const LiveActor*);
const void* smgpcNPCActorStarPointerPresence(const LiveActor*);

template <typename Tag, typename Tag::type Member>
struct NPCActorPrivateMemberBridge {
    friend typename Tag::type getNPCActorPrivateMember(Tag) {
        return Member;
    }
};

struct NPCActorSpineMember {
    using type = Spine* LiveActor::*;
    friend type getNPCActorPrivateMember(NPCActorSpineMember);
};

template struct NPCActorPrivateMemberBridge<NPCActorSpineMember, &LiveActor::mSpine>;

inline Spine* smgpcNPCActorSpine(LiveActor* actor) {
    return actor != nullptr ? actor->*getNPCActorPrivateMember(NPCActorSpineMember{}) : nullptr;
}

#define mModelManager smgpcNPCActorModelPresence(this)
#define mStarPointerTarget smgpcNPCActorStarPointerPresence(this)
#define mSpine smgpcNPCActorSpine(this)
#define createJointDelegatorWithNullChildFunc(host, function, name) createNPCActorJointDelegator(host, name)
#include "Game/NPC/NPCActor.cpp"
#undef createJointDelegatorWithNullChildFunc
#undef mSpine
#undef mStarPointerTarget
#undef mModelManager
