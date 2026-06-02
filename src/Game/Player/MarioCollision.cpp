#include "Game/Player/Mario.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioDamage.hpp"
#include "Game/Player/MarioHang.hpp"
#include "Game/Player/MarioMapCode.hpp"
#include "Game/Player/MarioState.hpp"
#include "Game/Player/MarioSwim.hpp"
#include "Game/Util/AreaObjUtil.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Map/SunshadeMapHolder.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "math_types.hpp"
#include <cstring>

namespace MR {
    void tryToUpdatePlayerRestartIdInfo(const TVec3f&);
    void calcCylinderCenterPos(TVec3f*, const AreaObj*);
};

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
    if (!_480->isValid() || !_484->isValid()) {
        return 0.0f;
    }

    TVec3f killVec;

    TVec3f toPressA = mPosition - *_480->calcAndGetPos(0);
    f32 distA = MR::vecKillElement(toPressA, *_480->calcAndGetNormal(0), &killVec);

    TVec3f toPressB = mPosition - *_484->calcAndGetPos(0);
    f32 distB = MR::vecKillElement(toPressB, *_484->calcAndGetNormal(0), &killVec);

    if (_480->getNormal(0)->dot(mAirGravityVec) < 0.0f) {
        TVec3f push = *_480->calcAndGetNormal(0);
        push.scale(MR::vecKillElement(mPosition - *_480->calcAndGetPos(0), *_480->calcAndGetNormal(0), &killVec));
        mPosition += push;
    }
    else {
        TVec3f push = *_484->calcAndGetNormal(0);
        push.scale(MR::vecKillElement(mPosition - *_484->calcAndGetPos(0), *_484->calcAndGetNormal(0), &killVec));
        mPosition += push;
    }

    mActor->mPosition = mPosition;

    if (distA > distB) {
        return distB;
    }

    return distA;
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
    TVec3f center = rPos + rMove;
    center.scale(0.5f);

    TVec3f offset = center - mPosition;
    MR::vecKillElement(offset, *getGravityVec(), &center);
    setTrans(mPosition + center, nullptr);
}

f32 Mario::calcDistWidth() {
    TVec3f hitPos;
    Triangle triangle;

    if (mMovementStates._8) {
        TVec3f offset = triangleNormal(mFrontWallTriangle) * cHeadOffset;
        if (MR::getFirstPolyOnLineToMap(&hitPos, &triangle, _4E8, offset)) {
            fixTransBetweenWall(hitPos, _4E8);
            return (hitPos - _4E8).length();
        }
    }
    else if (mMovementStates._19) {
        TVec3f offset = triangleNormal(mBackWallTriangle) * cHeadOffset;
        if (MR::getFirstPolyOnLineToMap(&hitPos, &triangle, _4F4, offset)) {
            fixTransBetweenWall(hitPos, _4F4);
            return (hitPos - _4F4).length();
        }
    }
    else if (mMovementStates._1A) {
        TVec3f offset = triangleNormal(mSideWallTriangle) * cHeadOffset;
        if (MR::getFirstPolyOnLineToMap(&hitPos, &triangle, _500, offset)) {
            fixTransBetweenWall(hitPos, _500);
            return (hitPos - _500).length();
        }
    }

    TVec3f center = mPosition + mHeadVec * cCeilProbe;
    const s32 hitNum = Collision::checkStrikeBallToMap(center, 20.0f, nullptr, nullptr);
    if (hitNum < 2) {
        return cHeadOffset;
    }

    f32 bestDist = cHeadOffset;
    TVec3f bestPosA;
    TVec3f bestPosB;

    for (u32 i = 0; i < static_cast< u32 >(hitNum); i++) {
        const HitInfo* pInfoA = Collision::getStrikeInfoMap(i);
        const TVec3f& normalA = triangleNormal(&pInfoA->mParentTriangle);

        for (u32 j = i + 1; j < static_cast< u32 >(hitNum); j++) {
            const HitInfo* pInfoB = Collision::getStrikeInfoMap(j);
            const TVec3f& normalB = triangleNormal(&pInfoB->mParentTriangle);
            if (normalA.dot(normalB) >= -0.707f) {
                continue;
            }

            TVec3f posB = pInfoB->mHitPos - center;
            TVec3f posA = pInfoA->mHitPos - center;
            if (posA.dot(posB) > 0.0f) {
                continue;
            }

            const f32 dist = (pInfoA->mHitPos - pInfoB->mHitPos).length();
            if (dist < bestDist) {
                bestDist = dist;
                bestPosA = pInfoA->mHitPos;
                bestPosB = pInfoB->mHitPos;
            }
        }
    }

    if (bestDist < cHeadOffset) {
        fixTransBetweenWall(bestPosA, bestPosB);
    }

    return bestDist;
}

void Mario::updateCameraPolygon() {
    Triangle* pTriangle = mMovementStates._1 ? mGroundPolygon : _45C;
    if (pTriangle->isValid()) {
        setCameraPolygon(pTriangle);
        return;
    }

    TVec3f startStep = *getGravityVec();
    startStep.scale(cHeadOffset);
    TVec3f start = mPosition - startStep;

    TVec3f ray = *getGravityVec();
    ray.scale(5000.0f);

    const Triangle* pCameraTriangle = MR::getCameraPolyFast(start, ray, nullptr);
    if (pCameraTriangle != nullptr) {
        setCameraPolygon(pCameraTriangle);
    }
}

void Mario::setCameraPolygon(const Triangle* pTriangle) {
    *_468 = *pTriangle;
    *_46C = *pTriangle;
}

