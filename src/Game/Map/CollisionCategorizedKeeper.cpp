#include "Game/Map/CollisionCategorizedKeeper.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/CollisionParts.hpp"
#include "Game/Util/CollisionPartsFilter.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/SceneUtil.hpp"

namespace {
    const s32 cHitInfoMax = 32;

    bool isGlobalZone(const CollisionCategorizedKeeper* pKeeper, const CollisionZone* pZone) {
        return pZone == pKeeper->mZones[0];
    }

    bool isPartsActive(const CollisionParts* pParts) {
        return pParts != nullptr && pParts->_CC;
    }

    bool isPartsInvalid(const CollisionPartsFilterBase* pFilter, const CollisionParts* pParts) {
        return pFilter != nullptr && pFilter->isInvalidParts(pParts);
    }

    bool isPartsNearPoint(CollisionParts* pParts, const TVec3f& rPoint, f32 radius) {
        TVec3f diff = pParts->getTrans();
        diff.sub(rPoint);

        return diff.squared() <= radius * radius;
    }

    bool isZoneNearSphere(CollisionCategorizedKeeper* pKeeper, const CollisionZone* pZone, const TVec3f& rCenter, f32 radius) {
        if (isGlobalZone(pKeeper, pZone)) {
            return true;
        }

        if (!pKeeper->isSphereOverlappingWithBox(pZone->_818, pZone->_824, rCenter, radius)) {
            return false;
        }

        TVec3f diff = pZone->_808;
        diff.sub(rCenter);
        const f32 totalRadius = radius + pZone->mRadius;

        return diff.squared() <= totalRadius * totalRadius;
    }

    bool isPartsNearSphere(CollisionParts* pParts, const TVec3f& rCenter, f32 radius) {
        return isPartsNearPoint(pParts, rCenter, radius + pParts->_D8);
    }

    void makeLineBounds(TVec3f* pMin, TVec3f* pMax, const TVec3f& rStart, const TVec3f& rOffset) {
        *pMin = rStart;
        *pMax = rStart;

        if (rOffset.x < 0.0f) {
            pMin->x += rOffset.x;
        } else {
            pMax->x += rOffset.x;
        }

        if (rOffset.y < 0.0f) {
            pMin->y += rOffset.y;
        } else {
            pMax->y += rOffset.y;
        }

        if (rOffset.z < 0.0f) {
            pMin->z += rOffset.z;
        } else {
            pMax->z += rOffset.z;
        }
    }

    TVec3f getLineEnd(const TVec3f& rStart, const TVec3f& rOffset) {
        TVec3f end = rStart;
        end.add(rOffset);
        return end;
    }

    bool isZoneNearLine(CollisionCategorizedKeeper* pKeeper, const CollisionZone* pZone, const TVec3f& rStart, const TVec3f& rOffset,
                        const TVec3f& rMin, const TVec3f& rMax) {
        if (isGlobalZone(pKeeper, pZone)) {
            return true;
        }

        if (!pKeeper->isSphereOverlappingWithBox(rMin, rMax, pZone->_808, pZone->mRadius)) {
            return false;
        }

        const TVec3f end = getLineEnd(rStart, rOffset);
        return MR::checkHitSegmentSphere(pZone->_808, rStart, end, pZone->mRadius, nullptr);
    }

    bool isPartsNearLine(CollisionCategorizedKeeper* pKeeper, CollisionParts* pParts, const TVec3f& rStart, const TVec3f& rOffset,
                         const TVec3f& rMin, const TVec3f& rMax) {
        const TVec3f trans = pParts->getTrans();

        if (!pKeeper->isSphereOverlappingWithBox(rMin, rMax, trans, pParts->_D8)) {
            return false;
        }

        const TVec3f end = getLineEnd(rStart, rOffset);
        return MR::checkHitSegmentSphere(trans, rStart, end, pParts->_D8, nullptr);
    }

    void makeAreaBounds(TVec3f* pMin, TVec3f* pMax, const TVec3f& rA, const TVec3f& rB) {
        pMin->x = rA.x < rB.x ? rA.x : rB.x;
        pMin->y = rA.y < rB.y ? rA.y : rB.y;
        pMin->z = rA.z < rB.z ? rA.z : rB.z;

        pMax->x = rA.x < rB.x ? rB.x : rA.x;
        pMax->y = rA.y < rB.y ? rB.y : rA.y;
        pMax->z = rA.z < rB.z ? rB.z : rA.z;
    }
};

