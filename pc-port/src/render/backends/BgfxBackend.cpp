#include "BgfxBackend.hpp"

#include <algorithm>
#include <array>
#include <deque>
#include <fstream>
#include <cstdlib>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>
#include <filesystem>

#if defined(__linux__)
#include <unistd.h>
#endif

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>

#include "ScreenshotWriter.hpp"

namespace smgpc::render::backends {
namespace {

std::optional<bgfx::RendererType::Enum> resolve_renderer_type_from_environment() {
    const char *value = std::getenv("SMGPC_BGFX_RENDERER");
    if (value == nullptr || value[0] == '\0') {
        return std::nullopt;
    }

    const auto renderer_name = std::string_view(value);
    if (renderer_name == "vulkan" || renderer_name == "vk") {
        return bgfx::RendererType::Vulkan;
    }
    if (renderer_name == "opengl" || renderer_name == "gl") {
        return bgfx::RendererType::OpenGL;
    }
    return bgfx::RendererType::Count;
}

[[nodiscard]] const char *shader_dir_name_from_renderer(bgfx::RendererType::Enum renderer_type) {
    if (renderer_type == bgfx::RendererType::Vulkan) {
        return "spirv";
    }
    return "glsl";
}

[[nodiscard]] std::uint32_t sampler_flags_for_wrap(std::uint8_t wrap_s, std::uint8_t wrap_t) {
    constexpr std::uint8_t GX_CLAMP = 0U;
    constexpr std::uint8_t GX_MIRROR = 2U;

    std::uint32_t flags = 0U;
    if (wrap_s == GX_CLAMP) {
        flags |= BGFX_SAMPLER_U_CLAMP;
    } else if (wrap_s == GX_MIRROR) {
        flags |= BGFX_SAMPLER_U_MIRROR;
    }

    if (wrap_t == GX_CLAMP) {
        flags |= BGFX_SAMPLER_V_CLAMP;
    } else if (wrap_t == GX_MIRROR) {
        flags |= BGFX_SAMPLER_V_MIRROR;
    }

    return flags;
}

[[nodiscard]] std::uint64_t texture_cache_key(const smgpc::render::core::RenderTextureRef &texture) {
    const auto sampler_flags = sampler_flags_for_wrap(texture.wrap_s, texture.wrap_t);
    return texture.id ^ (static_cast<std::uint64_t>(sampler_flags) << 32U);
}

[[nodiscard]] bool texture_ref_is_valid(const smgpc::render::core::RenderTextureRef &texture) {
    return texture.id != 0U && texture.rgba8 != nullptr && texture.width != 0U && texture.height != 0U;
}

[[nodiscard]] std::vector<std::uint8_t> build_rgba8_mip_chain(const smgpc::render::core::RenderTextureRef &texture) {
    const auto base_byte_count = static_cast<std::size_t>(texture.width) * static_cast<std::size_t>(texture.height) * 4U;
    std::vector<std::uint8_t> mip_chain(texture.rgba8, texture.rgba8 + base_byte_count);
    std::vector<std::uint8_t> current(texture.rgba8, texture.rgba8 + base_byte_count);

    std::uint32_t width = texture.width;
    std::uint32_t height = texture.height;
    while (width > 1U || height > 1U) {
        const std::uint32_t next_width = std::max<std::uint32_t>(1U, width / 2U);
        const std::uint32_t next_height = std::max<std::uint32_t>(1U, height / 2U);
        std::vector<std::uint8_t> next(static_cast<std::size_t>(next_width) * static_cast<std::size_t>(next_height) * 4U);

        for (std::uint32_t y = 0U; y < next_height; ++y) {
            for (std::uint32_t x = 0U; x < next_width; ++x) {
                std::array<std::uint32_t, 4U> sum {};
                std::uint32_t sample_count = 0U;
                for (std::uint32_t dy = 0U; dy < 2U; ++dy) {
                    const std::uint32_t source_y = y * 2U + dy;
                    if (source_y >= height) {
                        continue;
                    }
                    for (std::uint32_t dx = 0U; dx < 2U; ++dx) {
                        const std::uint32_t source_x = x * 2U + dx;
                        if (source_x >= width) {
                            continue;
                        }

                        const auto source_index = (static_cast<std::size_t>(source_y) * width + source_x) * 4U;
                        for (std::size_t channel = 0U; channel < sum.size(); ++channel) {
                            sum[channel] += current[source_index + channel];
                        }
                        ++sample_count;
                    }
                }

                const auto target_index = (static_cast<std::size_t>(y) * next_width + x) * 4U;
                for (std::size_t channel = 0U; channel < sum.size(); ++channel) {
                    next[target_index + channel] = static_cast<std::uint8_t>((sum[channel] + sample_count / 2U) / sample_count);
                }
            }
        }

        mip_chain.insert(mip_chain.end(), next.begin(), next.end());
        current = std::move(next);
        width = next_width;
        height = next_height;
    }

    return mip_chain;
}

[[nodiscard]] float triangle_texture_combine_uniform(smgpc::render::core::RenderTriangleTextureCombineMode mode) {
    switch (mode) {
    case smgpc::render::core::RenderTriangleTextureCombineMode::Multiply:
        return -1.0F;
    case smgpc::render::core::RenderTriangleTextureCombineMode::Add:
        return -2.0F;
    case smgpc::render::core::RenderTriangleTextureCombineMode::Screen:
        return -3.0F;
    case smgpc::render::core::RenderTriangleTextureCombineMode::J3dTevColorStages:
        return -4.0F;
    case smgpc::render::core::RenderTriangleTextureCombineMode::None:
    default:
        return 0.0F;
    }
}

[[nodiscard]] float optional_float_from_environment(const char *name, float fallback) {
    const char *value = std::getenv(name);
    if (value == nullptr || value[0] == '\0') {
        return fallback;
    }

    char *end = nullptr;
    const float parsed = std::strtof(value, &end);
    return end != value && *end == '\0' ? parsed : fallback;
}

void packed_color_to_uniform(std::uint32_t color, float *pOut) {
    pOut[0U] = static_cast<float>(color & 0xFFU) / 255.0F;
    pOut[1U] = static_cast<float>((color >> 8U) & 0xFFU) / 255.0F;
    pOut[2U] = static_cast<float>((color >> 16U) & 0xFFU) / 255.0F;
    pOut[3U] = static_cast<float>((color >> 24U) & 0xFFU) / 255.0F;
}

[[nodiscard]] std::uint64_t cull_state_for_mode(smgpc::render::core::RenderCullMode mode) {
    switch (mode) {
    case smgpc::render::core::RenderCullMode::Front:
        return BGFX_STATE_CULL_CW;
    case smgpc::render::core::RenderCullMode::Back:
        return BGFX_STATE_CULL_CCW;
    case smgpc::render::core::RenderCullMode::None:
    default:
        return 0U;
    }
}

[[nodiscard]] std::uint64_t depth_state_for_mode(smgpc::render::core::RenderDepthMode mode) {
    switch (mode) {
    case smgpc::render::core::RenderDepthMode::Less:
        return BGFX_STATE_DEPTH_TEST_LESS;
    case smgpc::render::core::RenderDepthMode::LessEqual:
        return BGFX_STATE_DEPTH_TEST_LEQUAL;
    case smgpc::render::core::RenderDepthMode::Always:
    default:
        return BGFX_STATE_DEPTH_TEST_ALWAYS;
    }
}

[[nodiscard]] std::uint64_t draw_state_for_j3d_batch(const smgpc::render::core::RenderJ3dMaterialBatch &batch) {
    if (std::getenv("SMGPC_J3D_DEBUG_VERTEX_COLOR") != nullptr) {
        return BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_ALWAYS;
    }

    std::uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | cull_state_for_mode(batch.cull_mode) | depth_state_for_mode(batch.depth_mode);
    if (batch.write_depth) {
        state |= BGFX_STATE_WRITE_Z;
    }
    if (batch.blend_mode == smgpc::render::core::RenderBlendMode::Additive) {
        state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_ONE);
    } else if (batch.blend_mode == smgpc::render::core::RenderBlendMode::Alpha) {
        state |= BGFX_STATE_BLEND_ALPHA;
    }
    return state;
}

[[nodiscard]] std::vector<std::byte> read_file_bytes(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    if (not file.is_open()) {
        throw std::runtime_error("Failed to read shader file: " + path.string());
    }

    std::vector<std::byte> bytes {};
    char byte {};
    while (file.get(byte)) {
        bytes.push_back(std::byte {static_cast<std::uint8_t>(byte)});
    }
    return bytes;
}

[[nodiscard]] std::optional<std::filesystem::path> resolve_executable_directory() {
#if defined(__linux__)
    std::array<char, 4096> buffer {};
    const auto bytes = ::readlink("/proc/self/exe", buffer.data(), buffer.size() - 1U);
    if (bytes <= 0) {
        return std::nullopt;
    }

    buffer[static_cast<std::size_t>(bytes)] = '\0';
    return std::filesystem::path(buffer.data()).parent_path();
#else
    return std::nullopt;
#endif
}

[[nodiscard]] std::filesystem::path resolve_shader_directory() {
    const auto backend_dir = shader_dir_name_from_renderer(bgfx::getRendererType());
    std::vector<std::filesystem::path> candidates {};

    if (const auto executable_directory = resolve_executable_directory(); executable_directory.has_value()) {
        candidates.push_back(*executable_directory / "shaders");
    }

    candidates.push_back(std::filesystem::current_path() / "build" / "linux" / "x86_64" / "debug" / "shaders");
    candidates.push_back(std::filesystem::current_path() / "build" / "linux" / "x86_64" / "release" / "shaders");
    candidates.push_back(std::filesystem::current_path() / "pc-port" / "build" / "linux" / "x86_64" / "debug" / "shaders");
    candidates.push_back(std::filesystem::current_path() / "pc-port" / "build" / "linux" / "x86_64" / "release" / "shaders");
    candidates.push_back(std::filesystem::current_path() / "shaders");
    candidates.push_back(std::filesystem::current_path().parent_path() / "build" / "linux" / "x86_64" / "debug" / "shaders");
    candidates.push_back(std::filesystem::current_path().parent_path() / "build" / "linux" / "x86_64" / "release" / "shaders");
    candidates.push_back(std::filesystem::current_path().parent_path() / "shaders");

    for (const auto &candidate : candidates) {
        const auto backend_candidate = candidate / backend_dir;
        if (std::filesystem::exists(backend_candidate / "vs_layout.bin") and std::filesystem::exists(backend_candidate / "fs_layout.bin")) {
            return backend_candidate;
        }
    }

    throw std::runtime_error("Failed to locate layout shaders in render backend directory path list.");
}

}  // namespace

