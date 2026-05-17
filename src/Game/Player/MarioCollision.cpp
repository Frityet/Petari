#include "Game/Player/Mario.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioMapCode.hpp"
#include "Game/Player/MarioState.hpp"
#include "Game/Player/MarioSwim.hpp"
#include "Game/Util/AreaObjUtil.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"

namespace {
    const f32 cBodyRadius = 50.0f;
    const f32 cHeadOffset = 100.0f;
    const f32 cFootOffset = 50.0f;
    const f32 cCeilProbe = 300.0f;
    const f32 cWallAngleD = 55.0f;
    const f32 cFloorAngleD = 60.0f;
    const f32 cDangerAngleD = 85.0f;

    inline bool isValidTriangle(const Triangle* pTriangle) {
        return pTriangle != nullptr && pTriangle->isValid();
    }

    inline const TVec3f& triangleNormal(const Triangle* pTriangle) {
        return *pTriangle->getNormal(0);
    }

    inline bool isPressSensor(const Triangle* pTriangle) {
        return pTriangle != nullptr && pTriangle->mSensor != nullptr && MR::isSensorPressObj(pTriangle->mSensor);
    }

    void appendUniqueTriangle(Triangle** ppStorage, u32* pCount, u32 maxCount, const Triangle& rTriangle) {
        if (*pCount >= maxCount) {
            return;
        }

        for (u32 i = 0; i < *pCount; i++) {
            if (ppStorage[i]->mParts == rTriangle.mParts && ppStorage[i]->mIdx == rTriangle.mIdx && ppStorage[i]->mSensor == rTriangle.mSensor) {
                return;
            }
        }

        *ppStorage[*pCount] = rTriangle;
        (*pCount)++;
    }

    bool isMovingMtx(const Triangle* pTriangle) {
        if (!isValidTriangle(pTriangle)) {
            return false;
        }

        return !MR::isSameMtx(pTriangle->getBaseMtx()->toMtxPtr(), pTriangle->getPrevBaseMtx()->toMtxPtr());
    }
};

bool Mario::isIgnoreTriangle(const Triangle* pTriangle) {
    return MR::isNearZero(triangleNormal(pTriangle).dot(*getGravityVec()), 0.01f);
}

void Mario::checkBaseTransBall() {
    TVec3f center = mPosition + mVelocity;
    TVec3f upper = center + mHeadVec * cFootOffset;
    s32 hitNum = Collision::checkStrikeBallToMap(upper, cBodyRadius, nullptr, nullptr);

    for (u32 i = 0; i < static_cast< u32 >(hitNum); i++) {
        doSwimmingHitCheck(Collision::getStrikeInfoMap(i), 0);
    }

    TVec3f lower = center - mHeadVec * cFootOffset;
    hitNum = Collision::checkStrikeBallToMap(lower, cBodyRadius, nullptr, nullptr);

    for (u32 i = 0; i < static_cast< u32 >(hitNum); i++) {
        doSwimmingHitCheck(Collision::getStrikeInfoMap(i), 1);
    }

    hitNum = Collision::checkStrikeBallToMap(center, cFootOffset, nullptr, nullptr);

    for (u32 i = 0; i < static_cast< u32 >(hitNum); i++) {
        doSwimmingHitCheck(Collision::getStrikeInfoMap(i), 2);
    }
}

