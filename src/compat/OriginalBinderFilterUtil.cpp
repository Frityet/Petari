// Original LiveActorUtil Binder filter ownership and dispatch.
#include "Game/LiveActor/Binder.hpp"
#include "Game/Util/CollisionPartsFilter.hpp"
#include "Game/Util/LiveActorUtil.hpp"

namespace MR {
    void setBinderCollisionPartsFilter(LiveActor* pActor, CollisionPartsFilterBase* pFilter) {
        pActor->mBinder->setCollisionPartsFilter(pFilter);
    }

    void setBinderExceptActor(LiveActor* pActor, const LiveActor* pExcept) {
        CollisionPartsFilterActor* pFilter = new CollisionPartsFilterActor(pExcept);
        pActor->mBinder->setCollisionPartsFilter(pFilter);
    }

    void setBindTriangleFilter(LiveActor* pActor, TriangleFilterBase* pTriangleFilter) {
        pActor->mBinder->setTriangleFilter(pTriangleFilter);
    }

}  // namespace MR