class BgfxBackend::Callbacks final : public bgfx::CallbackI {
public:
    void fatal(const char * /* file_path */, std::uint16_t /* line */, bgfx::Fatal::Enum /* code */, const char *message) override {
        throw std::runtime_error(message);
    }

    void traceVargs(const char * /* file_path */, std::uint16_t /* line */, const char * /* format */, va_list) override {
    }

    void profilerBegin(const char *, std::uint32_t, const char *, std::uint16_t) override {
    }

    void profilerBeginLiteral(const char *, std::uint32_t, const char *, std::uint16_t) override {
    }

    void profilerEnd() override {
    }

    void captureBegin(
        std::uint32_t,
        std::uint32_t,
        std::uint32_t,
        bgfx::TextureFormat::Enum,
        bool) override {
    }

    void captureEnd() override {
    }

    void captureFrame(const void *, std::uint32_t) override {
    }

    [[nodiscard]] uint32_t cacheReadSize(uint64_t) override {
        return 0U;
    }

    bool cacheRead(uint64_t, void *, uint32_t) override {
        return false;
    }

    void cacheWrite(uint64_t, const void *, uint32_t) override {
    }

    void screenShot(const char *file_path, uint32_t width, uint32_t height, uint32_t pitch, const void *data, uint32_t, bool yflip) override {
        const auto output_path = file_path == nullptr ? _output_directory / "capture.png" : std::filesystem::path(file_path);
        const auto success = write_screenshot_png(output_path, width, height, pitch, data, yflip);
        if (success) {
            auto lock = std::scoped_lock(_capture_mutex);
            _completed_captures.push_back(output_path);
        }
    }

    [[nodiscard]] std::optional<std::filesystem::path> poll_completed_capture() {
        auto lock = std::scoped_lock(_capture_mutex);
        if (_completed_captures.empty()) {
            return std::nullopt;
        }

        auto output_path = _completed_captures.front();
        _completed_captures.pop_front();
        return output_path;
    }

private:
    std::filesystem::path _output_directory {std::filesystem::temp_directory_path() / "smgpc"};
    std::deque<std::filesystem::path> _completed_captures {};
    mutable std::mutex _capture_mutex {};
};

BgfxBackend::BgfxBackend()
    : _callbacks(std::make_unique<Callbacks>()) {
}

BgfxBackend::~BgfxBackend() {
    shutdown();
}

void BgfxBackend::initialize(const smgpc::render::core::RenderInitDesc &description) {
    if (_initialized) {
        return;
    }

    _window_handle = description.native_window_handle;
    _display_handle = description.native_display_handle;
    _vsync_enabled = description.enable_vsync;

    const auto try_initialize = [&](bgfx::RendererType::Enum renderer_type) {
        auto init = bgfx::Init();
        init.platformData.nwh = _window_handle;
        init.platformData.ndt = _display_handle;
        init.type = renderer_type;
        init.resolution.width = description.width;
        init.resolution.height = description.height;
        init.resolution.reset = _vsync_enabled ? BGFX_RESET_VSYNC : BGFX_RESET_NONE;
        init.callback = _callbacks.get();
        return bgfx::init(init);
    };

    const auto requested_renderer = resolve_renderer_type_from_environment();
    if (requested_renderer.has_value()) {
        if (not try_initialize(*requested_renderer)) {
            throw std::runtime_error("Cannot init bgfx with requested renderer type");
        }
    } else {
#if defined(__linux__)
        constexpr std::array<bgfx::RendererType::Enum, 3> renderer_fallback {
            bgfx::RendererType::Vulkan,
            bgfx::RendererType::OpenGL,
            bgfx::RendererType::Count,
        };
#else
        constexpr std::array<bgfx::RendererType::Enum, 2> renderer_fallback {
            bgfx::RendererType::OpenGL,
            bgfx::RendererType::Count,
        };
#endif

        bool initialized = false;
        for (const auto renderer_type : renderer_fallback) {
            if (try_initialize(renderer_type)) {
                initialized = true;
                break;
            }
        }
        if (not initialized) {
            throw std::runtime_error("Cannot init bgfx");
        }
    }

    _initialized = true;
    _framebuffer_width = description.width;
    _framebuffer_height = description.height;
}

