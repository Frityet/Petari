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
    using CopyEvent = core::CopyEvent;
    using CopyEventKind = core::CopyEventKind;
    using CopyRect = core::CopyRect;
    using CopyViewport = core::CopyViewport;
    using NativeWindowHandle = core::NativeWindowHandle;
    using InputButton = core::InputButton;
    using BlendMode = core::BlendMode;
    using DepthCompare = core::DepthCompare;
    using CullMode = core::CullMode;
    using GxAlphaCompare2D = core::GxAlphaCompare2D;
    using GxBlendMode2D = core::GxBlendMode2D;
    using GxFog2D = core::GxFog2D;
    using TextureHandle = core::TextureHandle;
    using InputPointerState = core::InputPointerState;
    using GxMaterialTriangleBatch2D = core::GxMaterialTriangleBatch2D;
    using GxMaterialVertex2D = core::GxMaterialVertex2D;
    using GxTevRegisterColor2D = core::GxTevRegisterColor2D;
    using GxTevStage2D = core::GxTevStage2D;
    using GxTextureStage2D = core::GxTextureStage2D;
    using TexturedTriangleBatch2D = core::TexturedTriangleBatch2D;
    using TexturedQuad2D = core::TexturedQuad2D;
    using TexturedVertex2D = core::TexturedVertex2D;
    using PrimitiveTopology = core::PrimitiveTopology;
    using IWindowService = core::IWindowService;
    using IRendererEngine = core::IRendererEngine;
    using WindowConfiguration = core::WindowConfiguration;

    class IWindowFactory {
    public:
        virtual ~IWindowFactory() = default;
        virtual std::unique_ptr<IWindowService> create(const WindowConfiguration &configuration) const = 0;
    };

    [[nodiscard]] std::unique_ptr<IWindowFactory> create_default_window_factory(di::DependencyReference<logging::ILogger> logger);

    [[nodiscard]] std::unique_ptr<IRendererEngine> create_default_renderer_engine(di::DependencyReference<IWindowService> window_service,
                                                                                  di::DependencyReference<logging::ILogger> logger);

}  // namespace smgpc::render
