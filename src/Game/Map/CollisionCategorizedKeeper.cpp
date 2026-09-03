#include "Game/Map/CollisionCategorizedKeeper.hpp"
#include "Game/Map/CollisionDirector.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Map/CollisionParts.hpp"
#include "Game/Util/CollisionPartsFilter.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include <algorithm>

CollisionCategorizedKeeper::CollisionCategorizedKeeper(s32 category)
    : NameObj("地形コリジョンカテゴリキーパー"), mHitInfoArray(nullptr), _10(0), mZoneCount(0), mZoneNum(0), _9C(category), _A0(false), _A1(true) {
    mHitInfoArray = new HitInfo[32];
}

CollisionCategorizedKeeper::~CollisionCategorizedKeeper() {
}

void CollisionCategorizedKeeper::movement() {
    for (CollisionZone** pZone = mZones; pZone != mZones + mZoneNum; pZone++) {
        s32 count = (*pZone)->mNumParts;
        for (s32 i = 0; i < count; i++) {
            CollisionParts* pParts = (*pZone)->mPartsArray[i];
            if (!pParts->_CC) {
                continue;
            }
            if (_9C == pParts->mKeeperIndex) {
                pParts->updateMtx();
            }
            if (_A1) {
                (*pZone)->calcMinMaxAndRadius();
            } else if (pParts->_D4 == 0) {
                (*pZone)->calcMinMaxAndRadiusIfMoveOuter(pParts);
            }
        }
    }
    _A1 = false;
}

TVec3f CollisionParts::getTrans() {
    TVec3f translation;
    mBaseMatrix.getTrans(translation);

    return translation;
}

void CollisionCategorizedKeeper::addToZone(CollisionParts* pParts, s32 zone) {
    mZones[zone]->addParts(pParts);
    mZoneCount++;
}

void CollisionCategorizedKeeper::removeFromZone(CollisionParts* pParts, s32 zone) {
    mZones[zone]->eraseParts(pParts);
    mZoneCount--;
}

void CollisionCategorizedKeeper::addToGlobal(CollisionParts* pParts) {
    mZones[0]->addParts(pParts);
    mZoneCount++;
}

void CollisionCategorizedKeeper::removeFromGlobal(CollisionParts* pParts) {
    mZones[0]->eraseParts(pParts);
    mZoneCount--;
}

s32 CollisionCategorizedKeeper::checkStrikePoint(const TVec3f& rPos, HitInfo* pHitInfo) {
    MR::getCollisionDirector();
    _10 = 0;

    for (CollisionZone** pZone = mZones; pZone != mZones + mZoneNum; pZone++) {
        if (pZone != mZones) {
            if (!isSphereOverlappingWithBox((*pZone)->_818, (*pZone)->_824, rPos, 0.0f)) {
                continue;
            }

            f32 range = (*pZone)->mRadius;
            TVec3f distance((*pZone)->_808);
            distance -= rPos;

            if (range * range < distance.squared()) {
                continue;
            }
        }

        s32 partCount = (*pZone)->mNumParts;

        for (s32 i = 0; i < partCount; i++) {
            CollisionParts* pParts = (*pZone)->mPartsArray[i];

            if (!pParts->_CC) {
                continue;
            }

            f32 range = pParts->_D8;
            TVec3f distance;
            distance.x = MR::abs(pParts->getTrans().x - rPos.x);

            if (range < distance.x) {
                continue;
            }

            distance.y = MR::abs(pParts->getTrans().y - rPos.y);

            if (range < distance.y) {
                continue;
            }

            distance.z = MR::abs(pParts->getTrans().z - rPos.z);

            if (range < distance.z) {
                continue;
            }

            if (distance.squared() > range * range) {
                continue;
            }

            if (pParts->checkStrikePoint(pHitInfo, rPos)) {
                _10 = 1;
                return 1;
            }
        }
    }

    return _10;
}

