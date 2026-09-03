#include "Game/Player/MarioTeresa.hpp"

#include "Game/Animation/XanimePlayer.hpp"
#include "Game/Animation/XanimeResource.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/MapObj/BigFanHolder.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioAnimator.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Player/MarioParts.hpp"
#include "Game/Util/EffectUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "Game/Util/StringUtil.hpp"

namespace {
    struct TeresaGroupInfoRaw {
        const char* mName;
        f32 mRate;
        u32 _8;
        f32 mStart;
        f32 mEnd;
        f32 mLoop;
        u32 mAttribute;
        u8 mBckTableVariant;
        u8 _1D[3];
        void* _20[4];
        f32 mWeights[4];
        XanimeOfsInfo** _40;
        u32 mHash;
        const char* mBckName;
    };

    struct TeresaBckTable2Raw {
        const char* mName;
        XanimeBckTableEntry mEntries[2];
    };

    const char* const cTeresaBase = "基本";
    const char* const cTeresaBlink = "blink";
    const char* const cTeresaSleep = "sleep";
    const char* const cTeresaSleepEffect = "Sleep";
    const char* const cTeresaWait = "wait";
    const char* const cTeresaSpin = "spin";
    const char* const cTeresaFall = "fall";
    const char* const cTeresaRun = "run";
    const char* const cTeresaFly = "fly";

    XanimePlayer* getTeresaXanimePlayer(const MarioActor* pActor) {
        return reinterpret_cast< XanimePlayer* >(pActor->_9B8);
    }

    XanimeResourceTable* getTeresaResourceTable(const MarioActor* pActor) {
        return reinterpret_cast< XanimeResourceTable* >(pActor->_9BC);
    }
}

TeresaGroupInfoRaw teresaAnimeTable[2] = {
    {cTeresaBase, 1.0f, 0x10, 0.0f, 0.0f, 0.0f, 0, 0, {0, 0, 0}, {nullptr, nullptr, nullptr, nullptr},
     {0.0f, 0.0f, 0.0f, 0.0f}, nullptr, 0, nullptr},
    {"", 0.0f, 0, 0.0f, 0.0f, 0.0f, 0, 0, {0, 0, 0}, {nullptr, nullptr, nullptr, nullptr},
     {0.0f, 0.0f, 0.0f, 0.0f}, nullptr, 0, nullptr},
};

TeresaBckTable2Raw teresaAnime2[2] = {
    {cTeresaBase, {{cTeresaWait, 1.0f}, {cTeresaRun, 0.0f}}},
    {"", {{"", 0.0f}, {nullptr, 0.0f}}},
};

void Mario::startTeresaMode() {
    _418 = 0;
    _428 = 0;
    mMovementStates.jumping = true;
    mMovementStates._1 = false;
    mMovementStates._22 = false;
    mJumpVec.zero();
    changeStatus(mTeresa);
}

void MarioTeresa::updateDropFlag() {
    _58 = false;

    if (_34.dot(getAirGravityVec()) <= 1.0f) {
        return;
    }

    TVec3f horizontal;
    const f32 vertical = MR::vecKillElement(_34, getAirGravityVec(), &horizontal);
    if (MR::isNearZero(getStickP(), 0.001f) && horizontal.length() < vertical) {
        _58 = true;
    }
}

bool Mario::getHitWallNorm(TVec3f* pNorm) {
    const Triangle* pTriangle = nullptr;

    if (mMovementStates._8) {
        pTriangle = mFrontWallTriangle;
    } else if (mMovementStates._19) {
        pTriangle = mBackWallTriangle;
    } else if (mMovementStates._1A) {
        pTriangle = mSideWallTriangle;
    }

    if (pTriangle == nullptr || isThroughWall(pTriangle)) {
        return false;
    }

    *pNorm = *pTriangle->getNormal(0);
    return true;
}

void Mario::resetTeresaMode() {
    mActor->_F44 = true;
    _418 = 0;
    mTeresa->resetTeresaMode();
}

void MarioTeresa::resetTeresaMode() {
    _42 = 0;
    _44 = 0;
    _46 = 0;
    _48 = 0;
    _50 = 0.0f;
    _34.zero();
    _28.zero();
}

