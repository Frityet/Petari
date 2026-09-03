#include "JSystem/JKernel/JKRArchive.hpp"
#include "JSystem/JKernel/JKRFileFinder.hpp"

#include <cctype>
#include <cstring>
#include <limits>
#include <stdexcept>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (static_cast<s32>(sizeof(array) / sizeof((array)[0])))
#endif

namespace {
    using Bytes = std::span<const std::uint8_t>;

    void require_range(Bytes data, std::size_t offset, std::size_t size) {
        if (offset > data.size() || size > data.size() - offset) {
            throw std::invalid_argument("JKR archive metadata extends outside its retained resource");
        }
    }

    u16 read_u16(Bytes data, std::size_t offset) {
        require_range(data, offset, 2);
        return static_cast<u16>((u16(data[offset]) << 8) | data[offset + 1]);
    }

    u32 read_u32(Bytes data, std::size_t offset) {
        require_range(data, offset, 4);
        return (u32(data[offset]) << 24) | (u32(data[offset + 1]) << 16) |
               (u32(data[offset + 2]) << 8) | data[offset + 3];
    }
}

u32 JKRArchive::sCurrentDirID = 0;

JKRArchive::JKRArchive(const smgpc::resource::RarcArchive* archive) {
    attach_archive(archive);
}

void JKRArchive::attach_archive(const smgpc::resource::RarcArchive* archive) {
    if (archive == nullptr) {
        return;
    }
    const Bytes bytes = archive->bytes();
    const std::size_t info = read_u32(bytes, 8);
    require_range(bytes, info, 0x20);
    mNativeInfo = {read_u32(bytes, info), read_u32(bytes, info + 4),
                   read_u32(bytes, info + 8), read_u32(bytes, info + 12),
                   read_u32(bytes, info + 16), read_u32(bytes, info + 20),
                   read_u16(bytes, info + 24), read_u16(bytes, info + 26),
                   read_u32(bytes, info + 28)};
    const std::size_t directories = info + mNativeInfo.mDirOffset;
    const std::size_t files = info + mNativeInfo.mFileOffset;
    const std::size_t strings = info + mNativeInfo.mStringTableOffset;
    require_range(bytes, directories, std::size_t(mNativeInfo.mNrDirs) * 0x10);
    require_range(bytes, files, std::size_t(mNativeInfo.mNrFiles) * 0x14);
    require_range(bytes, strings, mNativeInfo.mStringTableSize);
    if (mNativeInfo.mNrDirs == 0 || mNativeInfo.mNrFiles > std::numeric_limits<s32>::max()) {
        throw std::invalid_argument("JKR archive directory/file count cannot be represented");
    }
    mNativeStrings.assign(bytes.begin() + strings, bytes.begin() + strings + mNativeInfo.mStringTableSize);
    const auto validate_name = [&](u32 offset) {
        if (offset >= mNativeStrings.size()) {
            throw std::invalid_argument("JKR archive name offset is outside its string table");
        }
        const void* end = std::memchr(mNativeStrings.data() + offset, 0, mNativeStrings.size() - offset);
        if (end == nullptr || static_cast<const char*>(end) - (mNativeStrings.data() + offset) >= 256) {
            throw std::invalid_argument("JKR archive name is unterminated or exceeds the original lookup buffer");
        }
    };
    mNativeDirs.reserve(mNativeInfo.mNrDirs);
    for (u32 i = 0; i < mNativeInfo.mNrDirs; ++i) {
        const std::size_t offset = directories + std::size_t(i) * 0x10;
        SDIDirEntry dir{read_u32(bytes, offset), read_u32(bytes, offset + 4),
                        read_u16(bytes, offset + 8), read_u16(bytes, offset + 10),
                        read_u32(bytes, offset + 12)};
        validate_name(dir.mNameOffset);
        if (dir.mFirstFileIndex > mNativeInfo.mNrFiles ||
            dir.mNrFiles > mNativeInfo.mNrFiles - dir.mFirstFileIndex) {
            throw std::invalid_argument("JKR archive directory range is outside its file table");
        }
        mNativeDirs.push_back(dir);
    }
    mNativeFiles.reserve(mNativeInfo.mNrFiles);
    for (u32 i = 0; i < mNativeInfo.mNrFiles; ++i) {
        const std::size_t offset = files + std::size_t(i) * 0x14;
        SDIFileEntry file{};
        file.mFileID = read_u16(bytes, offset);
        file.mHash = read_u16(bytes, offset + 2);
        const u32 flags_and_name = read_u32(bytes, offset + 4);
        file.mFlag = flags_and_name >> 24;
        file.mNameOffset = flags_and_name & 0xffffff;
        file.mDataOffset = read_u32(bytes, offset + 8);
        file.mDataSize = read_u32(bytes, offset + 12);
        file.mFileData = nullptr;
        validate_name(file.mNameOffset);
        if ((file.mFlag & FILE_FLAG_FOLDER) != 0 && file.mDirIndex >= mNativeInfo.mNrDirs &&
            std::strcmp(mNativeStrings.data() + file.mNameOffset, "..") != 0) {
            throw std::invalid_argument("JKR archive child directory is outside its directory table");
        }
        mNativeFiles.push_back(file);
    }
    mInfoBlock = &mNativeInfo;
    mDirs = mNativeDirs.data();
    mFiles = mNativeFiles.data();
    mStringTable = mNativeStrings.data();
    mLoaderName = mStringTable + mDirs->mNameOffset;
    mArchive = archive;
}