s32 CollisionCategorizedKeeper::checkStrikeBall(const TVec3f& rPos, f32 radius, bool movingReaction, const CollisionPartsFilterBase* pPartsFilter,
                                                const TriangleFilterBase* pTriangleFilter) {
    MR::getCollisionDirector();
    _10 = 0;
    s32 count = 0;

    for (CollisionZone** pZone = mZones; pZone != mZones + mZoneNum; pZone++) {
        if (pZone != mZones) {
            if (!isSphereOverlappingWithBox((*pZone)->_818, (*pZone)->_824, rPos, radius)) {
                continue;
            }

            f32 range = radius + (*pZone)->mRadius;
            TVec3f distance((*pZone)->_808);
            distance -= rPos;

            if (range * range < distance.squared()) {
                continue;
            }
        }

        s32 partCount = (*pZone)->mNumParts;

        for (s32 i = 0; i < partCount; i++) {
            CollisionParts* pParts = (*pZone)->mPartsArray[i];

            if (!pParts->_CC) {
                continue;
            }

            if (pPartsFilter != nullptr && pPartsFilter->isInvalidParts(pParts)) {
                continue;
            }

            f32 range = radius + pParts->_D8;
            TVec3f distance;
            distance.x = MR::abs(pParts->getTrans().x - rPos.x);

            if (range < distance.x) {
                continue;
            }

            distance.y = MR::abs(pParts->getTrans().y - rPos.y);

            if (range < distance.y) {
                continue;
            }

            distance.z = MR::abs(pParts->getTrans().z - rPos.z);

            if (range < distance.z) {
                continue;
            }

            if (distance.squared() > range * range) {
                continue;
            }

            count += pParts->checkStrikeBall(mHitInfoArray + count, 32 - count, rPos, radius, movingReaction, pTriangleFilter);

            if (count >= 32) {
                _10 = count;
                return count;
            }
        }
    }

    _10 = count;
    return count;
}

s32 CollisionCategorizedKeeper::checkStrikeBallWithThickness(const TVec3f& rPos, f32 radius, f32 thickness,
                                                             const CollisionPartsFilterBase* pPartsFilter,
                                                             const TriangleFilterBase* pTriangleFilter) {
    MR::getCollisionDirector();
    _10 = 0;
    s32 count = 0;

    for (CollisionZone** pZone = mZones; pZone != mZones + mZoneNum; pZone++) {
        if (pZone != mZones) {
            if (!isSphereOverlappingWithBox((*pZone)->_818, (*pZone)->_824, rPos, radius)) {
                continue;
            }

            f32 range = radius + (*pZone)->mRadius;
            TVec3f distance((*pZone)->_808);
            distance -= rPos;

            if (range * range < distance.squared()) {
                continue;
            }
        }

        s32 partCount = (*pZone)->mNumParts;

        for (s32 i = 0; i < partCount; i++) {
            CollisionParts* pParts = (*pZone)->mPartsArray[i];

            if (!pParts->_CC) {
                continue;
            }

            if (pPartsFilter != nullptr && pPartsFilter->isInvalidParts(pParts)) {
                continue;
            }

            f32 range = radius + pParts->_D8;
            TVec3f distance;
            distance.x = MR::abs(pParts->getTrans().x - rPos.x);

            if (range < distance.x) {
                continue;
            }

            distance.y = MR::abs(pParts->getTrans().y - rPos.y);

            if (range < distance.y) {
                continue;
            }

            distance.z = MR::abs(pParts->getTrans().z - rPos.z);

            if (range < distance.z) {
                continue;
            }

            if (distance.squared() > range * range) {
                continue;
            }

            count += pParts->checkStrikeBallWithThickness(mHitInfoArray + count, 32 - count, rPos, radius, thickness, pTriangleFilter);

            if (count >= 32) {
                _10 = count;
                return count;
            }
        }
    }

    _10 = count;
    return count;
}

s32 CollisionCategorizedKeeper::checkStrikeLine(const TVec3f& rPos, const TVec3f& rOffset, s32 capacity, const CollisionPartsFilterBase* pPartsFilter,
                                                const TriangleFilterBase* pTriangleFilter) {
    MR::getCollisionDirector();

    if (capacity == 0) {
        capacity = 32;
    }

    _10 = 0;
    s32 count = 0;
    TVec3f minimum(rPos);
    TVec3f maximum(rPos);

    if (rOffset.x < 0.0f) {
        minimum.x += rOffset.x;
    } else {
        maximum.x += rOffset.x;
    }

    if (rOffset.y < 0.0f) {
        minimum.y += rOffset.y;
    } else {
        maximum.y += rOffset.y;
    }

    if (rOffset.z < 0.0f) {
        minimum.z += rOffset.z;
    } else {
        maximum.z += rOffset.z;
    }

    for (CollisionZone** pZone = mZones; pZone != mZones + mZoneNum; pZone++) {
        if (pZone != mZones) {
            TVec3f position((*pZone)->_808);
            f32 radius = (*pZone)->mRadius;

            if (!isSphereOverlappingWithBox(minimum, maximum, position, radius)) {
                continue;
            }

            if (!MR::checkHitSegmentSphere(position, rPos, rPos + rOffset, radius, nullptr)) {
                continue;
            }
        }

        s32 partCount = (*pZone)->mNumParts;

        for (s32 i = 0; i < partCount; i++) {
            CollisionParts* pParts = (*pZone)->mPartsArray[i];

            if (!pParts->_CC) {
                continue;
            }

            if (pPartsFilter != nullptr && pPartsFilter->isInvalidParts(pParts)) {
                continue;
            }

            TVec3f position(pParts->getTrans());
            f32 radius = pParts->_D8;

            if (!isSphereOverlappingWithBox(minimum, maximum, position, radius)) {
                continue;
            }

            if (!MR::checkHitSegmentSphere(position, rPos, rPos + rOffset, radius, nullptr)) {
                continue;
            }

            count += pParts->checkStrikeLine(mHitInfoArray + count, capacity - count, rPos, rOffset, pTriangleFilter);

            if (capacity <= count) {
                _10 = count;
                return count;
            }
        }
    }

    _10 = count;
    return count;
}