void Mario::createAtField(bool useFloorRadius, f32 radius) {
    _578 = 0;

    f32 hitRadius = cBodyRadius;
    f32 gravityOffset = cBodyRadius;
    u32 passCount = 2;

    if (isSwimming()) {
        hitRadius = cFootOffset;
        gravityOffset = 30.0f;
    }

    if (mMovementStates._A) {
        hitRadius = cFootOffset;
        gravityOffset = cFootOffset;
        passCount = 1;
    }

    if (useFloorRadius) {
        hitRadius = radius < cFootOffset ? cFootOffset : radius;
        gravityOffset = 30.0f;
    }

    for (u32 pass = 0; pass < passCount && _578 < 0x20; pass++) {
        TVec3f gravityStep = *getAirGravityVec();
        gravityStep.scale(gravityOffset * static_cast< f32 >(pass + 1));
        TVec3f center = mPosition - gravityStep;
        s32 hitNum;

        if (useFloorRadius && mMovementStates._F && _544 > 1) {
            hitNum = Collision::checkStrikeBallToMap(center, hitRadius, nullptr, nullptr);
        }
        else {
            hitNum = Collision::checkStrikeBallToMapWithThickness(center, hitRadius, hitRadius, nullptr, nullptr);
        }

        for (u32 i = 0; i < static_cast< u32 >(hitNum) && _578 < 0x20; i++) {
            const HitInfo* pInfo = Collision::getStrikeInfoMap(i);
            const Triangle& rTriangle = pInfo->mParentTriangle;

            if (useFloorRadius && mMovementStates._F && _544 > 1) {
                const TVec3f& normal = triangleNormal(&rTriangle);
                f32 angle = calcAngleD(normal);
                if (angle < cWallAngleD || angle > 80.0f) {
                    continue;
                }

                TVec3f fromHit = pInfo->mHitPos - center;
                TVec3f horizontal;
                if (__fabsf(MR::vecKillElement(fromHit, *getAirGravityVec(), &horizontal)) > 1.0f) {
                    continue;
                }

                if (pInfo->isCollisionAtEdge()) {
                    TVec3f negNormal = normal;
                    negNormal.scale(-1.0f);
                    if (MR::diffAngleAbsHorizontal(mJumpVec, negNormal, *getAirGravityVec()) < 0.7853982f) {
                        continue;
                    }
                }
            }

            if (!useFloorRadius && !isSwimming()) {
                if (isPressSensor(&rTriangle) || isThroughWall(&rTriangle) || MR::isThroughPolygon(&rTriangle)) {
                    continue;
                }

                const TVec3f& normal = triangleNormal(&rTriangle);
                f32 limitAngle = _95C->getCode(&rTriangle) == 3 ? cWallAngleD : cDangerAngleD;

                if (_10._37 && isMovingMtx(&rTriangle) && mGroundPolygon->mSensor != rTriangle.mSensor) {
                    limitAngle = 0.0f;
                    push(normal * pInfo->_60);
                }

                if (calcAngleD(normal) < limitAngle) {
                    continue;
                }
            }

            appendUniqueTriangle(_57C, &_578, 0x20, rTriangle);
        }
    }
}

void Mario::doSwimmingHitCheck(const HitInfo* pHit, u32 hitKind) {
    if (pHit == nullptr) {
        return;
    }

    const Triangle& rTriangle = pHit->mParentTriangle;
    if (MR::isThroughPolygon(&rTriangle) || isThroughWall(&rTriangle)) {
        return;
    }

    damagePolygonCheck(&rTriangle);

    const TVec3f& normal = triangleNormal(&rTriangle);
    TVec3f negNormal = normal;
    negNormal.scale(-1.0f);
    if (mVelocity.dot(negNormal) < 0.0f) {
        return;
    }

    if (mSwim != nullptr) {
        mSwim->hitWall(normal, rTriangle.mSensor);
    }

    if (_97C != nullptr) {
        if (hitKind == 0) {
            _97C->hitWall(normal, rTriangle.mSensor);
        }
        else {
            _97C->hitPoly(static_cast< u8 >(hitKind), normal, rTriangle.mSensor);
        }
    }
}

