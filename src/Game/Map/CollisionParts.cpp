#include "Game/Map/CollisionParts.hpp"
#include "Game/Boss/BossStinkBug.hpp"
#include "Game/Camera/CameraPolygonCodeUtil.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/CollisionCategorizedKeeper.hpp"
#include "Game/Map/CollisionDirector.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Map/KCollision.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/TriangleFilter.hpp"

namespace {
    static const u32 cQueryMax = 128;

    Fxyz makeFxyz(const TVec3f& rVec) {
        Fxyz out;
        out.x = rVec.x;
        out.y = rVec.y;
        out.z = rVec.z;
        return out;
    }

    f32 dot3(const TVec3f& rA, const TVec3f& rB) {
        return rA.x * rB.x + rA.y * rB.y + rA.z * rB.z;
    }

    f32 averageScale(const TPos3f& rMtx) {
        TVec3f scale;
        rMtx.getScale(scale);

        const f32 average = (scale.x + scale.y + scale.z) / 3.0f;
        return MR::isNearZero(average) ? 1.0f : average;
    }

    f32 toWorldDistance(f32 localDistance, f32 localScale) {
        return MR::isNearZero(localScale) ? localDistance : localDistance / localScale;
    }

    TVec3f addScaled(const TVec3f& rBase, const TVec3f& rVec, f32 scale) {
        return TVec3f(rBase.x + rVec.x * scale, rBase.y + rVec.y * scale, rBase.z + rVec.z * scale);
    }

    TVec3f closestPointOnTriangle(const TVec3f& rPoint, const TVec3f& rA, const TVec3f& rB, const TVec3f& rC) {
        TVec3f ab = rB - rA;
        TVec3f ac = rC - rA;
        TVec3f ap = rPoint - rA;
        f32 d1 = dot3(ab, ap);
        f32 d2 = dot3(ac, ap);

        if (d1 <= 0.0f && d2 <= 0.0f) {
            return rA;
        }

        TVec3f bp = rPoint - rB;
        f32 d3 = dot3(ab, bp);
        f32 d4 = dot3(ac, bp);

        if (d3 >= 0.0f && d4 <= d3) {
            return rB;
        }

        f32 vc = d1 * d4 - d3 * d2;
        if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
            return addScaled(rA, ab, d1 / (d1 - d3));
        }

        TVec3f cp = rPoint - rC;
        f32 d5 = dot3(ab, cp);
        f32 d6 = dot3(ac, cp);

        if (d6 >= 0.0f && d5 <= d6) {
            return rC;
        }

        f32 vb = d5 * d2 - d1 * d6;
        if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
            return addScaled(rA, ac, d2 / (d2 - d6));
        }

        f32 va = d3 * d6 - d5 * d4;
        if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
            TVec3f bc = rC - rB;
            return addScaled(rB, bc, (d4 - d3) / ((d4 - d3) + (d5 - d6)));
        }

        const f32 denom = 1.0f / (va + vb + vc);
        const f32 v = vb * denom;
        const f32 w = vc * denom;
        return TVec3f(rA.x + ab.x * v + ac.x * w, rA.y + ab.y * v + ac.y * w, rA.z + ab.z * v + ac.z * w);
    }

    TVec3f calcLocalHitPosition(KCollisionServer* pServer, const KC_PrismData* pPrism, const TVec3f& rLocalCenter, u8 hitCode) {
        TVec3f pos0 = pServer->getPos(pPrism, 0);
        TVec3f pos1 = pServer->getPos(pPrism, 1);
        TVec3f pos2 = pServer->getPos(pPrism, 2);

        switch (hitCode) {
        case 5:
            return pos0;
        case 6:
            return pos1;
        case 7:
            return pos2;
        default:
            return closestPointOnTriangle(rLocalCenter, pos0, pos1, pos2);
        }
    }

    bool fillTriangle(CollisionParts* pParts, Triangle* pTriangle, KC_PrismData* pPrism, const TriangleFilterBase* pFilter) {
        pTriangle->fillData(pParts, pParts->mServer->toIndex(pPrism), pParts->mHitSensor);
        return pFilter == nullptr || !pFilter->isInvalidTriangle(pTriangle);
    }

    void fillHitInfo(CollisionParts* pParts, HitInfo* pInfo, KC_PrismData* pPrism, const TVec3f& rWorldCenter, const TVec3f& rLocalHitPos,
                     f32 distance, u8 hitCode, const TVec3f* pReaction) {
        if (pInfo == nullptr) {
            return;
        }

        pInfo->mParentTriangle.fillData(pParts, pParts->mServer->toIndex(pPrism), pParts->mHitSensor);
        pInfo->_60 = distance;
        pParts->mBaseMatrix.mult(rLocalHitPos, pInfo->mHitPos);
        pInfo->_70 = rWorldCenter;
        pInfo->_70.sub(pInfo->mHitPos);
        pInfo->_7C.zero();

        if (pReaction != nullptr) {
            pInfo->_7C = *pReaction;
        }

        pInfo->_88 = hitCode;
    }

    u32 clampQueryCount(u32 count) {
        return count < cQueryMax ? count : cQueryMax;
    }

    void updateMinMax(TVec3f* pMin, TVec3f* pMax, const TVec3f& rPoint) {
        if (rPoint.x < pMin->x) {
            pMin->x = rPoint.x;
        }

        if (rPoint.y < pMin->y) {
            pMin->y = rPoint.y;
        }

        if (rPoint.z < pMin->z) {
            pMin->z = rPoint.z;
        }

        if (pMax->x < rPoint.x) {
            pMax->x = rPoint.x;
        }

        if (pMax->y < rPoint.y) {
            pMax->y = rPoint.y;
        }

        if (pMax->z < rPoint.z) {
            pMax->z = rPoint.z;
        }
    }
};  // namespace

