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

[[nodiscard]] std::uint64_t texture_cache_key(const TextureRef &texture) {
    const auto sampler_flags = sampler_flags_for_wrap(texture.wrap_s, texture.wrap_t);
    return texture.id ^ (static_cast<std::uint64_t>(sampler_flags) << 32U);
}

void packed_color_to_uniform(std::uint32_t color, float *pOut) {
    pOut[0U] = static_cast<float>(color & 0xFFU) / 255.0F;
    pOut[1U] = static_cast<float>((color >> 8U) & 0xFFU) / 255.0F;
    pOut[2U] = static_cast<float>((color >> 16U) & 0xFFU) / 255.0F;
    pOut[3U] = static_cast<float>((color >> 24U) & 0xFFU) / 255.0F;
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
        .add(bgfx::Attrib::TexCoord0, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord1, 3, bgfx::AttribType::Float)
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
    if (bgfx::isValid(_wrap_params)) {
        bgfx::destroy(_wrap_params);
        _wrap_params = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(_tev_color0)) {
        bgfx::destroy(_tev_color0);
        _tev_color0 = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(_tev_color1)) {
        bgfx::destroy(_tev_color1);
        _tev_color1 = BGFX_INVALID_HANDLE;
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
    _wrap_params = bgfx::createUniform("u_wrap_params", bgfx::UniformType::Vec4);
    _tev_color0 = bgfx::createUniform("u_tev_color0", bgfx::UniformType::Vec4);
    _tev_color1 = bgfx::createUniform("u_tev_color1", bgfx::UniformType::Vec4);

    const std::uint32_t white_pixel = 0xFFFFFFFFU;
    const auto *white_memory = bgfx::copy(&white_pixel, sizeof(white_pixel));
    _white_texture = bgfx::createTexture2D(1U, 1U, false, 1U, bgfx::TextureFormat::BGRA8, 0U, white_memory);

    _initialized = true;
}

bgfx::TextureHandle LayoutBgfxRenderer::resolve_texture(const TextureRef &texture) {
    if (texture.id == 0U or texture.rgba8 == nullptr or texture.width == 0U or texture.height == 0U) {
        return _white_texture;
    }

    const auto cache_key = texture_cache_key(texture);
    const auto found = _texture_cache.find(cache_key);
    if (found != _texture_cache.end()) {
        return found->second;
    }

    const auto byte_count = static_cast<std::size_t>(texture.width) * texture.height * 4U;
    const auto *memory = bgfx::copy(texture.rgba8, static_cast<std::uint32_t>(byte_count));
    const auto sampler_flags = sampler_flags_for_wrap(texture.wrap_s, texture.wrap_t);
    const auto handle = bgfx::createTexture2D(
        texture.width,
        texture.height,
        false,
        1U,
        bgfx::TextureFormat::RGBA8,
        sampler_flags,
        memory);

    if (bgfx::isValid(handle)) {
        _texture_cache.emplace(cache_key, handle);
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
        const float quad_scale_x = quad.coordinate_width > 0.0F ? static_cast<float>(framebuffer_width) / quad.coordinate_width : scale_x;
        const float quad_scale_y = quad.coordinate_height > 0.0F ? static_cast<float>(framebuffer_height) / quad.coordinate_height : scale_y;
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
        const auto mask_texture_handle = quad.use_mask_texture
            ? resolve_texture(quad.mask_texture)
            : _white_texture;

        bgfx::setTexture(0U, _sampler, texture_handle, sampler_flags_for_wrap(quad.texture.wrap_s, quad.texture.wrap_t));
        bgfx::setTexture(
            1U,
            _mask_sampler,
            mask_texture_handle,
            quad.use_mask_texture ? sampler_flags_for_wrap(quad.mask_texture.wrap_s, quad.mask_texture.wrap_t) : BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
        const float mask_params[4] = {
            quad.use_mask_texture ? 1.0F : 0.0F,
            quad.invert_mask ? 1.0F : 0.0F,
            quad.mask_uses_alpha ? 1.0F : 0.0F,
            quad.texture_color_lerp ? 2.0F : (quad.texture_alpha_only ? (quad.tev_color_scale > 1.5F ? 1.25F : 1.0F) : 0.0F),
        };
        bgfx::setUniform(_mask_params, mask_params);
        const float wrap_params[4] = {
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
