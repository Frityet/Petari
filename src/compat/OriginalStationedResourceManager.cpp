#include "Game/System/ResourceHolderManager.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "Game/Util/FileUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "compat/ResourceHolderCompat.hpp"

#include <aurora/exception.hpp>
#include <stdexcept>

namespace {
    smgpc::compat::ResourceHolderService& resources() {
        auto* owner = smgpc::compat::ResourceHolderService::active();
        if (owner == nullptr)
            aurora::throw_host_exception<std::logic_error>("Original ResourceHolderManager requires its native archive resource owner");
        return *owner;
    }
    [[noreturn]] void unavailable_layout_holder() {
        aurora::throw_host_exception<std::logic_error>(
            "Original stationed LayoutHolder resource construction is not implemented");
    }
}

ResourceHolderManager::ResourceHolderManager() { mResourceArray.clear(); }
ResourceHolder* ResourceHolderManager::createAndAdd(const char* name, JKRHeap*) {
    return resources().create_and_add(name);
}
ResourceHolder* ResourceHolderManager::createAndAddStationed(const char* name) {
    JKRArchive* archive = nullptr;
    JKRHeap* heap = nullptr;
    MR::getMountedArchiveAndHeap(name, &archive, &heap);
    if (archive == nullptr || heap == nullptr)
        aurora::throw_host_exception<std::logic_error>("Stationed resource creation requires its original mounted archive and heap");
    return resources().create_and_add(name, heap);
}
LayoutHolder* ResourceHolderManager::createAndAddLayoutHolder(const char*, JKRHeap*) { unavailable_layout_holder(); }
LayoutHolder* ResourceHolderManager::createAndAddLayoutHolderStationed(const char*) { unavailable_layout_holder(); }
LayoutHolder* ResourceHolderManager::createAndAddLayoutHolderRawData(const char*) { unavailable_layout_holder(); }
void ResourceHolderManager::removeIfIsEqualHeap(JKRHeap* heap) { resources().remove_for_heap(heap); }

ResourceHolderManagerName2Resource::ResourceHolderManagerName2Resource()
    : mResourceHolder(nullptr), mLayoutHolder(nullptr), mHeap(nullptr) {}
ResourceHolderManagerName2Resource& ResourceHolderManagerName2Resource::operator=(const ResourceHolderManagerName2Resource& other) {
    mResourceHolder = other.mResourceHolder;
    mLayoutHolder = other.mLayoutHolder;
    mHash = other.mHash;
    mHeap = other.mHeap;
    return *this;
}
CreateResourceHolderArgs::CreateResourceHolderArgs() : mResourceHolder(nullptr), mLayoutHolder(nullptr), mHeap(nullptr) {}