bool MarioTeresa::isTeresaAccel() const {
    return _44 != 0;
}

MarioTeresa::MarioTeresa(MarioActor* pActor)
    : MarioState(pActor, MarioStatus_Teresa), _14(), _20(0.0f), _24(0.0f), _28(), _34(), _40(0), _42(0), _44(0), _46(0),
      _48(0), _4A(0), _4C(0.0f), _50(0.0f), _54(0.0f), _58(false), _59(false), _5A() {
    _14.zero();
    resetTeresaMode();
}

bool MarioTeresa::close() {
    return true;
}

bool MarioTeresa::update() {
    if (getPlayerMode() != 6) {
        return false;
    }

    if (getPlayer()->isStatusActive(MarioStatus_13) || mActor->_EA4) {
        return true;
    }

    _50 = 0.0f;
    _14 = getJumpVec();

    if (getPlayer()->_418 == 0) {
        _58 = false;
    }

    getPlayer()->mMovementStates._B = false;
    _20 = checkHeight();
    _24 = mActor->mConst->getTable()->mTeresaWaitHeight;

    checkAccel();
    checkWind();
    checkGroundReflect();
    checkWallCeilReflect();
    addVelocity(_34);
    procNoControl();
    procNearGroundControl();
    procAirControl();
    procDrop();

    getPlayer()->_3BC = 1;
    procControl();

    f32 speedRatio = _14.length() / 10.0f;
    if (speedRatio < 0.01f) {
        speedRatio = 0.0f;
    }
    getPlayer()->mWalkSpeed = MR::clamp(speedRatio, 0.0f, 1.0f);
    setJumpVec(_14);

    if (_42 == 0) {
        getPlayer()->playSoundTeresaFlying();
    }

    return true;
}

f32 MarioTeresa::checkHeight() {
    const MarioConstTable* pTable = mActor->mConst->getTable();
    f32 nearestHeight = getPlayer()->mVerticalSpeed;

    for (f32 searchOffset = 0.0f; searchOffset < 2.0f * pTable->mTeresaWaitHeight; searchOffset += 120.0f) {
        const TVec3f center = mActor->_2A0 + getGravityVec() * searchOffset;
        const u32 strikeCount = Collision::checkStrikeBallToMapWithThickness(center, 120.0f, 120.0f, nullptr, nullptr);
        bool foundGround = false;

        for (u32 i = 0; i < strikeCount; i++) {
            const HitInfo* pHit = Collision::getStrikeInfoMap(i);
            TVec3f normal(*pHit->mParentTriangle.getNormal(0));
            TVec3f toHit(pHit->mHitPos - mActor->_2A0);
            f32 height = toHit.length() - 110.0f;
            if (height < 0.0f) {
                height = 0.0f;
            }

            MR::normalizeOrZero(&toHit);
            if (normal.dot(getGravityVec()) < -0.1f && toHit.dot(getGravityVec()) > 0.1f) {
                if (nearestHeight > height) {
                    nearestHeight = height;
                }
                foundGround = true;
            }
        }

        if (foundGround) {
            break;
        }
    }

    return nearestHeight;
}

void MarioTeresa::checkAccel() {
    if (checkTrgA()) {
        if (_44 == 0) {
            getPlayer()->_1C._4 = true;
        }
        _44 = mActor->mConst->getTable()->mTeresaAccelTime;
    }

    if (_44 != 0) {
        _44--;
    }
}

void MarioTeresa::checkGroundReflect() {
    if (getPlayer()->mMovementStates._1) {
        getPlayer()->mMovementStates.jumping = false;
    }

    if (getPlayer()->mMovementStates.jumping || getPlayer()->_3CE == 0) {
        return;
    }

    getPlayer()->mDrawStates._C = false;
    getPlayer()->tryJump();
    cutGravityElementFromJumpVec(true);

    TVec3f horizontal;
    const f32 vertical = MR::vecKillElement(_34, getAirGravityVec(), &horizontal);
    _34 = horizontal;
    if (vertical < 0.0f) {
        _34 += getAirGravityVec() * vertical;
    }
}

