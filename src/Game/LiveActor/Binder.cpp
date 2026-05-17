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

Binder::Binder(MtxPtr mtx, const TVec3f* v1, const TVec3f* v2, f32 radius, f32 b, u32 c)
    : BinderParent(mtx), _10(v1), _14(v2), mRadius(radius), _1C(b), mOffsetVec(0), _24(c), mPlaneNum(0), mPlaneInfos(0), mFixReactionVector(0, 0, 0),
      mGroundInfo(), mWallInfo(), mRoofInfo(), _C8(131076.953125f), _158(131076.953125f), _1E8(131076.953125f) {
    if (!_24) {
        mPlaneInfos = nullptr;
    } else {
        mPlaneInfos = new HitInfo[_24];
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

const Triangle* Binder::getPlane(int idx) const {
    return &mPlaneInfos[idx].mParentTriangle;
}

u32 Binder::copyPlaneArrayAndSortingSensor(HitInfo** pInfos, u32) {
    if (_24 == 0) {
        u32 numPlanes = 0;

        if (0.0f >= _C8) {
            pInfos[numPlanes] = &mGroundInfo;
            numPlanes++;
        }

        if (0.0f >= _158) {
            pInfos[numPlanes] = &mWallInfo;
            numPlanes++;
        }

        if (0.0f >= _1E8) {
            pInfos[numPlanes] = &mRoofInfo;
            numPlanes++;
        }

        std::sort(pInfos, pInfos + numPlanes, compSensor);
        return numPlanes;
    }

    for (u32 i = 0; i < static_cast< u32 >(mPlaneNum); i++) {
        pInfos[i] = &mPlaneInfos[i];
    }

    std::sort(pInfos, pInfos + mPlaneNum, compSensor);
    return mPlaneNum;
}

bool Binder::compSensor(const HitInfo* pInfo1, const HitInfo* pInfo2) {
    return reinterpret_cast< u32 >(pInfo2->mParentTriangle.mSensor) <
           reinterpret_cast< u32 >(pInfo1->mParentTriangle.mSensor);
}

const TVec3f Binder::bind(const TVec3f& rMove) {
    bool wasSingleMove = _1EC._5;
    _1EC._5 = false;
    clear();

    TVec3f center(*_10);
    TVec3f move(rMove);

    if (_1EC._2 && mCollisionParts != nullptr) {
        MR::getCollisionDirector()->mKeepers[0]->addToGlobal(mCollisionParts);
    }

    if (mOffsetVec != nullptr) {
        if (_1EC._4 && _C != nullptr) {
            center.x += _C[0][0] * mOffsetVec->x;
            center.y += _C[1][0] * mOffsetVec->x;
            center.z += _C[2][0] * mOffsetVec->x;
            center.x += _C[0][1] * mOffsetVec->y;
            center.y += _C[1][1] * mOffsetVec->y;
            center.z += _C[2][1] * mOffsetVec->y;
            center.x += _C[0][2] * mOffsetVec->z;
            center.y += _C[1][2] * mOffsetVec->z;
            center.z += _C[2][2] * mOffsetVec->z;
        } else {
            center.add(*mOffsetVec);
        }
    } else if (_C != nullptr) {
        center.x += _C[0][1] * _1C;
        center.y += _C[1][1] * _1C;
        center.z += _C[2][1] * _1C;
    } else {
        center.y += _1C;
    }

    TVec3f bindStart(center);
    HitInfo localInfos[32];
    u32 maxInfos = _24;
    HitInfo* hitInfos = mPlaneInfos;

    if (maxInfos == 0) {
        hitInfos = localInfos;
        maxInfos = 32;
    }

    bool canMoveMore;
    u32 firstStoreNum = findBindedPos(&center, &move, &canMoveMore, hitInfos, maxInfos, false, wasSingleMove);

    if (firstStoreNum == 0) {
        if (_1EC._2 && mCollisionParts != nullptr) {
            MR::getCollisionDirector()->mKeepers[0]->removeFromGlobal(mCollisionParts);
        }

        return move;
    }

    TVec3f firstReaction(0, 0, 0);
    TVec3f loopReaction(0, 0, 0);
    TVec3f totalLoopReaction(0, 0, 0);
    obtainMomentFixReaction(hitInfos, maxInfos, &firstReaction, 0);
    center.add(firstReaction);

    TVec3f previousReaction(firstReaction);

    if (!wasSingleMove && canMoveMore) {
        TVec3f restMove(rMove);
        restMove.sub(move);
        moveAlongHittedPlanes(&move, &center, &restMove, rMove, previousReaction, hitInfos, maxInfos, &canMoveMore);
        obtainMomentFixReaction(hitInfos, maxInfos, &loopReaction, firstStoreNum);
        center.add(loopReaction);
        previousReaction.set< f32 >(loopReaction);
        totalLoopReaction.add(loopReaction);
        canMoveMore = false;
    }

    storeContactPlane(hitInfos, maxInfos);

    if (_1EC._0) {
        moveWithCollisionParts(&center, &move);
    }

    TVec3f fixReaction(firstReaction);
    fixReaction.add(totalLoopReaction);
    mFixReactionVector = fixReaction;

    TVec3f result(center);
    result.sub(bindStart);

    if (_1EC._2 && mCollisionParts != nullptr) {
        MR::getCollisionDirector()->mKeepers[0]->removeFromGlobal(mCollisionParts);
    }

    return result;
}

void Binder::moveAlongHittedPlanes(TVec3f* pMove, TVec3f* pCenter, TVec3f* pRestMove, const TVec3f& rOriginalMove, const TVec3f& rReaction,
                                   HitInfo* pInfos, u32 maxInfos, bool* pCanMoveMore) {
    TVec3f normal(rReaction);
    MR::normalizeOrZero(&normal);

    f32 dot = pRestMove->dot(normal);
    if (dot < 0.0f) {
        TVec3f fix(normal);
        fix.scale(dot);
        pRestMove->sub(fix);
    }

    if (rOriginalMove.dot(*pRestMove) < 0.0f) {
        *pCanMoveMore = false;
    } else {
        findBindedPos(pCenter, pRestMove, pCanMoveMore, pInfos, maxInfos, true, false);
        pMove->add(*pRestMove);
    }
}

u32 Binder::findBindedPos(TVec3f* pCenter, TVec3f* pMove, bool* pCanMoveMore, HitInfo* pInfos, u32 maxInfos, bool skipFirstCheck,
                          bool noAddOffset) {
    s32 stepNum = static_cast< s32 >(0.028571429f * pMove->length()) + 1;
    TVec3f step(*pMove);

    if (stepNum > 1) {
        step.scale(1.0f / static_cast< f32 >(stepNum));
    }

    pMove->zero();

    for (s32 i = 0; i <= stepNum; i++) {
        if (i != 0) {
            pCenter->add(step);
            pMove->add(step);
        } else if (_1EC._3 || skipFirstCheck) {
            continue;
        }

        s32 hitNum;
        if (_1EC._1) {
            hitNum = Collision::checkStrikeBallToMapWithMovingReaction(*pCenter, mRadius, mCollisionPartsFilter, mTriangleFilter);
        } else {
            hitNum = Collision::checkStrikeBallToMap(*pCenter, mRadius, mCollisionPartsFilter, mTriangleFilter);
        }

        if (hitNum != 0) {
            *pCanMoveMore = i != stepNum;
            return storeCurrentHitInfo(pInfos, maxInfos, noAddOffset);
        }
    }

    *pCanMoveMore = false;
    return 0;
}

bool Binder::moveWithCollisionParts(TVec3f* pCenter, TVec3f* pMove) {
    if (_C8 < 0.0f) {
        return false;
    }

    if (!mGroundInfo.mParentTriangle.isHostMoved()) {
        return false;
    }

    TVec3f forceMovePower;
    mGroundInfo.mParentTriangle.calcForceMovePower(&forceMovePower, *pCenter);
    pMove->add(forceMovePower);
    pCenter->add(forceMovePower);
    return true;
}

u32 Binder::storeCurrentHitInfo(HitInfo* pInfos, u32 maxInfos, bool noAddOffset) {
    u32 strikeNum = Collision::getStrikeInfoNumMap();
    u32 storedNum = 0;
    u32 i = 0;

    while (i < strikeNum) {
        u32 planeNum = mPlaneNum;
        if (maxInfos <= storedNum + planeNum) {
            mPlaneNum = maxInfos;
            return maxInfos - planeNum;
        }

        pInfos[planeNum + storedNum] = *Collision::getStrikeInfoMap(i);

        if (!noAddOffset) {
            pInfos[mPlaneNum + i]._60 += 1.2f;
        }

        storedNum++;
        i++;
    }

    mPlaneNum += storedNum;
    return storedNum;
}

void Binder::obtainMomentFixReaction(HitInfo* pInfos, u32, TVec3f* pReaction, u32 startIdx) {
    TVec3f positive(0, 0, 0);
    TVec3f negative(0, 0, 0);

    for (u32 i = startIdx; i < static_cast< u32 >(mPlaneNum); i++) {
        HitInfo* pInfo = &pInfos[i];
        TVec3f normal(*pInfo->mParentTriangle.getNormal(0));

        f32 x = normal.x * pInfo->_60;
        if (positive.x < x) {
            positive.x = x;
        } else if (x < negative.x) {
            negative.x = x;
        }

        f32 y = normal.y * pInfo->_60;
        if (positive.y < y) {
            positive.y = y;
        } else if (y < negative.y) {
            negative.y = y;
        }

        f32 z = normal.z * pInfo->_60;
        if (positive.z < z) {
            positive.z = z;
        } else if (z < negative.z) {
            negative.z = z;
        }

        if (_1EC._1 && !MR::isNearZero(pInfo->_7C)) {
            if (positive.x < pInfo->_7C.x) {
                positive.x = pInfo->_7C.x;
            } else if (pInfo->_7C.x < negative.x) {
                negative.x = pInfo->_7C.x;
            }

            if (positive.y < pInfo->_7C.y) {
                positive.y = pInfo->_7C.y;
            } else if (pInfo->_7C.y < negative.y) {
                negative.y = pInfo->_7C.y;
            }

            if (positive.z < pInfo->_7C.z) {
                positive.z = pInfo->_7C.z;
            } else if (pInfo->_7C.z < negative.z) {
                negative.z = pInfo->_7C.z;
            }
        }
    }

    pReaction->set< f32 >(positive);
    pReaction->add(negative);
}

void Binder::storeContactPlane(HitInfo* pInfos, u32) {
    for (u32 i = 0; i < static_cast< u32 >(mPlaneNum); i++) {
        HitInfo* pInfo = &pInfos[i];
        const TVec3f* pNormal = pInfo->mParentTriangle.getNormal(0);

        if (MR::isFloorPolygon(*pNormal, *_14)) {
            if (_C8 < pInfo->_60) {
                mGroundInfo = *pInfo;
                _C8 = pInfo->_60;
            }
        } else if (MR::isWallPolygon(*pNormal, *_14)) {
            if (_158 < pInfo->_60) {
                mWallInfo = *pInfo;
                _158 = pInfo->_60;
            }
        } else {
            if (_1E8 < pInfo->_60) {
                mRoofInfo = *pInfo;
                _1E8 = pInfo->_60;
            }
        }
    }
}
