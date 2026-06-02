#include "Game/Util/MapUtil.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/CollisionCategorizedKeeper.hpp"
#include "Game/Map/CollisionCode.hpp"
#include "Game/Map/CollisionDirector.hpp"
#include "Game/Map/CollisionParts.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Util/CollisionPartsFilter.hpp"
#include "Game/Util/GravityUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/TriangleFilter.hpp"

#include <revolution/mtx.h>

class TriangleFilterDangerCode : public TriangleFilterBase {
public:
    virtual bool isInvalidTriangle(const Triangle*) const;
};

static HitInfo mSortBuffer[32];
static u32 mSortCount;

namespace {
    u32 getStrikeInfoNumCategory(s32 category) {
        return MR::getCollisionDirector()->mKeepers[category]->_10;
    }

    bool getFirstPolyOnLineCategory(TVec3f* pDst, Triangle* pTriangle, const TVec3f& rStart, const TVec3f& rOffset,
                                    const TriangleFilterBase* pTriangleFilter, const CollisionPartsFilterBase* pPartsFilter, s32 category) {
        CollisionCategorizedKeeper* keeper = MR::getCollisionDirector()->mKeepers[category];
        const u32 hitCount = keeper->checkStrikeLine(rStart, rOffset, 0, pPartsFilter, pTriangleFilter);

        if (hitCount == 0) {
            return false;
        }

        HitInfo* nearestInfo = keeper->getStrikeInfo(0);
        TVec3f nearestDelta(nearestInfo->mHitPos);
        nearestDelta.sub(rStart);
        f32 nearestDistance = nearestDelta.squared();

        for (u32 i = 1; i < hitCount; i++) {
            HitInfo* hitInfo = keeper->getStrikeInfo(i);
            TVec3f delta(hitInfo->mHitPos);
            delta.sub(rStart);
            f32 distance = delta.squared();

            if (distance < nearestDistance) {
                nearestInfo = hitInfo;
                nearestDistance = distance;
            }
        }

        if (pDst != nullptr) {
            *pDst = nearestInfo->mHitPos;
        }

        if (pTriangle != nullptr) {
            *pTriangle = nearestInfo->mParentTriangle;
        }

        return true;
    }

    bool getFirstPolyOnLineCategoryExceptSensor(TVec3f* pDst, Triangle* pTriangle, const TVec3f& rStart, const TVec3f& rOffset,
                                                const HitSensor* pSensor, s32 category) {
        if (pSensor == nullptr) {
            return getFirstPolyOnLineCategory(pDst, pTriangle, rStart, rOffset, nullptr, nullptr, category);
        }

        CollisionPartsFilterSensor filter(pSensor);
        return getFirstPolyOnLineCategory(pDst, pTriangle, rStart, rOffset, nullptr, &filter, category);
    }

    bool getFirstPolyOnLineCategoryExceptActor(TVec3f* pDst, Triangle* pTriangle, const TVec3f& rStart, const TVec3f& rOffset,
                                               const LiveActor* pActor, s32 category) {
        if (pActor == nullptr) {
            return getFirstPolyOnLineCategory(pDst, pTriangle, rStart, rOffset, nullptr, nullptr, category);
        }

        CollisionPartsFilterActor filter(pActor);
        return getFirstPolyOnLineCategory(pDst, pTriangle, rStart, rOffset, nullptr, &filter, category);
    }
};  // namespace

namespace MR {
    const TVec3f* getNormal(const Triangle* pTriangle) {
        return pTriangle->getNormal(0);
    }

    bool isWallPolygon(const TVec3f& rParam1, const TVec3f& rParam2) {
        if (isNearZero(rParam1)) {
            return false;
        }

        return isWallPolygon(rParam1.dot(rParam2));
    }

    bool isFloorPolygon(const TVec3f& rParam1, const TVec3f& rParam2) {
        if (isNearZero(rParam1)) {
            return false;
        }

        return isFloorPolygon(rParam1.dot(rParam2));
    }

    bool isFloorPolygonCos(const TVec3f& rParam1, const TVec3f& rParam2, f32 param3) {
        if (isNearZero(rParam1)) {
            return false;
        }

        if (-rParam1.dot(rParam2) < param3) {
            return false;
        }

        return isFloorPolygon(rParam1.dot(rParam2));
    }

    bool isWallPolygon(f32 param1) {
        return __fabsf(param1) < 0.34202015f;
    }

    bool isFloorPolygon(f32 param1) {
        if (isWallPolygon(param1)) {
            return false;
        }

        return param1 < 0.0f;
    }

