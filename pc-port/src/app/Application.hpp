#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "AssetServices.hpp"
#include "GameAssetService.hpp"
#include "GameServices.hpp"
#include "Logger.hpp"
#include "RenderWindow.hpp"
#include "ServiceContainer.hpp"

namespace smgpc::app {

struct BootstrapConfiguration {
    int window_width {800};
    int window_height {600};
    std::string window_title {"SMG PC Port"};
    std::filesystem::path game_root {};
    std::filesystem::path asset_cache_root {};
    std::string game_version {"RMGK01"};
    std::string language {"KrKorean"};
    std::vector<assets::AssetId> startup_assets {};
};

class IApplication {
public:
    virtual ~IApplication() = default;
    [[nodiscard]] virtual int run() = 0;
};

struct ServiceGraphOverrides {
    std::shared_ptr<logging::ILogger> logger {};
    std::shared_ptr<render::IWindowFactory> window_factory {};
    std::shared_ptr<render::IRendererService> renderer_service {};
    std::shared_ptr<assets::IAssetLocator> asset_locator {};
    std::shared_ptr<assets::IAssetLoader> asset_loader {};
    std::shared_ptr<assets::IAssetConverter> asset_converter {};
    std::shared_ptr<assets::IAssetManager> asset_manager {};
    std::shared_ptr<assets::IGameAssetService> game_asset_service {};
    std::shared_ptr<game::IGame> game {};
    std::shared_ptr<IApplication> application {};
};

using ServiceGraph = di::ServiceContainer<
    logging::ILogger, render::IWindowFactory, render::IRendererService, assets::IAssetLocator, assets::IAssetLoader, assets::IAssetConverter, assets::IAssetManager, assets::IGameAssetService, game::IGame, IApplication>;

[[nodiscard]] ServiceGraph build_service_graph(const BootstrapConfiguration &configuration);
[[nodiscard]] ServiceGraph build_service_graph(const BootstrapConfiguration &configuration, ServiceGraphOverrides overrides);

}  // namespace smgpc::app