void MarioTeresa::procNoControl() {
    if (_46 != 0) {
        _46--;
        _48 = 0;
        _34.scale(mActor->mConst->getTable()->mTeresaWallReflectReduction);
    }

    if (MR::isNearZero(getStickP(), 0.001f)) {
        _34.scale(0.99f);
    }
}

void MarioTeresa::procNearGroundControl() {
    if (MR::isNearZero(getVelocity(), 0.001f)) {
        return;
    }

    const TVec3f& rGroundNormal = getPlayer()->getShadowNorm();
    if (calcAngleD(rGroundNormal) >= 60.0f || _20 >= _24 + 50.0f || _46 != 0 || _34.dot(rGroundNormal) >= 0.0f) {
        return;
    }

    const f32 speed = _34.length();
    TVec3f tangent(_34);
    if (MR::normalizeOrZero(&tangent)) {
        return;
    }

    TVec3f side;
    PSVECCrossProduct(&tangent, &rGroundNormal, &side);
    PSVECCrossProduct(&rGroundNormal, &side, &tangent);
    tangent.setLength(speed);

    const f32 blend = 0.5f * MR::clamp((_24 + 50.0f - _20) / 50.0f, 0.0f, 1.0f);
    const f32 angleRatio = MR::diffAngleAbs(_34, tangent) / MR::pi();
    MR::vecBlendSphere(_34, tangent, &_34, blend);
    _34.scale(1.0f - angleRatio);
}

void MarioTeresa::procDrop() {
    if (_20 > _24) {
        f32 ratio = 1.0f;
        const f32 excessHeight = _20 - _24;
        if (excessHeight < 100.0f) {
            ratio = excessHeight / 100.0f;
        }

        addTeresaVerticalVelocity((_58 ? 0.25f : 0.1f) * ratio);

        TVec3f horizontal;
        f32 vertical = MR::vecKillElement(_34, getAirGravityVec(), &horizontal);
        if (1.5f * vertical > excessHeight) {
            vertical *= 0.75f;
        }
        _34 = horizontal + getAirGravityVec() * vertical;
    } else {
        TVec3f horizontal;
        const f32 vertical = MR::vecKillElement(_34, getAirGravityVec(), &horizontal);
        _34 = horizontal;
        if (vertical < 0.0f) {
            _34 += getAirGravityVec() * vertical;
        }
    }

    if (_20 > _24 + 10.0f) {
        getPlayer()->mDrawStates._1C = true;
    }
}

void MarioTeresa::addTeresaVerticalVelocity(f32 acceleration) {
    _34 += getAirGravityVec() * acceleration;

    TVec3f horizontal;
    f32 vertical = MR::vecKillElement(_34, getAirGravityVec(), &horizontal);
    const MarioConstTable* pTable = mActor->mConst->getTable();
    f32 maxDropSpeed = 20.0f * pTable->mTeresaDropSpeedMax;
    const f32 maxRiseSpeed = 10.0f * pTable->mTeresaRiseSpeedMax;

    if (_58) {
        const f32 disappearRatio = static_cast< f32 >(getPlayer()->_418) / pTable->mTeresaWallThroughTime;
        maxDropSpeed *= 1.0f + 0.5f * MR::cos(MR::pi() * disappearRatio);
    }

    if (getPlayer()->mDrawStates._1F && _28.dot(getAirGravityVec()) > 0.707f) {
        maxDropSpeed *= 2.0f;
    }

    vertical = MR::clamp(vertical, -maxRiseSpeed, maxDropSpeed);
    _34 = horizontal + getAirGravityVec() * vertical;
    _50 += acceleration;
}

void MarioTeresa::addTeresaHorizontalVelocity(const TVec3f& rVelocity) {
    TVec3f addVelocityHorizontal;
    MR::vecKillElement(rVelocity, getAirGravityVec(), &addVelocityHorizontal);
    _34 += addVelocityHorizontal;

    TVec3f horizontal;
    const f32 vertical = MR::vecKillElement(_34, getAirGravityVec(), &horizontal);
    f32 maxSpeed = mActor->mConst->getTable()->mTeresaHorizontalSpeedMax;

    if (getPlayer()->mDrawStates._1F && _28.dot(horizontal) > 0.707f) {
        maxSpeed *= 2.0f;
    }

    if (horizontal.length() >= maxSpeed) {
        horizontal.setLength(maxSpeed);
    }

    _34 = horizontal + getAirGravityVec() * vertical;
}

