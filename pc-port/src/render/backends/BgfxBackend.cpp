#include "backends/BgfxBackend.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include "capture/ScreenshotService.hpp"

#include "shaders/generated/fs_gx_material_gl.h"
#include "shaders/generated/fs_gx_material_spv.h"
#include "shaders/generated/fs_texture_quad_gl.h"
#include "shaders/generated/fs_texture_quad_spv.h"
#include "shaders/generated/vs_gx_material_gl.h"
#include "shaders/generated/vs_gx_material_spv.h"
#include "shaders/generated/vs_texture_quad_gl.h"
#include "shaders/generated/vs_texture_quad_spv.h"

namespace smgpc::render::backends {
    namespace {
        constexpr auto GEOMETRY_KIND_TEXTURED = std::uint8_t{1U};
        constexpr auto GEOMETRY_KIND_GX_MATERIAL = std::uint8_t{2U};
        constexpr auto FNV_OFFSET_BASIS = std::uint64_t{14695981039346656037ULL};
        constexpr auto FNV_PRIME = std::uint64_t{1099511628211ULL};
        constexpr auto MAX_STATIC_GEOMETRY_CACHE_ENTRIES = std::size_t{256U};
        constexpr auto MAX_RECENT_GEOMETRY_KEYS = std::size_t{1024U};
        constexpr auto MAX_STATIC_GEOMETRY_CACHE_BYTES = std::size_t{32U * 1024U * 1024U};
        constexpr auto MAX_STATIC_GEOMETRY_VERTEX_BYTES = std::size_t{512U};
        constexpr auto MAX_STATIC_GEOMETRY_INDEX_BYTES = std::size_t{512U};

