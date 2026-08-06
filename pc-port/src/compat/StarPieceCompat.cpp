#include "Game/MapObj/StarPiece.hpp"

#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/MapObj/StarPieceGroup.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"

#include <unordered_map>

namespace NrvStarPieceCompat {
    NEW_NERVE(StarPieceNrvFloating, StarPiece, Floating);
    NEW_NERVE(StarPieceNrvRailMove, StarPiece, RailMove);
    NEW_NERVE(StarPieceNrvFall, StarPiece, Fall);
}  // namespace NrvStarPieceCompat

namespace {
    auto sDeclaredStarPieces = std::unordered_map<const NameObj*, s32>{};

    constexpr GXColor cStarPieceColors[] = {
        {0x80, 0x00, 0x99, 0xFF},
        {0xE6, 0xA0, 0x00, 0xFF},
        {0x46, 0xA1, 0x08, 0xFF},
        {0x37, 0x5A, 0xA0, 0xFF},
        {0xBE, 0x33, 0x0B, 0xFF},
        {0x80, 0x80, 0x80, 0xFF},
    };
}  // namespace

StarPiece::StarPiece(const char* pName)
    : LiveActor(pName), _8C(0.0F, 0.0F, 1.0F), _98(0.02F), _9C(0.0F), _A0(0.0F), _A4(0.0F), _A8(0.0F, 0.0F, 0.0F),
      _B4(0.0F, 0.0F, 0.0F), mDelegator(nullptr), mTargetSensor(nullptr), _C8(-1), mGettableDelayCounter(-1), mFallKillTimer(0),
      mColor(cStarPieceColors[0]), mGroupType(groupType_noGroup), mHostInfo(nullptr), mReceiverInfo(nullptr), mNumGift(1) {
    mFlags.isGoToPlayer = false;
    mFlags._1 = true;
    mFlags._2 = false;
    mFlags._3 = false;
    mFlags.isGroup = false;
    mFlags.InWater = false;
    mFlags._6 = false;
    mFlags.isLaunched = false;
}

void StarPiece::initAndSetFloatingFromGroup(const JMapInfoIter& rIter) {
    mGroupType = groupType_FloatingGroup;
    init(rIter);
    MR::invalidateClipping(this);
    mFlags.isGroup = true;
}

void StarPiece::initAndSetRailMoveFromGroup(const JMapInfoIter& rIter) {
    mGroupType = groupType_RailMoveGroup;
    init(rIter);
    MR::invalidateClipping(this);
    mFlags.isGroup = true;
}

void StarPiece::init(const JMapInfoIter& rIter) {
    if (MR::isValidInfo(rIter)) {
        MR::initDefaultPos(this, rIter);
    }

    initModelManagerWithAnm("StarPiece", nullptr, true);
    MR::connectToSceneNoSilhouettedMapObj(this);
    mScale.set(1.0F);
    initBinder(40.0F, 0.0F, 0U);
    if (mGroupType == groupType_RailMoveGroup) {
        initNerve(&NrvStarPieceCompat::StarPieceNrvRailMove::sInstance);
    } else {
        initNerve(&NrvStarPieceCompat::StarPieceNrvFloating::sInstance);
    }

    initEffectKeeper(0, "StarPiece", false);
    initSound(4, false);
    initHitSensor(2);
    MR::addHitSensorEye(this, "attack", 16, 30.0F, TVec3f(0.0F, 0.0F, 0.0F));
    MR::addHitSensor(this, "body", ATYPE_STAR_PIECE, 16, 30.0F, TVec3f(0.0F, 0.0F, 0.0F));

    if (mGroupType == groupType_noGroup) {
        appear();
    } else {
        makeActorDead();
    }
}

void StarPiece::initAfterPlacement() {
}

void StarPiece::appear() {
    LiveActor::appear();
}

void StarPiece::makeActorAppeared() {
    LiveActor::makeActorAppeared();
    if (auto* attack = getSensor("attack")) {
        attack->invalidate();
    }
    _C8 = -1;
}

void StarPiece::kill() {
    LiveActor::kill();
}