void Mario::doTeresaReflection(const TVec3f& rNormal, bool emitEffect) {
    mTeresa->doTeresaReflection(rNormal, emitEffect);
}

void MarioTeresa::doTeresaReflection(const TVec3f& rNormal, bool emitEffect) {
    TVec3f normal(rNormal);
    MR::normalizeOrZero(&normal);

    TVec3f tangent;
    f32 normalSpeed = MR::vecKillElement(_34, normal, &tangent);
    if (normalSpeed < 0.0f) {
        if (normalSpeed > -10.0f) {
            normalSpeed = -10.0f;
        }

        const TVec3f reflection = normal * -normalSpeed;
        TVec3f horizontal;
        const f32 vertical = MR::vecKillElement(reflection, getAirGravityVec(), &horizontal);
        addTeresaHorizontalVelocity(horizontal * 2.0f);
        addTeresaVerticalVelocity(vertical * 2.0f);
    } else {
        TVec3f horizontal;
        const f32 vertical = MR::vecKillElement(normal, getAirGravityVec(), &horizontal);
        addTeresaHorizontalVelocity(horizontal);
        addTeresaVerticalVelocity(vertical);
    }

    _46 = mActor->mConst->getTable()->mTeresaWallReflectTime;
    _14.zero();

    if (_42 == 0) {
        mActor->changeTeresaAnimation("hit", -1);
        playSound("声壁反射", -1);

        if (emitEffect) {
            playSound("テレサ壁反射", -1);
            MR::emitEffectHit(mActor->_9A4, getPlayer()->_25C, getPlayer()->_268, "WallHit");
        }
    }

    getPlayer()->_418 = 0;
    mActor->_946 = 5;
    _42++;
}

void MarioActor::initTeresaMarioAnimation() {
    _9B0 = 0.0f;

    XanimeResourceTable* pResourceTable =
        new XanimeResourceTable(MR::getResourceHolder(_9A4), reinterpret_cast< XanimeGroupInfo* >(teresaAnimeTable), nullptr, nullptr,
                                nullptr, reinterpret_cast< XanimeBckTable2* >(teresaAnime2), nullptr, nullptr, nullptr);
    _9BC = reinterpret_cast< u32 >(pResourceTable);

    XanimePlayer* pPlayer = new XanimePlayer(MR::getJ3DModel(_9A4), pResourceTable);
    _9B8 = reinterpret_cast< u32 >(pPlayer);
    pPlayer->setDefaultAnimation(cTeresaBase);
    pPlayer->changeAnimation(cTeresaBase);
    _9A4->mModelManager->mXanimePlayer = pPlayer;
}

void Mario::startTeresaDisappear() {
    _418 = mActor->mConst->getTable()->mTeresaWallThroughTime;
    playSound("テレサ消える", -1);
    playSound("声トルネード", -1);
    MR::startCSSound("CS_TERESA", nullptr, 0);
    mTeresa->updateDropFlag();
    resetSleepTimer();

    if (_962 == 5 && mVerticalSpeed < 100.0f) {
        mTeresa->_34 += getAirGravityVec() * 10.0f;
    }
}

bool MarioTeresa::start() {
    if (getPlayer()->_430 == 4) {
        getPlayer()->setFrontVecKeepUp(-getPlayer()->_220);
    }

    getPlayer()->_42A = 0;
    getPlayer()->_430 = 0;
    getPlayer()->cancelSquatMode();
    changeAnimation("落下", "落下");
    return true;
}

void MarioTeresa::checkWind() {
    TVec3f wind;
    f32 strength;
    BigFanFunction::calcWindInfo(&wind, mActor->_2A0, &strength);
    strength = MR::clamp(strength, 0.0f, 10.0f);

    if (strength > 0.0f) {
        playSound("テレサ風に乗る", static_cast< s32 >(50.0f * strength));
    }

    if (!MR::isNearZero(strength, 0.001f)) {
        getPlayer()->mDrawStates._1F = true;
    }

    wind.scale(strength);
    _28.scale(0.94f);
    _28 += wind * 0.15f;
    _34 += wind * 0.2f;

    if (_28.length() > 0.2f) {
        getPlayer()->mDrawStates._1F = true;
    }
}

