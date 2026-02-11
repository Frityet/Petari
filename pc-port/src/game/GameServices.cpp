#include "GameServices.hpp"

#include <chrono>
#include <memory>
#include <stdexcept>
#include <utility>

#include "AssetServices.hpp"
#include "Logger.hpp"
#include "RenderWindow.hpp"
#include "layout/LayoutBgfxRenderer.hpp"
#include "layout/LayoutDrawList.hpp"
#include "title/TitleAssets.hpp"
#include "title/TitleLayoutActor.hpp"
#include "title/TitleRuntimeMR.hpp"
#include "title/TitleSequenceProduct.hpp"

namespace smgpc::game {
namespace {

[[nodiscard]] const char *asset_error_to_string(assets::AssetErrorCode code) {
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

class DesktopGame final : public IGame {
public:
    DesktopGame(
        std::shared_ptr<render::IRendererService> renderer_service,
        std::shared_ptr<assets::IAssetManager> asset_manager,
        std::shared_ptr<logging::ILogger> logger)
        : _renderer_service(std::move(renderer_service)), _asset_manager(std::move(asset_manager)), _logger(std::move(logger)) {
        if (not _renderer_service or not _asset_manager or not _logger) {
            throw std::invalid_argument("DesktopGame requires non-null injected services.");
        }
    }

    [[nodiscard]] int run() override {
        _logger->info(__FILE__, __LINE__, logging::Category::GAME, "Starting game loop");

        auto loaded_title_assets = title::load_title_assets(*_asset_manager, *_logger);
        if (not loaded_title_assets) {
            _logger->error(
                __FILE__,
                __LINE__,
                logging::Category::GAME,
                "Failed to load title assets [{}]: {}",
                asset_error_to_string(loaded_title_assets.failure().code),
                loaded_title_assets.failure().message);
            return 1;
        }

        title::TitleLayoutActor logo_layout(&loaded_title_assets->title_logo);
        title::TitleLayoutActor press_start_layout(&loaded_title_assets->press_start);
        title::TitleSequenceProduct title_sequence(&logo_layout, &press_start_layout);
        render::layout::LayoutBgfxRenderer layout_renderer(_logger);
        render::layout::LayoutDrawList draw_list {};
        draw_list.reserve(512U);

        title_sequence.appear();
        title::MR::set_input_source(_renderer_service.get(), _logger.get());

        using Clock = std::chrono::steady_clock;
        constexpr double FIXED_STEP_SECONDS = 1.0 / 60.0;
        constexpr double MAX_DELTA_SECONDS = 0.25;

        auto previous_time = Clock::now();
        double accumulator = FIXED_STEP_SECONDS;

        while (_renderer_service->poll_events()) {
            const auto now = Clock::now();
            auto delta_seconds = std::chrono::duration<double>(now - previous_time).count();
            previous_time = now;

            if (delta_seconds < 0.0) {
                delta_seconds = 0.0;
            }
            if (delta_seconds > MAX_DELTA_SECONDS) {
                delta_seconds = MAX_DELTA_SECONDS;
            }

            accumulator += delta_seconds;
            while (accumulator >= FIXED_STEP_SECONDS) {
                title::MR::begin_frame();
                title_sequence.update();
                accumulator -= FIXED_STEP_SECONDS;
            }

            draw_list.clear();
            logo_layout.append_draw_commands(&draw_list, loaded_title_assets->fonts_by_name);
            press_start_layout.append_draw_commands(&draw_list, loaded_title_assets->fonts_by_name);

            _renderer_service->render_frame();
            const auto [framebuffer_width, framebuffer_height] = _renderer_service->framebuffer_size();
            layout_renderer.draw(draw_list, framebuffer_width, framebuffer_height);

            auto &renderer = _renderer_service->renderer();
            renderer.on_frame_enter();
            renderer.draw();
            renderer.on_frame_exit();

            if (not title_sequence.is_active()) {
                _logger->info(__FILE__, __LINE__, logging::Category::GAME, "Title sequence reached Dead state");
                break;
            }
        }

        _logger->info(__FILE__, __LINE__, logging::Category::GAME, "Exiting game loop");
        return 0;
    }

private:
    std::shared_ptr<render::IRendererService> _renderer_service {};
    std::shared_ptr<assets::IAssetManager> _asset_manager {};
    std::shared_ptr<logging::ILogger> _logger {};
};

}  // namespace

std::shared_ptr<IGame> create_default_game_service(
    std::shared_ptr<render::IRendererService> renderer_service,
    std::shared_ptr<assets::IAssetManager> asset_manager,
    std::shared_ptr<logging::ILogger> logger) {
    return std::make_shared<DesktopGame>(std::move(renderer_service), std::move(asset_manager), std::move(logger));
}

}  // namespace smgpc::game
