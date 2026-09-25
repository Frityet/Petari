#include "Game/System/ResourceHolderManager.hpp"
#include "Game/System/LayoutHolder.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "Game/Util/FileUtil.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/HashUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "Game/Util/StringUtil.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include <dolphin/gd/GDBase.h>
#include <aurora/exception.hpp>
#include <memory>
#include <stdexcept>

namespace {
    class GDCurrentRestorer {
    public:
        GDCurrentRestorer() : mObj(__GDCurrentDL) {
        }

        ~GDCurrentRestorer() {
            __GDCurrentDL = mObj;
        }

    private:
        /* 0x0 */ GDLObj* mObj;
    };
};  // namespace

ResourceHolderManager::ResourceHolderManager() {
    mResourceArray.clear();
}

// Native process teardown first checks every holder, so a live borrower cannot
// leave the registry partially retired. The process owner performs this check
// before releasing the singleton and before retiring FileLoader or its heaps.
ResourceHolderManager::~ResourceHolderManager() {
    validateRetirement();
    for (ResourceHolderManagerName2Resource* pIter = mResourceArray.begin(); pIter != mResourceArray.end(); pIter++) {
        const auto heap = smgpc::compat::JkrAllocationDomain::retain_heap(*pIter->mHeap);
        delete pIter->mResourceHolder;
        delete pIter->mLayoutHolder;
    }
    mResourceArray.clear();
}

void ResourceHolderManager::validateRetirement() const {
    for (const ResourceHolderManagerName2Resource* pIter = mResourceArray.begin(); pIter != mResourceArray.end(); pIter++) {
        if (pIter->mResourceHolder != nullptr) {
            pIter->mResourceHolder->ensureNativeResourcesUnborrowed();
        }
        if (pIter->mLayoutHolder != nullptr) {
            pIter->mLayoutHolder->ensureNativeResourcesUnborrowed();
        }
    }
}

void ResourceHolderManager::validateHeapRetirement(JKRHeap* pHeap) const {
    if (pHeap == nullptr) {
        return;
    }
    for (const ResourceHolderManagerName2Resource* pIter = mResourceArray.begin(); pIter != mResourceArray.end(); pIter++) {
        if (pIter->mHeap != pHeap && MR::getHeapNapa(pIter->mHeap) != pHeap && MR::getHeapGDDR3(pIter->mHeap) != pHeap) {
            continue;
        }
        if (pIter->mResourceHolder != nullptr) {
            pIter->mResourceHolder->ensureNativeResourcesUnborrowed();
        }
        if (pIter->mLayoutHolder != nullptr) {
            pIter->mLayoutHolder->ensureNativeResourcesUnborrowed();
        }
    }
}

u32 ResourceHolderManager::countNativeArchiveReferences(const JKRArchive* pArchive, JKRHeap* pHeap) const {
    u32 count = 0;
    if (pHeap == nullptr) {
        return count;
    }
    for (const ResourceHolderManagerName2Resource* pIter = mResourceArray.begin(); pIter != mResourceArray.end(); pIter++) {
        if (pIter->mHeap != pHeap && MR::getHeapNapa(pIter->mHeap) != pHeap && MR::getHeapGDDR3(pIter->mHeap) != pHeap) {
            continue;
        }
        if (pIter->mResourceHolder != nullptr && pIter->mResourceHolder->mArchive == pArchive) {
            count += pIter->mResourceHolder->nativeArchiveReferenceCount();
        }
        if (pIter->mLayoutHolder != nullptr && pIter->mLayoutHolder->mArchive == pArchive) {
            count += pIter->mLayoutHolder->nativeArchiveReferenceCount();
        }
    }
    return count;
}

ResourceHolder* ResourceHolderManager::createAndAdd(const char* pArcName, JKRHeap*) {
    return createAndAddInner(pArcName, MR::makeObjectArchiveFileName, startCreateResourceHolderOnMainThread)->mResourceHolder;
}

ResourceHolder* ResourceHolderManager::createAndAddStationed(const char* pParam1) {
    return createAndAddInnerStationed(pParam1, &ResourceHolderManager::createResourceHolder)->mResourceHolder;
}