void Mario::checkAllWall(const TVec3f& rCenter, f32 radius) {
    _4E8.zero();
    _4F4.zero();
    _4D8->mIdx = -1;
    _4DC->mIdx = -1;

    TVec3f center(rCenter);
    TVec3f wallFront(mFrontVec);
    if (mMovementStates._F) {
        TVec3f lastMove;
        mActor->getLastMove(&lastMove);
        MR::vecKillElement(lastMove, *getGravityVec(), &lastMove);
        if (!MR::isNearZero(lastMove, 0.001f)) {
            MR::normalize(&lastMove);
            wallFront = lastMove;
        }
    }

    const HitInfo* candidates[3] = { nullptr, nullptr, nullptr };
    bool throughWall = false;
    bool useTeresaFallback = false;
    s32 hitNum = 0;

    if (getPlayerMode() == 6) {
        radius = 110.0f;
        hitNum = Collision::checkStrikeBallToMapWithThickness(center, radius, radius, nullptr, nullptr);
        if (hitNum == 0) {
            useTeresaFallback = true;
            center += mFrontVec * 120.0f;
            hitNum = Collision::checkStrikeBallToMapWithThickness(center, 25.0f, 25.0f, nullptr, nullptr);
        }
    }
    else {
        hitNum = Collision::checkStrikeBallToMap(center, radius, nullptr, nullptr);
    }

    const MarioConstTable* pConst = mActor->getConst().getTable();
    for (u32 i = 0; i < static_cast< u32 >(hitNum); i++) {
        const HitInfo* pInfo = Collision::getStrikeInfoMap(i);
        const Triangle& rTriangle = pInfo->mParentTriangle;
        const TVec3f& normal = triangleNormal(&rTriangle);

        TVec3f hitDir = pInfo->mHitPos - center;
        MR::normalizeOrZero(&hitDir);

        if (_10_HIGH_WORD & 0x00000100) {
            if (__fabsf(normal.dot(_6A0)) > 0.707f) {
                continue;
            }
        }

        const f32 wallAngle = marioAcos(-getGravityVec()->dot(normal)) * 180.0f / 3.1415927f;
        TVec3f playerToHit = pInfo->mHitPos - mPosition;
        MR::normalizeOrZero(&playerToHit);

        if (wallAngle >= pConst->mFlatAngle && hitDir.dot(mFrontVec) > 0.0f && playerToHit.dot(mFrontVec) > 0.0f &&
            calcAngleD(normal) <= pConst->mSlipAngle && MR::diffAngleAbs(_368, normal) > 30.0f) {
            *_4DC = rTriangle;
            _518 = pInfo->mHitPos;
        }

        if (__fabsf(normal.dot(mHeadVec)) > 0.01f) {
            if (hitDir.dot(*getGravityVec()) > 0.0f && hitDir.dot(mFrontVec) > 0.707f) {
                *_4D8 = rTriangle;
                _50C = pInfo->mHitPos;
            }
            continue;
        }

        f32 front = normal.dot(wallFront);
        s32 slot = 1;
        if (front >= pConst->mWallBackAngleRange) {
            slot = 2;
        }
        else if (front <= -pConst->mWallFrontAngleRange) {
            slot = 0;
        }

        if (candidates[slot] != nullptr && candidates[slot]->_60 > pInfo->_60) {
            continue;
        }

        if (isThroughWall(&rTriangle)) {
            throughWall = true;
            mActor->_F44 = false;
        }
        else {
            candidates[slot] = pInfo;
        }
    }

    if (!throughWall && getPlayerMode() == 6 && !isStatusActive(0x13)) {
        mActor->_F44 = true;
    }

    HitInfo pointInfo;
    if (MR::checkStrikePointToMap(center, &pointInfo)) {
        const TVec3f& normal = triangleNormal(&pointInfo.mParentTriangle);
        if (__fabsf(normal.dot(mHeadVec)) <= 0.01f) {
            f32 front = normal.dot(wallFront);
            s32 slot = 1;
            if (front >= 0.707f) {
                slot = 2;
            }
            else if (front <= -0.707f) {
                slot = 0;
            }

            candidates[slot] = &pointInfo;
        }
    }

    mMovementStates._8 = false;
    mMovementStates._19 = false;
    mMovementStates._1A = false;
    _10_HIGH_WORD &= ~0x00003C00;

    if (candidates[0] != nullptr) {
        const HitInfo* pInfo = candidates[0];
        TVec3f hitDir = pInfo->mHitPos - center;
        MR::normalizeOrZero(&hitDir);
        const f32 frontPush = hitDir.dot(triangleNormal(&pInfo->mParentTriangle));

        *mFrontWallTriangle = pInfo->mParentTriangle;
        _4E8 = pInfo->mHitPos;
        _4E0 = triangleNormal(mFrontWallTriangle).dot(mFrontVec);

        f32 wallThreshold = -0.999f;
        if (isStatusActive(1)) {
            wallThreshold = -0.9f;
        }

        if (frontPush < wallThreshold) {
            mMovementStates._8 = true;
        }
        else {
            mMovementStates_HIGH_WORD |= 0x00002000;
        }
    }

    if (!useTeresaFallback && candidates[1] != nullptr) {
        bool useSide = true;
        const HitInfo* pInfo = candidates[1];
        if (isPlayerModeTeresa()) {
            TVec3f toHit = pInfo->mHitPos - center;
            if (triangleNormal(&pInfo->mParentTriangle).dot(toHit) >= 0.0f) {
                useSide = false;
            }
        }

        if (useSide) {
            *mSideWallTriangle = pInfo->mParentTriangle;
            _500 = pInfo->mHitPos;
            _4BC.x = triangleNormal(mSideWallTriangle).dot(mSideVec);
            mMovementStates._1A = true;

            if (pInfo->isCollisionAtCorner() || pInfo->isCollisionAtEdge()) {
                mMovementStates_HIGH_WORD |= 0x00001000;
            }
        }
    }

    if (candidates[2] != nullptr) {
        bool useBack = true;
        const HitInfo* pInfo = candidates[2];
        if (mMovementStates._8) {
            TVec3f backToCenter = _4F4 - center;
            TVec3f frontToCenter = _4E8 - center;
            if (backToCenter.dot(frontToCenter) >= 0.0f && backToCenter.length() <= frontToCenter.length()) {
                useBack = false;
            }
        }

        if (isPlayerModeTeresa()) {
            TVec3f toHit = pInfo->mHitPos - center;
            if (triangleNormal(&pInfo->mParentTriangle).dot(toHit) >= 0.0f) {
                useBack = false;
            }
        }

        if (useBack) {
            *mBackWallTriangle = pInfo->mParentTriangle;
            _4F4 = pInfo->mHitPos;
            _4E4 = triangleNormal(mBackWallTriangle).dot(mFrontVec);
            mMovementStates._19 = true;
        }
    }

    updateWallFloorCode();
    _38C = -_380;

    if (_400 != 0) {
        _400--;
        mMovementStates_LOW_WORD |= 0x00800000;
    }
}

