#pragma once

#define QUESTIONMARK_MAGIC 0x3F3F3F3F

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include <revolution/types.h>

#include "resource/RarcArchive.hpp"
#include "JSystem/JKernel/JKRFileLoader.hpp"

class JKRArcFinder;

class JKRArchive : public JKRFileLoader {
public:
    enum EMountDirection { MOUNT_DIRECTION_1 = 1 };
    enum EMountMode { MOUNT_MODE_0 = 0, MOUNT_MODE_MEM = 1, MOUNT_MODE_ARAM = 2, MOUNT_MODE_DVD = 3, MOUNT_MODE_COMP = 4 };
    enum EFileFlag {
        FILE_FLAG_FILE_SHIFT = 0,
        FILE_FLAG_FOLDER_SHIFT = 1,
        FILE_FLAG_COMPRESSED_SHIFT = 2,
        FILE_FLAG_MRAM_SHIFT = 4,
        FILE_FLAG_ARAM_SHIFT = 5,
        FILE_FLAG_DVD_SHIFT = 6,
        FILE_FLAG_IS_YAZ0_SHIFT = 7,

        FILE_FLAG_FILE = 1 << FILE_FLAG_FILE_SHIFT,
        FILE_FLAG_FOLDER = 1 << FILE_FLAG_FOLDER_SHIFT,
        FILE_FLAG_COMPRESSED = 1 << FILE_FLAG_COMPRESSED_SHIFT,
        FILE_FLAG_MRAM = 1 << FILE_FLAG_MRAM_SHIFT,
        FILE_FLAG_ARAM = 1 << FILE_FLAG_ARAM_SHIFT,
        FILE_FLAG_DVD = 1 << FILE_FLAG_DVD_SHIFT,
        FILE_FLAG_IS_YAZ0 = 1 << FILE_FLAG_IS_YAZ0_SHIFT
    };

    struct RarcHeader {
        /* 0x0 */ u32 mMagic;
        /* 0x4 */ u32 mFileSize;
        /* 0x8 */ u32 mHeaderSize;
        /* 0xC */ u32 mFileDataOffset;
        /* 0x10 */ u32 mTotalDataSize;
        /* 0x14 */ u32 mMRamDataSize;
        /* 0x18 */ u32 mARamDataSize;
        u32 _1C;
    };

    struct RarcInfoBlock {
        /* 0x0 */ u32 mNrDirs;
        /* 0x4 */ u32 mDirOffset;
        /* 0x8 */ u32 mNrFiles;
        /* 0xC */ u32 mFileOffset;
        /* 0x10 */ u32 mStringTableSize;
        /* 0x14 */ u32 mStringTableOffset;
        /* 0x18 */ u16 mNextAvailableFileID;
        /* 0x1A */ u16 mFileIDIsIndex;
        /* 0x1C */ u32 _1C;
    };

    struct SDIFileEntry {
        /* 0x0 */ u16 mFileID;
        /* 0x2 */ u16 mHash;
        /* 0x4 */ u32 mFlag : 8;
        /* 0x5 */ u32 mNameOffset : 24;
        union {
            /* 0x8 */ u32 mDataOffset;
            /* 0x8 */ u32 mDirIndex;
        };
        union {
            /* 0xC */ u32 mDataSize;
        };
        void* /* 0x10 */ mFileData;
    };

    struct SDIDirEntry {
        /* 0x0 */ u32 mID;
        /* 0x4 */ u32 mNameOffset;
        /* 0x8 */ u16 mHash;
        /* 0xA */ u16 mNrFiles;
        /* 0xC */ u32 mFirstFileIndex;
    };

    struct SDirEntry {
        /* 0x0 */ u8 mFileFlag;
        u8 _1;
        /* 0x2 */ u16 mFileID;
        /* 0x4 */ char* mName;
    };

    class CArcName {
    public:
        inline CArcName() {
        }

        CArcName(char const* data) {
            this->store(data);
        }
        CArcName(char const** data, char endChar) {
            *data = this->store(*data, endChar);
        }

        void store(const char*);
        const char* store(const char*, char);

        /* 0x0 */ u16 mHash;
        /* 0x2 */ u16 mLength;
        /* 0x4 */ char mName[256];
    };

    ~JKRArchive() override = default;

    JKRArchive(const JKRArchive&) = delete;
    JKRArchive& operator=(const JKRArchive&) = delete;

    [[nodiscard]] virtual void *getResource(const char *pPath) const;

    [[nodiscard]] virtual void *getResource(std::uint32_t, const char *pPath) const;

    [[nodiscard]] virtual void *getResource(std::uint16_t id) const;

    [[nodiscard]] virtual std::uint32_t getResSize(const void *pResource) const;
    [[nodiscard]] std::uint32_t getExpandedResSize(const void *pResource) const;

    [[nodiscard]] void* getIdxResource(u32);
    virtual void* fetchResource(SDIFileEntry*, u32*) = 0;