void Mario::doSpinPunchAroundPolygons() {
    if (!mActor->isPunching() || !mActor->isInPunchTimerRange()) {
        return;
    }

    TVec3f center = mPosition + mVelocity + mHeadVec * 30.0f;
    TVec3f side = mSideVec * 80.0f;
    TVec3f front = mFrontVec * 80.0f;
    TVec3f up = mHeadVec * 80.0f;
    TVec3f start = center + front + side + up;
    TVec3f end = center - front - side - up;
    Triangle triangles[0x100];
    const u32 numTriangles = MR::createAreaPolygonList(triangles, 0x100, start, end);

    for (u32 i = 0; i < numTriangles; i++) {
        if (triangles[i].mSensor != nullptr) {
            sendPunch(triangles[i].mSensor, true);
        }
    }
}

void Mario::checkMap() {
    calcShadowPos();

    if (isStatusActive(6)) {
        mGroundPos = mShadowPos;
    }

    TVec3f gravity = *getGravityVec();
    if (mMovementStates._1 && isSlipPolygon(mGroundPolygon)) {
        gravity = _374;
    }

    if (isUseSimpleGroundCheck()) {
        if (!mMovementStates._34) {
            mVerticalSpeed = (mGroundPos - mPosition).dot(gravity);
        }
        else {
            mVerticalSpeed = (mShadowPos - mPosition).dot(gravity);
        }

        if (!_24) {
            _148.zero();
        }
    }
    else {
        mVerticalSpeed = (mShadowPos - mPosition).dot(gravity);
    }

    if (mVerticalSpeed < 0.0f) {
        mVerticalSpeed = 0.0f;
    }

    if (_10._23 && calcAngleD(_368) >= 45.0f) {
        TVec3f start = mPosition + _368 * 140.0f;
        TVec3f offset = -_904 * 1000.0f;
        _10._24 = MR::getFirstPolyOnLineToMap(&_498, _460, start, offset);
    }
    else {
        _10._24 = 0;
    }
}

f32 Mario::calcDistToCeil(bool resetTimer) {
    if (resetTimer) {
        _730 = 0;
    }

    f32 startOffset = isStatusActive(5) ? 0.0f : 45.0f;
    Triangle triangle;
    TVec3f hitPos;

    for (u32 i = 0; i < 2; i++) {
        TVec3f startStep = *getAirGravityVec();
        startStep.scale(startOffset);
        TVec3f start = mPosition + startStep;
        TVec3f offset = *getAirGravityVec();
        offset.scale(-1.0f);
        offset.scale(cCeilProbe + startOffset);

        bool hit = MR::getFirstPolyOnLineToMap(&hitPos, &triangle, start, offset);
        if (hit && isThroughWall(&triangle)) {
            hit = false;
        }

        if (hit) {
            const bool oldGroundPress = isPressSensor(mGroundPolygon);
            if (!isPressSensor(&triangle) && _95C->getCode(&triangle) != 0x1D && !oldGroundPress && _960 != 0x1D) {
                f32 dist = MR::vecKillElement(hitPos - start, *getAirGravityVec(), &offset);
                if (dist < 0.0f) {
                    dist = -dist;
                }
                *_460 = triangle;
                _498 = hitPos;
                return dist;
            }
        }

        startOffset = 0.0f;
    }

    return cCeilProbe;
}

f32 Mario::calcDistToCeilOnPress() {
    if (!mMovementStates._1) {
        return calcDistToCeil(false);
    }

    TVec3f startStep = *getGravityVec() * 20.0f;
    TVec3f start = mPosition - startStep;
    TVec3f offset = *getGravityVec();
    offset.scale(-1.0f);
    offset.scale(cCeilProbe);
    TVec3f hitPos;

    if (MR::getFirstPolyOnLineToMap(&hitPos, _460, start, offset) && isPressSensor(_460)) {
        return __fabsf(MR::vecKillElement(hitPos - start, *getGravityVec(), &offset));
    }

    return calcDistToCeil(false);
}

f32 Mario::calcDistToCeilHead() {
    TVec3f start = mPosition + mHeadVec * cHeadOffset;
    TVec3f offset = mHeadVec * cCeilProbe;
    TVec3f hitPos;

    if (MR::getFirstPolyOnLineToMap(&hitPos, _460, start, offset) && !isThroughWall(_460)) {
        return __fabsf(MR::vecKillElement(hitPos - start, mHeadVec, &offset));
    }

    return cCeilProbe;
}

