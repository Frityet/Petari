#include "Game/AreaObj/CubeCamera.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Map/CollisionCode.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Map/SunshadeMapHolder.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioConst.hpp"
#include "Game/Player/MarioDamage.hpp"
#include "Game/Player/MarioHang.hpp"
#include "Game/Player/MarioMapCode.hpp"
#include "Game/Player/MarioRecovery.hpp"
#include "Game/Player/MarioSwim.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/AreaObjUtil.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "revolution/mtx.h"
#include <cstring>

bool Mario::isIgnoreTriangle(const Triangle* triangle) {
    return MR::isNearZero(triangle->getNormal(0)->dot(*getGravityVec()));
}

void Mario::checkBaseTransBall() {
    TVec3f center = mPosition + mVelocity + mHeadVec * 50.0f;
    u32 count = Collision::checkStrikeBallToMap(center, 50.0f, nullptr, nullptr);
    for (u32 i = 0; i < count; i++) {
        doSwimmingHitCheck(Collision::getStrikeInfoMap(i), 0);
    }
    center = mPosition + mVelocity - mHeadVec * 40.0f;
    count = Collision::checkStrikeBallToMap(center, 50.0f, nullptr, nullptr);
    for (u32 i = 0; i < count; i++) {
        doSwimmingHitCheck(Collision::getStrikeInfoMap(i), 1);
    }
    center = mPosition + mVelocity;
    count = Collision::checkStrikeBallToMap(center, 40.0f, nullptr, nullptr);
    for (u32 i = 0; i < count; i++) {
        doSwimmingHitCheck(Collision::getStrikeInfoMap(i), 2);
    }
}

void Mario::createAtField(bool force, f32 radius) {
    u32 hits;
    _578 = 0;
    f32 checkRadius = 50.0f;
    f32 offset = 50.0f;
    u32 count = 2;
    if (isSwimming()) {
        checkRadius = 40.0f;
        offset = 0.0f;
    }
    if (mMovementStates._A) {
        checkRadius = 40.0f;
        count = 1;
        offset = 40.0f;
    }
    if (force) {
        checkRadius = radius;
        if (radius < 40.0f) {
            checkRadius = 40.0f;
        }
        offset = 0.0f;
    }
    for (u32 i = 0; i < count; i++) {
        TVec3f center = mPosition - getAirGravityVec() * offset * (i + 1);
        if (force && mMovementStates._F && _544 > 1) {
            hits = Collision::checkStrikeBallToMap(center, checkRadius, nullptr, nullptr);
        } else {
            hits = Collision::checkStrikeBallToMapWithThickness(center, checkRadius, checkRadius, nullptr, nullptr);
        }
        for (u32 j = 0; j < hits; j++) {
            const HitInfo* hit = Collision::getStrikeInfoMap(j);
            const Triangle* triangle = &hit->mParentTriangle;
            if (force && mMovementStates._F && _544 > 1) {
                if (calcAngleD(*triangle->getNormal(0)) < 60.0f) {
                    continue;
                }
                if (calcAngleD(*triangle->getNormal(0)) > 120.0f) {
                    continue;
                }
                TVec3f horizontal;
                if (__fabsf(MR::vecKillElement(hit->mHitPos - center, getAirGravityVec(), &horizontal)) > 80.0f) {
                    continue;
                }
                if (hit->isCollisionAtEdge() && MR::diffAngleAbsHorizontal(mJumpVec, -*triangle->getNormal(0), getAirGravityVec()) < 0.7853982f) {
                    continue;
                }
            }
            if (!force && !isSwimming()) {
                if (triangle->mSensor && mActor->selectPushOff(triangle->mSensor)) {
                    continue;
                }
                if (isThroughWall(triangle)) {
                    continue;
                }
                const TVec3f* normal = triangle->getNormal(0);
                f32 angle = _95C->getCode(triangle) == CollisionFloorCode_NoSlip ? 60.0f : 45.0f;
                if (mMovementStates._37) {
                    if (!MR::isSameMtx(*triangle->getBaseMtx(), *triangle->getPrevBaseMtx()) && mGroundPolygon->mSensor != triangle->mSensor) {
                        angle = -1.0f;
                        push(*normal * hit->_60);
                    }
                }
                if (calcAngleD(*normal) < angle) {
                    continue;
                }
            }
            *_57C[_578] = *triangle;
            _578++;
            if (_578 == 32) {
                break;
            }
        }
        if (_578 == 32) {
            break;
        }
    }
}

void Mario::doSwimmingHitCheck(const HitInfo* hit, u32 type) {
    const Triangle* triangle = &hit->mParentTriangle;
    if (MR::isThroughPolygon(triangle)) {
        return;
    }
    if (isThroughWall(triangle)) {
        return;
    }
    if (damagePolygonCheck(triangle)) {
        return;
    }
    TVec3f velocity(mVelocity);
    const TVec3f* normal = triangle->getNormal(0);
    if (velocity.dot(-*normal) < 0.0f) {
        return;
    }
    mSwim->addVelocity(*normal, hit->_60);
    if (type == 0) {
        _97C->hitWall(*normal, triangle->mSensor);
    } else {
        _97C->hitPoly(type, *normal, triangle->mSensor);
    }
}

void Mario::doSpinPunchAroundPolygons() {
    if (!mActor->isPunching()) {
        return;
    }
    if (!mActor->isInPunchTimerRange()) {
        return;
    }
    TVec3f center = mPosition + mVelocity + mHeadVec * 80.0f;
    Triangle triangles[256];
    u32 count = MR::createAreaPolygonList(triangles, 256, center + mFrontVec * 120.0f + mSideVec * 120.0f + mHeadVec * 120.0f,
                                          center - mFrontVec * 120.0f - mSideVec * 120.0f - mHeadVec * 120.0f);
    for (u32 i = 0; i < count; i++) {
        sendPunch(triangles[i].mSensor, true);
    }
}

void Mario::checkMap() {
    calcShadowPos();
    if (isStatusActive(MarioStatus_Swim)) {
        mGroundPos = mShadowPos;
    }
    TVec3f gravity(*getGravityVec());
    if (mMovementStates._1 && isSlipPolygon(mGroundPolygon)) {
        gravity = _374;
    }
    if (isUseSimpleGroundCheck()) {
        if (!mMovementStates._14) {
            mVerticalSpeed = (mGroundPos - mPosition).dot(gravity);
        } else {
            mVerticalSpeed = (mShadowPos - mPosition).dot(gravity);
        }
        if (!_20._36) {
            _148.zero();
        }
    } else {
        mVerticalSpeed = (mShadowPos - mPosition).dot(gravity);
    }
    if (mVerticalSpeed < 0.0f) {
        mVerticalSpeed = 0.0f;
    }
    if (mMovementStates._23 && calcAngleD(_368) >= 30.0f) {
        TVec3f start = mPosition + _368 * 100.0f;
        mMovementStates._24 = MR::getFirstPolyOnLineToMap(&_498, _460, start, -_904 * 150.0f);
    } else {
        mMovementStates._24 = false;
    }
}

f32 Mario::calcDistToCeil(bool saveSensor) {
    Triangle triangle;
    TVec3f hitPos;
    f32 offset = 30.0f;
    if (saveSensor) {
        _730 = nullptr;
    }
    if (isStatusActive(MarioStatus_Hang)) {
        offset = 0.0f;
    }
    while (true) {
        TVec3f start = mPosition + getAirGravityVec() * offset;
        bool hit = MR::getFirstPolyOnLineToMap(&hitPos, &triangle, start, -getAirGravityVec() * (200.0f + offset));
        if (isThroughWall(&triangle)) {
            hit = false;
        }
        if (!hit) {
            break;
        }
        bool pressGround = false;
        if (mGroundPolygon->mSensor && MR::isSensorPressObj(mGroundPolygon->mSensor)) {
            pressGround = true;
        }
        if (!MR::isSensorPressObj(triangle.mSensor) && _95C->getCode(&triangle) != CollisionFloorCode_Press && !pressGround &&
            _960 != CollisionFloorCode_Press) {
            if ((hitPos - mPosition).dot(*getGravityVec()) >= 0.0f) {
                if (offset <= 1.0f) {
                    offset = -10.0f;
                    continue;
                }
                return 200.0f;
            }
        }
        _3B0 = *triangle.getNormal(0);
        if (saveSensor) {
            _730 = triangle.mSensor;
        }
        f32 distance = (hitPos - mPosition).length();
        if (!_4C8->isValid()) {
            *_4C8 = triangle;
        }
        return distance;
    }
    return 200.0f;
}