CollisionParts::CollisionParts() {
    _0 = nullptr;
    mHitSensor = nullptr;
    _CC = false;
    _CD = true;
    _CE = false;
    _CF = false;
    _D0 = false;
    _D4 = 0;
    _D8 = -1.0f;
    _DC = 1.0f;
    mKeeperIndex = -1;
    mZone = nullptr;

    mServer = new KCollisionServer();

    mPrevBaseMatrix.identity();
    mBaseMatrix.identity();
    mMatrix.identity();
    PSMTXInverse(reinterpret_cast< MtxPtr >(&mBaseMatrix), reinterpret_cast< MtxPtr >(&mInvBaseMatrix));
}

void CollisionParts::init(const TPos3f& a1, HitSensor* pHitSensor, const void* pKclData, const void* pMapInfo, s32 keeperIndex, bool a6) {
    mServer->init(const_cast< void* >(pKclData), pMapInfo);
    mHitSensor = pHitSensor;

    resetAllMtx(a1);

    TVec3f scale;
    mBaseMatrix.getScale(scale);

    CollisionDirector* director = MR::getCollisionDirector();
    CollisionCategorizedKeeper* keeper = director->mKeepers[keeperIndex];
    s32 zoneID = MR::getCurrentPlacementZoneId();

    mZone = keeper->getZone(zoneID);

    MR::initCameraCodeCollection(pHitSensor->mHost->mName, mZone->mZoneID);
    mServer->calcFarthestVertexDistance();
    MR::termCameraCodeCollection();

    updateBoundingSphereRange(scale);
    mKeeperIndex = keeperIndex;
}

void CollisionParts::addToBelongZone() {
    s32 index = mKeeperIndex;
    s32 zoneID = mZone->mZoneID;

    CollisionDirector* director = MR::getCollisionDirector();
    director->mKeepers[index]->addToZone(this, zoneID);
}

void CollisionParts::removeFromBelongZone() {
    s32 index = mKeeperIndex;
    s32 zoneID = mZone->mZoneID;

    CollisionDirector* director = MR::getCollisionDirector();
    director->mKeepers[index]->removeFromZone(this, zoneID);
}

void CollisionParts::initWithAutoEqualScale(const TPos3f& a1, HitSensor* pHitSensor, const void* pKclData, const void* pMapInfo, s32 keeperIndex,
                                            bool a6) {
    _CF = true;
    _D0 = false;

    init(a1, pHitSensor, pKclData, pMapInfo, keeperIndex, a6);
}

