#include "app/Application.hpp"
#include "assets/AssetServices.hpp"
#include "common/Logger.hpp"
#include "game/GameServices.hpp"
#include "tests/TestHarness.hpp"

#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct LogEntry {
    smgpc::logging::Level level {};
    smgpc::logging::Category category {};
    std::string message {};
};

class RecordingLogger final : public smgpc::logging::ILogger {
public:
    void write(std::FILE *, std::string_view, int, smgpc::logging::Level level, smgpc::logging::Category category, std::string_view message) override {
        entries.push_back(LogEntry {
            .level = level, .category = category, .message = std::string(message)
        });
    }

    std::vector<LogEntry> entries {};
};

class FakeGame final : public smgpc::game::IGame {
public:
    explicit FakeGame(int run_result)
        : _run_result(run_result) {
    }

    [[nodiscard]] int run() override {
        ++run_calls;
        return _run_result;
    }

    int run_calls {};

private:
    int _run_result {};
};

class FakeAssetManager final : public smgpc::assets::IAssetManager {
public:
    explicit FakeAssetManager(std::optional<smgpc::assets::AssetError> prepare_failure = std::nullopt)
        : _prepare_failure(std::move(prepare_failure)) {
    }

    [[nodiscard]] smgpc::assets::AssetResult<smgpc::assets::CachedAssetRecord> prepare_asset(const smgpc::assets::AssetId &id) override {
        ++prepare_asset_calls;
        prepared_asset_ids.push_back(id.logical_path);

        if (_prepare_failure.has_value()) {
            return *_prepare_failure;
        }

        return smgpc::assets::CachedAssetRecord {
            .id = id, .cached_path = {}, .conversion_profile = "fake", .source_hash = 0, .converted_size = 0
        };
    }

    [[nodiscard]] smgpc::assets::AssetResult<void> prepare_assets(std::span<const smgpc::assets::AssetId> ids) override {
        ++prepare_assets_calls;
        prepared_batch_ids.clear();
        for (const auto &id : ids) {
            prepared_batch_ids.push_back(id.logical_path);
        }

        if (_prepare_failure.has_value()) {
            return smgpc::assets::AssetResult<void>(*_prepare_failure);
        }

        return {};
    }

    [[nodiscard]] smgpc::assets::AssetResult<std::vector<std::byte>> load_cached_asset(const smgpc::assets::AssetId &) override {
        return std::vector<std::byte> {};
    }

    [[nodiscard]] std::optional<smgpc::assets::CachedAssetRecord> find_cached_asset(const smgpc::assets::AssetId &) const override {
        return std::nullopt;
    }

    int prepare_asset_calls {};
    int prepare_assets_calls {};
    std::vector<std::string> prepared_asset_ids {};
    std::vector<std::string> prepared_batch_ids {};

private:
    std::optional<smgpc::assets::AssetError> _prepare_failure {};
};

class FakeApplication final : public smgpc::app::IApplication {
public:
    explicit FakeApplication(int return_code)
        : _return_code(return_code) {
    }

    [[nodiscard]] int run() override {
        ++run_calls;
        return _return_code;
    }

    int run_calls {};

private:
    int _return_code {};
};

[[nodiscard]] smgpc::app::BootstrapConfiguration make_configuration() {
    return smgpc::app::BootstrapConfiguration {
        .window_width = 1024, .window_height = 768, .window_title = "test", .game_root = "/tmp/game", .asset_cache_root = "/tmp/cache", .game_version = "RMGK01", .language = "KrKorean"
    };
}

}  // namespace