u32 CollisionCategorizedKeeper::createAreaPolygonList(Triangle* pTriangles, u32 capacity, const TVec3f& rStart, const TVec3f& rEnd) {
    MR::getCollisionDirector();
    u32 count = 0;
    TVec3f minimum;
    TVec3f maximum;
    if (rStart.x < rEnd.x) {
        minimum.x = rStart.x;
        maximum.x = rEnd.x;
    } else {
        minimum.x = rEnd.x;
        maximum.x = rStart.x;
    }
    if (rStart.y < rEnd.y) {
        minimum.y = rStart.y;
        maximum.y = rEnd.y;
    } else {
        minimum.y = rEnd.y;
        maximum.y = rStart.y;
    }
    if (rStart.z < rEnd.z) {
        minimum.z = rStart.z;
        maximum.z = rEnd.z;
    } else {
        minimum.z = rEnd.z;
        maximum.z = rStart.z;
    }
    for (CollisionZone** pZone = mZones; pZone != mZones + mZoneNum; pZone++) {
        if (pZone != mZones) {
            if (!isSphereOverlappingWithBox(minimum, maximum, (*pZone)->_808, (*pZone)->mRadius)) {
                continue;
            }
        }
        s32 partCount = (*pZone)->mNumParts;
        for (s32 i = 0; i < partCount; i++) {
            CollisionParts* pParts = (*pZone)->mPartsArray[i];
            if (!pParts->_CC) {
                continue;
            }
            f32 radius = pParts->_D8;
            if (!isSphereOverlappingWithBox(minimum, maximum, pParts->getTrans(), radius)) {
                continue;
            }
            count += pParts->createAreaPolygonList(pTriangles + count, capacity - count, rStart, rEnd);
            if (capacity <= count) {
                return count;
            }
        }
    }
    return count;
}

u32 CollisionCategorizedKeeper::createAreaPolygonListArray(Triangle* pTriangles, u32 capacity, TVec3f* pPoints, u32 pointCount) {
    MR::getCollisionDirector();
    u32 count = 0;
    TVec3f minimum;
    TVec3f maximum;
    MR::createBoundingBox(pPoints, pointCount, &minimum, &maximum);
    for (CollisionZone** pZone = mZones; pZone != mZones + mZoneNum; pZone++) {
        if (pZone != mZones) {
            if (!isSphereOverlappingWithBox(minimum, maximum, (*pZone)->_808, (*pZone)->mRadius)) {
                continue;
            }
        }
        s32 partCount = (*pZone)->mNumParts;
        for (s32 i = 0; i < partCount; i++) {
            CollisionParts* pParts = (*pZone)->mPartsArray[i];
            if (!pParts->_CC) {
                continue;
            }
            f32 radius = pParts->_D8;
            if (!isSphereOverlappingWithBox(minimum, maximum, pParts->getTrans(), radius)) {
                continue;
            }
            count += pParts->createAreaPolygonListArray(pTriangles + count, capacity - count, pPoints, pointCount);
            if (capacity <= count) {
                return count;
            }
        }
    }
    return count;
}

bool CollisionCategorizedKeeper::isSphereOverlappingWithBox(const TVec3f& rMinimum, const TVec3f& rMaximum, const TVec3f& rPos, f32 radius) {
    if (rPos.x < rMinimum.x - radius || rMaximum.x + radius < rPos.x) {
        return false;
    }

    if (rPos.y < rMinimum.y - radius || rMaximum.y + radius < rPos.y) {
        return false;
    }

    if (rPos.z < rMinimum.z - radius || rMaximum.z + radius < rPos.z) {
        return false;
    }

    return true;
}

