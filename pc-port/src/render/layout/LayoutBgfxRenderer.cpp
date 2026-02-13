#include "LayoutBgfxRenderer.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <bgfx/bgfx.h>
#include <bx/math.h>
#if defined(__linux__)
#include <unistd.h>
#endif

#include "Logger.hpp"

namespace smgpc::render::layout {
namespace {

[[nodiscard]] const char *shader_backend_dir(bgfx::RendererType::Enum renderer_type) {
    switch (renderer_type) {
    case bgfx::RendererType::Vulkan:
        return "spirv";
    case bgfx::RendererType::OpenGL:
    case bgfx::RendererType::OpenGLES:
        return "glsl";
    default:
        return "glsl";
    }
}

[[nodiscard]] std::optional<std::filesystem::path> resolve_executable_path() {
#if defined(__linux__)
    std::array<char, 4096> buffer {};
    const auto bytes_written = ::readlink("/proc/self/exe", buffer.data(), buffer.size() - 1U);
    if (bytes_written <= 0) {
        return std::nullopt;
    }

    buffer[static_cast<std::size_t>(bytes_written)] = '\0';
    return std::filesystem::path(buffer.data());
#else
    return std::nullopt;
#endif
}

}  // namespace

bgfx::VertexLayout LayoutBgfxRenderer::Vertex::layout {};

void LayoutBgfxRenderer::Vertex::init_layout() {
    layout.begin()
        .add(bgfx::Attrib::Position, 4, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord1, 2, bgfx::AttribType::Float)
        .end();
}

LayoutBgfxRenderer::LayoutBgfxRenderer(di::DependencyReference<logging::ILogger> logger)
    : _logger(std::move(logger)) {
}

LayoutBgfxRenderer::~LayoutBgfxRenderer() {
    shutdown();
}

void LayoutBgfxRenderer::shutdown() {
    if (not _initialized) {
        return;
    }

    for (auto &[_, texture_handle] : _texture_cache) {
        if (bgfx::isValid(texture_handle)) {
            bgfx::destroy(texture_handle);
        }
    }
    _texture_cache.clear();

    if (bgfx::isValid(_white_texture)) {
        bgfx::destroy(_white_texture);
        _white_texture = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(_program)) {
        bgfx::destroy(_program);
        _program = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(_sampler)) {
        bgfx::destroy(_sampler);
        _sampler = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(_mask_sampler)) {
        bgfx::destroy(_mask_sampler);
        _mask_sampler = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(_mask_params)) {
        bgfx::destroy(_mask_params);
        _mask_params = BGFX_INVALID_HANDLE;
    }

    _initialized = false;
}

std::vector<std::byte> LayoutBgfxRenderer::read_file_bytes(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    if (not file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + path.string());
    }

    std::vector<std::byte> bytes {};
    for (auto iterator = std::istreambuf_iterator<char>(file); iterator != std::istreambuf_iterator<char>(); ++iterator) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(*iterator)));
    }
    return bytes;
}

std::filesystem::path LayoutBgfxRenderer::resolve_shader_directory() {
    const auto renderer_type = bgfx::getRendererType();
    const auto backend_dir = std::string(shader_backend_dir(renderer_type));
    std::vector<std::filesystem::path> base_candidates {};
    base_candidates.reserve(10U);

    if (const auto executable_path = resolve_executable_path(); executable_path.has_value()) {
        const auto executable_directory = executable_path->parent_path();
        if (not executable_directory.empty()) {
            base_candidates.push_back(executable_directory / "shaders");
        }
    }

    const std::array<std::filesystem::path, 8> legacy_candidates {
        std::filesystem::current_path() / "build" / "linux" / "x86_64" / "debug" / "shaders",
        std::filesystem::current_path() / "build" / "linux" / "x86_64" / "release" / "shaders",
        std::filesystem::current_path() / "pc-port" / "build" / "linux" / "x86_64" / "debug" / "shaders",
        std::filesystem::current_path() / "pc-port" / "build" / "linux" / "x86_64" / "release" / "shaders",
        std::filesystem::current_path() / "shaders",
        std::filesystem::current_path().parent_path() / "build" / "linux" / "x86_64" / "debug" / "shaders",
        std::filesystem::current_path().parent_path() / "build" / "linux" / "x86_64" / "release" / "shaders",
        std::filesystem::current_path().parent_path() / "shaders",
    };
    base_candidates.insert(base_candidates.end(), legacy_candidates.begin(), legacy_candidates.end());

    for (const auto &base_candidate : base_candidates) {
        const auto candidate = base_candidate / backend_dir;
        if (std::filesystem::exists(candidate / "vs_layout.bin") and std::filesystem::exists(candidate / "fs_layout.bin")) {
            return candidate;
        }
    }

    throw std::runtime_error("Failed to locate bgfx layout shader binaries for backend " + backend_dir + " (vs_layout.bin/fs_layout.bin).");
}

