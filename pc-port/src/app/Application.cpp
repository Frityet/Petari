#include "Application.hpp"

#include <array>
#include <memory>
#include <stdexcept>
#include <utility>

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
    DesktopApplication(std::shared_ptr<game::IGame> game, std::shared_ptr<assets::IAssetManager> asset_manager, std::shared_ptr<logging::ILogger> logger)
        : _game(std::move(game)), _asset_manager(std::move(asset_manager)), _logger(std::move(logger)) {
        if (not _game or not _asset_manager or not _logger) {
            throw std::invalid_argument("DesktopApplication requires non-null injected services.");
        }
    }

    [[nodiscard]] int run() override {
        const std::array<assets::AssetId, 3> BOOTSTRAP_ASSETS {
            assets::AssetId {.logical_path = "LayoutData/PressStart.arc"}, assets::AssetId {.logical_path = "LayoutData/TitleLogo.arc"}, assets::AssetId {.logical_path = "LayoutData/Font.arc"}, };

        const auto prepare_result = _asset_manager->prepare_assets(BOOTSTRAP_ASSETS);
        if (not prepare_result) {
            _logger->warning(__FILE__, __LINE__, logging::Category::APP, "Asset bootstrap prepare failed [{}]: {}", to_error_string(prepare_result.failure().code), prepare_result.failure().message);
        } else {
            _logger->info(__FILE__, __LINE__, logging::Category::APP, "Prepared {} startup assets in cache", BOOTSTRAP_ASSETS.size());
        }

        return _game->run();
    }

private:
    std::shared_ptr<game::IGame> _game {};
    std::shared_ptr<assets::IAssetManager> _asset_manager {};
    std::shared_ptr<logging::ILogger> _logger {};
};

}  // namespace

ServiceGraph build_service_graph(const BootstrapConfiguration &configuration) {
    return build_service_graph(configuration, ServiceGraphOverrides {});
}

ServiceGraph build_service_graph(const BootstrapConfiguration &configuration, ServiceGraphOverrides overrides) {
    ServiceGraph graph {};

    graph.register_instance<logging::ILogger>(overrides.logger ? std::move(overrides.logger) : logging::create_default_logger());

    if (overrides.window_factory) {
        graph.register_instance<render::IWindowFactory>(std::move(overrides.window_factory));
    }

    if (overrides.renderer_service) {
        graph.register_instance<render::IRendererService>(std::move(overrides.renderer_service));
    }

    if (overrides.asset_locator) {
        graph.register_instance<assets::IAssetLocator>(std::move(overrides.asset_locator));
    }

    if (overrides.asset_loader) {
        graph.register_instance<assets::IAssetLoader>(std::move(overrides.asset_loader));
    }

    if (overrides.asset_converter) {
        graph.register_instance<assets::IAssetConverter>(std::move(overrides.asset_converter));
    }

    if (overrides.asset_manager) {
        graph.register_instance<assets::IAssetManager>(std::move(overrides.asset_manager));
    }

    if (overrides.game) {
        graph.register_instance<game::IGame>(std::move(overrides.game));
    }

    if (overrides.application) {
        graph.register_instance<IApplication>(std::move(overrides.application));
    }

    if (not graph.has<render::IWindowFactory>() and
        not graph.has<render::IRendererService>() and
        not graph.has<game::IGame>() and
        not graph.has<IApplication>()) {
        graph.register_factory<render::IWindowFactory>([](ServiceGraph &services) {
                return render::create_default_window_factory(services.resolve_shared<logging::ILogger>());
            });
    }

    if (not graph.has<render::IRendererService>() and
        not graph.has<game::IGame>() and
        not graph.has<IApplication>()) {
        graph.register_factory<render::IRendererService>([configuration](ServiceGraph &services) {
                return render::create_default_renderer_service(services.resolve_shared<render::IWindowFactory>(), render::WindowConfiguration {
                        .width = configuration.window_width, .height = configuration.window_height, .title = configuration.window_title
                    }, services.resolve_shared<logging::ILogger>());
            });
    }

    if (not graph.has<assets::IAssetLocator>() and
        not graph.has<assets::IAssetLoader>() and
        not graph.has<assets::IAssetManager>() and
        not graph.has<IApplication>()) {
        graph.register_type<assets::IAssetLocator, assets::FilesystemAssetLocator>(assets::AssetLocatorConfiguration {
                .game_root = configuration.game_root, .version = configuration.game_version, .language = configuration.language
            });
    }

    if (not graph.has<assets::IAssetLoader>() and
        not graph.has<assets::IAssetManager>() and
        not graph.has<IApplication>()) {
        graph.register_factory<assets::IAssetLoader>([](ServiceGraph &services) {
                return std::make_shared<assets::FilesystemAssetLoader>(services.resolve_shared<assets::IAssetLocator>());
            });
    }

    if (not graph.has<assets::IAssetConverter>() and
        not graph.has<assets::IAssetManager>() and
        not graph.has<IApplication>()) {
        graph.register_type<assets::IAssetConverter, assets::PackedAssetConverter>();
    }

    if (not graph.has<assets::IAssetManager>() and
        not graph.has<IApplication>()) {
        graph.register_factory<assets::IAssetManager>([configuration](ServiceGraph &services) {
                return std::make_shared<assets::CachingAssetManager>(services.resolve_shared<assets::IAssetLoader>(), services.resolve_shared<assets::IAssetConverter>(), assets::AssetCacheConfiguration {
                        .cache_root = configuration.asset_cache_root, .version = configuration.game_version, .language = configuration.language
                    });
            });
    }

    if (not graph.has<game::IGame>() and
        not graph.has<IApplication>()) {
        graph.register_factory<game::IGame>([](ServiceGraph &services) {
                return game::create_default_game_service(services.resolve_shared<render::IRendererService>(), services.resolve_shared<logging::ILogger>());
            });
    }

    if (not graph.has<IApplication>()) {
        graph.register_factory<IApplication>([](ServiceGraph &services) {
                return std::make_shared<DesktopApplication>(services.resolve_shared<game::IGame>(), services.resolve_shared<assets::IAssetManager>(), services.resolve_shared<logging::ILogger>());
            });
    }

    return graph;
}

}  // namespace smgpc::app