void Mario::calcFrontFloor() {
    Triangle frontTriangle;
    Triangle floorTriangle;

    mMovementStates._A = false;
    mMovementStates._32 = false;
    _47C->mIdx = -1;
    _4E0 = 0.0f;

    TVec3f hitPos;
    TVec3f start;
    bool hitFront = false;

    if (mMovementStates._8) {
        hitPos = _4E8;
        start = mActor->_2A0;
        hitFront = true;
    }
    else {
        TVec3f ray = mFrontVec;
        ray.scale(100.0f);

        TVec3f gravityOffset = *getGravityVec();
        gravityOffset.scale(8.0f);
        start = mPosition - gravityOffset;

        if (mMovementStates._F && !MR::isNearZero(_328, 0.001f)) {
            MR::vecKillElement(_328, *getGravityVec(), &ray);
            ray.setLength(200.0f);
        }

        hitFront = MR::getFirstPolyOnLineToMap(&hitPos, &frontTriangle, start, ray);
    }

    if (!hitFront) {
        _4E0 = 0.0f;
        return;
    }

    TVec3f frontDelta = hitPos - start;
    _4E4 = frontDelta.dot(mFrontVec);

    const TVec3f& frontNormal = triangleNormal(&frontTriangle);
    f32 gravityDot = frontNormal.dot(*getAirGravityVec());
    if (__fabsf(gravityDot) >= 0.1f) {
        if (mMovementStates._1 && calcAngleD(_368) >= 25.0f) {
            mMovementStates._32 = false;
        }

        return;
    }

    f32 angle = marioAcos(-gravityDot);

    TVec3f airGravityStep = *getAirGravityVec();
    airGravityStep.scale(200.0f);
    TVec3f probeStart = hitPos - airGravityStep;
    probeStart += mFrontVec * 20.0f;

    TVec3f probeOffset = *getAirGravityVec();
    probeOffset.scale(210.0f);

    TVec3f floorPos;
    bool hitFloor = MR::getFirstPolyOnLineBFast(probeStart, probeOffset, &floorPos, &floorTriangle);
    if (hitFloor && triangleNormal(&floorTriangle).dot(*getAirGravityVec()) > -0.9f) {
        hitFloor = false;
    }

    if (!hitFloor) {
        _4E0 = 400.0f;
    }
    else {
        TVec3f heightDelta = hitPos - floorPos;
        _4E0 = heightDelta.dot(*getAirGravityVec());

        if (_4E0 > 0.0f) {
            mMovementStates._A = true;
        }

        f32 adjustedHeight = 0.0f;
        if (angle < 1.5707964f && angle != 0.0f) {
            f32 tangent = __fabsf(angle) > 0.0001f ? (MR::sin(angle) / MR::cos(angle)) : 0.0f;
            if (tangent != 0.0f) {
                adjustedHeight = _4E0 / tangent;
            }
        }

        TVec3f oldFloorPos = floorPos;
        floorPos -= mFrontVec * (10.0f + adjustedHeight);

        TVec3f existStep = *getAirGravityVec();
        existStep.scale(2.0f);
        TVec3f existStart = floorPos - existStep;
        TVec3f existOffset = *getGravityVec();
        existOffset.scale(1000.0f);
        if (!MR::isExistMapCollision(existStart, existOffset)) {
            floorPos = oldFloorPos;
        }
        else {
            _4A4 = floorPos;
            mMovementStates._32 = true;
            *_47C = floorTriangle;
        }
    }

    if (mMovementStates._1 && calcAngleD(_368) >= 25.0f) {
        mMovementStates._32 = false;
    }
}

const TVec3f& Mario::getWallNorm() const {
    if (mMovementStates._8) {
        return triangleNormal(mFrontWallTriangle);
    }

    if (mMovementStates._19) {
        return triangleNormal(mBackWallTriangle);
    }

    if (mMovementStates._1A) {
        return triangleNormal(mSideWallTriangle);
    }

    if (mFrontWallTriangle->isValid()) {
        return triangleNormal(mFrontWallTriangle);
    }

    return mFrontVec;
}

const TVec3f& Mario::getSideWallNorm() const {
    if (mMovementStates._1A) {
        return triangleNormal(mSideWallTriangle);
    }

    return *reinterpret_cast< const TVec3f* >(&gZeroVec);
}

const TVec3f& Mario::getFrontWallNorm() const {
    if (mMovementStates._8) {
        return triangleNormal(mFrontWallTriangle);
    }

    return *reinterpret_cast< const TVec3f* >(&gZeroVec);
}

const TVec3f& Mario::getBackWallNorm() const {
    if (mMovementStates._19) {
        return triangleNormal(mBackWallTriangle);
    }

    return *reinterpret_cast< const TVec3f* >(&gZeroVec);
}

const TVec3f& Mario::getWallPos() const {
    if (mMovementStates._8) {
        return _4E8;
    }

    if (mMovementStates._19) {
        return _4F4;
    }

    if (mMovementStates._1A) {
        return _500;
    }

    return mActor->_2A0;
}

const Triangle* Mario::getWallPolygon() const {
    if (mMovementStates._8) {
        return mFrontWallTriangle;
    }

    if (mMovementStates._19) {
        return mBackWallTriangle;
    }

    if (mMovementStates._1A) {
        return mSideWallTriangle;
    }

    if (mFrontWallTriangle->isValid()) {
        return mFrontWallTriangle;
    }

    return nullptr;
}

const Triangle* Mario::getGroundPolygon() const {
    return mGroundPolygon;
}

void Mario::updateFloorCode() {
    u16 groundCode = _95C->getCode(mGroundPolygon);
    if (groundCode != 0xFFFF) {
        _960 = groundCode;
    }

    u16 shadowCode = _95C->getCode(_45C);
    if (shadowCode != 0xFFFF) {
        _962 = shadowCode;
    }

    if (mActor->mAlphaEnable) {
        _41C = 15;
        return;
    }

    const MarioConstTable* pConst = mActor->getConst().getTable();
    const f32 floorAngle = calcPolygonAngleD(mGroundPolygon);
    if (floorAngle > pConst->mSlipAngle) {
        if (floorAngle > pConst->mForceWallAngle) {
            _41C = 0;
        }

        if (_41C != 0) {
            _41C--;
        }

        if (_41C == 0) {
            _960 = 0x80;
        }
    }
    else if (!isSlipFloorCode(_960)) {
        _41C = 15;
    }
}

void Mario::updateWallFloorCode() {
    if (mMovementStates._8 || mMovementStates._32) {
        u16 code = _95C->getCode(mFrontWallTriangle);
        if (code != 0xFFFF) {
            _964[0] = code;
        }
    }

    if (mMovementStates._19) {
        u16 code = _95C->getCode(mBackWallTriangle);
        if (code != 0xFFFF) {
            _964[1] = code;
        }
    }

    if (mMovementStates._1A) {
        u16 code = _95C->getCode(mSideWallTriangle);
        if (code != 0xFFFF) {
            _964[2] = code;
        }
    }
}

