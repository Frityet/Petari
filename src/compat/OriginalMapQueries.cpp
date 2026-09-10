// Original MapUtil entry points using the actual scene CollisionDirector.
#include "Game/Util/MapUtil.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/CollisionCategorizedKeeper.hpp"
#include "Game/Map/CollisionDirector.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Util/CollisionPartsFilter.hpp"
#include "Game/Util/TriangleFilter.hpp"

static HitInfo mSortBuffer[32];
static u32 mSortCount;

namespace {
    u32 getStrikeInfoNumCategory(s32 category) {
        return MR::getCollisionDirector()->getCategoryKeeper(category)->_10;
    }

    bool getFirstPolyOnLineCategory(TVec3f* pPos, Triangle* pTriangle, const TVec3f& rStart, const TVec3f& rOffset,
                                   const TriangleFilterBase* pTriangleFilter, const CollisionPartsFilterBase* pPartsFilter, s32 category) {
        u32 hitCount = MR::getCollisionDirector()->getCategoryKeeper(category)->checkStrikeLine(rStart, rOffset, 0, pPartsFilter, nullptr);
        if (hitCount == 0) {
            return false;
        }

        f32 distance = 1000000.0f;
        s32 nearest = -1;
        for (u32 i = 0; i < hitCount; i++) {
            HitInfo* pHit = MR::getCollisionDirector()->getCategoryKeeper(category)->getStrikeInfo(i);
            if (pTriangleFilter != nullptr && pTriangleFilter->isInvalidTriangle(&pHit->mParentTriangle)) {
                continue;
            }
            if (distance > pHit->_60) {
                nearest = i;
                distance = pHit->_60;
            }
        }
        if (nearest == -1) {
            return false;
        }

        HitInfo* pHit = MR::getCollisionDirector()->getCategoryKeeper(category)->getStrikeInfo(nearest);
        if (pPos != nullptr) {
            *pPos = pHit->mHitPos;
        }
        if (pTriangle != nullptr) {
            *pTriangle = pHit->mParentTriangle;
        }
        return true;
    }
}

namespace MR {
    bool getFirstPolyOnLineToMap(TVec3f* pPos, Triangle* pTriangle, const TVec3f& rStart, const TVec3f& rOffset) {
        return ::getFirstPolyOnLineCategory(pPos, pTriangle, rStart, rOffset, nullptr, nullptr, 0);
    }

    bool getFirstPolyOnLineToWaterSurface(TVec3f* pPos, Triangle* pTriangle, const TVec3f& rStart, const TVec3f& rOffset) {
        return ::getFirstPolyOnLineCategory(pPos, pTriangle, rStart, rOffset, nullptr, nullptr, 2);
    }

    bool getFirstPolyOnLineToMap(TVec3f* pPos, Triangle* pTriangle, const TVec3f& rStart, const TVec3f& rOffset,
                                const CollisionPartsFilterBase* pPartsFilter, const TriangleFilterBase* pTriangleFilter) {
        return ::getFirstPolyOnLineCategory(pPos, pTriangle, rStart, rOffset, pTriangleFilter, pPartsFilter, 0);
    }

    bool getFirstPolyOnLineToWaterSurface(TVec3f* pPos, Triangle* pTriangle, const TVec3f& rStart, const TVec3f& rOffset,
                                         const CollisionPartsFilterBase* pPartsFilter, const TriangleFilterBase* pTriangleFilter) {
        return ::getFirstPolyOnLineCategory(pPos, pTriangle, rStart, rOffset, pTriangleFilter, pPartsFilter, 2);
    }