f32 Mario::calcDistToCeilOnPress() {
    if (!_480->isValid() || !_484->isValid()) {
        return 0.0f;
    }
    TVec3f horizontal;
    f32 distance1 = MR::vecKillElement(mPosition - *_480->calcAndGetPos(0), *_480->calcAndGetNormal(0), &horizontal);
    f32 distance2 = MR::vecKillElement(mPosition - *_484->calcAndGetPos(0), *_484->calcAndGetNormal(0), &horizontal);
    if (_480->getNormal(0)->dot(mAirGravityVec) < 0.0f) {
        f32 distance = MR::vecKillElement(*_480->calcAndGetPos(0) - mPosition, *_480->calcAndGetNormal(0), &horizontal);
        mPosition += *_480->getNormal(0) * distance;
    } else {
        f32 distance = MR::vecKillElement(*_484->calcAndGetPos(0) - mPosition, *_484->calcAndGetNormal(0), &horizontal);
        mPosition += *_484->getNormal(0) * distance;
    }
    mActor->mPosition.set(mPosition);
    if (distance1 > distance2) {
        return distance2;
    }
    return distance1;
}

f32 Mario::calcDistToCeilHead() {
    Triangle triangle;
    TVec3f hitPos;
    TVec3f start = mActor->_2AC - mFrontVec * 40.0f;
    if (MR::getFirstPolyOnLineToMap(&hitPos, &triangle, start, -getAirGravityVec() * 80.0f)) {
        *getTmpPolygon() = triangle;
        return (hitPos - mPosition).length();
    }
    return 80.0f;
}

void Mario::fixTransBetweenWall(const TVec3f& first, const TVec3f& second) {
    TVec3f center = (first + second) * 0.5f;
    MR::vecKillElement(center - mPosition, *getGravityVec(), &center);
    setTrans(mPosition + center, nullptr);
}

f32 Mario::calcDistWidth() {
    Triangle triangle;
    TVec3f hitPos;
    if (mMovementStates._8) {
        if (MR::getFirstPolyOnLineToMap(&hitPos, &triangle, _4E8, *mFrontWallTriangle->getNormal(0) * 100.0f)) {
            fixTransBetweenWall(hitPos, _4E8);
            return (hitPos - _4E8).length();
        }
    }
    if (mMovementStates._19) {
        if (MR::getFirstPolyOnLineToMap(&hitPos, &triangle, _4F4, *mBackWallTriangle->getNormal(0) * 100.0f)) {
            fixTransBetweenWall(hitPos, _4F4);
            return (hitPos - _4F4).length();
        }
    }
    if (mMovementStates._1A) {
        if (MR::getFirstPolyOnLineToMap(&hitPos, &triangle, _500, *mSideWallTriangle->getNormal(0) * 100.0f)) {
            fixTransBetweenWall(hitPos, _500);
            return (hitPos - _500).length();
        }
    }
    TVec3f center = mPosition + mHeadVec * 80.0f;
    TVec3f firstPos;
    TVec3f secondPos;
    u32 count = Collision::checkStrikeBallToMap(center, 30.0f, nullptr, nullptr);
    if (count < 2) {
        return 100.0f;
    }
    f32 width = 100.0f;
    for (u32 i = 0; i < count; i++) {
        const HitInfo* first = Collision::getStrikeInfoMap(i);
        TVec3f firstNormal(*first->mParentTriangle.getNormal(0));
        for (u32 j = i + 1; j < count; j++) {
            const HitInfo* second = Collision::getStrikeInfoMap(j);
            TVec3f secondNormal(*second->mParentTriangle.getNormal(0));
            if (firstNormal.dot(secondNormal) >= -0.707f) {
                continue;
            }
            if ((first->mHitPos - center).dot(second->mHitPos - center) > 0.0f) {
                continue;
            }
            f32 distance = (first->mHitPos - second->mHitPos).length();
            if (distance < width) {
                width = distance;
                firstPos = first->mHitPos;
                secondPos = second->mHitPos;
            }
        }
    }
    if (width < 100.0f) {
        fixTransBetweenWall(firstPos, secondPos);
    }
    return width;
}

void Mario::updateCameraPolygon() {
    HitSensor* sensor;
    const Triangle* triangle;
    if (mMovementStates._1) {
        triangle = mGroundPolygon;
    } else {
        triangle = _45C;
    }
    sensor = triangle->mSensor;
    if (triangle->isValid()) {
        if (!sensor) {
            setCameraPolygon(triangle);
            return;
        }
        setCameraPolygon(triangle);
        return;
    }
    TVec3f start = mPosition - *getGravityVec() * 100.0f;
    const Triangle* cameraTriangle = MR::getCameraPolyFast(start, *getGravityVec() * 5000.0f, nullptr);
    if (cameraTriangle) {
        setCameraPolygon(cameraTriangle);
    }
}

void Mario::setCameraPolygon(const Triangle* triangle) {
    *_468 = *triangle;
    if (!triangle->mSensor) {
        *_46C = *triangle;
        return;
    }
    *_46C = *triangle;
}