bool CollisionCategorizedKeeper::searchSameHostParts(CollisionParts** pResult, CollisionParts* pParts) const {
    for (CollisionZone* const* pZone = mZones; pZone != mZones + mZoneNum; pZone++) {
        s32 count = (*pZone)->mNumParts;
        for (s32 i = 0; i < count; i++) {
            CollisionParts* pCandidate = (*pZone)->mPartsArray[i];
            if (pCandidate->mHitSensor->mHost == pParts->mHitSensor->mHost) {
                *pResult = pCandidate;
                return true;
            }
        }
    }
    return false;
}

HitInfo* CollisionCategorizedKeeper::getStrikeInfo(u32 index) {
    return &mHitInfoArray[index];
}

CollisionZone* CollisionCategorizedKeeper::getZone(int zone) {
    if (!_A0) {
        s32 count = MR::getZoneNum();
        for (s32 i = 0; i < count; i++) {
            CollisionZone* pZone = new CollisionZone(i);
            s32 index = mZoneNum++;
            mZones[index] = pZone;
        }
        _A0 = true;
    }
    return mZones[zone];
}

CollisionZone::CollisionZone(s32 zoneID) : mZoneID(zoneID), mNumParts(0), _808(0, 0, 0), mRadius(0.0f), _818(0, 0, 0), _824(0, 0, 0) {
}

void CollisionZone::addParts(CollisionParts* pParts) {
    s32 cnt = mNumParts;
    mNumParts++;
    mPartsArray[cnt] = pParts;

    if (mZoneID) {
        calcMinMaxAndRadius();
    }
}

void CollisionZone::calcMinMaxAndRadius() {
    _818.zero();
    _824.zero();
    mRadius = 0.0f;

    for (CollisionParts** pParts = mPartsArray; pParts != mPartsArray + mNumParts; pParts++) {
        TVec3f minimum((*pParts)->getTrans());
        TVec3f maximum((*pParts)->getTrans());
        f32 radius = (*pParts)->_D8;
        minimum -= TVec3f(radius, radius, radius);
        maximum += TVec3f(radius, radius, radius);
        addAndUpdateMinMax(minimum, maximum);
    }

    _808 = (_824 + _818) * 0.5f;
    f32 radius = 0.0f;
    for (CollisionParts** pParts = mPartsArray; pParts != mPartsArray + mNumParts; pParts++) {
        TVec3f distance((*pParts)->getTrans());
        distance -= _808;
        f32 extent = distance.length();
        extent += (*pParts)->_D8;
        if (radius < extent) {
            radius = extent;
        }
    }
    mRadius = radius;
}

void CollisionZone::calcMinMaxAndRadiusIfMoveOuter(CollisionParts* pParts) {
    f32 radius = pParts->_D8;
    TVec3f position = pParts->getTrans();
    TVec3f minimum = _818;
    TVec3f maximum = _824;
    minimum.x += radius;
    minimum.y += radius;
    minimum.z += radius;
    maximum.x -= radius;
    maximum.y -= radius;
    maximum.z -= radius;
    if (!MR::isInRange(position.x, minimum.x, maximum.x) || !MR::isInRange(position.y, minimum.y, maximum.y) ||
        !MR::isInRange(position.z, minimum.z, maximum.z)) {
        calcMinMaxAndRadius();
    }
}

void CollisionZone::addAndUpdateMinMax(TVec3f minimum, TVec3f maximum) {
    if (mRadius == 0.0f) {
        mRadius = 0.1f;
        _818.set(minimum);
        _824.set(maximum);
    } else {
        if (minimum.x < _818.x) {
            _818.x = minimum.x;
        }
        if (minimum.y < _818.y) {
            _818.y = minimum.y;
        }
        if (minimum.z < _818.z) {
            _818.z = minimum.z;
        }
        if (_824.x < maximum.x) {
            _824.x = maximum.x;
        }
        if (_824.y < maximum.y) {
            _824.y = maximum.y;
        }
        if (_824.z < maximum.z) {
            _824.z = maximum.z;
        }
    }
}

void CollisionZone::eraseParts(CollisionParts* pParts) {
    CollisionParts** pEnd = mPartsArray + mNumParts;
    CollisionParts** pFound = std::find(mPartsArray, pEnd, pParts);
    if (pFound == pEnd) {
        return;
    }

    mPartsArray[pFound - mPartsArray] = mPartsArray[mNumParts - 1];
    mNumParts--;
}
