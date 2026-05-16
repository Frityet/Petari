#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include <bgfx/bgfx.h>

#include "core/RenderTypes.hpp"

namespace smgpc::render::backends {

    class BgfxCallback;

    class BgfxBackend final {
    public:
        BgfxBackend();
        ~BgfxBackend();

        BgfxBackend(const BgfxBackend&) = delete;
        BgfxBackend& operator=(const BgfxBackend&) = delete;

        void initialize(const core::RenderInitDesc& description);
        void shutdown();
        void begin_frame(const core::FrameContext& frame_context);
        void end_frame();
        void resize(std::uint16_t width, std::uint16_t height);
        void request_screenshot_png(const std::filesystem::path& path);
        [[nodiscard]] core::TextureHandle create_rgba8_texture(std::uint16_t width, std::uint16_t height, std::span< const std::uint8_t > rgba);
        void destroy_texture(core::TextureHandle texture);
        void submit_textured_quad(core::TextureHandle texture, const core::TexturedQuad2D& quad);
        void submit_textured_triangles(core::TextureHandle texture, const core::TexturedTriangleBatch2D& batch);
        void submit_gx_material_triangles(const core::GxMaterialTriangleBatch2D& batch);

        [[nodiscard]] core::FramebufferInfo framebuffer_size() const;
        [[nodiscard]] core::FramebufferInfo logical_framebuffer_size() const;
        [[nodiscard]] bool is_initialized() const;

    private:
        bool _initialized = false;
        bool _vsync_enabled = true;
        std::uint16_t _framebuffer_width = 1U;
        std::uint16_t _framebuffer_height = 1U;
        std::uint16_t _logical_framebuffer_width = core::kWiiLogicalFramebufferWidth;
        std::uint16_t _logical_framebuffer_height = core::kWiiLogicalFramebufferHeight;
        std::string _pending_screenshot_path{};
        std::unique_ptr< BgfxCallback > _callback{};
        std::vector< bgfx::TextureHandle > _textures{};
        bgfx::VertexLayout _textured_quad_layout{};
        bgfx::VertexLayout _gx_material_layout{};
        bgfx::ProgramHandle _textured_quad_program = BGFX_INVALID_HANDLE;
        bgfx::ProgramHandle _gx_material_program = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle _texture_sampler = BGFX_INVALID_HANDLE;
        std::array< bgfx::UniformHandle, core::kMaxGxMaterialTextureStages2D > _gx_material_samplers{};
        bgfx::UniformHandle _gx_material_params = BGFX_INVALID_HANDLE;
        std::array< bgfx::UniformHandle, core::kMaxGxMaterialTevStages2D > _gx_tev_color_inputs{};
        std::array< bgfx::UniformHandle, core::kMaxGxMaterialTevStages2D > _gx_tev_alpha_inputs{};
        std::array< bgfx::UniformHandle, core::kMaxGxMaterialTevStages2D > _gx_tev_color_operations{};
        std::array< bgfx::UniformHandle, core::kMaxGxMaterialTevStages2D > _gx_tev_alpha_operations{};
        std::array< bgfx::UniformHandle, core::kMaxGxMaterialTevStages2D > _gx_tev_outputs{};
        std::array< bgfx::UniformHandle, core::kMaxGxMaterialTevStages2D > _gx_tev_konst_colors{};
        std::array< bgfx::UniformHandle, 4U > _gx_tev_initial_registers{};
        bgfx::UniformHandle _gx_alpha_compare_0 = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle _gx_alpha_compare_1 = BGFX_INVALID_HANDLE;
    };

}  // namespace smgpc::render::backends
