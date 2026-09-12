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
LayoutHolder* ResourceHolderManager::createAndAddLayoutHolder(const char* name, JKRHeap*) {
    return resources().create_layout(name);
}
LayoutHolder* ResourceHolderManager::createAndAddLayoutHolderStationed(const char* name) {
    return resources().create_layout_from_mounted(name);
}
LayoutHolder* ResourceHolderManager::createAndAddLayoutHolderRawData(const char* name) {
    return resources().create_layout_from_mounted(name);
}
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
