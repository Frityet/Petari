#include "Game/LiveActor/Nerve.hpp"
#include "Game/Enemy/KarikariDirector.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioAbyssDamage.hpp"
#include "Game/Player/MarioAccess.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioBlown.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Player/MarioDamage.hpp"
#include "Game/Player/MarioDarkDamage.hpp"
#include "Game/Player/MarioFaint.hpp"
#include "Game/Player/MarioFireDamage.hpp"
#include "Game/Player/MarioFireDance.hpp"
#include "Game/Player/MarioFireRun.hpp"
#include "Game/Player/MarioFreeze.hpp"
#include "Game/Player/MarioMapCode.hpp"
#include "Game/Player/MarioParalyze.hpp"
#include "Game/Player/MarioSwim.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/SequenceUtil.hpp"

bool Mario::isDamaging() const {
    if (isAnimationRun("水上ダメージ中")) {
        return true;
    }

    if (_41E != 0) {
        return true;
    }
    if (mMovementStates._1B) {
        return true;
    }
    if (mMovementStates._27) {
        return true;
    }
    if (mMovementStates._2C) {
        return true;
    }
    if (_10.jumping) {
        return true;
    }
    if (_10._14) {
        return true;
    }
    if (_10._18) {
        return true;
    }

    switch (getCurrentStatus()) {
    case MarioStatus_FireDamage:
    case MarioStatus_FireDance:
    case MarioStatus_FireRun:
    case MarioStatus_Paralyze:
    case MarioStatus_AbyssDamage:
    case MarioStatus_Freeze:
    case MarioStatus_Crush:
        return true;
    case MarioStatus_Damage:
        return mDamage->_12;
    case MarioStatus_Faint:
        return mFaint->_25;
    default:
        return false;
    }
}

bool Mario::damageLarge(const TVec3f& rDamage) {
    if (damage(rDamage)) {
        if (isStatusActive(MarioStatus_Swim)) {
            mSwim->_AD = 1;
        } else {
            const MarioConstTable* table = mActor->mConst->getTable();
            mDamage->setVecSize(table->mJumpDistLargeDamage, table->mJumpHeightLargeDamage);
            playSound("投げられ", -1);
        }
        return true;
    }
    return false;
}

void Mario::decDamageAfterTimer() {
    if (mDamage->_16 != 0) {
        --mDamage->_16;
    }
    if (mFaint->_14 != 0) {
        --mFaint->_14;
    }
    if (mParalyze->_16 != 0) {
        --mParalyze->_16;
    }
    if (mFreeze->_1C != 0) {
        --mFreeze->_1C;
    }

    mFireDamage->decAfterTimer();

    if (_41E != 0) {
        --_41E;
    }
}

bool Mario::checkDamage() {
    decDamageAfterTimer();

    if (mMovementStates._1F || mActor->_EA4 || MR::isDemoActive() || isStatusActive(MarioStatus_Talk)) {
        return false;
    }

    if (isStatusActive(MarioStatus_Recovery) || isInvincible()) {
        mMovementStates._1B = false;
        mMovementStates._27 = false;
        mMovementStates._2C = false;
        _10.jumping = false;
        _10._14 = false;
        _10._18 = false;
        mFaint->_24 = false;
        return false;
    }

    if (isStatusActive(MarioStatus_Swim)) {
        checkWaterDamage();
        return false;
    }

    if (mMovementStates._1B) {
        mMovementStates._1B = false;
        changeStatus(mDamage);
        mMovementStates._27 = false;
        mFaint->_24 = false;
        mMovementStates._2C = false;
        return true;
    }

    if (mMovementStates._27) {
        mMovementStates._27 = false;
        mMovementStates._2C = false;
        changeStatus(mFaint);
        return true;
    }

    if (mMovementStates._2C) {
        mMovementStates._2C = false;
        changeStatus(mBlown);
        return true;
    }

    if (_10.jumping) {
        _10.jumping = false;
        doFireDanceWithInitialDamage(1);
        return true;
    }

    if (_10._14) {
        _10._14 = false;
        doParalyze();
        return true;
    }

    if (_10._18) {
        _10._18 = false;
        if (tryCrush()) {
            return true;
        }
    }

    checkKarikariDamage();
    return false;
}