void Mario::saveLastSafetyTrans() {
    if (isStatusActive(8) || isStatusActive(0x13)) {
        return;
    }

    if ((_1C_WORD & 0x00004000) == 0) {
        return;
    }

    if (_96A != 0) {
        _96A--;
        return;
    }

    switch (_960) {
    case 0:
    case 3:
    case 13:
    case 14:
    case 20:
    case 21:
    case 22:
    case 23:
    case 30:
        break;
    default:
        return;
    }

    if (mGroundPolygon->mSensor->isType(0x48)) {
        return;
    }

    if (mMovementStates._8 || mMovementStates._32 || mMovementStates._19 || mMovementStates._1A) {
        return;
    }

    if (mDrawStates._C || _1C._F || mVerticalSpeed >= 10.0f) {
        return;
    }

    if (calcAngleD(triangleNormal(mGroundPolygon)) >= 15.0f) {
        return;
    }

    if (!MR::isSameMtx(mGroundPolygon->getBaseMtx()->toMtxPtr(), mGroundPolygon->getPrevBaseMtx()->toMtxPtr())) {
        return;
    }

    if (_7E0->isValid() && MR::isSameMtx(_7E0->getBaseMtx()->toMtxPtr(), _7E4.toMtxPtr()) &&
        MR::isSameMtx(_7E0->getBaseMtx()->toMtxPtr(), _7E0->getPrevBaseMtx()->toMtxPtr())) {
        _814 = _7D4;
        *_820 = *_7E0;
        PSMTXCopy(_7E4.toMtxPtr(), _824.toMtxPtr());
    }

    *_7E0 = *mGroundPolygon;
    PSMTXCopy(mGroundPolygon->getBaseMtx()->toMtxPtr(), _7E4.toMtxPtr());

    TVec3f safety = mPosition;
    safety.scale(30.0f);
    safety += *mGroundPolygon->getPos(0);
    safety += *mGroundPolygon->getPos(1);
    safety += *mGroundPolygon->getPos(2);
    safety.scale(0.125f);
    _7D4 = safety;

    TVec3f safetyOffset = _7D4 - mPosition;
    if (safetyOffset.length() > 50.0f) {
        safetyOffset.setLength(50.0f);
        _7D4 = mPosition + safetyOffset;
    }

    _1C_WORD |= 0x00020000;
}

void Mario::setNotSafetyTimer() {
    _96A = 2;

    if (_1C._E) {
        _7E0->mIdx = -1;
    }
}

TVec3f* Mario::getLastSafetyTrans(TVec3f* pOut) const {
    if (pOut != nullptr) {
        *pOut = *getAirGravityVec();
        pOut->scale(-1.0f);
    }

    if (_7E0->isValid() && MR::isSameMtx(_7E0->getBaseMtx()->toMtxPtr(), const_cast<TMtx34f&>(_7E4).toMtxPtr())) {
        if (pOut != nullptr) {
            *pOut = *_7E0->calcAndGetNormal(0);
        }

        return const_cast<TVec3f*>(&_7D4);
    }

    if (_820->isValid()) {
        if (pOut != nullptr) {
            *pOut = *_820->calcAndGetNormal(0);
        }
    }

    return const_cast<TVec3f*>(&_814);
}

bool Mario::checkCurrentFloorCodeSevere(u32 code) const {
    if (_960 != code) {
        return false;
    }

    TVec3f shadowToGround = mShadowPos - mGroundPos;
    TVec3f horizontal;
    f32 vertical = MR::vecKillElement(shadowToGround, getAirGravityVec(), &horizontal);

    if (vertical > 10.0f) {
        return _95C->getCode(mGroundPolygon) == code;
    }

    u32 shadowCode = _95C->getCode(_45C);
    if (shadowCode != code) {
        return false;
    }

    return _95C->getCode(mGroundPolygon) == shadowCode;
}

bool Mario::isCurrentFloorSink() const {
    return checkCurrentFloorCodeSevere(0x11) || checkCurrentFloorCodeSevere(0x1F) || checkCurrentFloorCodeSevere(0x12) ||
           checkCurrentFloorCodeSevere(0x19);
}

bool Mario::isCurrentFloorSand() const {
    if (mDrawStates.mIsUnderwater || mDrawStates._13) {
        return false;
    }

    return checkCurrentFloorCodeSevere(0xD) || checkCurrentFloorCodeSevere(0x1E);
}

bool Mario::isCurrentShadowFloorDangerAction() const {
    u16 shadowCode = _95C->getCode(_45C);
    u16 groundCode = _95C->getCode(mGroundPolygon);
    if (shadowCode == groundCode) {
        return false;
    }

    if (isPlayerModeTeresa()) {
        return false;
    }

    switch (shadowCode) {
    case 4:
    case 6:
    case 7:
    case 8:
    case 11:
    case 15:
    case 17:
    case 18:
    case 24:
    case 25:
    case 27:
    case 28:
    case 31:
    case 34:
        return true;
    case 10:
        return !isPlayerModeIce();
    default:
        return false;
    }
}

bool Mario::checkBaseTransPoint() {
    TVec3f point = *getGravityVec();
    point.scale(100.0f);
    point = mPosition - point;
    const bool pointHit = MR::checkStrikePointToMap(point, nullptr);

    const s32 hitNum = Collision::checkStrikeBallToMap(mPosition, 0.0f, nullptr, nullptr);
    bool pushed = false;
    for (u32 i = 0; i < static_cast<u32>(hitNum); i++) {
        const HitInfo* pInfo = Collision::getStrikeInfoMap(i);
        const Triangle& rTriangle = pInfo->mParentTriangle;
        if (MR::isThroughPolygon(&rTriangle) || isThroughWall(&rTriangle)) {
            continue;
        }

        if (pInfo->isCollisionAtEdge() || pInfo->isCollisionAtCorner()) {
            mDrawStates_WORD |= 0x00400000;
            continue;
        }

        const TVec3f& normal = triangleNormal(&rTriangle);
        if (MR::isNearZero(normal.dot(_368), 0.01f) && !pointHit) {
            continue;
        }

        TVec3f velocityDir(_16C);
        MR::normalizeOrZero(&velocityDir);

        TVec3f pushDir = -normal;
        if (_16C.dot(pushDir) < 0.707f) {
            continue;
        }

        if (__fabsf(_16C.dot(normal)) < pInfo->_60) {
            continue;
        }

        if (MR::isNearZero(pInfo->_60, 0.1f)) {
            continue;
        }

        TVec3f toHit = pInfo->mHitPos - mPosition;
        if (toHit.dot(mHeadVec) < 0.0f) {
            continue;
        }

        TVec3f push = normal;
        push.scale(pInfo->_60);
        addTrans(push, "base point");
        pushed = true;

        if (!mMovementStates._1 && mMovementStates.jumping && !isRising()) {
            if (normal.dot(*getGravityVec()) < 0.5f) {
                TVec3f blownVec(mFrontVec);
                blownVec.scale(5.0f);
                blown(blownVec);
            }
        }
    }

    return pushed;
}

