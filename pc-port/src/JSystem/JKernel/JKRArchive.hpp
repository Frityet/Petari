#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <utility>

#include "Game/compat/RarcArchive.hpp"

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

    [[nodiscard]] virtual void *getResource(std::uint16_t) const {
        return nullptr;
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

    [[nodiscard]] virtual bool contains(const char *pPath) const {
        return mArchive != nullptr && pPath != nullptr && mArchive->contains(pPath);
    }

protected:
    explicit JKRArchive(smgpc::game::RarcArchive *archive)
        : mArchive(archive) {
    }

    [[nodiscard]] std::span<const std::uint8_t> resource_data(std::string_view path) const {
        if (mArchive == nullptr || path.empty()) {
            return {};
        }

        const auto *entry = mArchive->find(path);
        if (entry == nullptr && path.starts_with('/')) {
            entry = mArchive->find(path.substr(1U));
        }
        if (entry == nullptr) {
            return {};
        }

        return mArchive->file_data(*entry);
    }

    smgpc::game::RarcArchive *mArchive = nullptr;
};

class JKRMemArchive final : public JKRArchive {
public:
    explicit JKRMemArchive(smgpc::game::RarcArchive &archive)
        : JKRArchive(&archive) {
    }

    explicit JKRMemArchive(smgpc::game::RarcArchive archive)
        : JKRArchive(nullptr), mOwnedArchive(std::make_unique<smgpc::game::RarcArchive>(std::move(archive))) {
        mArchive = mOwnedArchive.get();
    }

private:
    std::unique_ptr<smgpc::game::RarcArchive> mOwnedArchive;
};
