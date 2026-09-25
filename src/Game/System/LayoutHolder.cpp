#include "Game/System/LayoutHolder.hpp"
#include "Game/System/FileLoader.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "resource/RarcArchive.hpp"
#include "resource/GameResourceRuntime.hpp"
#include "layout/LytTexMap.hpp"
#include "layout/NativeLayoutResource.hpp"
#include <map>
#include <JSystem/JKernel/JKRArchive.hpp>
#include <JSystem/JKernel/JKRFileFinder.hpp>
#include <cstdio>
#include <cstring>
#include <aurora/endian.hpp>
#include <aurora/exception.hpp>
#include <stdexcept>

extern "C" int strncasecmp(const char*, const char*, size_t);

namespace {
    const char* sLayoutExt[] = {
        ".brlyt",
        nullptr,
    };
    const char* sAnimationExt[] = {
        ".brlan",
        nullptr,
    };
};  // namespace

struct LayoutHolder::NativeResources {
    std::shared_ptr<smgpc::compat::JkrAllocationDomain> domain;
    std::shared_ptr<const void> archiveLifetime;
    std::shared_ptr<const smgpc::resource::RarcArchive> source;
    std::filesystem::path path;
    std::map<std::string, std::shared_ptr<const nw4r::lyt::HostTextureResourceState>, std::less<>> textures;
};

LayoutHolder::LayoutHolder(JKRArchive& rArchive) : nw4r::lyt::ResourceAccessor(), mArchive(&rArchive) {
    try {
        {
            const aurora::allocation::HostAllocationScope host;
            mNativeResources = std::make_shared<NativeResources>();
            mNativeResources->domain = smgpc::compat::JkrAllocationDomain::retain_heap(*MR::getCurrentHeap());
            mNativeResources->archiveLifetime = rArchive.retainNativeResources();
            mNativeResources->source = rArchive.retainSource();
            mNativeResources->path = rArchive.mLoaderName ? rArchive.mLoaderName : "";
            if (const auto* loader = SingletonHolder<FileLoader>::get())
                if (const auto* entry = loader->mArchiveHolder->findEntry(&rArchive))
                    mNativeResources->path = entry->mArchiveName;
        }
        const smgpc::compat::JkrAllocationScope original(mNativeResources->domain);
        initializeArc();
    } catch (...) {
        destroyNativeResources();
        throw;
    }
}

LayoutHolder::~LayoutHolder() {
    destroyNativeResources();
}

void LayoutHolder::destroyNativeResources() noexcept {
    const aurora::allocation::HostAllocationScope host;
    for (auto* table : {&mLayoutRes, &mAnimRes, &mResOther}) {
        for (u32 i = 0; i < table->mCount; ++i)
            delete[] table->mFileInfoTable[i].mName;
        delete[] table->mFileInfoTable;
        table->mFileInfoTable = nullptr;
        table->mCount = 0;
    }
    mNativeResources.reset();
}

std::shared_ptr<const void> LayoutHolder::retainNativeResources() const {
    return mNativeResources;
}

std::size_t LayoutHolder::nativeArchiveReferenceCount() const noexcept {
    return 1;
}

void LayoutHolder::ensureNativeResourcesUnborrowed() const {
    if (mNativeResources.use_count() != 1)
        aurora::throw_host_exception<std::logic_error>("Cannot unload an original resource heap with live layout owners");
}

const smgpc::resource::RarcArchive& LayoutHolder::nativeResourceSource() const {
    return *mNativeResources->source;
}

const std::filesystem::path& LayoutHolder::nativeResourcePath() const {
    return mNativeResources->path;
}

JKRHeap& LayoutHolder::heap() const noexcept {
    return mNativeResources->domain->heap();
}

void* LayoutHolder::GetResource(u32 type, const char* pName, u32* pSize) {
    void* resource = nullptr;
    switch (type) {
    case 'blyt':
        resource = mLayoutRes.getRes(pName);
        break;
    case 'anim':
        resource = mAnimRes.getRes(pName);
        break;
    default:
        if (strstr(pName, ".brfnt") == nullptr) {
            resource = mResOther.getRes(pName);
        }
        break;
    }

    if (resource == nullptr) {
        if (pSize) *pSize = 0;
        return nullptr;
    }
    const u32 size = mArchive->getResSize(resource);
    // Preserve the original accessor's metadata word, not a host object size.
    if (pSize) {
        if (size < 8)
            aurora::throw_host_exception<std::runtime_error>("Layout resource has no metadata word");
        *pSize = aurora::endian::read_u32(static_cast<const u8*>(resource) + 4);
    }
    if (type == 'timg') {
        // NW4R consumes widened descriptors; packed Wii offsets never become
        // native pointers. The cached backing is also retained by materials.
        const aurora::allocation::HostAllocationScope host;
        auto it = mNativeResources->textures.find(pName);
        if (it == mNativeResources->textures.end()) {
            auto* runtime = smgpc::resource::GameResourceRuntime::active();
            auto texture = smgpc::layout::make_tex_map(pName,
                std::span(static_cast<const std::uint8_t*>(resource), size),
                runtime ? runtime->mem1_heap() : nullptr);
            auto state = texture.GetHostResourceState();
            if (!state || !state->tpl_palette)
                aurora::throw_host_exception<std::runtime_error>("Layout material requires a native TPL palette");
            it = mNativeResources->textures.emplace(pName, std::move(state)).first;
        }
        return const_cast<TPLPalette*>(it->second->tpl_palette);
    }
    return resource;
}