bool Mario::checkHeadPoint() {
    f32 hitRadius = 40.0f;
    f32 headOffset = mMovementStates._A ? 50.0f : 110.0f;
    bool damageHead = false;

    TVec3f gravityStep = *getGravityVec();
    gravityStep.scale(headOffset);
    TVec3f checkPos = mPosition - gravityStep;

    if (isStatusActive(2)) {
        mActor->calcHeadPos();
        checkPos = mActor->_2AC;
        if (mDamage->_18 == 0) {
            hitRadius = 50.0f;
        }
        damageHead = true;
    }

    if ((mMovementStates_HIGH_WORD & 3) != 0) {
        mActor->calcHeadPos();
        TVec3f velocityStep = _16C;
        velocityStep.scale(3.0f);
        checkPos = mActor->_2AC + velocityStep;
    }

    if (getPlayerMode() == 4 && (mMovementStates_HIGH_WORD & 0x10000000) == 0 && !mMovementStates._A && !isStatusActive(0x1C) &&
        !isStatusActive(0x15) && !isStatusActive(0x1B) && !mActor->mAlphaEnable) {
        if (isStatusActive(0x16)) {
            return false;
        }

        checkPos = mActor->_2AC;
    }

    if (isSwimming()) {
        TVec3f swimStep = mHeadVec;
        swimStep.scale(60.0f);
        checkPos = mPosition + swimStep;
    }

    const s32 hitNum = Collision::checkStrikeBallToMap(checkPos, hitRadius, nullptr, nullptr);
    Triangle hitTriangles[0x20];
    TVec3f pushVec;
    pushVec.zero();

    u32 storedCount = 0;
    for (u32 i = 0; i < static_cast< u32 >(hitNum); i++) {
        const HitInfo* pInfo = Collision::getStrikeInfoMap(i);
        const Triangle& rTriangle = pInfo->mParentTriangle;
        if (MR::isThroughPolygon(&rTriangle) || isThroughWall(&rTriangle)) {
            continue;
        }

        const TVec3f& normal = triangleNormal(&rTriangle);
        if (!isSwimming() && (mMovementStates_HIGH_WORD & 3) == 0) {
            const f32 normalGravity = normal.dot(*getGravityVec());
            if (damageHead) {
                if (normalGravity < -0.707f) {
                    continue;
                }
            }
            else if (mMovementStates.jumping && (normalGravity < 0.0f || normalGravity < -0.707f)) {
                continue;
            }
        }

        TVec3f killed;
        if (MR::vecKillElement(pushVec, normal, &killed) >= pInfo->_60) {
            continue;
        }

        pushVec = killed + normal * pInfo->_60;

        if (isHeadPushEnableArea()) {
            addVelocity(normal, pInfo->_60);
        }

        if (isSwimming()) {
            mSwim->hitHead(pInfo);
            TVec3f fromCenter = pInfo->mHitPos - checkPos;
            addVelocity(normal, hitRadius - fromCenter.length());
        }

        const char* pWallCode = MR::getWallCodeString(&rTriangle);
        if (pWallCode != nullptr && strcmp(pWallCode, "頭ぶつけ") == 0) {
            mDrawStates_WORD |= 0x400;
        }

        if (storedCount < 0x20) {
            hitTriangles[storedCount] = rTriangle;
            storedCount++;
        }
    }

    addVelocity(pushVec);

    if (!MR::isNearZero(pushVec, 0.001f) && (mMovementStates_HIGH_WORD & 3) != 0) {
        TVec3f pushDir = pushVec;
        MR::normalizeOrZero(&pushDir);

        if (calcAngleD(pushDir) > 45.0f) {
            TVec3f blownVec = pushVec;
            blownVec.scale(0.2f);
            blown(blownVec);
            _402 = 0;
            _428 = 60;
            mMovementStates_HIGH_WORD |= 0x00100000;
            mMovementStates_HIGH_WORD &= ~3;
        }
        else if (_1FC.dot(pushVec) < 0.0f) {
            mMovementStates_LOW_WORD |= 0x40000000;
            mJumpVec.zero();
        }
    }

    bool canSendPunch = false;
    if (isSwimming() && mSwim->check7Aand7C()) {
        canSendPunch = true;
    }
    if (mMovementStates.jumping && isRising()) {
        canSendPunch = true;
    }

    if (canSendPunch) {
        for (u32 i = 0; i < storedCount; i++) {
            if (triangleNormal(&hitTriangles[i]).dot(*getGravityVec()) > 0.5f) {
                mActor->sendMsgUpperPunch(hitTriangles[i].mSensor);
            }
        }
    }

    return hitNum != 0;
}

const TVec3f* Mario::calcShadowPos() {
    TVec3f startStep = *getGravityVec();
    startStep.scale(cHeadOffset);
    TVec3f start = mPosition - startStep;

    if ((getCurrentStatus() == 5 && mHang->_12 < 2) || mActor->_EA4) {
        mActor->getRealPos("Spine1", &start);
    }

    const f32 prevShadowDist = (mShadowPos - mPosition).length();
    TVec3f lineStart = start;
    TVec3f offset = *getGravityVec();
    offset.scale(400.0f);

    u32 probeCount = 6;
    if (isStatusActive(0x13)) {
        probeCount = 30;
    }

    for (u32 i = 0; i < probeCount; i++) {
        mMovementStates._2 = MR::getFirstPolyOnLineToMap(&mShadowPos, _45C, lineStart, offset, nullptr, _458);
        if (mMovementStates._2) {
            break;
        }

        lineStart += offset;
    }

    const f32 shadowDist = (mShadowPos - mPosition).length();
    bool retryNearBody = false;
    if (mMovementStates._2 && __fabsf(prevShadowDist - (shadowDist + 10.0f)) > cHeadOffset) {
        retryNearBody = true;
    }

    if (!mMovementStates._2 || retryNearBody) {
        offset.setLength(prevShadowDist + cFootOffset + cHeadOffset);
        TVec3f sideFront = mSideVec + mFrontVec;
        sideFront.scale(0.1f);
        lineStart = start + sideFront;
        mMovementStates._2 = MR::getFirstPolyOnLineToMap(&mShadowPos, _45C, lineStart, offset, nullptr, _458);
        if (retryNearBody) {
            mMovementStates._2 = true;
        }
    }

    if (!mMovementStates._2) {
        TVec3f failOffset = *getGravityVec();
        failOffset.scale(1000.0f);
        mShadowPos = mPosition + failOffset;
    }

    return &mShadowPos;
}

