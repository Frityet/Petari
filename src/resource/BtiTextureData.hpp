#pragma once

#include "Mem1ResourceHeap.hpp"
#include "NativeTextureImage.hpp"

namespace smgpc::resource {
    class BtiTextureData final {
    public:
        BtiTextureData(std::span<const std::uint8_t>, std::shared_ptr<Mem1ResourceHeap>);
        BtiTextureData(BtiTextureData&&) noexcept = default;
        BtiTextureData(const BtiTextureData&) = delete;
        BtiTextureData& operator=(const BtiTextureData&) = delete;
        [[nodiscard]] const ResTIMG* image() const noexcept;

    private:
        Mem1ResourceHeap::Allocation _allocation;
    };
}