void StarPiece::makeActorDead() {
    LiveActor::makeActorDead();
    mTargetSensor = nullptr;
    mReceiverInfo = nullptr;
    _C8 = -1;
    mFlags._6 = false;
    mFlags.isLaunched = false;
    mHostInfo = nullptr;
}

void StarPiece::startClipped() {
    LiveActor::startClipped();
    _C8 = -1;
}

void StarPiece::control() {
}

void StarPiece::calcAndSetBaseMtx() {
    LiveActor::calcAndSetBaseMtx();
}

void StarPiece::attackSensor(HitSensor*, HitSensor*) {
}

bool StarPiece::receiveOtherMsg(u32 msg, HitSensor*, HitSensor*) {
    if (MR::isMsgItemGet(msg) || MR::isMsgInhaleBlackHole(msg)) {
        makeActorDead();
        return true;
    }
    return false;
}

void StarPiece::setColor(s32 colorIndex) {
    if (colorIndex >= 0 && colorIndex < getNumColor()) {
        mColor = cStarPieceColors[colorIndex];
    }
}

s32 StarPiece::getNumColor() {
    return static_cast<s32>(sizeof(cStarPieceColors) / sizeof(cStarPieceColors[0]));
}

void StarPiece::appearFromGroup() {
    appear();
    if (mGroupType == groupType_RailMoveGroup) {
        setNerve(&NrvStarPieceCompat::StarPieceNrvRailMove::sInstance);
    } else {
        setNerve(&NrvStarPieceCompat::StarPieceNrvFloating::sInstance);
    }
}

void StarPiece::changeScale(f32 scale) {
    mScale.set(scale);
}

void StarPiece::exeFloating() {
    mRotation.y += 1.0F;
    if (mRotation.y >= 360.0F) {
        mRotation.y -= 360.0F;
    }
}

void StarPiece::exeRailMove() {
    exeFloating();
}

void StarPiece::exeFall() {
    mVelocity.add(mGravity);
}

bool StarPiece::setFall() {
    if (MR::isDead(this) || (!isOnRailMove() && !isFloat())) {
        return false;
    }
    mFlags._1 = true;
    setNerve(&NrvStarPieceCompat::StarPieceNrvFall::sInstance);
    return true;
}

bool StarPiece::isOnRailMove() {
    return !MR::isDead(this) && isNerve(&NrvStarPieceCompat::StarPieceNrvRailMove::sInstance);
}

bool StarPiece::isFloat() {
    return !MR::isDead(this) && isNerve(&NrvStarPieceCompat::StarPieceNrvFloating::sInstance);
}

namespace MR {
    void declareStarPiece(const NameObj* pObj, s32 num) {
        if (pObj != nullptr) {
            sDeclaredStarPieces[pObj] = num;
        }
    }
}  // namespace MR

namespace smgpc::compat {
    std::size_t declared_star_piece_count(const NameObj* owner) {
        const auto found = sDeclaredStarPieces.find(owner);
        return found != sDeclaredStarPieces.end() && found->second > 0 ? static_cast<std::size_t>(found->second) : 0U;
    }

    void release_star_piece_runtime_state(const LiveActor* actor) {
        auto* group = dynamic_cast<const StarPieceGroup*>(actor);
        if (group == nullptr) {
            return;
        }

        sDeclaredStarPieces.erase(group);
        auto* mutable_group = const_cast<StarPieceGroup*>(group);
        if (mutable_group->mPieces != nullptr) {
            for (auto index = s32{}; index < mutable_group->mNumPieces; ++index) {
                if (mutable_group->mPieces[index] != nullptr) {
                    release_actor_runtime_state(mutable_group->mPieces[index]);
                    delete mutable_group->mPieces[index];
                }
            }
            delete[] mutable_group->mPieces;
            mutable_group->mPieces = nullptr;
        }
        delete[] mutable_group->mRailCoords;
        mutable_group->mRailCoords = nullptr;
        mutable_group->mNumPieces = 0;
    }
}  // namespace smgpc::compat