u16 Mario::getDamageAfterTimer() const {
    u16 timer = mDamage->_16;
    if (timer < mFaint->_14) {
        timer = mFaint->_14;
    }
    if (timer < mParalyze->_16) {
        timer = mParalyze->_16;
    }
    if (timer < mFreeze->_1C) {
        timer = mFreeze->_1C;
    }
    if (timer < mFireDamage->_12) {
        timer = mFireDamage->_12;
    }
    if (timer < _41E) {
        timer = _41E;
    }
    return timer;
}

bool Mario::damageFloorCheck() {
    if (mMovementStates._1F) {
        return false;
    }
    if (_1C._16) {
        return false;
    }

    switch (_960) {
    case 24:
        if (checkCurrentFloorCodeSevere(24) && doNeedleWithInitialDamage(mGroundPolygon)) {
            return true;
        }
        break;
    case 0x81:
        if (checkCurrentFloorCodeSevere(0x81) && doFireDanceWithInitialDamage(1)) {
            return true;
        }
        break;
    case 1:
        MarioAccess::forceKill(3, 0);
        return true;
    case 10:
        if (checkCurrentFloorCodeSevere(10) && doFireDanceWithInitialDamage(1)) {
            return true;
        }
        break;
    case 4: {
        if (isDamaging()) {
            return false;
        }
        TVec3f damage = _368;
        damage.scale(10.0f);
        if (this->damage(damage)) {
            return true;
        }
        break;
    }
    case 15:
        if (doParalyze()) {
            return true;
        }
        break;
    case 16:
        if (doRecovery()) {
            return true;
        }
        break;
    default:
        break;
    }

    return false;
}

bool Mario::damageWallCheck() {
    if (mMovementStates._1F) {
        return false;
    }

    TVec3f wallNormal;
    if (checkWallCodeNorm(6, &wallNormal, false)) {
        wallNormal.scale(5.0f);
        return doFlipJump(wallNormal);
    }

    if (checkWallFloorCode(1)) {
        mActor->forceKill(3);
        return true;
    }
    if (checkWallFloorCode(10) && doFireDanceWithInitialDamage(1)) {
        return true;
    }
    if (checkWallFloorCode(24) && doNeedleWithInitialDamage(1)) {
        return true;
    }
    if (checkWallFloorCode(15) && doParalyze()) {
        return true;
    }
    if (checkWallFloorCode(16) && doRecovery()) {
        return true;
    }
    if (checkWallFloorCode(4)) {
        TVec3f damage = getWallNorm();
        damage.scale(10.0f);
        if (this->damage(damage)) {
            return true;
        }
    }

    return false;
}

bool Mario::damagePolygonCheck(const Triangle* pTriangle) {
    if (mMovementStates._1F) {
        return false;
    }

    switch (_95C->getCode(pTriangle)) {
    case 0x81:
        if (doFireDanceWithInitialDamage(1)) {
            return true;
        }
        break;
    case 1:
        MarioAccess::forceKill(3, 0);
        return true;
    case 10:
        if (doFireDanceWithInitialDamage(1)) {
            return true;
        }
        break;
    case 4: {
        TVec3f damage = *MR::getNormal(pTriangle);
        damage.scale(10.0f);
        mSwim->addDamage(damage);
        return true;
    }
    case 15:
        if (doParalyze()) {
            return true;
        }
        break;
    case 16:
        if (doRecovery()) {
            return true;
        }
        break;
    case 24:
        doNeedleWithInitialDamage(pTriangle);
        return true;
    case 25:
        return true;
    default:
        break;
    }

    return false;
}

bool Mario::flipLarge(const TVec3f& rDamage) {
    if (damage(rDamage)) {
        if (isStatusActive(MarioStatus_Swim)) {
            mSwim->_AD = 1;
            mSwim->mDamageType = 1;
        } else {
            mDamage->_11 = 1;
        }

        mDamage->setVecSize(rDamage.length(), 0.0f);
        return true;
    }
    return false;
}

bool Mario::isEnableAddDamage() const {
    if (getCurrentStatus() == MarioStatus_Talk) {
        return false;
    }
    if (isDamaging()) {
        return false;
    }
    if (mActor->_390 != 0) {
        return false;
    }
    if (isInvincible()) {
        return false;
    }
    return getDamageAfterTimer() == 0;
}