bool Mario::updateBinderInfo() {
    _3A4.zero();
    _4C8->mIdx = -1;

    Binder* pBinder = mActor->mBinder;
    if (pBinder == nullptr) {
        return false;
    }

    const s32 planeNum = pBinder->mPlaneNum;
    if (planeNum == 0) {
        return false;
    }

    bool canStoreWall = true;
    bool canPushGround = true;
    for (s32 i = 0; i < planeNum; i++) {
        const HitInfo& rInfo = pBinder->mPlaneInfos[i];
        const Triangle* pPlane = &rInfo.mParentTriangle;
        const TVec3f& normal = triangleNormal(pPlane);

        if (MR::isThroughPolygon(pPlane) || isThroughWall(pPlane)) {
            continue;
        }

        const f32 gravityDot = normal.dot(*getGravityVec());
        f32 wallDotLimit = 0.1f;
        if (_430 == 10) {
            wallDotLimit = 0.707f;
        }

        if (gravityDot < _3C) {
            mDrawStates_WORD |= 0x02000000;

            if (canPushGround && mMovementStates.jumping && mVerticalSpeed > 0.0f) {
                TVec3f pushDir = mActor->_2A0 - rInfo.mHitPos;
                bool needsPush = MR::diffAngleAbs(pushDir, normal) > 0.10471976f;

                if (!rInfo.isCollisionAtCorner() && !rInfo.isCollisionAtEdge() && !needsPush) {
                    canPushGround = false;
                }
                else {
                    if (rInfo.isCollisionAtEdge()) {
                        pushDir = *pPlane->getNormal(rInfo._88 - 1);
                    }
                    else {
                        pushDir = mPosition - rInfo.mHitPos;
                    }

                    TVec3f pushBase(pushDir);
                    MR::normalizeOrZero(&pushBase);

                    f32 pushScale = pushBase.dot(normal);
                    if (pushScale >= 0.707f) {
                        if (rInfo.isCollisionAtEdge()) {
                            MR::vecKillElement(mVelocity, normal, &mVelocity);
                            pushScale = 0.0f;
                        }
                    }
                    else if (pushScale <= -0.707f) {
                        pushScale = 1.0f;
                    }
                    else {
                        pushScale = (0.707f - __fabsf(pushScale)) / 0.707f;
                    }

                    MR::vecKillElement(pushDir, *getAirGravityVec(), &pushDir);
                    if (MR::normalizeOrZero(&pushDir)) {
                        TVec3f killed;
                        MR::vecKillElement(pushBase, *getAirGravityVec(), &killed);
                        pushDir = killed;
                        MR::normalizeOrZero(&pushDir);
                    }

                    if (mMovementStates._B) {
                        if (MR::diffAngleAbsHorizontal(mActor->_288, pushDir, *getAirGravityVec()) >= 150.0f) {
                            if (_45C->isValid()) {
                                const TVec3f& shadowNormal = triangleNormal(_45C);
                                if (normal.dot(shadowNormal) <= -0.8f) {
                                    f32 removed = MR::vecKillElement(mJumpVec, normal, &mJumpVec);
                                    TVec3f fix = normal;
                                    fix.scale(removed * -2.0f);
                                    mJumpVec += fix;
                                    mJumpVec += shadowNormal;
                                }
                                else if (mMovementStates._1A) {
                                    bool flipPush = MR::diffAngleAbs(pushDir, *getAirGravityVec()) >= 120.0f;
                                    if (!flipPush && MR::diffAngleAbsHorizontal(mJumpVec, pushDir, *getAirGravityVec()) < 70.0f) {
                                        flipPush = true;
                                    }
                                    if (flipPush) {
                                        pushDir = -pushDir;
                                    }
                                    mMovementStates_LOW_WORD &= ~0x00000400;
                                }
                            }
                            else if (mMovementStates._1A) {
                                bool flipPush = MR::diffAngleAbs(pushDir, *getAirGravityVec()) >= 120.0f;
                                if (!flipPush && MR::diffAngleAbsHorizontal(mJumpVec, pushDir, *getAirGravityVec()) < 70.0f) {
                                    flipPush = true;
                                }
                                if (flipPush) {
                                    pushDir = -pushDir;
                                }
                                mMovementStates_LOW_WORD &= ~0x00000400;
                            }
                        }
                    }

                    if (mMovementStates.jumping && !isRising()) {
                        TVec3f actorMove;
                        mActor->getLastMove(&actorMove);
                        if (MR::isNearZero(actorMove.dot(*getGravityVec()), 0.001f)
                            && (mMovementStates._8 || mMovementStates._19 || mMovementStates._1A)) {
                            cutGravityElementFromJumpVec(true);

                            TVec3f wallFix = getWallNorm();
                            wallFix.scale(3.0f);
                            TVec3f gravityFix = *getAirGravityVec();
                            gravityFix.scale(-5.0f);
                            mJumpVec += gravityFix + wallFix;

                            if (!isCeiling()) {
                                TVec3f trans = *getAirGravityVec();
                                trans.scale(-1.0f);
                                addTrans(trans, nullptr);
                            }

                            pushDir.zero();
                        }
                    }

                    TVec3f pushVec(pushDir);
                    pushVec.scale(rInfo._60 * pushScale);
                    push(pushVec);
                    canPushGround = false;

                    if (getPlayerMode() == 6) {
                        _25C = rInfo.mHitPos;
                        _268 = normal;
                        doTeresaReflection(pushBase, true);
                    }

                    _1C_WORD |= 0x00040000;
                }
            }

            if (mMovementStates.jumping && mMovementStates._B) {
                TVec3f actorMove;
                mActor->getLastMove(&actorMove);
                if (MR::isNearZero(actorMove.dot(*getGravityVec()), 0.001f)) {
                    mActor->sendMsgToSensor(pPlane->mSensor, 0xB4);
                    if (!mActor->sendMsgToSensor(pPlane->mSensor, 3)) {
                        TVec3f toHit = mPosition - rInfo.mHitPos;
                        TVec3f front(mFrontVec);
                        MR::normalizeOrZero(&front);

                        f32 blend = 0.0f;
                        if (!MR::isNearZero(front, 0.001f)) {
                            blend = front.dot(normal);
                        }

                        if (blend < 0.0f) {
                            blend = 0.0f;
                        }
                        else if (blend > 1.0f) {
                            blend = 1.0f;
                        }

                        MR::vecBlendSphere(mFrontVec, toHit, &toHit, blend);
                        MR::vecKillElement(toHit, *getGravityVec(), &toHit);
                        MR::normalizeOrZero(&toHit);
                        if (MR::isNearZero(toHit, 0.001f)) {
                            toHit = mFrontVec;
                        }

                        f32 pushAmount = rInfo._60;
                        if (pushAmount == 0.0f) {
                            pushAmount = 1.0f;
                        }

                        toHit.scale(pushAmount);
                        push(toHit);
                        canPushGround = false;
                    }
                }
            }
        }
        else if (gravityDot >= wallDotLimit) {
            if (canStoreWall) {
                TVec3f killed;
                const f32 removed = MR::vecKillElement(mJumpVec, normal, &killed);
                if ((removed < 0.0f || mPrevDrawStates._1E) && calcAngleD(normal) > 100.0f) {
                    mJumpVec = killed;
                    u32 vibLevel = 0;
                    startPadVib(vibLevel);
                    *_4C8 = *pPlane;
                    canStoreWall = false;
                }
            }

            _3A4 += normal;
        }
    }

    MR::normalizeOrZero(&_3A4);

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
    }

    if (pBinder->mRoofInfo.mParentTriangle.isValid()) {
        *_460 = pBinder->mRoofInfo.mParentTriangle;
        _498 = pBinder->mRoofInfo.mHitPos;
        _10._24 = 1;
    }
    else {
        _10._24 = 0;
    }

    return true;
}

bool Mario::isThroughWall(const Triangle* pTriangle) const {
    if (getPlayerMode() != 6) {
        return false;
    }

    if (_418 == 0) {
        return false;
    }

    const char* pWallCode = MR::getWallCodeString(pTriangle);
    if (pWallCode == nullptr) {
        return false;
    }

    return strcmp(pWallCode, "GhostThroughCode") == 0;
}

