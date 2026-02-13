#include "BgfxBackend.hpp"

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

    if (bgfx::isValid(_sampler)) {
        bgfx::destroy(_sampler);
    }

    if (bgfx::isValid(_mask_sampler)) {
        bgfx::destroy(_mask_sampler);
    }

    if (bgfx::isValid(_mask_params)) {
        bgfx::destroy(_mask_params);
    }

    if (bgfx::isValid(_white_texture)) {
        bgfx::destroy(_white_texture);
    }

    _layout_program = BGFX_INVALID_HANDLE;
    _sampler = BGFX_INVALID_HANDLE;
    _mask_sampler = BGFX_INVALID_HANDLE;
    _mask_params = BGFX_INVALID_HANDLE;
    _white_texture = BGFX_INVALID_HANDLE;
    _layout_resources_ready = false;

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
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord1, 2, bgfx::AttribType::Float)
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

    const std::uint32_t white_pixel = 0xFFFFFFFFU;
    const auto *white_memory = bgfx::copy(&white_pixel, sizeof(white_pixel));
    _white_texture = bgfx::createTexture2D(1U, 1U, false, 1U, bgfx::TextureFormat::BGRA8, 0U, white_memory);

    _layout_resources_ready = true;
}

bgfx::TextureHandle BgfxBackend::resolve_texture(const smgpc::render::core::RenderTextureRef &texture) {
    if (texture.id == 0U || texture.rgba8 == nullptr || texture.width == 0U || texture.height == 0U) {
        return _white_texture;
    }

    const auto found = _textures.find(texture.id);
    if (found != _textures.end()) {
        return found->second;
    }

    const auto byte_count = static_cast<uint64_t>(texture.width) * texture.height * 4ULL;
    const auto *memory = bgfx::copy(texture.rgba8, static_cast<std::uint32_t>(byte_count));
    const auto handle = bgfx::createTexture2D(
        texture.width,
        texture.height,
        false,
        1U,
        bgfx::TextureFormat::RGBA8,
        BGFX_TEXTURE_NONE,
        memory);

    if (bgfx::isValid(handle)) {
        _textures.emplace(texture.id, handle);
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
}

void BgfxBackend::execute_draw_layout(const smgpc::render::core::RenderDrawLayoutCommand &command) {
    ensure_layout_resources();

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

    bgfx::setViewRect(0U, 0U, 0U, framebuffer_width, framebuffer_height);
    bgfx::setViewMode(0U, bgfx::ViewMode::Sequential);

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
    bgfx::setViewTransform(0U, nullptr, ortho);

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

    const auto draw_quad = [&](const smgpc::render::core::RenderLayoutQuad &quad) {
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

        struct Vertex {
            float x;
            float y;
            float z;
            float w;
            std::uint32_t abgr;
            float u;
            float v;
            float u_mask;
            float v_mask;
        };

        auto *vertices = reinterpret_cast<Vertex *>(vertex_buffer.data);
        vertices[0] = Vertex {
            .x = quad.x0 * scale_x,
            .y = quad.y0 * scale_y,
            .z = 0.0F,
            .w = 1.0F,
            .abgr = quad.color_tl,
            .u = quad.u0,
            .v = quad.v0,
            .u_mask = quad.u0_secondary,
            .v_mask = quad.v0_secondary,
        };
        vertices[1] = Vertex {
            .x = quad.x1 * scale_x,
            .y = quad.y0 * scale_y,
            .z = 0.0F,
            .w = 1.0F,
            .abgr = quad.color_tr,
            .u = quad.u1,
            .v = quad.v0,
            .u_mask = quad.u1_secondary,
            .v_mask = quad.v0_secondary,
        };
        vertices[2] = Vertex {
            .x = quad.x0 * scale_x,
            .y = quad.y1 * scale_y,
            .z = 0.0F,
            .w = 1.0F,
            .abgr = quad.color_bl,
            .u = quad.u0,
            .v = quad.v1,
            .u_mask = quad.u0_secondary,
            .v_mask = quad.v1_secondary,
        };
        vertices[3] = Vertex {
            .x = quad.x1 * scale_x,
            .y = quad.y1 * scale_y,
            .z = 0.0F,
            .w = 1.0F,
            .abgr = quad.color_br,
            .u = quad.u1,
            .v = quad.v1,
            .u_mask = quad.u1_secondary,
            .v_mask = quad.v1_secondary,
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

        bgfx::setTexture(0U, _sampler, texture_handle);
        bgfx::setTexture(1U, _mask_sampler, mask_texture_handle);
        const float mask_params[] {
            quad.use_mask_texture ? 1.0F : 0.0F,
            quad.invert_mask ? 1.0F : 0.0F,
            quad.mask_uses_alpha ? 1.0F : 0.0F,
            0.0F,
        };
        bgfx::setUniform(_mask_params, mask_params);
        bgfx::setScissor(0U, 0U, framebuffer_width, framebuffer_height);
        bgfx::setState(
            quad.blend_mode == smgpc::render::core::RenderBlendMode::Additive
                ? DRAW_STATE_ADDITIVE
                : DRAW_STATE_ALPHA);
        bgfx::setVertexBuffer(0U, &vertex_buffer);
        bgfx::setIndexBuffer(&index_buffer);
        bgfx::setTransform(nullptr);
        bgfx::submit(0U, _layout_program);
    };

    if (command.debug_solid_quad) {
        bgfx::touch(0U);
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
        bgfx::touch(0U);
        return;
    }

    if (command.quads.empty()) {
        bgfx::touch(0U);
        return;
    }

    for (const auto &quad : command.quads) {
        draw_quad(quad);
    }
}

}  // namespace smgpc::render::backends