bool Mario::damage(const TVec3f& rDamage) {
    _7C4 = rDamage;

    if (!isEnableAddDamage() || mFaint->_14 != 0 || mDamage->_16 != 0 || mMovementStates._1B) {
        return false;
    }

    if (mMovementStates._F) {
        forceStopTornado();
    }

    mDamage->setVec(rDamage);
    stopWalk();
    forceStopTornado();
    mActor->damageDropThrowMemoSensor();
    if (isStatusActive(MarioStatus_Damage)) {
        closeStatus(mDamage);
    }
    mMovementStates._1B = true;
    return true;
}

MarioDamage::MarioDamage(MarioActor* pActor)
    : MarioState(pActor, MarioStatus_Damage), _11(0), _12(0), _14(0), _16(0), _18(0) {
    _1C.zero();
    _28 = nullptr;
    _2C = nullptr;
}

bool MarioDamage::start() {
    _14 = 0;
    _18 = 0;

    if (_1C.dot(getPlayer()->mFrontVec) > 0.0f) {
        changeAnimationNonStop("中後ダメージ");
        _28 = "中後ダメージ空中";
        _2C = "中後ダメージ着地";
        getPlayer()->setFrontVecKeepUp(_1C);
    } else {
        changeAnimationNonStop("中ダメージ");
        _28 = "中ダメージ空中";
        _2C = "中ダメージ着地";
        getPlayer()->setFrontVecKeepUp(-_1C);
    }

    if (_11 == 0) {
        playEffect("ダメージ");
    }
    startPadVib(3);

    getPlayer()->mMovementStates._1 = false;
    getPlayer()->mMovementStates.jumping = true;
    getPlayer()->mMovementStates._B = false;
    getPlayer()->mMovementStates._3E = 0;

    _1C += -mActor->_240 * mActor->mConst->getTable()->mJumpHeightDamage;
    getPlayer()->mJumpVec = _1C;
    addVelocity(_1C);

    _12 = !_11;
    if (_11 != 0) {
        playSound("声投げられ", -1);
        playSound("投げられ", -1);
        _11 = 0;
        mActor->resetPlayerModeOnNoDamage();
    } else {
        playSound("声小ダメージ", -1);
        playSound("ダメージ", -1);
        mActor->decLifeMiddle();
        mActor->resetPlayerModeOnDamage();
    }

    return true;
}

void MarioDamage::setVec(const TVec3f& rVec) {
    MR::vecKillElement(rVec, mActor->_240, &_1C);
    _1C.setLength(mActor->mConst->getTable()->mJumpDistDamage);
}

void MarioDamage::setVecSize(f32 horizontal, f32 vertical) {
    _1C.setLength(horizontal);
    _1C += -mActor->_240 * vertical;
}

void MarioDamage::stopHead(const TVec3f& rNormal) {
    if (_18 == 0) {
        TVec3f horizontal;
        const f32 vertical = MR::vecKillElement(_1C, mActor->_240, &horizontal);
        const f32 normal = MR::vecKillElement(horizontal, rNormal, &_1C);
        _1C += mActor->_240 * vertical;
        if (normal < 0.0f) {
            _1C += rNormal * -normal * 0.5f;
        }
    } else {
        TVec3f horizontal;
        MR::vecKillElement(rNormal, getAirGravityVec(), &horizontal);
        if (!MR::normalizeOrZero(&horizontal)) {
            const f32 normal = MR::vecKillElement(_1C, rNormal, &_1C);
            if (normal < 0.0f) {
                _1C += rNormal * -normal * 0.5f;
            }
        }
    }
}