void BgfxBackend::shutdown() {
    if (not _initialized) {
        return;
    }

    for (auto &[_, handle] : _textures) {
        if (bgfx::isValid(handle)) {
            bgfx::destroy(handle);
        }
    }
    _textures.clear();

    if (bgfx::isValid(_layout_program)) {
        bgfx::destroy(_layout_program);
    }
    if (bgfx::isValid(_j3d_program)) {
        bgfx::destroy(_j3d_program);
    }

    if (bgfx::isValid(_sampler)) {
        bgfx::destroy(_sampler);
    }

    if (bgfx::isValid(_mask_sampler)) {
        bgfx::destroy(_mask_sampler);
    }

    if (bgfx::isValid(_mask_params)) {
        bgfx::destroy(_mask_params);
    }
    if (bgfx::isValid(_wrap_params)) {
        bgfx::destroy(_wrap_params);
    }
    if (bgfx::isValid(_tev_color0)) {
        bgfx::destroy(_tev_color0);
    }
    if (bgfx::isValid(_tev_color1)) {
        bgfx::destroy(_tev_color1);
    }
    if (bgfx::isValid(_triangle_tev_stages)) {
        bgfx::destroy(_triangle_tev_stages);
    }
    for (auto &sampler : _j3d_samplers) {
        if (bgfx::isValid(sampler)) {
            bgfx::destroy(sampler);
        }
    }
    if (bgfx::isValid(_j3d_params)) {
        bgfx::destroy(_j3d_params);
    }
    if (bgfx::isValid(_j3d_wrap_params)) {
        bgfx::destroy(_j3d_wrap_params);
    }
    if (bgfx::isValid(_j3d_tev_color0)) {
        bgfx::destroy(_j3d_tev_color0);
    }
    if (bgfx::isValid(_j3d_tev_color1)) {
        bgfx::destroy(_j3d_tev_color1);
    }
    if (bgfx::isValid(_j3d_tev_colors)) {
        bgfx::destroy(_j3d_tev_colors);
    }
    if (bgfx::isValid(_j3d_k_colors)) {
        bgfx::destroy(_j3d_k_colors);
    }
    if (bgfx::isValid(_j3d_tev_stages)) {
        bgfx::destroy(_j3d_tev_stages);
    }
    if (bgfx::isValid(_j3d_tev_alpha_stages)) {
        bgfx::destroy(_j3d_tev_alpha_stages);
    }
    if (bgfx::isValid(_j3d_tev_color_dests)) {
        bgfx::destroy(_j3d_tev_color_dests);
    }
    if (bgfx::isValid(_j3d_tev_alpha_dests)) {
        bgfx::destroy(_j3d_tev_alpha_dests);
    }
    if (bgfx::isValid(_j3d_tev_texture_indices)) {
        bgfx::destroy(_j3d_tev_texture_indices);
    }
    if (bgfx::isValid(_j3d_tev_k_color_selectors)) {
        bgfx::destroy(_j3d_tev_k_color_selectors);
    }
    if (bgfx::isValid(_j3d_tev_k_alpha_selectors)) {
        bgfx::destroy(_j3d_tev_k_alpha_selectors);
    }
    if (bgfx::isValid(_j3d_tev_texture_swizzles)) {
        bgfx::destroy(_j3d_tev_texture_swizzles);
    }
    if (bgfx::isValid(_j3d_tev_raster_swizzles)) {
        bgfx::destroy(_j3d_tev_raster_swizzles);
    }
    if (bgfx::isValid(_j3d_alpha_compare)) {
        bgfx::destroy(_j3d_alpha_compare);
    }
    if (bgfx::isValid(_j3d_alpha_compare_extra)) {
        bgfx::destroy(_j3d_alpha_compare_extra);
    }
    if (bgfx::isValid(_j3d_texture_sizes)) {
        bgfx::destroy(_j3d_texture_sizes);
    }
    if (bgfx::isValid(_j3d_ind_params)) {
        bgfx::destroy(_j3d_ind_params);
    }
    if (bgfx::isValid(_j3d_ind_orders)) {
        bgfx::destroy(_j3d_ind_orders);
    }
    if (bgfx::isValid(_j3d_ind_matrices)) {
        bgfx::destroy(_j3d_ind_matrices);
    }
    if (bgfx::isValid(_j3d_tev_indirects)) {
        bgfx::destroy(_j3d_tev_indirects);
    }

    if (bgfx::isValid(_white_texture)) {
        bgfx::destroy(_white_texture);
    }

    _layout_program = BGFX_INVALID_HANDLE;
    _j3d_program = BGFX_INVALID_HANDLE;
    _sampler = BGFX_INVALID_HANDLE;
    _mask_sampler = BGFX_INVALID_HANDLE;
    _mask_params = BGFX_INVALID_HANDLE;
    _wrap_params = BGFX_INVALID_HANDLE;
    _tev_color0 = BGFX_INVALID_HANDLE;
    _tev_color1 = BGFX_INVALID_HANDLE;
    _triangle_tev_stages = BGFX_INVALID_HANDLE;
    for (auto &sampler : _j3d_samplers) {
        sampler = BGFX_INVALID_HANDLE;
    }
    _j3d_params = BGFX_INVALID_HANDLE;
    _j3d_wrap_params = BGFX_INVALID_HANDLE;
    _j3d_tev_color0 = BGFX_INVALID_HANDLE;
    _j3d_tev_color1 = BGFX_INVALID_HANDLE;
    _j3d_tev_colors = BGFX_INVALID_HANDLE;
    _j3d_k_colors = BGFX_INVALID_HANDLE;
    _j3d_tev_stages = BGFX_INVALID_HANDLE;
    _j3d_tev_alpha_stages = BGFX_INVALID_HANDLE;
    _j3d_tev_color_dests = BGFX_INVALID_HANDLE;
    _j3d_tev_alpha_dests = BGFX_INVALID_HANDLE;
    _j3d_tev_texture_indices = BGFX_INVALID_HANDLE;
    _j3d_tev_k_color_selectors = BGFX_INVALID_HANDLE;
    _j3d_tev_k_alpha_selectors = BGFX_INVALID_HANDLE;
    _j3d_tev_texture_swizzles = BGFX_INVALID_HANDLE;
    _j3d_tev_raster_swizzles = BGFX_INVALID_HANDLE;
    _j3d_alpha_compare = BGFX_INVALID_HANDLE;
    _j3d_alpha_compare_extra = BGFX_INVALID_HANDLE;
    _j3d_texture_sizes = BGFX_INVALID_HANDLE;
    _j3d_ind_params = BGFX_INVALID_HANDLE;
    _j3d_ind_orders = BGFX_INVALID_HANDLE;
    _j3d_ind_matrices = BGFX_INVALID_HANDLE;
    _j3d_tev_indirects = BGFX_INVALID_HANDLE;
    _white_texture = BGFX_INVALID_HANDLE;
    _layout_resources_ready = false;
    _j3d_resources_ready = false;

    bgfx::shutdown();
    _initialized = false;
}

void BgfxBackend::begin_frame(const smgpc::render::core::FrameContext &frame_context) {
    if (not _initialized) {
        return;
    }

    bgfx::setViewRect(0U, 0U, 0U, frame_context.framebuffer.width, frame_context.framebuffer.height);
    bgfx::setViewMode(0U, bgfx::ViewMode::Sequential);
    bgfx::setViewClear(0U, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0F, 0U);
}

void BgfxBackend::execute(const smgpc::render::core::RenderCommandBuffer &commands) {
    if (not _initialized) {
        return;
    }

    for (const auto &command : commands.commands()) {
        execute_command(command);
    }
}

void BgfxBackend::execute_command(const smgpc::render::core::RenderCommand &command) {
    using CommandType = smgpc::render::core::RenderCommandType;
    switch (command.type) {
    case CommandType::Clear: {
        execute_clear(std::get<smgpc::render::core::RenderClearCommand>(command.payload));
        break;
    }
    case CommandType::SetViewport: {
        const auto &viewport = std::get<smgpc::render::core::RenderViewportCommand>(command.payload);
        bgfx::setViewRect(viewport.view_id, viewport.x, viewport.y, viewport.width, viewport.height);
        break;
    }
    case CommandType::SetScissor: {
        const auto &scissor = std::get<smgpc::render::core::RenderScissorCommand>(command.payload);
        bgfx::setScissor(scissor.x, scissor.y, scissor.width, scissor.height);
        break;
    }
    case CommandType::DrawLayout: {
        execute_draw_layout(std::get<smgpc::render::core::RenderDrawLayoutCommand>(command.payload));
        break;
    }
    case CommandType::DrawJ3d: {
        execute_draw_j3d(std::get<smgpc::render::core::RenderDrawJ3dCommand>(command.payload));
        break;
    }
    case CommandType::Marker: {
        (void)std::get<smgpc::render::core::RenderMarkerCommand>(command.payload);
        break;
    }
    case CommandType::BindTexture:
    case CommandType::Barrier:
    default:
        break;
    }
}

void BgfxBackend::end_frame() {
    if (not _initialized) {
        return;
    }

    bgfx::frame();
}

void BgfxBackend::resize(std::uint16_t width, std::uint16_t height) {
    if (not _initialized || width == 0U || height == 0U) {
        return;
    }

    _framebuffer_width = width;
    _framebuffer_height = height;
    bgfx::reset(width, height, _vsync_enabled ? BGFX_RESET_VSYNC : BGFX_RESET_NONE);
}

void BgfxBackend::request_capture(const smgpc::render::core::RenderCaptureRequest &request) {
    if (not _initialized) {
        return;
    }

    _pending_capture_paths.push_back(request.path.string());
    bgfx::requestScreenShot(BGFX_INVALID_HANDLE, _pending_capture_paths.back().c_str());
}

std::optional<std::filesystem::path> BgfxBackend::poll_completed_capture() {
    if (not _initialized) {
        return std::nullopt;
    }

    return _callbacks->poll_completed_capture();
}

smgpc::render::core::FramebufferInfo BgfxBackend::framebuffer_size() const {
    return smgpc::render::core::FramebufferInfo {
        .width = _framebuffer_width,
        .height = _framebuffer_height,
    };
}

bool BgfxBackend::is_initialized() const {
    return _initialized;
}

void BgfxBackend::ensure_layout_resources() {
    if (_layout_resources_ready) {
        return;
    }

    _layout_vertex = bgfx::VertexLayout();
    _layout_vertex.begin()
        .add(bgfx::Attrib::Position, 4, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .add(bgfx::Attrib::TexCoord0, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord1, 3, bgfx::AttribType::Float)
        .end();

    const auto shader_directory = resolve_shader_directory();
    const auto vertex_shader_bytes = read_file_bytes(shader_directory / "vs_layout.bin");
    const auto fragment_shader_bytes = read_file_bytes(shader_directory / "fs_layout.bin");
    const auto *vertex_memory = bgfx::copy(vertex_shader_bytes.data(), static_cast<uint32_t>(vertex_shader_bytes.size()));
    const auto *fragment_memory = bgfx::copy(fragment_shader_bytes.data(), static_cast<uint32_t>(fragment_shader_bytes.size()));
    const auto vertex_shader = bgfx::createShader(vertex_memory);
    const auto fragment_shader = bgfx::createShader(fragment_memory);
    _layout_program = bgfx::createProgram(vertex_shader, fragment_shader, true);

    _sampler = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);
    _mask_sampler = bgfx::createUniform("s_mask", bgfx::UniformType::Sampler);
    _mask_params = bgfx::createUniform("u_mask_params", bgfx::UniformType::Vec4);
    _wrap_params = bgfx::createUniform("u_wrap_params", bgfx::UniformType::Vec4);
    _tev_color0 = bgfx::createUniform("u_tev_color0", bgfx::UniformType::Vec4);
    _tev_color1 = bgfx::createUniform("u_tev_color1", bgfx::UniformType::Vec4);
    _triangle_tev_stages = bgfx::createUniform("u_triangle_tev_stages", bgfx::UniformType::Vec4, 4U);

    if (not bgfx::isValid(_white_texture)) {
        const std::uint32_t white_pixel = 0xFFFFFFFFU;
        const auto *white_memory = bgfx::copy(&white_pixel, sizeof(white_pixel));
        _white_texture = bgfx::createTexture2D(1U, 1U, false, 1U, bgfx::TextureFormat::BGRA8, 0U, white_memory);
    }

    _layout_resources_ready = true;
}