void Mario::checkAllWall(const TVec3f& position, f32 radius) {
    TVec3f center(position);
    TVec3f front(mFrontVec);
    _4D8->mIdx = -1;
    _4DC->mIdx = -1;
    if (mMovementStates._F) {
        TVec3f direction;
        mActor->getLastMove(&direction);
        MR::vecKillElement(direction, *getGravityVec(), &direction);
        if (!MR::isNearZero(direction)) {
            MR::normalize(&direction);
            front = direction;
        }
    }
    const HitInfo* walls[3];
    for (s32 i = 0; i < 3; i++) {
        walls[i] = nullptr;
    }
    s32 count;
    bool forwardCheck = false;
    if (getPlayerMode() == PlayerMode_Teresa) {
        radius = 110.0f;
        count = Collision::checkStrikeBallToMapWithThickness(center, radius, radius, nullptr, nullptr);
        if (!count) {
            forwardCheck = true;
            center += mFrontVec * 120.0f;
            count = Collision::checkStrikeBallToMapWithThickness(center, 25.0f, 25.0f, nullptr, nullptr);
        }
    } else {
        count = Collision::checkStrikeBallToMap(center, radius, nullptr, nullptr);
    }
    bool through = false;
    for (u32 i = 0; i < count; i++) {
        const HitInfo* hit = Collision::getStrikeInfoMap(i);
        const Triangle* triangle = &hit->mParentTriangle;
        TVec3f normal(*triangle->getNormal(0));
        TVec3f direction(hit->mHitPos);
        direction -= center;
        MR::normalizeOrZero(&direction);
        if (mMovementStates._37 && __fabsf(normal.dot(_6A0)) > 0.707f) {
            continue;
        }
        f32 angle = 180.0f * (marioAcos(-getGravityVec()->dot(normal)) / 3.14159f);
        TVec3f fromBase(hit->mHitPos);
        fromBase -= mPosition;
        MR::normalizeOrZero(&fromBase);
        if (angle >= mActor->getConst().getTable()->mFlatAngle && direction.dot(mFrontVec) > 0.0f && fromBase.dot(mFrontVec) > 0.0f) {
            if (calcAngleD(normal) <= mActor->getConst().getTable()->mSlipAngle && MR::diffAngleAbs(_368, normal) > 30.0f) {
                *_4DC = *triangle;
                _518 = hit->mHitPos;
            }
        }
        if (__fabsf(normal.dot(mHeadVec)) > 0.5f) {
            if (direction.dot(*getGravityVec()) > 0.0f && direction.dot(mFrontVec) > 0.707f) {
                *_4D8 = *triangle;
                _50C = hit->mHitPos;
            }
            continue;
        }
        f32 facing = normal.dot(front);
        s32 side;
        if (facing >= mActor->getConst().getTable()->mWallBackAngleRange) {
            side = 2;
        } else if (facing <= -mActor->getConst().getTable()->mWallFrontAngleRange) {
            side = 0;
        } else {
            side = 1;
        }
        if (walls[side] && walls[side]->_60 > hit->_60) {
            continue;
        }
        if (isThroughWall(triangle)) {
            through = true;
            mActor->_F44 = false;
        } else {
            walls[side] = hit;
        }
    }
    if (!through && getPlayerMode() == PlayerMode_Teresa && !isStatusActive(MarioStatus_Recovery)) {
        mActor->_F44 = true;
    }
    if (count >= 1U) {
        f32 minAngle = 180.0f;
        f32 maxAngle = 0.0f;
        u32 minIndex = 0;
        u32 maxIndex = 0;
        for (u32 i = 0; i < count; i++) {
            const HitInfo* hit = Collision::getStrikeInfoMap(i);
            const Triangle* triangle = &hit->mParentTriangle;
            if (mMovementStates._37 && __fabsf(triangle->getNormal(0)->dot(_6A0)) > 0.707f) {
                continue;
            }
            f32 angle = calcAngleD(*triangle->getNormal(0));
            if (angle < minAngle) {
                minAngle = angle;
                minIndex = i;
            }
            if (angle > maxAngle) {
                maxAngle = angle;
                maxIndex = i;
            }
        }
        f32 groundAngle;
        if (mMovementStates._1) {
            groundAngle = calcAngleD(_368);
            if (groundAngle < minAngle) {
                minAngle = groundAngle;
            }
            if (groundAngle > maxAngle) {
                maxAngle = groundAngle;
            }
        }
        if (maxAngle - minAngle < 60.0f && maxAngle - minAngle > 5.0f && maxAngle < 80.0f) {
            mDrawStates._F = true;
            if (maxAngle > groundAngle) {
                _380 = *Collision::getStrikeInfoMap(maxIndex)->mParentTriangle.getNormal(0);
            } else if (minAngle < groundAngle) {
                _380 = *Collision::getStrikeInfoMap(minIndex)->mParentTriangle.getNormal(0);
            }
        }
        if (mMovementStates._1 && maxAngle > 60.0f && maxAngle != groundAngle && maxAngle < 80.0f) {
            TVec3f hitPos(Collision::getStrikeInfoMap(maxIndex)->mHitPos);
            TVec3f direction(center);
            direction -= hitPos;
            if (!MR::normalizeOrZero(&direction)) {
                mDrawStates._17 = true;
                TVec3f side;
                PSVECCrossProduct(Collision::getStrikeInfoMap(maxIndex)->mParentTriangle.getNormal(0), &getAirGravityVec(), &side);
                MR::normalize(&side);
                TVec3f slope;
                PSVECCrossProduct(&_368, &side, &slope);
                MR::normalize(&slope);
                _2C4 = slope;
            }
        }
    }
    HitInfo pointHit;
    if (MR::checkStrikePointToMap(center, &pointHit)) {
        TVec3f normal(*pointHit.mParentTriangle.getNormal(0));
        if (__fabsf(normal.dot(mHeadVec)) <= 0.5f) {
            f32 facing = normal.dot(front);
            s32 side;
            if (facing >= 0.707f) {
                side = 2;
            } else if (facing <= -0.707f) {
                side = 0;
            } else {
                side = 1;
            }
            walls[side] = &pointHit;
        }
    }
    mMovementStates._8 = false;
    mMovementStates._19 = false;
    mMovementStates._1A = false;
    mMovementStates._32 = false;
    mMovementStates._33 = false;
    if (walls[0]) {
        TVec3f direction(walls[0]->mHitPos);
        direction -= center;
        MR::normalizeOrZero(&direction);
        f32 dot = direction.dot(*walls[0]->mParentTriangle.getNormal(0));
        *mFrontWallTriangle = walls[0]->mParentTriangle;
        _4E8 = walls[0]->mHitPos;
        f32 threshold = -0.999f;
        if (isStatusActive(MarioStatus_Wall)) {
            threshold = -0.9f;
        }
        if (dot < threshold) {
            mMovementStates._8 = true;
        } else {
            mMovementStates._32 = true;
        }
        if (mMovementStates._1 && getMovementStates()._15 && mMovementStates._39) {
            if ((_4A4 - _4E8).dot(*getGravityVec()) >= 0.0f) {
                mMovementStates._8 = false;
            }
        }
        if (!_60D && !mMovementStates._8) {
            if (calcAngleD(*mFrontWallTriangle->getNormal(0)) < mActor->getConst().getTable()->mSlipAngle) {
                bool noSlip = !isSlipFloorCode(_95C->getCode(mFrontWallTriangle));
                if (noSlip && walls[0]->mParentTriangle.getNormal(0)->dot(mFrontVec) < 0.0f && direction.dot(mFrontVec) > 0.0f &&
                    direction.dot(*walls[0]->mParentTriangle.getNormal(0)) < 0.0f) {
                    mDrawStates._F = true;
                    _380 = *mFrontWallTriangle->getNormal(0);
                }
            }
        }
    }
    if (!forwardCheck) {
        if (walls[1]) {
            bool valid = true;
            if (isPlayerModeTeresa()) {
                if (walls[1]->mParentTriangle.getNormal(0)->dot(walls[1]->mHitPos - center) >= 0.0f) {
                    valid = false;
                }
            }
            if (valid) {
                *mSideWallTriangle = walls[1]->mParentTriangle;
                _500 = walls[1]->mHitPos;
                mMovementStates._1A = true;
                if (walls[1]->isCollisionAtCorner() || walls[1]->isCollisionAtEdge()) {
                    mMovementStates._33 = true;
                }
            }
        }
        if (walls[2]) {
            bool valid = true;
            if (mMovementStates._8) {
                TVec3f backDistance = _4F4 - center;
                TVec3f frontDistance = _4E8 - center;
                if (backDistance.dot(frontDistance) >= 0.0f && backDistance.length() >= frontDistance.length()) {
                    valid = false;
                }
            }
            if (isPlayerModeTeresa()) {
                if (walls[2]->mParentTriangle.getNormal(0)->dot(walls[2]->mHitPos - center) >= 0.0f) {
                    valid = false;
                }
            }
            if (valid) {
                *mBackWallTriangle = walls[2]->mParentTriangle;
                _4F4 = walls[2]->mHitPos;
                mMovementStates._19 = true;
            }
        }
    }
    updateWallFloorCode();
    _38C = -_380;
    if (_400) {
        _400--;
        mMovementStates._8 = true;
    }
}

void Mario::calcFrontFloor() {
    Triangle wall;
    Triangle floor;
    TVec3f start;
    TVec3f wallPos;
    TVec3f floorPos;
    TVec3f direction;
    mMovementStates._15 = false;
    mMovementStates._39 = false;
    _47C->mIdx = -1;
    _4E0 = 0.0f;
    bool hit;
    if (mMovementStates._8) {
        wallPos = _4E8;
        start = mActor->_2A0;
        hit = true;
    } else {
        direction = mFrontVec * 100.0f;
        start = mPosition - *getGravityVec() * 8.0f;
        if (mMovementStates._F && !MR::isNearZero(_328)) {
            MR::vecKillElement(_328, *getGravityVec(), &direction);
            direction.setLength(200.0f);
        }
        hit = MR::getFirstPolyOnLineToMap(&wallPos, &wall, start, direction);
    }
    if (hit) {
        _4E4 = (wallPos - start).dot(mFrontVec);
        TVec3f normal(*wall.getNormal(0));
        f32 dot = normal.dot(getAirGravityVec());
        if (__fabsf(dot) < 0.1f) {
            f32 angle = marioAcos(-dot);
            start = wallPos - getAirGravityVec() * 200.0f;
            start += mFrontVec * 20.0f;
            direction = getAirGravityVec() * 210.0f;
            bool floorHit = MR::getFirstPolyOnLineBFast(start, direction, &floorPos, &floor);
            if (floorHit && floor.getNormal(0)->dot(getAirGravityVec()) > -0.9f) {
                floorHit = false;
            }
            if (floorHit) {
                _4E0 = (wallPos - floorPos).dot(getAirGravityVec());
                if (_4E0 > 0.0f) {
                    mMovementStates._15 = true;
                }
                f32 offset = 0.0f;
                if (angle < 1.5707964f && angle != 0.0f) {
                    offset = _4E0 / (JMASinRadian(angle) / JMACosRadian(angle));
                }
                TVec3f originalPos(floorPos);
                floorPos -= mFrontVec * (15.0f + offset);
                if (!MR::isExistMapCollision(floorPos - getAirGravityVec() * 5.0f, *getGravityVec() * 10.0f)) {
                    floorPos = originalPos;
                } else {
                    _4A4 = floorPos;
                    mMovementStates._39 = true;
                    *_47C = floor;
                }
            } else {
                _4E0 = 400.0f;
            }
        }
    } else {
        _4E0 = 0.0f;
    }
    if (mMovementStates._1 && calcAngleD(_368) >= 25.0f) {
        mMovementStates._39 = false;
    }
}