bool MarioDamage::update() {
    ++_14;
    if (mActor->_EA4) {
        return true;
    }

    switch (_18) {
    case 0: {
        addVelocity(_1C);
        _1C += mActor->_240 * mActor->mConst->getTable()->mGravityDamage;

        if (_14 > 20) {
            if (_28 != nullptr) {
                changeAnimation(_28, static_cast< const char* >(nullptr));
            }
            if (getPlayer()->_1C._0) {
                const f32 gravity = MR::vecKillElement(_1C, getAirGravityVec(), &_1C);
                _1C.x *= 0.95f;
                _1C.y *= 0.95f;
                _1C.z *= 0.95f;
                _1C += getAirGravityVec() * gravity;
            }
        }

        if (getPlayer()->mMovementStates._1) {
            if (getPlayer()->mVerticalSpeed > 30.0f
                && mActor->selectDamagePop(getSensor(getGroundPolygon()))) {
                _1C += getPlayer()->_368 * 20.0f;
                getPlayer()->mMovementStates._1 = false;
                getPlayer()->mMovementStates.jumping = true;
            } else {
                getPlayer()->mMovementStates.jumping = false;
                playSound("吹っ飛び倒れ", -1);
                changeAnimation(_2C, static_cast< const char* >(nullptr));
                playEffect("共通ダメージ着地");
                MR::vecKillElement(_1C, mActor->_240, &_1C);
                _14 = 0;
                ++_18;
                if (mActor->mHealth == 0) {
                    _18 = 2;
                }
            }
        } else if (mActor->mHealth == 0) {
            if (_14 > 240) {
                mActor->forceGameOverAbyss();
            }
        } else if (_14 > 360) {
            mActor->forceGameOverAbyss();
        }
        break;
    }
    case 1:
        if (!getPlayer()->mMovementStates._1) {
            getPlayer()->mMovementStates.jumping = true;
            _18 = 0;
            break;
        }
        MR::vecKillElement(_1C, getAirGravityVec(), &_1C);
        addVelocity(_1C);
        _1C.x *= 0.95f;
        _1C.y *= 0.95f;
        _1C.z *= 0.95f;
        if (!isAnimationRun(_2C)) {
            return false;
        }
        if (_14 > 15 && checkTrgA()) {
            getPlayer()->tryJump();
            return false;
        }
        break;
    case 2:
        if (!getPlayer()->mMovementStates._1) {
            getPlayer()->mMovementStates.jumping = true;
            _18 = 0;
        } else if (_14 == 40) {
            if (mActor->mHealth == 0) {
                mActor->forceGameOver();
            } else {
                return false;
            }
        }
        break;
    }

    getPlayer()->mJumpVec = _1C;
    return true;
}

bool MarioDamage::close() {
    stopAnimation("ダメージ", static_cast< const char* >(nullptr));
    stopAnimation("ダメージ着地", "基本");
    if (_12 != 0) {
        _16 = 120;
    }
    return true;
}

bool MarioDamage::notice() {
    if (mActor->mHealth == 0) {
        if (getNoticedStatus() == MarioStatus_Swim) {
            mActor->forceGameOver();
        }
        return true;
    }
    return false;
}

MarioFireDamage::MarioFireDamage(MarioActor* pActor) : MarioState(pActor, MarioStatus_FireDamage), _12(0) {
}

void MarioFireDamage::decAfterTimer() {
    if (_12 != 0 && !isStatusActiveID(MarioStatus_FireDamage) && !isStatusActiveID(MarioStatus_FireRun)) {
        --_12;
    }
}

bool Mario::doAbyssDamage() {
    if (getCurrentStatus() == MarioStatus_AbyssDamage) {
        return false;
    }
    stopWalk();
    mActor->damageDropThrowMemoSensor();
    MR::removeAllClingingKarikari();
    mActor->_A6E = false;
    changeStatus(mAbyssDamage);
    return true;
}

MarioAbyssDamage::MarioAbyssDamage(MarioActor* pActor)
    : MarioState(pActor, MarioStatus_AbyssDamage), _12(0), _14(0) {
    _18.zero();
}

bool MarioAbyssDamage::start() {
    _12 = 0;
    _14 = 0;
    mActor->forceGameOverAbyss();
    return false;
}

bool MarioAbyssDamage::update() {
    addTrans(_18, "Module");
    switch (_14) {
    case 0:
        ++_14;
        _12 = 120;
        MR::requestStartGameOverDemo();
        break;
    case 1:
        if (_12 != 0) {
            --_12;
        }
        if (_12 == 0) {
            mActor->forceGameOverAbyss();
            return false;
        }
        break;
    }
    return true;
}

bool MarioAbyssDamage::close() {
    return true;
}

bool MarioAbyssDamage::notice() {
    return true;
}

void Mario::connectToFireRun() {
    if (mActor->mHealth != 0) {
        changeStatus(mFireRun);
        stopJump();
        mFireRun->_12 = 1;
    }
}

MarioFireRun::MarioFireRun(MarioActor* pActor)
    : MarioState(pActor, MarioStatus_FireRun), _12(0), _14(0), _18(0.0f) {
}