std::shared_ptr<const nw4r::lyt::HostTextureResourceState> LayoutHolder::GetHostTextureResourceState(const char* pName) {
    GetResource('timg', pName, nullptr);
    const auto it = mNativeResources->textures.find(pName);
    return it == mNativeResources->textures.end() ? nullptr : it->second;
}

nw4r::ut::Font* LayoutHolder::GetFont(const char* pName) {
    if (strncasecmp(pName, "MessageFont26", strlen("MessageFont26")) == 0) {
        return MR::getFontOnCurrentLanguage();
    }
    if (strncasecmp(pName, "MenuFont64", strlen("MenuFont64")) == 0) {
        return MR::getMenuFontNW4R();
    }
    if (strncasecmp(pName, "NumberFont", strlen("NumberFont")) == 0) {
        return MR::getNumberFontNW4R();
    }
    if (strncasecmp(pName, "PictureFont", strlen("PictureFont")) == 0) {
        return MR::getPictureFontNW4R();
    }
    if (strncasecmp(pName, "CinemaFont26", strlen("CinemaFont26")) == 0) {
        return MR::getCinemaFontNW4R();
    }
    return MR::getFontOnCurrentLanguage();
}

bool LayoutHolder::isAnimationHashEqual(u32 hash, u32 index) const {
    return mAnimRes.getFileInfo(index)->mHashCode == hash;
}

void LayoutHolder::initializeArc() {
    u32 resourceCount = mArchive->countResource();
    resourceCount -= initEachResTable(&mLayoutRes, sLayoutExt);
    resourceCount -= initEachResTable(&mAnimRes, sAnimationExt);
    if (resourceCount != 0) {
        mResOther.newFileInfoTable(resourceCount);
    }

    mount(nullptr);
}

JKRFileFinder* LayoutHolder::getFileFinder(const char* pPath) {
    if (pPath == nullptr) {
        return mArchive->getFirstFile("/");
    }
    return mArchive->getFirstFile(pPath);
}

u32 LayoutHolder::initEachResTable(ResTable* pTable, const char* const* pExtensions) {
    s32 resourceCount = 0;
    for (u32 i = 0; pExtensions[i] != nullptr; i++) {
        resourceCount += count(pExtensions[i], nullptr);
    }
    if (resourceCount != 0) {
        pTable->newFileInfoTable(resourceCount);
    }
    return resourceCount;
}

s32 LayoutHolder::count(const char* pExtension, const char* pPath) {
    s32 resourceCount = 0;
    std::unique_ptr<JKRFileFinder> finder(getFileFinder(pPath));
    while (finder->mHasMoreFiles) {
        if (finder->mFileIsFolder) {
            if (finder->mName[0] != '.') {
                char path[128];
                sprintf(path, "%s%s%s", pPath, "/", finder->mName);
                resourceCount += count(pExtension, path);
            }
        } else if (pExtension == nullptr || strstr(finder->mName, pExtension) != nullptr) {
            resourceCount++;
        }
        finder->findNextFile();
    }
    return resourceCount;
}

void LayoutHolder::mount(char* pPath) {
    std::unique_ptr<JKRFileFinder> finder(getFileFinder(pPath));
    while (finder->mHasMoreFiles) {
        if (finder->mFileIsFolder) {
            if (finder->mName[0] != '.') {
                char path[128];
                snprintf(path, 128, "%s/%s", pPath, finder->mName);
                mount(path);
            }
        } else {
            mArchive->getFileAttribute(finder->mFileID);
            ResFileInfo* info = createAndRegisterObject(finder->mName, mArchive->getResource(finder->mFileID));
            info->_8 = mArchive->getResource(finder->mFileID);
            info->_4 = mArchive->getResSize(info->_8);
            info->_C = finder->mFileID;
        }
        finder->findNextFile();
    }
}

ResFileInfo* LayoutHolder::createAndRegisterObject(const char* pName, void* pResource) {
    if (strstr(pName, ".brlyt") != nullptr || strstr(pName, ".brlan") != nullptr) {
        smgpc::layout::NativeLayoutResource::validate_archive_span(
            std::span(static_cast<const std::uint8_t*>(pResource), mArchive->getResSize(pResource)));
    }
    if (strstr(pName, ".brlyt") != nullptr) {
        return mLayoutRes.add(pName, pResource, false);
    }
    if (strstr(pName, ".brlan") != nullptr) {
        return mAnimRes.add(pName, pResource, false);
    }
    return mResOther.add(pName, pResource, false);
}

void* LayoutHolder::getResOther(const char* pName) const {
    return mResOther.getRes(pName);
}

u32 LayoutHolder::getResOtherNum() const {
    return mResOther.mCount;
}

const char* LayoutHolder::getResOtherName(u32 index) const {
    return mResOther.getResName(index);
}

void* LayoutHolder::getResOther(u32 index) const {
    return mResOther.getRes(index);
}

bool LayoutHolder::isExistResOther(const char* pName) const {
    return mResOther.isExistRes(pName);
}
