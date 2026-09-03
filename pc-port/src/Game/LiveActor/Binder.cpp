#include "Game/LiveActor/Binder.hpp"
#include "Game/Map/CollisionCategorizedKeeper.hpp"
#include "Game/Map/CollisionDirector.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include <algorithm>

HitInfo& HitInfo::operator=(const HitInfo& rOther) {
    mParentTriangle.mParts = rOther.mParentTriangle.mParts;
    mParentTriangle.mIdx = rOther.mParentTriangle.mIdx;
    mParentTriangle.mSensor = rOther.mParentTriangle.mSensor;
    mParentTriangle.mNormals[0] = rOther.mParentTriangle.mNormals[0];
    mParentTriangle.mNormals[1] = rOther.mParentTriangle.mNormals[1];
    mParentTriangle.mNormals[2] = rOther.mParentTriangle.mNormals[2];
    mParentTriangle.mNormals[3] = rOther.mParentTriangle.mNormals[3];
    mParentTriangle.mPos[0] = rOther.mParentTriangle.mPos[0];
    mParentTriangle.mPos[1] = rOther.mParentTriangle.mPos[1];
    mParentTriangle.mPos[2] = rOther.mParentTriangle.mPos[2];
    _60 = rOther._60;
    mHitPos = rOther.mHitPos;
    _70 = rOther._70;
    _7C = rOther._7C;
    _88 = rOther._88;

    return *this;
}

Binder::Binder(MtxPtr mtx, const TVec3f* v1, const TVec3f* v2, f32 radius, f32 offsetY, u32 planeNum)
    : mTriangleFilter(), mCollisionPartsFilter(), mExCollisionParts(), _C(mtx), _10(v1), _14(v2), mRadius(radius), mOffsetY(offsetY), mOffsetVec(),
      _24(planeNum), mPlaneNum(), mPlane(), mFixReactionVector(0, 0, 0), mGroundInfo(), mWallInfo(), mRoofInfo(), _C8(), _158(), _1E8() {
    if (_24 == 0) {
        mPlane = nullptr;
    } else {
        mPlane = new HitInfo[_24];
    }

    clear();
    _1EC._0 = true;
    _1EC._1 = true;
    _1EC._2 = false;
    _1EC._3 = false;
    _1EC._4 = false;
    _1EC._5 = false;
}

void Binder::setTriangleFilter(TriangleFilterBase* pFilter) {
    mTriangleFilter = pFilter;
}

void Binder::setCollisionPartsFilter(CollisionPartsFilterBase* pFilter) {
    mCollisionPartsFilter = pFilter;
}

void Binder::clear() {
    mPlaneNum = 0;
    _C8 = -99999.0f;
    _158 = -99999.0f;
    _1E8 = -99999.0f;
    mFixReactionVector.zero();
}

const HitInfo* Binder::getPlane(int index) const {
    return &mPlane[index];
}

u32 Binder::copyPlaneArrayAndSortingSensor(HitInfo** pPlane, u32) {
    if (_24 == 0) {
        u32 count = 0;
        if (isBindedGround()) {
            pPlane[count++] = &mGroundInfo;
        }
        if (isBindedWall()) {
            pPlane[count++] = &mWallInfo;
        }
        if (isBindedRoof()) {
            pPlane[count++] = &mRoofInfo;
        }
        std::sort(pPlane, pPlane + count, compSensor);
        return count;
    }

    for (u32 i = 0; i < mPlaneNum; i++) {
        pPlane[i] = &mPlane[i];
    }
    std::sort(pPlane, pPlane + mPlaneNum, compSensor);
    return mPlaneNum;
}

