#include <aurora/exception.hpp>
#include "JSystem/JKernel/JKRArchive.hpp"
#include "JSystem/JKernel/JKRFileFinder.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "compat/JkrAllocationDomain.hpp"

#include <cctype>
#include <cstring>
#include <limits>
#include <mutex>
#include <stdexcept>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (static_cast<s32>(sizeof(array) / sizeof((array)[0])))
#endif

namespace {
    using Bytes = std::span<const std::uint8_t>;
    OSMutex loader_mutex;
    std::once_flag loader_mutex_initialized;

    struct VolumeLock {
        VolumeLock() {
            JKRFileLoader::initializeVolumeList();
            OSLockMutex(&loader_mutex);
        }
        ~VolumeLock() { OSUnlockMutex(&loader_mutex); }
    };

    void require_range(Bytes data, std::size_t offset, std::size_t size) {
        if (offset > data.size() || size > data.size() - offset) {
            aurora::throw_host_exception<std::invalid_argument>("JKR archive metadata extends outside its retained resource");
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
JSUList<JKRFileLoader> JKRFileLoader::sFileLoaderList;
JSUList<JKRFileLoader>& JKRFileLoader::sVolumeList = JKRFileLoader::sFileLoaderList;
JKRFileLoader* JKRFileLoader::gCurrentFileLoader = nullptr;

JKRFileLoader::JKRFileLoader()
    : JKRDisposer(), mLoaderLink(this), mLoaderName(nullptr), mLoaderType(0), mIsMounted(false), _34(0) {}

JKRFileLoader::~JKRFileLoader() {
    VolumeLock lock;
    if (mLoaderLink.getList() == &sFileLoaderList) sFileLoaderList.remove(&mLoaderLink);
    if (gCurrentFileLoader == this) gCurrentFileLoader = nullptr;
}

void JKRFileLoader::unmount() {
    bool destroy = false;
    {
        VolumeLock lock;
        if (_34 != 0) destroy = --_34 == 0;
    }
    if (destroy) delete this;
}

void* JKRFileLoader::getGlbResource(const char* name, JKRFileLoader* loader) {
    if (loader != nullptr) return loader->getResource(0, name);
    VolumeLock lock;
    for (auto* link = sFileLoaderList.getFirst(); link != nullptr; link = link->getNext()) {
        if (void* resource = link->getObject()->getResource(0, name)) return resource;
    }
    return nullptr;
}

void JKRFileLoader::initializeVolumeList() {
    smgpc::compat::JkrHostAllocationScope host;
    // The original initializes the list's mutex, not the list itself. Native
    // owners may already have mounted archives before Game process startup.
    std::call_once(loader_mutex_initialized, [] { OSInitMutex(&loader_mutex); });
}
void JKRFileLoader::prependVolumeList(JSULink<JKRFileLoader>* loader) {
    VolumeLock lock;
    sFileLoaderList.prepend(loader);
}
void JKRFileLoader::removeVolumeList(JSULink<JKRFileLoader>* loader) {
    VolumeLock lock;
    sFileLoaderList.remove(loader);
}

JKRArchive* JKRArchive::check_mount_already(std::uintptr_t identity) {
    VolumeLock lock;
    for (auto* link = sFileLoaderList.getFirst(); link != nullptr; link = link->getNext()) {
        auto* loader = link->getObject();
        if (loader->mLoaderType != 0x52415243) continue;
        auto* archive = static_cast<JKRArchive*>(loader);
        if (archive->mEntryNum == identity) {
            // The retail lookup increments the existing mount's reference
            // count even when mountFixed subsequently rejects the duplicate.
            ++archive->_34;
            return archive;
        }
    }
    return nullptr;
}

JKRMemArchive::JKRMemArchive() : JKRArchive(nullptr) {}

JKRMemArchive::JKRMemArchive(const smgpc::resource::RarcArchive& archive)
    : JKRArchive(&archive), mHeader(reinterpret_cast<RarcHeader*>(const_cast<u8*>(archive.bytes().data()))),
      mFileDataStart(const_cast<u8*>(archive.file_data_start())) {
    publish_mount(archive.bytes().data());
}

JKRMemArchive::JKRMemArchive(smgpc::resource::RarcArchive&& archive) : JKRArchive(nullptr) {
    smgpc::compat::JkrHostAllocationScope host;
    mOwnedArchive = std::make_unique<smgpc::resource::RarcArchive>(std::move(archive));
    attach_archive(mOwnedArchive.get());
    mHeader = reinterpret_cast<RarcHeader*>(const_cast<u8*>(mOwnedArchive->bytes().data()));
    mFileDataStart = const_cast<u8*>(mOwnedArchive->file_data_start());
    publish_mount(mOwnedArchive->bytes().data());
}

void JKRMemArchive::publish_mount(const void* identity) {
    VolumeLock lock;
    mEntryNum = reinterpret_cast<std::uintptr_t>(identity);
    mMountMode = MOUNT_MODE_MEM;
    mLoaderType = 0x52415243;
    _34 = 1;
    if (gCurrentFileLoader == nullptr) {
        gCurrentFileLoader = this;
        sCurrentDirID = 0;
    }
    prependVolumeList(&mLoaderLink);
    mIsMounted = true;
}

JKRMemArchive::~JKRMemArchive() {
    smgpc::compat::JkrHostAllocationScope host;
    if (mIsMounted) {
        removeVolumeList(&mLoaderLink);
        mIsMounted = false;
    }
    // Parsed metadata borrows the fixed buffer; discard it before releasing
    // ownership through the original heap that allocated the supplied bytes.
    attach_archive(nullptr);
    mOwnedArchive.reset();
    if (_6C && mHeader != nullptr) JKRHeap::free(mHeader, mHeap);
    mHeader = nullptr;
    mFileDataStart = nullptr;
}

bool JKRMemArchive::mountFixed(void* data, JKRMemBreakFlag breakFlag) {
    if (data == nullptr) return false;
    // This SDK entry point has no length argument: its caller supplies a valid
    // RARC header and the declared buffer. Native callers can use the span API.
    const Bytes header(static_cast<const u8*>(data), sizeof(RarcHeader));
    return mountFixed(Bytes(static_cast<const u8*>(data), read_u32(header, 4)), breakFlag);
}

bool JKRMemArchive::mountFixed(Bytes bytes, JKRMemBreakFlag breakFlag) {
    smgpc::compat::JkrHostAllocationScope host;
    VolumeLock lock;
    if (bytes.data() == nullptr) return false;
    if (check_mount_already(reinterpret_cast<std::uintptr_t>(bytes.data())) != nullptr) return false;
    if (mIsMounted) return false;
    require_range(bytes, 0, sizeof(RarcHeader));
    const u32 size = read_u32(bytes, 4);
    require_range(bytes, 0, size);
    if (size < 0x40) aurora::throw_host_exception<std::invalid_argument>("Fixed archive is smaller than its RARC header and info block");
    auto* heap = JKRHeap::findFromRoot(const_cast<u8*>(bytes.data()));
    if (breakFlag == JKR_MEM_BREAK_FLAG_1 && heap == nullptr)
        aurora::throw_host_exception<std::invalid_argument>("Owned fixed archives require an actual JKR buffer allocation");
    auto parsed = std::make_unique<smgpc::resource::RarcArchive>(
        smgpc::resource::RarcArchive::from_borrowed(bytes.first(size)));
    attach_archive(parsed.get());
    mOwnedArchive = std::move(parsed);
    mHeap = heap;
    mHeader = reinterpret_cast<RarcHeader*>(const_cast<u8*>(bytes.data()));
    mFileDataStart = const_cast<u8*>(mOwnedArchive->file_data_start());
    _6C = breakFlag == JKR_MEM_BREAK_FLAG_1;
    publish_mount(bytes.data());
    return true;
}

JKRArchive::JKRArchive(const smgpc::resource::RarcArchive* archive) {
    attach_archive(archive);
}

void JKRArchive::attach_archive(const smgpc::resource::RarcArchive* archive) {
    smgpc::compat::JkrHostAllocationScope host;
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
    const std::size_t info = read_u32(bytes, 8);
    require_range(bytes, info, 0x20);
    nativeInfo = {read_u32(bytes, info), read_u32(bytes, info + 4),
                   read_u32(bytes, info + 8), read_u32(bytes, info + 12),
                   read_u32(bytes, info + 16), read_u32(bytes, info + 20),
                   read_u16(bytes, info + 24), read_u16(bytes, info + 26),
                   read_u32(bytes, info + 28)};
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
        const void* end = std::memchr(nativeStrings.data() + offset, 0, nativeStrings.size() - offset);
        if (end == nullptr || static_cast<const char*>(end) - (nativeStrings.data() + offset) >= 256) {
            aurora::throw_host_exception<std::invalid_argument>("JKR archive name is unterminated or exceeds the original lookup buffer");
        }
    };
    nativeDirs.reserve(nativeInfo.mNrDirs);
    for (u32 i = 0; i < nativeInfo.mNrDirs; ++i) {
        const std::size_t offset = directories + std::size_t(i) * 0x10;
        SDIDirEntry dir{read_u32(bytes, offset), read_u32(bytes, offset + 4),
                        read_u16(bytes, offset + 8), read_u16(bytes, offset + 10),
                        read_u32(bytes, offset + 12)};
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
        file.mFileID = read_u16(bytes, offset);
        file.mHash = read_u16(bytes, offset + 2);
        const u32 flags_and_name = read_u32(bytes, offset + 4);
        file.mFlag = flags_and_name >> 24;
        file.mNameOffset = flags_and_name & 0xffffff;
        file.mDataOffset = read_u32(bytes, offset + 8);
        file.mDataSize = read_u32(bytes, offset + 12);
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
    if (mInfoBlock == nullptr || directoryId >= mInfoBlock->mNrDirs) {
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
    if (mInfoBlock != nullptr && index < mInfoBlock->mNrFiles) {
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
    if (mInfoBlock == nullptr) return 0;
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
