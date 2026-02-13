#pragma once

#include <cstdint>

#include "ServiceProvider.hpp"
#include "LayoutDrawList.hpp"
#include "core/RenderCommandBuffer.hpp"

namespace smgpc::logging {
class ILogger;
}

namespace smgpc::render::layout {

class LayoutRenderPass {
public:
    explicit LayoutRenderPass(smgpc::di::OptionalDependencyReference<smgpc::logging::ILogger> logger = nullptr);
    ~LayoutRenderPass() = default;

    LayoutRenderPass(const LayoutRenderPass &) = delete;
    LayoutRenderPass &operator=(const LayoutRenderPass &) = delete;
    LayoutRenderPass(LayoutRenderPass &&) = default;
    LayoutRenderPass &operator=(LayoutRenderPass &&) = delete;

    void record(
        smgpc::render::core::RenderCommandBuffer &commands,
        const LayoutDrawList &draw_list,
        std::uint16_t framebuffer_width,
        std::uint16_t framebuffer_height,
        float layout_width,
        float layout_height);

private:
    [[nodiscard]] static bool env_is_enabled(const char *name);
    [[nodiscard]] static smgpc::render::core::RenderBlendMode map_blend_mode(BlendMode blend_mode);

    smgpc::di::OptionalDependencyReference<smgpc::logging::ILogger> _logger;
};

}  // namespace smgpc::render::layout