void Mario::fixTransBetweenWall(const TVec3f& rPos, const TVec3f& rMove) {
    TVec3f hitPos;
    Triangle triangle;

    if (MR::getFirstPolyOnLineToMap(&hitPos, &triangle, rPos, rMove) && !isThroughWall(&triangle)) {
        TVec3f pushDir = rPos - hitPos;
        TVec3f horizontal;
        MR::vecKillElement(pushDir, triangleNormal(&triangle), &horizontal);
        mPosition += horizontal;
        mActor->mPosition = mPosition;
    }
}

f32 Mario::calcDistWidth() {
    TVec3f hitPos;
    Triangle triangle;
    f32 nearest = cCeilProbe;

    const TVec3f dirs[4] = { mFrontVec, -mFrontVec, mSideVec, -mSideVec };
    for (u32 i = 0; i < 4; i++) {
        TVec3f offset = dirs[i] * cCeilProbe;
        if (MR::getFirstPolyOnLineToMap(&hitPos, &triangle, mPosition, offset) && !isThroughWall(&triangle)) {
            f32 dist = __fabsf(MR::vecKillElement(hitPos - mPosition, dirs[i], &offset));
            if (dist < nearest) {
                nearest = dist;
            }
        }
    }

    return nearest;
}

void Mario::updateCameraPolygon() {
    const Triangle* pTriangle = MR::getCameraPolyFast(mPosition, *getGravityVec(), nullptr);
    setCameraPolygon(pTriangle);
}

void Mario::setCameraPolygon(const Triangle* pTriangle) {
    if (pTriangle != nullptr) {
        *_470 = *pTriangle;
    }
}

void Mario::checkAllWall(const TVec3f& rCenter, f32 radius) {
    _568 = _56C = 0;
    _564 = -1;
    _4E8.zero();
    _4F4.zero();

    const s32 hitNum = Collision::checkStrikeBallToMap(rCenter, radius, nullptr, _458);
    for (u32 i = 0; i < static_cast< u32 >(hitNum); i++) {
        const HitInfo* pInfo = Collision::getStrikeInfoMap(i);
        const Triangle& rTriangle = pInfo->mParentTriangle;
        if (isThroughWall(&rTriangle)) {
            continue;
        }

        const TVec3f& normal = triangleNormal(&rTriangle);
        f32 front = normal.dot(mFrontVec);
        f32 side = normal.dot(mSideVec);

        if (front < _4E0) {
            _4E0 = front;
            *mFrontWallTriangle = rTriangle;
            _4E8 = pInfo->mHitPos;
            _568 = 1;
        }
        if (front > _4E4) {
            _4E4 = front;
            *mBackWallTriangle = rTriangle;
            _4F4 = pInfo->mHitPos;
            _56C = 1;
        }
        if (__fabsf(side) > __fabsf(_4BC.x)) {
            _4BC.x = side;
            *mSideWallTriangle = rTriangle;
        }
    }
}

void Mario::calcFrontFloor() {
    TVec3f frontOffset = mFrontVec * cBodyRadius;
    TVec3f gravityOffset = *getGravityVec();
    gravityOffset.scale(20.0f);
    TVec3f start = mPosition + frontOffset - gravityOffset;
    TVec3f offset = *getGravityVec();
    offset.scale(cCeilProbe);
    MR::getFirstPolyOnLineToMap(&_4A4, _474, start, offset);
}

const TVec3f& Mario::getWallNorm() const {
    if (isValidTriangle(mFrontWallTriangle)) {
        return triangleNormal(mFrontWallTriangle);
    }

    return _368;
}

const TVec3f& Mario::getSideWallNorm() const {
    if (isValidTriangle(mSideWallTriangle)) {
        return triangleNormal(mSideWallTriangle);
    }

    return _344;
}