void BgfxBackend::ensure_j3d_resources() {
    if (_j3d_resources_ready) {
        return;
    }

    _j3d_vertex = bgfx::VertexLayout();
    _j3d_vertex.begin()
        .add(bgfx::Attrib::Position, 4, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .add(bgfx::Attrib::TexCoord0, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord1, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord2, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord3, 3, bgfx::AttribType::Float)
        .end();

    const auto shader_directory = resolve_shader_directory();
    const auto vertex_shader_bytes = read_file_bytes(shader_directory / "vs_j3d.bin");
    const auto fragment_shader_bytes = read_file_bytes(shader_directory / "fs_j3d.bin");
    const auto *vertex_memory = bgfx::copy(vertex_shader_bytes.data(), static_cast<uint32_t>(vertex_shader_bytes.size()));
    const auto *fragment_memory = bgfx::copy(fragment_shader_bytes.data(), static_cast<uint32_t>(fragment_shader_bytes.size()));
    const auto vertex_shader = bgfx::createShader(vertex_memory);
    const auto fragment_shader = bgfx::createShader(fragment_memory);
    _j3d_program = bgfx::createProgram(vertex_shader, fragment_shader, true);

    _j3d_samplers[0U] = bgfx::createUniform("s_j3d_tex0", bgfx::UniformType::Sampler);
    _j3d_samplers[1U] = bgfx::createUniform("s_j3d_tex1", bgfx::UniformType::Sampler);
    _j3d_samplers[2U] = bgfx::createUniform("s_j3d_tex2", bgfx::UniformType::Sampler);
    _j3d_samplers[3U] = bgfx::createUniform("s_j3d_tex3", bgfx::UniformType::Sampler);
    _j3d_params = bgfx::createUniform("u_j3d_params", bgfx::UniformType::Vec4);
    _j3d_wrap_params = bgfx::createUniform("u_j3d_wrap_params", bgfx::UniformType::Vec4, 2U);
    _j3d_tev_color0 = bgfx::createUniform("u_j3d_tev_color0", bgfx::UniformType::Vec4);
    _j3d_tev_color1 = bgfx::createUniform("u_j3d_tev_color1", bgfx::UniformType::Vec4);
    _j3d_tev_colors = bgfx::createUniform("u_j3d_tev_colors", bgfx::UniformType::Vec4, 4U);
    _j3d_k_colors = bgfx::createUniform("u_j3d_k_colors", bgfx::UniformType::Vec4, 4U);
    _j3d_tev_stages = bgfx::createUniform("u_j3d_tev_stages", bgfx::UniformType::Vec4, 8U);
    _j3d_tev_alpha_stages = bgfx::createUniform("u_j3d_tev_alpha_stages", bgfx::UniformType::Vec4, 8U);
    _j3d_tev_color_dests = bgfx::createUniform("u_j3d_tev_color_dests", bgfx::UniformType::Vec4);
    _j3d_tev_alpha_dests = bgfx::createUniform("u_j3d_tev_alpha_dests", bgfx::UniformType::Vec4);
    _j3d_tev_texture_indices = bgfx::createUniform("u_j3d_tev_texture_indices", bgfx::UniformType::Vec4);
    _j3d_tev_k_color_selectors = bgfx::createUniform("u_j3d_tev_k_color_selectors", bgfx::UniformType::Vec4);
    _j3d_tev_k_alpha_selectors = bgfx::createUniform("u_j3d_tev_k_alpha_selectors", bgfx::UniformType::Vec4);
    _j3d_tev_texture_swizzles = bgfx::createUniform("u_j3d_tev_texture_swizzles", bgfx::UniformType::Vec4, 4U);
    _j3d_tev_raster_swizzles = bgfx::createUniform("u_j3d_tev_raster_swizzles", bgfx::UniformType::Vec4, 4U);
    _j3d_alpha_compare = bgfx::createUniform("u_j3d_alpha_compare", bgfx::UniformType::Vec4);
    _j3d_alpha_compare_extra = bgfx::createUniform("u_j3d_alpha_compare_extra", bgfx::UniformType::Vec4);
    _j3d_texture_sizes = bgfx::createUniform("u_j3d_texture_sizes", bgfx::UniformType::Vec4, 4U);
    _j3d_ind_params = bgfx::createUniform("u_j3d_ind_params", bgfx::UniformType::Vec4);
    _j3d_ind_orders = bgfx::createUniform("u_j3d_ind_orders", bgfx::UniformType::Vec4, 4U);
    _j3d_ind_matrices = bgfx::createUniform("u_j3d_ind_matrices", bgfx::UniformType::Vec4, 6U);
    _j3d_tev_indirects = bgfx::createUniform("u_j3d_tev_indirects", bgfx::UniformType::Vec4, 8U);

    if (not bgfx::isValid(_white_texture)) {
        const std::uint32_t white_pixel = 0xFFFFFFFFU;
        const auto *white_memory = bgfx::copy(&white_pixel, sizeof(white_pixel));
        _white_texture = bgfx::createTexture2D(1U, 1U, false, 1U, bgfx::TextureFormat::BGRA8, 0U, white_memory);
    }

    _j3d_resources_ready = true;
}

bgfx::TextureHandle BgfxBackend::resolve_texture(const smgpc::render::core::RenderTextureRef &texture) {
    if (texture.id == 0U || texture.rgba8 == nullptr || texture.width == 0U || texture.height == 0U) {
        return _white_texture;
    }

    const auto cache_key = texture_cache_key(texture);
    const auto found = _textures.find(cache_key);
    if (found != _textures.end()) {
        return found->second;
    }

    const bool use_generated_mips = std::getenv("SMGPC_DISABLE_GENERATED_MIPS") == nullptr && (texture.width > 1U || texture.height > 1U);
    const auto upload_bytes = use_generated_mips ? build_rgba8_mip_chain(texture) :
                                                   std::vector<std::uint8_t>(
                                                       texture.rgba8,
                                                       texture.rgba8 + static_cast<std::size_t>(texture.width) * texture.height * 4U);
    const auto *memory = bgfx::copy(upload_bytes.data(), static_cast<std::uint32_t>(upload_bytes.size()));
    const auto sampler_flags = sampler_flags_for_wrap(texture.wrap_s, texture.wrap_t);
    const auto handle = bgfx::createTexture2D(
            texture.width,
            texture.height,
            use_generated_mips,
            1U,
            bgfx::TextureFormat::RGBA8,
            sampler_flags,
            memory);

    if (bgfx::isValid(handle)) {
        _textures.emplace(cache_key, handle);
        return handle;
    }

    return _white_texture;
}

void BgfxBackend::execute_clear(const smgpc::render::core::RenderClearCommand &command) {
    std::uint32_t flags = 0U;
    if (command.clear_color) {
        flags |= BGFX_CLEAR_COLOR;
    }
    if (command.clear_depth) {
        flags |= BGFX_CLEAR_DEPTH;
    }
    if (command.clear_stencil) {
        flags |= BGFX_CLEAR_STENCIL;
    }

    bgfx::setViewClear(
        command.view_id,
        flags,
        command.color_value,
        command.depth_value,
        command.stencil_value);
    bgfx::touch(command.view_id);
}

void BgfxBackend::execute_draw_layout(const smgpc::render::core::RenderDrawLayoutCommand &command) {
    ensure_layout_resources();

    const auto view_id = command.view_id;
    const auto framebuffer_width = command.framebuffer_width;
    const auto framebuffer_height = command.framebuffer_height;

    float layout_width = command.layout_width;
    float layout_height = command.layout_height;
    if (layout_width <= 0.0F) {
        layout_width = static_cast<float>(framebuffer_width);
    }
    if (layout_height <= 0.0F) {
        layout_height = static_cast<float>(framebuffer_height);
    }

    const float scale_x = static_cast<float>(framebuffer_width) / layout_width;
    const float scale_y = static_cast<float>(framebuffer_height) / layout_height;

    bgfx::setViewRect(view_id, 0U, 0U, framebuffer_width, framebuffer_height);
    bgfx::setViewMode(view_id, bgfx::ViewMode::Sequential);

    float ortho[16] {};
    bx::mtxOrtho(
        ortho,
        0.0F,
        static_cast<float>(framebuffer_width),
        static_cast<float>(framebuffer_height),
        0.0F,
        -1.0F,
        1.0F,
        0.0F,
        bgfx::getCaps()->homogeneousDepth);
    bgfx::setViewTransform(view_id, nullptr, ortho);

    constexpr std::uint64_t DRAW_STATE_ALPHA =
        BGFX_STATE_WRITE_RGB |
        BGFX_STATE_WRITE_A |
        BGFX_STATE_DEPTH_TEST_ALWAYS |
        BGFX_STATE_BLEND_ALPHA;
    constexpr std::uint64_t DRAW_STATE_ADDITIVE =
        BGFX_STATE_WRITE_RGB |
        BGFX_STATE_WRITE_A |
        BGFX_STATE_DEPTH_TEST_ALWAYS |
        BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_ONE);

    struct Vertex {
        float x;
        float y;
        float z;
        float w;
        std::uint32_t abgr;
        float u;
        float v;
        float q;
        float u_mask;
        float v_mask;
        float q_mask;
    };

    const auto draw_quad = [&](const smgpc::render::core::RenderLayoutQuad &quad) {
        const float quad_scale_x = quad.coordinate_width > 0.0F ? static_cast<float>(framebuffer_width) / quad.coordinate_width : scale_x;
        const float quad_scale_y = quad.coordinate_height > 0.0F ? static_cast<float>(framebuffer_height) / quad.coordinate_height : scale_y;
        constexpr std::uint32_t VERTEX_COUNT = 4U;
        constexpr std::uint32_t INDEX_COUNT = 6U;

        if (bgfx::getAvailTransientVertexBuffer(VERTEX_COUNT, _layout_vertex) < VERTEX_COUNT) {
            return;
        }
        if (bgfx::getAvailTransientIndexBuffer(INDEX_COUNT) < INDEX_COUNT) {
            return;
        }

        bgfx::TransientVertexBuffer vertex_buffer {};
        bgfx::TransientIndexBuffer index_buffer {};
        bgfx::allocTransientVertexBuffer(&vertex_buffer, VERTEX_COUNT, _layout_vertex);
        bgfx::allocTransientIndexBuffer(&index_buffer, INDEX_COUNT);

        auto *vertices = reinterpret_cast<Vertex *>(vertex_buffer.data);
        const float x_tl = quad.use_custom_vertices ? quad.x_tl : quad.x0;
        const float y_tl = quad.use_custom_vertices ? quad.y_tl : quad.y0;
        const float x_tr = quad.use_custom_vertices ? quad.x_tr : quad.x1;
        const float y_tr = quad.use_custom_vertices ? quad.y_tr : quad.y0;
        const float x_bl = quad.use_custom_vertices ? quad.x_bl : quad.x0;
        const float y_bl = quad.use_custom_vertices ? quad.y_bl : quad.y1;
        const float x_br = quad.use_custom_vertices ? quad.x_br : quad.x1;
        const float y_br = quad.use_custom_vertices ? quad.y_br : quad.y1;
        const float u_tl = quad.use_custom_tex_coords ? quad.u_tl : quad.u0;
        const float v_tl = quad.use_custom_tex_coords ? quad.v_tl : quad.v0;
        const float q_tl = quad.use_custom_tex_coords ? quad.q_tl : quad.q0;
        const float u_tr = quad.use_custom_tex_coords ? quad.u_tr : quad.u1;
        const float v_tr = quad.use_custom_tex_coords ? quad.v_tr : quad.v0;
        const float q_tr = quad.use_custom_tex_coords ? quad.q_tr : quad.q1;
        const float u_bl = quad.use_custom_tex_coords ? quad.u_bl : quad.u0;
        const float v_bl = quad.use_custom_tex_coords ? quad.v_bl : quad.v1;
        const float q_bl = quad.use_custom_tex_coords ? quad.q_bl : quad.q0;
        const float u_br = quad.use_custom_tex_coords ? quad.u_br : quad.u1;
        const float v_br = quad.use_custom_tex_coords ? quad.v_br : quad.v1;
        const float q_br = quad.use_custom_tex_coords ? quad.q_br : quad.q1;
        const float u_tl_secondary = quad.use_custom_tex_coords ? quad.u_tl_secondary : quad.u0_secondary;
        const float v_tl_secondary = quad.use_custom_tex_coords ? quad.v_tl_secondary : quad.v0_secondary;
        const float q_tl_secondary = quad.use_custom_tex_coords ? quad.q_tl_secondary : quad.q0_secondary;
        const float u_tr_secondary = quad.use_custom_tex_coords ? quad.u_tr_secondary : quad.u1_secondary;
        const float v_tr_secondary = quad.use_custom_tex_coords ? quad.v_tr_secondary : quad.v0_secondary;
        const float q_tr_secondary = quad.use_custom_tex_coords ? quad.q_tr_secondary : quad.q1_secondary;
        const float u_bl_secondary = quad.use_custom_tex_coords ? quad.u_bl_secondary : quad.u0_secondary;
        const float v_bl_secondary = quad.use_custom_tex_coords ? quad.v_bl_secondary : quad.v1_secondary;
        const float q_bl_secondary = quad.use_custom_tex_coords ? quad.q_bl_secondary : quad.q0_secondary;
        const float u_br_secondary = quad.use_custom_tex_coords ? quad.u_br_secondary : quad.u1_secondary;
        const float v_br_secondary = quad.use_custom_tex_coords ? quad.v_br_secondary : quad.v1_secondary;
        const float q_br_secondary = quad.use_custom_tex_coords ? quad.q_br_secondary : quad.q1_secondary;
        vertices[0] = Vertex {
            .x = x_tl * quad_scale_x,
            .y = y_tl * quad_scale_y,
            .z = 0.0F,
            .w = 1.0F,
            .abgr = quad.color_tl,
            .u = u_tl,
            .v = v_tl,
            .q = q_tl,
            .u_mask = u_tl_secondary,
            .v_mask = v_tl_secondary,
            .q_mask = q_tl_secondary,
        };
        vertices[1] = Vertex {
            .x = x_tr * quad_scale_x,
            .y = y_tr * quad_scale_y,
            .z = 0.0F,
            .w = 1.0F,
            .abgr = quad.color_tr,
            .u = u_tr,
            .v = v_tr,
            .q = q_tr,
            .u_mask = u_tr_secondary,
            .v_mask = v_tr_secondary,
            .q_mask = q_tr_secondary,
        };
        vertices[2] = Vertex {
            .x = x_bl * quad_scale_x,
            .y = y_bl * quad_scale_y,
            .z = 0.0F,
            .w = 1.0F,
            .abgr = quad.color_bl,
            .u = u_bl,
            .v = v_bl,
            .q = q_bl,
            .u_mask = u_bl_secondary,
            .v_mask = v_bl_secondary,
            .q_mask = q_bl_secondary,
        };
        vertices[3] = Vertex {
            .x = x_br * quad_scale_x,
            .y = y_br * quad_scale_y,
            .z = 0.0F,
            .w = 1.0F,
            .abgr = quad.color_br,
            .u = u_br,
            .v = v_br,
            .q = q_br,
            .u_mask = u_br_secondary,
            .v_mask = v_br_secondary,
            .q_mask = q_br_secondary,
        };

        auto *indices = reinterpret_cast<std::uint16_t *>(index_buffer.data);
        indices[0] = 0U;
        indices[1] = 1U;
        indices[2] = 2U;
        indices[3] = 1U;
        indices[4] = 3U;
        indices[5] = 2U;

        const auto texture_handle = resolve_texture(quad.texture);
        const auto mask_texture_handle = quad.use_mask_texture ? resolve_texture(quad.mask_texture) : _white_texture;

        bgfx::setTexture(0U, _sampler, texture_handle, sampler_flags_for_wrap(quad.texture.wrap_s, quad.texture.wrap_t));
        bgfx::setTexture(
            1U,
            _mask_sampler,
            mask_texture_handle,
            quad.use_mask_texture ? sampler_flags_for_wrap(quad.mask_texture.wrap_s, quad.mask_texture.wrap_t) : BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
        const float mask_params[] {
            quad.use_mask_texture ? 1.0F : 0.0F,
            quad.invert_mask ? 1.0F : 0.0F,
            quad.mask_uses_alpha ? 1.0F : 0.0F,
            quad.texture_color_lerp ? 2.0F : (quad.texture_alpha_only ? (quad.tev_color_scale > 1.5F ? 1.25F : 1.0F) : 0.0F),
        };
        bgfx::setUniform(_mask_params, mask_params);
        const float wrap_params[] {
            static_cast<float>(quad.texture.wrap_s),
            static_cast<float>(quad.texture.wrap_t),
            quad.use_mask_texture ? static_cast<float>(quad.mask_texture.wrap_s) : 0.0F,
            quad.use_mask_texture ? static_cast<float>(quad.mask_texture.wrap_t) : 0.0F,
        };
        bgfx::setUniform(_wrap_params, wrap_params);
        float tev_color0[4] {};
        float tev_color1[4] {};
        packed_color_to_uniform(quad.tev_color0, tev_color0);
        packed_color_to_uniform(quad.tev_color1, tev_color1);
        bgfx::setUniform(_tev_color0, tev_color0);
        bgfx::setUniform(_tev_color1, tev_color1);
        bgfx::setScissor(0U, 0U, framebuffer_width, framebuffer_height);
        bgfx::setState(
            quad.blend_mode == smgpc::render::core::RenderBlendMode::Additive
                ? DRAW_STATE_ADDITIVE
                : DRAW_STATE_ALPHA);
        bgfx::setVertexBuffer(0U, &vertex_buffer);
        bgfx::setIndexBuffer(&index_buffer);
        bgfx::setTransform(nullptr);
        bgfx::submit(view_id, _layout_program);
    };

    const auto draw_triangle_batch = [&](const smgpc::render::core::RenderLayoutTriangleBatch &batch) {
        if (batch.vertices.empty()) {
            return;
        }

        const float batch_scale_x = batch.coordinate_width > 0.0F ? static_cast<float>(framebuffer_width) / batch.coordinate_width : scale_x;
        const float batch_scale_y = batch.coordinate_height > 0.0F ? static_cast<float>(framebuffer_height) / batch.coordinate_height : scale_y;

        std::size_t cursor = 0U;
        while (cursor < batch.vertices.size()) {
            const auto remaining = batch.vertices.size() - cursor;
            const auto available = static_cast<std::size_t>(bgfx::getAvailTransientVertexBuffer(
                static_cast<std::uint32_t>(std::min<std::size_t>(remaining, 65535U)), _layout_vertex));
            const auto vertex_count = std::min(remaining, available - (available % 3U));
            if (vertex_count < 3U) {
                return;
            }

            bgfx::TransientVertexBuffer vertex_buffer {};
            bgfx::allocTransientVertexBuffer(&vertex_buffer, static_cast<std::uint32_t>(vertex_count), _layout_vertex);
            auto *vertices = reinterpret_cast<Vertex *>(vertex_buffer.data);
            for (std::size_t index = 0U; index < vertex_count; ++index) {
                const auto &source = batch.vertices[cursor + index];
                vertices[index] = Vertex {
                    .x = source.x * batch_scale_x,
                    .y = source.y * batch_scale_y,
                    .z = source.z,
                    .w = 1.0F,
                    .abgr = source.color,
                    .u = source.u,
                    .v = source.v,
                    .q = source.q,
                    .u_mask = source.u_secondary,
                    .v_mask = source.v_secondary,
                    .q_mask = source.q_secondary,
                };
            }

            const auto texture_handle = resolve_texture(batch.texture);
            const bool use_secondary_texture = texture_ref_is_valid(batch.secondary_texture) &&
                                               batch.secondary_texture_mode !=
                                                   smgpc::render::core::RenderTriangleTextureCombineMode::None;
            const auto secondary_texture_handle = use_secondary_texture ? resolve_texture(batch.secondary_texture) : _white_texture;
            bgfx::setTexture(0U, _sampler, texture_handle, sampler_flags_for_wrap(batch.texture.wrap_s, batch.texture.wrap_t));
            bgfx::setTexture(
                1U,
                _mask_sampler,
                secondary_texture_handle,
                use_secondary_texture ? sampler_flags_for_wrap(batch.secondary_texture.wrap_s, batch.secondary_texture.wrap_t)
                                      : BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
            const float mask_params[] {
                use_secondary_texture && batch.secondary_texture_mode == smgpc::render::core::RenderTriangleTextureCombineMode::J3dTevColorStages
                    ? static_cast<float>(batch.tev_stage_count)
                    : 0.0F,
                0.0F,
                0.0F,
                use_secondary_texture ? triangle_texture_combine_uniform(batch.secondary_texture_mode) : 0.0F,
            };
            bgfx::setUniform(_mask_params, mask_params);
            const float triangle_tev_stages[] {
                static_cast<float>(batch.tev_stages[0U].color_args.a),
                static_cast<float>(batch.tev_stages[0U].color_args.b),
                static_cast<float>(batch.tev_stages[0U].color_args.c),
                static_cast<float>(batch.tev_stages[0U].color_args.d),
                static_cast<float>(batch.tev_stages[0U].color_op.op),
                static_cast<float>(batch.tev_stages[0U].color_op.bias),
                static_cast<float>(batch.tev_stages[0U].color_op.scale),
                static_cast<float>(batch.tev_stages[0U].color_op.clamp),
                static_cast<float>(batch.tev_stages[1U].color_args.a),
                static_cast<float>(batch.tev_stages[1U].color_args.b),
                static_cast<float>(batch.tev_stages[1U].color_args.c),
                static_cast<float>(batch.tev_stages[1U].color_args.d),
                static_cast<float>(batch.tev_stages[1U].color_op.op),
                static_cast<float>(batch.tev_stages[1U].color_op.bias),
                static_cast<float>(batch.tev_stages[1U].color_op.scale),
                static_cast<float>(batch.tev_stages[1U].color_op.clamp),
            };
            bgfx::setUniform(_triangle_tev_stages, triangle_tev_stages, 4U);
            const float wrap_params[] {
                static_cast<float>(batch.texture.wrap_s),
                static_cast<float>(batch.texture.wrap_t),
                use_secondary_texture ? static_cast<float>(batch.secondary_texture.wrap_s) : 0.0F,
                use_secondary_texture ? static_cast<float>(batch.secondary_texture.wrap_t) : 0.0F,
            };
            bgfx::setUniform(_wrap_params, wrap_params);
            float tev_color0[4] {};
            float tev_color1[4] {};
            packed_color_to_uniform(batch.tev_color0, tev_color0);
            packed_color_to_uniform(batch.tev_color1, tev_color1);
            bgfx::setUniform(_tev_color0, tev_color0);
            bgfx::setUniform(_tev_color1, tev_color1);
            bgfx::setScissor(0U, 0U, framebuffer_width, framebuffer_height);
            bgfx::setState(
                batch.blend_mode == smgpc::render::core::RenderBlendMode::Additive
                    ? DRAW_STATE_ADDITIVE
                    : DRAW_STATE_ALPHA);
            bgfx::setVertexBuffer(0U, &vertex_buffer, 0U, static_cast<std::uint32_t>(vertex_count));
            bgfx::setTransform(nullptr);
            bgfx::submit(view_id, _layout_program);

            cursor += vertex_count;
        }
    };

    if (command.debug_solid_quad) {
        bgfx::touch(view_id);
        draw_quad({
            .x0 = 0.0F,
            .y0 = 0.0F,
            .x1 = static_cast<float>(framebuffer_width),
            .y1 = static_cast<float>(framebuffer_height),
            .u0 = 0.0F,
            .v0 = 0.0F,
            .u1 = 1.0F,
            .v1 = 1.0F,
            .u0_secondary = 0.0F,
            .v0_secondary = 0.0F,
            .u1_secondary = 1.0F,
            .v1_secondary = 1.0F,
            .color_tl = 0xFFFFFFFFU,
            .color_tr = 0xFFFFFFFFU,
            .color_bl = 0xFFFFFFFFU,
            .color_br = 0xFFFFFFFFU,
            .blend_mode = smgpc::render::core::RenderBlendMode::Alpha,
            .texture = {
                .id = 0U,
                .rgba8 = nullptr,
                .width = 0U,
                .height = 0U,
            },
        });
        return;
    }

    if (command.debug_force_touch || command.debug_touch_only) {
        bgfx::touch(view_id);
        return;
    }

    if (command.quads.empty() && command.triangle_batches.empty()) {
        bgfx::touch(view_id);
        return;
    }

    if (!command.draw_order.empty()) {
        for (const auto &item : command.draw_order) {
            if (item.kind == smgpc::render::core::RenderLayoutDrawItemKind::TriangleBatch) {
                if (item.index < command.triangle_batches.size()) {
                    draw_triangle_batch(command.triangle_batches[item.index]);
                }
            } else if (item.index < command.quads.size()) {
                draw_quad(command.quads[item.index]);
            }
        }
        return;
    }

    for (const auto &batch : command.triangle_batches) {
        draw_triangle_batch(batch);
    }

    for (const auto &quad : command.quads) {
        draw_quad(quad);
    }
}

void BgfxBackend::execute_draw_j3d(const smgpc::render::core::RenderDrawJ3dCommand &command) {
    ensure_j3d_resources();

    const auto view_id = command.view_id;
    const auto framebuffer_width = command.framebuffer_width;
    const auto framebuffer_height = command.framebuffer_height;

    bgfx::setViewRect(view_id, 0U, 0U, framebuffer_width, framebuffer_height);
    bgfx::setViewMode(view_id, bgfx::ViewMode::Sequential);
    std::array<float, 16U> view_matrix = command.view_matrix;
    std::array<float, 16U> projection_matrix = command.projection_matrix;
    if (command.use_camera) {
        const bx::Vec3 eye {command.camera_eye[0U], command.camera_eye[1U], command.camera_eye[2U]};
        const bx::Vec3 target {command.camera_target[0U], command.camera_target[1U], command.camera_target[2U]};
        const bx::Vec3 up {command.camera_up[0U], command.camera_up[1U], command.camera_up[2U]};
        const float aspect = framebuffer_height == 0U ? 1.0F : static_cast<float>(framebuffer_width) / static_cast<float>(framebuffer_height);
        bx::mtxLookAt(view_matrix.data(), eye, target, up, bx::Handedness::Left);
        bx::mtxProj(projection_matrix.data(), command.camera_fovy_degrees, aspect, command.camera_near, command.camera_far, bgfx::getCaps()->homogeneousDepth, bx::Handedness::Left);
    }
    bgfx::setViewTransform(view_id, view_matrix.data(), projection_matrix.data());

    if (command.batches.empty()) {
        bgfx::touch(view_id);
        return;
    }

    struct Vertex {
        float x;
        float y;
        float z;
        float w;
        std::uint32_t abgr;
        float u;
        float v;
        float q;
        float u_secondary;
        float v_secondary;
        float q_secondary;
        float u2;
        float v2;
        float q2;
        float u3;
        float v3;
        float q3;
    };

    if (std::getenv("SMGPC_J3D_DEBUG_CAMERA_TRIANGLE") != nullptr) {
        if (bgfx::getAvailTransientVertexBuffer(3U, _j3d_vertex) >= 3U && bgfx::getAvailTransientIndexBuffer(3U) >= 3U) {
            bgfx::TransientVertexBuffer vertex_buffer {};
            bgfx::TransientIndexBuffer index_buffer {};
            bgfx::allocTransientVertexBuffer(&vertex_buffer, 3U, _j3d_vertex);
            bgfx::allocTransientIndexBuffer(&index_buffer, 3U);
            auto *vertices = reinterpret_cast<Vertex *>(vertex_buffer.data);
            vertices[0] = Vertex{.x = -12000.0F, .y = command.camera_target[1U] - 10000.0F, .z = command.camera_target[2U], .w = 1.0F, .abgr = 0xFFFF0000U};
            vertices[1] = Vertex{.x = 12000.0F, .y = command.camera_target[1U] - 10000.0F, .z = command.camera_target[2U], .w = 1.0F, .abgr = 0xFFFF0000U};
            vertices[2] = Vertex{.x = 0.0F, .y = command.camera_target[1U] + 12000.0F, .z = command.camera_target[2U], .w = 1.0F, .abgr = 0xFFFF0000U};
            auto *indices = reinterpret_cast<std::uint16_t *>(index_buffer.data);
            indices[0] = 0U;
            indices[1] = 1U;
            indices[2] = 2U;
            const float debug_params[] {0.0F, 0.0F, 1.0F, 0.0F};
            bgfx::setUniform(_j3d_params, debug_params);
            for (std::uint8_t texture_index = 0U; texture_index < _j3d_samplers.size(); ++texture_index) {
                bgfx::setTexture(texture_index, _j3d_samplers[texture_index], _white_texture, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
            }
            bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_ALWAYS);
            bgfx::setVertexBuffer(0U, &vertex_buffer, 0U, 3U);
            bgfx::setIndexBuffer(&index_buffer);
            bgfx::setTransform(nullptr);
            bgfx::submit(view_id, _j3d_program);
            if (std::getenv("SMGPC_J3D_DEBUG_CAMERA_TRIANGLE_ONLY") != nullptr) {
                return;
            }
        }
    }

    for (const auto &batch : command.batches) {
        if (batch.vertices.empty()) {
            continue;
        }

        std::array<smgpc::render::core::RenderTextureRef, 4U> textures = batch.textures;
        if (batch.texture_count == 0U) {
            textures[0U] = batch.texture;
            textures[1U] = batch.secondary_texture;
        }
        const std::uint8_t texture_count = batch.texture_count == 0U ? static_cast<std::uint8_t>(2U) : std::min<std::uint8_t>(batch.texture_count, static_cast<std::uint8_t>(textures.size()));
        const bool use_j3d_tev = texture_count > 0U && batch.secondary_texture_mode == smgpc::render::core::RenderTriangleTextureCombineMode::J3dTevColorStages && batch.tev_stage_count > 0U;
        const bool use_secondary_texture = texture_count > 1U && texture_ref_is_valid(textures[1U]) && batch.secondary_texture_mode != smgpc::render::core::RenderTriangleTextureCombineMode::None;

        std::size_t cursor = 0U;
        while (cursor < batch.vertices.size()) {
            const auto remaining = batch.vertices.size() - cursor;
            const auto request_count = static_cast<std::uint32_t>(std::min<std::size_t>(remaining, 65535U));
            const auto available = static_cast<std::size_t>(bgfx::getAvailTransientVertexBuffer(request_count, _j3d_vertex));
            if (available < 3U) {
                return;
            }
            const auto vertex_count = std::min(remaining, available - (available % 3U));
            if (vertex_count < 3U) {
                return;
            }

            bgfx::TransientVertexBuffer vertex_buffer {};
            bgfx::allocTransientVertexBuffer(&vertex_buffer, static_cast<std::uint32_t>(vertex_count), _j3d_vertex);
            auto *vertices = reinterpret_cast<Vertex *>(vertex_buffer.data);
            for (std::size_t index = 0U; index < vertex_count; ++index) {
                const auto &source = batch.vertices[cursor + index];
                vertices[index] = Vertex {
                    .x = source.x,
                    .y = source.y,
                    .z = source.z,
                    .w = 1.0F,
                    .abgr = source.color,
                    .u = source.u,
                    .v = source.v,
                    .q = source.q,
                    .u_secondary = source.u_secondary,
                    .v_secondary = source.v_secondary,
                    .q_secondary = source.q_secondary,
                    .u2 = source.u2,
                    .v2 = source.v2,
                    .q2 = source.q2,
                    .u3 = source.u3,
                    .v3 = source.v3,
                    .q3 = source.q3,
                };
            }

            for (std::uint8_t texture_index = 0U; texture_index < _j3d_samplers.size(); ++texture_index) {
                const auto &texture = texture_index < texture_count ? textures[texture_index] : textures[0U];
                const bool valid_texture = texture_ref_is_valid(texture);
                bgfx::setTexture(
                    texture_index,
                    _j3d_samplers[texture_index],
                    valid_texture ? resolve_texture(texture) : _white_texture,
                    valid_texture ? sampler_flags_for_wrap(texture.wrap_s, texture.wrap_t) : BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
            }
            const float params[] {
                use_j3d_tev ? static_cast<float>(batch.tev_stage_count) : 0.0F,
                optional_float_from_environment("SMGPC_J3D_DEBUG_TEXTURE_SLOT", 0.0F),
                std::getenv("SMGPC_J3D_DEBUG_VERTEX_COLOR") != nullptr ? 1.0F : 0.0F,
                use_j3d_tev || use_secondary_texture ? triangle_texture_combine_uniform(batch.secondary_texture_mode) : 0.0F,
            };
            bgfx::setUniform(_j3d_params, params);
            const float wrap_params[8U] {
                static_cast<float>(textures[0U].wrap_s),
                static_cast<float>(textures[0U].wrap_t),
                static_cast<float>(textures[1U].wrap_s),
                static_cast<float>(textures[1U].wrap_t),
                static_cast<float>(textures[2U].wrap_s),
                static_cast<float>(textures[2U].wrap_t),
                static_cast<float>(textures[3U].wrap_s),
                static_cast<float>(textures[3U].wrap_t),
            };
            bgfx::setUniform(_j3d_wrap_params, wrap_params, 2U);
            const float texture_sizes[16U] {
                static_cast<float>(textures[0U].width == 0U ? 1U : textures[0U].width),
                static_cast<float>(textures[0U].height == 0U ? 1U : textures[0U].height),
                0.0F,
                0.0F,
                static_cast<float>(textures[1U].width == 0U ? 1U : textures[1U].width),
                static_cast<float>(textures[1U].height == 0U ? 1U : textures[1U].height),
                0.0F,
                0.0F,
                static_cast<float>(textures[2U].width == 0U ? 1U : textures[2U].width),
                static_cast<float>(textures[2U].height == 0U ? 1U : textures[2U].height),
                0.0F,
                0.0F,
                static_cast<float>(textures[3U].width == 0U ? 1U : textures[3U].width),
                static_cast<float>(textures[3U].height == 0U ? 1U : textures[3U].height),
                0.0F,
                0.0F,
            };
            bgfx::setUniform(_j3d_texture_sizes, texture_sizes, 4U);
            float tev_color0[4] {};
            float tev_color1[4] {};
            packed_color_to_uniform(batch.tev_color0, tev_color0);
            packed_color_to_uniform(batch.tev_color1, tev_color1);
            bgfx::setUniform(_j3d_tev_color0, tev_color0);
            bgfx::setUniform(_j3d_tev_color1, tev_color1);
            std::array<float, 16U> tev_colors {};
            std::array<float, 16U> tev_k_colors {};
            for (std::size_t color_index = 0U; color_index < 4U; ++color_index) {
                packed_color_to_uniform(batch.tev_colors[color_index], &tev_colors[color_index * 4U]);
                packed_color_to_uniform(batch.tev_k_colors[color_index], &tev_k_colors[color_index * 4U]);
            }
            bgfx::setUniform(_j3d_tev_colors, tev_colors.data(), 4U);
            bgfx::setUniform(_j3d_k_colors, tev_k_colors.data(), 4U);

            std::array<float, 32U> tev_stages {};
            std::array<float, 32U> tev_alpha_stages {};
            for (std::size_t stage_index = 0U; stage_index < batch.tev_stages.size(); ++stage_index) {
                const auto &stage = batch.tev_stages[stage_index];
                const auto base = stage_index * 8U;
                tev_stages[base + 0U] = static_cast<float>(stage.color_args.a);
                tev_stages[base + 1U] = static_cast<float>(stage.color_args.b);
                tev_stages[base + 2U] = static_cast<float>(stage.color_args.c);
                tev_stages[base + 3U] = static_cast<float>(stage.color_args.d);
                tev_stages[base + 4U] = static_cast<float>(stage.color_op.op);
                tev_stages[base + 5U] = static_cast<float>(stage.color_op.bias);
                tev_stages[base + 6U] = static_cast<float>(stage.color_op.scale);
                tev_stages[base + 7U] = static_cast<float>(stage.color_op.clamp);
                tev_alpha_stages[base + 0U] = static_cast<float>(stage.alpha_args.a);
                tev_alpha_stages[base + 1U] = static_cast<float>(stage.alpha_args.b);
                tev_alpha_stages[base + 2U] = static_cast<float>(stage.alpha_args.c);
                tev_alpha_stages[base + 3U] = static_cast<float>(stage.alpha_args.d);
                tev_alpha_stages[base + 4U] = static_cast<float>(stage.alpha_op.op);
                tev_alpha_stages[base + 5U] = static_cast<float>(stage.alpha_op.bias);
                tev_alpha_stages[base + 6U] = static_cast<float>(stage.alpha_op.scale);
                tev_alpha_stages[base + 7U] = static_cast<float>(stage.alpha_op.clamp);
            }
            bgfx::setUniform(_j3d_tev_stages, tev_stages.data(), 8U);
            bgfx::setUniform(_j3d_tev_alpha_stages, tev_alpha_stages.data(), 8U);
            const float tev_color_dests[] {
                static_cast<float>(batch.tev_stages[0U].color_op.output_register),
                static_cast<float>(batch.tev_stages[1U].color_op.output_register),
                static_cast<float>(batch.tev_stages[2U].color_op.output_register),
                static_cast<float>(batch.tev_stages[3U].color_op.output_register),
            };
            const float tev_alpha_dests[] {
                static_cast<float>(batch.tev_stages[0U].alpha_op.output_register),
                static_cast<float>(batch.tev_stages[1U].alpha_op.output_register),
                static_cast<float>(batch.tev_stages[2U].alpha_op.output_register),
                static_cast<float>(batch.tev_stages[3U].alpha_op.output_register),
            };
            bgfx::setUniform(_j3d_tev_color_dests, tev_color_dests);
            bgfx::setUniform(_j3d_tev_alpha_dests, tev_alpha_dests);
            const float tev_texture_indices[] {
                static_cast<float>(batch.tev_stage_texture_indices[0U]),
                static_cast<float>(batch.tev_stage_texture_indices[1U]),
                static_cast<float>(batch.tev_stage_texture_indices[2U]),
                static_cast<float>(batch.tev_stage_texture_indices[3U]),
            };
            bgfx::setUniform(_j3d_tev_texture_indices, tev_texture_indices);
            const float tev_k_color_selectors[] {
                static_cast<float>(batch.tev_stage_k_color_selectors[0U]),
                static_cast<float>(batch.tev_stage_k_color_selectors[1U]),
                static_cast<float>(batch.tev_stage_k_color_selectors[2U]),
                static_cast<float>(batch.tev_stage_k_color_selectors[3U]),
            };
            bgfx::setUniform(_j3d_tev_k_color_selectors, tev_k_color_selectors);
            const float tev_k_alpha_selectors[] {
                static_cast<float>(batch.tev_stage_k_alpha_selectors[0U]),
                static_cast<float>(batch.tev_stage_k_alpha_selectors[1U]),
                static_cast<float>(batch.tev_stage_k_alpha_selectors[2U]),
                static_cast<float>(batch.tev_stage_k_alpha_selectors[3U]),
            };
            bgfx::setUniform(_j3d_tev_k_alpha_selectors, tev_k_alpha_selectors);
            std::array<float, 16U> tev_texture_swizzles {};
            std::array<float, 16U> tev_raster_swizzles {};
            for (std::size_t stage_index = 0U; stage_index < 4U; ++stage_index) {
                for (std::size_t channel_index = 0U; channel_index < 4U; ++channel_index) {
                    tev_texture_swizzles[stage_index * 4U + channel_index] =
                        static_cast<float>(batch.tev_stage_texture_swizzles[stage_index].channels[channel_index]);
                    tev_raster_swizzles[stage_index * 4U + channel_index] =
                        static_cast<float>(batch.tev_stage_raster_swizzles[stage_index].channels[channel_index]);
                }
            }
            bgfx::setUniform(_j3d_tev_texture_swizzles, tev_texture_swizzles.data(), 4U);
            bgfx::setUniform(_j3d_tev_raster_swizzles, tev_raster_swizzles.data(), 4U);
            const float alpha_compare[] {
                static_cast<float>(batch.alpha_compare.comp0),
                static_cast<float>(batch.alpha_compare.ref0),
                static_cast<float>(batch.alpha_compare.op),
                static_cast<float>(batch.alpha_compare.comp1),
            };
            const float alpha_compare_extra[] {
                static_cast<float>(batch.alpha_compare.ref1),
                batch.alpha_compare.valid ? 1.0F : 0.0F,
                0.0F,
                0.0F,
            };
            bgfx::setUniform(_j3d_alpha_compare, alpha_compare);
            bgfx::setUniform(_j3d_alpha_compare_extra, alpha_compare_extra);
            const float ind_params[] {
                static_cast<float>(batch.indirect_texture_stage_count),
                0.0F,
                0.0F,
                0.0F,
            };
            bgfx::setUniform(_j3d_ind_params, ind_params);
            std::array<float, 16U> ind_orders {};
            for (std::size_t stage_index = 0U; stage_index < batch.indirect_texture_orders.size(); ++stage_index) {
                const auto &order = batch.indirect_texture_orders[stage_index];
                const auto base = stage_index * 4U;
                ind_orders[base + 0U] = order.valid ? static_cast<float>(order.texture_index) : 0.0F;
                ind_orders[base + 1U] = static_cast<float>(order.scale_s);
                ind_orders[base + 2U] = static_cast<float>(order.scale_t);
                ind_orders[base + 3U] = order.valid ? 1.0F : 0.0F;
            }
            bgfx::setUniform(_j3d_ind_orders, ind_orders.data(), 4U);
            std::array<float, 24U> ind_matrices {};
            for (std::size_t matrix_index = 0U; matrix_index < batch.indirect_texture_matrices.size(); ++matrix_index) {
                const auto &matrix = batch.indirect_texture_matrices[matrix_index];
                const auto base = matrix_index * 8U;
                ind_matrices[base + 0U] = matrix.values[0U];
                ind_matrices[base + 1U] = matrix.values[1U];
                ind_matrices[base + 2U] = matrix.values[2U];
                ind_matrices[base + 3U] = matrix.valid ? static_cast<float>(matrix.scale_exponent) : 0.0F;
                ind_matrices[base + 4U] = matrix.values[3U];
                ind_matrices[base + 5U] = matrix.values[4U];
                ind_matrices[base + 6U] = matrix.values[5U];
                ind_matrices[base + 7U] = matrix.valid ? static_cast<float>(matrix.scale_exponent) : 0.0F;
            }
            bgfx::setUniform(_j3d_ind_matrices, ind_matrices.data(), 6U);
            std::array<float, 32U> tev_indirects {};
            for (std::size_t stage_index = 0U; stage_index < batch.indirect_tev_stages.size(); ++stage_index) {
                const auto &stage = batch.indirect_tev_stages[stage_index];
                const auto base = stage_index * 8U;
                tev_indirects[base + 0U] = stage.valid ? static_cast<float>(stage.ind_stage) : 0.0F;
                tev_indirects[base + 1U] = static_cast<float>(stage.format);
                tev_indirects[base + 2U] = static_cast<float>(stage.bias);
                tev_indirects[base + 3U] = static_cast<float>(stage.matrix);
                tev_indirects[base + 4U] = static_cast<float>(stage.wrap_s);
                tev_indirects[base + 5U] = static_cast<float>(stage.wrap_t);
                tev_indirects[base + 6U] = stage.valid ? static_cast<float>(stage.add_prev) : 0.0F;
                tev_indirects[base + 7U] = stage.valid ? 1.0F : 0.0F;
            }
            bgfx::setUniform(_j3d_tev_indirects, tev_indirects.data(), 8U);

            bgfx::setScissor(0U, 0U, framebuffer_width, framebuffer_height);
            bgfx::setState(draw_state_for_j3d_batch(batch));
            bgfx::setVertexBuffer(0U, &vertex_buffer, 0U, static_cast<std::uint32_t>(vertex_count));
            bgfx::setTransform(nullptr);
            bgfx::submit(view_id, _j3d_program);

            cursor += vertex_count;
        }
    }
}

}  // namespace smgpc::render::backends
