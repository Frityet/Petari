#pragma once

#include "resource/Mem1ResourceHeap.hpp"
#include "revolution/tpl.h"
#include <vector>

namespace smgpc::resource {
    // Widened native descriptors own a bounded copy of the original packed
    // resource. No numeric Wii-address classification or in-place relocation.
    class TplTextureData final {
    public:
        TplTextureData(std::span<const std::uint8_t>, std::shared_ptr<Mem1ResourceHeap>);
        TplTextureData(const TplTextureData&) = delete;
        TplTextureData& operator=(const TplTextureData&) = delete;
        [[nodiscard]] const TPLPalette& palette() const noexcept { return _palette; }
    private:
        Mem1ResourceHeap::Allocation _allocation;
        std::vector<std::uint8_t> _host_bytes;
        std::vector<TPLHeader> _headers;
        std::vector<TPLClutHeader> _cluts;
        std::vector<TPLDescriptor> _descriptors;
        TPLPalette _palette{};
    };
}