const TVec3f& Mario::getWallNorm() const {
    if (mMovementStates._8) {
        return *mFrontWallTriangle->getNormal(0);
    }
    if (mMovementStates._19) {
        return *mBackWallTriangle->getNormal(0);
    }
    if (mMovementStates._1A) {
        return *mSideWallTriangle->getNormal(0);
    }
    if (mFrontWallTriangle->isValid()) {
        return *mFrontWallTriangle->getNormal(0);
    }
    return mFrontVec;
}

const TVec3f& Mario::getSideWallNorm() const {
    if (mMovementStates._1A) {
        return *mSideWallTriangle->getNormal(0);
    }
    return TVec3f(gZeroVec);
}

const TVec3f& Mario::getFrontWallNorm() const {
    if (mMovementStates._8) {
        return *mFrontWallTriangle->getNormal(0);
    }
    return TVec3f(gZeroVec);
}

const TVec3f& Mario::getBackWallNorm() const {
    if (mMovementStates._19) {
        return *mBackWallTriangle->getNormal(0);
    }
    return TVec3f(gZeroVec);
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
    s32 code = _95C->getCode(mGroundPolygon);
    if (code != -1) {
        _960 = code;
    }
    code = _95C->getCode(_45C);
    if (code != -1) {
        _962 = code;
    }
    if (mActor->mBeeWallWalk) {
        _41C = 15;
        return;
    }
    f32 angle = calcPolygonAngleD(mGroundPolygon);
    if (angle > mActor->getConst().getTable()->mSlipAngle) {
        if (angle > mActor->getConst().getTable()->mForceWallAngle) {
            _41C = 0;
        }
        if (_41C) {
            _41C--;
        }
        if (!_41C) {
            _960 = 0x80;
        }
    } else {
        bool noSlip = !isSlipFloorCode(_960);
        if (noSlip) {
            _41C = 15;
        }
    }
}

void Mario::updateWallFloorCode() {
    if (mMovementStates._8 || mMovementStates._32) {
        s32 code = _95C->getCode(mFrontWallTriangle);
        if (code != -1) {
            _964[0] = code;
        }
    }
    if (mMovementStates._19) {
        s32 code = _95C->getCode(mBackWallTriangle);
        if (code != -1) {
            _964[1] = code;
        }
    }
    if (mMovementStates._1A) {
        s32 code = _95C->getCode(mSideWallTriangle);
        if (code != -1) {
            _964[2] = code;
        }
    }
}

void Mario::saveLastSafetyTrans() {
    if (isStatusActive(MarioStatus_FireDamage)) {
        return;
    }
    if (isStatusActive(MarioStatus_Recovery)) {
        return;
    }
    if (!_1C._11) {
        return;
    }
    if (_96A) {
        _96A--;
        return;
    }
    switch (_960) {
    case CollisionFloorCode_Normal:
    case CollisionFloorCode_NoSlip:
    case CollisionFloorCode_Sand:
    case CollisionFloorCode_Glass:
    case CollisionFloorCode_WaterBottomH:
    case CollisionFloorCode_WaterBottomM:
    case CollisionFloorCode_WaterBottomL:
    case CollisionFloorCode_Wet:
    case CollisionFloorCode_NoStampSand:
        break;
    default:
        return;
    }
    if (mGroundPolygon->mSensor->isType(0x48)) {
        return;
    }
    if (mMovementStates._8) {
        return;
    }
    if (mMovementStates._32) {
        return;
    }
    if (mMovementStates._19) {
        return;
    }
    if (mMovementStates._1A) {
        return;
    }
    if (mDrawStates._C) {
        return;
    }
    if (_1C._F) {
        return;
    }
    if (!(mVerticalSpeed < 10.0f)) {
        return;
    }
    if (!(calcAngleD(*mGroundPolygon->getNormal(0)) < 15.0f)) {
        return;
    }
    if (!MR::isSameMtx(*mGroundPolygon->getBaseMtx(), *mGroundPolygon->getPrevBaseMtx())) {
        return;
    }
    if (_7E0->isValid() && MR::isSameMtx(*_7E0->getBaseMtx(), _7E4) && MR::isSameMtx(*_7E0->getBaseMtx(), *_7E0->getPrevBaseMtx())) {
        _814 = _7D4;
        *_820 = *_7E0;
        PSMTXCopy(_7E4, _824);
    }
    *_7E0 = *mGroundPolygon;
    PSMTXCopy(*mGroundPolygon->getBaseMtx(), _7E4);
    _7D4 = (mPosition * 5.0f + *mGroundPolygon->getPos(0) + *mGroundPolygon->getPos(1) + *mGroundPolygon->getPos(2)) * 0.125f;
    if ((_7D4 - mPosition).length() > 50.0f) {
        TVec3f direction = _7D4 - mPosition;
        direction.setLength(50.0f);
        _7D4 = mPosition + direction;
    }
    _1C._E = true;
}

void Mario::setNotSafetyTimer() {
    _96A = 2;
    if (_1C._E) {
        _7E0->mIdx = -1;
    }
}

const TVec3f* Mario::getLastSafetyTrans(TVec3f* normal) const {
    if (normal) {
        normal->set(-getAirGravityVec());
    }
    if (_7E0->isValid() && MR::isSameMtx(*_7E0->getBaseMtx(), const_cast< TMtx34f& >(_7E4))) {
        if (normal) {
            normal->set(*_7E0->calcAndGetNormal(0));
        }
        return &_7D4;
    }
    if (_820->isValid() && normal) {
        normal->set(*_820->calcAndGetNormal(0));
    }
    return &_814;
}

bool Mario::checkCurrentFloorCodeSevere(u32 code) const {
    if (_960 != code) {
        return false;
    }
    TVec3f horizontal;
    if (MR::vecKillElement(mShadowPos - mGroundPos, getAirGravityVec(), &horizontal) > 30.0f) {
        return code == _95C->getCode(mGroundPolygon);
    }
    u32 shadowCode = _95C->getCode(_45C);
    if (shadowCode != code) {
        return false;
    }
    return shadowCode == _95C->getCode(mGroundPolygon);
}

bool Mario::isCurrentFloorSink() const {
    return checkCurrentFloorCodeSevere(CollisionFloorCode_Sink) || checkCurrentFloorCodeSevere(CollisionFloorCode_SinkDeathMud) ||
           checkCurrentFloorCodeSevere(CollisionFloorCode_SinkPoison) || checkCurrentFloorCodeSevere(CollisionFloorCode_SinkDeath);
}

bool Mario::isCurrentFloorSand() const {
    if (getPlayer()->mDrawStates.mIsUnderwater) {
        return false;
    }
    if (getPlayer()->mDrawStates._13) {
        return false;
    }
    return checkCurrentFloorCodeSevere(CollisionFloorCode_Sand) || checkCurrentFloorCodeSevere(CollisionFloorCode_NoStampSand);
}

bool Mario::isCurrentShadowFloorDangerAction() const {
    s32 code = _95C->getCode(_45C);
    if (code == _95C->getCode(mGroundPolygon)) {
        return false;
    }
    if (isPlayerModeTeresa()) {
        return false;
    }
    switch (code) {
    case CollisionFloorCode_DamageFire:
        if (isPlayerModeIce()) {
            return false;
        }
    case CollisionFloorCode_DamageNormal:
    case CollisionFloorCode_JumpLow:
    case CollisionFloorCode_JumpMiddle:
    case CollisionFloorCode_JumpHigh:
    case CollisionFloorCode_JumpNormal:
    case CollisionFloorCode_DamageElectric:
    case CollisionFloorCode_Sink:
    case CollisionFloorCode_SinkPoison:
    case CollisionFloorCode_Needle:
    case CollisionFloorCode_SinkDeath:
    case CollisionFloorCode_RailMove:
    case CollisionFloorCode_AreaMove:
    case CollisionFloorCode_SinkDeathMud:
    case CollisionFloorCode_JumpParasol:
        return true;
    default:
        return false;
    }
}

