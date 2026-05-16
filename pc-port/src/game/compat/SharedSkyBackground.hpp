#pragma once

#include <cstdint>

#include "compat/FileSelectPreviewTextures.hpp"

namespace smgpc::render::layout {
    class LayoutDrawList;
}

namespace smgpc::game::compat {

    struct SharedSkyBackgroundOptions {
        bool show_bottom_haze { true };
    };

    void append_shared_sky_background_draw_commands(render::layout::LayoutDrawList *pDrawList, const file_select_preview::SkyTextures &textures,
                                                    std::uint64_t frame, const SharedSkyBackgroundOptions &options = {});

}  // namespace smgpc::game::compat
