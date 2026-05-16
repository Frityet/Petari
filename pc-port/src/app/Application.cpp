#include "Application.hpp"

#include <memory>
#include <utility>

namespace smgpc::app {
namespace {

class DesktopApplication final : public IApplication {
public:
    DesktopApplication(di::DependencyReference<render::IWindowService> window_service, di::DependencyReference<render::IRendererEngine> renderer_engine, di::DependencyReference<logging::ILogger> logger)
        : _window_service(std::move(window_service)),
          _renderer_engine(std::move(renderer_engine)),
          _logger(std::move(logger)) {
    }

    [[nodiscard]] int run() override {
        _logger->info(logging::Category::APP, logging::Message {"Running blank BGFX window"});

        while (_window_service->poll_events()) {
            (void)_renderer_engine->begin_frame();
            _renderer_engine->end_frame();
        }

        return 0;
    }

private:
    di::DependencyReference<render::IWindowService> _window_service;
    di::DependencyReference<render::IRendererEngine> _renderer_engine;
    di::DependencyReference<logging::ILogger> _logger;
};

}  // namespace

ServiceGraph build_service_graph(const BootstrapConfiguration &configuration) {
    return build_service_graph(configuration, ServiceGraphOverrides {});
}

ServiceGraph build_service_graph(const BootstrapConfiguration &configuration, ServiceGraphOverrides &&overrides) {
    ServiceGraph graph {};

    if (overrides.logger) {
        graph.register_service<di::SingletonService<logging::ILogger>>(std::move(overrides.logger));
    } else {
        graph.register_service<di::SingletonService<logging::ILogger>>(logging::create_default_logger());
    }

    if (overrides.window_factory) {
        graph.register_service<di::SingletonService<render::IWindowFactory>>(std::move(overrides.window_factory));
    } else {
        graph.register_service<di::SingletonService<render::IWindowFactory>, logging::ILogger>([](di::DependencyReference<logging::ILogger> logger) {
            return render::create_default_window_factory(std::move(logger));
        });
    }

    if (overrides.window_service) {
        graph.register_service<di::SingletonService<render::IWindowService>>(std::move(overrides.window_service));
    } else {
        graph.register_service<di::SingletonService<render::IWindowService>, render::IWindowFactory>([configuration](di::DependencyReference<render::IWindowFactory> window_factory) {
            return window_factory->create(render::WindowConfiguration {
                .width = configuration.window_width,
                .height = configuration.window_height,
                .title = configuration.window_title,
            });
        });
    }

    if (overrides.renderer_engine) {
        graph.register_service<di::SingletonService<render::IRendererEngine>>(std::move(overrides.renderer_engine));
    } else {
        graph.register_service<di::SingletonService<render::IRendererEngine>, render::IWindowService, logging::ILogger>(
            [](di::DependencyReference<render::IWindowService> window_service, di::DependencyReference<logging::ILogger> logger) {
                return render::create_default_renderer_engine(std::move(window_service), std::move(logger));
            });
    }

    if (overrides.application) {
        graph.register_service<di::SingletonService<IApplication>>(std::move(overrides.application));
    } else {
        graph.register_service<di::SingletonService<IApplication>, render::IWindowService, render::IRendererEngine, logging::ILogger>(
            [](di::DependencyReference<render::IWindowService> window_service, di::DependencyReference<render::IRendererEngine> renderer_engine, di::DependencyReference<logging::ILogger> logger) {
                return std::make_unique<DesktopApplication>(std::move(window_service), std::move(renderer_engine), std::move(logger));
            });
    }

    return graph;
}

}  // namespace smgpc::app
