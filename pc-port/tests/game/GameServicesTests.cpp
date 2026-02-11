#include "common/Logger.hpp"
#include "game/GameServices.hpp"
#include "render/RenderWindow.hpp"
#include "tests/TestHarness.hpp"

#include <memory>
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

    CountingRenderer renderer_instance {};
    int render_frame_calls {};

private:
    std::vector<bool> _poll_results {};
    std::size_t _poll_index {};
};

}  // namespace

$test("Game::create_default_game_service rejects null dependencies") {
    auto logger = std::make_shared<RecordingLogger>();
    auto renderer_service = std::make_shared<FakeRendererService>(std::vector<bool> {});
    bool threw_for_renderer = false;
    bool threw_for_logger = false;

    try {
        (void)smgpc::game::create_default_game_service(nullptr, logger);
    } catch (const std::invalid_argument &) {
        threw_for_renderer = true;
    }

    try {
        (void)smgpc::game::create_default_game_service(renderer_service, nullptr);
    } catch (const std::invalid_argument &) {
        threw_for_logger = true;
    }

    $pc_port_require(threw_for_renderer);
    $pc_port_require(threw_for_logger);
}

$test("Game::run executes frame loop and logs lifecycle") {
    auto logger = std::make_shared<RecordingLogger>();
    auto renderer_service = std::make_shared<FakeRendererService>(std::vector<bool> {true, true, false});
    auto game = smgpc::game::create_default_game_service(renderer_service, logger);

    const int run_result = game->run();

    $pc_port_require_eq(run_result, 0);
    $pc_port_require_eq(renderer_service->render_frame_calls, 2);
    $pc_port_require_eq(renderer_service->renderer_instance.enter_calls, 2);
    $pc_port_require_eq(renderer_service->renderer_instance.draw_calls, 2);
    $pc_port_require_eq(renderer_service->renderer_instance.exit_calls, 2);

    $pc_port_require_eq(logger->entries.size(), static_cast<std::size_t>(2));
    $pc_port_require(logger->entries[0].level == smgpc::logging::Level::INFO);
    $pc_port_require(logger->entries[0].category == smgpc::logging::Category::GAME);
    $pc_port_require(logger->entries[0].message.find("Starting game loop") != std::string::npos);
    $pc_port_require(logger->entries[1].level == smgpc::logging::Level::INFO);
    $pc_port_require(logger->entries[1].category == smgpc::logging::Category::GAME);
    $pc_port_require(logger->entries[1].message.find("Exiting game loop") != std::string::npos);
}