void Mario::checkBaseTransPoint() {
    bool inside = MR::checkStrikePointToMap(mPosition - *getGravityVec() * 30.0f, nullptr);
    u32 count = Collision::checkStrikeBallToMap(mPosition, 1.0f, nullptr, nullptr);
    for (u32 i = 0; i < count; i++) {
        const HitInfo* hit = Collision::getStrikeInfoMap(i);
        const Triangle* triangle = &hit->mParentTriangle;
        if (MR::isThroughPolygon(triangle) || isThroughWall(triangle)) {
            continue;
        }
        if (hit->isCollisionAtEdge() || hit->isCollisionAtCorner()) {
            mDrawStates._9 = true;
            continue;
        }
        const TVec3f* normal = triangle->getNormal(0);
        if (MR::isNearZero(normal->dot(_368), 0.3f) && !inside) {
            continue;
        }
        TVec3f direction(_16C);
        MR::normalizeOrZero(&direction);
        if (_16C.dot(-*normal) < 0.707f) {
            continue;
        }
        if (__fabsf(_16C.dot(*normal)) < hit->_60 || MR::isNearZero(hit->_60, 1.01f)) {
            continue;
        }
        if ((hit->mHitPos - mPosition).dot(mHeadVec) < 0.0f) {
            continue;
        }
        addTrans(*normal * (hit->_60 - 1.0f), "めりこみ");
        if (!mMovementStates._1 && mMovementStates.jumping && !isRising() && normal->dot(*getGravityVec()) < -0.99f) {
            addVelocity(mFrontVec, 5.0f);
        }
    }
}

void Mario::checkHeadPoint() {
    f32 radius = 40.0f;
    f32 height = 110.0f;
    bool damaging = false;
    if (mMovementStates._A) {
        height = 50.0f;
    }
    TVec3f center = mPosition - *getGravityVec() * height;
    if (isStatusActive(MarioStatus_Damage)) {
        mActor->calcHeadPos();
        center = mActor->_2AC;
        if (!mDamage->_18) {
            radius = 50.0f;
        }
        damaging = true;
    }
    if (mMovementStates._3E) {
        mActor->calcHeadPos();
        center = mActor->_2AC + _16C * 2.0f;
    }
    if (getPlayerMode() == PlayerMode_Bee && !getPlayer()->mMovementStates._23 && !getPlayer()->mMovementStates._A &&
        !isStatusActive(MarioStatus_Wait) && !isStatusActive(MarioStatus_SideStep) && !isStatusActive(MarioStatus_Bury) && !mActor->mBeeWallWalk) {
        if (isStatusActive(MarioStatus_Stick)) {
            return;
        }
        center = mActor->_2AC;
    }
    if (isSwimming()) {
        center = mPosition + mHeadVec * 60.0f;
    }
    u32 count = Collision::checkStrikeBallToMap(center, radius, nullptr, nullptr);
    Triangle triangles[32];
    u32 triangleCount = 0;
    TVec3f reaction;
    reaction.zero();
    for (u32 i = 0; i < count; i++) {
        const HitInfo* hit = Collision::getStrikeInfoMap(i);
        const Triangle* triangle = &hit->mParentTriangle;
        if (MR::isThroughPolygon(triangle) || isThroughWall(triangle)) {
            continue;
        }
        const TVec3f* normal = triangle->getNormal(0);
        if (!isSwimming() && !mMovementStates._3E) {
            if (damaging) {
                if (normal->dot(*getGravityVec()) < -0.707f) {
                    continue;
                }
            } else if (mMovementStates.jumping) {
                if (normal->dot(*getGravityVec()) < 0.0f || normal->dot(*getGravityVec()) < -0.707f) {
                    continue;
                }
            }
        }
        TVec3f horizontal;
        if (MR::vecKillElement(reaction, *normal, &horizontal) < hit->_60) {
            reaction = horizontal + *normal * hit->_60;
            if (isHeadPushEnableArea()) {
                addVelocity(*normal, hit->_60);
            }
            if (isSwimming()) {
                mSwim->hitHead(hit);
                addVelocity(*triangle->getNormal(0), radius - (hit->mHitPos - center).length());
            }
            const char* wallCode = MR::getWallCodeString(triangle);
            if (wallCode && !strcmp(wallCode, "Fur")) {
                mDrawStates._15 = true;
            }
            if (damaging) {
                mDamage->stopHead(*triangle->getNormal(0));
            }
            triangles[triangleCount] = *triangle;
            triangleCount++;
        }
    }
    addVelocity(reaction);
    if (!MR::isNearZero(reaction) && mMovementStates._3E) {
        TVec3f normal(reaction);
        MR::normalizeOrZero(&normal);
        if (calcAngleD(normal) > 45.0f) {
            blown(reaction * 0.2f);
            mMovementStates._2B = true;
            _402 = 0;
            _428 = 60;
            mMovementStates._3E = 0;
        } else if (_1FC.dot(reaction) < 0.0f) {
            mMovementStates._1 = true;
            mJumpVec.zero();
            changeAnimation("空中一回転", static_cast< const char* >(nullptr));
        }
    }
    bool swimSpin = false;
    if (isSwimming()) {
        if (mSwim->check7Aand7C()) {
            swimSpin = true;
        }
    }
    if ((mMovementStates.jumping && isRising()) || swimSpin) {
        for (u32 i = 0; i < triangleCount; i++) {
            if (triangles[i].getNormal(0)->dot(*getGravityVec()) > 0.707f) {
                mActor->sendMsgUpperPunch(triangles[i].mSensor);
            }
        }
    }
}

void Mario::calcShadowPos() {
    f32 offset = 100.0f;
    TVec3f start = mPosition - *getGravityVec() * offset;
    if ((getCurrentStatus() == MarioStatus_Hang && mHang->_12 < 2) || mActor->_EA4) {
        mActor->getRealPos("Spine1", &start);
    }
    f32 previousDistance = (mShadowPos - mPosition).length();
    TVec3f point(start);
    TVec3f direction(*getGravityVec());
    direction.scale(400.0f);
    u32 count = 6;
    if (isStatusActive(MarioStatus_Recovery)) {
        count = 30;
    }
    for (u32 i = 0; i < count; i++) {
        mMovementStates._2 = MR::getFirstPolyOnLineToMap(&mShadowPos, _45C, point, direction, nullptr, _458);
        if (mMovementStates._2) {
            break;
        }
        point += direction;
    }
    f32 distance = 10.0f + (mShadowPos - mPosition).length();
    bool changed = false;
    if (mMovementStates._2 && __fabsf(previousDistance - distance) > 100.0f) {
        changed = true;
    }
    if (!mMovementStates._2 || changed) {
        direction.setLength(100.0f + (50.0f + previousDistance));
        point = start + (mSideVec + mFrontVec) * 0.1f;
        mMovementStates._2 = MR::getFirstPolyOnLineToMap(&mShadowPos, _45C, point, direction, nullptr, _458);
        if (changed) {
            mMovementStates._2 = true;
        }
    }
    if (!mMovementStates._2) {
        mShadowPos = mPosition + *getGravityVec() * 2500.0f;
    }
}

