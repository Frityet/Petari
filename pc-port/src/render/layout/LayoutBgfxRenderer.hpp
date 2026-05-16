#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <unordered_map>
#include <vector>

#include <bgfx/bgfx.h>

#include "LayoutDrawList.hpp"
#include "ServiceProvider.hpp"

namespace smgpc::logging {
class ILogger;
}

namespace smgpc::render::layout {

class LayoutBgfxRenderer {
public:
    explicit LayoutBgfxRenderer(di::DependencyReference<logging::ILogger> logger);
    ~LayoutBgfxRenderer();

    LayoutBgfxRenderer(const LayoutBgfxRenderer &) = delete;
    LayoutBgfxRenderer &operator=(const LayoutBgfxRenderer &) = delete;

    void draw(
        const LayoutDrawList &draw_list,
        std::uint16_t framebuffer_width,
        std::uint16_t framebuffer_height,
        float layout_width,
        float layout_height);

private:
    struct Vertex {
        float x {};
        float y {};
        float z {};
        float w {1.0F};
        std::uint32_t abgr {};
        float u {};
        float v {};
        float q {1.0F};
        float u_mask {};
        float v_mask {};
        float q_mask {1.0F};

        static bgfx::VertexLayout layout;
        static void init_layout();
    };

    void ensure_initialized();
    void shutdown();

    [[nodiscard]] bgfx::TextureHandle resolve_texture(const TextureRef &texture);
    [[nodiscard]] static std::vector<std::byte> read_file_bytes(const std::filesystem::path &path);
    [[nodiscard]] static std::filesystem::path resolve_shader_directory();
    [[nodiscard]] static constexpr bgfx::UniformHandle invalid_uniform_handle() { return bgfx::UniformHandle {bgfx::kInvalidHandle}; }
    [[nodiscard]] static constexpr bgfx::ProgramHandle invalid_program_handle() { return bgfx::ProgramHandle {bgfx::kInvalidHandle}; }
    [[nodiscard]] static constexpr bgfx::TextureHandle invalid_texture_handle() { return bgfx::TextureHandle {bgfx::kInvalidHandle}; }

    di::DependencyReference<logging::ILogger> _logger;

    bool _initialized {};
    bgfx::UniformHandle _sampler {invalid_uniform_handle()};
    bgfx::UniformHandle _mask_sampler {invalid_uniform_handle()};
    bgfx::UniformHandle _mask_params {invalid_uniform_handle()};
    bgfx::UniformHandle _wrap_params {invalid_uniform_handle()};
    bgfx::UniformHandle _tev_color0 {invalid_uniform_handle()};
    bgfx::UniformHandle _tev_color1 {invalid_uniform_handle()};
    bgfx::ProgramHandle _program {invalid_program_handle()};
    bgfx::TextureHandle _white_texture {invalid_texture_handle()};
    std::unordered_map<std::uint64_t, bgfx::TextureHandle> _texture_cache {};
};

}  // namespace smgpc::render::layout