    u32 getNearPolyOnLineSort(const TVec3f& rReference, const TVec3f& rStart, const TVec3f& rOffset, const HitSensor* pExceptSensor) {
        u32 hitCount = getCollisionDirector()->getCategoryKeeper(0)->checkStrikeLine(rStart, rOffset, 0, nullptr, nullptr);
        if (hitCount == 0) {
            return 0;
        }

        HitInfo* candidates[32];
        u32 excludedCount = 0;
        for (u32 i = 0; i < hitCount; i++) {
            candidates[i] = getCollisionDirector()->getCategoryKeeper(0)->getStrikeInfo(i);
            if (pExceptSensor != nullptr && candidates[i]->mParentTriangle.mSensor == pExceptSensor) {
                candidates[i] = nullptr;
                excludedCount++;
            }
        }

        mSortCount = hitCount - excludedCount;
        if (mSortCount >= 32) {
            mSortCount = 32;
        }
        for (u32 i = 0; i < mSortCount; i++) {
            f32 nearestDistance = 1000000.0f;
            u32 nearestIndex = 0;
            for (u32 j = 0; j < hitCount; j++) {
                if (candidates[j] == nullptr) {
                    continue;
                }
                HitInfo* info = getCollisionDirector()->getCategoryKeeper(0)->getStrikeInfo(j);
                TVec3f offset(rReference);
                offset.sub(info->mHitPos);
                f32 distance = PSVECMag(&offset);
                if (nearestDistance > distance) {
                    nearestIndex = j;
                    nearestDistance = distance;
                }
            }
            mSortBuffer[i] = *getCollisionDirector()->getCategoryKeeper(0)->getStrikeInfo(nearestIndex);
            candidates[nearestIndex] = nullptr;
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

    bool isExistMapCollisionExceptActor(const TVec3f& rStart, const TVec3f& rOffset, const LiveActor* pActor) {
        CollisionPartsFilterActor filter(pActor);
        return getCollisionDirector()->getCategoryKeeper(0)->checkStrikeLine(rStart, rOffset, 1, &filter, nullptr) != 0;
    }

    bool trySetMoveLimitCollision(LiveActor* pActor) {
        TVec3f start(pActor->mPosition);
        TVec3f offset(pActor->mGravity);
        start -= offset * 150.0f;
        offset *= 1000.0f;

        if (getCollisionDirector()->getCategoryKeeper(3)->checkStrikeLine(start, offset, 0, nullptr, nullptr) != 0) {
            HitInfo* pHit = getCollisionDirector()->getCategoryKeeper(3)->getStrikeInfo(0);
            pActor->mBinder->setExCollisionParts(pHit->mParentTriangle.mParts);
            return true;
        }

        if (getCollisionDirector()->getCategoryKeeper(0)->checkStrikeLine(start, offset, 0, nullptr, nullptr) != 0) {
            HitInfo* pHit = getCollisionDirector()->getCategoryKeeper(0)->getStrikeInfo(0);
            CollisionParts* pParts = nullptr;
            CollisionParts* pMapParts = pHit->mParentTriangle.mParts;
            getCollisionDirector()->getCategoryKeeper(3)->searchSameHostParts(&pParts, pMapParts);
            pActor->mBinder->setExCollisionParts(pParts);
            return true;
        }

        return false;
    }
}

namespace Collision {
    s32 checkStrikeLineToMap(const TVec3f& rStart, const TVec3f& rOffset, s32 maxCount,
                            const CollisionPartsFilterBase* pPartsFilter, const TriangleFilterBase* pTriangleFilter) {
        return MR::getCollisionDirector()->getCategoryKeeper(0)->checkStrikeLine(rStart, rOffset, maxCount, pPartsFilter, pTriangleFilter);
    }

    s32 checkStrikeLineToSunshade(const TVec3f& rStart, const TVec3f& rOffset, s32 maxCount,
                                 const CollisionPartsFilterBase* pPartsFilter, const TriangleFilterBase* pTriangleFilter) {
        return MR::getCollisionDirector()->getCategoryKeeper(1)->checkStrikeLine(rStart, rOffset, maxCount, pPartsFilter, pTriangleFilter);
    }

    const HitInfo* getStrikeInfoMap(u32 index) {
        return MR::getCollisionDirector()->getCategoryKeeper(0)->getStrikeInfo(index);
    }

    u32 getStrikeInfoNumMap() {
        return ::getStrikeInfoNumCategory(0);
    }
}
