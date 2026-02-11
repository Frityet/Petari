#include "GameServices.hpp"

#include <memory>
#include <stdexcept>
#include <utility>

#include "Logger.hpp"
#include "RenderWindow.hpp"

namespace smgpc::game {
namespace {

class DesktopGame final : public IGame {
public:
    DesktopGame(std::shared_ptr<render::IRendererService> renderer_service, std::shared_ptr<logging::ILogger> logger)
        : _renderer_service(std::move(renderer_service)), _logger(std::move(logger)) {
        if (not _renderer_service or not _logger) {
            throw std::invalid_argument("DesktopGame requires non-null injected services.");
        }
    }

    [[nodiscard]] int run() override {
        _logger->info(__FILE__, __LINE__, logging::Category::GAME, "Starting game loop");

        while (_renderer_service->poll_events()) {
            _renderer_service->render_frame();

            auto &renderer = _renderer_service->renderer();
            renderer.on_frame_enter();
            renderer.draw();
            renderer.on_frame_exit();
        }

        _logger->info(__FILE__, __LINE__, logging::Category::GAME, "Exiting game loop");
        return 0;
    }

private:
    std::shared_ptr<render::IRendererService> _renderer_service {};
    std::shared_ptr<logging::ILogger> _logger {};
};

}  // namespace

std::shared_ptr<IGame> create_default_game_service(std::shared_ptr<render::IRendererService> renderer_service, std::shared_ptr<logging::ILogger> logger) {
    return std::make_shared<DesktopGame>(std::move(renderer_service), std::move(logger));
}

}  // namespace smgpc::game