    bool isCeilingPolygon(f32 param1) {
        if (isWallPolygon(param1) || isFloorPolygon(param1)) {
            return false;
        }

        return true;
    }

    bool isWaterPolygon(const Triangle* pTriangle) {
        const char* pFloorCodeString = getFloorCodeString(pTriangle);

        if (pFloorCodeString != nullptr) {
            if (strcmp(pFloorCodeString, "Water") == 0) {
                return true;
            }

            if (strcmp(pFloorCodeString, "Shallow") == 0) {
                return true;
            }
        }

        return false;
    }

    bool isThroughPolygon(const Triangle* pTriangle) {
        const char* pFloorCodeString = getFloorCodeString(pTriangle);

        if (pFloorCodeString != nullptr) {
            if (strcmp(pFloorCodeString, "Water") == 0) {
                return true;
            }

            if (strcmp(pFloorCodeString, "Shallow") == 0) {
                return true;
            }

            if (strcmp(pFloorCodeString, "PullBack") == 0) {
                return true;
            }
        }

        return false;
    }

    bool getFirstPolyOnLineToMap(TVec3f* pDst, Triangle* pTriangle, const TVec3f& rStart, const TVec3f& rOffset) {
        return getFirstPolyOnLineCategory(pDst, pTriangle, rStart, rOffset, nullptr, nullptr, 0);
    }

    bool getFirstPolyOnLineToMapAndMoveLimit(TVec3f* pDst, Triangle* pTriangle, const TVec3f& rStart, const TVec3f& rOffset) {
        TVec3f mapPos;
        Triangle mapTriangle;
        bool hitMap = getFirstPolyOnLineCategory(&mapPos, &mapTriangle, rStart, rOffset, nullptr, nullptr, 0);

        TVec3f moveLimitPos;
        Triangle moveLimitTriangle;
        bool hitMoveLimit = getFirstPolyOnLineCategory(&moveLimitPos, &moveLimitTriangle, rStart, rOffset, nullptr, nullptr, 3);

        if (hitMap && hitMoveLimit && PSVECDistance(&rStart, &moveLimitPos) <= PSVECDistance(&rStart, &mapPos)) {
            hitMap = false;
        }

        if (hitMap) {
            if (pDst != nullptr) {
                *pDst = mapPos;
            }

            if (pTriangle != nullptr) {
                *pTriangle = mapTriangle;
            }

            return true;
        }

        if (hitMoveLimit) {
            if (pDst != nullptr) {
                *pDst = moveLimitPos;
            }

            if (pTriangle != nullptr) {
                *pTriangle = moveLimitTriangle;
            }

            return true;
        }

        return false;
    }

    bool getFirstPolyOnLineToWaterSurface(TVec3f* pDst, Triangle* pTriangle, const TVec3f& rStart, const TVec3f& rOffset) {
        return getFirstPolyOnLineCategory(pDst, pTriangle, rStart, rOffset, nullptr, nullptr, 2);
    }

    bool getFirstPolyOnLineToMapExceptSensor(TVec3f* pDst, Triangle* pTriangle, const TVec3f& rStart, const TVec3f& rOffset,
                                             const HitSensor* pSensor) {
        return getFirstPolyOnLineCategoryExceptSensor(pDst, pTriangle, rStart, rOffset, pSensor, 0);
    }

    bool getFirstPolyOnLineToMapExceptActor(TVec3f* pDst, Triangle* pTriangle, const TVec3f& rStart, const TVec3f& rOffset,
                                            const LiveActor* pActor) {
        return getFirstPolyOnLineCategoryExceptActor(pDst, pTriangle, rStart, rOffset, pActor, 0);
    }

    bool getFirstPolyOnLineToMap(TVec3f* pDst, Triangle* pTriangle, const TVec3f& rStart, const TVec3f& rOffset,
                                 const CollisionPartsFilterBase* pPartsFilter, const TriangleFilterBase* pTriangleFilter) {
        return getFirstPolyOnLineCategory(pDst, pTriangle, rStart, rOffset, pTriangleFilter, pPartsFilter, 0);
    }

    bool getFirstPolyOnLineToWaterSurface(TVec3f* pDst, Triangle* pTriangle, const TVec3f& rStart, const TVec3f& rOffset,
                                          const CollisionPartsFilterBase* pPartsFilter, const TriangleFilterBase* pTriangleFilter) {
        return getFirstPolyOnLineCategory(pDst, pTriangle, rStart, rOffset, pTriangleFilter, pPartsFilter, 2);
    }