CollisionCategorizedKeeper::CollisionCategorizedKeeper(s32 keeperIndex)
    : NameObj("CollisionCategorizedKeeper"), mHitInfoArray(nullptr), _10(0), mZoneCount(0), mZoneNum(0), _9C(keeperIndex), _A0(false), _A1(true),
      _A2(false), _A3(false) {
    for (s32 i = 0; i < 0x20; i++) {
        mZones[i] = nullptr;
    }

    mHitInfoArray = new HitInfo[cHitInfoMax];
}

CollisionCategorizedKeeper::~CollisionCategorizedKeeper() {
    delete[] mHitInfoArray;

    for (s32 i = 0; i < mZoneNum; i++) {
        delete mZones[i];
        mZones[i] = nullptr;
    }
}

void CollisionCategorizedKeeper::movement() {
    for (s32 zoneIdx = 0; zoneIdx < mZoneNum; zoneIdx++) {
        CollisionZone* zone = mZones[zoneIdx];

        for (s32 partsIdx = 0; partsIdx < zone->mNumParts; partsIdx++) {
            CollisionParts* parts = zone->mPartsArray[partsIdx];

            if (!isPartsActive(parts)) {
                continue;
            }

            if (_9C == static_cast< u32 >(parts->mKeeperIndex)) {
                parts->updateMtx();
            }

            if (_A1) {
                zone->calcMinMaxAndRadius();
            } else if (parts->_D4 == 0) {
                zone->calcMinMaxAndRadiusIfMoveOuter(parts);
            }
        }
    }

    _A1 = false;
}

