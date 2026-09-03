#include "Game/Util/MapUtil.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/CollisionCategorizedKeeper.hpp"
#include "Game/Map/CollisionCode.hpp"
#include "Game/Map/CollisionDirector.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/CollisionPartsFilter.hpp"
#include "Game/Util/TriangleFilter.hpp"

static HitInfo mSortBuffer[32];
static u32 mSortCount;

namespace {
    u32 getStrikeInfoNumCategory(s32 category) NO_INLINE {
        return MR::getCollisionDirector()->getCategoryKeeper(category)->_10;
    }

    bool getFirstPolyOnLineCategory(TVec3f* pPosition, Triangle* pTriangle, const TVec3f& rStart, const TVec3f& rDirection,
                                    const TriangleFilterBase* pTriangleFilter, const CollisionPartsFilterBase* pPartsFilter,
                                    s32 category) NO_INLINE {
        u32 count = MR::getCollisionDirector()->getCategoryKeeper(category)->checkStrikeLine(rStart, rDirection, 0, pPartsFilter, nullptr);
        if (count == 0) {
            return false;
        }

        f32 distance = 1000000.0f;
        s32 index = -1;
        for (u32 i = 0; i < count; i++) {
            const HitInfo* pInfo = MR::getCollisionDirector()->getCategoryKeeper(category)->getStrikeInfo(i);
            if (pTriangleFilter != nullptr && pTriangleFilter->isInvalidTriangle(&pInfo->mParentTriangle)) {
                continue;
            }
            if (distance > pInfo->_60) {
                index = i;
                distance = pInfo->_60;
            }
        }
        if (index == -1) {
            return false;
        }

        const HitInfo* pInfo = MR::getCollisionDirector()->getCategoryKeeper(category)->getStrikeInfo(index);
        if (pPosition != nullptr) {
            *pPosition = pInfo->mHitPos;
        }
        if (pTriangle != nullptr) {
            *pTriangle = pInfo->mParentTriangle;
        }
        return true;
    }

    bool getFirstPolyOnLineCategoryExceptSensor(TVec3f* pPosition, Triangle* pTriangle, const TVec3f& rStart,
                                                const TVec3f& rDirection, const HitSensor* pSensor, s32 category) NO_INLINE {
        CollisionPartsFilterSensor filter(pSensor);
        return getFirstPolyOnLineCategory(pPosition, pTriangle, rStart, rDirection, nullptr, &filter, category);
    }