void MarioTeresa::checkWallCeilReflect() {
    TVec3f collisionNormal;
    bool hasCollision = false;

    if (getPlayer()->calcDistToCeil(false) < 200.0f) {
        collisionNormal = getPlayer()->_3B0;
        hasCollision = true;
    } else {
        hasCollision = getPlayer()->getHitWallNorm(&collisionNormal);
    }

    if (!hasCollision) {
        _42 = 0;
        return;
    }

    TVec3f tangent;
    const f32 normalSpeed = MR::vecKillElement(_34, collisionNormal, &tangent);
    _34 = tangent;
    if (normalSpeed < 0.0f) {
        _34 += collisionNormal * -normalSpeed;
    } else {
        _34 += collisionNormal * (1.5f * normalSpeed);
    }

    if (_42 == 0) {
        playSound("テレサ壁反射", -1);
        playSound("声壁反射", -1);
        mActor->changeTeresaAnimation("hit", -1);
        MR::emitEffectHit(mActor->_9A4, getPlayer()->getWallPos(), getPlayer()->getWallNorm(), "WallHit");
        mActor->_946 = 10;
        _46 = mActor->mConst->getTable()->mTeresaWallReflectTime;
        _14.zero();
        getPlayer()->_418 = 0;
    }

    _42++;
}

void MarioTeresa::procAirControl() {
    const MarioConstTable* pTable = mActor->mConst->getTable();
    if (getPlayer()->_418 >= pTable->mTeresaWallThroughTime - 30) {
        return;
    }

    if ((!isTeresaAccel() || _46 != 0) && _48 == 0) {
        return;
    }

    if (getPlayer()->_1C._4) {
        if (_34.dot(getAirGravityVec()) > 0.0f) {
            addTeresaVerticalVelocity(-2.0f);
        } else {
            addTeresaVerticalVelocity(-0.4f);
        }
    } else {
        addTeresaVerticalVelocity(-0.25f);
    }

    if (isTeresaAccel()) {
        playSound("テレサ踏ん張り", -1);
        if (getPlayer()->_1C._4) {
            _48 = pTable->mTeresaTrgOnPushTime1;
        } else if (_48 < 15) {
            _48 = pTable->mTeresaTrgOnPushTime2;
        }
    }

    if (_48 != 0) {
        _48--;
    }
}

void MarioTeresa::procControl() {
    if (_46 == 0) {
        if (!MR::isNearZero(getStickP(), 0.001f)) {
            getPlayer()->setFrontVecKeepUp(getWorldPadDir(), mActor->mConst->getTable()->mTeresaAirWalkTurnSpd);
        }

        if (getPlayer()->_402 < (mActor->mConst->getTable()->mAirWalkTime >> 1)) {
            getAnimator()->setSpeed(1.5f);
        }

        f32 acceleration = 0.3f;
        if (getPlayer()->_418 > (mActor->mConst->getTable()->mTeresaWallThroughTime >> 1)) {
            acceleration = 2.0f;
        }
        addTeresaHorizontalVelocity(getWorldPadDir() * acceleration);
    }

    if (checkTrgZ()) {
        if (_34.dot(getAirGravityVec()) < 0.0f) {
            MR::vecKillElement(_34, getAirGravityVec(), &_34);
        }
        mActor->changeTeresaAnimation("fallquicklystart", -1);
        playSound("声壁押し", -1);
    }
}

void MarioActor::runTeresaBaseAnimation() {
    XanimePlayer* pPlayer = getTeresaXanimePlayer(this);
    if (!mMario->isStatusActive(MarioStatus_Wait) && !pPlayer->isRun(cTeresaBase)) {
        pPlayer->changeAnimation(cTeresaBase);
        _9B4 = MR::getRandom(60L, 180L);
        MR::startBtp(_9A4, cTeresaBlink);
    }
}