const TVec3f Binder::bind(const TVec3f& rMovement) {
    bool noMargin = _1EC._5;
    _1EC._5 = false;
    clear();

    TVec3f position(*_10);
    TVec3f movement(rMovement);
    if (_1EC._2 && mExCollisionParts != nullptr) {
        MR::getCollisionDirector()->getCategoryKeeper(0)->addToGlobal(mExCollisionParts);
    }

    if (mOffsetVec != nullptr) {
        if (_1EC._4 && _C != nullptr) {
            position.x += _C[0][0] * mOffsetVec->x;
            position.y += _C[1][0] * mOffsetVec->x;
            position.z += _C[2][0] * mOffsetVec->x;
            position.x += _C[0][1] * mOffsetVec->y;
            position.y += _C[1][1] * mOffsetVec->y;
            position.z += _C[2][1] * mOffsetVec->y;
            position.x += _C[0][2] * mOffsetVec->z;
            position.y += _C[1][2] * mOffsetVec->z;
            position.z += _C[2][2] * mOffsetVec->z;
        } else {
            position.x += mOffsetVec->x;
            position.y += mOffsetVec->y;
            position.z += mOffsetVec->z;
        }
    } else if (_C != nullptr) {
        position.x += _C[0][1] * mOffsetY;
        position.y += _C[1][1] * mOffsetY;
        position.z += _C[2][1] * mOffsetY;
    } else {
        position.y += mOffsetY;
    }

    TVec3f previousPosition(position);
    HitInfo temporaryPlane[32];
    u32 capacity = _24;
    HitInfo* pPlane;
    if (capacity == 0) {
        pPlane = temporaryPlane;
        capacity = 32;
    } else {
        pPlane = mPlane;
    }

    bool canMoveMore;
    u32 firstPlaneNum = findBindedPos(&position, &movement, &canMoveMore, pPlane, capacity, false, noMargin);
    TVec3f result;
    if (firstPlaneNum == 0) {
        result = movement;
    } else {
        TVec3f firstReaction(0, 0, 0);
        TVec3f nextReaction(0, 0, 0);
        TVec3f additionalReaction(0, 0, 0);
        obtainMomentFixReaction(pPlane, capacity, &firstReaction, 0);
        position.add(firstReaction);
        TVec3f lastReaction(firstReaction);
        while (!noMargin && canMoveMore) {
            TVec3f remaining(rMovement);
            remaining.sub(movement);
            moveAlongHittedPlanes(&movement, &position, &remaining, rMovement, lastReaction, pPlane, capacity, &canMoveMore);
            obtainMomentFixReaction(pPlane, capacity, &nextReaction, firstPlaneNum);
            position.add(nextReaction);
            lastReaction.set(nextReaction);
            additionalReaction.add(nextReaction);
            canMoveMore = false;
        }

        storeContactPlane(pPlane, capacity);
        if (_1EC._0) {
            moveWithCollisionParts(&position, &movement);
        }

        TVec3f fixReaction(firstReaction);
        fixReaction.add(additionalReaction);
        mFixReactionVector = fixReaction;
        TVec3f displacement(position);
        displacement.sub(previousPosition);
        result = displacement;
    }

    if (_1EC._2 && mExCollisionParts != nullptr) {
        MR::getCollisionDirector()->getCategoryKeeper(0)->removeFromGlobal(mExCollisionParts);
    }
    return result;
}

void Binder::moveAlongHittedPlanes(TVec3f* pMovement, TVec3f* pPosition, TVec3f* pRemaining,
                                   const TVec3f& rRequested, const TVec3f& rReaction, HitInfo* pPlane,
                                   u32 capacity, bool* pCanMoveMore) {
    TVec3f normal(rReaction);
    MR::normalizeOrZero(&normal);
    f32 inward = pRemaining->dot(normal);
    if (inward < 0.0f) {
        TVec3f projected(normal);
        projected.scale(inward);
        pRemaining->sub(projected);
    }

    if (rRequested.dot(*pRemaining) < 0.0f) {
        *pCanMoveMore = false;
    } else {
        findBindedPos(pPosition, pRemaining, pCanMoveMore, pPlane, capacity, true, false);
        pMovement->add(*pRemaining);
    }
}

u32 Binder::findBindedPos(TVec3f* pPosition, TVec3f* pMovement, bool* pCanMoveMore,
                        HitInfo* pPlane, u32 capacity, bool skipInitial, bool noMargin) {
    s32 stepCount = static_cast<s32>(pMovement->length() * (1.0f / 35.0f)) + 1;
    TVec3f step(*pMovement);
    if (stepCount > 1) {
        step.scale(1.0f / stepCount);
    }

    pMovement->zero();
    for (s32 i = 0; i <= stepCount; i++) {
        if (i != 0) {
            pPosition->add(step);
            pMovement->add(step);
        } else if (_1EC._3 || skipInitial) {
            continue;
        }

        s32 hitCount;
        if (_1EC._1) {
            hitCount = Collision::checkStrikeBallToMapWithMovingReaction(*pPosition, mRadius, mCollisionPartsFilter, mTriangleFilter);
        } else {
            hitCount = Collision::checkStrikeBallToMap(*pPosition, mRadius, mCollisionPartsFilter, mTriangleFilter);
        }
        if (hitCount != 0) {
            if (i == stepCount) {
                *pCanMoveMore = false;
            } else {
                *pCanMoveMore = true;
            }
            return storeCurrentHitInfo(pPlane, capacity, noMargin);
        }
    }

    *pCanMoveMore = false;
    return 0;
}