bool Mario::updateBinderInfo() {
    s32 count;
    bool firstCeiling = true;
    bool firstGround = true;
    _3A4.zero();
    _4C8->mIdx = -1;
    Binder* binder = mActor->mBinder;
    if (!binder) {
        return false;
    }
    count = binder->mPlaneNum;
    if (!count) {
        return false;
    }
    for (u32 i = 0; i < count; i++) {
        const HitInfo* hit = binder->getPlane(i);
        const Triangle* triangle = &hit->mParentTriangle;
        TVec3f normal(*triangle->getNormal(0));
        if (MR::isThroughPolygon(triangle) || isThroughWall(triangle)) {
            continue;
        }
        f32 dot = normal.dot(*getGravityVec());
        f32 ceilingThreshold = 0.1f;
        if (_430 == 10) {
            ceilingThreshold = 0.707f;
        }
        if (dot < _3C) {
            mDrawStates._6 = true;
            if (firstGround && mMovementStates.jumping && mVerticalSpeed > 30.0f) {
                bool oblique = false;
                TVec3f fromHit = mActor->_2A0 - hit->mHitPos;
                if (MR::diffAngleAbs(fromHit, *triangle->getNormal(0)) > 0.10471976f) {
                    oblique = true;
                }
                if (hit->isCollisionAtCorner() || hit->isCollisionAtEdge() || oblique) {
                    TVec3f pushDirection;
                    if (hit->isCollisionAtEdge()) {
                        pushDirection = *triangle->getNormal(hit->_88 - 1);
                    } else if (!hit->isCollisionAtCorner()) {
                        pushDirection = mPosition - hit->mHitPos;
                    } else {
                        pushDirection = mPosition - hit->mHitPos;
                    }
                    TVec3f direction(fromHit);
                    MR::normalizeOrZero(&direction);
                    f32 facing = direction.dot(*triangle->getNormal(0));
                    f32 scale = facing;
                    if (facing >= 0.707f) {
                        if (hit->isCollisionAtEdge()) {
                            MR::vecKillElement(mVelocity, *triangle->getNormal(0), &mVelocity);
                            scale = 1.0f;
                        }
                    } else if (facing <= -0.707f) {
                        scale = 0.0f;
                    } else {
                        scale = (0.707f - __fabsf(facing)) / 0.707f;
                    }
                    MR::vecKillElement(pushDirection, getAirGravityVec(), &pushDirection);
                    if (MR::normalizeOrZero(&pushDirection)) {
                        MR::vecKillElement(fromHit, getAirGravityVec(), &pushDirection);
                        MR::normalizeOrZero(&pushDirection);
                    }
                    if (mMovementStates._14) {
                        const TVec3f& lastDirection = mActor->_288;
                        if (MR::diffAngleAbsHorizontal(lastDirection, pushDirection, getAirGravityVec()) >= 2.3561945f) {
                            if (_45C->isValid()) {
                                if (triangle->getNormal(0)->dot(*_45C->getNormal(0)) <= 0.17f) {
                                    f32 component = MR::vecKillElement(mJumpVec, *triangle->getNormal(0), &mJumpVec);
                                    mJumpVec += *triangle->getNormal(0) * component * 0.5f;
                                    mJumpVec += *_45C->getNormal(0);
                                } else {
                                    if (mMovementStates._1A && (MR::diffAngleAbs(pushDirection, getAirGravityVec()) >= 1.4959966f ||
                                                                MR::diffAngleAbsHorizontal(mJumpVec, pushDirection, getAirGravityVec()) < 0.0f)) {
                                        pushDirection = -pushDirection;
                                    }
                                    mMovementStates._14 = false;
                                }
                            } else {
                                if (mMovementStates._1A && (MR::diffAngleAbs(pushDirection, getAirGravityVec()) >= 1.4959966f ||
                                                            MR::diffAngleAbsHorizontal(mJumpVec, pushDirection, getAirGravityVec()) < 0.0f)) {
                                    pushDirection = -pushDirection;
                                }
                                mMovementStates._14 = false;
                            }
                        }
                        if (mMovementStates.jumping && !isRising()) {
                            TVec3f horizontal;
                            const TVec3f& lastMove = mActor->_27C;
                            if (MR::isNearZero(MR::vecKillElement(lastMove, getAirGravityVec(), &horizontal)) &&
                                (mMovementStates._8 || mMovementStates._19 || mMovementStates._1A)) {
                                cutGravityElementFromJumpVec(true);
                                mJumpVec += -getAirGravityVec() * 5.0f + getWallNorm() * 2.0f;
                                if (!isCeiling()) {
                                    addTrans(getAirGravityVec() * -30.0f, nullptr);
                                }
                                pushDirection.zero();
                            }
                        }
                    }
                    push(pushDirection * hit->_60 * scale);
                    firstGround = false;
                    if (getPlayerMode() == PlayerMode_Teresa) {
                        _25C = hit->mHitPos;
                        _268 = *triangle->getNormal(0);
                        doTeresaReflection(fromHit, true);
                    }
                    _1C._D = true;
                }
            }
            if (mMovementStates.jumping && mMovementStates._B) {
                TVec3f lastMove;
                mActor->getLastMove(&lastMove);
                if (MR::isNearZero(lastMove.dot(*getGravityVec()))) {
                    mActor->sendMsgToSensor(triangle->mSensor, 0xB4);
                    if (!mActor->sendMsgToSensor(triangle->mSensor, 3)) {
                        TVec3f direction = mPosition - hit->mHitPos;
                        TVec3f originalDirection(direction);
                        TVec3f velocity(_1A8);
                        MR::normalizeOrZero(&velocity);
                        f32 blend = 1.0f;
                        if (!MR::isNearZero(velocity)) {
                            blend = velocity.dot(*triangle->getNormal(0));
                        }
                        if (blend < 0.0f) {
                            blend = 0.0f;
                        } else if (blend > 1.0f) {
                            blend = 1.0f;
                        }
                        MR::vecBlendSphere(_1A8, direction, &direction, blend);
                        MR::vecKillElement(direction, *getGravityVec(), &direction);
                        MR::normalizeOrZero(&direction);
                        if (MR::isNearZero(direction)) {
                            direction = mFrontVec;
                        }
                        f32 distance = hit->_60;
                        if (distance == 0.0f) {
                            distance = 1.0f;
                        }
                        push(direction * distance);
                        firstGround = false;
                    }
                }
            }
        } else if (!(dot < ceilingThreshold)) {
            TVec3f horizontal;
            if (firstCeiling && (MR::vecKillElement(mJumpVec, normal, &horizontal) < 0.0f || mPrevDrawStates._1E) && calcAngleD(normal) > 100.0f) {
                if (strcmp(triangle->mSensor->mHost->mName, "マンホールのふた(クッパ船)")) {
                    mJumpVec = horizontal;
                }
                startPadVib(0UL);
                firstCeiling = false;
                *_4C8 = *triangle;
            }
            _3A4 += normal;
        }
    }
    MR::normalizeOrZero(&_3A4);
    return true;
}

bool Mario::isThroughWall(const Triangle* triangle) const {
    if (getPlayerMode() == PlayerMode_Teresa) {
        if (!_418) {
            return false;
        }
        const char* code = MR::getWallCodeString(triangle);
        if (code && !strcmp(code, "GhostThroughCode")) {
            return true;
        }
    }
    return false;
}