void MarioActor::changeTeresaAnimation(const char* pName, s32 interpolation) {
    if (MR::isBckPlaying(_9A4, cTeresaSleep)) {
        MR::deleteEffect(_9A4, cTeresaSleepEffect);
    }

    if (interpolation == -1) {
        MR::startBck(_9A4, pName, nullptr);
    } else {
        MR::startBckWithInterpole(_9A4, pName, interpolation);
    }

    if (MR::isEqualString(pName, cTeresaWait) || MR::isEqualString(pName, cTeresaRun)) {
        _9B4 = MR::getRandom(60L, 180L);
        MR::stopBtp(_9A4);
    } else {
        _9B4 = 0;
        MR::startBtp(_9A4, MR::isExistBtp(_9A4, pName) ? pName : cTeresaBlink);
    }
}

void MarioActor::updateTeresaAnimation() {
    XanimePlayer* pPlayer = getTeresaXanimePlayer(this);
    MarioTeresa* pTeresa = mMario->mTeresa;

    if (MR::isBckPlaying(_9A4, cTeresaSleep) &&
        (pTeresa->isTeresaAccel() || mMario->getStickP() != 0.0f || mMario->mJumpVec.length() > 1.0f)) {
        runTeresaBaseAnimation();
        MR::deleteEffect(_9A4, cTeresaSleepEffect);
    }

    if (MR::isBckOneTimeAndStopped(_9A4)) {
        runTeresaBaseAnimation();
    }

    bool canSelectMovementAnimation = !MR::isBckPlaying(_9A4, "hit");
    if (MR::isBckPlaying(_9A4, cTeresaSpin) && !pTeresa->isTeresaAccel()) {
        canSelectMovementAnimation = false;
    }

    if (canSelectMovementAnimation) {
        if (pTeresa->isTeresaAccel()) {
            if (!MR::isBckPlaying(_9A4, cTeresaFly) && !MR::isBckPlaying(_9A4, cTeresaSpin)) {
                changeTeresaAnimation(cTeresaFly, 16);
            }
        } else {
            const f32 gravitySpeed = getLastMove().dot(getGravityVec());
            if (MR::isBckPlaying(_9A4, cTeresaFly) || pPlayer->isRun(cTeresaBase)) {
                if (gravitySpeed >= 1.0f) {
                    changeTeresaAnimation(cTeresaFall, 16);
                } else if (!mMario->mDrawStates._1C && MR::abs(gravitySpeed) < 1.0f) {
                    runTeresaBaseAnimation();
                }
            } else if (MR::isBckPlaying(_9A4, cTeresaFall) && gravitySpeed < 1.0f) {
                runTeresaBaseAnimation();
            }
        }

        if (pPlayer->isRun(cTeresaBase)) {
            _9B0 = MR::clamp(mMario->mJumpVec.length() / 10.0f, 0.0f, 1.0f);
            pPlayer->changeTrackWeight(0, 1.0f - _9B0);
            pPlayer->changeTrackWeight(1, _9B0);
        }
    }

    const MarioConstTable* pTable = mConst->getTable();
    if (mMario->_418 != 0) {
        if (!MR::isBckPlaying(_9A4, cTeresaSpin) && mMario->_418 > pTable->mTeresaWallThroughTime - 3) {
            changeTeresaAnimation(cTeresaSpin, -1);
        }

        if (_9A8 < pTable->mTeresaAlphaLevelMax) {
            _9A8 += pTable->mTeresaAlphaLevelInc;
        }

        mMario->_418--;
        if (mMario->_418 == 0) {
            playSound("テレサ現れる", -1);
        }
    } else {
        if (_9A8 > 0.0f) {
            _9A8 -= pTable->mTeresaAlphaLevelDec;
        }
        if (_9A8 > 0.0f && pTeresa->_46 != 0) {
            _9A8 -= pTable->mTeresaAlphaLevelDec;
        }
    }

    MR::setBrkFrameAndStop(_9A4, _9A8);

    if (_9B4 != 0 && --_9B4 == 0) {
        _9B4 = MR::getRandom(90L, 240L);
        MR::startBtp(_9A4, cTeresaBlink);
    }
}

bool MarioTeresa::keep() {
    return update();
}

bool MarioTeresa::notice() {
    return true;
}