void LayoutBgfxRenderer::ensure_initialized() {
    if (_initialized) {
        return;
    }

    Vertex::init_layout();

    const auto shader_dir = resolve_shader_directory();
    _logger->info(__FILE__, __LINE__, logging::Category::RENDERER, "Layout renderer loading shaders from {}", shader_dir.string());
    const auto vertex_shader_bytes = read_file_bytes(shader_dir / "vs_layout.bin");
    const auto fragment_shader_bytes = read_file_bytes(shader_dir / "fs_layout.bin");

    const auto *vertex_shader_memory = bgfx::copy(vertex_shader_bytes.data(), static_cast<std::uint32_t>(vertex_shader_bytes.size()));
    const auto *fragment_shader_memory = bgfx::copy(fragment_shader_bytes.data(), static_cast<std::uint32_t>(fragment_shader_bytes.size()));

    const auto vertex_shader = bgfx::createShader(vertex_shader_memory);
    const auto fragment_shader = bgfx::createShader(fragment_shader_memory);
    if (not bgfx::isValid(vertex_shader) or not bgfx::isValid(fragment_shader)) {
        throw std::runtime_error("Failed to create bgfx layout shader handles.");
    }

    _program = bgfx::createProgram(vertex_shader, fragment_shader, true);
    if (not bgfx::isValid(_program)) {
        throw std::runtime_error("Failed to create bgfx layout program.");
    }

    _sampler = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);
    _mask_sampler = bgfx::createUniform("s_mask", bgfx::UniformType::Sampler);
    _mask_params = bgfx::createUniform("u_mask_params", bgfx::UniformType::Vec4);

    const std::uint32_t white_pixel = 0xFFFFFFFFU;
    const auto *white_memory = bgfx::copy(&white_pixel, sizeof(white_pixel));
    _white_texture = bgfx::createTexture2D(1U, 1U, false, 1U, bgfx::TextureFormat::BGRA8, 0U, white_memory);

    _initialized = true;
}

bgfx::TextureHandle LayoutBgfxRenderer::resolve_texture(const TextureRef &texture) {
    if (texture.id == 0U or texture.rgba8 == nullptr or texture.width == 0U or texture.height == 0U) {
        return _white_texture;
    }

    const auto found = _texture_cache.find(texture.id);
    if (found != _texture_cache.end()) {
        return found->second;
    }

    const auto byte_count = static_cast<std::size_t>(texture.width) * texture.height * 4U;
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
        _texture_cache.emplace(texture.id, handle);
        return handle;
    }

    _logger->warning(__FILE__, __LINE__, logging::Category::RENDERER, "Failed to create texture for draw-list id {}", texture.id);
    return _white_texture;
}