$test("Application::build_service_graph with overrides runs and prepares assets") {
    auto logger = std::make_shared<RecordingLogger>();
    auto game = std::make_shared<FakeGame>(91);
    auto assets = std::make_shared<FakeAssetManager>();

    smgpc::app::ServiceGraphOverrides overrides {};
    overrides.logger = logger;
    overrides.game = game;
    overrides.asset_manager = assets;

    auto configuration = make_configuration();
    configuration.startup_assets = {
        smgpc::assets::AssetId {.logical_path = "LayoutData/PressStart.arc"},
        smgpc::assets::AssetId {.logical_path = "LayoutData/TitleLogo.arc"},
        smgpc::assets::AssetId {.logical_path = "LayoutData/Font.arc"},
    };

    auto services = smgpc::app::build_service_graph(configuration, overrides);

    $pc_port_require(services.resolve_shared<smgpc::logging::ILogger>() == logger);
    $pc_port_require(services.resolve_shared<smgpc::game::IGame>() == game);
    $pc_port_require(services.resolve_shared<smgpc::assets::IAssetManager>() == assets);

    const int run_result = services.resolve<smgpc::app::IApplication>().run();

    $pc_port_require_eq(run_result, 91);
    $pc_port_require_eq(game->run_calls, 1);
    $pc_port_require_eq(assets->prepare_assets_calls, 1);
    $pc_port_require_eq(assets->prepared_batch_ids.size(), static_cast<std::size_t>(3));
    $pc_port_require_eq(assets->prepared_batch_ids[0], std::string("LayoutData/PressStart.arc"));
    $pc_port_require_eq(assets->prepared_batch_ids[1], std::string("LayoutData/TitleLogo.arc"));
    $pc_port_require_eq(assets->prepared_batch_ids[2], std::string("LayoutData/Font.arc"));

    bool found_prepared_log = false;
    for (const auto &entry : logger->entries) {
        if (entry.level == smgpc::logging::Level::INFO and
            entry.category == smgpc::logging::Category::APP and
            entry.message.find("Prepared 3 startup assets in cache") != std::string::npos) {
            found_prepared_log = true;
            break;
        }
    }
    $pc_port_require(found_prepared_log);
}

$test("Application logs warning when bootstrap asset preparation fails") {
    auto logger = std::make_shared<RecordingLogger>();
    auto game = std::make_shared<FakeGame>(3);
    auto assets = std::make_shared<FakeAssetManager>(smgpc::assets::AssetError {
            .code = smgpc::assets::AssetErrorCode::IoFailure, .message = "cache write failed"
        });

    smgpc::app::ServiceGraphOverrides overrides {};
    overrides.logger = logger;
    overrides.game = game;
    overrides.asset_manager = assets;

    auto configuration = make_configuration();
    configuration.startup_assets = {
        smgpc::assets::AssetId {.logical_path = "LayoutData/PressStart.arc"},
        smgpc::assets::AssetId {.logical_path = "LayoutData/TitleLogo.arc"},
        smgpc::assets::AssetId {.logical_path = "LayoutData/Font.arc"},
    };

    auto services = smgpc::app::build_service_graph(configuration, overrides);
    const int run_result = services.resolve<smgpc::app::IApplication>().run();

    $pc_port_require_eq(run_result, 3);
    $pc_port_require_eq(game->run_calls, 1);
    $pc_port_require_eq(assets->prepare_assets_calls, 1);

    bool found_warning_log = false;
    for (const auto &entry : logger->entries) {
        if (entry.level == smgpc::logging::Level::WARNING and
            entry.category == smgpc::logging::Category::APP and
            entry.message.find("io-failure") != std::string::npos and
            entry.message.find("cache write failed") != std::string::npos) {
            found_warning_log = true;
            break;
        }
    }
    $pc_port_require(found_warning_log);
}

$test("Application overrides can replace IApplication registration") {
    auto logger = std::make_shared<RecordingLogger>();
    auto fake_app = std::make_shared<FakeApplication>(17);

    smgpc::app::ServiceGraphOverrides overrides {};
    overrides.logger = logger;
    overrides.application = fake_app;

    auto services = smgpc::app::build_service_graph(make_configuration(), overrides);
    auto app = services.resolve_shared<smgpc::app::IApplication>();

    $pc_port_require(app == fake_app);
    $pc_port_require_eq(app->run(), 17);
    $pc_port_require_eq(fake_app->run_calls, 1);
}

$test("Application skips bootstrap preparation when startup list is empty") {
    auto logger = std::make_shared<RecordingLogger>();
    auto game = std::make_shared<FakeGame>(5);
    auto assets = std::make_shared<FakeAssetManager>();

    smgpc::app::ServiceGraphOverrides overrides {};
    overrides.logger = logger;
    overrides.game = game;
    overrides.asset_manager = assets;

    auto services = smgpc::app::build_service_graph(make_configuration(), overrides);
    const int run_result = services.resolve<smgpc::app::IApplication>().run();

    $pc_port_require_eq(run_result, 5);
    $pc_port_require_eq(game->run_calls, 1);
    $pc_port_require_eq(assets->prepare_assets_calls, 0);
}
