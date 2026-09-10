// Original MapUtil entry points using the actual scene CollisionDirector.
#include "Game/Util/MapUtil.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/CollisionCategorizedKeeper.hpp"
#include "Game/Map/CollisionDirector.hpp"

namespace MR {
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
