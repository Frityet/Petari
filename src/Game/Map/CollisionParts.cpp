#include "Game/Map/CollisionParts.hpp"
#include "Game/Camera/CameraPolygonCodeUtil.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/CollisionCategorizedKeeper.hpp"
#include "Game/Map/CollisionDirector.hpp"
#include "Game/Map/KCollision.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/TriangleFilter.hpp"

void FORCE_SCALE() {
    TVec3f vec;
    vec.scale(1.0f);
}

CollisionParts::CollisionParts() : _0(), mHitSensor(), _CC(), _CD(true), _CE(), _CF(), _D0(), _D4(), _D8(-1.0f), _DC(1.0f), mKeeperIndex(-1), mZone() {
    mServer = new KCollisionServer();

    mPrevBaseMatrix.identity();
    mBaseMatrix.identity();
    mMatrix.identity();
    PSMTXInverse(mBaseMatrix.toMtxPtr(), mInvBaseMatrix.toMtxPtr());
}

void CollisionParts::init(const TPos3f& a1, HitSensor* pHitSensor, const void* pKclData, const void* pMapInfo, s32 keeperIndex, bool a6) {
    mServer->init(const_cast< void* >(pKclData), pMapInfo);
    mHitSensor = pHitSensor;

    resetAllMtx(a1);

    TVec3f scale;
    mBaseMatrix.getScale(scale);

    mZone = MR::getCollisionDirector()->getCategoryKeeper(keeperIndex)->getZone(MR::getCurrentPlacementZoneId());

    MR::initCameraCodeCollection(pHitSensor->mHost->mName, mZone->mZoneID);
    mServer->calcFarthestVertexDistance();
    MR::termCameraCodeCollection();

    updateBoundingSphereRange(scale);
    mKeeperIndex = keeperIndex;
}

void CollisionParts::addToBelongZone() {
    s32 zoneID = mZone->mZoneID;

    MR::getCollisionDirector()->getCategoryKeeper(mKeeperIndex)->addToZone(this, zoneID);
}

void CollisionParts::removeFromBelongZone() {
    s32 zoneID = mZone->mZoneID;

    MR::getCollisionDirector()->getCategoryKeeper(mKeeperIndex)->removeFromZone(this, zoneID);
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
        TPos3f matrix(_0);
        makeEqualScale(reinterpret_cast< MtxPtr >(&matrix));

        resetAllMtxPrivate(matrix);
    }
}

void CollisionParts::forceResetAllMtxAndSetUpdateMtxOneTime() {
    TPos3f matrix(_0);
    makeEqualScale(reinterpret_cast< MtxPtr >(&matrix));
    resetAllMtxPrivate(matrix);

    _CE = true;
}

void CollisionParts::resetAllMtxPrivate(const TPos3f& a1) {
    mPrevBaseMatrix.setInline(a1);
    mBaseMatrix.setInline(a1);
    mMatrix.setInline(a1);
    PSMTXInverse(reinterpret_cast< MtxPtr >(&mBaseMatrix), reinterpret_cast< MtxPtr >(&mInvBaseMatrix));
}

void CollisionParts::setMtx(const TPos3f& matrix) {
    mMatrix.setInline(matrix);
}

void CollisionParts::setMtx() {
    mMatrix.setInline(_0);
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
            mPrevBaseMatrix.setInline(mBaseMatrix);
            mBaseMatrix.setInline(mMatrix);
            PSMTXInverse(reinterpret_cast< MtxPtr >(&mBaseMatrix), reinterpret_cast< MtxPtr >(&mInvBaseMatrix));
        }
    }
}