void CollisionParts::initWithNotUsingScale(const TPos3f& a1, HitSensor* pHitSensor, const void* pKclData, const void* pMapInfo, s32 keeperIndex,
                                           bool a6) {
    _CF = false;
    _D0 = true;

    init(a1, pHitSensor, pKclData, pMapInfo, keeperIndex, a6);
}

void CollisionParts::resetAllMtx(const TPos3f& a1) {
    bool reset = false;

    if (_CD || _CE) {
        reset = true;
    }

    if (!reset) {
        return;
    }

    resetAllMtxPrivate(a1);
}

void CollisionParts::resetAllMtx() {
    bool reset = false;

    if (_CD || _CE) {
        reset = true;
    }

    if (reset) {
        TPos3f matrix;
        JMath::gekko_ps_copy12(&matrix, _0);
        makeEqualScale(reinterpret_cast< MtxPtr >(&matrix));

        resetAllMtxPrivate(matrix);
    }
}

void CollisionParts::forceResetAllMtxAndSetUpdateMtxOneTime() {
    TPos3f matrix;
    JMath::gekko_ps_copy12(&matrix, _0);
    makeEqualScale(reinterpret_cast< MtxPtr >(&matrix));
    resetAllMtxPrivate(matrix);

    _CE = true;
}

void CollisionParts::resetAllMtxPrivate(const TPos3f& a1) {
    JMath::gekko_ps_copy12(&mPrevBaseMatrix, &a1);
    JMath::gekko_ps_copy12(&mBaseMatrix, &a1);
    JMath::gekko_ps_copy12(&mMatrix, &a1);
    PSMTXInverse(reinterpret_cast< MtxPtr >(&mBaseMatrix), reinterpret_cast< MtxPtr >(&mInvBaseMatrix));
}

void CollisionParts::setMtx(const TPos3f& matrix) {
    JMath::gekko_ps_copy12(&mMatrix, &matrix);
}

void CollisionParts::setMtx() {
    JMath::gekko_ps_copy12(&mMatrix, _0);
}

void CollisionParts::updateMtx() {
    bool bVar1 = false;

    if (_CD || _CE) {
        bVar1 = true;
    }

    if (!bVar1) {
        if (MR::isSameMtx(reinterpret_cast< MtxPtr >(&mMatrix), reinterpret_cast< MtxPtr >(&mBaseMatrix))) {
            _D4++;
        }
    } else {
        if (MR::isSameMtx(reinterpret_cast< MtxPtr >(&mMatrix), reinterpret_cast< MtxPtr >(&mBaseMatrix))) {
            _D4++;
        } else {
            if (_CE) {
                _D4 = 1;
            } else {
                _D4 = 0;
            }

            f32 dVar4 = makeEqualScale(reinterpret_cast< MtxPtr >(&mMatrix));
            _E8 = dVar4;
            f32 var = dVar4 - _DC;
            _EC = dVar4;
            _F0 = dVar4;

            if (!MR::isNearZero(var)) {
                updateBoundingSphereRangePrivate(dVar4);
            }
        }

        _CE = false;

        if (_D4 < 2) {
            JMath::gekko_ps_copy12(&mPrevBaseMatrix, &mBaseMatrix);
            JMath::gekko_ps_copy12(&mBaseMatrix, &mMatrix);
            PSMTXInverse(reinterpret_cast< MtxPtr >(&mBaseMatrix), reinterpret_cast< MtxPtr >(&mInvBaseMatrix));
        }
    }
}

// Issues with assignments of scaleDiff
f32 CollisionParts::makeEqualScale(MtxPtr matrix) {
    TPos3f& mtx = *reinterpret_cast< TPos3f* >(matrix);

    TVec3f scale;
    mtx.getScale(scale);

    TVec3f scaleDiff;
    scaleDiff.x = scale.z - scale.x;
    scaleDiff.y = scale.y - scale.z;
    scaleDiff.z = scale.x - scale.y;

    if (MR::isNearZero(scaleDiff.x) && MR::isNearZero(scaleDiff.y) && MR::isNearZero(scaleDiff.z)) {
        return scale.x;
    }

    f32 uniformScale = 1.0f;
    TVec3f invScale;

    if (_D0) {
        invScale.set(uniformScale / scale.x, uniformScale / scale.y, uniformScale / scale.z);
        uniformScale = 1.0f;
    } else if (_CF) {
        uniformScale = (scale.x + scale.y + scale.z) / 3.0f;
        invScale.set(uniformScale / scale.x, uniformScale / scale.y, uniformScale / scale.z);
    }

    mtx.mMtx[0][0] *= invScale.x;
    mtx.mMtx[1][0] *= invScale.x;
    mtx.mMtx[2][0] *= invScale.x;

    mtx.mMtx[0][1] *= invScale.y;
    mtx.mMtx[1][1] *= invScale.y;
    mtx.mMtx[2][1] *= invScale.y;

    mtx.mMtx[0][2] *= invScale.z;
    mtx.mMtx[1][2] *= invScale.z;
    mtx.mMtx[2][2] *= invScale.z;

    return uniformScale;
}

