#include "JSystem/JKernel/JKRArchive.hpp"
#include <aurora/allocation.hpp>
#include <aurora/endian.hpp>
#include <aurora/exception.hpp>
#include <cctype>
#include <limits>
#include <stdexcept>

namespace {
    using aurora::endian::read_big;
    using Bytes = std::span<const std::uint8_t>;

    void require_range(Bytes data, std::size_t offset, std::size_t size) {
        if (offset > data.size() || size > data.size() - offset) {
            aurora::throw_host_exception<std::invalid_argument>("JKR archive metadata extends outside its retained resource");
        }
    }
}  // namespace

u32 JKRArchive::sCurrentDirID = 0;

JKRArchive::JKRArchive(const smgpc::resource::RarcArchive *archive) {
    attach_archive(archive);
}

void JKRArchive::attach_archive(const smgpc::resource::RarcArchive *archive) {
    aurora::allocation::HostAllocationScope host;
    if (archive == nullptr) {
        mArchive = nullptr;
        mInfoBlock = nullptr;
        mDirs = nullptr;
        mFiles = nullptr;
        mStringTable = nullptr;
        mLoaderName = nullptr;
        mNativeDirs.clear();
        mNativeFiles.clear();
        mNativeStrings.clear();
        return;
    }
    RarcInfoBlock nativeInfo{};
    std::vector<SDIDirEntry> nativeDirs;
    std::vector<SDIFileEntry> nativeFiles;
    std::vector<char> nativeStrings;
    const Bytes bytes = archive->bytes();
    const std::size_t info = read_big<u32>(bytes, 8);
    require_range(bytes, info, 0x20);
    nativeInfo = {read_big<u32>(bytes, info), read_big<u32>(bytes, info + 4),
                  read_big<u32>(bytes, info + 8), read_big<u32>(bytes, info + 12),
                  read_big<u32>(bytes, info + 16), read_big<u32>(bytes, info + 20),
                  read_big<u16>(bytes, info + 24), read_big<u16>(bytes, info + 26),
                  read_big<u32>(bytes, info + 28)};
    const std::size_t directories = info + nativeInfo.mDirOffset;
    const std::size_t files = info + nativeInfo.mFileOffset;
    const std::size_t strings = info + nativeInfo.mStringTableOffset;
    require_range(bytes, directories, std::size_t(nativeInfo.mNrDirs) * 0x10);
    require_range(bytes, files, std::size_t(nativeInfo.mNrFiles) * 0x14);
    require_range(bytes, strings, nativeInfo.mStringTableSize);
    if (nativeInfo.mNrDirs == 0 || nativeInfo.mNrFiles > std::numeric_limits<s32>::max()) {
        aurora::throw_host_exception<std::invalid_argument>("JKR archive directory/file count cannot be represented");
    }
    nativeStrings.assign(bytes.begin() + strings, bytes.begin() + strings + nativeInfo.mStringTableSize);
    const auto validate_name = [&](u32 offset) {
        if (offset >= nativeStrings.size()) {
            aurora::throw_host_exception<std::invalid_argument>("JKR archive name offset is outside its string table");
        }
        const void *end = std::memchr(nativeStrings.data() + offset, 0, nativeStrings.size() - offset);
        if (end == nullptr || static_cast<const char *>(end) - (nativeStrings.data() + offset) >= 256) {
            aurora::throw_host_exception<std::invalid_argument>("JKR archive name is unterminated or exceeds the original lookup buffer");
        }
    };
    nativeDirs.reserve(nativeInfo.mNrDirs);
    for (u32 i = 0; i < nativeInfo.mNrDirs; ++i) {
        const std::size_t offset = directories + std::size_t(i) * 0x10;
        SDIDirEntry dir{read_big<u32>(bytes, offset), read_big<u32>(bytes, offset + 4),
                        read_big<u16>(bytes, offset + 8), read_big<u16>(bytes, offset + 10),
                        read_big<u32>(bytes, offset + 12)};
        validate_name(dir.mNameOffset);
        if (dir.mFirstFileIndex > nativeInfo.mNrFiles ||
            dir.mNrFiles > nativeInfo.mNrFiles - dir.mFirstFileIndex) {
            aurora::throw_host_exception<std::invalid_argument>("JKR archive directory range is outside its file table");
        }
        nativeDirs.push_back(dir);
    }
    nativeFiles.reserve(nativeInfo.mNrFiles);
    for (u32 i = 0; i < nativeInfo.mNrFiles; ++i) {
        const std::size_t offset = files + std::size_t(i) * 0x14;
        SDIFileEntry file{};
        file.mFileID = read_big<u16>(bytes, offset);
        file.mHash = read_big<u16>(bytes, offset + 2);
        const u32 flags_and_name = read_big<u32>(bytes, offset + 4);
        file.mFlag = flags_and_name >> 24;
        file.mNameOffset = flags_and_name & 0xffffff;
        file.mDataOffset = read_big<u32>(bytes, offset + 8);
        file.mDataSize = read_big<u32>(bytes, offset + 12);
        file.mFileData = nullptr;
        validate_name(file.mNameOffset);
        if ((file.mFlag & FILE_FLAG_FOLDER) != 0 && file.mDirIndex >= nativeInfo.mNrDirs &&
            std::strcmp(nativeStrings.data() + file.mNameOffset, "..") != 0) {
            aurora::throw_host_exception<std::invalid_argument>("JKR archive child directory is outside its directory table");
        }
        nativeFiles.push_back(file);
    }
    mNativeInfo = nativeInfo;
    mNativeDirs = std::move(nativeDirs);
    mNativeFiles = std::move(nativeFiles);
    mNativeStrings = std::move(nativeStrings);
    mInfoBlock = &mNativeInfo;
    mDirs = mNativeDirs.data();
    mFiles = mNativeFiles.data();
    mStringTable = mNativeStrings.data();
    mLoaderName = mStringTable + mDirs->mNameOffset;
    mArchive = archive;
}

