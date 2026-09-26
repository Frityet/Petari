#include "JSystem/JKernel/JKRArchive.hpp"
#include "JSystem/JKernel/JKRFileFinder.hpp"

JKRArchive *JKRArchive::check_mount_already(std::uintptr_t identity) {
    VolumeLock lock;
    for (auto *link = sFileLoaderList.getFirst(); link != nullptr; link = link->getNext()) {
        auto *loader = link->getObject();
        if (loader->mLoaderType != 0x52415243)
            continue;
        auto *archive = static_cast<JKRArchive *>(loader);
        if (archive->mEntryNum == identity) {
            // The retail lookup increments the existing mount's reference
            // count even when mountFixed subsequently rejects the duplicate.
            ++archive->_34;
            return archive;
        }
    }
    return nullptr;
}

s32 JKRArchive::countFile(const char *pName) const {
    SDIDirEntry *dir;

    if (*pName == '/') {
        pName++;

        if (*pName == 0) {
            pName = nullptr;
        }

        dir = findDirectory(pName, 0);
    } else {
        dir = findDirectory(pName, sCurrentDirID);
    }

    if (dir != nullptr) {
        return dir->mNrFiles;
    }

    return 0;
}

JKRArcFinder *JKRArchive::getFirstFile(const char *pName) const {
    SDIDirEntry *dir;

    if (*pName == '/') {
        pName++;

        if (*pName == 0) {
            pName = nullptr;
        }

        dir = findDirectory(pName, 0);
    } else {
        dir = findDirectory(pName, sCurrentDirID);
    }

    if (dir != nullptr) {
        // Bad to cast to non-const
        return new JKRArcFinder(const_cast<JKRArchive *>(this), dir->mFirstFileIndex, dir->mNrFiles);
    }

    return nullptr;
}

bool JKRArchive::getDirEntry(SDirEntry *pDir, u32 fileIndex) const {
    SDIFileEntry *file = findIdxResource(fileIndex);

    if (file == nullptr) {
        return false;
    }

    pDir->mFileFlag = file->mFlag;
    pDir->mFileID = file->mFileID;
    pDir->mName = mStringTable + file->mNameOffset;

    return true;
}

u32 JKRArchive::countResource() const {
    if (mInfoBlock == nullptr)
        return 0;
    u32 count = 0;

    for (u32 i = 0; i < mInfoBlock->mNrFiles; i++) {
        if ((mFiles[i].mFlag & FILE_FLAG_FILE) != 0) {
            count++;
        }
    }

    return count;
}

u32 JKRArchive::getFileAttribute(u32 fileIndex) const {
    SDIFileEntry *file = findIdxResource(fileIndex);

    if (file != nullptr) {
        return file->mFlag;
    }

    return 0;
}

void *JKRArchive::getIdxResource(u32 fileIndex) {
    SDIFileEntry *file = findIdxResource(fileIndex);

    if (file != nullptr) {
        return fetchResource(file, 0);
    }

    return nullptr;
}

void *JKRArchive::getResource(const char *pPath) const {
    const auto* entry = mArchive && pPath ? mArchive->find_resource(pPath) : nullptr;
    return entry ? const_cast<JKRArchive*>(this)->fetchResource(findIdxResource(entry->file_entry_index), nullptr) : nullptr;
}

void *JKRArchive::getResource(std::uint32_t, const char *pPath) const {
    return getResource(pPath);
}

void *JKRArchive::getResource(std::uint16_t id) const {
    auto* file = findIdResource(id);
    return file ? const_cast<JKRArchive*>(this)->fetchResource(file, nullptr) : nullptr;
}

std::uint32_t JKRArchive::getResSize(const void *pResource) const {
    if (pResource == nullptr || mArchive == nullptr) {
        return 0U;
    }

    for (const auto &entry : mArchive->entries()) {
        const auto data = resource_data(entry);
        if (!data.empty() && data.data() == pResource) {
            return static_cast<std::uint32_t>(data.size());
        }
    }

    return 0U;
}

std::uint32_t JKRArchive::readResource(void *pBuffer, std::uint32_t bufferSize, const char *pPath) const {
    const auto* entry = mArchive && pPath ? mArchive->find_resource(pPath) : nullptr;
    if (!entry || !pBuffer || !bufferSize) return 0;
    u32 size;
    fetchResource(pBuffer, bufferSize, findIdxResource(entry->file_entry_index), &size);
    return size;
}

std::uint32_t JKRArchive::readResource(void *pBuffer, std::uint32_t bufferSize, std::uint16_t fileId) const {
    SDIFileEntry* file = findIdResource(fileId);
    if (!file || !pBuffer || !bufferSize) return 0;
    u32 size;
    fetchResource(pBuffer, bufferSize, file, &size);
    return size;
}

bool JKRArchive::contains(const char *pPath) const {
    return mArchive != nullptr && pPath != nullptr && mArchive->contains_resource(pPath);
}

std::uint32_t JKRArchive::getExpandedResSize(const void* resource) const {
    return getResSize(resource);
}