void CollisionParts::updateBoundingSphereRange() {
    TMtx34f matrix;
    JMath::gekko_ps_copy12(&matrix, _0);
    f32 scale = makeEqualScale(reinterpret_cast< MtxPtr >(&matrix));
    updateBoundingSphereRangePrivate(scale);
}

void CollisionParts::updateBoundingSphereRange(TVec3f a1) {
    f32 range = (a1.x + a1.y + a1.z) / 3.0f;
    updateBoundingSphereRangePrivate(range);
}

void CollisionParts::updateBoundingSphereRangePrivate(f32 scale) {
    _DC = scale;
    _D8 = scale * mServer->mMaxVertexDistance;
}

const char* CollisionParts::getHostName() const {
    if (mHitSensor == nullptr) {
        return nullptr;
    }

    LiveActor* actor = mHitSensor->mHost;

    if (actor == nullptr) {
        return nullptr;
    }

    return actor->mName;
}

s32 CollisionParts::getPlacementZoneID() const {
    return mZone->mZoneID;
}

bool CollisionParts::checkStrikePoint(HitInfo* pHitInfo, const TVec3f& rPoint) {
    TVec3f localPoint;
    mInvBaseMatrix.mult(rPoint, localPoint);

    Fxyz point = makeFxyz(localPoint);
    f32 distance = 0.0f;
    KC_PrismData* prism = mServer->checkPoint(&point, 1.0f, &distance);

    if (prism == nullptr) {
        return false;
    }

    TVec3f localHit;
    projectToPlane(&localHit, localPoint, mServer->getPos(prism, 0), *mServer->getFaceNormal(prism));
    fillHitInfo(this, pHitInfo, prism, rPoint, localHit, toWorldDistance(distance, averageScale(mInvBaseMatrix)), 1, nullptr);
    return true;
}

u32 CollisionParts::checkStrikeBall(HitInfo* pHitInfo, u32 maxInfo, const TVec3f& rCenter, f32 radius, bool movingReaction,
                                    const TriangleFilterBase* pFilter) {
    if (maxInfo == 0) {
        return 0;
    }

    TVec3f localCenter;
    mInvBaseMatrix.mult(rCenter, localCenter);

    const f32 localScale = averageScale(mInvBaseMatrix);
    const f32 localRadius = radius * localScale;
    KC_PrismData* prisms[cQueryMax];
    f32 distances[cQueryMax];
    u8 hitCodes[cQueryMax];
    TVec3f reaction;
    TVec3f* pReaction = nullptr;

    if (movingReaction && _D4 == 0) {
        TVec3f prevWorld;
        mPrevBaseMatrix.mult(localCenter, prevWorld);
        reaction = rCenter;
        reaction.sub(prevWorld);
        pReaction = &reaction;
    }

    return checkStrikeBallCore(pHitInfo, maxInfo, rCenter, localCenter, localRadius, localRadius, localScale, prisms, distances, hitCodes, pFilter,
                               pReaction);
}