const TVec3f& Mario::getFrontWallNorm() const {
    if (isValidTriangle(mFrontWallTriangle)) {
        return triangleNormal(mFrontWallTriangle);
    }

    return mFrontVec;
}

const TVec3f& Mario::getBackWallNorm() const {
    if (isValidTriangle(mBackWallTriangle)) {
        return triangleNormal(mBackWallTriangle);
    }

    return _220;
}

const TVec3f& Mario::getWallPos() const {
    return _4E8;
}

const Triangle* Mario::getWallPolygon() const {
    if (isValidTriangle(mFrontWallTriangle)) {
        return mFrontWallTriangle;
    }
    if (isValidTriangle(mSideWallTriangle)) {
        return mSideWallTriangle;
    }
    if (isValidTriangle(mBackWallTriangle)) {
        return mBackWallTriangle;
    }

    return nullptr;
}

const Triangle* Mario::getGroundPolygon() const {
    return mGroundPolygon;
}

void Mario::updateFloorCode() {
    _962 = _960;
    _960 = _95C->getCode(mGroundPolygon);
    _964[2] = _964[1];
    _964[1] = _964[0];
    _964[0] = _960;
}

void Mario::updateWallFloorCode() {
    const Triangle* pWall = getWallPolygon();
    _96A = pWall != nullptr ? _95C->getCode(pWall) : 0xFFFF;
}

void Mario::saveLastSafetyTrans() {
    if (mMovementStates._1 && mGroundPolygon->isValid() && !MR::isGroundCodeDeath(mGroundPolygon) && !MR::isGroundCodeDamage(mGroundPolygon)) {
        _8A4 = mPosition;
        *_8C8 = *mGroundPolygon;
        _898 = 1;
    }
}

void Mario::setNotSafetyTimer() {
    _3C2 = 30;
}

TVec3f* Mario::getLastSafetyTrans(TVec3f* pOut) const {
    *pOut = _8A4;
    return pOut;
}

bool Mario::checkCurrentFloorCodeSevere(u32 code) const {
    if (_960 == code || _962 == code) {
        return true;
    }

    for (u32 i = 0; i < 3; i++) {
        if (_964[i] == code) {
            return true;
        }
    }

    return false;
}

bool Mario::isCurrentFloorSink() const {
    return checkCurrentFloorCodeSevere(0x1B) || MR::isGroundCodeSinkDeath(mGroundPolygon) || MR::isGroundCodeSinkDeathMud(mGroundPolygon);
}

bool Mario::isCurrentFloorSand() const {
    return MR::isGroundCodeSand(mGroundPolygon) || MR::isGroundCodeNoStampSand(mGroundPolygon);
}

bool Mario::isCurrentShadowFloorDangerAction() const {
    return MR::isGroundCodeDeath(_45C) || MR::isGroundCodeDamage(_45C) || MR::isGroundCodeDamageFire(_45C) || MR::isGroundCodeDamageElectric(_45C);
}

bool Mario::checkBaseTransPoint() {
    HitInfo hitInfo;
    if (Collision::checkStrikePointToMap(mPosition + mVelocity, &hitInfo) == 0) {
        return false;
    }

    if (MR::isThroughPolygon(&hitInfo.mParentTriangle) || isThroughWall(&hitInfo.mParentTriangle)) {
        return false;
    }

    *_45C = hitInfo.mParentTriangle;
    _4C = hitInfo.mHitPos;
    return true;
}

bool Mario::checkHeadPoint() {
    HitInfo hitInfo;
    TVec3f pos = mPosition + mHeadVec * cHeadOffset;
    if (Collision::checkStrikePointToMap(pos, &hitInfo) == 0) {
        return false;
    }

    if (MR::isThroughPolygon(&hitInfo.mParentTriangle) || isThroughWall(&hitInfo.mParentTriangle)) {
        return false;
    }

    *_460 = hitInfo.mParentTriangle;
    _498 = hitInfo.mHitPos;
    return true;
}

