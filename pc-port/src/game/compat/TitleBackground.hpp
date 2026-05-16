#pragma once

#include <cstdint>
#include <optional>

#include "compat/FileSelectPreviewTextures.hpp"
#include "core/RenderCommandBuffer.hpp"
#include "layout/LayoutDrawList.hpp"
#include "layout/Tpl.hpp"

class LayoutActor;

namespace smgpc::game::compat {

class TitleBackground {
public:
    TitleBackground();

    void appendDrawCommands(render::layout::LayoutDrawList *pDrawList, std::uint64_t frame) const;
    void appendJ3dDrawCommands(render::core::RenderCommandBuffer *pCommands, std::uint64_t frame, std::uint16_t framebufferWidth,
                               std::uint16_t framebufferHeight) const;
    void appendLogoOverlayDrawCommands(render::layout::LayoutDrawList *pDrawList, const LayoutActor *pLogoLayout, std::uint64_t frame) const;

private:
    file_select_preview::SkyTextures _skyTextures {};
    std::optional<assets::layout::tpl::DecodedImage> _titleSpace {};
    std::optional<assets::layout::tpl::DecodedImage> _titleMask {};
};

}  // namespace smgpc::game::compat
