#pragma once

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

class JKRArcFinder;

class JKRArchive {
public:
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

    virtual ~JKRArchive() = default;

    JKRArchive(const JKRArchive&) = delete;
    JKRArchive& operator=(const JKRArchive&) = delete;

    [[nodiscard]] virtual void *getResource(const char *pPath) const {
        const auto data = resource_data(pPath == nullptr ? std::string_view{} : std::string_view(pPath));
        return data.empty() ? nullptr : const_cast<std::uint8_t *>(data.data());
    }

    [[nodiscard]] virtual void *getResource(std::uint32_t, const char *pPath) const {
        return getResource(pPath);
    }

    [[nodiscard]] virtual void *getResource(std::uint16_t id) const {
        const auto *entry = mArchive == nullptr ? nullptr : mArchive->find_by_file_id(id);
        if (entry == nullptr) {
            return nullptr;
        }

        const auto data = mArchive->file_data(*entry);
        return data.empty() ? nullptr : const_cast<std::uint8_t *>(data.data());
    }

    [[nodiscard]] virtual std::uint32_t getResSize(const void *pResource) const {
        if (pResource == nullptr || mArchive == nullptr) {
            return 0U;
        }

        for (const auto &entry : mArchive->entries()) {
            const auto data = mArchive->file_data(entry);
            if (!data.empty() && data.data() == pResource) {
                return static_cast<std::uint32_t>(data.size());
            }
        }

        return 0U;
    }

    [[nodiscard]] void* getIdxResource(u32);
    virtual void* fetchResource(SDIFileEntry*, u32*) = 0;

    [[nodiscard]] u32 countResource() const;
    [[nodiscard]] virtual s32 countFile(const char*) const;
    [[nodiscard]] JKRArcFinder* getFirstFile(const char*) const;
    [[nodiscard]] bool getDirEntry(SDirEntry*, u32) const;
    [[nodiscard]] u32 getFileAttribute(u32) const;
    [[nodiscard]] bool isSameName(CArcName&, u32, u16) const;
    [[nodiscard]] SDIDirEntry* findDirectory(const char*, u32) const;
    [[nodiscard]] SDIFileEntry* findIdxResource(u32) const;

    static u32 sCurrentDirID;
    char* mLoaderName = nullptr;
    RarcInfoBlock* mInfoBlock = nullptr;
    SDIDirEntry* mDirs = nullptr;
    SDIFileEntry* mFiles = nullptr;
    char* mStringTable = nullptr;

    [[nodiscard]] virtual std::uint32_t readResource(void *pBuffer, std::uint32_t bufferSize, const char *pPath) const {
        const auto data = resource_data(pPath == nullptr ? std::string_view{} : std::string_view(pPath));
        if (data.empty() || pBuffer == nullptr || bufferSize == 0U) {
            return 0U;
        }

        const auto copy_size = std::min<std::size_t>(bufferSize, data.size());
        std::memcpy(pBuffer, data.data(), copy_size);
        return static_cast<std::uint32_t>(copy_size);
    }

    [[nodiscard]] virtual std::uint32_t readResource(void *pBuffer, std::uint32_t bufferSize, std::uint16_t fileId) const {
        const auto *entry = mArchive == nullptr ? nullptr : mArchive->find_by_file_id(fileId);
        if (entry == nullptr || pBuffer == nullptr || bufferSize == 0U) {
            return 0U;
        }

        const auto data = mArchive->file_data(*entry);
        const auto copy_size = std::min<std::size_t>(bufferSize, data.size());
        std::memcpy(pBuffer, data.data(), copy_size);
        return static_cast<std::uint32_t>(copy_size);
    }

    [[nodiscard]] virtual bool contains(const char *pPath) const {
        return mArchive != nullptr && pPath != nullptr && mArchive->contains_resource(pPath);
    }

protected:
    explicit JKRArchive(const smgpc::resource::RarcArchive* archive);
    void attach_archive(const smgpc::resource::RarcArchive* archive);

    [[nodiscard]] std::span<const std::uint8_t> resource_data(std::string_view path) const {
        if (mArchive == nullptr || path.empty()) {
            return {};
        }

        const auto *entry = mArchive->find_resource(path);
        if (entry == nullptr) {
            return {};
        }

        return mArchive->file_data(*entry);
    }

    const smgpc::resource::RarcArchive *mArchive = nullptr;

private:
    RarcInfoBlock mNativeInfo{};
    std::vector<SDIDirEntry> mNativeDirs;
    std::vector<SDIFileEntry> mNativeFiles;
    std::vector<char> mNativeStrings;
};

class JKRMemArchive final : public JKRArchive {
public:
    explicit JKRMemArchive(const smgpc::resource::RarcArchive &archive)
        : JKRArchive(&archive), mFileDataStart(const_cast<u8*>(archive.file_data_start())) {
    }

    explicit JKRMemArchive(smgpc::resource::RarcArchive &&archive)
        : JKRArchive(nullptr), mOwnedArchive(std::make_unique<smgpc::resource::RarcArchive>(std::move(archive))) {
        attach_archive(mOwnedArchive.get());
        mFileDataStart = const_cast<u8*>(mOwnedArchive->file_data_start());
    }

    void* fetchResource(SDIFileEntry*, u32*) override;

private:
    u8* mFileDataStart = nullptr;
    std::unique_ptr<smgpc::resource::RarcArchive> mOwnedArchive;
};
