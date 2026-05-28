#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <string_view>
#include <utility>

#include "resource/RarcArchive.hpp"

class JKRArchive {
public:
    virtual ~JKRArchive() = default;

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

    [[nodiscard]] virtual std::uint32_t countResource() const {
        return mArchive == nullptr ? 0U : static_cast<std::uint32_t>(mArchive->entries().size());
    }

    [[nodiscard]] virtual std::uint32_t countFile(const char *pDirectory) const {
        return mArchive == nullptr || pDirectory == nullptr ? 0U : mArchive->count_directory_files(pDirectory);
    }

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
    explicit JKRArchive(const smgpc::resource::RarcArchive *archive)
        : mArchive(archive) {
    }

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
};

class JKRMemArchive final : public JKRArchive {
public:
    explicit JKRMemArchive(const smgpc::resource::RarcArchive &archive)
        : JKRArchive(&archive) {
    }

    explicit JKRMemArchive(smgpc::resource::RarcArchive &&archive)
        : JKRArchive(nullptr), mOwnedArchive(std::make_unique<smgpc::resource::RarcArchive>(std::move(archive))) {
        mArchive = mOwnedArchive.get();
    }

private:
    std::unique_ptr<smgpc::resource::RarcArchive> mOwnedArchive;
};