TVec3f CollisionParts::getTrans() {
    TVec3f translation;
    mBaseMatrix.getTransInline(translation);

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

s32 CollisionCategorizedKeeper::checkStrikePoint(const TVec3f& rPoint, HitInfo* pHitInfo) {
    _10 = 0;

    for (s32 zoneIdx = 0; zoneIdx < mZoneNum; zoneIdx++) {
        CollisionZone* zone = mZones[zoneIdx];

        if (!isZoneNearSphere(this, zone, rPoint, 0.0f)) {
            continue;
        }

        for (s32 partsIdx = 0; partsIdx < zone->mNumParts; partsIdx++) {
            CollisionParts* parts = zone->mPartsArray[partsIdx];

            if (!isPartsActive(parts) || !isPartsNearPoint(parts, rPoint, parts->_D8)) {
                continue;
            }

            if (parts->checkStrikePoint(pHitInfo, rPoint)) {
                _10 = 1;
                return 1;
            }
        }
    }

    return _10;
}

s32 CollisionCategorizedKeeper::checkStrikeBall(const TVec3f& rCenter, f32 radius, bool movingReaction, const CollisionPartsFilterBase* pPartsFilter,
                                                const TriangleFilterBase* pTriangleFilter) {
    _10 = 0;

    for (s32 zoneIdx = 0; zoneIdx < mZoneNum; zoneIdx++) {
        CollisionZone* zone = mZones[zoneIdx];

        if (!isZoneNearSphere(this, zone, rCenter, radius)) {
            continue;
        }

        for (s32 partsIdx = 0; partsIdx < zone->mNumParts; partsIdx++) {
            CollisionParts* parts = zone->mPartsArray[partsIdx];

            if (!isPartsActive(parts) || isPartsInvalid(pPartsFilter, parts) || !isPartsNearSphere(parts, rCenter, radius)) {
                continue;
            }

            const u32 hitCount =
                parts->checkStrikeBall(&mHitInfoArray[_10], cHitInfoMax - _10, rCenter, radius, movingReaction, pTriangleFilter);
            _10 += hitCount;

            if (_10 >= cHitInfoMax) {
                return _10;
            }
        }
    }

    return _10;
}

s32 CollisionCategorizedKeeper::checkStrikeBallWithThickness(const TVec3f& rCenter, f32 radius, f32 thickness,
                                                             const CollisionPartsFilterBase* pPartsFilter,
                                                             const TriangleFilterBase* pTriangleFilter) {
    _10 = 0;

    for (s32 zoneIdx = 0; zoneIdx < mZoneNum; zoneIdx++) {
        CollisionZone* zone = mZones[zoneIdx];

        if (!isZoneNearSphere(this, zone, rCenter, radius)) {
            continue;
        }

        for (s32 partsIdx = 0; partsIdx < zone->mNumParts; partsIdx++) {
            CollisionParts* parts = zone->mPartsArray[partsIdx];

            if (!isPartsActive(parts) || isPartsInvalid(pPartsFilter, parts) || !isPartsNearSphere(parts, rCenter, radius)) {
                continue;
            }

            const u32 hitCount =
                parts->checkStrikeBallWithThickness(&mHitInfoArray[_10], cHitInfoMax - _10, rCenter, radius, thickness, pTriangleFilter);
            _10 += hitCount;

            if (_10 >= cHitInfoMax) {
                return _10;
            }
        }
    }

    return _10;
}

s32 CollisionCategorizedKeeper::checkStrikeLine(const TVec3f& rStart, const TVec3f& rOffset, s32 maxHits,
                                                const CollisionPartsFilterBase* pPartsFilter, const TriangleFilterBase* pTriangleFilter) {
    if (maxHits == 0) {
        maxHits = cHitInfoMax;
    }

    _10 = 0;

    TVec3f min;
    TVec3f max;
    makeLineBounds(&min, &max, rStart, rOffset);

    for (s32 zoneIdx = 0; zoneIdx < mZoneNum; zoneIdx++) {
        CollisionZone* zone = mZones[zoneIdx];

        if (!isZoneNearLine(this, zone, rStart, rOffset, min, max)) {
            continue;
        }

        for (s32 partsIdx = 0; partsIdx < zone->mNumParts; partsIdx++) {
            CollisionParts* parts = zone->mPartsArray[partsIdx];

            if (!isPartsActive(parts) || isPartsInvalid(pPartsFilter, parts) || !isPartsNearLine(this, parts, rStart, rOffset, min, max)) {
                continue;
            }

            const u32 hitCount = parts->checkStrikeLine(&mHitInfoArray[_10], maxHits - _10, rStart, rOffset, pTriangleFilter);
            _10 += hitCount;

            if (_10 >= maxHits) {
                return _10;
            }
        }
    }

    return _10;
}

u32 CollisionCategorizedKeeper::createAreaPolygonList(Triangle* pTriangles, u32 maxTriangles, const TVec3f& rA, const TVec3f& rB) {
    u32 triangleCount = 0;

    TVec3f min;
    TVec3f max;
    makeAreaBounds(&min, &max, rA, rB);

    for (s32 zoneIdx = 0; zoneIdx < mZoneNum; zoneIdx++) {
        CollisionZone* zone = mZones[zoneIdx];

        if (!isGlobalZone(this, zone) && !isSphereOverlappingWithBox(min, max, zone->_808, zone->mRadius)) {
            continue;
        }

        for (s32 partsIdx = 0; partsIdx < zone->mNumParts; partsIdx++) {
            CollisionParts* parts = zone->mPartsArray[partsIdx];

            if (!isPartsActive(parts) || !isSphereOverlappingWithBox(min, max, parts->getTrans(), parts->_D8)) {
                continue;
            }

            triangleCount += parts->createAreaPolygonList(&pTriangles[triangleCount], maxTriangles - triangleCount, rA, rB);

            if (triangleCount >= maxTriangles) {
                return triangleCount;
            }
        }
    }

    return triangleCount;
}

u32 CollisionCategorizedKeeper::createAreaPolygonListArray(Triangle* pTriangles, u32 maxTriangles, TVec3f* pPoints, u32 pointCount) {
    u32 triangleCount = 0;

    TVec3f min;
    TVec3f max;
    MR::createBoundingBox(pPoints, pointCount, &min, &max);

    for (s32 zoneIdx = 0; zoneIdx < mZoneNum; zoneIdx++) {
        CollisionZone* zone = mZones[zoneIdx];

        if (!isGlobalZone(this, zone) && !isSphereOverlappingWithBox(min, max, zone->_808, zone->mRadius)) {
            continue;
        }

        for (s32 partsIdx = 0; partsIdx < zone->mNumParts; partsIdx++) {
            CollisionParts* parts = zone->mPartsArray[partsIdx];

            if (!isPartsActive(parts) || !isSphereOverlappingWithBox(min, max, parts->getTrans(), parts->_D8)) {
                continue;
            }

            triangleCount += parts->createAreaPolygonListArray(&pTriangles[triangleCount], maxTriangles - triangleCount, pPoints, pointCount);

            if (triangleCount >= maxTriangles) {
                return triangleCount;
            }
        }
    }

    return triangleCount;
}

bool CollisionCategorizedKeeper::isSphereOverlappingWithBox(const TVec3f& rMin, const TVec3f& rMax, const TVec3f& rCenter, f32 radius) {
    return rMin.x - radius <= rCenter.x && rCenter.x <= rMax.x + radius && rMin.y - radius <= rCenter.y && rCenter.y <= rMax.y + radius &&
           rMin.z - radius <= rCenter.z && rCenter.z <= rMax.z + radius;
}

bool CollisionCategorizedKeeper::searchSameHostParts(CollisionParts** ppParts, CollisionParts* pParts) const {
    if (pParts == nullptr || pParts->mHitSensor == nullptr) {
        return false;
    }

    LiveActor* host = pParts->mHitSensor->mHost;

    for (s32 zoneIdx = 0; zoneIdx < mZoneNum; zoneIdx++) {
        CollisionZone* zone = mZones[zoneIdx];

        for (s32 partsIdx = 0; partsIdx < zone->mNumParts; partsIdx++) {
            CollisionParts* other = zone->mPartsArray[partsIdx];

            if (other != nullptr && other->mHitSensor != nullptr && other->mHitSensor->mHost == host) {
                *ppParts = other;
                return true;
            }
        }
    }

    return false;
}

HitInfo* CollisionCategorizedKeeper::getStrikeInfo(u32 index) {
    return &mHitInfoArray[index];
}

CollisionZone* CollisionCategorizedKeeper::getZone(int zoneID) {
    if (!_A0) {
        const s32 zoneNum = MR::getZoneNum();

        for (s32 i = 0; i < zoneNum; i++) {
            mZones[mZoneNum] = new CollisionZone(i);
            mZoneNum++;
        }

        _A0 = true;
    }

    return mZones[zoneID];
}

CollisionZone::CollisionZone(s32 zoneID) : mZoneID(zoneID), mNumParts(0), _808(0, 0, 0), mRadius(0.0f), _818(0, 0, 0), _824(0, 0, 0) {}

void CollisionZone::addParts(CollisionParts* pParts) {
    s32 cnt = mNumParts;
    mNumParts++;
    mPartsArray[cnt] = pParts;

    if (mZoneID) {
        calcMinMaxAndRadius();
    }
}

void CollisionZone::calcMinMaxAndRadius() {
    _808.zero();
    _818.zero();
    _824.zero();
    mRadius = 0.0f;

    if (mNumParts == 0) {
        return;
    }

    for (s32 i = 0; i < mNumParts; i++) {
        CollisionParts* parts = mPartsArray[i];
        const f32 radius = parts->_D8;
        TVec3f min = parts->getTrans();
        TVec3f max = min;

        min.x -= radius;
        min.y -= radius;
        min.z -= radius;
        max.x += radius;
        max.y += radius;
        max.z += radius;

        addAndUpdateMinMax(min, max);
    }

    _808.x = (_818.x + _824.x) * 0.5f;
    _808.y = (_818.y + _824.y) * 0.5f;
    _808.z = (_818.z + _824.z) * 0.5f;

    for (s32 i = 0; i < mNumParts; i++) {
        CollisionParts* parts = mPartsArray[i];
        TVec3f diff = parts->getTrans();
        diff.sub(_808);

        const f32 radius = diff.length() + parts->_D8;

        if (mRadius < radius) {
            mRadius = radius;
        }
    }
}

void CollisionZone::calcMinMaxAndRadiusIfMoveOuter(CollisionParts* pParts) {
    const f32 radius = pParts->_D8;
    const TVec3f trans = pParts->getTrans();

    if (!MR::isInRange(trans.x, _824.x + radius, _818.x - radius) || !MR::isInRange(trans.y, _824.y + radius, _818.y - radius) ||
        !MR::isInRange(trans.z, _824.z + radius, _818.z - radius)) {
        calcMinMaxAndRadius();
    }
}

void CollisionZone::addAndUpdateMinMax(TVec3f min, TVec3f max) {
    if (mRadius == 0.0f) {
        mRadius = 0.1f;
        _818.set(min);
        _824.set(max);
        return;
    }

    if (min.x < _818.x) {
        _818.x = min.x;
    }

    if (min.y < _818.y) {
        _818.y = min.y;
    }

    if (min.z < _818.z) {
        _818.z = min.z;
    }

    if (_824.x < max.x) {
        _824.x = max.x;
    }

    if (_824.y < max.y) {
        _824.y = max.y;
    }

    if (_824.z < max.z) {
        _824.z = max.z;
    }
}

void CollisionZone::eraseParts(CollisionParts* pParts) {
    for (s32 i = 0; i < mNumParts; i++) {
        if (mPartsArray[i] != pParts) {
            continue;
        }

        mPartsArray[i] = mPartsArray[mNumParts - 1];
        mNumParts--;
        return;
    }
}
