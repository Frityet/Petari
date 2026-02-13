#pragma once

#include <memory>

#include "ServiceProvider.hpp"

namespace smgpc::logging {
    class ILogger;
}

namespace smgpc::render {
    class IRendererService;
}

namespace smgpc::assets {
    class IGameAssetService;
}

namespace smgpc::game {
    class IGame {
    public:
        virtual ~IGame() = default;
        [[nodiscard]] virtual int run() = 0;
    };

[[nodiscard]] std::unique_ptr<IGame> create_default_game_service(
    di::DependencyReference<render::IRendererService> renderer_service,
    di::DependencyReference<assets::IGameAssetService> asset_service,
    di::DependencyReference<logging::ILogger> logger);
}  // namespace smgpc::game