    bool getFirstPolyNormalOnLineToMap(TVec3f* pNormal, const TVec3f& rStart, const TVec3f& rOffset, TVec3f* pHitPos,
                                       const HitSensor* pSensor) {
        Triangle triangle;

        if (!getFirstPolyOnLineCategoryExceptSensor(pHitPos, &triangle, rStart, rOffset, pSensor, 0)) {
            return false;
        }

        *pNormal = *triangle.getFaceNormal();
        return true;
    }

    u32 getNearPolyOnLineSort(const TVec3f& rReferencePos, const TVec3f& rStart, const TVec3f& rOffset, const HitSensor* pSensor) {
        const u32 hitCount = getCollisionDirector()->mKeepers[0]->checkStrikeLine(rStart, rOffset, 0, nullptr, nullptr);

        if (hitCount == 0) {
            return 0;
        }

        HitInfo* hitInfos[32];
        u32 exceptCount = 0;

        for (u32 i = 0; i < hitCount; i++) {
            hitInfos[i] = getCollisionDirector()->mKeepers[0]->getStrikeInfo(i);

            if (pSensor != nullptr && hitInfos[i]->mParentTriangle.mSensor == pSensor) {
                hitInfos[i] = nullptr;
                exceptCount++;
            }
        }

        mSortCount = hitCount - exceptCount;

        if (mSortCount >= 32) {
            mSortCount = 32;
        }

        for (u32 sortIdx = 0; sortIdx < mSortCount; sortIdx++) {
            f32 minDistance = 1000000.0f;
            u32 nearestIdx = 0;

            for (u32 hitIdx = 0; hitIdx < hitCount; hitIdx++) {
                if (hitInfos[hitIdx] == nullptr) {
                    continue;
                }

                HitInfo* hitInfo = getCollisionDirector()->mKeepers[0]->getStrikeInfo(hitIdx);
                TVec3f delta(rReferencePos);
                delta.sub(hitInfo->mHitPos);

                f32 distance = PSVECMag(&delta);

                if (distance < minDistance) {
                    nearestIdx = hitIdx;
                    minDistance = distance;
                }
            }

            HitInfo* hitInfo = getCollisionDirector()->mKeepers[0]->getStrikeInfo(nearestIdx);
            HitInfo* sortedInfo = &mSortBuffer[sortIdx];
            sortedInfo->mParentTriangle = hitInfo->mParentTriangle;
            sortedInfo->_60 = hitInfo->_60;
            sortedInfo->mHitPos = hitInfo->mHitPos;
            sortedInfo->_70 = hitInfo->_70;
            sortedInfo->_7C = hitInfo->_7C;
            sortedInfo->_88 = hitInfo->_88;
            hitInfos[nearestIdx] = nullptr;
        }

        return mSortCount;
    }

    bool getSortedPoly(TVec3f* pDst, Triangle* pTriangle, u32 sortIndex) {
        if (mSortCount <= sortIndex) {
            return false;
        }

        HitInfo* hitInfo = &mSortBuffer[sortIndex];

        if (pTriangle != nullptr) {
            *pTriangle = hitInfo->mParentTriangle;
        }

        if (pDst != nullptr) {
            *pDst = hitInfo->mHitPos;
        }

        return true;
    }

    const Triangle* getSortedPoly(u32 sortIndex) {
        if (mSortCount <= sortIndex) {
            return nullptr;
        }

        return &mSortBuffer[sortIndex].mParentTriangle;
    }

    bool isExistMapCollision(const TVec3f& rParam1, const TVec3f& rParam2) {
        return getCollisionDirector()->mKeepers[0]->checkStrikeLine(rParam1, rParam2, 1, nullptr, nullptr) != 0;
    }

    bool isExistMoveLimitCollision(const TVec3f& rParam1, const TVec3f& rParam2) {
        return getCollisionDirector()->mKeepers[3]->checkStrikeLine(rParam1, rParam2, 1, nullptr, nullptr) != 0;
    }

    bool isExistMapCollisionExceptActor(const TVec3f& rStart, const TVec3f& rOffset, const LiveActor* pActor) {
        CollisionPartsFilterActor filter(pActor);
        return getCollisionDirector()->mKeepers[0]->checkStrikeLine(rStart, rOffset, 1, &filter, nullptr) != 0;
    }

    bool checkStrikePointToMap(const TVec3f& rParam1, HitInfo* pParam2) {
        return getCollisionDirector()->mKeepers[0]->checkStrikePoint(rParam1, pParam2) != 0;
    }

