#pragma once

#include <memory>

#include "ServiceProvider.hpp"
#include "core/IRenderContext.hpp"
#include "core/RenderCommandBuffer.hpp"

namespace smgpc::logging {
class ILogger;
}

namespace smgpc::render {

using FrameContext = core::FrameContext;
using FramebufferInfo = core::FramebufferInfo;
using CursorPosition = core::CursorPosition;
using NativeWindowHandle = core::NativeWindowHandle;
using RenderCaptureRequest = core::RenderCaptureRequest;
using RenderCommandBuffer = core::RenderCommandBuffer;

using IWindowService = core::IWindowService;
using IInputService = core::IInputService;
using IInputSnapshot = core::IInputSnapshot;
using IRendererEngine = core::IRendererEngine;
using WindowConfiguration = core::WindowConfiguration;

class IWindowFactory {
public:
    virtual ~IWindowFactory() = default;
    virtual std::unique_ptr<IWindowService> create(const WindowConfiguration &configuration) const = 0;
};

[[nodiscard]] std::unique_ptr<IWindowFactory> create_default_window_factory(
    di::DependencyReference<logging::ILogger> logger);

[[nodiscard]] std::unique_ptr<IInputService> create_default_input_service(
    di::DependencyReference<IWindowService> window_service,
    di::DependencyReference<logging::ILogger> logger);

[[nodiscard]] std::unique_ptr<IRendererEngine> create_default_renderer_engine(
    di::DependencyReference<IWindowService> window_service,
    di::DependencyReference<IInputService> input_service,
    di::DependencyReference<logging::ILogger> logger);

}  // namespace smgpc::render