LayoutHolder* ResourceHolderManager::createAndAddLayoutHolder(const char* pParam1, JKRHeap*) {
    return createAndAddInner(pParam1, MR::makeLayoutArchiveFileName, startCreateLayoutHolderOnMainThread)->mLayoutHolder;
}

LayoutHolder* ResourceHolderManager::createAndAddLayoutHolderStationed(const char* pParam1) {
    return createAndAddInnerStationed(pParam1, &ResourceHolderManager::createLayoutHolder)->mLayoutHolder;
}

LayoutHolder* ResourceHolderManager::createAndAddLayoutHolderRawData(const char* pParam1) {
    FuncPtrC create = &ResourceHolderManager::createLayoutHolder;
    CreateResourceHolderArgs args = CreateResourceHolderArgs();
    (this->*create)(pParam1, &args);

    return add(pParam1, args)->mLayoutHolder;
}

void ResourceHolderManager::removeIfIsEqualHeap(JKRHeap* pHeap) {
    validateHeapRetirement(pHeap);

    if (pHeap == nullptr) {
        return;
    }

    for (ResourceHolderManagerName2Resource* pIter = mResourceArray.begin(); pIter != mResourceArray.end(); pIter++) {
        if (pIter->mHeap != pHeap && MR::getHeapNapa(pIter->mHeap) != pHeap && MR::getHeapGDDR3(pIter->mHeap) != pHeap) {
            continue;
        }

        // Keep the actual allocation domain alive through the holder's final
        // delete, after its embedded native backing has released its own token.
        const auto heap = smgpc::compat::JkrAllocationDomain::retain_heap(*pIter->mHeap);
        if (pIter->mResourceHolder != nullptr) {
            delete pIter->mResourceHolder;
        }

        if (pIter->mLayoutHolder != nullptr) {
            delete pIter->mLayoutHolder;
        }

        pIter->mHeap = nullptr;
        pIter->mLayoutHolder = nullptr;
        pIter->mResourceHolder = nullptr;
    }

    for (ResourceHolderManagerName2Resource* pIter = mResourceArray.begin(); pIter != mResourceArray.end();) {
        if (pIter->mHeap == nullptr) {
            mResourceArray.erase(pIter);
        } else {
            pIter++;
        }
    }
}

void ResourceHolderManager::startCreateResourceHolderOnMainThread(const char* pParam1, CreateResourceHolderArgs* pArgs) {
    MR::FunctorV2M< ResourceHolderManager*, FuncPtrC, const char*, CreateResourceHolderArgs* > func =
        MR::Functor(SingletonHolder< ResourceHolderManager >::get(), &ResourceHolderManager::createResourceHolder, pParam1, pArgs);

    MR::startFunctionAsyncExecuteOnMainThread(func, pParam1);
}

void ResourceHolderManager::startCreateLayoutHolderOnMainThread(const char* pParam1, CreateResourceHolderArgs* pArgs) {
    MR::FunctorV2M< ResourceHolderManager*, FuncPtrC, const char*, CreateResourceHolderArgs* > func =
        MR::Functor(SingletonHolder< ResourceHolderManager >::get(), &ResourceHolderManager::createLayoutHolder, pParam1, pArgs);

    MR::startFunctionAsyncExecuteOnMainThread(func, pParam1);
}

ResourceHolderManagerName2Resource* ResourceHolderManager::createAndAddInner(const char* pArcName,
                                                                             MakeArchiveFileNameFuncPtr pMakeArchiveFileNameFunc, FuncPtrB pParam3) {
    ResourceHolderManagerName2Resource* pName2Resource = find(pArcName);

    if (pName2Resource != nullptr) {
        return pName2Resource;
    }

    char archivePath[128];
    (*pMakeArchiveFileNameFunc)(archivePath, sizeof(archivePath), pArcName);

    if (MR::receiveArchive(archivePath) == nullptr) {
        MR::mountArchive(archivePath, MR::getCurrentHeap());
    }

    CreateResourceHolderArgs args = CreateResourceHolderArgs();

    (*pParam3)(archivePath, &args);
    MR::waitForEndFunctionAsyncExecute(archivePath);

    return add(archivePath, args);
}