    bool checkStrikeBallToMap(const TVec3f& rParam1, f32 param2) {
        return getCollisionDirector()->mKeepers[0]->checkStrikeBall(rParam1, param2, false, nullptr, nullptr) != 0;
    }

    bool calcMapGround(const TVec3f& rPos, TVec3f* pGroundPos, f32 length) {
        TVec3f offset(0.0f, -length, 0.0f);
        return getFirstPolyOnLineCategory(pGroundPos, nullptr, rPos, offset, nullptr, nullptr, 0);
    }

    bool calcMapGroundUpper(TVec3f* pGroundPos, const LiveActor* pActor) {
        CollisionParts* parts = getCollisionParts(pActor);
        TVec3f gravity;
        calcGravityVector(pActor, pActor->mPosition, &gravity, nullptr, 0);

        f32 radius = 0.0f;
        calcModelBoundingRadius(&radius, pActor);

        TVec3f offsetToUpper(gravity);
        offsetToUpper *= 2.5f * radius;

        TVec3f start(pActor->mPosition);
        start.sub(offsetToUpper);

        TVec3f offset(gravity);
        offset *= 4.0f * radius;

        HitInfo hitInfo;
        parts->checkStrikeLine(&hitInfo, 1, start, offset, nullptr);

        CollisionPartsFilterSensor filter(parts->mHitSensor);
        return getFirstPolyOnLineCategory(pGroundPos, nullptr, hitInfo.mHitPos, offset, nullptr, &filter, 0);
    }

    bool isFallNextMove(const LiveActor* pActor, f32 param2, f32 param3, f32 param4, const TriangleFilterBase* pParam5) {
        return isFallNextMove(pActor->mPosition, pActor->mVelocity, pActor->mGravity, param2, param3, param4, pParam5);
    }

    bool isFallNextMove(const TVec3f& rPosition, const TVec3f& rVelocity, const TVec3f& rGravity, f32 moveLength, f32 dropLength,
                        f32 checkLength, const TriangleFilterBase* pFilter) {
        if (isNearZero(rGravity)) {
            return false;
        }

        TVec3f horizontal;
        JMAVECScaleAdd(&rGravity, &rVelocity, &horizontal, -rGravity.dot(rVelocity));

        if (isNearZero(horizontal)) {
            return false;
        }

        normalize(&horizontal);
        horizontal *= moveLength;

        TVec3f down(rGravity);
        down *= dropLength;

        TVec3f start(horizontal);
        start.add(rPosition);
        start.sub(down);

        TVec3f offset;
        offset = start;
        TVec3f longDown(rGravity);
        longDown *= dropLength + checkLength;

        return getCollisionDirector()->mKeepers[0]->checkStrikeLine(offset, longDown, 1, nullptr, pFilter) == 0;
    }

    bool isFallOrDangerNextMove(const LiveActor* pActor, f32 moveLength, f32 dropLength, f32 checkLength) {
        TriangleFilterDangerCode filter;
        return isFallNextMove(pActor->mPosition, pActor->mVelocity, pActor->mGravity, moveLength, dropLength, checkLength, &filter);
    }

    bool isFallOrDangerNextMove(const TVec3f& rPosition, const TVec3f& rVelocity, const TVec3f& rGravity, f32 moveLength, f32 dropLength,
                                f32 checkLength) {
        TriangleFilterDangerCode filter;
        return isFallNextMove(rPosition, rVelocity, rGravity, moveLength, dropLength, checkLength, &filter);
    }

    void calcVelocityMovingPoint(const Triangle* pTriangle, const TVec3f& rPosition, TVec3f* pVelocity) {
        TPos3f* prevMtx = pTriangle->getPrevBaseMtx();

        if (isSameMtx(pTriangle->getBaseMtx()->toMtxPtr(), prevMtx->toMtxPtr())) {
            pVelocity->z = 0.0f;
            pVelocity->y = 0.0f;
            pVelocity->x = 0.0f;
            return;
        }

        TVec3f localPos;
        PSMTXMultVec(pTriangle->getBaseInvMtx()->toMtxPtr(), &rPosition, &localPos);

        TVec3f prevPos;
        PSMTXMultVec(pTriangle->getPrevBaseMtx()->toMtxPtr(), &localPos, &prevPos);

        TVec3f velocity(rPosition);
        velocity.sub(prevPos);
        *pVelocity = velocity;
    }

    u32 createAreaPolygonList(Triangle* pTriangle, u32 param2, const TVec3f& rParam3, const TVec3f& rParam4) {
        return getCollisionDirector()->mKeepers[0]->createAreaPolygonList(pTriangle, param2, rParam3, rParam4);
    }