u32 CollisionParts::checkStrikeBallCore(HitInfo* pHitInfo, u32 maxInfo, const TVec3f& rWorldCenter, const TVec3f& rLocalCenter, f32 radius,
                                        f32 maxDistance, f32 localScale, KC_PrismData** pPrisms, f32* pDistances, u8* pHitCodes,
                                        const TriangleFilterBase* pFilter, const TVec3f* pReaction) {
    if (maxInfo == 0) {
        return 0;
    }

    Fxyz center = makeFxyz(rLocalCenter);
    const u32 queryMax = clampQueryCount(maxInfo);
    const u32 hitCount = mServer->checkSphere(&center, radius, maxDistance, queryMax, pPrisms, pDistances, pHitCodes);
    u32 storedCount = 0;

    for (u32 i = 0; i < hitCount && storedCount < maxInfo; i++) {
        Triangle triangle;

        if (!fillTriangle(this, &triangle, pPrisms[i], pFilter)) {
            continue;
        }

        const TVec3f localHit = calcLocalHitPosition(mServer, pPrisms[i], rLocalCenter, pHitCodes[i]);
        fillHitInfo(this, &pHitInfo[storedCount], pPrisms[i], rWorldCenter, localHit, toWorldDistance(pDistances[i], localScale), pHitCodes[i],
                    pReaction);
        storedCount++;
    }

    return storedCount;
}

u32 CollisionParts::checkStrikeBallWithThickness(HitInfo* pHitInfo, u32 maxInfo, const TVec3f& rCenter, f32 radius, f32 thickness,
                                                 const TriangleFilterBase* pFilter) {
    if (maxInfo == 0) {
        return 0;
    }

    TVec3f localCenter;
    mInvBaseMatrix.mult(rCenter, localCenter);

    const f32 localScale = averageScale(mInvBaseMatrix);
    const f32 localRadius = radius * localScale;
    const f32 localThickness = thickness * localScale;
    Fxyz center = makeFxyz(localCenter);
    KC_PrismData* prisms[cQueryMax];
    f32 distances[cQueryMax];
    u8 hitCodes[cQueryMax];
    const u32 queryMax = clampQueryCount(maxInfo);
    const u32 hitCount = mServer->checkSphereWithThickness(&center, localRadius, localThickness, queryMax, prisms, distances, hitCodes, 1.0f);
    u32 storedCount = 0;

    for (u32 i = 0; i < hitCount && storedCount < maxInfo; i++) {
        Triangle triangle;

        if (!fillTriangle(this, &triangle, prisms[i], pFilter)) {
            continue;
        }

        const TVec3f localHit = calcLocalHitPosition(mServer, prisms[i], localCenter, hitCodes[i]);
        fillHitInfo(this, &pHitInfo[storedCount], prisms[i], rCenter, localHit, toWorldDistance(distances[i], localScale), hitCodes[i], nullptr);
        storedCount++;
    }

    return storedCount;
}

void CollisionParts::calcCollidePosition(TVec3f* pOut, const KC_PrismData& rPrism, u8 hitCode) {
    const KC_PrismData* prism = &rPrism;
    const TVec3f pos0 = mServer->getPos(prism, 0);
    const TVec3f pos1 = mServer->getPos(prism, 1);
    const TVec3f pos2 = mServer->getPos(prism, 2);

    switch (hitCode) {
    case 2:
        pOut->set((pos0.x + pos2.x) * 0.5f, (pos0.y + pos2.y) * 0.5f, (pos0.z + pos2.z) * 0.5f);
        break;
    case 3:
        pOut->set((pos0.x + pos1.x) * 0.5f, (pos0.y + pos1.y) * 0.5f, (pos0.z + pos1.z) * 0.5f);
        break;
    case 4:
        pOut->set((pos1.x + pos2.x) * 0.5f, (pos1.y + pos2.y) * 0.5f, (pos1.z + pos2.z) * 0.5f);
        break;
    case 5:
        pOut->set(pos0);
        break;
    case 6:
        pOut->set(pos1);
        break;
    case 7:
        pOut->set(pos2);
        break;
    default:
        pOut->set((pos0.x + pos1.x + pos2.x) / 3.0f, (pos0.y + pos1.y + pos2.y) / 3.0f, (pos0.z + pos1.z + pos2.z) / 3.0f);
        break;
    }
}

// Instruction order
void CollisionParts::projectToPlane(TVec3f* pProjected, const TVec3f& rPos, const TVec3f& rOrigin, const TVec3f& rNormal) {
    TVec3f projected = rPos;

    TVec3f relative = rPos;
    relative.sub(rOrigin);

    f32 distance = relative.dot(rNormal);

    TVec3f negNormal = TVec3f(-rNormal);
    negNormal.scale(distance);
    projected.add(negNormal);
    pProjected->set(projected);
}