f32 CollisionParts::makeEqualScale(MtxPtr matrix) {
    TPos3f& mtx = *reinterpret_cast< TPos3f* >(matrix);

    TVec3f scale;
    mtx.getScale(scale);

    TVec3f scaleDiff;
    scaleDiff.x = scale.x - scale.y;
    scaleDiff.y = scale.y - scale.z;
    scaleDiff.z = scale.z - scale.x;

    if (MR::isNearZero(scaleDiff.x) && MR::isNearZero(scaleDiff.y) && MR::isNearZero(scaleDiff.z)) {
        return scale.x;
    }

    f32 uniformScale = 1.0f;
    TVec3f invScale;

    if (_D0) {
        invScale.set< f32 >(uniformScale / scale.x, uniformScale / scale.y, uniformScale / scale.z);
        uniformScale = 1.0f;
    } else if (_CF) {
        uniformScale = (scale.x + scale.y + scale.z) / 3.0f;
        invScale.set< f32 >(uniformScale / scale.x, uniformScale / scale.y, uniformScale / scale.z);
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
    TPos3f matrix(_0);
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

u32 CollisionParts::checkStrikeBall(HitInfo* pHitInfo, u32 capacity, const TVec3f& rPos, f32 radius, bool movingReaction,
                                   const TriangleFilterBase* pFilter) {
    KC_PrismData* prisms[64];
    f32 distances[64];
    u8 features[64];
    TVec3f localPos;
    mInvBaseMatrix.mult(rPos, localPos);

    TVec3f scale;
    mInvBaseMatrix.getScale(scale);
    f32 localScale = (scale.x + scale.y + scale.z) / 3.0f;
    f32 worldScale = 1.0f / localScale;
    radius *= localScale;
    TVec3f movePower(0, 0, 0);

    if (movingReaction && _D4 == 0) {
        TPos3f inversePrevious;
        PSMTXInverse(mPrevBaseMatrix.toMtxPtr(), inversePrevious.toMtxPtr());
        TVec3f previousPos;
        inversePrevious.mult(rPos, previousPos);
        TVec3f movement = localPos - previousPos;
        TVec3f worldMovement(movement);
        mBaseMatrix.mult33(worldMovement, worldMovement);
        s32 stepCount = static_cast< s32 >((1.0f / 35.0f) * movement.length()) + 1;
        TVec3f step(movement);

        if (stepCount > 1) {
            step.scale(1.0f / stepCount);
        }

        TVec3f offset(0.0f, 0.0f, 0.0f);

        for (s32 i = 0; i <= stepCount; i++) {
            movePower.set(-(movement - offset));
            mBaseMatrix.mult33(movePower, movePower);
            const TVec3f* pRejectNormal = &worldMovement;

            if (i == stepCount) {
                pRejectNormal = nullptr;
            }

            u32 count = checkStrikeBallCore(pHitInfo, capacity, previousPos + offset, movePower, radius, localScale, worldScale, prisms,
                                           distances, features, pFilter, pRejectNormal);

            if (count != 0) {
                return count;
            }

            offset.add(step);
        }

        return 0;
    }

    return checkStrikeBallCore(pHitInfo, capacity, localPos, TVec3f(0, 0, 0), radius, localScale, worldScale, prisms, distances, features,
                               pFilter, nullptr);
}

u32 CollisionParts::checkStrikeBallCore(HitInfo* pHitInfo, u32 capacity, const TVec3f& rLocalPos, const TVec3f& rMovePower, f32 radius,
                                       f32 localScale, f32 worldScale, KC_PrismData** pPrisms, f32* pDistances, u8* pFeatures,
                                       const TriangleFilterBase* pFilter, const TVec3f* pRejectNormal) {
    u32 count = mServer->checkSphere(reinterpret_cast< Fxyz* >(const_cast< TVec3f* >(&rLocalPos)), radius, localScale, capacity, pPrisms,
                                     pDistances, pFeatures);
    u32 acceptedCount = 0;

    for (u32 i = 0; i < count; i++) {
        HitInfo* pHit = &pHitInfo[acceptedCount];
        TVec3f position(rLocalPos);
        calcCollidePosition(&position, *pPrisms[i], pFeatures[i]);
        mBaseMatrix.mult(position, pHit->mHitPos);
        pHit->mParentTriangle.fillData(this, mServer->toIndex(pPrisms[i]), mHitSensor);

        if (pFilter != nullptr && pFilter->isInvalidTriangle(&pHit->mParentTriangle)) {
            continue;
        }

        if (pRejectNormal != nullptr && 0.0f < pRejectNormal->dot(*pHit->mParentTriangle.getFaceNormal())) {
            continue;
        }

        pHit->_60 = worldScale * pDistances[i];
        pHit->_88 = pFeatures[i];
        pHit->_7C.set(rMovePower);
        const TVec3f* pNormal = pHit->mParentTriangle.getFaceNormal();
        pHit->_7C.scale(pNormal->dot(pHit->_7C), *pNormal);
        acceptedCount++;
    }

    return acceptedCount;
}

u32 CollisionParts::checkStrikeBallWithThickness(HitInfo* pHitInfo, u32 capacity, const TVec3f& rPos, f32 radius, f32 thickness,
                                                const TriangleFilterBase* pFilter) {
    KC_PrismData* prisms[64];
    f32 distances[64];
    u8 features[64];
    TVec3f localPos;
    mInvBaseMatrix.mult(rPos, localPos);
    TVec3f scale;
    mInvBaseMatrix.getScale(scale);
    f32 localScale = (scale.x + scale.y + scale.z) / 3.0f;
    Fxyz position;
    position.x = localPos.x;
    position.y = localPos.y;
    position.z = localPos.z;
    u32 count = mServer->checkSphereWithThickness(&position, radius * localScale, localScale, capacity, prisms, distances, features,
                                                 thickness);
    f32 worldScale = 1.0f / localScale;
    u32 acceptedCount = 0;

    for (u32 i = 0; i < count; i++) {
        HitInfo* pHit = &pHitInfo[acceptedCount];
        TVec3f hitPos(localPos);
        calcCollidePosition(&hitPos, *prisms[i], features[i]);
        mBaseMatrix.mult(hitPos, pHit->mHitPos);
        pHit->mParentTriangle.fillData(this, mServer->toIndex(prisms[i]), mHitSensor);

        if (pFilter != nullptr && pFilter->isInvalidTriangle(&pHit->mParentTriangle)) {
            continue;
        }

        pHit->_60 = worldScale * distances[i];
        pHit->_88 = features[i];
        acceptedCount++;
    }

    return acceptedCount;
}

void CollisionParts::calcCollidePosition(TVec3f* pPos, const KC_PrismData& rPrism, u8 feature) {
    TVec3f offset;
    TVec3f edgeNormal;

    switch (feature) {
    case 1:
        projectToPlane(pPos, *pPos, mServer->getPos(&rPrism, 0), *mServer->getNormal(rPrism.mNormalIndex));
        break;
    case 2:
        projectToPlane(pPos, *pPos, mServer->getPos(&rPrism, 0), *mServer->getNormal(rPrism.mNormalIndex));
        edgeNormal.set(*mServer->getNormal(rPrism.mEdgeIndices[0]));
        offset.set(*pPos);
        offset.sub(mServer->getPos(&rPrism, 0));
        pPos->add(-edgeNormal * offset.dot(edgeNormal));
        break;
    case 3:
        projectToPlane(pPos, *pPos, mServer->getPos(&rPrism, 0), *mServer->getNormal(rPrism.mNormalIndex));
        edgeNormal.set(*mServer->getNormal(rPrism.mEdgeIndices[1]));
        offset.set(*pPos);
        offset.sub(mServer->getPos(&rPrism, 0));
        pPos->add(-edgeNormal * offset.dot(edgeNormal));
        break;
    case 4:
        projectToPlane(pPos, *pPos, mServer->getPos(&rPrism, 0), *mServer->getNormal(rPrism.mNormalIndex));
        edgeNormal.set(*mServer->getNormal(rPrism.mEdgeIndices[2]));
        offset.set(*pPos);
        offset.sub(mServer->getPos(&rPrism, 1));
        pPos->add(-edgeNormal * offset.dot(edgeNormal));
        break;
    case 5:
        pPos->set(mServer->getPos(&rPrism, 0));
        break;
    case 6:
        pPos->set(mServer->getPos(&rPrism, 1));
        break;
    case 7:
        pPos->set(mServer->getPos(&rPrism, 2));
        break;
    }
}

// Instruction order
void CollisionParts::projectToPlane(TVec3f* pProjected, const TVec3f& rPos, const TVec3f& rOrigin, const TVec3f& rNormal) {
    TVec3f projected = rPos;

    f32 distance = (rPos - rOrigin).dot(rNormal);

    projected.add(-rNormal * distance);
    pProjected->set(projected);
}

u32 CollisionParts::checkStrikeLine(HitInfo* pHitInfo, u32 capacity, const TVec3f& rPos, const TVec3f& rOffset,
                                   const TriangleFilterBase* pFilter) {
    f32 distances[64];
    KC_PrismData* prisms[64];
    u8 features[64];
    f32 length = rOffset.length();
    TVec3f localPos;
    TVec3f localOffset;
    mInvBaseMatrix.mult(rPos, localPos);
    mInvBaseMatrix.mult(rPos + rOffset, localOffset);
    localOffset = localOffset - localPos;
    u32 count = 0;
    mServer->checkArrow(localPos, localOffset, distances, features, &count, prisms, capacity);
    u32 acceptedCount = 0;

    for (u32 i = 0; i < count; i++) {
        HitInfo* pHit = &pHitInfo[acceptedCount];
        TVec3f hitPos = localPos + localOffset * distances[i];
        mBaseMatrix.mult(hitPos, hitPos);
        pHit->mParentTriangle.fillData(this, mServer->toIndex(prisms[i]), mHitSensor);

        if (pFilter != nullptr && pFilter->isInvalidTriangle(&pHit->mParentTriangle)) {
            continue;
        }

        pHit->_60 = length * distances[i];
        pHit->mHitPos = hitPos;
        pHit->_88 = features[i];
        acceptedCount++;
    }

    return acceptedCount;
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
