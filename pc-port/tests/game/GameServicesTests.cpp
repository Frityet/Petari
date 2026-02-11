#include "common/Logger.hpp"
#include "assets/AssetServices.hpp"
#include "game/GameServices.hpp"
#include "render/RenderWindow.hpp"
#include "tests/TestHarness.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
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

class CountingRenderer final : public smgpc::render::Renderer {
public:
    void on_frame_enter() override {
        ++enter_calls;
    }

    void draw() override {
        ++draw_calls;
    }

    void on_frame_exit() override {
        ++exit_calls;
    }

    int enter_calls {};
    int draw_calls {};
    int exit_calls {};
};

class FakeRendererService final : public smgpc::render::IRendererService {
public:
    explicit FakeRendererService(std::vector<bool> poll_results)
        : _poll_results(std::move(poll_results)) {
    }

    smgpc::render::Renderer &renderer() override {
        return renderer_instance;
    }

    bool poll_events() override {
        if (_poll_index >= _poll_results.size()) {
            return false;
        }
        return _poll_results[_poll_index++];
    }

    void render_frame() override {
        ++render_frame_calls;
    }

    [[nodiscard]] bool is_key_down(int) const override {
        return false;
    }

    [[nodiscard]] std::pair<std::uint16_t, std::uint16_t> framebuffer_size() const override {
        return {320U, 240U};
    }

    CountingRenderer renderer_instance {};
    int render_frame_calls {};

private:
    std::vector<bool> _poll_results {};
    std::size_t _poll_index {};
};

class FakeAssetManager final : public smgpc::assets::IAssetManager {
public:
    [[nodiscard]] smgpc::assets::AssetResult<smgpc::assets::CachedAssetRecord> prepare_asset(const smgpc::assets::AssetId &id) override {
        return smgpc::assets::CachedAssetRecord {.id = id};
    }

    [[nodiscard]] smgpc::assets::AssetResult<void> prepare_assets(std::span<const smgpc::assets::AssetId>) override {
        return {};
    }

    [[nodiscard]] smgpc::assets::AssetResult<std::vector<std::byte>> load_cached_asset(const smgpc::assets::AssetId &) override {
        return std::vector<std::byte> {};
    }

    [[nodiscard]] std::optional<smgpc::assets::CachedAssetRecord> find_cached_asset(const smgpc::assets::AssetId &) const override {
        return std::nullopt;
    }
};

}  // namespace

$test("Game::create_default_game_service rejects null dependencies") {
    auto logger = std::make_shared<RecordingLogger>();
    auto renderer_service = std::make_shared<FakeRendererService>(std::vector<bool> {});
    auto asset_manager = std::make_shared<FakeAssetManager>();
    bool threw_for_renderer = false;
    bool threw_for_assets = false;
    bool threw_for_logger = false;

    try {
        (void)smgpc::game::create_default_game_service(nullptr, asset_manager, logger);
    } catch (const std::invalid_argument &) {
        threw_for_renderer = true;
    }

    try {
        (void)smgpc::game::create_default_game_service(renderer_service, nullptr, logger);
    } catch (const std::invalid_argument &) {
        threw_for_assets = true;
    }

    try {
        (void)smgpc::game::create_default_game_service(renderer_service, asset_manager, nullptr);
    } catch (const std::invalid_argument &) {
        threw_for_logger = true;
    }

    $pc_port_require(threw_for_renderer);
    $pc_port_require(threw_for_assets);
    $pc_port_require(threw_for_logger);
}

$test("Game::run fails cleanly when title assets cannot load") {
    auto logger = std::make_shared<RecordingLogger>();
    auto renderer_service = std::make_shared<FakeRendererService>(std::vector<bool> {true, true, false});
    auto asset_manager = std::make_shared<FakeAssetManager>();
    auto game = smgpc::game::create_default_game_service(renderer_service, asset_manager, logger);

    const int run_result = game->run();

    $pc_port_require_eq(run_result, 1);
    $pc_port_require_eq(renderer_service->render_frame_calls, 0);
    $pc_port_require_eq(renderer_service->renderer_instance.enter_calls, 0);
    $pc_port_require_eq(renderer_service->renderer_instance.draw_calls, 0);
    $pc_port_require_eq(renderer_service->renderer_instance.exit_calls, 0);

    bool saw_start_log = false;
    bool saw_failure_log = false;
    for (const auto &entry : logger->entries) {
        if (entry.level == smgpc::logging::Level::INFO &&
            entry.category == smgpc::logging::Category::GAME &&
            entry.message.find("Starting game loop") != std::string::npos) {
            saw_start_log = true;
        }

        if (entry.level == smgpc::logging::Level::ERROR &&
            entry.category == smgpc::logging::Category::GAME &&
            entry.message.find("Failed to load title assets") != std::string::npos) {
            saw_failure_log = true;
        }
    }

    $pc_port_require(saw_start_log);
    $pc_port_require(saw_failure_log);
}
