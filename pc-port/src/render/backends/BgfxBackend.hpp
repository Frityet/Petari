#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <memory>
#include <optional>
#include <unordered_map>
#include <string>
#include <string_view>

#include <bgfx/bgfx.h>

#include "IRenderBackend.hpp"

namespace smgpc::render::backends {

class BgfxBackend final : public IRenderBackend {
public:
    BgfxBackend();
    ~BgfxBackend() override;

    BgfxBackend(const BgfxBackend &) = delete;
    BgfxBackend &operator=(const BgfxBackend &) = delete;

    void initialize(const smgpc::render::core::RenderInitDesc &description) override;
    void shutdown() override;
    void begin_frame(const smgpc::render::core::FrameContext &frame_context) override;
    void execute(const smgpc::render::core::RenderCommandBuffer &commands) override;
    void end_frame() override;
    void resize(std::uint16_t width, std::uint16_t height) override;
    void request_capture(const smgpc::render::core::RenderCaptureRequest &request) override;
    [[nodiscard]] std::optional<std::filesystem::path> poll_completed_capture() override;
    [[nodiscard]] smgpc::render::core::FramebufferInfo framebuffer_size() const override;
    [[nodiscard]] bool is_initialized() const override;

private:
    class Callbacks;

    void execute_command(const smgpc::render::core::RenderCommand &command);
    void ensure_layout_resources();
    void ensure_j3d_resources();
    void execute_draw_layout(const smgpc::render::core::RenderDrawLayoutCommand &command);
    void execute_draw_j3d(const smgpc::render::core::RenderDrawJ3dCommand &command);
    void execute_clear(const smgpc::render::core::RenderClearCommand &command);
    [[nodiscard]] bgfx::TextureHandle resolve_texture(const smgpc::render::core::RenderTextureRef &texture);

    bool _initialized {false};
    bool _vsync_enabled {true};
    std::uint16_t _framebuffer_width {1U};
    std::uint16_t _framebuffer_height {1U};
    void *_window_handle {nullptr};
    void *_display_handle {nullptr};

    std::unique_ptr<Callbacks> _callbacks {};
    bool _layout_resources_ready {false};
    bool _j3d_resources_ready {false};

    std::deque<std::string> _pending_capture_paths {};
    std::unordered_map<std::uint64_t, bgfx::TextureHandle> _textures {};

    [[nodiscard]] static constexpr bgfx::ProgramHandle invalid_program_handle() { return bgfx::ProgramHandle {bgfx::kInvalidHandle}; }
    [[nodiscard]] static constexpr bgfx::UniformHandle invalid_uniform_handle() { return bgfx::UniformHandle {bgfx::kInvalidHandle}; }
    [[nodiscard]] static constexpr bgfx::TextureHandle invalid_texture_handle() { return bgfx::TextureHandle {bgfx::kInvalidHandle}; }

    bgfx::ProgramHandle _layout_program {invalid_program_handle()};
    bgfx::UniformHandle _sampler {invalid_uniform_handle()};
    bgfx::UniformHandle _mask_sampler {invalid_uniform_handle()};
    bgfx::UniformHandle _mask_params {invalid_uniform_handle()};
    bgfx::UniformHandle _wrap_params {invalid_uniform_handle()};
    bgfx::UniformHandle _tev_color0 {invalid_uniform_handle()};
    bgfx::UniformHandle _tev_color1 {invalid_uniform_handle()};
    bgfx::UniformHandle _triangle_tev_stages {invalid_uniform_handle()};
    bgfx::TextureHandle _white_texture {invalid_texture_handle()};
    bgfx::VertexLayout _layout_vertex {};

    bgfx::ProgramHandle _j3d_program {invalid_program_handle()};
    std::array<bgfx::UniformHandle, 4U> _j3d_samplers {
        invalid_uniform_handle(),
        invalid_uniform_handle(),
        invalid_uniform_handle(),
        invalid_uniform_handle(),
    };
    bgfx::UniformHandle _j3d_params {invalid_uniform_handle()};
    bgfx::UniformHandle _j3d_wrap_params {invalid_uniform_handle()};
    bgfx::UniformHandle _j3d_tev_color0 {invalid_uniform_handle()};
    bgfx::UniformHandle _j3d_tev_color1 {invalid_uniform_handle()};
    bgfx::UniformHandle _j3d_tev_colors {invalid_uniform_handle()};
    bgfx::UniformHandle _j3d_k_colors {invalid_uniform_handle()};
    bgfx::UniformHandle _j3d_tev_stages {invalid_uniform_handle()};
    bgfx::UniformHandle _j3d_tev_alpha_stages {invalid_uniform_handle()};
    bgfx::UniformHandle _j3d_tev_color_dests {invalid_uniform_handle()};
    bgfx::UniformHandle _j3d_tev_alpha_dests {invalid_uniform_handle()};
    bgfx::UniformHandle _j3d_tev_texture_indices {invalid_uniform_handle()};
    bgfx::UniformHandle _j3d_tev_k_color_selectors {invalid_uniform_handle()};
    bgfx::UniformHandle _j3d_tev_k_alpha_selectors {invalid_uniform_handle()};
    bgfx::UniformHandle _j3d_tev_texture_swizzles {invalid_uniform_handle()};
    bgfx::UniformHandle _j3d_tev_raster_swizzles {invalid_uniform_handle()};
    bgfx::UniformHandle _j3d_alpha_compare {invalid_uniform_handle()};
    bgfx::UniformHandle _j3d_alpha_compare_extra {invalid_uniform_handle()};
    bgfx::UniformHandle _j3d_texture_sizes {invalid_uniform_handle()};
    bgfx::UniformHandle _j3d_ind_params {invalid_uniform_handle()};
    bgfx::UniformHandle _j3d_ind_orders {invalid_uniform_handle()};
    bgfx::UniformHandle _j3d_ind_matrices {invalid_uniform_handle()};
    bgfx::UniformHandle _j3d_tev_indirects {invalid_uniform_handle()};
    bgfx::VertexLayout _j3d_vertex {};
};

}  // namespace smgpc::render::backends
