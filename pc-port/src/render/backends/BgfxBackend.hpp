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
        struct QuadVertex {
            float x = 0.0F;
            float y = 0.0F;
            float z = 0.0F;
            float u = 0.0F;
            float v = 0.0F;
            std::uint32_t abgr = 0xffffffffU;
        };

        struct GxMaterialVertex {
            float x = 0.0F;
            float y = 0.0F;
            float z = 0.0F;
            float u0 = 0.0F;
            float v0 = 0.0F;
            float q0 = 1.0F;
            float u1 = 0.0F;
            float v1 = 0.0F;
            float q1 = 1.0F;
            float u2 = 0.0F;
            float v2 = 0.0F;
            float q2 = 1.0F;
            float clip_w = 1.0F;
            std::uint32_t abgr = 0xffffffffU;
        };

        struct StaticGeometryEntry {
            std::uint8_t kind = 0U;
            std::uint64_t hash = 0U;
            std::uint32_t vertex_count = 0U;
            std::uint32_t index_count = 0U;
            std::size_t byte_size = 0U;
            std::uint64_t last_used_frame = 0U;
            bgfx::VertexBufferHandle vertex_buffer = BGFX_INVALID_HANDLE;
            bgfx::IndexBufferHandle index_buffer = BGFX_INVALID_HANDLE;
        };

        struct RecentGeometryKey {
            std::uint8_t kind = 0U;
            std::uint64_t hash = 0U;
            std::uint32_t vertex_count = 0U;
            std::uint32_t index_count = 0U;
            std::uint64_t last_seen_frame = 0U;
            std::uint32_t seen_count = 0U;
        };

        [[nodiscard]] std::uint64_t hash_textured_geometry(const core::TexturedTriangleBatch2D& batch) const;
        [[nodiscard]] std::uint64_t hash_gx_material_geometry(const core::GxMaterialTriangleBatch2D& batch) const;
        [[nodiscard]] StaticGeometryEntry *find_static_geometry(std::uint8_t kind, std::uint64_t hash, std::uint32_t vertex_count,
                                                                std::uint32_t index_count);
        [[nodiscard]] bool should_promote_static_geometry(std::uint8_t kind, std::uint64_t hash, std::uint32_t vertex_count,
                                                          std::uint32_t index_count);
        void fill_quad_vertices(const core::TexturedTriangleBatch2D& batch, std::vector<QuadVertex>& out) const;
        void fill_gx_material_vertices(const core::GxMaterialTriangleBatch2D& batch, std::vector<GxMaterialVertex>& out) const;
        [[nodiscard]] StaticGeometryEntry *create_static_textured_geometry(std::uint64_t hash, const core::TexturedTriangleBatch2D& batch);
        [[nodiscard]] StaticGeometryEntry *create_static_gx_material_geometry(std::uint64_t hash, const core::GxMaterialTriangleBatch2D& batch);
        void evict_static_geometry_cache();
        void clear_static_geometry_cache();
        void print_static_geometry_cache_stats() const;
        void print_backend_stats() const;

        bool _initialized = false;
        bool _vsync_enabled = true;
        bool _backend_stats_enabled = false;
        bool _static_geometry_cache_enabled = false;
        bool _static_geometry_cache_stats_enabled = false;
        std::uint16_t _framebuffer_width = 1U;
        std::uint16_t _framebuffer_height = 1U;
        std::uint16_t _logical_framebuffer_width = core::kWiiLogicalFramebufferWidth;
        std::uint16_t _logical_framebuffer_height = core::kWiiLogicalFramebufferHeight;
        std::uint64_t _frame_index = 0U;
        std::string _pending_screenshot_path{};
        std::unique_ptr< BgfxCallback > _callback{};
        std::vector< bgfx::TextureHandle > _textures{};
        std::vector< StaticGeometryEntry > _static_geometry_cache{};
        std::vector< RecentGeometryKey > _recent_geometry_keys{};
        std::size_t _static_geometry_cache_bytes = 0U;
        std::uint64_t _static_geometry_cache_hits = 0U;
        std::uint64_t _static_geometry_cache_misses = 0U;
        std::uint64_t _static_geometry_cache_promotions = 0U;
        std::uint64_t _static_geometry_cache_evictions = 0U;
        std::uint64_t _static_geometry_transient_submits = 0U;
        std::uint64_t _textured_triangle_submits = 0U;
        std::uint64_t _gx_material_submits = 0U;
        std::uint64_t _dropped_transient_submits = 0U;
        std::uint64_t _texture_binds = 0U;
        std::uint64_t _uniform_updates = 0U;
        std::uint64_t _transient_vertex_bytes = 0U;
        std::uint64_t _transient_index_bytes = 0U;
        std::uint64_t _bgfx_frame_count = 0U;
        double _bgfx_frame_ms = 0.0;
        std::vector< QuadVertex > _quad_upload_vertices{};
        std::vector< GxMaterialVertex > _gx_upload_vertices{};
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
