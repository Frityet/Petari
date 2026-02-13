#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <unordered_map>

#include "GameAssetService.hpp"
#include "compat/Types.hpp"

class JKRHeap {
};

namespace JKRDvdRipper {

enum EAllocDirection {
    ALLOC_DIR_TOP,
    ALLOC_DIR_BOTTOM,
};

}  // namespace JKRDvdRipper

class JKRArchive {
public:
    virtual ~JKRArchive() = default;

    [[nodiscard]] virtual std::span<const std::byte> getResourceSpan(const char *pFilePath) const = 0;
    [[nodiscard]] virtual void *getResource(const char *pFilePath) const;
    [[nodiscard]] virtual u32 getResSize(const void *pResource) const = 0;
};

class JKRMemArchive final : public JKRArchive {
public:
    explicit JKRMemArchive(std::shared_ptr<const smgpc::assets::MountedArchiveData> archive_data);

    [[nodiscard]] std::span<const std::byte> getResourceSpan(const char *pFilePath) const override;
    [[nodiscard]] void *getResource(const char *pFilePath) const override;
    [[nodiscard]] u32 getResSize(const void *pResource) const override;

    [[nodiscard]] const smgpc::assets::layout::RarcArchive *archive() const;

private:
    std::shared_ptr<const smgpc::assets::MountedArchiveData> mArchiveData {};
    std::unordered_map<const void *, u32> mResourceSizes {};
};