    u32 createAreaPolygonListArray(Triangle* pTriangle, u32 param2, TVec3f* pParam3, u32 param4) {
        return getCollisionDirector()->mKeepers[0]->createAreaPolygonListArray(pTriangle, param2, pParam3, param4);
    }

    bool trySetMoveLimitCollision(LiveActor* pActor) {
        TVec3f start(pActor->mPosition);
        TVec3f offset(pActor->mGravity);
        TVec3f backOffset(offset);
        backOffset *= 150.0f;
        start.sub(backOffset);
        offset *= 1000.0f;

        if (getCollisionDirector()->mKeepers[3]->checkStrikeLine(start, offset, 0, nullptr, nullptr) != 0) {
            HitInfo* hitInfo = getCollisionDirector()->mKeepers[3]->getStrikeInfo(0);
            pActor->mBinder->setExCollisionParts(hitInfo->mParentTriangle.mParts);
            return true;
        }

        if (getCollisionDirector()->mKeepers[0]->checkStrikeLine(start, offset, 0, nullptr, nullptr) == 0) {
            return false;
        }

        HitInfo* hitInfo = getCollisionDirector()->mKeepers[0]->getStrikeInfo(0);
        CollisionParts* sameHostParts = nullptr;
        CollisionParts* mapParts = hitInfo->mParentTriangle.mParts;
        getCollisionDirector()->mKeepers[3]->searchSameHostParts(&sameHostParts, mapParts);
        pActor->mBinder->setExCollisionParts(sameHostParts);
        return true;
    }

    bool isBindedGroundIce(const LiveActor* pActor) {
        Binder* binder = pActor->mBinder;

        if (binder == nullptr) {
            return false;
        }

        if (0.0f <= binder->_C8) {
            return isGroundCodeIce(&binder->mGroundInfo.mParentTriangle);
        }

        return false;
    }

    bool isBindedGroundSand(const LiveActor* pActor) {
        Binder* binder = pActor->mBinder;

        if (binder == nullptr) {
            return false;
        }

        if (0.0f <= binder->_C8) {
            return isGroundCodeSand(&binder->mGroundInfo.mParentTriangle);
        }

        return false;
    }

    bool isBindedGroundDamageFire(const LiveActor* pActor) {
        Binder* binder = pActor->mBinder;

        if (binder == nullptr) {
            return false;
        }

        if (0.0f <= binder->_C8) {
            return isGroundCodeDamageFire(&binder->mGroundInfo.mParentTriangle);
        }

        return false;
    }

    bool isBindedGroundWaterBottomH(const LiveActor* pActor) {
        Binder* binder = pActor->mBinder;

        if (binder == nullptr) {
            return false;
        }

        if (0.0f <= binder->_C8) {
            return isGroundCodeWaterBottomH(&binder->mGroundInfo.mParentTriangle);
        }

        return false;
    }

    bool isBindedGroundWaterBottomM(const LiveActor* pActor) {
        Binder* binder = pActor->mBinder;

        if (binder == nullptr) {
            return false;
        }

        if (0.0f <= binder->_C8) {
            return isGroundCodeWaterBottomM(&binder->mGroundInfo.mParentTriangle);
        }

        return false;
    }

    bool isBindedGroundWater(const LiveActor* pActor) {
        Binder* binder = pActor->mBinder;

        if (binder == nullptr) {
            return false;
        }

        if (0.0f <= binder->_C8) {
            return isGroundCodeWaterIter(binder->mGroundInfo.mParentTriangle.getAttributes());
        }

        return false;
    }

    bool isBindedGroundSinkDeath(const LiveActor* pActor) {
        Binder* binder = pActor->mBinder;

        if (binder == nullptr) {
            return false;
        }

        if (0.0f <= binder->_C8) {
            return isGroundCodeSinkDeath(&binder->mGroundInfo.mParentTriangle);
        }

        return false;
    }

    bool isBindedGroundAreaMove(const LiveActor* pActor) {
        Binder* binder = pActor->mBinder;

        if (binder == nullptr) {
            return false;
        }

        if (0.0f <= binder->_C8) {
            return isGroundCodeAreaMove(&binder->mGroundInfo.mParentTriangle);
        }

        return false;
    }

    bool isBindedGroundRailMove(const LiveActor* pActor) {
        Binder* binder = pActor->mBinder;

        if (binder == nullptr) {
            return false;
        }

        if (0.0f <= binder->_C8) {
            return isGroundCodeRailMove(&binder->mGroundInfo.mParentTriangle);
        }

        return false;
    }