bool Binder::moveWithCollisionParts(TVec3f* pPosition, TVec3f* pMovement) {
    if (_C8 < 0.0f) {
        return false;
    }
    if (!mGroundInfo.mParentTriangle.isHostMoved()) {
        return false;
    }

    TVec3f power;
    mGroundInfo.mParentTriangle.calcForceMovePower(&power, *pPosition);
    pMovement->add(power);
    pPosition->add(power);
    return true;
}

u32 Binder::storeCurrentHitInfo(HitInfo* pPlane, u32 capacity, bool noMargin) {
    u32 hitCount = Collision::getStrikeInfoNumMap();
    u32 stored = 0;
    for (u32 i = 0; i < hitCount; i++) {
        if (capacity <= stored + mPlaneNum) {
            u32 previousCount = mPlaneNum;
            mPlaneNum = capacity;
            return capacity - previousCount;
        }

        pPlane[mPlaneNum + stored] = *Collision::getStrikeInfoMap(i);
        if (!noMargin) {
            pPlane[mPlaneNum + i]._60 += 1.2f;
        }
        stored++;
    }
    mPlaneNum += stored;
    return stored;
}

void Binder::obtainMomentFixReaction(HitInfo* pPlane, u32, TVec3f* pReaction, u32 start) {
    TVec3f positive(0, 0, 0);
    TVec3f negative(0, 0, 0);

    for (u32 i = start; i < mPlaneNum; i++) {
        HitInfo& rInfo = pPlane[i];
        TVec3f normal(*rInfo.mParentTriangle.getNormal(0));
        f32 x = normal.x * rInfo._60;
        if (positive.x < x) {
            positive.x = x;
        } else if (x < negative.x) {
            negative.x = x;
        }

        f32 y = normal.y * rInfo._60;
        if (positive.y < y) {
            positive.y = y;
        } else if (y < negative.y) {
            negative.y = y;
        }

        f32 z = normal.z * rInfo._60;
        if (positive.z < z) {
            positive.z = z;
        } else if (z < negative.z) {
            negative.z = z;
        }

        if (_1EC._1 && !MR::isNearZero(rInfo._7C)) {
            if (positive.x < rInfo._7C.x) {
                positive.x = rInfo._7C.x;
            } else if (rInfo._7C.x < negative.x) {
                negative.x = rInfo._7C.x;
            }

            if (positive.y < rInfo._7C.y) {
                positive.y = rInfo._7C.y;
            } else if (rInfo._7C.y < negative.y) {
                negative.y = rInfo._7C.y;
            }

            if (positive.z < rInfo._7C.z) {
                positive.z = rInfo._7C.z;
            } else if (rInfo._7C.z < negative.z) {
                negative.z = rInfo._7C.z;
            }
        }
    }

    pReaction->set(positive);
    pReaction->add(negative);
}

void Binder::storeContactPlane(HitInfo* pPlane, u32) {
    for (u32 i = 0; i < mPlaneNum; i++) {
        HitInfo& rInfo = pPlane[i];
        const TVec3f* pNormal = rInfo.mParentTriangle.getNormal(0);
        if (MR::isFloorPolygon(*pNormal, *_14)) {
            if (_C8 < rInfo._60) {
                mGroundInfo = rInfo;
                _C8 = rInfo._60;
            }
        } else if (MR::isWallPolygon(*pNormal, *_14)) {
            if (_158 < rInfo._60) {
                mWallInfo = rInfo;
                _158 = rInfo._60;
            }
        } else if (_1E8 < rInfo._60) {
            mRoofInfo = rInfo;
            _1E8 = rInfo._60;
        }
    }
}

bool Binder::compSensor(const HitInfo* pPlane1, const HitInfo* pPlane2) {
    return pPlane1->mParentTriangle.mSensor > pPlane2->mParentTriangle.mSensor;
}
