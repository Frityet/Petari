#include "Application.hpp"
#include "Logger.hpp"

#include <array>
#include <exception>
#include <filesystem>
#include <optional>
#include <utility>
#if defined(__linux__)
#include <unistd.h>
#endif

namespace {

[[nodiscard]] std::optional<std::filesystem::path> discover_game_root(std::filesystem::path start) {
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

    return std::nullopt;
}

[[nodiscard]] std::optional<std::filesystem::path> resolve_executable_directory() {
#if defined(__linux__)
    std::array<char, 4096> buffer {};
    const auto bytes = ::readlink("/proc/self/exe", buffer.data(), buffer.size() - 1U);
    if (bytes <= 0) {
        return std::nullopt;
    }

    buffer[static_cast<std::size_t>(bytes)] = '\0';
    auto executable_path = std::filesystem::path(buffer.data());
    if (executable_path.has_parent_path()) {
        return executable_path.parent_path();
    }
#endif

    return std::nullopt;
}

[[nodiscard]] std::filesystem::path resolve_game_root() {
    if (const auto executable_directory = resolve_executable_directory(); executable_directory.has_value()) {
        if (const auto found = discover_game_root(*executable_directory); found.has_value()) {
            return *found;
        }
    }

    if (const auto found = discover_game_root(std::filesystem::current_path()); found.has_value()) {
        return *found;
    }

    return std::filesystem::current_path();
}

}  // namespace

int main() try {
    const auto game_root = resolve_game_root();

    smgpc::app::BootstrapConfiguration configuration {
        .window_width = 800, .window_height = 600, .window_title = "SMG PC Port", .game_root = game_root, .asset_cache_root = game_root/"pc-port"/".cache"/"assets", .game_version = "RMGK01", .language = "KrKorean"
    };

    auto services = smgpc::app::build_service_graph(configuration);
    auto &logger = services.get<smgpc::logging::ILogger>();
    logger.info(__FILE__, __LINE__, smgpc::logging::Category::APP, "Started app at {}", game_root.string());

    auto &application = services.get<smgpc::app::IApplication>();
    return application.run();
} catch (const std::exception &e) {
    auto fallback_logger = smgpc::logging::create_default_logger();
    fallback_logger->fatal(__FILE__, __LINE__, smgpc::logging::Category::APP, "Uncaught exception {}", e.what());
    return 1;
}