    bool isBindedGroundBrake(const LiveActor* pActor) {
        Binder* binder = pActor->mBinder;

        if (binder == nullptr) {
            return false;
        }

        if (0.0f <= binder->_C8) {
            return isGroundCodeBrake(&binder->mGroundInfo.mParentTriangle);
        }

        return false;
    }

    bool isBindedDamageFire(const LiveActor* pActor) {
        Binder* binder = pActor->mBinder;

        if (binder == nullptr) {
            return false;
        }

        if (0.0f <= binder->_C8 && isGroundCodeDamageFire(&binder->mGroundInfo.mParentTriangle)) {
            return true;
        }

        if (0.0f <= pActor->mBinder->_158 && isGroundCodeDamageFire(&pActor->mBinder->mWallInfo.mParentTriangle)) {
            return true;
        }

        if (0.0f <= pActor->mBinder->_1E8 && isGroundCodeDamageFire(&pActor->mBinder->mRoofInfo.mParentTriangle)) {
            return true;
        }

        return false;
    }

    bool isBindedDamageElectric(const LiveActor* pActor) {
        Binder* binder = pActor->mBinder;

        if (binder == nullptr) {
            return false;
        }

        if (0.0f <= binder->_C8 && isGroundCodeDamageElectric(&binder->mGroundInfo.mParentTriangle)) {
            return true;
        }

        if (0.0f <= pActor->mBinder->_158 && isGroundCodeDamageElectric(&pActor->mBinder->mWallInfo.mParentTriangle)) {
            return true;
        }

        if (0.0f <= pActor->mBinder->_1E8 && isGroundCodeDamageElectric(&pActor->mBinder->mRoofInfo.mParentTriangle)) {
            return true;
        }

        return false;
    }

    u32 getCameraID(const Triangle* pTriangle) {
        return getCollisionDirector()->mCode->getCameraID(*pTriangle);
    }

    const char* getFloorCodeString(const Triangle* pTriangle) {
        return getCollisionDirector()->mCode->getFloorCodeString(*pTriangle);
    }

    const char* getWallCodeString(const Triangle* pTriangle) {
        return getCollisionDirector()->mCode->getWallCodeString(*pTriangle);
    }

    const char* getSoundCodeString(const Triangle* pTriangle) {
        return getCollisionDirector()->mCode->getSoundCodeString(*pTriangle);
    }

    s32 getFloorCodeIndex(const JMapInfoIter& rIter) {
        return getCollisionDirector()->mCode->getFloorCode(rIter);
    }

    s32 getSoundCodeIndex(const JMapInfoIter& rIter) {
        return getCollisionDirector()->mCode->getSoundCode(rIter);
    }

    s32 getFloorCodeIndex(const Triangle* pTriangle) {
        return getCollisionDirector()->mCode->getFloorCode(pTriangle->getAttributes());
    }

    s32 getWallCodeIndex(const Triangle* pTriangle) {
        return getCollisionDirector()->mCode->getWallCode(pTriangle->getAttributes());
    }

    s32 getSoundCodeIndex(const Triangle* pTriangle) {
        return getCollisionDirector()->mCode->getSoundCode(pTriangle->getAttributes());
    }

    s32 getCameraCodeIndex(const Triangle* pTriangle) {
        return getCollisionDirector()->mCode->getCameraCode(pTriangle->getAttributes());
    }

    bool isGroundCodeWaterIter(const JMapInfoIter& rIter) {
        s32 code = getFloorCodeIndex(rIter);

        return code == CollisionFloorCode_WaterBottomH || code == CollisionFloorCode_WaterBottomM || code == CollisionFloorCode_WaterBottomL ||
               code == CollisionFloorCode_Wet;
    }

    bool isGroundCodeDeath(const Triangle* pTriangle) {
        return getFloorCodeIndex(pTriangle) == CollisionFloorCode_Death;
    }

    bool isGroundCodeDamage(const Triangle* pTriangle) {
        return getFloorCodeIndex(pTriangle) == CollisionFloorCode_DamageNormal;
    }

    bool isGroundCodeIce(const Triangle* pTriangle) {
        return getFloorCodeIndex(pTriangle) == CollisionFloorCode_Ice;
    }

    bool isGroundCodeDamageFire(const Triangle* pTriangle) {
        return getFloorCodeIndex(pTriangle) == CollisionFloorCode_DamageFire;
    }

    bool isGroundCodeFireDance(const Triangle* pTriangle) {
        return getFloorCodeIndex(pTriangle) == CollisionFloorCode_FireDance;
    }

