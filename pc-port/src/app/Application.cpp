#include "Application.hpp"

#include <memory>
#include <utility>
#include <vector>

#include "Logger.hpp"

namespace smgpc::app {
namespace {

[[nodiscard]] constexpr const char *to_error_string(assets::AssetErrorCode code) {
    switch (code) {
    case assets::AssetErrorCode::NotFound:
        return "not-found";
    case assets::AssetErrorCode::IoFailure:
        return "io-failure";
    case assets::AssetErrorCode::InvalidFormat:
        return "invalid-format";
    }

    return "unknown";
}

class DesktopApplication final : public IApplication {
public:
    DesktopApplication(
        di::DependencyReference<game::IGame> game,
        di::DependencyReference<assets::IAssetManager> asset_manager,
        di::DependencyReference<logging::ILogger> logger,
        std::vector<assets::AssetId> startup_assets)
        : _game(std::move(game)),
          _asset_manager(std::move(asset_manager)),
          _logger(std::move(logger)),
          _startup_assets(std::move(startup_assets)) {
    }

    [[nodiscard]] int run() override {
        if (not _startup_assets.empty()) {
            const auto prepare_result = _asset_manager->prepare_assets(_startup_assets);
            if (not prepare_result) {
                _logger->warning(
                    __FILE__,
                    __LINE__,
                    logging::Category::APP,
                    "Asset bootstrap prepare failed [{}]: {}",
                    to_error_string(prepare_result.failure().code),
                    prepare_result.failure().message);
            } else {
                _logger->info(
                    __FILE__,
                    __LINE__,
                    logging::Category::APP,
                    "Prepared {} startup assets in cache",
                    _startup_assets.size());
            }
        }

        return _game->run();
    }

private:
    di::DependencyReference<game::IGame> _game;
    di::DependencyReference<assets::IAssetManager> _asset_manager;
    di::DependencyReference<logging::ILogger> _logger;
    std::vector<assets::AssetId> _startup_assets {};
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
    }

    if (overrides.window_service) {
        graph.register_service<di::SingletonService<render::IWindowService>>(std::move(overrides.window_service));
    }

    if (overrides.input_service) {
        graph.register_service<di::SingletonService<render::IInputService>>(std::move(overrides.input_service));
    }

    if (overrides.renderer_engine) {
        graph.register_service<di::SingletonService<render::IRendererEngine>>(std::move(overrides.renderer_engine));
    }

    if (overrides.asset_locator) {
        graph.register_service<di::SingletonService<assets::IAssetLocator>>(std::move(overrides.asset_locator));
    }

    if (overrides.asset_loader) {
        graph.register_service<di::SingletonService<assets::IAssetLoader>>(std::move(overrides.asset_loader));
    }

    if (overrides.asset_converter) {
        graph.register_service<di::SingletonService<assets::IAssetConverter>>(std::move(overrides.asset_converter));
    }

    if (overrides.asset_manager) {
        graph.register_service<di::SingletonService<assets::IAssetManager>>(std::move(overrides.asset_manager));
    }

    if (overrides.game_asset_service) {
        graph.register_service<di::SingletonService<assets::IGameAssetService>>(std::move(overrides.game_asset_service));
    }

    if (overrides.game) {
        graph.register_service<di::SingletonService<game::IGame>>(std::move(overrides.game));
    }

    if (overrides.application) {
        graph.register_service<di::SingletonService<IApplication>>(std::move(overrides.application));
    }

    if (not graph.has<render::IWindowFactory>() and not graph.has<render::IWindowService>() and not graph.has<game::IGame>() and not graph.has<IApplication>()) {
        graph.register_service<di::SingletonService<render::IWindowFactory>, logging::ILogger>([](di::DependencyReference<logging::ILogger> logger) {
            return render::create_default_window_factory(std::move(logger));
        });
    }

    if (not graph.has<render::IWindowService>() and
        not graph.has<game::IGame>() and
        not graph.has<IApplication>()) {
        graph.register_service<di::SingletonService<render::IWindowService>, render::IWindowFactory, logging::ILogger>(
            [configuration](di::DependencyReference<render::IWindowFactory> window_factory,
                di::DependencyReference<logging::ILogger> logger) {
                (void)logger;
                return window_factory->create(render::WindowConfiguration {
                    .width = configuration.window_width,
                    .height = configuration.window_height,
                    .title = configuration.window_title,
                });
            });
    }

    if (not graph.has<render::IInputService>() and
        not graph.has<game::IGame>() and
        not graph.has<IApplication>()) {
        graph.register_service<di::SingletonService<render::IInputService>, render::IWindowService, logging::ILogger>(
            [](di::DependencyReference<render::IWindowService> window_service, di::DependencyReference<logging::ILogger> logger) {
                return render::create_default_input_service(std::move(window_service), std::move(logger));
            });
    }