bool MarioFireRun::start() {
    _12 = mActor->mConst->getTable()->mFireRunTimer1;
    _14 = 0;
    stopAnimationUpper(static_cast< const char* >(nullptr), static_cast< const char* >(nullptr));
    changeAnimation("ファイアラン前兆", static_cast< const char* >(nullptr));

    if (!getPlayer()->mMovementStates._1) {
        _18 = -mActor->mConst->getTable()->mFireRunFirstJump;
    } else {
        _18 = 0.0f;
    }
    return true;
}

bool MarioFireRun::move() {
    if (getStickX() != 0.0f || getStickY() != 0.0f) {
        TVec3f& padDir = getWorldPadDir();
        MarioConst* constants = mActor->mConst;
        getPlayer()->setFrontVecKeepUp(padDir, constants->getTable()->mFireRunTurnRatio);
    }

    if (getPlayer()->checkTrgA() || mActor->isRequestJump()) {
        Mario* player = getPlayer();
        player->mWalkSpeed = 1.0f;
        getPlayer()->tryJump();
        return false;
    }
    return true;
}

bool MarioFireRun::update() {
    switch (_14) {
    case 0:
        if (!getPlayer()->mMovementStates._1) {
            const f32 jumpSpeed = _18;
            TVec3f jumpGravity = getAirGravityVec();
            jumpGravity.scale(jumpSpeed);
            getPlayer()->mJumpVec = jumpGravity;

            const f32 velocitySpeed = _18;
            TVec3f velocityGravity = getAirGravityVec();
            velocityGravity.scale(velocitySpeed);
            addVelocity(velocityGravity);

            _18 += mActor->mConst->getTable()->mFireRunGravity;
            if (_18 > 50.0f) {
                _18 = 50.0f;
            }

            TVec3f& actorVelocity = mActor->_288;
            if (actorVelocity.dot(getAirGravityVec()) < -10.0f) {
                _18 = 0.0f;
                TVec3f front = getFrontVec();
                front.scale(5.0f);
                addVelocity(front);
            }
        } else if (_12 != 0) {
            --_12;
        } else {
            ++_14;
            _12 = mActor->mConst->getTable()->mFireRunTimer2;
            if (mActor->mHealth == 0) {
                _12 >>= 1;
            }
            _18 = 0.0f;
            changeAnimation("炎のランナー", static_cast< const char* >(nullptr));
        }
        break;

    case 1: {
        playSound("炎ダメージ炎上中", -1);
        if (!getPlayer()->mMovementStates._1) {
            _14 = 2;
            _12 += mActor->mConst->getTable()->mFireRunTimer3;
        }

        const f32 runSpeed = mActor->mConst->getTable()->mFireRunSpeed;
        TVec3f front = getFrontVec();
        front.scale(runSpeed);
        addVelocity(front);

        if (_12 != 0) {
            --_12;
        }
        if (_12 == 0) {
            _12 = mActor->mConst->getTable()->mFireRunTimer3;
            ++_14;
        }
        return move();
    }

    case 2:
        if (mActor->isEnableNerveChange() && getStickP() > 0.7f) {
            return false;
        }

        if (!getPlayer()->mMovementStates._1) {
            const f32 jumpSpeed = _18;
            TVec3f jumpGravity = getAirGravityVec();
            jumpGravity.scale(jumpSpeed);
            getPlayer()->mJumpVec = jumpGravity;

            const f32 velocitySpeed = _18;
            TVec3f velocityGravity = getAirGravityVec();
            velocityGravity.scale(velocitySpeed);
            addVelocity(velocityGravity);

            _18 += mActor->mConst->getTable()->mFireRunGravity;
            if (_18 > 50.0f) {
                _18 = 50.0f;
            }

            TVec3f& actorVelocity = mActor->_288;
            if (actorVelocity.dot(getAirGravityVec()) < -10.0f) {
                _18 = 0.0f;
                TVec3f front = getFrontVec();
                front.scale(5.0f);
                addVelocity(front);
            }

            if (_12 != 0) {
                --_12;
            }
            break;
        } else {
            const MarioConstTable* table = mActor->mConst->getTable();
            if (_12 > table->mFireRunTimer3) {
                const f32 runSpeed = table->mFireRunSpeed;
                addVelocity(getFrontVec() * runSpeed);
            } else {
                const f32 timer = static_cast< f32 >(table->mFireRunTimer3);
                const f32 remaining = static_cast< f32 >(_12);
                const f32 runSpeed = table->mFireRunSpeed;
                addVelocity(getFrontVec() * runSpeed * remaining * (1.0f / timer));
            }

            if (mActor->isEnableNerveChange()) {
                if (_12 != 0) {
                    --_12;
                }
                if (_12 == 0) {
                    return false;
                }
            }
            return move();
        }
    }

    return true;
}