void JKRArchive::CArcName::store(const char* name) {
    mHash = 0;
    s32 length = 0;
    while (*name) {
        s32 ch = tolower(*name);
        mHash = ch + mHash * 3;
        if (length < ARRAY_SIZE(mName)) {
            mName[length++] = ch;
        }
        name++;
    }

    mLength = (u16)length;
    mName[length] = 0;
}

const char* JKRArchive::CArcName::store(const char* name, char endChar) {
    mHash = 0;
    s32 length = 0;
    while (*name && *name != endChar) {
        s32 lch = tolower((int)*name);
        mHash = lch + mHash * 3;
        if (length < ARRAY_SIZE(mName)) {
            mName[length++] = lch;
        }
        name++;
    }

    mLength = (u16)length;
    mName[length] = 0;

    if (*name == 0) {
        return NULL;
    }
    return name + 1;
}

bool JKRArchive::isSameName(CArcName& rName, u32 nameOffset, u16 hash) const {
    if (rName.mHash != hash) {
        return false;
    }

    return strcmp(mStringTable + nameOffset, rName.mName) == 0;
}

JKRArchive::SDIDirEntry* JKRArchive::findDirectory(const char* name, u32 directoryId) const {
    // Root ".." entries may contain the original 0xffffffff parent sentinel.
    // Reject that absent directory before forming a native array pointer.
    if (directoryId >= mInfoBlock->mNrDirs) {
        return nullptr;
    }
    if (name == NULL) {
        return mDirs + directoryId;
    }

    CArcName arcName(&name, '/');
    SDIDirEntry* dirEntry = mDirs + directoryId;
    SDIFileEntry* fileEntry = mFiles + dirEntry->mFirstFileIndex;

    for (int i = 0; i < dirEntry->mNrFiles; i++) {
        if (isSameName(arcName, fileEntry->mNameOffset, fileEntry->mHash)) {
            if ((fileEntry->mFlag) & 2) {
                return findDirectory(name, fileEntry->mDataOffset);
            }
            break;
        }
        fileEntry++;
    }

    return NULL;
}

JKRArchive::SDIFileEntry* JKRArchive::findIdxResource(u32 index) const {
    if (index < mInfoBlock->mNrFiles) {
        return &mFiles[index];
    }

    return nullptr;
}

s32 JKRArchive::countFile(const char* pName) const {
    SDIDirEntry* dir;

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

JKRArcFinder* JKRArchive::getFirstFile(const char* pName) const {
    SDIDirEntry* dir;

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
        return new JKRArcFinder(const_cast< JKRArchive* >(this), dir->mFirstFileIndex, dir->mNrFiles);
    }

    return nullptr;
}

bool JKRArchive::getDirEntry(SDirEntry* pDir, u32 fileIndex) const {
    SDIFileEntry* file = findIdxResource(fileIndex);

    if (file == nullptr) {
        return false;
    }

    pDir->mFileFlag = file->mFlag;
    pDir->mFileID = file->mFileID;
    pDir->mName = mStringTable + file->mNameOffset;

    return true;
}

u32 JKRArchive::countResource() const {
    u32 count = 0;

    for (u32 i = 0; i < mInfoBlock->mNrFiles; i++) {
        if ((mFiles[i].mFlag & FILE_FLAG_FILE) != 0) {
            count++;
        }
    }

    return count;
}

u32 JKRArchive::getFileAttribute(u32 fileIndex) const {
    SDIFileEntry* file = findIdxResource(fileIndex);

    if (file != nullptr) {
        return file->mFlag;
    }

    return 0;
}