u32 CollisionParts::checkStrikeLine(HitInfo* pHitInfo, u32 maxInfo, const TVec3f& rStart, const TVec3f& rOffset, const TriangleFilterBase* pFilter) {
    if (maxInfo == 0) {
        return 0;
    }

    TVec3f worldEnd = rStart;
    worldEnd.add(rOffset);

    TVec3f localStart;
    TVec3f localEnd;
    mInvBaseMatrix.mult(rStart, localStart);
    mInvBaseMatrix.mult(worldEnd, localEnd);

    TVec3f localOffset = localEnd;
    localOffset.sub(localStart);

    KC_PrismData* prisms[cQueryMax];
    f32 distances[cQueryMax];
    u8 hitCodes[cQueryMax];
    u32 hitCount = 0;
    mServer->checkArrow(localStart, localOffset, distances, hitCodes, &hitCount, prisms, clampQueryCount(maxInfo));

    u32 storedCount = 0;

    for (u32 i = 0; i < hitCount && storedCount < maxInfo; i++) {
        Triangle triangle;

        if (!fillTriangle(this, &triangle, prisms[i], pFilter)) {
            continue;
        }

        TVec3f localHit = localStart;
        TVec3f scaledOffset = localOffset;
        scaledOffset.scale(distances[i]);
        localHit.add(scaledOffset);
        fillHitInfo(this, &pHitInfo[storedCount], prisms[i], rStart, localHit, distances[i], 1, nullptr);
        storedCount++;
    }

    return storedCount;
}

u32 CollisionParts::createAreaPolygonList(Triangle* pTriangles, u32 maxTriangles, const TVec3f& rMin, const TVec3f& rMax) {
    if (maxTriangles == 0) {
        return 0;
    }

    TVec3f localMin;
    TVec3f localMax;
    mInvBaseMatrix.mult(rMin, localMin);
    mInvBaseMatrix.mult(rMax, localMax);

    Fxyz areaMin = makeFxyz(localMin);
    Fxyz areaMax = makeFxyz(localMax);
    KC_PrismData* prisms[cQueryMax];
    const u32 hitCount = mServer->checkArea3D(&areaMin, &areaMax, prisms, clampQueryCount(maxTriangles));
    u32 storedCount = 0;

    for (u32 i = 0; i < hitCount && storedCount < maxTriangles; i++) {
        pTriangles[storedCount].fillData(this, mServer->toIndex(prisms[i]), mHitSensor);
        storedCount++;
    }

    return storedCount;
}

u32 CollisionParts::createAreaPolygonListArray(Triangle* pTriangles, u32 maxTriangles, TVec3f* pPoints, u32 pointCount) {
    if (maxTriangles == 0 || pointCount == 0) {
        return 0;
    }

    TVec3f localPoint;
    mInvBaseMatrix.mult(pPoints[0], localPoint);

    TVec3f localMin = localPoint;
    TVec3f localMax = localPoint;

    for (u32 i = 1; i < pointCount; i++) {
        mInvBaseMatrix.mult(pPoints[i], localPoint);
        updateMinMax(&localMin, &localMax, localPoint);
    }

    Fxyz areaMin = makeFxyz(localMin);
    Fxyz areaMax = makeFxyz(localMax);
    KC_PrismData* prisms[cQueryMax];
    const u32 hitCount = mServer->checkArea3D(&areaMin, &areaMax, prisms, clampQueryCount(maxTriangles));
    u32 storedCount = 0;

    for (u32 i = 0; i < hitCount && storedCount < maxTriangles; i++) {
        pTriangles[storedCount].fillData(this, mServer->toIndex(prisms[i]), mHitSensor);
        storedCount++;
    }

    return storedCount;
}

void CollisionParts::calcForceMovePower(TVec3f* a1, const TVec3f& a2) const {
    TVec3f tStack88 = a2;
    TMtx34f auStack76;
    PSMTXInverse((MtxPtr)&mPrevBaseMatrix, reinterpret_cast< MtxPtr >(&auStack76));

    auStack76.mult(tStack88, tStack88);
    mBaseMatrix.mult(tStack88, tStack88);

    tStack88.sub(a2);
    *a1 = tStack88;
}