bool MarioFireRun::close() {
    if (mActor->mHealth == 0) {
        mActor->forceGameOver();
    }

    if (getPlayer()->mMovementStates.jumping) {
        stopAnimation("炎のランナー", "落下");
    } else {
        stopAnimation("炎のランナー", "基本");
        if (getStickP() < 0.1f) {
            playSound("声炎ダメージ終了", -1);
        }
    }

    stopEffect("炎ダメージ煙");
    stopEffect("炎ダメージ青煙");
    mActor->_1B4 = 0;
    return true;
}

bool MarioFireRun::notice() {
    return false;
}

bool Mario::doFireDanceWithInitialDamage(u8 damageCount) {
    if (mMovementStates._1F) {
        return false;
    }

    const bool result = doFireDance();
    if (result) {
        for (u32 i = 0; i < damageCount; ++i) {
            mActor->decLife(0);
        }
        if (mActor->mHealth == 0) {
            mActor->forceGameOverNonStop();
        }
    }
    return result;
}

bool Mario::doFireObjHitWithInitialDamage() {
    bool result;
    if (!isEnableAddDamage()) {
        result = false;
    } else {
        result = doFireDanceWithInitialDamage(1);
    }
    return result;
}

bool Mario::doNeedleWithInitialDamage(u8 damageCount) {
    if (mMovementStates._1F) {
        return false;
    }

    if (getPlayerMode() == 6) {
        doTeresaReflection(getWallNorm(), false);
        return false;
    }

    const bool result = doNeedle(nullptr);
    if (result) {
        for (u32 i = 0; i < damageCount; ++i) {
            mActor->decLife(0);
        }
        if (mActor->mHealth == 0) {
            mActor->forceGameOverNonStop();
        }
    }
    return result;
}

bool Mario::doNeedleWithInitialDamage(const Triangle* pTriangle) {
    if (mMovementStates._1F) {
        return false;
    }

    if (getPlayerMode() == 6) {
        doTeresaReflection(*MR::getNormal(pTriangle), false);
        return false;
    }

    const bool result = doNeedle(pTriangle);
    if (result) {
        mActor->decLife(0);
        if (mActor->mHealth == 0) {
            mActor->forceGameOverNonStop();
        }
    }
    return result;
}

bool Mario::doNeedle(const Triangle* pTriangle) {
    if (getCurrentStatus() == MarioStatus_FireDamage) {
        return false;
    }
    if (getCurrentStatus() == MarioStatus_FireRun) {
        return false;
    }
    if (getCurrentStatus() == MarioStatus_FireDance) {
        return false;
    }
    if (mMovementStates._1B) {
        return false;
    }

    if (getPlayerMode() == 6) {
        if (pTriangle != nullptr) {
            doTeresaReflection(*MR::getNormal(pTriangle), false);
        }
        return false;
    }

    if (isInvincible()) {
        return false;
    }

    mActor->resetPlayerModeOnDamage();
    getPlayer()->mMovementStates._B = false;
    getPlayer()->mMovementStates._A = false;
    mActor->damageDropThrowMemoSensor();
    mFireDance->_29 = 1;
    changeStatus(mFireDance);
    return true;
}

bool Mario::doFireDance() {
    if (getCurrentStatus() == MarioStatus_Paralyze) {
        return false;
    }
    if (getCurrentStatus() == MarioStatus_FireDamage) {
        return false;
    }
    if (getCurrentStatus() == MarioStatus_FireRun) {
        return false;
    }
    if (getCurrentStatus() == MarioStatus_FireDance) {
        return false;
    }
    if (mMovementStates._1B) {
        return false;
    }
    if (isInvincible()) {
        return false;
    }
    if (getPlayerMode() == 3) {
        return false;
    }

    mActor->resetPlayerModeOnDamage();
    getPlayer()->mMovementStates._B = false;
    getPlayer()->mMovementStates._A = false;
    mActor->damageDropThrowMemoSensor();
    mFireDance->_29 = 0;
    changeStatus(mFireDance);
    mFireDamage->_12 = 120;
    return true;
}

