#include "Game/NPC/NPCActor.hpp"

#include "Game/LiveActor/LodCtrl.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/ActorShadowUtil.hpp"
#include "Game/Util/ActorSwitchUtil.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/RailUtil.hpp"
#include "Game/Util/StarPointerUtil.hpp"
#include "render/J3dMaterialRuntime.hpp"

#include <cmath>

namespace NrvNPCActorCompat {
    NEW_NERVE(NPCActorNrvReaction, NPCActor, Reaction);
    NEW_NERVE(NPCActorNrvWait, NPCActor, Wait);
    NEW_NERVE(NPCActorNrvTalk, NPCActor, Talk);
    NEW_NERVE(NPCActorNrvNull, NPCActor, Null);
}  // namespace NrvNPCActorCompat

namespace {
    constexpr auto cDegToRad = 3.14159265358979323846F / 180.0F;

    TQuat4f quat_from_euler_degree(const TVec3f& rotation) {
        const auto hx = rotation.x * cDegToRad * 0.5F;
        const auto hy = rotation.y * cDegToRad * 0.5F;
        const auto hz = rotation.z * cDegToRad * 0.5F;
        const auto sx = std::sin(hx);
        const auto cx = std::cos(hx);
        const auto sy = std::sin(hy);
        const auto cy = std::cos(hy);
        const auto sz = std::sin(hz);
        const auto cz = std::cos(hz);
        auto quat = TQuat4f{
            (sx * cy * cz) - (cx * sy * sz),
            (cx * sy * cz) + (sx * cy * sz),
            (cx * cy * sz) - (sx * sy * cz),
            (cx * cy * cz) + (sx * sy * sz),
        };
        quat.normalize();
        return quat;
    }

    smgpc::render::J3dMatrix3x4 actor_matrix(const NPCActor& actor) {
        auto side = TVec3f{};
        auto up = TVec3f{};
        auto front = TVec3f{};
        actor._A0.getXDir(side);
        actor._A0.getYDir(up);
        actor._A0.getZDir(front);
        return smgpc::render::J3dMatrix3x4{{
            side.x * actor.mScale.x,
            up.x * actor.mScale.y,
            front.x * actor.mScale.z,
            actor.mPosition.x,
            side.y * actor.mScale.x,
            up.y * actor.mScale.y,
            front.y * actor.mScale.z,
            actor.mPosition.y,
            side.z * actor.mScale.x,
            up.z * actor.mScale.y,
            front.z * actor.mScale.z,
            actor.mPosition.z,
        }};
    }
}  // namespace

NPCActorCaps::NPCActorCaps(const char* pName)
    : _0(pName), mModel(false), mObjectName(pName), mMakeActor(false), mHostIO(false), mMessage(false), _F(false), _10(pName),
      mMessageOffset(0.0F, 150.0F, 0.0F), mTalkMtx(nullptr), mTalkJointName(nullptr), mInterpole(false), mConnectTo(false),
      mLightCtrl(false), mEffect(false), mSound(false), mSoundSize(4), mAttribute(false), mPosition(false), mLodCtrl(false),
      mNerve(false), mBinder(false), mBinderSize(50.0F), mSensor(false), mSensorJoint(nullptr), mSensorSize(50.0F),
      mSensorOffset(0.0F, 50.0F, 0.0F), mSensorMax(1), mShadow(false), _5D(false), _5E(0), _5F(0), mShadowSize(50.0F),
      mRailRider(false), mSwitchDead(true), mSwitchAppear(false), _67(false), mPointer(false), _6C(nullptr), _70(nullptr),
      mStarPointerOffs(), mPointerSize(80.0F), mSceneConnectionType(0),
      mWaitNerve(&NrvNPCActorCompat::NPCActorNrvWait::sInstance), mTalkNerve(&NrvNPCActorCompat::NPCActorNrvTalk::sInstance),
      mReactionNerve(&NrvNPCActorCompat::NPCActorNrvReaction::sInstance) {
}

void NPCActorCaps::setDefault() {
    mMakeActor = true;
    mHostIO = true;
    mInterpole = true;
    mConnectTo = true;
    mLightCtrl = true;
    mEffect = true;
    mSound = true;
    mAttribute = true;
    mPosition = true;
    mLodCtrl = true;
    mNerve = true;
    mSensor = true;
    mBinder = true;
    mShadow = true;
    mRailRider = true;
    mSwitchDead = true;
    mSwitchAppear = true;
    mPointer = true;
    mModel = true;
    mMessage = true;
}

void NPCActorCaps::setIndirect() {
    mSceneConnectionType = 2;
}

NPCActor::NPCActor(const char* pName)
    : LiveActor(pName), mLodCtrl(nullptr), mMsgCtrl(nullptr), _94(nullptr), _98(nullptr), _9C(0), _A0(0.0F, 0.0F, 0.0F, 1.0F),
      _B0(0.0F, 0.0F, 0.0F, 1.0F), _C0(), _CC(), _D8(0), _D9(0), _DA(0), _DB(0), _DC(0), _DD(0), _DE(0), _DF(0),
      _E0(0), _E1(0), _E2(0), _E3(0), _E4(0), _E5(0), _E6(0), _E7(0), mParam{}, _10C(2.0F), _110(0.1F), _114(0.08F),
      _118(1.0F), _11C(nullptr), _120(nullptr), _124(0), _125(0), _126(0), _127(0), _128(1), _12C(0.0F), _130(nullptr),
      _134(nullptr), _138(nullptr), _13C(nullptr), mScaleController(nullptr), mDelegator(nullptr), mCurNerve(nullptr),
      mWaitNerve(&NrvNPCActorCompat::NPCActorNrvWait::sInstance), mTalkNerve(&NrvNPCActorCompat::NPCActorNrvTalk::sInstance),
      mReactionNerve(&NrvNPCActorCompat::NPCActorNrvReaction::sInstance), _158(0x400) {
    mParam._0 = 1;
    mParam._1 = 1;
    mParam._4 = 2000.0F;
    mParam._8 = 4.0F;
    mParam._C = 0.0F;
    mParam._10 = 0.0F;
    mParam._14 = nullptr;
    mParam._18 = nullptr;
    mParam._1C = nullptr;
    mParam._20 = nullptr;
}