    [[nodiscard]] u32 countResource() const;
    [[nodiscard]] virtual s32 countFile(const char*) const;
    [[nodiscard]] JKRArcFinder* getFirstFile(const char*) const;
    [[nodiscard]] bool getDirEntry(SDirEntry*, u32) const;
    [[nodiscard]] u32 getFileAttribute(u32) const;
    // Native resource readers use the original archive's complete byte bounds.
    // Fixed mounts still borrow caller/FileEntry bytes. Keep a native resource
    // token while borrowing archive records or raw bytes; source ownership
    // alone retains parsed metadata, not that original fixed buffer.
    [[nodiscard]] const smgpc::resource::RarcArchive& source() const;
    [[nodiscard]] std::shared_ptr<const smgpc::resource::RarcArchive> retainSource() const;
    [[nodiscard]] std::shared_ptr<const void> retainNativeResources() const;
    // Override the native cache until the returned handle retires. Overlapping
    // overrides may retire in either order; each handle holds one archive lease.
    [[nodiscard]] std::shared_ptr<const void> overrideNativeResource(u32 fileEntryIndex, void* replacement);
    void validateNativeRetirement(std::size_t releasingBorrows = 0) const;
    [[nodiscard]] bool isSameName(CArcName&, u32, u16) const;
    [[nodiscard]] SDIDirEntry* findDirectory(const char*, u32) const;
    [[nodiscard]] SDIFileEntry* findIdxResource(u32) const;

    static JKRArchive* mount(const char*, EMountMode, JKRHeap*, EMountDirection);
    [[nodiscard]] SDIFileEntry* findIdResource(u16) const;
    static u32 sCurrentDirID;
    static JKRArchive* check_mount_already(std::uintptr_t);
    // Distinct from JKRDisposer::mHeap: this owns archive bytes, while the
    // disposer heap owns the loader object itself.
    JKRHeap* mHeap = nullptr;
    u8 mMountMode = MOUNT_MODE_0;
    std::uintptr_t mEntryNum = 0;
    RarcInfoBlock* mInfoBlock = nullptr;
    SDIDirEntry* mDirs = nullptr;
    SDIFileEntry* mFiles = nullptr;
    char* mStringTable = nullptr;

    [[nodiscard]] virtual std::uint32_t readResource(void *pBuffer, std::uint32_t bufferSize, const char *pPath) const;

    [[nodiscard]] virtual std::uint32_t readResource(void *pBuffer, std::uint32_t bufferSize, std::uint16_t fileId) const;

    [[nodiscard]] virtual bool contains(const char *pPath) const;

protected:
    explicit JKRArchive(const smgpc::resource::RarcArchive* archive);
    void attach_archive(const smgpc::resource::RarcArchive* archive);
    void attach_archive(std::shared_ptr<const smgpc::resource::RarcArchive> archive);

    [[nodiscard]] std::span<const std::uint8_t> resource_data(std::string_view path) const;

    [[nodiscard]] std::span<const std::uint8_t> resource_data(const smgpc::resource::RarcEntry& entry) const;

    const smgpc::resource::RarcArchive *mArchive = nullptr;

private:
    struct NativeResourceOverride;
    void removeNativeResourceOverride(const NativeResourceOverride&) noexcept;
    struct NativeResourceOverrides {
        void* original = nullptr;
        std::vector<std::weak_ptr<const NativeResourceOverride>> active;
    };
    std::vector<NativeResourceOverrides> mNativeResourceOverrides;
    std::shared_ptr<const smgpc::resource::RarcArchive> mNativeSource;
    std::shared_ptr<const void> mNativeResourceToken;
    RarcInfoBlock mNativeInfo{};
    std::vector<SDIDirEntry> mNativeDirs;
    std::vector<SDIFileEntry> mNativeFiles;
    std::vector<char> mNativeStrings;
};

enum JKRMemBreakFlag { JKR_MEM_BREAK_FLAG_0 = 0, JKR_MEM_BREAK_FLAG_1 = 1 };

class JKRMemArchive final : public JKRArchive {
public:
    JKRMemArchive();
    ~JKRMemArchive() override;
    explicit JKRMemArchive(const smgpc::resource::RarcArchive& archive);
    explicit JKRMemArchive(smgpc::resource::RarcArchive&& archive);
    bool mountFixed(void*, JKRMemBreakFlag);
    bool mountFixed(std::span<const u8>, JKRMemBreakFlag);

    void* fetchResource(SDIFileEntry*, u32*) override;
    static s32 fetchResource_subroutine(u8*, u32, u8*, u32, int);

    RarcHeader* mHeader = nullptr;
    u8* mFileDataStart = nullptr;
    bool _6C = false;

private:
    void publish_mount(const void* identity);
    std::shared_ptr<smgpc::resource::RarcArchive> mOwnedArchive;
};
