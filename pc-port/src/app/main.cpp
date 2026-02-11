#include "Application.hpp"
#include "Logger.hpp"

#include <exception>
#include <filesystem>
#include <utility>

namespace {

[[nodiscard]] std::filesystem::path discover_game_root(std::filesystem::path start) {
    auto cursor = std::move(start);
    for (int i = 0; i < 12; ++i) {
        if (std::filesystem::exists(cursor/"orig")) {
            return cursor;
        }

        if (not cursor.has_parent_path()) {
            break;
        }
        cursor = cursor.parent_path();
    }

    return std::filesystem::current_path();
}

}  // namespace

int main() try {
    const auto game_root = discover_game_root(std::filesystem::current_path());

    smgpc::app::BootstrapConfiguration configuration {
        .window_width = 800, .window_height = 600, .window_title = "SMG PC Port", .game_root = game_root, .asset_cache_root = game_root/"pc-port"/".cache"/"assets", .game_version = "RMGK01", .language = "KrKorean"
    };

    auto services = smgpc::app::build_service_graph(configuration);
    auto logger = services.resolve_shared<smgpc::logging::ILogger>();
    logger->info(__FILE__, __LINE__, smgpc::logging::Category::APP, "Started app at {}", game_root.string());

    auto &application = services.resolve<smgpc::app::IApplication>();
    return application.run();
} catch (const std::exception &e) {
    auto fallback_logger = smgpc::logging::create_default_logger();
    fallback_logger->fatal(__FILE__, __LINE__, smgpc::logging::Category::APP, "Uncaught exception {}", e.what());
    return 1;
}
