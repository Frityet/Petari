#include "backends/BgfxBackend.hpp"

#include <algorithm>
#include <array>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include "capture/ScreenshotService.hpp"

#include "shaders/generated/fs_texture_quad_gl.h"
#include "shaders/generated/fs_texture_quad_spv.h"
#include "shaders/generated/vs_texture_quad_gl.h"
#include "shaders/generated/vs_texture_quad_spv.h"

namespace smgpc::render::backends {
    namespace {

        struct QuadVertex {
            float x = 0.0F;
            float y = 0.0F;
            float z = 0.0F;
            float u = 0.0F;
            float v = 0.0F;
            std::uint32_t abgr = 0xffffffffU;
        };

        [[nodiscard]] std::optional< bgfx::RendererType::Enum > resolve_renderer_type_from_environment() {
            const char* value = std::getenv("SMGPC_BGFX_RENDERER");
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

        [[nodiscard]] const bgfx::Memory* copy_shader_memory(const std::uint8_t* data, std::size_t size) {
            return bgfx::copy(data, static_cast< std::uint32_t >(size));
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

        [[nodiscard]] std::uint32_t pack_abgr(const core::TexturedVertex2D& vertex) {
            return (static_cast< std::uint32_t >(vertex.color[3U]) << 24U) | (static_cast< std::uint32_t >(vertex.color[2U]) << 16U) |
                   (static_cast< std::uint32_t >(vertex.color[1U]) << 8U) | static_cast< std::uint32_t >(vertex.color[0U]);
        }

        [[nodiscard]] bool is_valid(bgfx::TextureHandle handle) {
            return bgfx::isValid(handle);
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

    }  // namespace

    class BgfxCallback final : public bgfx::CallbackI {
    public:
        void fatal(const char* _filePath, std::uint16_t _line, bgfx::Fatal::Enum _code, const char* _str) override {
            std::fprintf(stderr, "bgfx fatal at %s:%u: %s\n", _filePath != nullptr ? _filePath : "<unknown>", _line,
                         _str != nullptr ? _str : "<no message>");
            if (_code != bgfx::Fatal::DebugCheck) {
                std::abort();
            }
        }

        void traceVargs(const char* _filePath, std::uint16_t _line, const char* _format, va_list _argList) override {
            std::fprintf(stderr, "bgfx trace at %s:%u: ", _filePath != nullptr ? _filePath : "<unknown>", _line);
            std::vfprintf(stderr, _format, _argList);
        }

        void profilerBegin(const char*, std::uint32_t, const char*, std::uint16_t) override {
        }
        void profilerBeginLiteral(const char*, std::uint32_t, const char*, std::uint16_t) override {
        }
        void profilerEnd() override {
        }

        std::uint32_t cacheReadSize(std::uint64_t) override {
            return 0U;
        }

        bool cacheRead(std::uint64_t, void*, std::uint32_t) override {
            return false;
        }

        void cacheWrite(std::uint64_t, const void*, std::uint32_t) override {
        }

        void screenShot(const char* _filePath, std::uint32_t _width, std::uint32_t _height, std::uint32_t _pitch, bgfx::TextureFormat::Enum _format,
                        const void* _data, std::uint32_t _size, bool _yflip) override {
            if (_filePath == nullptr || _data == nullptr || _size == 0U) {
                return;
            }

            if (_format != bgfx::TextureFormat::BGRA8 && _format != bgfx::TextureFormat::RGBA8) {
                std::fprintf(stderr, "bgfx screenshot had unsupported texture format %d\n", static_cast< int >(_format));
                return;
            }

            const auto pixel_format = _format == bgfx::TextureFormat::BGRA8 ? capture::PixelFormat::BGRA8 : capture::PixelFormat::RGBA8;
            try {
                _screenshot_service->write_png(std::filesystem::path(_filePath),
                                               capture::ScreenshotImageView{
                                                   .width = _width,
                                                   .height = _height,
                                                   .pitch = _pitch,
                                                   .pixels = std::span< const std::uint8_t >(static_cast< const std::uint8_t* >(_data), _size),
                                                   .format = pixel_format,
                                                   .origin_bottom_left = _yflip,
                                               });
            } catch (const std::exception& e) {
                std::fprintf(stderr, "failed to write bgfx screenshot PNG %s: %s\n", _filePath, e.what());
            }
        }

        void captureBegin(std::uint32_t, std::uint32_t, std::uint32_t, bgfx::TextureFormat::Enum, bool) override {
        }
        void captureEnd() override {
        }
        void captureFrame(const void*, std::uint32_t) override {
        }

    private:
        std::unique_ptr< capture::IScreenshotService > _screenshot_service{capture::create_png_screenshot_service()};
    };

    BgfxBackend::BgfxBackend() : _callback(std::make_unique< BgfxCallback >()) {
    }

    BgfxBackend::~BgfxBackend() {
        shutdown();
    }

    void BgfxBackend::initialize(const core::RenderInitDesc& description) {
        if (_initialized) {
            return;
        }

        _vsync_enabled = description.enable_vsync;

        const auto try_initialize = [&](bgfx::RendererType::Enum renderer_type) {
            auto init = bgfx::Init();
            init.type = renderer_type;
            init.platformData.nwh = description.native_window_handle;
            init.platformData.ndt = description.native_display_handle;
            init.resolution.width = static_cast< std::uint32_t >(description.width);
            init.resolution.height = static_cast< std::uint32_t >(description.height);
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
            constexpr std::array< bgfx::RendererType::Enum, 3U > renderer_fallback{
                bgfx::RendererType::Vulkan,
                bgfx::RendererType::OpenGL,
                bgfx::RendererType::Count,
            };
#else
            constexpr std::array< bgfx::RendererType::Enum, 2U > renderer_fallback{
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
        _framebuffer_width = static_cast< std::uint16_t >(description.width);
        _framebuffer_height = static_cast< std::uint16_t >(description.height);
        _logical_framebuffer_width = std::max< std::uint16_t >(1U, description.logical_width);
        _logical_framebuffer_height = std::max< std::uint16_t >(1U, description.logical_height);

        _textured_quad_layout.begin()
            .add(bgfx::Attrib::Position, 3U, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2U, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4U, bgfx::AttribType::Uint8, true)
            .end();

        _texture_sampler = bgfx::createUniform("s_texture", bgfx::UniformType::Sampler);
        _textured_quad_program =
            bgfx::createProgram(create_vertex_shader_for_current_renderer(), create_fragment_shader_for_current_renderer(), true);
    }

    void BgfxBackend::shutdown() {
        if (not _initialized) {
            return;
        }

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

        if (bgfx::isValid(_texture_sampler)) {
            bgfx::destroy(_texture_sampler);
            _texture_sampler = BGFX_INVALID_HANDLE;
        }

        bgfx::shutdown();
        _initialized = false;
    }

    void BgfxBackend::begin_frame(const core::FrameContext& frame_context) {
        if (not _initialized) {
            return;
        }

        bgfx::setViewRect(0U, 0U, 0U, frame_context.framebuffer.width, frame_context.framebuffer.height);
        bgfx::setViewClear(0U, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0F, 0U);
        bgfx::touch(0U);
    }

    void BgfxBackend::end_frame() {
        if (_initialized) {
            bgfx::frame();
        }
    }

    void BgfxBackend::resize(std::uint16_t width, std::uint16_t height) {
        if (not _initialized || width == 0U || height == 0U) {
            return;
        }

        _framebuffer_width = width;
        _framebuffer_height = height;
        bgfx::reset(width, height, _vsync_enabled ? BGFX_RESET_VSYNC : BGFX_RESET_NONE);
    }

    void BgfxBackend::request_screenshot_png(const std::filesystem::path& path) {
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

    core::TextureHandle BgfxBackend::create_rgba8_texture(std::uint16_t width, std::uint16_t height, std::span< const std::uint8_t > rgba) {
        if (not _initialized || width == 0U || height == 0U) {
            return {};
        }

        const auto expected_size = static_cast< std::size_t >(width) * static_cast< std::size_t >(height) * 4U;
        if (rgba.size() < expected_size) {
            throw std::runtime_error("RGBA8 texture upload buffer is shorter than the declared texture dimensions");
        }

        const auto* memory = bgfx::copy(rgba.data(), static_cast< std::uint32_t >(expected_size));
        const auto handle =
            bgfx::createTexture2D(width, height, false, 1U, bgfx::TextureFormat::RGBA8, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP, memory);
        if (!bgfx::isValid(handle)) {
            return {};
        }

        _textures.push_back(handle);
        return core::TextureHandle{.value = static_cast< std::uint32_t >(_textures.size() - 1U)};
    }

    void BgfxBackend::destroy_texture(core::TextureHandle texture) {
        if (!texture.is_valid() || texture.value >= _textures.size()) {
            return;
        }

        auto& handle = _textures[texture.value];
        if (is_valid(handle)) {
            bgfx::destroy(handle);
            handle = BGFX_INVALID_HANDLE;
        }
    }

    void BgfxBackend::submit_textured_quad(core::TextureHandle texture, const core::TexturedQuad2D& quad) {
        constexpr std::array< std::uint16_t, 6U > indices{0U, 1U, 2U, 0U, 2U, 3U};
        submit_textured_triangles(texture, core::TexturedTriangleBatch2D{
                                               .vertices = std::span< const core::TexturedVertex2D >(quad.vertices.data(), quad.vertices.size()),
                                               .indices = std::span< const std::uint16_t >(indices.data(), indices.size()),
                                               .wrap_u = quad.wrap_u,
                                               .wrap_v = quad.wrap_v,
                                               .blend = quad.blend,
                                               .blend_mode = quad.blend_mode,
                                               .depth_test = quad.depth_test,
                                               .depth_write = quad.depth_write,
                                               .depth_compare = quad.depth_compare,
                                           });
    }

    void BgfxBackend::submit_textured_triangles(core::TextureHandle texture, const core::TexturedTriangleBatch2D& batch) {
        if (not _initialized || !texture.is_valid() || texture.value >= _textures.size() || !is_valid(_textures[texture.value]) ||
            !bgfx::isValid(_textured_quad_program)) {
            return;
        }

        if (batch.vertices.empty() || batch.indices.empty()) {
            return;
        }

        const auto vertex_count = static_cast< std::uint32_t >(batch.vertices.size());
        const auto index_count = static_cast< std::uint32_t >(batch.indices.size());
        if (bgfx::getAvailTransientVertexBuffer(vertex_count, _textured_quad_layout) < vertex_count ||
            bgfx::getAvailTransientIndexBuffer(index_count) < index_count) {
            return;
        }

        bgfx::TransientVertexBuffer vertex_buffer{};
        bgfx::TransientIndexBuffer index_buffer{};
        bgfx::allocTransientVertexBuffer(&vertex_buffer, vertex_count, _textured_quad_layout);
        bgfx::allocTransientIndexBuffer(&index_buffer, index_count);

        const auto fit_scale = std::min(static_cast< float >(_framebuffer_width) / static_cast< float >(_logical_framebuffer_width),
                                        static_cast< float >(_framebuffer_height) / static_cast< float >(_logical_framebuffer_height));
        auto* vertices = reinterpret_cast< QuadVertex* >(vertex_buffer.data);
        for (auto i = 0U; i < vertex_count; ++i) {
            const auto& source = batch.vertices[i];
            vertices[i] = QuadVertex{
                .x = (source.x * fit_scale) / (static_cast< float >(_framebuffer_width) * 0.5F),
                .y = (source.y * fit_scale) / (static_cast< float >(_framebuffer_height) * 0.5F),
                .z = source.z,
                .u = source.u,
                .v = source.v,
                .abgr = pack_abgr(source),
            };
        }

        auto* indices = reinterpret_cast< std::uint16_t* >(index_buffer.data);
        std::ranges::copy(batch.indices, indices);

        bgfx::setVertexBuffer(0U, &vertex_buffer);
        bgfx::setIndexBuffer(&index_buffer);
        auto sampler_flags = std::uint32_t{};
        if (!batch.wrap_u) {
            sampler_flags |= BGFX_SAMPLER_U_CLAMP;
        }
        if (!batch.wrap_v) {
            sampler_flags |= BGFX_SAMPLER_V_CLAMP;
        }
        bgfx::setTexture(0U, _texture_sampler, _textures[texture.value], sampler_flags);
        auto state = std::uint64_t{BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A};
        if (batch.depth_test) {
            state |= depth_compare_state(batch.depth_compare);
        }
        if (batch.depth_write) {
            state |= BGFX_STATE_WRITE_Z;
        }
        if (batch.blend && batch.blend_mode == core::BlendMode::Additive) {
            state |= BGFX_STATE_BLEND_ADD;
        } else if (batch.blend) {
            state |= BGFX_STATE_BLEND_ALPHA;
        }
        bgfx::setState(state);
        bgfx::submit(0U, _textured_quad_program);
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
