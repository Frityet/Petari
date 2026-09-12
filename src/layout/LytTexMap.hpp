#pragma once

#include "nw4r/lyt/texMap.h"
#include "resource/TplTexture.hpp"

#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace smgpc::resource { class Mem1ResourceHeap; }

namespace nw4r::lyt {
    struct HostTextureResourceState final {
        std::string name;
        std::shared_ptr<const void> backing;
        const TPLPalette* tpl_palette = nullptr;
    };
}

namespace smgpc::layout {
    // Decode bounded archive metadata once, retaining the unchanged GX image
    // bytes. Null heap is the explicit standalone host-renderer allocation path.
    [[nodiscard]] nw4r::lyt::TexMap make_tex_map(std::string_view resource_name,
        std::span<const std::uint8_t> bytes, std::shared_ptr<resource::Mem1ResourceHeap> heap);
    [[nodiscard]] std::string tex_map_name(const nw4r::lyt::TexMap&);
    [[nodiscard]] resource::DecodedTexture decode_tex_map(const nw4r::lyt::TexMap&);
}