void NPCActor::init(const JMapInfoIter& rIter) {
    LiveActor::init(rIter);
}

void NPCActor::initAfterPlacement() {
}

void NPCActor::initialize(const JMapInfoIter& rIter, const NPCActorCaps& rCaps) {
    if (rCaps.mPosition) {
        MR::initDefaultPos(this, rIter);
        _A0 = quat_from_euler_degree(mRotation);
        _B0 = _A0;
        _C0 = mPosition;
        _CC = mRotation;
    }
    if (rCaps.mModel) {
        initModelManagerWithAnm(rCaps.mObjectName, nullptr, false);
    }
    if (rCaps.mConnectTo) {
        if (rCaps.mSceneConnectionType == 0) {
            MR::connectToSceneNpc(this);
        } else {
            MR::connectToSceneNpcMovement(this);
        }
    }
    if (rCaps.mLightCtrl) {
        MR::initLightCtrl(this);
    }
    if (rCaps.mSound) {
        initSound(rCaps.mSoundSize, false);
    }
    if (rCaps.mNerve) {
        mWaitNerve = rCaps.mWaitNerve;
        mTalkNerve = rCaps.mTalkNerve;
        mReactionNerve = rCaps.mReactionNerve;
        if (mWaitNerve != nullptr) {
            initNerve(mWaitNerve);
        }
    }
    if (rCaps.mSensor) {
        initHitSensor(rCaps.mSensorMax);
        MR::addHitSensorNpc(this, "Body", 8U, rCaps.mSensorSize, rCaps.mSensorOffset);
    }
    if (rCaps.mBinder) {
        initBinder(rCaps.mBinderSize, rCaps.mBinderSize, 0U);
        MR::onCalcGravity(this);
    }
    if (rCaps.mEffect) {
        initEffectKeeper(0, nullptr, false);
    }
    if (rCaps.mShadow) {
        MR::initShadowVolumeSphere(this, rCaps.mShadowSize);
        MR::onCalcShadowOneTime(this, nullptr);
    }
    if (rCaps.mRailRider && MR::isConnectedWithRail(rIter)) {
        initRailRider(rIter);
        MR::moveCoordToStartPos(this);
    }
    if (rCaps.mPointer) {
        MR::initStarPointerTarget(this, rCaps.mPointerSize, rCaps.mStarPointerOffs);
    }
    if (rCaps.mMessage) {
        initTalkCtrl(rIter, rCaps._10, rCaps.mMessageOffset, rCaps.mTalkMtx);
    }
    if (rCaps.mLodCtrl) {
        mLodCtrl = MR::createLodCtrlNPC(this, rIter);
    }

    (void)MR::useStageSwitchReadA(this, rIter);
    (void)MR::useStageSwitchReadB(this, rIter);
    MR::useStageSwitchSleep(this, rIter);
    if (rCaps.mSwitchDead) {
        (void)MR::useStageSwitchWriteDead(this, rIter);
    }

    if (rCaps._67) {
        makeActorDead();
    } else if (rCaps.mSwitchAppear && MR::useStageSwitchReadAppear(this, rIter)) {
        MR::syncStageSwitchAppear(this);
        if (rCaps.mMakeActor) {
            makeActorDead();
        }
    } else if (rCaps.mMakeActor) {
        makeActorAppeared();
    }
}

bool NPCActor::initTalkCtrl(const JMapInfoIter& rIter, const char* pName, const TVec3f& rOffset, MtxPtr pMtx) {
    auto message_id = s32{-1};
    MR::getJMapInfoMessageID(rIter, &message_id);
    if (message_id < 0) {
        return false;
    }
    mMsgCtrl = MR::createTalkCtrl(this, rIter, pName, rOffset, pMtx);
    MR::onRootNodeAutomatic(mMsgCtrl);
    return true;
}

bool NPCActor::initTalkCtrlDirect(const JMapInfoIter& rIter, const char* pName, const TVec3f& rOffset, MtxPtr pMtx) {
    mMsgCtrl = MR::createTalkCtrlDirect(this, rIter, pName, rOffset, pMtx);
    MR::onRootNodeAutomatic(mMsgCtrl);
    return true;
}

void NPCActor::makeActorAppeared() {
    LiveActor::makeActorAppeared();
    if (mLodCtrl != nullptr) {
        mLodCtrl->appear();
    }
}

void NPCActor::makeActorDead() {
    if (mLodCtrl != nullptr) {
        mLodCtrl->kill();
    }
    LiveActor::makeActorDead();
}

void NPCActor::kill() {
    if (MR::isValidSwitchDead(this)) {
        MR::onSwitchDead(this);
    }
    LiveActor::kill();
}

void NPCActor::control() {
    if (mLodCtrl != nullptr) {
        mLodCtrl->update();
    }
}

void NPCActor::calcAndSetBaseMtx() {
    _A0.normalize();
    setBaseMatrix(actor_matrix(*this));
}

void NPCActor::attackSensor(HitSensor*, HitSensor*) {
}

bool NPCActor::receiveMsgPlayerAttack(u32, HitSensor*, HitSensor*) {
    return false;
}

void NPCActor::exeReaction() {
}

void NPCActor::exeWait() {
}

void NPCActor::exeTalk() {
}

void NPCActor::exeNull() {
}
