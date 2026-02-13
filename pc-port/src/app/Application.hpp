#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "AssetServices.hpp"
#include "GameAssetService.hpp"
#include "GameServices.hpp"
#include "Logger.hpp"
#include "RenderWindow.hpp"
#include "ServiceProvider.hpp"

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
    std::unique_ptr<logging::ILogger> logger {};
    std::unique_ptr<render::IWindowFactory> window_factory {};
    std::unique_ptr<render::IRendererService> renderer_service {};
    std::unique_ptr<assets::IAssetLocator> asset_locator {};
    std::unique_ptr<assets::IAssetLoader> asset_loader {};
    std::unique_ptr<assets::IAssetConverter> asset_converter {};
    std::unique_ptr<assets::IAssetManager> asset_manager {};
    std::unique_ptr<assets::IGameAssetService> game_asset_service {};
    std::unique_ptr<game::IGame> game {};
    std::unique_ptr<IApplication> application {};
};

using ServiceGraph = di::ServiceProvider<
    di::SingletonService<logging::ILogger>,
    di::SingletonService<render::IWindowFactory>,
    di::SingletonService<render::IRendererService>,
    di::SingletonService<assets::IAssetLocator>,
    di::SingletonService<assets::IAssetLoader>,
    di::SingletonService<assets::IAssetConverter>,
    di::SingletonService<assets::IAssetManager>,
    di::SingletonService<assets::IGameAssetService>,
    di::SingletonService<game::IGame>,
    di::SingletonService<IApplication>>;

[[nodiscard]] ServiceGraph build_service_graph(const BootstrapConfiguration &configuration);
[[nodiscard]] ServiceGraph build_service_graph(const BootstrapConfiguration &configuration, ServiceGraphOverrides &&overrides);

}  // namespace smgpc::app