    bool isGroundCodeSand(const Triangle* pTriangle) {
        return getFloorCodeIndex(pTriangle) == CollisionFloorCode_Sand;
    }

    bool isGroundCodeDamageElectric(const Triangle* pTriangle) {
        return getFloorCodeIndex(pTriangle) == CollisionFloorCode_DamageElectric;
    }

    bool isGroundCodeWaterBottomH(const Triangle* pTriangle) {
        return getFloorCodeIndex(pTriangle) == CollisionFloorCode_WaterBottomH;
    }

    bool isGroundCodeWaterBottomM(const Triangle* pTriangle) {
        return getFloorCodeIndex(pTriangle) == CollisionFloorCode_WaterBottomM;
    }

    bool isGroundCodeSinkDeath(const Triangle* pTriangle) {
        return getFloorCodeIndex(pTriangle) == CollisionFloorCode_SinkDeath;
    }

    bool isGroundCodeRailMove(const Triangle* pTriangle) {
        return getFloorCodeIndex(pTriangle) == CollisionFloorCode_RailMove;
    }

    bool isGroundCodeAreaMove(const Triangle* pTriangle) {
        return getFloorCodeIndex(pTriangle) == CollisionFloorCode_AreaMove;
    }

    bool isGroundCodeNoStampSand(const Triangle* pTriangle) {
        return getFloorCodeIndex(pTriangle) == CollisionFloorCode_NoStampSand;
    }

    bool isGroundCodeSinkDeathMud(const Triangle* pTriangle) {
        return getFloorCodeIndex(pTriangle) == CollisionFloorCode_SinkDeathMud;
    }

    bool isGroundCodeBrake(const Triangle* pTriangle) {
        return getFloorCodeIndex(pTriangle) == CollisionFloorCode_Brake;
    }

    bool isWallCodeGhostThrough(const Triangle* pTriangle) {
        return getWallCodeIndex(pTriangle) == CollisionWallCode_GhostThroughCode;
    }

    bool isWallCodeRebound(const Triangle* pTriangle) {
        return getWallCodeIndex(pTriangle) == CollisionWallCode_Rebound;
    }

    bool isWallCodeNoAction(const Triangle* pTriangle) {
        return getWallCodeIndex(pTriangle) == CollisionWallCode_NoAction;
    }

    bool isSoundCodeSand(const Triangle* pTriangle) {
        return getSoundCodeIndex(pTriangle) == CollisionSoundCode_Sand;
    }

    bool isCameraCodeThrough(const Triangle* pTriangle) {
        return getCameraCodeIndex(pTriangle) == CollisionCameraCode_Through;
    }

    bool isCodeSand(const Triangle* pTriangle) {
        bool ret = true;
        bool isSand = true;
        bool isSoundSand = true;

        if (getSoundCodeIndex(pTriangle) != CollisionSoundCode_Sand && getSoundCodeIndex(pTriangle) != CollisionSoundCode_Beach) {
            isSoundSand = false;
        }

        if (!isSoundSand && !isGroundCodeSand(pTriangle)) {
            isSand = false;
        }

        if (!isSand && !isGroundCodeNoStampSand(pTriangle)) {
            ret = false;
        }

        return ret;
    }

    const Triangle* getCameraPolyFast(const TVec3f& rStart, const TVec3f& rOffset, const HitSensor* pSensor) {
        Triangle triangle;
        f32 remainingLength = PSVECMag(&rOffset);
        TVec3f unitOffset(rOffset);
        PSVECMag(&unitOffset);
        PSVECNormalize(&unitOffset, &unitOffset);

        TVec3f currentStart(rStart);
        TVec3f currentOffset(unitOffset);
        currentOffset.scale(5000.0f);

        while (true) {
            f32 stepLength = 5000.0f;

            if (remainingLength < stepLength) {
                stepLength = remainingLength;
                TVec3f shortOffset(unitOffset);
                shortOffset.scale(remainingLength);
                currentOffset = shortOffset;
            }

            if (getNearPolyOnLineSort(currentStart, currentStart, currentOffset, pSensor) != 0) {
                return getSortedPoly(0);
            }

            remainingLength -= stepLength;
            currentStart.add(currentOffset);

            if (isNearZero(remainingLength, 0.001f)) {
                return nullptr;
            }
        }
    }