    bool getFirstPolyOnLineCategoryExceptActor(TVec3f* pPosition, Triangle* pTriangle, const TVec3f& rStart,
                                               const TVec3f& rDirection, const LiveActor* pActor, s32 category) NO_INLINE {
        CollisionPartsFilterActor filter(pActor);
        return getFirstPolyOnLineCategory(pPosition, pTriangle, rStart, rDirection, nullptr, &filter, category);
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
        return MR::abs(param1) < 0.34202015f;
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

    bool getFirstPolyOnLineToMap(TVec3f* pPosition, Triangle* pTriangle, const TVec3f& rStart, const TVec3f& rDirection) {
        return ::getFirstPolyOnLineCategory(pPosition, pTriangle, rStart, rDirection, nullptr, nullptr, 0);
    }

    bool getFirstPolyOnLineToMapAndMoveLimit(TVec3f* pPosition, Triangle* pTriangle, const TVec3f& rStart, const TVec3f& rDirection) {
        Triangle mapTriangle;
        TVec3f mapPosition;
        bool mapHit = ::getFirstPolyOnLineCategory(&mapPosition, &mapTriangle, rStart, rDirection, nullptr, nullptr, 0);
        Triangle limitTriangle;
        TVec3f limitPosition;
        bool limitHit = ::getFirstPolyOnLineCategory(&limitPosition, &limitTriangle, rStart, rDirection, nullptr, nullptr, 3);
        if (mapHit && limitHit) {
            f32 limitDistance = PSVECDistance(&rStart, &limitPosition);
            if (PSVECDistance(&rStart, &mapPosition) >= limitDistance) {
                mapHit = false;
            }
        }
        if (mapHit) {
            if (pPosition != nullptr) {
                pPosition->set< f32 >(mapPosition);
            }
            if (pTriangle != nullptr) {
                *pTriangle = mapTriangle;
            }
            return true;
        }
        if (limitHit) {
            if (pPosition != nullptr) {
                pPosition->set< f32 >(limitPosition);
            }
            if (pTriangle != nullptr) {
                *pTriangle = limitTriangle;
            }
            return true;
        }
        return false;
    }

    bool getFirstPolyOnLineToWaterSurface(TVec3f* pPosition, Triangle* pTriangle, const TVec3f& rStart, const TVec3f& rDirection) {
        return ::getFirstPolyOnLineCategory(pPosition, pTriangle, rStart, rDirection, nullptr, nullptr, 2);
    }

    bool getFirstPolyOnLineToMapExceptSensor(TVec3f* pPosition, Triangle* pTriangle, const TVec3f& rStart, const TVec3f& rDirection,
                                            const HitSensor* pSensor) {
        return ::getFirstPolyOnLineCategoryExceptSensor(pPosition, pTriangle, rStart, rDirection, pSensor, 0);
    }

    bool getFirstPolyOnLineToMapExceptActor(TVec3f* pPosition, Triangle* pTriangle, const TVec3f& rStart, const TVec3f& rDirection,
                                           const LiveActor* pActor) {
        return ::getFirstPolyOnLineCategoryExceptActor(pPosition, pTriangle, rStart, rDirection, pActor, 0);
    }

    bool getFirstPolyOnLineToMap(TVec3f* pPosition, Triangle* pTriangle, const TVec3f& rStart, const TVec3f& rDirection,
                                 const CollisionPartsFilterBase* pPartsFilter, const TriangleFilterBase* pTriangleFilter) {
        return ::getFirstPolyOnLineCategory(pPosition, pTriangle, rStart, rDirection, pTriangleFilter, pPartsFilter, 0);
    }

    bool getFirstPolyOnLineToWaterSurface(TVec3f* pPosition, Triangle* pTriangle, const TVec3f& rStart, const TVec3f& rDirection,
                                          const CollisionPartsFilterBase* pPartsFilter, const TriangleFilterBase* pTriangleFilter) {
        return ::getFirstPolyOnLineCategory(pPosition, pTriangle, rStart, rDirection, pTriangleFilter, pPartsFilter, 2);
    }

    bool getFirstPolyNormalOnLineToMap(TVec3f* pNormal, const TVec3f& rStart, const TVec3f& rDirection, TVec3f* pPosition,
                                       const HitSensor* pSensor) {
        Triangle triangle;
        if (!::getFirstPolyOnLineCategoryExceptSensor(pPosition, &triangle, rStart, rDirection, pSensor, 0)) {
            return false;
        }
        pNormal->set< f32 >(*triangle.getFaceNormal());
        return true;
    }

    u32 getNearPolyOnLineSort(const TVec3f& rOrigin, const TVec3f& rStart, const TVec3f& rDirection, const HitSensor* pSensor) {
        u32 count = getCollisionDirector()->getCategoryKeeper(0)->checkStrikeLine(rStart, rDirection, 0, nullptr, nullptr);
        if (count == 0) {
            return 0;
        }

        const HitInfo* infos[32];
        u32 excluded = 0;
        for (u32 i = 0; i < count; i++) {
            infos[i] = getCollisionDirector()->getCategoryKeeper(0)->getStrikeInfo(i);
            if (pSensor != nullptr && infos[i]->mParentTriangle.mSensor == pSensor) {
                infos[i] = nullptr;
                excluded++;
            }
        }

        mSortCount = count - excluded;
        if (mSortCount >= 32) {
            mSortCount = 32;
        }
        for (u32 i = 0; i < mSortCount; i++) {
            f32 distance = 1000000.0f;
            u32 index = 0;
            for (u32 j = 0; j < count; j++) {
                if (infos[j] == nullptr) {
                    continue;
                }
                const HitInfo* pInfo = getCollisionDirector()->getCategoryKeeper(0)->getStrikeInfo(j);
                TVec3f delta(rOrigin);
                delta.sub(pInfo->mHitPos);
                f32 length = PSVECMag(&delta);
                if (distance > length) {
                    index = j;
                    distance = length;
                }
            }
            const HitInfo* pInfo = getCollisionDirector()->getCategoryKeeper(0)->getStrikeInfo(index);
            HitInfo& rInfo = mSortBuffer[i];
            rInfo.mParentTriangle = pInfo->mParentTriangle;
            rInfo._60 = pInfo->_60;
            rInfo.mHitPos = pInfo->mHitPos;
            rInfo._70 = pInfo->_70;
            rInfo._7C = pInfo->_7C;
            rInfo._88 = pInfo->_88;
            infos[index] = nullptr;
        }
        return mSortCount;
    }

    bool getSortedPoly(TVec3f* pDst, Triangle* pTriangle, u32 sortIndex) {
        if (mSortCount <= sortIndex) {
            return false;
        }

        if (pTriangle != nullptr) {
            *pTriangle = mSortBuffer[sortIndex].mParentTriangle;
        }

        if (pDst != nullptr) {
            *pDst = mSortBuffer[sortIndex].mHitPos;
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
        return getCollisionDirector()->getCategoryKeeper(0)->checkStrikeLine(rParam1, rParam2, 1, nullptr, nullptr) != 0;
    }

    bool isExistMoveLimitCollision(const TVec3f& rParam1, const TVec3f& rParam2) {
        return getCollisionDirector()->getCategoryKeeper(3)->checkStrikeLine(rParam1, rParam2, 1, nullptr, nullptr) != 0;
    }

    // isExistMapCollisionExceptActor

    bool checkStrikePointToMap(const TVec3f& rParam1, HitInfo* pParam2) {
        return getCollisionDirector()->getCategoryKeeper(0)->checkStrikePoint(rParam1, pParam2) != 0;
    }

    bool checkStrikeBallToMap(const TVec3f& rParam1, f32 param2) {
        return getCollisionDirector()->getCategoryKeeper(0)->checkStrikeBall(rParam1, param2, false, nullptr, nullptr) != 0;
    }

    // calcMapGround
    // calcMapGroundUpper

    bool isFallNextMove(const LiveActor* pActor, f32 param2, f32 param3, f32 param4, const TriangleFilterBase* pParam5) {
        return isFallNextMove(pActor->mPosition, pActor->mVelocity, pActor->mGravity, param2, param3, param4, pParam5);
    }

    // isFallNextMove
    // isFallOrDangerNextMove
    // isFallOrDangerNextMove
    // calcVelocityMovingPoint

    u32 createAreaPolygonList(Triangle* pTriangle, u32 param2, const TVec3f& rParam3, const TVec3f& rParam4) {
        return getCollisionDirector()->getCategoryKeeper(0)->createAreaPolygonList(pTriangle, param2, rParam3, rParam4);
    }

    u32 createAreaPolygonListArray(Triangle* pTriangle, u32 param2, TVec3f* pParam3, u32 param4) {
        return getCollisionDirector()->getCategoryKeeper(0)->createAreaPolygonListArray(pTriangle, param2, pParam3, param4);
    }

    // trySetMoveLimitCollision

    bool isBindedGroundIce(const LiveActor* pActor) {
        if (pActor->mBinder == nullptr) {
            return false;
        }

        if (!pActor->mBinder->isBindedGround()) {
            return false;
        }

        return isGroundCodeIce(&pActor->mBinder->mGroundInfo.mParentTriangle);
    }

    bool isBindedGroundSand(const LiveActor* pActor) {
        if (pActor->mBinder == nullptr) {
            return false;
        }

        if (!pActor->mBinder->isBindedGround()) {
            return false;
        }

        return isGroundCodeSand(&pActor->mBinder->mGroundInfo.mParentTriangle);
    }

    bool isBindedGroundDamageFire(const LiveActor* pActor) {
        if (pActor->mBinder == nullptr) {
            return false;
        }

        if (!pActor->mBinder->isBindedGround()) {
            return false;
        }

        return isGroundCodeDamageFire(&pActor->mBinder->mGroundInfo.mParentTriangle);
    }

    bool isBindedGroundWaterBottomH(const LiveActor* pActor) {
        if (pActor->mBinder == nullptr) {
            return false;
        }

        if (!pActor->mBinder->isBindedGround()) {
            return false;
        }

        return isGroundCodeWaterBottomH(&pActor->mBinder->mGroundInfo.mParentTriangle);
    }

    bool isBindedGroundWaterBottomM(const LiveActor* pActor) {
        if (pActor->mBinder == nullptr) {
            return false;
        }

        if (!pActor->mBinder->isBindedGround()) {
            return false;
        }

        return isGroundCodeWaterBottomM(&pActor->mBinder->mGroundInfo.mParentTriangle);
    }

    bool isBindedGroundWater(const LiveActor* pActor) {
        if (pActor->mBinder == nullptr) {
            return false;
        }

        if (!pActor->mBinder->isBindedGround()) {
            return false;
        }

        return isGroundCodeWaterIter(pActor->mBinder->mGroundInfo.mParentTriangle.getAttributes());
    }

    bool isBindedGroundSinkDeath(const LiveActor* pActor) {
        if (pActor->mBinder == nullptr) {
            return false;
        }

        if (!pActor->mBinder->isBindedGround()) {
            return false;
        }

        return isGroundCodeSinkDeath(&pActor->mBinder->mGroundInfo.mParentTriangle);
    }

    bool isBindedGroundAreaMove(const LiveActor* pActor) {
        if (pActor->mBinder == nullptr) {
            return false;
        }

        if (!pActor->mBinder->isBindedGround()) {
            return false;
        }

        return isGroundCodeAreaMove(&pActor->mBinder->mGroundInfo.mParentTriangle);
    }

    bool isBindedGroundRailMove(const LiveActor* pActor) {
        if (pActor->mBinder == nullptr) {
            return false;
        }

        if (!pActor->mBinder->isBindedGround()) {
            return false;
        }

        return isGroundCodeRailMove(&pActor->mBinder->mGroundInfo.mParentTriangle);
    }

    bool isBindedGroundBrake(const LiveActor* pActor) {
        if (pActor->mBinder == nullptr) {
            return false;
        }

        if (!pActor->mBinder->isBindedGround()) {
            return false;
        }

        return isGroundCodeBrake(&pActor->mBinder->mGroundInfo.mParentTriangle);
    }

    bool isBindedDamageFire(const LiveActor* pActor) {
        if (pActor->mBinder == nullptr) {
            return false;
        }

        if (pActor->mBinder->isBindedGround() && isGroundCodeDamageFire(&pActor->mBinder->mGroundInfo.mParentTriangle)) {
            return true;
        }

        if (pActor->mBinder->isBindedWall() && isGroundCodeDamageFire(&pActor->mBinder->mWallInfo.mParentTriangle)) {
            return true;
        }

        if (pActor->mBinder->isBindedRoof() && isGroundCodeDamageFire(&pActor->mBinder->mRoofInfo.mParentTriangle)) {
            return true;
        }

        return false;
    }

    bool isBindedDamageElectric(const LiveActor* pActor) {
        if (pActor->mBinder == nullptr) {
            return false;
        }

        if (pActor->mBinder->isBindedGround() && isGroundCodeDamageElectric(&pActor->mBinder->mGroundInfo.mParentTriangle)) {
            return true;
        }

        if (pActor->mBinder->isBindedWall() && isGroundCodeDamageElectric(&pActor->mBinder->mWallInfo.mParentTriangle)) {
            return true;
        }

        if (pActor->mBinder->isBindedRoof() && isGroundCodeDamageElectric(&pActor->mBinder->mRoofInfo.mParentTriangle)) {
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
        return isSoundCodeSand(pTriangle) || isGroundCodeSand(pTriangle) || isGroundCodeNoStampSand(pTriangle);
    }

    const Triangle* getCameraPolyFast(const TVec3f& rStart, const TVec3f& rDirection, const HitSensor* pSensor) {
        Triangle triangle;
        f32 remaining = PSVECMag(&rDirection);
        TVec3f direction(rDirection);
        direction.normalize();
        TVec3f position(rStart);
        TVec3f offset(direction);
        offset *= 5000.0f;
        do {
            f32 step = 5000.0f;
            if (remaining < step) {
                step = remaining;
                TVec3f tail(direction);
                tail *= remaining;
                offset = tail;
            }
            u32 count = getNearPolyOnLineSort(position, position, offset, pSensor);
            while (count != 0) {
                return getSortedPoly(0);
            }
            remaining -= step;
            position += offset;
        } while (!isNearZero(remaining, 0.001f));
        return nullptr;
    }

    bool getFirstPolyOnLineBFast(const TVec3f& rStart, const TVec3f& rDirection, TVec3f* pPosition, Triangle* pTriangle) {
        Triangle triangle;
        f32 remaining = PSVECMag(&rDirection);
        TVec3f direction(rDirection);
        direction.normalize();
        TVec3f position(rStart);
        TVec3f offset(direction);
        offset *= 5000.0f;
        do {
            f32 step = 5000.0f;
            if (remaining < step) {
                step = remaining;
                TVec3f tail(direction);
                tail *= remaining;
                offset = tail;
            }
            u32 count = getNearPolyOnLineSort(position, position, offset, nullptr);
            for (u32 i = 0; i < count; i++) {
                Triangle hitTriangle;
                TVec3f hitPosition;
                if (getSortedPoly(&hitPosition, &hitTriangle, i)) {
                    if (isWaterPolygon(&hitTriangle)) {
                        continue;
                    }
                    TVec3f back(*hitTriangle.getNormal(0));
                    back *= 5.0f;
                    TVec3f probePosition(hitPosition);
                    probePosition.sub(back);
                    TVec3f probeDirection(*hitTriangle.getNormal(0));
                    probeDirection *= 35.0f;
                    if (isExistMapCollision(probePosition, probeDirection)) {
                        continue;
                    }
                }
                if (pPosition != nullptr) {
                    *pPosition = hitPosition;
                }
                if (pTriangle != nullptr) {
                    *pTriangle = hitTriangle;
                }
                return true;
            }
            remaining -= step;
            position += offset;
        } while (!isNearZero(remaining, 0.001f));
        return false;
    }
};  // namespace MR

namespace Collision {
    s32 checkStrikePointToMap(const TVec3f& rPosition, HitInfo* pInfo) {
        return MR::getCollisionDirector()->getCategoryKeeper(0)->checkStrikePoint(rPosition, pInfo);
    }

    s32 checkStrikeBallToMap(const TVec3f& rPosition, f32 radius, const CollisionPartsFilterBase* pPartsFilter,
                              const TriangleFilterBase* pTriangleFilter) {
        return MR::getCollisionDirector()->getCategoryKeeper(0)->checkStrikeBall(rPosition, radius, false, pPartsFilter, pTriangleFilter);
    }

    s32 checkStrikeBallToMapWithMovingReaction(const TVec3f& rPosition, f32 radius, const CollisionPartsFilterBase* pPartsFilter,
                                                const TriangleFilterBase* pTriangleFilter) {
        return MR::getCollisionDirector()->getCategoryKeeper(0)->checkStrikeBall(rPosition, radius, true, pPartsFilter, pTriangleFilter);
    }

    s32 checkStrikeBallToMapWithThickness(const TVec3f& rPosition, f32 radius, f32 thickness,
                                           const CollisionPartsFilterBase* pPartsFilter, const TriangleFilterBase* pTriangleFilter) {
        return MR::getCollisionDirector()->getCategoryKeeper(0)->checkStrikeBallWithThickness(rPosition, radius, thickness, pPartsFilter,
                                                                                            pTriangleFilter);
    }

    s32 checkStrikeLineToMap(const TVec3f& rStart, const TVec3f& rDirection, s32 count, const CollisionPartsFilterBase* pPartsFilter,
                              const TriangleFilterBase* pTriangleFilter) {
        return MR::getCollisionDirector()->getCategoryKeeper(0)->checkStrikeLine(rStart, rDirection, count, pPartsFilter, pTriangleFilter);
    }

    s32 checkStrikeLineToSunshade(const TVec3f& rStart, const TVec3f& rDirection, s32 count, const CollisionPartsFilterBase* pPartsFilter,
                                   const TriangleFilterBase* pTriangleFilter) {
        return MR::getCollisionDirector()->getCategoryKeeper(1)->checkStrikeLine(rStart, rDirection, count, pPartsFilter, pTriangleFilter);
    }

    const HitInfo* getStrikeInfoMap(u32 index) {
        return MR::getCollisionDirector()->getCategoryKeeper(0)->getStrikeInfo(index);
    }

    u32 getStrikeInfoNumMap() {
        return ::getStrikeInfoNumCategory(0);
    }
}  // namespace Collision
