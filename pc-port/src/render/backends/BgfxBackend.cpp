#include "backends/BgfxBackend.hpp"

#include <array>
#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <string_view>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

namespace smgpc::render::backends {
namespace {

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

}  // namespace

BgfxBackend::~BgfxBackend() {
    shutdown();
}

void BgfxBackend::initialize(const core::RenderInitDesc &description) {
    if (_initialized) {
        return;
    }

    _vsync_enabled = description.enable_vsync;

    const auto try_initialize = [&](bgfx::RendererType::Enum renderer_type) {
        auto init = bgfx::Init();
        init.type = renderer_type;
        init.platformData.nwh = description.native_window_handle;
        init.platformData.ndt = description.native_display_handle;
        init.resolution.width = static_cast<std::uint32_t>(description.width);
        init.resolution.height = static_cast<std::uint32_t>(description.height);
        init.resolution.reset = _vsync_enabled ? BGFX_RESET_VSYNC : BGFX_RESET_NONE;
        return bgfx::init(init);
    };

    if (const auto requested_renderer = resolve_renderer_type_from_environment(); requested_renderer.has_value()) {
        if (not try_initialize(*requested_renderer)) {
            throw std::runtime_error("Cannot init bgfx with requested renderer type");
        }
    } else {
#if defined(__linux__)
        constexpr std::array<bgfx::RendererType::Enum, 3U> renderer_fallback {
            bgfx::RendererType::Vulkan,
            bgfx::RendererType::OpenGL,
            bgfx::RendererType::Count,
        };
#else
        constexpr std::array<bgfx::RendererType::Enum, 2U> renderer_fallback {
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
}

void BgfxBackend::shutdown() {
    if (not _initialized) {
        return;
    }

    bgfx::shutdown();
    _initialized = false;
}

void BgfxBackend::begin_frame(const core::FrameContext &frame_context) {
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

core::FramebufferInfo BgfxBackend::framebuffer_size() const {
    return {
        .width = _framebuffer_width,
        .height = _framebuffer_height,
    };
}

bool BgfxBackend::is_initialized() const {
    return _initialized;
}

}  // namespace smgpc::render::backends
