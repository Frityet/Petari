#pragma once

#include <memory>

#include "ServiceProvider.hpp"
#include "core/IRenderContext.hpp"

namespace smgpc::logging {
    class ILogger;
}

namespace smgpc::render {

    using FrameContext = core::FrameContext;
    using FramebufferInfo = core::FramebufferInfo;
    using NativeWindowHandle = core::NativeWindowHandle;
    using InputButton = core::InputButton;
    using BlendMode = core::BlendMode;
    using DepthCompare = core::DepthCompare;
    using TextureHandle = core::TextureHandle;
    using TexturedTriangleBatch2D = core::TexturedTriangleBatch2D;
    using TexturedQuad2D = core::TexturedQuad2D;
    using TexturedVertex2D = core::TexturedVertex2D;
    using IWindowService = core::IWindowService;
    using IRendererEngine = core::IRendererEngine;
    using WindowConfiguration = core::WindowConfiguration;

    class IWindowFactory {
    public:
        virtual ~IWindowFactory() = default;
        virtual std::unique_ptr< IWindowService > create(const WindowConfiguration& configuration) const = 0;
    };

    [[nodiscard]] std::unique_ptr< IWindowFactory > create_default_window_factory(di::DependencyReference< logging::ILogger > logger);

    [[nodiscard]] std::unique_ptr< IRendererEngine > create_default_renderer_engine(di::DependencyReference< IWindowService > window_service,
                                                                                    di::DependencyReference< logging::ILogger > logger);

}  // namespace smgpc::render