ResourceHolderManagerName2Resource* ResourceHolderManager::createAndAddInnerStationed(const char* pParam1, FuncPtrC pParam2) {
    CreateResourceHolderArgs args = CreateResourceHolderArgs();

    MR::startFunctionAsyncExecuteOnMainThread(MR::Functor(this, pParam2, pParam1, &args), pParam1);

    ResourceHolderManagerName2Resource* pName2Resource = add(pParam1, args);

    return pName2Resource;
}

void ResourceHolderManager::createResourceHolder(const char* pParam1, CreateResourceHolderArgs* pArgs) {
    ::GDCurrentRestorer gdRestorer = ::GDCurrentRestorer();
    JKRArchive* pArchive = nullptr;

    MR::getMountedArchiveAndHeap(pParam1, &pArchive, &pArgs->mHeap);
    if (pArchive == nullptr || pArgs->mHeap == nullptr) {
        aurora::throw_host_exception<std::logic_error>("ResourceHolderManager requires the original mounted archive and heap");
    }
    MR::CurrentHeapRestorer heapRestorer(pArgs->mHeap);

    pArgs->mResourceHolder = new ResourceHolder(*pArchive);
}

void ResourceHolderManager::createLayoutHolder(const char* pParam1, CreateResourceHolderArgs* pArgs) {
    ::GDCurrentRestorer gdRestorer = ::GDCurrentRestorer();
    JKRArchive* pArchive = nullptr;

    MR::getMountedArchiveAndHeap(pParam1, &pArchive, &pArgs->mHeap);
    if (pArchive == nullptr || pArgs->mHeap == nullptr) {
        aurora::throw_host_exception<std::logic_error>("ResourceHolderManager requires the original mounted archive and heap");
    }
    MR::CurrentHeapRestorer heapRestorer(pArgs->mHeap);

    pArgs->mLayoutHolder = new LayoutHolder(*pArchive);
}

ResourceHolderManagerName2Resource* ResourceHolderManager::add(const char* pParam1, const CreateResourceHolderArgs& rArgs) {
    // Own newly constructed holders even when the original fixed registry is
    // exhausted. Do not publish a partial entry or leak its native resources.
    const auto heap = smgpc::compat::JkrAllocationDomain::retain_heap(*rArgs.mHeap);
    std::unique_ptr<ResourceHolder> resource(rArgs.mResourceHolder);
    std::unique_ptr<LayoutHolder> layout(rArgs.mLayoutHolder);
    if (mResourceArray.size() == mResourceArray.capacity()) {
        aurora::throw_host_exception<std::length_error>("ResourceHolderManager registry is full");
    }
    ResourceHolderManagerName2Resource name2Resource = ResourceHolderManagerName2Resource();
    name2Resource.mHash = MR::getHashCodeLower(MR::getBasename(pParam1));
    name2Resource.mResourceHolder = rArgs.mResourceHolder;
    name2Resource.mLayoutHolder = rArgs.mLayoutHolder;
    name2Resource.mHeap = rArgs.mHeap;

    mResourceArray.push_back(name2Resource);
    resource.release();
    layout.release();

    return &mResourceArray[mResourceArray.size() - 1];
}

ResourceHolderManagerName2Resource* ResourceHolderManager::find(const char* pParam1) {
    ResourceHolderManagerName2Resource name2Resource = ResourceHolderManagerName2Resource();
    name2Resource.mHash = MR::getHashCodeLower(pParam1);

    ResourceHolderManagerName2Resource* pIter;
    ResourceHolderManagerName2Resource* pEnd = mResourceArray.end();

    for (pIter = mResourceArray.begin(); pIter != pEnd && pIter->mHash != name2Resource.mHash; pIter++) {
    }

    if (pIter == pEnd) {
        return nullptr;
    }

    return pIter;
}

ResourceHolderManagerName2Resource::ResourceHolderManagerName2Resource() : mResourceHolder(), mLayoutHolder(), mHeap() {
}

ResourceHolderManagerName2Resource& ResourceHolderManagerName2Resource::operator=(const ResourceHolderManagerName2Resource& rOther) {
    mResourceHolder = rOther.mResourceHolder;
    mLayoutHolder = rOther.mLayoutHolder;
    mHash = rOther.mHash;
    mHeap = rOther.mHeap;

    return *this;
}

CreateResourceHolderArgs::CreateResourceHolderArgs() : mResourceHolder(), mLayoutHolder(), mHeap() {
}
