#pragma once

#include <memory>
#include <string>

#include "Logger.hpp"
#include "RendererService.hpp"
#include "ServiceProvider.hpp"

namespace smgpc::app {

struct BootstrapConfiguration {
    int window_width = 800;
    int window_height = 600;
    std::string window_title = "SMG PC Port";
};

class IApplication {
public:
    virtual ~IApplication() = default;
    [[nodiscard]] virtual int run() = 0;
};

struct ServiceGraphOverrides {
    std::unique_ptr<logging::ILogger> logger = {};
    std::unique_ptr<render::IWindowFactory> window_factory = {};
    std::unique_ptr<render::IWindowService> window_service = {};
    std::unique_ptr<render::IRendererEngine> renderer_engine = {};
    std::unique_ptr<IApplication> application = {};
};

using ServiceGraph = di::ServiceProvider<
    di::SingletonService<logging::ILogger>,
    di::SingletonService<render::IWindowFactory>,
    di::SingletonService<render::IWindowService>,
    di::SingletonService<render::IRendererEngine>,
    di::SingletonService<IApplication>
>;

[[nodiscard]] ServiceGraph build_service_graph(const BootstrapConfiguration &configuration);
[[nodiscard]] ServiceGraph build_service_graph(const BootstrapConfiguration &configuration, ServiceGraphOverrides &&overrides);

}  // namespace smgpc::app