MarioFireDance::MarioFireDance(MarioActor* pActor)
    : MarioState(pActor, MarioStatus_FireDance) {
    _14.zero();
    _20 = 0.0f;
    _24 = 0;
    _26 = 0;
    _28 = 0;
    _29 = 0;
}

bool MarioFireDance::start() {
    stopAnimationUpper(static_cast< const char* >(nullptr), static_cast< const char* >(nullptr));
    _20 = -mActor->mConst->getTable()->mFireDanceFirstJump;
    MR::vecKillElement(getPlayer()->mJumpVec, getAirGravityVec(), &_14);
    _24 = 0;
    _26 = 0;

    if (getPlayer()->mDrawStates._10) {
        _20 *= 0.25f;
        _14 = getPlayer()->getWallNorm() * mActor->mConst->getTable()->mFireDanceFirstJump * 0.5f;
        getPlayer()->setFrontVecKeepUp(getPlayer()->getWallNorm());
        _24 = 1;
        _26 = 60;
    }

    _28 = 0;
    impact();
    impactEffect();
    startPadVib(3);

    TVec3f gravity = getAirGravityVec();
    gravity.scale(_20);
    getPlayer()->mJumpVec = gravity;
    return true;
}

void MarioFireDance::impact() {
    changeAnimation("ファイアダンス", static_cast< const char* >(nullptr));
    if (!getPlayer()->mDrawStates._10) {
        if (_14.length() > 2.0f * mActor->mConst->getTable()->mFireDanceMoveSpeed) {
            _14.setLength(2.0f * mActor->mConst->getTable()->mFireDanceMoveSpeed);
        }
        _14.setLength(0.5f * _14.length());
    }

    getPlayer()->mMovementStates._1 = false;
    getPlayer()->mMovementStates.jumping = true;
}

void MarioFireDance::impactEffect() {
    playSound("ダメージ", -1);
    playEffect("ダメージ");

    switch (_29) {
    case 0:
        playSound("声炎ダメージ", -1);
        playSound("炎ダメージ", -1);
        if (mActor->_1B4 != 0) {
            playEffect("炎ダメージ青煙");
        } else {
            playEffect("炎ダメージ煙");
        }
        break;
    case 1:
        playSound("声針ダメージ", -1);
        playSound("針ダメージ", -1);
        break;
    }
}

bool MarioFireDance::update() {
    if (getPlayer()->mMovementStates._1 && !getPlayer()->isRising()) {
        if (getPlayer()->_960 != 0x81 && getPlayer()->_960 != 10 && getPlayer()->_960 != 24) {
            if (_28 == 1) {
                if (MR::getPlayerLeft() == 0 && mActor->mHealth == 0) {
                    mActor->changeGameOverAnimation();
                    return true;
                }
                getPlayer()->connectToFireRun();
                return false;
            }

            ++_28;
            _20 = -mActor->mConst->getTable()->mFireDanceSecondJump;
            impact();
            startPadVib(static_cast< u32 >(0));
            playSound("炎ダメージ復帰バウンド", -1);
            if (_29 == 1) {
                playSound("声針ダメージ中", -1);
            } else {
                playSound("声炎ダメージ中", -1);
            }
            changeAnimation("ファイアダンス", static_cast< const char* >(nullptr));
        } else {
            if (_29 == 0) {
                mActor->decLifeLarge();
            } else {
                mActor->decLifeMiddle();
            }
            if (mActor->mHealth == 0) {
                mActor->forceGameOverNonStop();
            }
            _20 = -mActor->mConst->getTable()->mFireDanceFirstJump;
            impact();
            impactEffect();
            startPadVib(3);
        }
    }

    const f32 jumpSpeed = _20;
    TVec3f jumpGravity = getAirGravityVec();
    jumpGravity.scale(jumpSpeed);
    getPlayer()->mJumpVec = jumpGravity;

    const f32 velocitySpeed = _20;
    TVec3f velocityGravity = getAirGravityVec();
    velocityGravity.scale(velocitySpeed);
    addVelocity(velocityGravity);

    if (_20 < 0.0f) {
        _20 += mActor->mConst->getTable()->mFireDanceGravityRise;
    } else {
        _20 += mActor->mConst->getTable()->mFireDanceGravityDrop;
    }
    if (_20 > 50.0f) {
        _20 = 50.0f;
    }

    if (getStickP() != 0.0f) {
        if (_26 != 0) {
            --_26;
        } else {
            TVec3f& padDir = getWorldPadDir();
            MarioConst* constants = mActor->mConst;
            getPlayer()->setFrontVecKeepUp(padDir, constants->getTable()->mFireDanceTurnRatio);
            const f32 moveAcc = mActor->mConst->getTable()->mFireDanceMoveAcc;
            TVec3f front = getFrontVec();
            front.scale(moveAcc);
            _14 += front;
        }
    }

    MR::vecKillElement(_14, getAirGravityVec(), &_14);
    if (!_24 && _14.length() > mActor->mConst->getTable()->mFireDanceMoveSpeed) {
        _14.setLength(mActor->mConst->getTable()->mFireDanceMoveSpeed);
    }
    addVelocity(_14);
    return true;
}