bool Mario::checkGround() {
    if (isStatusActive(0x13) || isStatusActive(0x13)) {
        return false;
    }

    if (mMovementStates._14) {
        if (mMovementStates_HIGH_WORD & 0x00000200) {
            if (!checkGroundOnSlope()) {
                return false;
            }

            _1C_WORD |= 0x00001800;
            return true;
        }
    }
    else if (isUseSimpleGroundCheck()) {
        if (!(mMovementStates_HIGH_WORD & 0x00000200)) {
            if (MR::isNearZero(getGravityVec()->y - 1.0f, 0.001f)) {
                mActor->setBlendMtxTimer(4);
            }

            mMovementStates_HIGH_WORD |= 0x00000200;
        }

        if (!checkGroundOnSlope()) {
            return false;
        }

        _1C_WORD |= 0x00001800;
        return true;
    }
    else if (mMovementStates_HIGH_WORD & 0x00000200) {
        if (MR::isNearZero(getGravityVec()->y - 1.0f, 0.001f)) {
            mActor->setBlendMtxTimer(4);
        }

        mMovementStates_HIGH_WORD &= ~0x00000200;
    }

    TVec3f groundBase;
    if (isAnimationRun("崖ふんばり")) {
        groundBase = -*getGravityVec();
    }
    else {
        groundBase = *_45C->getNormal(0);
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

    Mtx rot;
    PSMTXRotAxisRad(rot, &groundBase, 2.0943952f);

    Triangle hitTriangles[4];
    TVec3f hitPositions[4];
    bool hitFlags[4] = { false, false, false, false };
    bool rejectedByLift[4] = { false, false, false, false };

    mMovementStates_LOW_WORD &= ~0x00000800;

    f32 verticalLimit = 30.0f;
    if (mMovementStates.jumping && isRising()) {
        verticalLimit = 10.0f;
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
    if (!mTargetWalkSpeedIndex && mMovementStates._1 && !mMovementStates.jumping && !(mMovementStates_HIGH_WORD & 0x10000000)) {
        probeCount = 4;
    }
    if (isAnimationRun("壁押し", 0)) {
        probeCount = 4;
    }

    TVec3f selectedGround;
    u32 hitCount = 0;
    bool shouldCommitGround = true;
    for (u32 i = 0; i < probeCount; i++) {
        TVec3f start = mPosition + probeStep;
        TVec3f gravityBack = *getGravityVec();
        gravityBack.scale(30.0f);
        start -= gravityBack;
        if (i == 3) {
            start = mPosition - gravityBack;
        }

        TVec3f ray = *getGravityVec();
        ray.scale(100.0f);

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

            {
                TVec3f toHit = mPosition - hitPositions[i];
                if (__fabsf(toHit.dot(*getGravityVec())) > verticalLimit) {
                    hitFlags[i] = false;
                    rejectedByLift[i] = true;
                }
            }

            if (calcAngleD(*hitTriangles[i].getNormal(0)) >= 80.0f) {
                hitFlags[i] = false;
            }

            {
                const f32 dot = getGravityVec()->dot(*hitTriangles[i].getNormal(0));
                if (getCurrentStatus() != 7 && dot > _3C) {
                    hitFlags[i] = false;
                }
            }

            if (isStatusActive(0x13) && MR::isThroughPolygon(&hitTriangles[i])) {
                hitFlags[i] = false;
            }

            if (hitFlags[i]) {
                hitAverage += hitPositions[i];
                hitCount++;

                if ((mMovementStates_HIGH_WORD & 0x10000000) && !shouldCommitGround) {
                    TVec3f velDir(_16C);
                    MR::normalizeOrZero(&velDir);
                    TVec3f oldToHit(selectedGround - mPosition);
                    TVec3f newToHit(hitPositions[i] - mPosition);
                    if (newToHit.dot(velDir) > oldToHit.dot(velDir)) {
                        shouldCommitGround = true;
                    }
                }

                if (shouldCommitGround) {
                    setGroundNorm(*hitTriangles[i].getNormal(0));
                    *mGroundPolygon = hitTriangles[i];
                    selectedGround = hitPositions[i];
                    mGroundPos = selectedGround;
                    recordLastGround();
                    shouldCommitGround = false;
                }

                const char* wallCode = MR::getWallCodeString(&hitTriangles[i]);
                if (wallCode != nullptr && strcmp(wallCode, "Fur") == 0) {
                    mDrawStates_WORD |= 0x00000040;
                }
            }
        }

        PSMTXMultVec(rot, &probeStep, &probeStep);
    }

    u32 presentCount = 0;
    while (presentCount < probeCount && hitFlags[presentCount]) {
        presentCount++;
    }
    if (presentCount == probeCount) {
        _1C_WORD |= 0x00004000;
    }

    if (!isNoWalkFallOnDossun() && !isStatusActive(3) && !isStatusActive(2) && !isStatusActive(0x22) && !isStatusActive(5)
        && (!mMovementStates._18 || _10._10) && ((probeCount == 4 && !hitFlags[3]) || _960 == 0x13)) {
        do {
            if (!hitFlags[0] && !hitFlags[1] && !hitFlags[2]) {
                return false;
            }

            if (!hitFlags[0]) {
                bool moved = false;
                if (!mMovementStates._8) {
                    TVec3f push = mFrontVec;
                    push.scale(6.0f);
                    addTrans(push, "前方WKFALL");
                    moved = true;
                }

                if (hitFlags[1] && hitFlags[2]) {
                    if (!moved) {
                        break;
                    }
                }
                else if (!hitFlags[1]) {
                    if ((mMovementStates._1A || mMovementStates._8 || mMovementStates._19) && mSideVec.dot(getWallNorm()) < 0.0f) {
                        if (moved) {
                            TVec3f front = mFrontVec;
                            front.scale(6.0f);
                            TVec3f side = -mSideVec;
                            side.scale(3.0f);
                            addTrans(side - front, "+逆-左WKFALL");
                        }
                        break;
                    }

                    TVec3f push = mSideVec;
                    push.scale(3.0f);
                    addTrans(push, "+左WKFALL");
                    moved = true;
                }
                else {
                    if ((mMovementStates._1A || mMovementStates._8 || mMovementStates._19) && -mSideVec.dot(getWallNorm()) < 0.0f) {
                        if (moved) {
                            TVec3f front = mFrontVec;
                            front.scale(6.0f);
                            TVec3f side = mSideVec;
                            side.scale(3.0f);
                            addTrans(side - front, "+逆-右WKFALL");
                        }
                        break;
                    }

                    TVec3f push = -mSideVec;
                    push.scale(3.0f);
                    addTrans(push, "+右WKFALL");
                    moved = true;
                }

                if (moved) {
                    mDrawStates_WORD |= 0x00200000;
                }
                return true;
            }
            else if (!hitFlags[1] && !hitFlags[2]) {
                stopWalk();
                if (_3CE < 24 && mJumpVec.dot(mFrontVec) >= 0.0f && _960 != 0x13) {
                    if (!mMovementStates._8 && !mMovementStates._32) {
                        TVec3f push = mFrontVec;
                        push.scale(3.0f);
                        addTrans(push, "+後ろ1WKFALL");
                        mDrawStates_WORD |= 0x00200000;
                    }
                    break;
                }

                if (!mMovementStates._19) {
                    mDrawStates_WORD |= 0x00000080;
                    TVec3f push = mFrontVec;
                    push.scale(-6.0f);
                    addTrans(push, "+後ろ2WKFALL");
                    mDrawStates_WORD |= 0x00200000;
                }

                if (_3C6 > 8) {
                    TVec3f push = mSideVec;
                    push.scale(5.0f);
                    addTrans(push, "+SIDEFALL");
                }
                return true;
            }
            else if (_960 == 0x13) {
                if (!hitFlags[2]) {
                    TVec3f push = -mSideVec;
                    push.scale(5.0f);
                    addTrans(push, "-L-SIDEFALL");
                }
                else if (!hitFlags[1]) {
                    TVec3f push = mSideVec;
                    push.scale(5.0f);
                    addTrans(push, "+R-SIDEFALL");
                }
                else {
                    TVec3f push = mFrontVec;
                    push.scale(5.0f);
                    addTrans(push, "+F-SIDEFALL");
                }
                mDrawStates_WORD |= 0x00200000;
            }
        } while (false);
    }

    bool noGround = hitCount == 0;

    s32 sameSensorBalance = 0;
    s32 agreeingNormals = 0;
    TVec3f groundHorizontalNormal;
    MR::vecKillElement(*mGroundPolygon->getNormal(0), getAirGravityVec(), &groundHorizontalNormal);
    for (u32 i = 0; i < hitCount; ++i) {
        if (hitFlags[i]) {
            sameSensorBalance += hitTriangles[i].mSensor == mGroundPolygon->mSensor ? 1 : -1;
            TVec3f horizontalNormal;
            MR::vecKillElement(*hitTriangles[i].getNormal(0), getAirGravityVec(), &horizontalNormal);
            if (horizontalNormal.dot(groundHorizontalNormal) >= 0.0f) {
                ++agreeingNormals;
            }
        }
    }
    if (sameSensorBalance > 0) {
        _1C_WORD |= 0x1000;
    }
    if (agreeingNormals >= 3) {
        _1C_WORD |= 0x800;
    }
    _8EC = sameSensorBalance;

    if (mMovementStates._D) {
        mMovementStates._D = false;
        TVec3f horizontal(mGroundPos - mPosition);
        f32 height = MR::vecKillElement(horizontal, *getGravityVec(), &horizontal);
        addTrans(*getGravityVec() * height, "force Trans");
        return true;
    }

    if (hitCount != 0) {
        f32 height = 0.0f;
        TVec3f horizontal;
        if (calcAngleD(*_45C->getNormal(0)) < 55.0f) {
            height = MR::vecKillElement(mShadowPos - mPosition, *getGravityVec(), &horizontal);
        }
        if (__fabsf(height) > 30.0f) {
            height = MR::vecKillElement(mGroundPos - mPosition, *getGravityVec(), &horizontal);
        }
        if (__fabsf(height) < 30.0f && __fabsf(height) > 1.0f && mMovementStates._1) {
            f32 groundAlignment = getGravityVec()->dot(-_368);
            if (groundAlignment > 0.99f) {
                if (!mDrawStates._9 && !_4D8->isValid()) {
                    if (!isStatusActive(0x22)) {
                        addTrans(*getGravityVec() * height, "force Trs2");
                    }
                    return true;
                }
            }
            else if (groundAlignment > 0.0f && !mDrawStates._9 && mMovementStates._23) {
                addTrans(-_368 * height * groundAlignment, "force Trs3");
                return true;
            }
        }
    }

    if (mMovementStates.jumping && isRising()) {
        return false;
    }

    if (!isStatusActive(0x1B) && probeCount == 4 && noGround) {
        if (hitFlags[3]) {
            TVec3f toHit = mPosition - hitPositions[3];
            if (__fabsf(toHit.dot(*getGravityVec())) < verticalLimit) {
                noGround = false;
            }
        }

        bool canForward = !mMovementStates._8;
        bool canBackward = !mMovementStates._19;
        if (mActor->_288.dot(mFrontVec) < 0.0f) {
            if (canBackward) {
                TVec3f push = -mFrontVec;
                push.scale(6.0f);
                addTrans(push, "no-g(back)");
            }
        }
        else if (canForward) {
            TVec3f push = mFrontVec;
            push.scale(6.0f);
            addTrans(push, "no-g");
        }
    }

    if (noGround) {
        mMovementStates_LOW_WORD |= 0x00000800;
        return false;
    }

    if (!mActor->_EA4 && getCurrentStatus() != 5) {
        if ((mMovementStates.jumping && hitCount != 0) || (mMovementStates._1 && mVerticalSpeed >= 5.0f)) {
            TVec3f groundSeparation = mGroundPos - mShadowPos;
            TVec3f horizontalGround;
            f32 groundDistance = MR::vecKillElement(groundSeparation, _368, &horizontalGround);
            f32 maxSlide = 5.0f;
            horizontalGround = mShadowPos - mPosition;

            if (mMovementStates._8 && horizontalGround.dot(getFrontWallNorm()) < -0.01f) {
                maxSlide = 0.0f;
            }
            if (mMovementStates._1A && horizontalGround.dot(getSideWallNorm()) < -0.01f) {
                maxSlide = 0.0f;
            }
            if (mMovementStates._8 && horizontalGround.dot(getBackWallNorm()) < -0.01f) {
                maxSlide = 0.0f;
            }

            if (maxSlide != 0.0f) {
                if (PSVECMag(&horizontalGround) > maxSlide) {
                    mShadowPos = mShadowPos - mPosition;
                    mShadowPos.setLength(maxSlide);
                    mShadowPos += mPosition;
                }
                if (groundDistance < verticalLimit) {
                    setTrans(mShadowPos, nullptr);
                }
            }
        }

        if (hitCount == 0) {
            TVec3f toShadow = mShadowPos - mPosition;
            if (__fabsf(toShadow.dot(*getGravityVec())) < verticalLimit) {
                TVec3f horizontal = mShadowPos - mPosition;
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
    }

    if (hitCount == 0) {
        TVec3f toGround = mGroundPos - mPosition;
        if (__fabsf(toGround.dot(*getGravityVec())) < verticalLimit) {
            TVec3f horizontal = mGroundPos - mPosition;
            f32 alongGravity = MR::vecKillElement(horizontal, *getGravityVec(), &horizontal);
            if (!MR::isNearZero(alongGravity, 1.0f)) {
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

    return hitCount != 0 || (mDrawStates_WORD >> 31);
}

CubeCameraArea* Mario::getCameraCubeCode() const {
    if ((isSwimming() && mSwim->isOnWaterSurface()) || (mMovementStates.jumping && isRising())) {
        return static_cast< CubeCameraArea* >(MR::getAreaObj("CubeCamera", mPosition - *getGravityVec() * 100.0f));
    }
    return static_cast< CubeCameraArea* >(MR::getAreaObj("CubeCamera", mPosition));
}

void Mario::updateCubeCode() {
    CubeCameraArea* camera;
    if ((isSwimming() && mSwim->isOnWaterSurface()) || (mMovementStates.jumping && isRising())) {
        camera = static_cast< CubeCameraArea* >(MR::getAreaObj("CubeCamera", mPosition - *getGravityVec() * 100.0f));
    } else {
        camera = static_cast< CubeCameraArea* >(MR::getAreaObj("CubeCamera", mPosition));
    }
    _568 = camera;
    _570 = 0;
    if (camera) {
        _564 = MR::getAreaObjArg(camera, 0);
        switch (MR::getAreaObjArg(camera, 1)) {
        case 1:
            mDrawStates._2 = true;
            break;
        case 2:
            mDrawStates._3 = true;
            break;
        }
    } else {
        _564 = -1;
    }
    MR::tryToUpdatePlayerRestartIdInfo(mPosition);
    AreaObj* pullBack = MR::getAreaObj("PullBackCube", mPosition);
    if (!pullBack) {
        pullBack = MR::getAreaObj("PullBackCylinder", mPosition);
        if (pullBack && MR::getAreaObjArg(pullBack, 0) != 1) {
            TVec3f position;
            TVec3f up;
            MR::calcCylinderPos(&position, pullBack);
            MR::calcCylinderUpVec(&up, pullBack);
            MarioRecovery* recovery = mRecovery;
            recovery->_4C = position;
            recovery->_58 = up;
            recovery->_12 = true;
        }
    }
    if (pullBack) {
        doRecovery();
    }
    _10._6 = false;
    if (MR::getAreaObj("PlaneCollisionCube", mPosition)) {
        _10._6 = true;
    }
    _10._13 = false;
    AreaObj* tower = MR::getAreaObj("TowerModeCylinder", mPosition);
    if (tower) {
        _10._13 = true;
        MR::calcCylinderCenterPos(&_6F4, tower);
        MR::calcCylinderUpVec(&_700, tower);
        _718 = MR::getCylinderRadius(tower);
    }
    if (MR::getAreaObj("ForbidTriangleJumpCube", mPosition)) {
        mDrawStates._3 = true;
    }
    if (getPlayerMode() == PlayerMode_Teresa) {
        if (MR::getAreaObj("GlaringLightArea", mPosition)) {
            mActor->setPlayerMode(PlayerMode_Normal, true);
        }
        if (!MR::isInShadeFromTheSun(mPosition, 2000.0f)) {
            mActor->setPlayerMode(PlayerMode_Normal, true);
        }
    }
    if (MR::getAreaObj("FallsCube", mActor->_2AC)) {
        touchWater();
        playEffectRTZ("水壁ヒット", mHeadVec, mActor->_2AC);
    }
    if (MR::getAreaObj("HeavySteeringCube", mPosition)) {
        _10._11 = true;
        _10._12 = true;
    } else {
        _10._11 = false;
        _10._12 = false;
    }
    if (MR::getAreaObj("DashChargeCylinder", mPosition)) {
        _434 = mActor->getConst().getTable()->mItemDashTimer;
    }
    AreaObj* raster = MR::getAreaObj("RasterScrollCube", mPosition);
    if (raster) {
        s32 first = MR::getAreaObjArg(raster, 0);
        s32 second = MR::getAreaObjArg(raster, 1);
        mActor->setRasterScroll(first, second, MR::getAreaObjArg(raster, 2));
    }
    if (MR::getAreaObj("ForbidJumpCube", mPosition)) {
        _1C._7 = true;
    }
    if (!isStatusActive(MarioStatus_DarkDamage)) {
        if (MR::getAreaObj("DarkMatterCube", mPosition)) {
            mActor->forceKill(4);
        }
        mVelocity.zero();
    }
}
