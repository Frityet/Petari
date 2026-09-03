// Complete original resource accessors; the full ModelManager/provider import
// replaces this translation unit when actor model ownership is activated.
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Animation/XanimeResource.hpp"
#include "Game/Util/LiveActorUtil.hpp"

ResourceHolder* ModelManager::getResourceHolder() const {
    if (mXanimeResourceTable == nullptr) {
        return mModelResourceHolder;
    }

    return mXanimeResourceTable->mResourceHolder;
}

namespace MR {
    ResourceHolder* getResourceHolder(const LiveActor* pActor) {
        if (pActor->mModelManager != nullptr) {
            return pActor->mModelManager->getResourceHolder();
        }

        return nullptr;
    }

}  // namespace MR