bool MarioFireDance::close() {
    stopAnimation("ファイアダンス", static_cast< const char* >(nullptr));
    stopEffect("炎ダメージ煙");
    stopEffect("炎ダメージ青煙");
    return true;
}

void Mario::checkKarikariDamage() {
    if (!_1C._5 || isDamaging()) {
        _7D0 = 120;
        return;
    }

    if (mActor->_934 || mActor->_EA4 || isStatusActive(MarioStatus_Talk) || !_1C._5) {
        return;
    }

    if (_7D0 != 0 && --_7D0 == 0) {
        if (mActor->mHealth == 1) {
            faint(mHeadVec);
            _7D0 = 120;
        } else {
            startPadVib(2);
            playSound("声小ダメージ", -1);
            playSound("ダメージ", -1);
            mActor->decLifeSmall();
            mActor->_BC4 = 16;
            _7D0 = 120;
            if (mActor->mHealth == 0) {
                mActor->forceGameOver();
            }
        }
    }
}

bool Mario::doDarkDamage() {
    if (getCurrentStatus() == MarioStatus_DarkDamage) {
        return false;
    }

    mActor->_3C0 = true;
    stopWalk();
    mActor->damageDropThrowMemoSensor();
    playSound("声沼沈み", -1);
    playEffect("ダークマター死亡");
    setSeVersion(1);
    changeStatus(mDarkDamage);
    return true;
}

MarioDarkDamage::MarioDarkDamage(MarioActor* pActor)
    : MarioState(pActor, MarioStatus_DarkDamage), _12(0), _14(0) {
}

bool MarioDarkDamage::start() {
    _12 = 0;
    _14 = 0;
    return true;
}

bool MarioDarkDamage::update() {
    switch (_14) {
    case 0:
        ++_14;
        _12 = 150;
        MR::requestStartGameOverDemo();
        break;
    case 1:
        if (_12 != 0) {
            --_12;
        }
        if (_12 == 0) {
            mActor->forceKill(3);
            MarioActor* actor = mActor;
            actor->_481 = 1;
            actor->updateHand();
            actor->updateFace();
        }
        break;
    }

    if (_12 != 0) {
        playSound("ダークマター沈み", -1);
    }
    return true;
}

bool MarioState::close() {
    return true;
}

bool MarioState::update() {
    return true;
}

bool MarioState::start() {
    return true;
}

bool MarioDarkDamage::notice() {
    return true;
}

namespace NrvMarioActor {
    INIT_NERVE(MarioActorNrvWait);
    INIT_NERVE(MarioActorNrvGameOver);
    INIT_NERVE(MarioActorNrvGameOverAbyss);
    INIT_NERVE(MarioActorNrvGameOverAbyss2);
    INIT_NERVE(MarioActorNrvGameOverFire);
    INIT_NERVE(MarioActorNrvGameOverBlackHole);
    INIT_NERVE(MarioActorNrvGameOverNonStop);
    INIT_NERVE(MarioActorNrvGameOverSink);
    INIT_NERVE(MarioActorNrvTimeWait);
    INIT_NERVE(MarioActorNrvNoRush);
};  // namespace NrvMarioActor
