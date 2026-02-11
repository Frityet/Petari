#include "LayoutBgfxRenderer.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

#include <bgfx/bgfx.h>
#include <bx/math.h>

#include "Logger.hpp"

namespace smgpc::render::layout {

bgfx::VertexLayout LayoutBgfxRenderer::Vertex::layout {};

void LayoutBgfxRenderer::Vertex::init_layout() {
    layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();
}

LayoutBgfxRenderer::LayoutBgfxRenderer(std::shared_ptr<logging::ILogger> logger)
    : _logger(std::move(logger)) {
    if (not _logger) {
        throw std::invalid_argument("LayoutBgfxRenderer requires a logger.");
    }
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
    const std::array<std::filesystem::path, 8> candidates {
        std::filesystem::current_path() / "build" / "linux" / "x86_64" / "debug" / "shaders" / "glsl",
        std::filesystem::current_path() / "build" / "linux" / "x86_64" / "release" / "shaders" / "glsl",
        std::filesystem::current_path() / "pc-port" / "build" / "linux" / "x86_64" / "debug" / "shaders" / "glsl",
        std::filesystem::current_path() / "pc-port" / "build" / "linux" / "x86_64" / "release" / "shaders" / "glsl",
        std::filesystem::current_path() / "shaders" / "glsl",
        std::filesystem::current_path().parent_path() / "build" / "linux" / "x86_64" / "debug" / "shaders" / "glsl",
        std::filesystem::current_path().parent_path() / "build" / "linux" / "x86_64" / "release" / "shaders" / "glsl",
        std::filesystem::current_path().parent_path() / "shaders" / "glsl",
    };

    for (const auto &candidate : candidates) {
        if (std::filesystem::exists(candidate / "vs_layout.bin") and std::filesystem::exists(candidate / "fs_layout.bin")) {
            return candidate;
        }
    }

    throw std::runtime_error("Failed to locate bgfx layout shader binaries (vs_layout.bin/fs_layout.bin).");
}

void LayoutBgfxRenderer::ensure_initialized() {
    if (_initialized) {
        return;
    }

    Vertex::init_layout();

    const auto shader_dir = resolve_shader_directory();
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

void LayoutBgfxRenderer::draw(const LayoutDrawList &draw_list, std::uint16_t framebuffer_width, std::uint16_t framebuffer_height) {
    ensure_initialized();

    if (framebuffer_width == 0U or framebuffer_height == 0U) {
        return;
    }

    float ortho[16] {};
    bx::mtxOrtho(
        ortho,
        0.0F,
        static_cast<float>(framebuffer_width),
        static_cast<float>(framebuffer_height),
        0.0F,
        0.0F,
        1000.0F,
        0.0F,
        bgfx::getCaps()->homogeneousDepth);

    bgfx::setViewTransform(0U, nullptr, ortho);

    constexpr std::uint64_t DRAW_STATE =
        BGFX_STATE_WRITE_RGB |
        BGFX_STATE_WRITE_A |
        BGFX_STATE_BLEND_ALPHA |
        BGFX_STATE_MSAA;

    for (const auto &quad : draw_list.quads()) {
        bgfx::TransientVertexBuffer vertex_buffer {};
        bgfx::TransientIndexBuffer index_buffer {};

        constexpr std::uint32_t VERTEX_COUNT = 4U;
        constexpr std::uint32_t INDEX_COUNT = 6U;
        if (bgfx::getAvailTransientVertexBuffer(VERTEX_COUNT, Vertex::layout) < VERTEX_COUNT ||
            bgfx::getAvailTransientIndexBuffer(INDEX_COUNT) < INDEX_COUNT) {
            continue;
        }

        bgfx::allocTransientVertexBuffer(&vertex_buffer, VERTEX_COUNT, Vertex::layout);
        bgfx::allocTransientIndexBuffer(&index_buffer, INDEX_COUNT);

        auto *vertices = reinterpret_cast<Vertex *>(vertex_buffer.data);
        vertices[0] = Vertex {.x = quad.x0, .y = quad.y0, .z = 0.0F, .abgr = quad.color_tl, .u = quad.u0, .v = quad.v0};
        vertices[1] = Vertex {.x = quad.x1, .y = quad.y0, .z = 0.0F, .abgr = quad.color_tr, .u = quad.u1, .v = quad.v0};
        vertices[2] = Vertex {.x = quad.x0, .y = quad.y1, .z = 0.0F, .abgr = quad.color_bl, .u = quad.u0, .v = quad.v1};
        vertices[3] = Vertex {.x = quad.x1, .y = quad.y1, .z = 0.0F, .abgr = quad.color_br, .u = quad.u1, .v = quad.v1};

        auto *indices = reinterpret_cast<std::uint16_t *>(index_buffer.data);
        indices[0] = 0U;
        indices[1] = 1U;
        indices[2] = 2U;
        indices[3] = 1U;
        indices[4] = 3U;
        indices[5] = 2U;

        const auto texture_handle = resolve_texture(quad.texture);

        bgfx::setTexture(0U, _sampler, texture_handle);
        bgfx::setState(DRAW_STATE);
        bgfx::setVertexBuffer(0U, &vertex_buffer);
        bgfx::setIndexBuffer(&index_buffer);
        bgfx::submit(0U, _program);
    }
}

}  // namespace smgpc::render::layout