const TVec3f* Mario::calcShadowPos() {
    TVec3f startStep = *getGravityVec();
    startStep.scale(20.0f);
    TVec3f start = mPosition - startStep;
    TVec3f offset = *getGravityVec();
    offset.scale(1000.0f);
    TVec3f hitPos;

    if (MR::getFirstPolyOnLineToMap(&hitPos, _45C, start, offset) && !isThroughWall(_45C)) {
        mShadowPos = hitPos;
    }
    else {
        mShadowPos = mPosition;
    }

    return &mShadowPos;
}

void Mario::updateBinderInfo() {
    Binder* pBinder = mActor->mBinder;
    if (pBinder == nullptr) {
        return;
    }

    if (pBinder->mGroundInfo.mParentTriangle.isValid()) {
        *mGroundPolygon = pBinder->mGroundInfo.mParentTriangle;
        mGroundPos = pBinder->mGroundInfo.mHitPos;
        _368 = triangleNormal(mGroundPolygon);
        _374 = -_368;
        mMovementStates._1 = 1;
    }
    else {
        mMovementStates._1 = 0;
    }

    if (pBinder->mWallInfo.mParentTriangle.isValid()) {
        *mFrontWallTriangle = pBinder->mWallInfo.mParentTriangle;
        _4E8 = pBinder->mWallInfo.mHitPos;
        _568 = 1;
    }
    else {
        _568 = 0;
    }

    if (pBinder->mRoofInfo.mParentTriangle.isValid()) {
        *_460 = pBinder->mRoofInfo.mParentTriangle;
        _498 = pBinder->mRoofInfo.mHitPos;
        _10._24 = 1;
    }
    else {
        _10._24 = 0;
    }
}

bool Mario::isThroughWall(const Triangle* pTriangle) const {
    if (!isValidTriangle(pTriangle)) {
        return true;
    }

    if (MR::isThroughPolygon(pTriangle)) {
        return true;
    }

    return MR::isWallCodeGhostThrough(pTriangle) || MR::isWallCodeNoAction(pTriangle);
}

bool Mario::checkGround() {
    if (!isEnableCheckGround()) {
        mMovementStates._1 = 0;
        return false;
    }

    if (mActor->mBinder != nullptr) {
        updateBinderInfo();
    }
    else {
        calcShadowPos();
        *mGroundPolygon = *_45C;
        mGroundPos = mShadowPos;
        _368 = triangleNormal(mGroundPolygon);
        _374 = -_368;
        mMovementStates._1 = mGroundPolygon->isValid();
    }

    updateFloorCode();
    updateWallFloorCode();
    saveLastSafetyTrans();
    return mMovementStates._1;
}

CubeCameraArea* Mario::getCameraCubeCode() const {
    if (isSwimming()) {
        bool isSurface = mSwim->mIsOnSurface || mSwim->mIsSwimmingAtSurface;

        if (isSurface) {
            TVec3f gravity(*getGravityVec());
            gravity.scale(100.0f);
            TVec3f pos(mPosition);
            pos += gravity;
            return reinterpret_cast< CubeCameraArea* >(MR::getAreaObj("CubeCamera", pos));
        }
    }
    else if (mMovementStates.jumping && isRising()) {
        TVec3f gravity(*getGravityVec());
        gravity.scale(100.0f);
        TVec3f pos(mPosition);
        pos += gravity;
        return reinterpret_cast< CubeCameraArea* >(MR::getAreaObj("CubeCamera", pos));
    }

    return reinterpret_cast< CubeCameraArea* >(MR::getAreaObj("CubeCamera", mPosition));
}

void Mario::updateCubeCode() {
    const Triangle* pTriangle = _470;
    if (pTriangle != nullptr && pTriangle->isValid()) {
        _750 = MR::getCameraID(pTriangle);
    }
}
