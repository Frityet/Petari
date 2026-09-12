#include "Game/System/LayoutHolder.hpp"
#include "Game/Util/SystemUtil.hpp"
#include <JSystem/JKernel/JKRArchive.hpp>
#include <JSystem/JKernel/JKRFileFinder.hpp>
#include <cstdio>
#include <cstring>
#include <aurora/endian.hpp>

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

LayoutHolder::LayoutHolder(JKRArchive& rArchive) : nw4r::lyt::ResourceAccessor(), mArchive(&rArchive) {
    initializeArc();
}

LayoutHolder::~LayoutHolder() {
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

    if (pSize != nullptr) {
        // Wii resources retain their original byte order and may be unaligned on the host.
        const u8* bytes = static_cast< const u8* >(resource);
        *pSize = bytes != nullptr ? aurora::endian::read_u32(bytes + 4) : 0;
    }
    return resource;
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
    JKRFileFinder* finder = getFileFinder(pPath);
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
    delete finder;
    return resourceCount;
}

void LayoutHolder::mount(char* pPath) {
    JKRFileFinder* finder = getFileFinder(pPath);
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
    delete finder;
}

ResFileInfo* LayoutHolder::createAndRegisterObject(const char* pName, void* pResource) {
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