    if (not graph.has<render::IRendererEngine>() and
        not graph.has<game::IGame>() and
        not graph.has<IApplication>()) {
        graph.register_service<
            di::SingletonService<render::IRendererEngine>,
            render::IWindowService,
            render::IInputService,
            logging::ILogger>(
            [](di::DependencyReference<render::IWindowService> window_service,
                di::DependencyReference<render::IInputService> input_service,
                di::DependencyReference<logging::ILogger> logger) {
                return render::create_default_renderer_engine(
                    std::move(window_service),
                    std::move(input_service),
                    std::move(logger));
            });
    }

    if (not graph.has<assets::IAssetLocator>() and
        not graph.has<assets::IAssetLoader>() and
        not graph.has<assets::IAssetManager>() and
        not graph.has<IApplication>()) {
        graph.register_service<di::SingletonService<assets::IAssetLocator>>([configuration]() {
            return std::make_unique<assets::FilesystemAssetLocator>(assets::AssetLocatorConfiguration {
                .game_root = configuration.game_root,
                .version = configuration.game_version,
                .language = configuration.language,
            });
        });
    }

    if (not graph.has<assets::IAssetLoader>() and
        not graph.has<assets::IAssetManager>() and
        not graph.has<IApplication>()) {
        graph.register_service<di::SingletonService<assets::IAssetLoader>, assets::IAssetLocator>([](di::DependencyReference<assets::IAssetLocator> locator) {
            return std::make_unique<assets::FilesystemAssetLoader>(std::move(locator));
        });
    }

    if (not graph.has<assets::IAssetConverter>() and
        not graph.has<assets::IAssetManager>() and
        not graph.has<IApplication>()) {
        graph.register_service<di::SingletonService<assets::IAssetConverter>>([]() {
            return std::make_unique<assets::PackedAssetConverter>();
        });
    }

    if (not graph.has<assets::IAssetManager>() and
        not graph.has<IApplication>()) {
        graph.register_service<di::SingletonService<assets::IAssetManager>, assets::IAssetLoader, assets::IAssetConverter>(
            [configuration](di::DependencyReference<assets::IAssetLoader> asset_loader, di::DependencyReference<assets::IAssetConverter> converter) {
                return std::make_unique<assets::CachingAssetManager>(
                    std::move(asset_loader),
                    std::move(converter),
                    assets::AssetCacheConfiguration {
                        .cache_root = configuration.asset_cache_root,
                        .version = configuration.game_version,
                        .language = configuration.language,
                    });
            });
    }

    if (not graph.has<assets::IGameAssetService>() and
        not graph.has<game::IGame>() and
        not graph.has<IApplication>()) {
        graph.register_service<di::SingletonService<assets::IGameAssetService>, assets::IAssetManager, logging::ILogger>(
            [configuration](di::DependencyReference<assets::IAssetManager> asset_manager, di::DependencyReference<logging::ILogger> logger) {
                return assets::create_default_game_asset_service(
                    std::move(asset_manager),
                    assets::GameAssetPathResolverConfiguration {
                        .game_root = configuration.game_root,
                        .version = configuration.game_version,
                        .language = configuration.language,
                        .is_widescreen = static_cast<float>(configuration.window_width) / static_cast<float>(configuration.window_height == 0 ? 1 : configuration.window_height) > 1.34F,
                    },
                    std::move(logger));
            });
    }

    if (not graph.has<game::IGame>() and
        not graph.has<IApplication>()) {
        graph.register_service<
            di::SingletonService<game::IGame>,
            render::IWindowService,
            render::IInputService,
            render::IRendererEngine,
            assets::IGameAssetService,
            logging::ILogger>(
            [](di::DependencyReference<render::IWindowService> window_service,
                di::DependencyReference<render::IInputService> input_service,
                di::DependencyReference<render::IRendererEngine> renderer_engine,
                di::DependencyReference<assets::IGameAssetService> asset_service,
                di::DependencyReference<logging::ILogger> logger) {
                return game::create_default_game_service(
                    std::move(window_service),
                    std::move(input_service),
                    std::move(renderer_engine),
                    std::move(asset_service),
                    std::move(logger));
            });
    }

    if (not graph.has<IApplication>()) {
        graph.register_service<di::SingletonService<IApplication>, game::IGame, assets::IAssetManager, logging::ILogger>(
            [startup_assets = configuration.startup_assets](di::DependencyReference<game::IGame> game,
                di::DependencyReference<assets::IAssetManager> asset_manager,
                di::DependencyReference<logging::ILogger> logger) {
                return std::make_unique<DesktopApplication>(
                    std::move(game),
                    std::move(asset_manager),
                    std::move(logger),
                    std::move(startup_assets));
            });
    }

    return graph;
}

}  // namespace smgpc::app
