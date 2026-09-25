#include "Game/Map/CollisionCategorizedKeeper.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Map/CollisionDirector.hpp"
#include "Game/Map/CollisionParts.hpp"
#include "Game/Util/CollisionPartsFilter.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "compat/Cp932Literal.hpp"
#include <algorithm>

CollisionCategorizedKeeper::CollisionCategorizedKeeper(s32 category)
    : NameObj(CP932("地形コリジョンカテゴリキーパー")), mHitInfoArray(nullptr), _10(0), mZoneCount(0), mZones{}, mZoneNum(0), _9C(category),
      _A0(false), _A1(true) {
    mHitInfoArray = new HitInfo[32];
}

CollisionCategorizedKeeper::~CollisionCategorizedKeeper() {
    for (s32 i = 0; i < mZoneNum; i++)
        delete mZones[i];
    delete[] mHitInfoArray;
}

void CollisionCategorizedKeeper::requireNativeGeometryPublished() const {
    // Check every enabled original part before spatial culling. A malformed
    // mutable source quarantines only the category that actually owns it.
    for (s32 zone = 0; zone < mZoneNum; ++zone)
        for (s32 part = 0; part < mZones[zone]->mNumParts; ++part)
            mZones[zone]->mPartsArray[part]->requireNativeGeometryPublished();
}

CollisionZone* CollisionCategorizedKeeper::getZone(int zoneID) {
    if (!_A0) {
        s32 zoneCount = MR::getZoneNum();
        for (s32 i = 0; i < zoneCount; i++) {
            CollisionZone* zone = new CollisionZone(i);
            mZones[mZoneNum++] = zone;
        }
        _A0 = true;
    }
    return mZones[zoneID];
}

TVec3f CollisionParts::getTrans() {
    TVec3f translation;
    mBaseMatrix.getTrans(translation);

    return translation;
}

void CollisionCategorizedKeeper::movement() {
    for (CollisionZone** zone = mZones; zone != mZones + mZoneNum; zone++) {
        s32 partCount = (*zone)->mNumParts;
        for (s32 i = 0; i < partCount; i++) {
            CollisionParts* part = (*zone)->mPartsArray[i];
            if (!part->_CC) {
                continue;
            }
            if (_9C == part->mKeeperIndex) {
                part->updateMtx();
            }
            if (_A1) {
                (*zone)->calcMinMaxAndRadius();
            } else if (part->_D4 == 0) {
                (*zone)->calcMinMaxAndRadiusIfMoveOuter(part);
            }
        }
    }
    _A1 = false;
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
    requireNativeGeometryPublished();
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
    requireNativeGeometryPublished();
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
    requireNativeGeometryPublished();
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

s32 CollisionCategorizedKeeper::checkStrikeLine(const TVec3f& rStart, const TVec3f& rOffset, s32 maxCount,
                                                const CollisionPartsFilterBase* pPartsFilter, const TriangleFilterBase* pTriangleFilter) {
    requireNativeGeometryPublished();
    MR::getCollisionDirector();

    if (maxCount == 0) {
        maxCount = 32;
    }

    _10 = 0;
    s32 hitCount = 0;
    TVec3f boxMin(rStart);
    TVec3f boxMax(rStart);

    if (rOffset.x < 0.0f) {
        boxMin.x += rOffset.x;
    } else {
        boxMax.x += rOffset.x;
    }
    if (rOffset.y < 0.0f) {
        boxMin.y += rOffset.y;
    } else {
        boxMax.y += rOffset.y;
    }
    if (rOffset.z < 0.0f) {
        boxMin.z += rOffset.z;
    } else {
        boxMax.z += rOffset.z;
    }

    for (CollisionZone** zone = mZones; zone != mZones + mZoneNum; zone++) {
        if (zone != mZones) {
            TVec3f center((*zone)->_808);
            f32 radius = (*zone)->mRadius;
            if (!isSphereOverlappingWithBox(boxMin, boxMax, center, radius)) {
                continue;
            }
            if (!MR::checkHitSegmentSphere(center, rStart, rStart + rOffset, radius, nullptr)) {
                continue;
            }
        }

        s32 partCount = (*zone)->mNumParts;
        for (s32 i = 0; i < partCount; i++) {
            CollisionParts* part = (*zone)->mPartsArray[i];
            if (!part->_CC) {
                continue;
            }
            if (pPartsFilter != nullptr && pPartsFilter->isInvalidParts(part)) {
                continue;
            }

            TVec3f center(part->getTrans());
            f32 radius = part->_D8;
            if (!isSphereOverlappingWithBox(boxMin, boxMax, center, radius)) {
                continue;
            }
            if (!MR::checkHitSegmentSphere(center, rStart, rStart + rOffset, radius, nullptr)) {
                continue;
            }

            hitCount += part->checkStrikeLine(mHitInfoArray + hitCount, maxCount - hitCount, rStart, rOffset, pTriangleFilter);
            if (maxCount <= hitCount) {
                _10 = hitCount;
                return hitCount;
            }
        }
    }

    _10 = hitCount;
    return hitCount;
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

u32 CollisionCategorizedKeeper::createAreaPolygonList(Triangle* pTriangles, u32 capacity, const TVec3f& rStart, const TVec3f& rEnd) {
    requireNativeGeometryPublished();
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
    requireNativeGeometryPublished();
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

bool CollisionCategorizedKeeper::isSphereOverlappingWithBox(const TVec3f& rMin, const TVec3f& rMax, const TVec3f& rCenter, f32 radius) {
    if (rCenter.x < rMin.x - radius || rMax.x + radius < rCenter.x) {
        return false;
    }

    if (rCenter.y < rMin.y - radius || rMax.y + radius < rCenter.y) {
        return false;
    }

    if (rCenter.z < rMin.z - radius || rMax.z + radius < rCenter.z) {
        return false;
    }

    return true;
}

bool CollisionCategorizedKeeper::searchSameHostParts(CollisionParts** ppParts, CollisionParts* pParts) const {
    for (CollisionZone* const* zone = mZones; zone != mZones + mZoneNum; zone++) {
        s32 partCount = (*zone)->mNumParts;
        for (s32 i = 0; i < partCount; i++) {
            CollisionParts* part = (*zone)->mPartsArray[i];
            if (part->mHitSensor->mHost == pParts->mHitSensor->mHost) {
                *ppParts = part;
                return true;
            }
        }
    }

    return false;
}

HitInfo* CollisionCategorizedKeeper::getStrikeInfo(u32 index) {
    return &mHitInfoArray[index];
}