    bool getFirstPolyOnLineBFast(const TVec3f& rStart, const TVec3f& rOffset, TVec3f* pHitPos, Triangle* pTriangle) {
        Triangle triangle;
        f32 remainingLength = PSVECMag(&rOffset);
        TVec3f unitOffset(rOffset);
        PSVECMag(&unitOffset);
        PSVECNormalize(&unitOffset, &unitOffset);

        TVec3f currentStart(rStart);
        TVec3f currentOffset(unitOffset);
        currentOffset.scale(5000.0f);

        while (true) {
            f32 stepLength = 5000.0f;

            if (remainingLength < stepLength) {
                stepLength = remainingLength;
                TVec3f shortOffset(unitOffset);
                shortOffset.scale(remainingLength);
                currentOffset = shortOffset;
            }

            u32 count = getNearPolyOnLineSort(currentStart, currentStart, currentOffset, nullptr);

            for (u32 i = 0; i < count; i++) {
                TVec3f hitPos;
                Triangle hitTriangle;

                if (!getSortedPoly(&hitPos, &hitTriangle, i) || isWaterPolygon(&hitTriangle)) {
                    continue;
                }

                TVec3f backOffset(*hitTriangle.getNormal(0));
                backOffset.scale(5.0f);

                TVec3f checkStart(hitPos);
                checkStart.sub(backOffset);

                TVec3f checkOffset(*hitTriangle.getNormal(0));
                checkOffset.scale(35.0f);

                if (getCollisionDirector()->mKeepers[0]->checkStrikeLine(checkStart, checkOffset, 1, nullptr, nullptr) != 0) {
                    continue;
                }

                if (pHitPos != nullptr) {
                    *pHitPos = hitPos;
                }

                if (pTriangle != nullptr) {
                    *pTriangle = hitTriangle;
                }

                return true;
            }

            remainingLength -= stepLength;
            currentStart.add(currentOffset);

            if (isNearZero(remainingLength, 0.001f)) {
                return false;
            }
        }
    }
};  // namespace MR

namespace Collision {
    s32 checkStrikePointToMap(const TVec3f& rPos, HitInfo* pHitInfo) {
        return MR::getCollisionDirector()->mKeepers[0]->checkStrikePoint(rPos, pHitInfo);
    }

    s32 checkStrikeBallToMap(const TVec3f& rCenter, f32 radius, const CollisionPartsFilterBase* pPartsFilter,
                             const TriangleFilterBase* pTriangleFilter) {
        return MR::getCollisionDirector()->mKeepers[0]->checkStrikeBall(rCenter, radius, false, pPartsFilter, pTriangleFilter);
    }

    s32 checkStrikeBallToMapWithMovingReaction(const TVec3f& rCenter, f32 radius, const CollisionPartsFilterBase* pPartsFilter,
                                               const TriangleFilterBase* pTriangleFilter) {
        return MR::getCollisionDirector()->mKeepers[0]->checkStrikeBall(rCenter, radius, true, pPartsFilter, pTriangleFilter);
    }

    s32 checkStrikeBallToMapWithThickness(const TVec3f& rCenter, f32 radius, f32 thickness, const CollisionPartsFilterBase* pPartsFilter,
                                          const TriangleFilterBase* pTriangleFilter) {
        return MR::getCollisionDirector()->mKeepers[0]->checkStrikeBallWithThickness(rCenter, radius, thickness, pPartsFilter, pTriangleFilter);
    }

    s32 checkStrikeLineToMap(const TVec3f& rStart, const TVec3f& rOffset, s32 maxHits, const CollisionPartsFilterBase* pPartsFilter,
                             const TriangleFilterBase* pTriangleFilter) {
        return MR::getCollisionDirector()->mKeepers[0]->checkStrikeLine(rStart, rOffset, maxHits, pPartsFilter, pTriangleFilter);
    }

    s32 checkStrikeLineToSunshade(const TVec3f& rStart, const TVec3f& rOffset, s32 maxHits, const CollisionPartsFilterBase* pPartsFilter,
                                  const TriangleFilterBase* pTriangleFilter) {
        return MR::getCollisionDirector()->mKeepers[1]->checkStrikeLine(rStart, rOffset, maxHits, pPartsFilter, pTriangleFilter);
    }

    const HitInfo* getStrikeInfoMap(u32 index) {
        return MR::getCollisionDirector()->mKeepers[0]->getStrikeInfo(index);
    }

    u32 getStrikeInfoNumMap() {
        return getStrikeInfoNumCategory(0);
    }
};  // namespace Collision

void Binder::setExCollisionParts(CollisionParts* pParts) {
    mCollisionParts = pParts;

    u8* flags = reinterpret_cast<u8*>(&_1EC);
    if (pParts == nullptr) {
        *flags &= ~0x20;
    }
    else {
        *flags |= 0x20;
    }
}