bool Mario::checkGround() {
    if (isStatusActive(0x13)) {
        return false;
    }

    if (mMovementStates._B) {
        if (_10_HIGH_WORD & 0x00000200) {
            if (!checkGroundOnSlope()) {
                return false;
            }

            _1C_WORD |= 0x00001800;
            return true;
        }
    }
    else if (isUseSimpleGroundCheck()) {
        if (!(_10_HIGH_WORD & 0x00000200)) {
            if (MR::isNearZero(getGravityVec()->y - 1.0f, 0.001f)) {
                mActor->setBlendMtxTimer(4);
            }

            _10_HIGH_WORD |= 0x00000200;
        }

        if (!checkGroundOnSlope()) {
            return false;
        }

        _1C_WORD |= 0x00001800;
        return true;
    }
    else if (_10_HIGH_WORD & 0x00000200) {
        if (MR::isNearZero(getGravityVec()->y - 1.0f, 0.001f)) {
            mActor->setBlendMtxTimer(4);
        }

        _10_HIGH_WORD &= ~0x00000200;
    }

    TVec3f groundBase;
    if (_45C->isValid()) {
        groundBase = triangleNormal(_45C);
    }
    else {
        groundBase = -*getGravityVec();
    }

    TVec3f probeStep;
    MR::vecKillElement(mFrontVec, groundBase, &probeStep);
    if (MR::isNearZero(probeStep, 0.001f)) {
        return false;
    }

    probeStep.setLength(50.0f);
    if (MR::isNearZero(groundBase, 0.001f)) {
        return false;
    }

    Triangle hitTriangles[4];
    TVec3f hitPositions[4];
    bool hitFlags[4] = { false, false, false, false };
    bool rejectedByLift[4] = { false, false, false, false };

    mMovementStates_LOW_WORD &= ~0x00000400;

    f32 verticalLimit = 100.0f;
    if (mMovementStates.jumping && isRising()) {
        verticalLimit = 300.0f;
    }
    if (getCurrentStatus() == 7) {
        verticalLimit = 100.0f;
    }
    if (getCurrentStatus() == 0xD) {
        verticalLimit = 5.0f;
    }

    TVec3f hitAverage;
    hitAverage.zero();

    u32 probeCount = 3;
    if (!_71C && mMovementStates._1 && !mMovementStates.jumping && !(_10_HIGH_WORD & 0x10000000)) {
        probeCount = 4;
    }
    if (isAnimationRun("Run", 0)) {
        probeCount = 4;
    }

    u32 hitCount = 0;
    bool shouldCommitGround = true;
    for (u32 i = 0; i < probeCount; i++) {
        TVec3f start = mPosition + probeStep;
        TVec3f gravityBack = *getGravityVec();
        gravityBack.scale(100.0f);
        start -= gravityBack;
        if (i == 3) {
            start = mPosition - gravityBack;
        }

        TVec3f ray = *getGravityVec();
        ray.scale(500.0f);

        hitFlags[i] = MR::getFirstPolyOnLineBFast(start, ray, &hitPositions[i], &hitTriangles[i]);
        if (i == 3) {
            break;
        }

        if (hitFlags[i]) {
            if (_414 != 0) {
                TVec3f toHit = mPosition - hitPositions[i];
                if (__fabsf(toHit.dot(*getGravityVec())) > 0.0f) {
                    hitFlags[i] = false;
                    rejectedByLift[i] = true;
                }
            }

            if (hitFlags[i]) {
                TVec3f toHit = mPosition - hitPositions[i];
                if (__fabsf(toHit.dot(*getGravityVec())) > verticalLimit) {
                    hitFlags[i] = false;
                    rejectedByLift[i] = true;
                }
            }

            if (hitFlags[i] && calcAngleD(triangleNormal(&hitTriangles[i])) >= 80.0f) {
                hitFlags[i] = false;
            }

            if (hitFlags[i]) {
                const f32 dot = triangleNormal(&hitTriangles[i]).dot(*getGravityVec());
                if (getCurrentStatus() != 7 && dot > _3C) {
                    hitFlags[i] = false;
                }
            }

            if (hitFlags[i] && isStatusActive(0x13) && MR::isThroughPolygon(&hitTriangles[i])) {
                hitFlags[i] = false;
            }

            if (hitFlags[i]) {
                hitAverage += hitPositions[i];
                hitCount++;

                if ((_10_HIGH_WORD & 0x10000000) && !shouldCommitGround) {
                    TVec3f velDir(_16C);
                    MR::normalizeOrZero(&velDir);
                    TVec3f oldToHit(mGroundPos - mPosition);
                    TVec3f newToHit(hitPositions[i] - mPosition);
                    if (newToHit.dot(velDir) > oldToHit.dot(velDir)) {
                        shouldCommitGround = true;
                    }
                }

                if (shouldCommitGround) {
                    setGroundNorm(triangleNormal(&hitTriangles[i]));
                    *mGroundPolygon = hitTriangles[i];
                    mGroundPos = hitPositions[i];
                    recordLastGround();
                    shouldCommitGround = false;
                }

                const char* wallCode = MR::getWallCodeString(&hitTriangles[i]);
                if (wallCode != nullptr && strcmp(wallCode, "Sand") == 0) {
                    mDrawStates_WORD |= 0x00000040;
                }
            }
        }

        Mtx rot;
        PSMTXRotAxisRad(rot, &groundBase, 2.0943952f);
        PSMTXMultVec(rot, &probeStep, &probeStep);
    }

    if (hitCount == probeCount) {
        _1C_WORD |= 0x00004000;
    }

    if (!isNoWalkFallOnDossun() && !isStatusActive(3) && !isStatusActive(2) && !isStatusActive(0x22) && !isStatusActive(5)) {
        if (!hitFlags[0] && !hitFlags[1] && !hitFlags[2]) {
            return false;
        }

        if (!hitFlags[0]) {
            bool moved = false;
            if (!mMovementStates._8) {
                TVec3f push = mFrontVec;
                push.scale(-5.0f);
                addTrans(push, "ground check");
                moved = true;
            }

            if (hitFlags[1] && hitFlags[2] && !moved) {
                return true;
            }

            if (!hitFlags[1]) {
                TVec3f push = mSideVec;
                push.scale(3.0f);
                addTrans(push, "ground check side");
                moved = true;
            }
            else if (!hitFlags[2]) {
                TVec3f push = mSideVec;
                push.scale(-1.0f);
                push.scale(3.0f);
                addTrans(push, "ground check back");
                moved = true;
            }

            if (moved) {
                mDrawStates_WORD |= 0x00200000;
                return true;
            }
        }
        else if (!hitFlags[1] && !hitFlags[2]) {
            stopWalk();
            if (_3CE < 24 && mJumpVec.dot(mFrontVec) >= 0.0f && _960 != 0x13 && !mMovementStates._8 && !(_10_HIGH_WORD & 0x00002000)) {
                TVec3f push = mFrontVec;
                push.scale(3.0f);
                addTrans(push, "ground check front");
                mDrawStates_WORD |= 0x00200000;
                return true;
            }

            if (!mMovementStates._19) {
                mDrawStates_WORD |= 0x00000080;
                TVec3f push = mFrontVec;
                push.scale(-1.0f);
                addTrans(push, "ground check stop");
                mDrawStates_WORD |= 0x00200000;
            }

            if (_3C6 > 8) {
                TVec3f push = mSideVec;
                push.scale(5.0f);
                addTrans(push, "ground check adjust");
            }

            return true;
        }
    }

    if (hitCount == 0) {
        mMovementStates_HIGH_WORD |= 0x00000800;
        return false;
    }

    _8EC = hitCount;

    if (mMovementStates.jumping && isRising()) {
        return false;
    }

    if (!isStatusActive(0x1B) && probeCount == 4 && rejectedByLift[3]) {
        TVec3f push = mFrontVec;
        if (mMovementStates._8) {
            push = -push;
        }
        push.scale(5.0f);
        addTrans(push, "ground check 4");
    }

    if (mActor->_EA4 || getCurrentStatus() == 5) {
        return hitCount != 0 || mVerticalSpeed < 5.0f || (mDrawStates_WORD >> 31);
    }

    if (hitCount != 0) {
        TVec3f horizontalGround = mGroundPos - mPosition;
        MR::vecKillElement(horizontalGround, *getGravityVec(), &horizontalGround);

        f32 maxSlide = 5.0f;
        if (mMovementStates._8 && getFrontWallNorm().dot(horizontalGround) < -0.1f) {
            maxSlide = 0.0f;
        }
        if (mMovementStates._1A && getSideWallNorm().dot(horizontalGround) < -0.1f) {
            maxSlide = 0.0f;
        }

        if (maxSlide != 0.0f && PSVECMag(&horizontalGround) > maxSlide) {
            horizontalGround.setLength(maxSlide);
            mShadowPos = mPosition + horizontalGround;
            if (hitAverage.dot(*getGravityVec()) < verticalLimit) {
                setTrans(mShadowPos, nullptr);
            }
        }
    }

    if (hitCount == 0) {
        TVec3f toGround = mGroundPos - mPosition;
        if (__fabsf(toGround.dot(*getGravityVec())) < verticalLimit) {
            TVec3f horizontal = mGroundPos - mPosition;
            f32 alongGravity = MR::vecKillElement(horizontal, *getGravityVec(), &horizontal);
            if (!MR::isNearZero(alongGravity, 1.0f) || !mMovementStates._1) {
                TVec3f fix = *getGravityVec();
                fix.scale(alongGravity);
                mVelocity += fix;
                mDrawStates_WORD |= 0x80000000;
            }

            return true;
        }
    }

    if (mVerticalSpeed < 5.0f) {
        return true;
    }

    if (mDrawStates_WORD & 0x02000000) {
        return true;
    }

    return hitCount != 0;
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
    AreaObj* pCameraArea = reinterpret_cast< AreaObj* >(getCameraCubeCode());
    _568 = pCameraArea;
    _570 = 0;

    if (pCameraArea != nullptr) {
        _564 = MR::getAreaObjArg(pCameraArea, 0);

        const s32 cameraArg = MR::getAreaObjArg(pCameraArea, 1);
        if (cameraArg == 1) {
            mDrawStates_WORD |= 0x20000000;
        }
        else if (cameraArg == 2) {
            mDrawStates_WORD |= 0x10000000;
        }
    }
    else {
        _564 = -1;
    }

    MR::tryToUpdatePlayerRestartIdInfo(mPosition);

    AreaObj* pPullBackArea = MR::getAreaObj("PullBackCube", mPosition);
    if (pPullBackArea == nullptr) {
        pPullBackArea = MR::getAreaObj("PullBackCylinder", mPosition);

        if (pPullBackArea != nullptr && MR::getAreaObjArg(pPullBackArea, 0) != 1) {
            MR::calcCylinderCenterPos(&_6F4, pPullBackArea);
            MR::calcCylinderUpVec(&_700, pPullBackArea);
        }
    }

    if (pPullBackArea != nullptr) {
        doRecovery();
    }

    _10._9 = 0;
    if (MR::getAreaObj("PlaneCollisionCube", mPosition) != nullptr) {
        _10._9 = 1;
    }

    _10._13 = 0;
    AreaObj* pTowerModeArea = MR::getAreaObj("TowerModeCylinder", mPosition);
    if (pTowerModeArea != nullptr) {
        _10._13 = 1;
        MR::calcCylinderCenterPos(&_6F4, pTowerModeArea);
        MR::calcCylinderUpVec(&_700, pTowerModeArea);
        _718 = MR::getCylinderRadius(pTowerModeArea);
    }

    if (MR::getAreaObj("ForbidTriangleJumpCube", mPosition) != nullptr) {
        mDrawStates_WORD |= 0x10000000;
    }

    if (getPlayerMode() == 6) {
        if (MR::getAreaObj("GlaringLightArea", mPosition) != nullptr) {
            mActor->setPlayerMode(0, true);
        }

        if (!MR::isInShadeFromTheSun(mPosition, 2000.0f)) {
            mActor->setPlayerMode(0, true);
        }
    }

    if (MR::getAreaObj("FallsCube", mActor->_2AC) != nullptr) {
        touchWater();
        playEffectRTZ("水しぶき", mHeadVec, mActor->_2AC);
    }

    if (MR::getAreaObj("HeavySteeringCube", mPosition) != nullptr) {
        _10._D = 1;
        _10._E = 1;
    }
    else {
        _10._D = 0;
        _10._E = 0;
    }

    AreaObj* pDashChargeArea = MR::getAreaObj("DashChargeCylinder", mPosition);
    if (pDashChargeArea != nullptr) {
        _434 = mActor->getConst().getTable()->mItemDashTimer;
    }

    AreaObj* pRasterArea = MR::getAreaObj("RasterScrollCube", mPosition);
    if (pRasterArea != nullptr) {
        const s32 arg0 = MR::getAreaObjArg(pRasterArea, 0);
        const s32 arg1 = MR::getAreaObjArg(pRasterArea, 1);
        const s32 arg2 = MR::getAreaObjArg(pRasterArea, 2);
        mActor->setRasterScroll(arg0, arg1, arg2);
    }

    if (MR::getAreaObj("ForbidJumpCube", mPosition) != nullptr) {
        _1C_WORD |= 0x1000000;
    }

    if (!isStatusActive(0x23)) {
        if (MR::getAreaObj("DarkMatterCube", mPosition) != nullptr) {
            mActor->forceKill(4);
        }

        mVelocity.zero();
    }
}