void LayoutBgfxRenderer::draw(
    const LayoutDrawList &draw_list,
    std::uint16_t framebuffer_width,
    std::uint16_t framebuffer_height,
    float layout_width,
    float layout_height) {
    ensure_initialized();

    if (framebuffer_width == 0U or framebuffer_height == 0U) {
        return;
    }

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

    if (layout_width <= 0.0F) {
        layout_width = static_cast<float>(framebuffer_width);
    }
    if (layout_height <= 0.0F) {
        layout_height = static_cast<float>(framebuffer_height);
    }
    const float scale_x = static_cast<float>(framebuffer_width) / layout_width;
    const float scale_y = static_cast<float>(framebuffer_height) / layout_height;

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

    const bool debug_solid_quad = [] {
        const char *value = std::getenv("SMGPC_DEBUG_SOLID_QUAD");
        return value != nullptr && value[0] != '\0' && value[0] != '0';
    }();
    const bool debug_force_touch = [] {
        const char *value = std::getenv("SMGPC_DEBUG_FORCE_TOUCH");
        return value != nullptr && value[0] != '\0' && value[0] != '0';
    }();
    const bool debug_trace = [] {
        const char *value = std::getenv("SMGPC_DEBUG_LAYOUT_TRACE");
        return value != nullptr && value[0] != '\0' && value[0] != '0';
    }();
    if (debug_trace) {
        _logger->debug(
            __FILE__,
            __LINE__,
            logging::Category::RENDERER,
            "layout draw begin: quads={}, fb={}x{}, debug_solid_quad={}",
            draw_list.quads().size(),
            framebuffer_width,
            framebuffer_height,
            debug_solid_quad);
    }

    const auto draw_quad = [&](const QuadCommand &quad) {
        constexpr std::uint32_t VERTEX_COUNT = 4U;
        constexpr std::uint32_t INDEX_COUNT = 6U;

        const auto available_vertices = bgfx::getAvailTransientVertexBuffer(VERTEX_COUNT, Vertex::layout);
        const auto available_indices = bgfx::getAvailTransientIndexBuffer(INDEX_COUNT);
        if (available_vertices < VERTEX_COUNT || available_indices < INDEX_COUNT) {
            if (debug_trace) {
                _logger->warning(
                    __FILE__,
                    __LINE__,
                    logging::Category::RENDERER,
                    "layout draw skipped quad: avail_vtx={}, avail_idx={}",
                    available_vertices,
                    available_indices);
            }
            return;
        }

        bgfx::TransientVertexBuffer vertex_buffer {};
        bgfx::TransientIndexBuffer index_buffer {};
        bgfx::allocTransientVertexBuffer(&vertex_buffer, VERTEX_COUNT, Vertex::layout);
        bgfx::allocTransientIndexBuffer(&index_buffer, INDEX_COUNT);

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
        const auto mask_texture_handle = quad.use_mask_texture
            ? resolve_texture(quad.mask_texture)
            : _white_texture;

        bgfx::setTexture(0U, _sampler, texture_handle);
        bgfx::setTexture(1U, _mask_sampler, mask_texture_handle);
        const float mask_params[4] = {
            quad.use_mask_texture ? 1.0F : 0.0F,
            quad.invert_mask ? 1.0F : 0.0F,
            quad.mask_uses_alpha ? 1.0F : 0.0F,
            0.0F,
        };
        bgfx::setUniform(_mask_params, mask_params);
        bgfx::setScissor(0U, 0U, framebuffer_width, framebuffer_height);
        bgfx::setState(quad.blend_mode == BlendMode::Additive ? DRAW_STATE_ADDITIVE : DRAW_STATE_ALPHA);
        bgfx::setVertexBuffer(0U, &vertex_buffer);
        bgfx::setIndexBuffer(&index_buffer);
        float model[16] {};
        bx::mtxIdentity(model);
        bgfx::setTransform(model);
        bgfx::submit(0U, _program);
        if (debug_trace) {
            std::uint32_t texel_rgba = 0U;
            if (quad.texture.rgba8 != nullptr and quad.texture.width > 0U and quad.texture.height > 0U) {
                texel_rgba =
                    (static_cast<std::uint32_t>(quad.texture.rgba8[0]) << 24U) |
                    (static_cast<std::uint32_t>(quad.texture.rgba8[1]) << 16U) |
                    (static_cast<std::uint32_t>(quad.texture.rgba8[2]) << 8U) |
                    static_cast<std::uint32_t>(quad.texture.rgba8[3]);
            }
            _logger->debug(
                __FILE__,
                __LINE__,
                logging::Category::RENDERER,
                "layout submit quad x=[{:.2f},{:.2f}] y=[{:.2f},{:.2f}] uv=[{:.3f},{:.3f}]..[{:.3f},{:.3f}] tex={} {}x{} c={:08x}/{:08x}/{:08x}/{:08x} texel_rgba={:08x}",
                quad.x0,
                quad.x1,
                quad.y0,
                quad.y1,
                quad.u0,
                quad.v0,
                quad.u1,
                quad.v1,
                quad.texture.id,
                quad.texture.width,
                quad.texture.height,
                quad.color_tl,
                quad.color_tr,
                quad.color_bl,
                quad.color_br,
                texel_rgba);
        }
    };

    if (debug_force_touch) {
        bgfx::touch(0U);
    }

    if (debug_solid_quad) {
        draw_quad(QuadCommand {
            .x0 = 0.0F,
            .y0 = 0.0F,
            .x1 = static_cast<float>(framebuffer_width),
            .y1 = static_cast<float>(framebuffer_height),
            .u0 = 0.0F,
            .v0 = 0.0F,
            .u1 = 1.0F,
            .v1 = 1.0F,
            .color_tl = 0xFF0000FFU,
            .color_tr = 0xFF0000FFU,
            .color_bl = 0xFF0000FFU,
            .color_br = 0xFF0000FFU,
            .texture = TextureRef {
                .id = 0U,
                .rgba8 = nullptr,
                .width = 0U,
                .height = 0U,
            },
        });
        return;
    }

    const bool debug_touch_only = [] {
        const char *value = std::getenv("SMGPC_DEBUG_TOUCH_ONLY");
        return value != nullptr && value[0] != '\0' && value[0] != '0';
    }();
    if (debug_touch_only) {
        bgfx::touch(0U);
        return;
    }

    if (draw_list.quads().empty()) {
        bgfx::touch(0U);
        return;
    }

    for (const auto &quad : draw_list.quads()) {
        draw_quad(quad);
    }
}

}  // namespace smgpc::render::layout
