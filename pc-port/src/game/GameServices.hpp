#pragma once

#include <memory>

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

    [[nodiscard]] std::shared_ptr<IGame> create_default_game_service(
        std::shared_ptr<render::IRendererService> renderer_service,
        std::shared_ptr<assets::IGameAssetService> asset_service,
        std::shared_ptr<logging::ILogger> logger);
}  // namespace smgpc::game