        [[nodiscard]] std::optional<bgfx::RendererType::Enum> resolve_renderer_type_from_environment() {
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

        [[nodiscard]] bool environment_flag_enabled(const char *name, bool default_value) {
            const char *value = std::getenv(name);
            if (value == nullptr || value[0] == '\0') {
                return default_value;
            }

            const auto flag = std::string_view(value);
            if (flag == "0" || flag == "false" || flag == "FALSE" || flag == "off" || flag == "OFF") {
                return false;
            }
            return true;
        }

        [[nodiscard]] const bgfx::Memory *copy_shader_memory(const std::uint8_t *data, std::size_t size) {
            return bgfx::copy(data, static_cast<std::uint32_t>(size));
        }

        void hash_u8(std::uint64_t &hash, std::uint8_t value) {
            hash ^= value;
            hash *= FNV_PRIME;
        }

        void hash_u16(std::uint64_t &hash, std::uint16_t value) {
            hash_u8(hash, static_cast<std::uint8_t>(value & 0xffU));
            hash_u8(hash, static_cast<std::uint8_t>((value >> 8U) & 0xffU));
        }

        void hash_u32(std::uint64_t &hash, std::uint32_t value) {
            hash_u16(hash, static_cast<std::uint16_t>(value & 0xffffU));
            hash_u16(hash, static_cast<std::uint16_t>((value >> 16U) & 0xffffU));
        }

        void hash_float(std::uint64_t &hash, float value) {
            hash_u32(hash, std::bit_cast<std::uint32_t>(value));
        }

        [[nodiscard]] bool static_geometry_cache_candidate(std::uint8_t kind, std::uint32_t vertex_count, std::uint32_t index_count) {
            constexpr auto quad_vertex_bytes = sizeof(float) * 5U + sizeof(std::uint32_t);
            constexpr auto gx_material_vertex_bytes = sizeof(float) * 13U + sizeof(std::uint32_t);
            const auto vertex_size = kind == GEOMETRY_KIND_GX_MATERIAL ? gx_material_vertex_bytes : quad_vertex_bytes;
            return static_cast<std::size_t>(vertex_count) * vertex_size <= MAX_STATIC_GEOMETRY_VERTEX_BYTES &&
                   static_cast<std::size_t>(index_count) * sizeof(std::uint16_t) <= MAX_STATIC_GEOMETRY_INDEX_BYTES;
        }

        [[nodiscard]] bgfx::ShaderHandle create_vertex_shader_for_current_renderer() {
            switch (bgfx::getRendererType()) {
            case bgfx::RendererType::Vulkan:
                return bgfx::createShader(copy_shader_memory(vs_texture_quad_spv, sizeof(vs_texture_quad_spv)));
            case bgfx::RendererType::OpenGL:
                return bgfx::createShader(copy_shader_memory(vs_texture_quad_gl, sizeof(vs_texture_quad_gl)));
            default:
                return bgfx::createShader(copy_shader_memory(vs_texture_quad_gl, sizeof(vs_texture_quad_gl)));
            }
        }

        [[nodiscard]] bgfx::ShaderHandle create_fragment_shader_for_current_renderer() {
            switch (bgfx::getRendererType()) {
            case bgfx::RendererType::Vulkan:
                return bgfx::createShader(copy_shader_memory(fs_texture_quad_spv, sizeof(fs_texture_quad_spv)));
            case bgfx::RendererType::OpenGL:
                return bgfx::createShader(copy_shader_memory(fs_texture_quad_gl, sizeof(fs_texture_quad_gl)));
            default:
                return bgfx::createShader(copy_shader_memory(fs_texture_quad_gl, sizeof(fs_texture_quad_gl)));
            }
        }

        [[nodiscard]] bgfx::ShaderHandle create_gx_material_vertex_shader_for_current_renderer() {
            switch (bgfx::getRendererType()) {
            case bgfx::RendererType::Vulkan:
                return bgfx::createShader(copy_shader_memory(vs_gx_material_spv, sizeof(vs_gx_material_spv)));
            case bgfx::RendererType::OpenGL:
                return bgfx::createShader(copy_shader_memory(vs_gx_material_gl, sizeof(vs_gx_material_gl)));
            default:
                return bgfx::createShader(copy_shader_memory(vs_gx_material_gl, sizeof(vs_gx_material_gl)));
            }
        }

        [[nodiscard]] bgfx::ShaderHandle create_gx_material_fragment_shader_for_current_renderer() {
            switch (bgfx::getRendererType()) {
            case bgfx::RendererType::Vulkan:
                return bgfx::createShader(copy_shader_memory(fs_gx_material_spv, sizeof(fs_gx_material_spv)));
            case bgfx::RendererType::OpenGL:
                return bgfx::createShader(copy_shader_memory(fs_gx_material_gl, sizeof(fs_gx_material_gl)));
            default:
                return bgfx::createShader(copy_shader_memory(fs_gx_material_gl, sizeof(fs_gx_material_gl)));
            }
        }

        [[nodiscard]] std::uint32_t pack_abgr(const core::TexturedVertex2D &vertex) {
            return (static_cast<std::uint32_t>(vertex.color[3U]) << 24U) | (static_cast<std::uint32_t>(vertex.color[2U]) << 16U) |
                   (static_cast<std::uint32_t>(vertex.color[1U]) << 8U) | static_cast<std::uint32_t>(vertex.color[0U]);
        }

        [[nodiscard]] std::uint32_t pack_abgr(const core::GxMaterialVertex2D &vertex) {
            return (static_cast<std::uint32_t>(vertex.color[3U]) << 24U) | (static_cast<std::uint32_t>(vertex.color[2U]) << 16U) |
                   (static_cast<std::uint32_t>(vertex.color[1U]) << 8U) | static_cast<std::uint32_t>(vertex.color[0U]);
        }

        [[nodiscard]] bool is_valid(bgfx::TextureHandle handle) {
            return bgfx::isValid(handle);
        }

        [[nodiscard]] double elapsed_ms(std::chrono::steady_clock::time_point begin, std::chrono::steady_clock::time_point end) {
            return std::chrono::duration<double, std::milli>(end - begin).count();
        }

        [[nodiscard]] std::uint64_t depth_compare_state(core::DepthCompare compare) {
            switch (compare) {
            case core::DepthCompare::Never:
                return BGFX_STATE_DEPTH_TEST_NEVER;
            case core::DepthCompare::Less:
                return BGFX_STATE_DEPTH_TEST_LESS;
            case core::DepthCompare::Equal:
                return BGFX_STATE_DEPTH_TEST_EQUAL;
            case core::DepthCompare::LessEqual:
                return BGFX_STATE_DEPTH_TEST_LEQUAL;
            case core::DepthCompare::Greater:
                return BGFX_STATE_DEPTH_TEST_GREATER;
            case core::DepthCompare::NotEqual:
                return BGFX_STATE_DEPTH_TEST_NOTEQUAL;
            case core::DepthCompare::GreaterEqual:
                return BGFX_STATE_DEPTH_TEST_GEQUAL;
            case core::DepthCompare::Always:
                return BGFX_STATE_DEPTH_TEST_ALWAYS;
            }

            return BGFX_STATE_DEPTH_TEST_LEQUAL;
        }

        [[nodiscard]] std::uint64_t cull_state(core::CullMode mode) {
            switch (mode) {
            case core::CullMode::None:
                return 0U;
            case core::CullMode::Front:
                return BGFX_STATE_CULL_CW;
            case core::CullMode::Back:
                return BGFX_STATE_CULL_CCW;
            case core::CullMode::FrontAndBack:
                return 0U;
            }

            return 0U;
        }

        [[nodiscard]] std::uint64_t primitive_topology_state(core::PrimitiveTopology topology) {
            switch (topology) {
            case core::PrimitiveTopology::Triangles:
                return 0U;
            case core::PrimitiveTopology::TriangleStrip:
                return BGFX_STATE_PT_TRISTRIP;
            }

            return 0U;
        }

        [[nodiscard]] std::uint64_t gx_src_blend_factor(std::uint8_t factor) {
            switch (factor) {
            case 0U:
                return BGFX_STATE_BLEND_ZERO;
            case 1U:
                return BGFX_STATE_BLEND_ONE;
            case 2U:
                return BGFX_STATE_BLEND_DST_COLOR;
            case 3U:
                return BGFX_STATE_BLEND_INV_DST_COLOR;
            case 4U:
                return BGFX_STATE_BLEND_SRC_ALPHA;
            case 5U:
                return BGFX_STATE_BLEND_INV_SRC_ALPHA;
            case 6U:
                return BGFX_STATE_BLEND_DST_ALPHA;
            case 7U:
                return BGFX_STATE_BLEND_INV_DST_ALPHA;
            default:
                return BGFX_STATE_BLEND_ONE;
            }
        }

        [[nodiscard]] std::uint64_t gx_dst_blend_factor(std::uint8_t factor) {
            switch (factor) {
            case 0U:
                return BGFX_STATE_BLEND_ZERO;
            case 1U:
                return BGFX_STATE_BLEND_ONE;
            case 2U:
                return BGFX_STATE_BLEND_SRC_COLOR;
            case 3U:
                return BGFX_STATE_BLEND_INV_SRC_COLOR;
            case 4U:
                return BGFX_STATE_BLEND_SRC_ALPHA;
            case 5U:
                return BGFX_STATE_BLEND_INV_SRC_ALPHA;
            case 6U:
                return BGFX_STATE_BLEND_DST_ALPHA;
            case 7U:
                return BGFX_STATE_BLEND_INV_DST_ALPHA;
            default:
                return BGFX_STATE_BLEND_ZERO;
            }
        }

        [[nodiscard]] std::uint64_t gx_logic_blend_state(std::uint8_t op) {
            switch (op) {
            case 0U:
                return BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ZERO, BGFX_STATE_BLEND_ZERO);
            case 1U:
                return BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_DST_COLOR, BGFX_STATE_BLEND_ZERO);
            case 2U:
                return BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_SRC_COLOR) |
                       BGFX_STATE_BLEND_EQUATION(BGFX_STATE_BLEND_EQUATION_REVSUB);
            case 3U:
                return 0U;
            case 4U:
                return BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_DST_COLOR, BGFX_STATE_BLEND_ONE) |
                       BGFX_STATE_BLEND_EQUATION(BGFX_STATE_BLEND_EQUATION_REVSUB);
            case 5U:
                return BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ZERO, BGFX_STATE_BLEND_ONE);
            case 6U:
            case 8U:
            case 14U:
                return BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_INV_DST_COLOR, BGFX_STATE_BLEND_INV_SRC_COLOR);
            case 7U:
            case 13U:
                return BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_INV_DST_COLOR, BGFX_STATE_BLEND_ONE);
            case 9U:
            case 10U:
                return BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_INV_DST_COLOR, BGFX_STATE_BLEND_ZERO);
            case 11U:
                return BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_DST_ALPHA);
            case 12U:
                return 0U;
            case 15U:
                return BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ZERO);
            default:
                return 0U;
            }
        }

        [[nodiscard]] std::uint64_t gx_blend_state(const core::GxBlendMode2D &blend) {
            constexpr auto gx_bm_none = std::uint8_t{0U};
            constexpr auto gx_bm_blend = std::uint8_t{1U};
            constexpr auto gx_bm_logic = std::uint8_t{2U};
            constexpr auto gx_bm_subtract = std::uint8_t{3U};

            if (!blend.enabled || blend.type == gx_bm_none) {
                return 0U;
            }
            if (blend.type == gx_bm_blend) {
                return BGFX_STATE_BLEND_FUNC(gx_src_blend_factor(blend.src_factor), gx_dst_blend_factor(blend.dst_factor));
            }
            if (blend.type == gx_bm_subtract) {
                return BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE) |
                       BGFX_STATE_BLEND_EQUATION(BGFX_STATE_BLEND_EQUATION_REVSUB);
            }
            if (blend.type == gx_bm_logic) {
                return gx_logic_blend_state(blend.op);
            }

            return 0U;
        }

        [[nodiscard]] std::uint64_t gx_write_state(const core::GxBlendMode2D &blend) {
            auto state = std::uint64_t{};
            if (blend.color_update) {
                state |= BGFX_STATE_WRITE_RGB;
            }
            if (blend.alpha_update) {
                state |= BGFX_STATE_WRITE_A;
            }
            return state;
        }

        [[nodiscard]] std::uint32_t gx_sampler_wrap_u(std::uint8_t wrap) {
            constexpr auto gx_repeat = std::uint8_t{1U};
            constexpr auto gx_mirror = std::uint8_t{2U};
            if (wrap == gx_repeat) {
                return 0U;
            }
            if (wrap == gx_mirror) {
                return BGFX_SAMPLER_U_MIRROR;
            }
            return BGFX_SAMPLER_U_CLAMP;
        }

        [[nodiscard]] std::uint32_t gx_sampler_wrap_v(std::uint8_t wrap) {
            constexpr auto gx_repeat = std::uint8_t{1U};
            constexpr auto gx_mirror = std::uint8_t{2U};
            if (wrap == gx_repeat) {
                return 0U;
            }
            if (wrap == gx_mirror) {
                return BGFX_SAMPLER_V_MIRROR;
            }
            return BGFX_SAMPLER_V_CLAMP;
        }

        [[nodiscard]] std::uint32_t gx_sampler_filter(std::uint8_t min_filter, std::uint8_t mag_filter) {
            constexpr auto gx_near = std::uint8_t{0U};
            constexpr auto gx_near_mip_near = std::uint8_t{2U};
            constexpr auto gx_lin_mip_near = std::uint8_t{3U};
            constexpr auto gx_near_mip_lin = std::uint8_t{4U};

            auto flags = std::uint32_t{};
            if (min_filter == gx_near || min_filter == gx_near_mip_near || min_filter == gx_near_mip_lin) {
                flags |= BGFX_SAMPLER_MIN_POINT;
            }
            if (mag_filter == gx_near) {
                flags |= BGFX_SAMPLER_MAG_POINT;
            }
            if (min_filter == gx_near_mip_near || min_filter == gx_lin_mip_near) {
                flags |= BGFX_SAMPLER_MIP_POINT;
            }
            return flags;
        }

        [[nodiscard]] std::uint32_t gx_sampler_state(std::uint8_t wrap_u, std::uint8_t wrap_v, std::uint8_t min_filter,
                                                     std::uint8_t mag_filter) {
            return gx_sampler_wrap_u(wrap_u) | gx_sampler_wrap_v(wrap_v) | gx_sampler_filter(min_filter, mag_filter);
        }

    }  // namespace

    class BgfxCallback final : public bgfx::CallbackI {
    public:
        BgfxCallback()
            : _screenshot_service(environment_flag_enabled("SMGPC_ASYNC_SCREENSHOT_PNG", true) ? capture::create_async_png_screenshot_service() : capture::create_png_screenshot_service()) {
        }

        ~BgfxCallback() override {
            try {
                _screenshot_service->flush();
            } catch (const std::exception &e) {
                std::fprintf(stderr, "failed to flush pending bgfx screenshot PNG writes: %s\n", e.what());
            }
        }

        void fatal(const char *_filePath, std::uint16_t _line, bgfx::Fatal::Enum _code, const char *_str) override {
            std::fprintf(stderr, "bgfx fatal at %s:%u: %s\n", _filePath != nullptr ? _filePath : "<unknown>", _line,
                         _str != nullptr ? _str : "<no message>");
            if (_code != bgfx::Fatal::DebugCheck) {
                std::abort();
            }
        }

        void traceVargs(const char *_filePath, std::uint16_t _line, const char *_format, va_list _argList) override {
#ifndef NDEBUG
            std::fprintf(stderr, "bgfx trace at %s:%u: ", _filePath != nullptr ? _filePath : "<unknown>", _line);
            std::vfprintf(stderr, _format, _argList);
#else
            (void)_filePath;
            (void)_line;
            (void)_format;
            (void)_argList;
#endif
        }

        void profilerBegin(const char *, std::uint32_t, const char *, std::uint16_t) override {
        }
        void profilerBeginLiteral(const char *, std::uint32_t, const char *, std::uint16_t) override {
        }
        void profilerEnd() override {
        }

        std::uint32_t cacheReadSize(std::uint64_t) override {
            return 0U;
        }

        bool cacheRead(std::uint64_t, void *, std::uint32_t) override {
            return false;
        }

        void cacheWrite(std::uint64_t, const void *, std::uint32_t) override {
        }

        void screenShot(const char *_filePath, std::uint32_t _width, std::uint32_t _height, std::uint32_t _pitch, bgfx::TextureFormat::Enum _format,
                        const void *_data, std::uint32_t _size, bool _yflip) override {
            if (_filePath == nullptr || _data == nullptr || _size == 0U) {
                return;
            }

            if (_format != bgfx::TextureFormat::BGRA8 && _format != bgfx::TextureFormat::RGBA8) {
                std::fprintf(stderr, "bgfx screenshot had unsupported texture format %d\n", static_cast<int>(_format));
                return;
            }

            const auto pixel_format = _format == bgfx::TextureFormat::BGRA8 ? capture::PixelFormat::BGRA8 : capture::PixelFormat::RGBA8;
            try {
                _screenshot_service->write_png(std::filesystem::path(_filePath),
                                               capture::ScreenshotImageView{
                                                   .width = _width,
                                                   .height = _height,
                                                   .pitch = _pitch,
                                                   .pixels = std::span<const std::uint8_t>(static_cast<const std::uint8_t *>(_data), _size),
                                                   .format = pixel_format,
                                                   .origin_bottom_left = _yflip,
                                               });
            } catch (const std::exception &e) {
                std::fprintf(stderr, "failed to write bgfx screenshot PNG %s: %s\n", _filePath, e.what());
            }
        }

        void captureBegin(std::uint32_t, std::uint32_t, std::uint32_t, bgfx::TextureFormat::Enum, bool) override {
        }
        void captureEnd() override {
        }
        void captureFrame(const void *, std::uint32_t) override {
        }

    private:
        std::unique_ptr<capture::IScreenshotService> _screenshot_service{};
    };

    BgfxBackend::BgfxBackend() : _callback(std::make_unique<BgfxCallback>()) {
        _gx_material_samplers.fill(BGFX_INVALID_HANDLE);
        _gx_tev_color_inputs.fill(BGFX_INVALID_HANDLE);
        _gx_tev_alpha_inputs.fill(BGFX_INVALID_HANDLE);
        _gx_tev_color_operations.fill(BGFX_INVALID_HANDLE);
        _gx_tev_alpha_operations.fill(BGFX_INVALID_HANDLE);
        _gx_tev_outputs.fill(BGFX_INVALID_HANDLE);
        _gx_tev_konst_colors.fill(BGFX_INVALID_HANDLE);
        _gx_tev_initial_registers.fill(BGFX_INVALID_HANDLE);
    }

    BgfxBackend::~BgfxBackend() {
        shutdown();
    }

    void BgfxBackend::initialize(const core::RenderInitDesc &description) {
        if (_initialized) {
            return;
        }

        _vsync_enabled = description.enable_vsync;
        if (environment_flag_enabled("SMGPC_BGFX_SINGLE_THREADED", false)) {
            bgfx::renderFrame();
        }

        const auto try_initialize = [&](bgfx::RendererType::Enum renderer_type) {
            auto init = bgfx::Init();
            init.type = renderer_type;
            init.platformData.nwh = description.native_window_handle;
            init.platformData.ndt = description.native_display_handle;
            init.resolution.width = static_cast<std::uint32_t>(description.width);
            init.resolution.height = static_cast<std::uint32_t>(description.height);
            init.resolution.reset = _vsync_enabled ? BGFX_RESET_VSYNC : BGFX_RESET_NONE;
            init.callback = _callback.get();
            return bgfx::init(init);
        };

        if (const auto requested_renderer = resolve_renderer_type_from_environment(); requested_renderer.has_value()) {
            if (not try_initialize(*requested_renderer)) {
                throw std::runtime_error("Cannot init bgfx with requested renderer type");
            }
        } else {
#if defined(__linux__)
            constexpr std::array<bgfx::RendererType::Enum, 3U> renderer_fallback{
                bgfx::RendererType::Vulkan,
                bgfx::RendererType::OpenGL,
                bgfx::RendererType::Count,
            };
#else
            constexpr std::array<bgfx::RendererType::Enum, 2U> renderer_fallback{
                bgfx::RendererType::Count,
                bgfx::RendererType::OpenGL,
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
        _framebuffer_width = static_cast<std::uint16_t>(description.width);
        _framebuffer_height = static_cast<std::uint16_t>(description.height);
        _logical_framebuffer_width = std::max<std::uint16_t>(1U, description.logical_width);
        _logical_framebuffer_height = std::max<std::uint16_t>(1U, description.logical_height);
        bgfx::setViewMode(0U, bgfx::ViewMode::Sequential);
        _static_geometry_cache_enabled = environment_flag_enabled("SMGPC_RENDER_STATIC_GEOMETRY_CACHE", true);
#ifndef NDEBUG
        _backend_stats_enabled = environment_flag_enabled("SMGPC_RENDER_BACKEND_STATS", false);
        _static_geometry_cache_stats_enabled = environment_flag_enabled("SMGPC_RENDER_GEOMETRY_CACHE_STATS", false);
#else
        _backend_stats_enabled = false;
        _static_geometry_cache_stats_enabled = false;
#endif

        _textured_quad_layout.begin()
            .add(bgfx::Attrib::Position, 3U, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2U, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4U, bgfx::AttribType::Uint8, true)
            .end();

        _gx_material_layout.begin()
            .add(bgfx::Attrib::Position, 3U, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 3U, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord1, 3U, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord2, 3U, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord3, 1U, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4U, bgfx::AttribType::Uint8, true)
            .end();

        _texture_sampler = bgfx::createUniform("s_texture", bgfx::UniformType::Sampler);
        for (auto i = 0U; i < _gx_material_samplers.size(); ++i) {
            const auto name = std::string("s_gx_texture") + static_cast<char>('0' + i);
            _gx_material_samplers[i] = bgfx::createUniform(name.c_str(), bgfx::UniformType::Sampler);
        }
        _gx_material_params = bgfx::createUniform("u_gx_material_params", bgfx::UniformType::Vec4);
        for (auto i = 0U; i < core::kMaxGxMaterialTevStages2D; ++i) {
            const auto suffix = static_cast<char>('0' + i);
            _gx_tev_color_inputs[i] = bgfx::createUniform((std::string("u_gx_tev_color_in") + suffix).c_str(), bgfx::UniformType::Vec4);
            _gx_tev_alpha_inputs[i] = bgfx::createUniform((std::string("u_gx_tev_alpha_in") + suffix).c_str(), bgfx::UniformType::Vec4);
            _gx_tev_color_operations[i] = bgfx::createUniform((std::string("u_gx_tev_color_op") + suffix).c_str(), bgfx::UniformType::Vec4);
            _gx_tev_alpha_operations[i] = bgfx::createUniform((std::string("u_gx_tev_alpha_op") + suffix).c_str(), bgfx::UniformType::Vec4);
            _gx_tev_outputs[i] = bgfx::createUniform((std::string("u_gx_tev_out") + suffix).c_str(), bgfx::UniformType::Vec4);
            _gx_tev_konst_colors[i] = bgfx::createUniform((std::string("u_gx_tev_konst") + suffix).c_str(), bgfx::UniformType::Vec4);
        }
        for (auto i = 0U; i < _gx_tev_initial_registers.size(); ++i) {
            const auto suffix = static_cast<char>('0' + i);
            _gx_tev_initial_registers[i] = bgfx::createUniform((std::string("u_gx_tev_reg") + suffix).c_str(), bgfx::UniformType::Vec4);
        }
        _gx_alpha_compare_0 = bgfx::createUniform("u_gx_alpha_compare0", bgfx::UniformType::Vec4);
        _gx_alpha_compare_1 = bgfx::createUniform("u_gx_alpha_compare1", bgfx::UniformType::Vec4);
        _textured_quad_program =
            bgfx::createProgram(create_vertex_shader_for_current_renderer(), create_fragment_shader_for_current_renderer(), true);
        _gx_material_program =
            bgfx::createProgram(create_gx_material_vertex_shader_for_current_renderer(), create_gx_material_fragment_shader_for_current_renderer(), true);
    }

    void BgfxBackend::shutdown() {
        if (not _initialized) {
            return;
        }

        print_backend_stats();
        print_static_geometry_cache_stats();
        clear_static_geometry_cache();
        _recent_geometry_keys.clear();

        for (const auto texture : _textures) {
            if (is_valid(texture)) {
                bgfx::destroy(texture);
            }
        }
        _textures.clear();

        if (bgfx::isValid(_textured_quad_program)) {
            bgfx::destroy(_textured_quad_program);
            _textured_quad_program = BGFX_INVALID_HANDLE;
        }

        if (bgfx::isValid(_gx_material_program)) {
            bgfx::destroy(_gx_material_program);
            _gx_material_program = BGFX_INVALID_HANDLE;
        }

        if (bgfx::isValid(_texture_sampler)) {
            bgfx::destroy(_texture_sampler);
            _texture_sampler = BGFX_INVALID_HANDLE;
        }

        for (auto &sampler : _gx_material_samplers) {
            if (bgfx::isValid(sampler)) {
                bgfx::destroy(sampler);
                sampler = BGFX_INVALID_HANDLE;
            }
        }

        if (bgfx::isValid(_gx_material_params)) {
            bgfx::destroy(_gx_material_params);
            _gx_material_params = BGFX_INVALID_HANDLE;
        }

        const auto destroy_uniforms = [](auto &uniforms) {
            for (auto &uniform : uniforms) {
                if (bgfx::isValid(uniform)) {
                    bgfx::destroy(uniform);
                    uniform = BGFX_INVALID_HANDLE;
                }
            }
        };
        destroy_uniforms(_gx_tev_color_inputs);
        destroy_uniforms(_gx_tev_alpha_inputs);
        destroy_uniforms(_gx_tev_color_operations);
        destroy_uniforms(_gx_tev_alpha_operations);
        destroy_uniforms(_gx_tev_outputs);
        destroy_uniforms(_gx_tev_konst_colors);
        destroy_uniforms(_gx_tev_initial_registers);
        if (bgfx::isValid(_gx_alpha_compare_0)) {
            bgfx::destroy(_gx_alpha_compare_0);
            _gx_alpha_compare_0 = BGFX_INVALID_HANDLE;
        }
        if (bgfx::isValid(_gx_alpha_compare_1)) {
            bgfx::destroy(_gx_alpha_compare_1);
            _gx_alpha_compare_1 = BGFX_INVALID_HANDLE;
        }

        bgfx::shutdown();
        _initialized = false;
    }

    void BgfxBackend::begin_frame(const core::FrameContext &frame_context) {
        if (not _initialized) {
            return;
        }

        _frame_index = frame_context.frame_index;
        bgfx::setViewRect(0U, 0U, 0U, frame_context.framebuffer.width, frame_context.framebuffer.height);
        bgfx::setViewClear(0U, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0F, 0U);
        bgfx::touch(0U);
    }

    void BgfxBackend::end_frame() {
        if (!_initialized) {
            return;
        }

        if (_backend_stats_enabled) {
            const auto begin = std::chrono::steady_clock::now();
            bgfx::frame();
            const auto end = std::chrono::steady_clock::now();
            _bgfx_frame_ms += elapsed_ms(begin, end);
            ++_bgfx_frame_count;
        } else {
            bgfx::frame();
        }
    }

    void BgfxBackend::resize(std::uint16_t width, std::uint16_t height) {
        if (not _initialized || width == 0U || height == 0U) {
            return;
        }

        _framebuffer_width = width;
        _framebuffer_height = height;
        clear_static_geometry_cache();
        _recent_geometry_keys.clear();
        bgfx::reset(width, height, _vsync_enabled ? BGFX_RESET_VSYNC : BGFX_RESET_NONE);
    }

    void BgfxBackend::request_screenshot_png(const std::filesystem::path &path) {
        if (not _initialized) {
            return;
        }

        auto screenshot_path = path;
        if (screenshot_path.extension().empty()) {
            screenshot_path.replace_extension(".png");
        }

        _pending_screenshot_path = screenshot_path.string();
        bgfx::requestScreenShot(BGFX_INVALID_HANDLE, _pending_screenshot_path.c_str());
    }

    core::TextureHandle BgfxBackend::create_rgba8_texture(std::uint16_t width, std::uint16_t height, std::span<const std::uint8_t> rgba) {
        if (not _initialized || width == 0U || height == 0U) {
            return {};
        }

        const auto expected_size = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U;
        if (rgba.size() < expected_size) {
            throw std::runtime_error("RGBA8 texture upload buffer is shorter than the declared texture dimensions");
        }

        const auto *memory = bgfx::copy(rgba.data(), static_cast<std::uint32_t>(expected_size));
        const auto handle =
            bgfx::createTexture2D(width, height, false, 1U, bgfx::TextureFormat::RGBA8, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP, memory);
        if (!bgfx::isValid(handle)) {
            return {};
        }

        _textures.push_back(handle);
        return core::TextureHandle{.value = static_cast<std::uint32_t>(_textures.size() - 1U)};
    }

    void BgfxBackend::destroy_texture(core::TextureHandle texture) {
        if (!texture.is_valid() || texture.value >= _textures.size()) {
            return;
        }

        auto &handle = _textures[texture.value];
        if (is_valid(handle)) {
            bgfx::destroy(handle);
            handle = BGFX_INVALID_HANDLE;
        }
    }

    std::uint64_t BgfxBackend::hash_textured_geometry(const core::TexturedTriangleBatch2D &batch) const {
        auto hash = FNV_OFFSET_BASIS;
        hash_u8(hash, GEOMETRY_KIND_TEXTURED);
        hash_u8(hash, static_cast<std::uint8_t>(batch.primitive_topology));
        hash_u32(hash, static_cast<std::uint32_t>(batch.vertices.size()));
        hash_u32(hash, static_cast<std::uint32_t>(batch.indices.size()));
        hash_u16(hash, _framebuffer_width);
        hash_u16(hash, _framebuffer_height);
        hash_u16(hash, _logical_framebuffer_width);
        hash_u16(hash, _logical_framebuffer_height);
        for (const auto &vertex : batch.vertices) {
            hash_float(hash, vertex.x);
            hash_float(hash, vertex.y);
            hash_float(hash, vertex.z);
            hash_float(hash, vertex.u);
            hash_float(hash, vertex.v);
            for (const auto channel : vertex.color) {
                hash_u8(hash, channel);
            }
        }
        for (const auto index : batch.indices) {
            hash_u16(hash, index);
        }
        return hash;
    }

    std::uint64_t BgfxBackend::hash_gx_material_geometry(const core::GxMaterialTriangleBatch2D &batch) const {
        auto hash = FNV_OFFSET_BASIS;
        hash_u8(hash, GEOMETRY_KIND_GX_MATERIAL);
        hash_u8(hash, static_cast<std::uint8_t>(batch.primitive_topology));
        hash_u32(hash, static_cast<std::uint32_t>(batch.vertices.size()));
        hash_u32(hash, static_cast<std::uint32_t>(batch.indices.size()));
        hash_u16(hash, _framebuffer_width);
        hash_u16(hash, _framebuffer_height);
        hash_u16(hash, _logical_framebuffer_width);
        hash_u16(hash, _logical_framebuffer_height);
        for (const auto &vertex : batch.vertices) {
            hash_float(hash, vertex.x);
            hash_float(hash, vertex.y);
            hash_float(hash, vertex.z);
            hash_float(hash, vertex.clip_w);
            for (const auto &coord : vertex.tex_coords) {
                hash_float(hash, coord[0U]);
                hash_float(hash, coord[1U]);
                hash_float(hash, coord[2U]);
            }
            for (const auto channel : vertex.color) {
                hash_u8(hash, channel);
            }
        }
        for (const auto index : batch.indices) {
            hash_u16(hash, index);
        }
        return hash;
    }

    BgfxBackend::StaticGeometryEntry *BgfxBackend::find_static_geometry(std::uint8_t kind, std::uint64_t hash, std::uint32_t vertex_count,
                                                                        std::uint32_t index_count) {
        auto found = std::ranges::find_if(_static_geometry_cache, [&](const auto &entry) {
            return entry.kind == kind && entry.hash == hash && entry.vertex_count == vertex_count && entry.index_count == index_count;
        });
        if (found == _static_geometry_cache.end()) {
            ++_static_geometry_cache_misses;
            return nullptr;
        }

        found->last_used_frame = _frame_index;
        ++_static_geometry_cache_hits;
        return &*found;
    }

    bool BgfxBackend::should_promote_static_geometry(std::uint8_t kind, std::uint64_t hash, std::uint32_t vertex_count, std::uint32_t index_count) {
        if (!_static_geometry_cache_enabled) {
            return false;
        }

        auto found = std::ranges::find_if(_recent_geometry_keys, [&](const auto &key) {
            return key.kind == kind && key.hash == hash && key.vertex_count == vertex_count && key.index_count == index_count;
        });
        if (found == _recent_geometry_keys.end()) {
            _recent_geometry_keys.push_back(RecentGeometryKey{
                .kind = kind,
                .hash = hash,
                .vertex_count = vertex_count,
                .index_count = index_count,
                .last_seen_frame = _frame_index,
                .seen_count = 1U,
            });
            if (_recent_geometry_keys.size() > MAX_RECENT_GEOMETRY_KEYS) {
                const auto oldest = std::ranges::min_element(_recent_geometry_keys, {}, &RecentGeometryKey::last_seen_frame);
                if (oldest != _recent_geometry_keys.end()) {
                    _recent_geometry_keys.erase(oldest);
                }
            }
            return false;
        }

        found->last_seen_frame = _frame_index;
        ++found->seen_count;
        return found->seen_count >= 2U;
    }

    void BgfxBackend::fill_quad_vertices(const core::TexturedTriangleBatch2D &batch, std::vector<QuadVertex> &out) const {
        out.clear();
        out.reserve(batch.vertices.size());
        const auto fit_scale = std::min(static_cast<float>(_framebuffer_width) / static_cast<float>(_logical_framebuffer_width),
                                        static_cast<float>(_framebuffer_height) / static_cast<float>(_logical_framebuffer_height));
        for (const auto &source : batch.vertices) {
            out.push_back(QuadVertex{
                .x = (source.x * fit_scale) / (static_cast<float>(_framebuffer_width) * 0.5F),
                .y = (source.y * fit_scale) / (static_cast<float>(_framebuffer_height) * 0.5F),
                .z = source.z,
                .u = source.u,
                .v = source.v,
                .abgr = pack_abgr(source),
            });
        }
    }

    void BgfxBackend::fill_gx_material_vertices(const core::GxMaterialTriangleBatch2D &batch, std::vector<GxMaterialVertex> &out) const {
        out.clear();
        out.reserve(batch.vertices.size());
        const auto fit_scale = std::min(static_cast<float>(_framebuffer_width) / static_cast<float>(_logical_framebuffer_width),
                                        static_cast<float>(_framebuffer_height) / static_cast<float>(_logical_framebuffer_height));
        for (const auto &source : batch.vertices) {
            const auto clip_w = std::max(source.clip_w, 1.0F);
            const auto ndc_x = (source.x * fit_scale) / (static_cast<float>(_framebuffer_width) * 0.5F);
            const auto ndc_y = (source.y * fit_scale) / (static_cast<float>(_framebuffer_height) * 0.5F);
            out.push_back(GxMaterialVertex{
                .x = ndc_x * clip_w,
                .y = ndc_y * clip_w,
                .z = source.z * clip_w,
                .u0 = source.tex_coords[0U][0U],
                .v0 = source.tex_coords[0U][1U],
                .q0 = source.tex_coords[0U][2U],
                .u1 = source.tex_coords[1U][0U],
                .v1 = source.tex_coords[1U][1U],
                .q1 = source.tex_coords[1U][2U],
                .u2 = source.tex_coords[2U][0U],
                .v2 = source.tex_coords[2U][1U],
                .q2 = source.tex_coords[2U][2U],
                .clip_w = clip_w,
                .abgr = pack_abgr(source),
            });
        }
    }

    BgfxBackend::StaticGeometryEntry *BgfxBackend::create_static_textured_geometry(std::uint64_t hash, const core::TexturedTriangleBatch2D &batch) {
        fill_quad_vertices(batch, _quad_upload_vertices);
        if (_quad_upload_vertices.empty()) {
            return nullptr;
        }

        const auto vertex_bytes = _quad_upload_vertices.size() * sizeof(QuadVertex);
        const auto index_bytes = batch.indices.size() * sizeof(std::uint16_t);
        auto vertex_buffer = bgfx::createVertexBuffer(bgfx::copy(_quad_upload_vertices.data(), static_cast<std::uint32_t>(vertex_bytes)),
                                                      _textured_quad_layout);
        auto index_buffer = bgfx::createIndexBuffer(bgfx::copy(batch.indices.data(), static_cast<std::uint32_t>(index_bytes)));
        if (!bgfx::isValid(vertex_buffer) || !bgfx::isValid(index_buffer)) {
            if (bgfx::isValid(vertex_buffer)) {
                bgfx::destroy(vertex_buffer);
            }
            if (bgfx::isValid(index_buffer)) {
                bgfx::destroy(index_buffer);
            }
            return nullptr;
        }

        _static_geometry_cache.push_back(StaticGeometryEntry{
            .kind = GEOMETRY_KIND_TEXTURED,
            .hash = hash,
            .vertex_count = static_cast<std::uint32_t>(batch.vertices.size()),
            .index_count = static_cast<std::uint32_t>(batch.indices.size()),
            .byte_size = vertex_bytes + index_bytes,
            .last_used_frame = _frame_index,
            .vertex_buffer = vertex_buffer,
            .index_buffer = index_buffer,
        });
        _static_geometry_cache_bytes += vertex_bytes + index_bytes;
        ++_static_geometry_cache_promotions;
        evict_static_geometry_cache();
        return &_static_geometry_cache.back();
    }

    BgfxBackend::StaticGeometryEntry *BgfxBackend::create_static_gx_material_geometry(std::uint64_t hash,
                                                                                      const core::GxMaterialTriangleBatch2D &batch) {
        fill_gx_material_vertices(batch, _gx_upload_vertices);
        if (_gx_upload_vertices.empty()) {
            return nullptr;
        }

        const auto vertex_bytes = _gx_upload_vertices.size() * sizeof(GxMaterialVertex);
        const auto index_bytes = batch.indices.size() * sizeof(std::uint16_t);
        auto vertex_buffer =
            bgfx::createVertexBuffer(bgfx::copy(_gx_upload_vertices.data(), static_cast<std::uint32_t>(vertex_bytes)), _gx_material_layout);
        auto index_buffer = bgfx::createIndexBuffer(bgfx::copy(batch.indices.data(), static_cast<std::uint32_t>(index_bytes)));
        if (!bgfx::isValid(vertex_buffer) || !bgfx::isValid(index_buffer)) {
            if (bgfx::isValid(vertex_buffer)) {
                bgfx::destroy(vertex_buffer);
            }
            if (bgfx::isValid(index_buffer)) {
                bgfx::destroy(index_buffer);
            }
            return nullptr;
        }

        _static_geometry_cache.push_back(StaticGeometryEntry{
            .kind = GEOMETRY_KIND_GX_MATERIAL,
            .hash = hash,
            .vertex_count = static_cast<std::uint32_t>(batch.vertices.size()),
            .index_count = static_cast<std::uint32_t>(batch.indices.size()),
            .byte_size = vertex_bytes + index_bytes,
            .last_used_frame = _frame_index,
            .vertex_buffer = vertex_buffer,
            .index_buffer = index_buffer,
        });
        _static_geometry_cache_bytes += vertex_bytes + index_bytes;
        ++_static_geometry_cache_promotions;
        evict_static_geometry_cache();
        return &_static_geometry_cache.back();
    }

    void BgfxBackend::evict_static_geometry_cache() {
        while (_static_geometry_cache.size() > MAX_STATIC_GEOMETRY_CACHE_ENTRIES ||
               _static_geometry_cache_bytes > MAX_STATIC_GEOMETRY_CACHE_BYTES) {
            auto oldest = std::ranges::min_element(_static_geometry_cache, {}, &StaticGeometryEntry::last_used_frame);
            if (oldest == _static_geometry_cache.end()) {
                return;
            }
            if (bgfx::isValid(oldest->vertex_buffer)) {
                bgfx::destroy(oldest->vertex_buffer);
            }
            if (bgfx::isValid(oldest->index_buffer)) {
                bgfx::destroy(oldest->index_buffer);
            }
            _static_geometry_cache_bytes -= std::min(_static_geometry_cache_bytes, oldest->byte_size);
            _static_geometry_cache.erase(oldest);
            ++_static_geometry_cache_evictions;
        }
    }

    void BgfxBackend::clear_static_geometry_cache() {
        for (const auto &entry : _static_geometry_cache) {
            if (bgfx::isValid(entry.vertex_buffer)) {
                bgfx::destroy(entry.vertex_buffer);
            }
            if (bgfx::isValid(entry.index_buffer)) {
                bgfx::destroy(entry.index_buffer);
            }
        }
        _static_geometry_cache.clear();
        _static_geometry_cache_bytes = 0U;
    }

    void BgfxBackend::print_backend_stats() const {
#ifndef NDEBUG
        if (!_backend_stats_enabled) {
            return;
        }

        const auto average_frame_ms = _bgfx_frame_count == 0U ? 0.0 : _bgfx_frame_ms / static_cast<double>(_bgfx_frame_count);
        std::fprintf(stderr,
                     "SMGPC render backend: frames=%llu bgfx_frame_avg=%.3fms textured=%llu gx=%llu transient_vb=%llu transient_ib=%llu "
                     "texture_binds=%llu uniform_updates=%llu dropped_transient=%llu\n",
                     static_cast<unsigned long long>(_bgfx_frame_count), average_frame_ms,
                     static_cast<unsigned long long>(_textured_triangle_submits),
                     static_cast<unsigned long long>(_gx_material_submits),
                     static_cast<unsigned long long>(_transient_vertex_bytes),
                     static_cast<unsigned long long>(_transient_index_bytes),
                     static_cast<unsigned long long>(_texture_binds),
                     static_cast<unsigned long long>(_uniform_updates),
                     static_cast<unsigned long long>(_dropped_transient_submits));
#endif
    }

    void BgfxBackend::print_static_geometry_cache_stats() const {
#ifndef NDEBUG
        if (!_static_geometry_cache_stats_enabled) {
            return;
        }

        std::fprintf(stderr,
                     "SMGPC geometry cache: enabled=%d entries=%zu bytes=%zu hits=%llu misses=%llu promotions=%llu evictions=%llu transient=%llu\n",
                     _static_geometry_cache_enabled ? 1 : 0, _static_geometry_cache.size(), _static_geometry_cache_bytes,
                     static_cast<unsigned long long>(_static_geometry_cache_hits),
                     static_cast<unsigned long long>(_static_geometry_cache_misses),
                     static_cast<unsigned long long>(_static_geometry_cache_promotions),
                     static_cast<unsigned long long>(_static_geometry_cache_evictions),
                     static_cast<unsigned long long>(_static_geometry_transient_submits));
#endif
    }

    void BgfxBackend::submit_textured_quad(core::TextureHandle texture, const core::TexturedQuad2D &quad) {
        constexpr std::array<std::uint16_t, 6U> indices{0U, 1U, 2U, 0U, 2U, 3U};
        submit_textured_triangles(texture, core::TexturedTriangleBatch2D{
                                               .vertices = std::span<const core::TexturedVertex2D>(quad.vertices.data(), quad.vertices.size()),
                                               .indices = std::span<const std::uint16_t>(indices.data(), indices.size()),
                                               .wrap_u = quad.wrap_u,
                                               .wrap_v = quad.wrap_v,
                                               .min_filter = quad.min_filter,
                                               .mag_filter = quad.mag_filter,
                                               .blend = quad.blend,
                                               .blend_mode = quad.blend_mode,
                                               .gx_blend = quad.gx_blend,
                                               .depth_test = quad.depth_test,
                                               .depth_write = quad.depth_write,
                                               .depth_compare = quad.depth_compare,
                                               .cull_mode = quad.cull_mode,
                                           });
    }

    void BgfxBackend::submit_textured_triangles(core::TextureHandle texture, const core::TexturedTriangleBatch2D &batch) {
        if (not _initialized || !texture.is_valid() || texture.value >= _textures.size() || !is_valid(_textures[texture.value]) ||
            !bgfx::isValid(_textured_quad_program)) {
            return;
        }

        if (batch.vertices.empty() || batch.indices.empty()) {
            return;
        }
        if (batch.cull_mode == core::CullMode::FrontAndBack) {
            return;
        }

        const auto vertex_count = static_cast<std::uint32_t>(batch.vertices.size());
        const auto index_count = static_cast<std::uint32_t>(batch.indices.size());
        auto *static_geometry = static_cast<StaticGeometryEntry *>(nullptr);
        if (_static_geometry_cache_enabled && static_geometry_cache_candidate(GEOMETRY_KIND_TEXTURED, vertex_count, index_count)) {
            const auto geometry_hash = hash_textured_geometry(batch);
            static_geometry = find_static_geometry(GEOMETRY_KIND_TEXTURED, geometry_hash, vertex_count, index_count);
            if (static_geometry == nullptr && should_promote_static_geometry(GEOMETRY_KIND_TEXTURED, geometry_hash, vertex_count, index_count)) {
                static_geometry = create_static_textured_geometry(geometry_hash, batch);
            }
        }

        auto transient_vertex_buffer = bgfx::TransientVertexBuffer{};
        auto transient_index_buffer = bgfx::TransientIndexBuffer{};
        if (static_geometry != nullptr) {
            bgfx::setVertexBuffer(0U, static_geometry->vertex_buffer);
            bgfx::setIndexBuffer(static_geometry->index_buffer);
        } else {
            if (bgfx::getAvailTransientVertexBuffer(vertex_count, _textured_quad_layout) < vertex_count ||
                bgfx::getAvailTransientIndexBuffer(index_count) < index_count) {
                ++_dropped_transient_submits;
                return;
            }
            ++_static_geometry_transient_submits;
            _transient_vertex_bytes += static_cast<std::uint64_t>(vertex_count) * sizeof(QuadVertex);
            _transient_index_bytes += static_cast<std::uint64_t>(index_count) * sizeof(std::uint16_t);

            bgfx::allocTransientVertexBuffer(&transient_vertex_buffer, vertex_count, _textured_quad_layout);
            bgfx::allocTransientIndexBuffer(&transient_index_buffer, index_count);

            const auto fit_scale = std::min(static_cast<float>(_framebuffer_width) / static_cast<float>(_logical_framebuffer_width),
                                            static_cast<float>(_framebuffer_height) / static_cast<float>(_logical_framebuffer_height));
            auto *vertices = reinterpret_cast<QuadVertex *>(transient_vertex_buffer.data);
            for (auto i = 0U; i < vertex_count; ++i) {
                const auto &source = batch.vertices[i];
                vertices[i] = QuadVertex{
                    .x = (source.x * fit_scale) / (static_cast<float>(_framebuffer_width) * 0.5F),
                    .y = (source.y * fit_scale) / (static_cast<float>(_framebuffer_height) * 0.5F),
                    .z = source.z,
                    .u = source.u,
                    .v = source.v,
                    .abgr = pack_abgr(source),
                };
            }

            auto *indices = reinterpret_cast<std::uint16_t *>(transient_index_buffer.data);
            std::ranges::copy(batch.indices, indices);

            bgfx::setVertexBuffer(0U, &transient_vertex_buffer);
            bgfx::setIndexBuffer(&transient_index_buffer);
        }
        const auto sampler_flags = gx_sampler_state(batch.wrap_u, batch.wrap_v, batch.min_filter, batch.mag_filter);
        bgfx::setTexture(0U, _texture_sampler, _textures[texture.value], sampler_flags);
        ++_texture_binds;
        auto state = gx_write_state(batch.gx_blend);
        if (batch.depth_test) {
            state |= depth_compare_state(batch.depth_compare);
        }
        if (batch.depth_write) {
            state |= BGFX_STATE_WRITE_Z;
        }
        state |= cull_state(batch.cull_mode);
        state |= primitive_topology_state(batch.primitive_topology);
        if (batch.gx_blend.enabled) {
            state |= gx_blend_state(batch.gx_blend);
        } else if (batch.blend && batch.blend_mode == core::BlendMode::Additive) {
            state |= BGFX_STATE_BLEND_ADD;
        } else if (batch.blend) {
            state |= BGFX_STATE_BLEND_ALPHA;
        }
        bgfx::setState(state);
        bgfx::submit(0U, _textured_quad_program);
        ++_textured_triangle_submits;
    }

    void BgfxBackend::submit_gx_material_triangles(const core::GxMaterialTriangleBatch2D &batch) {
        if (not _initialized || !bgfx::isValid(_gx_material_program)) {
            return;
        }

        if (batch.vertices.empty() || batch.indices.empty() || batch.texture_stages.size() > core::kMaxGxMaterialTextureStages2D ||
            batch.tev_stages.size() > core::kMaxGxMaterialTevStages2D) {
            return;
        }
        if (batch.cull_mode == core::CullMode::FrontAndBack) {
            return;
        }

        for (const auto &stage : batch.texture_stages) {
            if (!stage.texture.is_valid() || stage.texture.value >= _textures.size() || !is_valid(_textures[stage.texture.value])) {
                return;
            }
        }

        const auto vertex_count = static_cast<std::uint32_t>(batch.vertices.size());
        const auto index_count = static_cast<std::uint32_t>(batch.indices.size());
        auto *static_geometry = static_cast<StaticGeometryEntry *>(nullptr);
        if (_static_geometry_cache_enabled && static_geometry_cache_candidate(GEOMETRY_KIND_GX_MATERIAL, vertex_count, index_count)) {
            const auto geometry_hash = hash_gx_material_geometry(batch);
            static_geometry = find_static_geometry(GEOMETRY_KIND_GX_MATERIAL, geometry_hash, vertex_count, index_count);
            if (static_geometry == nullptr &&
                should_promote_static_geometry(GEOMETRY_KIND_GX_MATERIAL, geometry_hash, vertex_count, index_count)) {
                static_geometry = create_static_gx_material_geometry(geometry_hash, batch);
            }
        }

        auto transient_vertex_buffer = bgfx::TransientVertexBuffer{};
        auto transient_index_buffer = bgfx::TransientIndexBuffer{};
        if (static_geometry != nullptr) {
            bgfx::setVertexBuffer(0U, static_geometry->vertex_buffer);
            bgfx::setIndexBuffer(static_geometry->index_buffer);
        } else {
            if (bgfx::getAvailTransientVertexBuffer(vertex_count, _gx_material_layout) < vertex_count ||
                bgfx::getAvailTransientIndexBuffer(index_count) < index_count) {
                ++_dropped_transient_submits;
                return;
            }
            ++_static_geometry_transient_submits;
            _transient_vertex_bytes += static_cast<std::uint64_t>(vertex_count) * sizeof(GxMaterialVertex);
            _transient_index_bytes += static_cast<std::uint64_t>(index_count) * sizeof(std::uint16_t);

            bgfx::allocTransientVertexBuffer(&transient_vertex_buffer, vertex_count, _gx_material_layout);
            bgfx::allocTransientIndexBuffer(&transient_index_buffer, index_count);

            const auto fit_scale = std::min(static_cast<float>(_framebuffer_width) / static_cast<float>(_logical_framebuffer_width),
                                            static_cast<float>(_framebuffer_height) / static_cast<float>(_logical_framebuffer_height));
            auto *vertices = reinterpret_cast<GxMaterialVertex *>(transient_vertex_buffer.data);
            for (auto i = 0U; i < vertex_count; ++i) {
                const auto &source = batch.vertices[i];
                const auto clip_w = std::max(source.clip_w, 1.0F);
                const auto ndc_x = (source.x * fit_scale) / (static_cast<float>(_framebuffer_width) * 0.5F);
                const auto ndc_y = (source.y * fit_scale) / (static_cast<float>(_framebuffer_height) * 0.5F);
                vertices[i] = GxMaterialVertex{
                    .x = ndc_x * clip_w,
                    .y = ndc_y * clip_w,
                    .z = source.z * clip_w,
                    .u0 = source.tex_coords[0U][0U],
                    .v0 = source.tex_coords[0U][1U],
                    .q0 = source.tex_coords[0U][2U],
                    .u1 = source.tex_coords[1U][0U],
                    .v1 = source.tex_coords[1U][1U],
                    .q1 = source.tex_coords[1U][2U],
                    .u2 = source.tex_coords[2U][0U],
                    .v2 = source.tex_coords[2U][1U],
                    .q2 = source.tex_coords[2U][2U],
                    .clip_w = clip_w,
                    .abgr = pack_abgr(source),
                };
            }

            auto *indices = reinterpret_cast<std::uint16_t *>(transient_index_buffer.data);
            std::ranges::copy(batch.indices, indices);

            bgfx::setVertexBuffer(0U, &transient_vertex_buffer);
            bgfx::setIndexBuffer(&transient_index_buffer);
        }
        const auto tev_stage_count = std::min<std::size_t>(
            core::kMaxGxMaterialTevStages2D, batch.tev_stages.empty() ? batch.texture_stages.size() : batch.tev_stages.size());

        constexpr auto default_color_in = std::array<std::uint8_t, 4U>{15U, 8U, 10U, 15U};
        constexpr auto default_alpha_in = std::array<std::uint8_t, 4U>{7U, 4U, 5U, 7U};
        auto used_texture_stages = std::array<bool, core::kMaxGxMaterialTextureStages2D>{};
        for (auto i = 0U; i < tev_stage_count; ++i) {
            const auto *tev_stage = i < batch.tev_stages.size() ? &batch.tev_stages[i] : nullptr;
            const auto texture_stage = static_cast<std::size_t>(tev_stage == nullptr ? i : tev_stage->texture_stage);
            if (texture_stage < used_texture_stages.size() && texture_stage < batch.texture_stages.size()) {
                used_texture_stages[texture_stage] = true;
            }
        }

        for (auto stage_index = 0U; stage_index < batch.texture_stages.size(); ++stage_index) {
            if (!used_texture_stages[stage_index]) {
                continue;
            }
            const auto &stage = batch.texture_stages[stage_index];
            const auto sampler_flags = gx_sampler_state(stage.wrap_u, stage.wrap_v, stage.min_filter, stage.mag_filter);
            bgfx::setTexture(static_cast<std::uint8_t>(stage_index), _gx_material_samplers[stage_index], _textures[stage.texture.value], sampler_flags);
            ++_texture_binds;
        }

        const std::array<float, 4U> material_params{
            static_cast<float>(tev_stage_count),
            0.0F,
            0.0F,
            0.0F,
        };
        bgfx::setUniform(_gx_material_params, material_params.data());
        ++_uniform_updates;

        for (auto i = 0U; i < tev_stage_count; ++i) {
            const auto *stage = i < batch.tev_stages.size() ? &batch.tev_stages[i] : nullptr;
            const auto color_in = stage == nullptr ? default_color_in : stage->color_in;
            const auto alpha_in = stage == nullptr ? default_alpha_in : stage->alpha_in;
            const std::array<float, 4U> color_inputs{
                static_cast<float>(color_in[0U]),
                static_cast<float>(color_in[1U]),
                static_cast<float>(color_in[2U]),
                static_cast<float>(color_in[3U]),
            };
            const std::array<float, 4U> alpha_inputs{
                static_cast<float>(alpha_in[0U]),
                static_cast<float>(alpha_in[1U]),
                static_cast<float>(alpha_in[2U]),
                static_cast<float>(alpha_in[3U]),
            };
            const std::array<float, 4U> color_operation{
                static_cast<float>(stage == nullptr ? 0U : stage->color_op),
                static_cast<float>(stage == nullptr ? 0U : stage->color_bias),
                static_cast<float>(stage == nullptr ? 0U : stage->color_scale),
                stage == nullptr || stage->color_clamp ? 1.0F : 0.0F,
            };
            const std::array<float, 4U> alpha_operation{
                static_cast<float>(stage == nullptr ? 0U : stage->alpha_op),
                static_cast<float>(stage == nullptr ? 0U : stage->alpha_bias),
                static_cast<float>(stage == nullptr ? 0U : stage->alpha_scale),
                stage == nullptr || stage->alpha_clamp ? 1.0F : 0.0F,
            };
            const std::array<float, 4U> outputs{
                static_cast<float>(stage == nullptr ? 0U : stage->color_out),
                static_cast<float>(stage == nullptr ? 0U : stage->alpha_out),
                static_cast<float>(stage == nullptr ? i : stage->texture_stage),
                0.0F,
            };
            const std::array<float, 4U> stage_konst{
                static_cast<float>(stage == nullptr ? 0U : stage->konst_color[0U]) / 255.0F,
                static_cast<float>(stage == nullptr ? 0U : stage->konst_color[1U]) / 255.0F,
                static_cast<float>(stage == nullptr ? 0U : stage->konst_color[2U]) / 255.0F,
                static_cast<float>(stage == nullptr ? 0U : stage->konst_color[3U]) / 255.0F,
            };
            bgfx::setUniform(_gx_tev_color_inputs[i], color_inputs.data());
            bgfx::setUniform(_gx_tev_alpha_inputs[i], alpha_inputs.data());
            bgfx::setUniform(_gx_tev_color_operations[i], color_operation.data());
            bgfx::setUniform(_gx_tev_alpha_operations[i], alpha_operation.data());
            bgfx::setUniform(_gx_tev_outputs[i], outputs.data());
            bgfx::setUniform(_gx_tev_konst_colors[i], stage_konst.data());
            _uniform_updates += 6U;
        }
        for (auto i = 0U; i < _gx_tev_initial_registers.size(); ++i) {
            const std::array<float, 4U> initial_register{
                static_cast<float>(batch.initial_tev_registers[i][0U]) / 255.0F,
                static_cast<float>(batch.initial_tev_registers[i][1U]) / 255.0F,
                static_cast<float>(batch.initial_tev_registers[i][2U]) / 255.0F,
                static_cast<float>(batch.initial_tev_registers[i][3U]) / 255.0F,
            };
            bgfx::setUniform(_gx_tev_initial_registers[i], initial_register.data());
            ++_uniform_updates;
        }

        const std::array<float, 4U> alpha_compare_0{
            static_cast<float>(batch.alpha_compare.comp0),
            static_cast<float>(batch.alpha_compare.ref0) / 255.0F,
            static_cast<float>(batch.alpha_compare.op),
            static_cast<float>(batch.alpha_compare.comp1),
        };
        const std::array<float, 4U> alpha_compare_1{
            static_cast<float>(batch.alpha_compare.ref1) / 255.0F,
            batch.alpha_compare.enabled ? 1.0F : 0.0F,
            0.0F,
            0.0F,
        };
        bgfx::setUniform(_gx_alpha_compare_1, alpha_compare_1.data());
        ++_uniform_updates;
        if (batch.alpha_compare.enabled) {
            bgfx::setUniform(_gx_alpha_compare_0, alpha_compare_0.data());
            ++_uniform_updates;
        }

        auto state = gx_write_state(batch.blend);
        if (batch.depth_test) {
            state |= depth_compare_state(batch.depth_compare);
        }
        if (batch.depth_write) {
            state |= BGFX_STATE_WRITE_Z;
        }
        state |= cull_state(batch.cull_mode);
        state |= primitive_topology_state(batch.primitive_topology);
        state |= gx_blend_state(batch.blend);
        bgfx::setState(state);
        bgfx::submit(0U, _gx_material_program);
        ++_gx_material_submits;
    }

    core::FramebufferInfo BgfxBackend::framebuffer_size() const {
        return {
            .width = _framebuffer_width,
            .height = _framebuffer_height,
        };
    }

    core::FramebufferInfo BgfxBackend::logical_framebuffer_size() const {
        return {
            .width = _logical_framebuffer_width,
            .height = _logical_framebuffer_height,
        };
    }

    bool BgfxBackend::is_initialized() const {
        return _initialized;
    }

}  // namespace smgpc::render::backends
