#include "Game/Map/CollisionCategorizedKeeper.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Map/CollisionParts.hpp"
#include "Game/Map/CollisionDirector.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include <algorithm>

CollisionCategorizedKeeper::CollisionCategorizedKeeper(s32 category)
    : NameObj("地形コリジョンカテゴリキーパー"), mHitInfoArray(nullptr), _10(0), mZoneCount(0), mZoneNum(0), _9C(category), _A0(false), _A1(true) {
    mHitInfoArray = new HitInfo[32];
}

CollisionCategorizedKeeper::~CollisionCategorizedKeeper() {
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

u32 CollisionCategorizedKeeper::createAreaPolygonListArray(Triangle* pTriangles, u32 maxCount, TVec3f* pPoints, u32 pointCount) {
    MR::getCollisionDirector();

    TVec3f boxMin;
    TVec3f boxMax;
    u32 foundCount = 0;
    MR::createBoundingBox(pPoints, pointCount, &boxMin, &boxMax);

    for (CollisionZone** zone = mZones; zone != mZones + mZoneNum; zone++) {
        if (zone != mZones && !isSphereOverlappingWithBox(boxMin, boxMax, (*zone)->_808, (*zone)->mRadius)) {
            continue;
        }

        s32 partCount = (*zone)->mNumParts;

        for (s32 i = 0; i < partCount; i++) {
            CollisionParts* part = (*zone)->mPartsArray[i];

            if (!part->_CC) {
                continue;
            }

            if (!isSphereOverlappingWithBox(boxMin, boxMax, part->getTrans(), part->_D8)) {
                continue;
            }

            foundCount += part->createAreaPolygonListArray(pTriangles + foundCount, maxCount - foundCount, pPoints, pointCount);

            if (maxCount <= foundCount) {
                return foundCount;
            }
        }
    }

    return foundCount;
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