void JKRArchive::CArcName::store(const char *name) {
    mHash = 0;
    s32 length = 0;
    while (*name) {
        s32 ch = tolower(*name);
        mHash = ch + mHash * 3;
        if (length >= static_cast<s32>(sizeof(mName)) - 1) {
            aurora::throw_host_exception<std::invalid_argument>("JKR archive name exceeds the original lookup buffer");
        }
        mName[length++] = ch;
        name++;
    }

    mLength = (u16)length;
    mName[length] = 0;
}

const char *JKRArchive::CArcName::store(const char *name, char endChar) {
    mHash = 0;
    s32 length = 0;
    while (*name && *name != endChar) {
        s32 lch = tolower((int)*name);
        mHash = lch + mHash * 3;
        if (length >= static_cast<s32>(sizeof(mName)) - 1) {
            aurora::throw_host_exception<std::invalid_argument>("JKR archive name exceeds the original lookup buffer");
        }
        mName[length++] = lch;
        name++;
    }

    mLength = (u16)length;
    mName[length] = 0;

    if (*name == 0) {
        return NULL;
    }
    return name + 1;
}

bool JKRArchive::isSameName(CArcName &rName, u32 nameOffset, u16 hash) const {
    if (rName.mHash != hash) {
        return false;
    }

    return strcmp(mStringTable + nameOffset, rName.mName) == 0;
}

JKRArchive::SDIDirEntry *JKRArchive::findDirectory(const char *name, u32 directoryId) const {
    // Root ".." entries may contain the original 0xffffffff parent sentinel.
    // Reject that absent directory before forming a native array pointer.
    if (mInfoBlock == nullptr || directoryId >= mInfoBlock->mNrDirs) {
        return nullptr;
    }
    if (name == NULL) {
        return mDirs + directoryId;
    }

    CArcName arcName(&name, '/');
    SDIDirEntry *dirEntry = mDirs + directoryId;
    SDIFileEntry *fileEntry = mFiles + dirEntry->mFirstFileIndex;

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

JKRArchive::SDIFileEntry *JKRArchive::findIdxResource(u32 index) const {
    if (mInfoBlock != nullptr && index < mInfoBlock->mNrFiles) {
        return &mFiles[index];
    }

    return nullptr;
}

std::span<const std::uint8_t> JKRArchive::resource_data(std::string_view path) const {
    if (mArchive == nullptr || path.empty()) {
        return {};
    }

    const auto *entry = mArchive->find_resource(path);
    if (entry == nullptr) {
        return {};
    }

    return resource_data(*entry);
}

std::span<const std::uint8_t> JKRArchive::resource_data(const smgpc::resource::RarcEntry &entry) const {
    const auto &native = mNativeFiles.at(entry.file_entry_index);
    if (native.mFileData != nullptr) {
        return {static_cast<const std::uint8_t *>(native.mFileData), native.mDataSize};
    }
    return mArchive->file_data(entry);
}
