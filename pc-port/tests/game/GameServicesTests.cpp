#include "common/Logger.hpp"
#include "assets/GameAssetService.hpp"
#include "game/GameServices.hpp"
#include "render/RenderWindow.hpp"
#include "tests/TestHarness.hpp"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
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

    void capture_next_frame(std::filesystem::path output_path) override {
        capture_paths.push_back(std::move(output_path));
    }

    [[nodiscard]] std::optional<std::filesystem::path> poll_completed_capture() override {
        return std::nullopt;
    }

    [[nodiscard]] bool is_key_down(int) const override {
        return false;
    }

    [[nodiscard]] std::pair<std::uint16_t, std::uint16_t> framebuffer_size() const override {
        return {320U, 240U};
    }

    CountingRenderer renderer_instance {};
    int render_frame_calls {};
    std::vector<std::filesystem::path> capture_paths {};

private:
    std::vector<bool> _poll_results {};
    std::size_t _poll_index {};
};

class FakeGameAssetService final : public smgpc::assets::IGameAssetService {
public:
    void request_load_file(std::string_view file_path) override {
        requested_files.push_back(std::string(file_path));
    }

    [[nodiscard]] std::shared_ptr<const std::vector<std::byte>> receive_file(std::string_view file_path) override {
        requested_files.push_back(std::string(file_path));
        return nullptr;
    }

    [[nodiscard]] bool is_loaded_file(std::string_view) const override {
        return false;
    }

    void request_mount_archive(std::string_view archive_path) override {
        requested_archives.push_back(std::string(archive_path));
    }

    [[nodiscard]] std::shared_ptr<const smgpc::assets::MountedArchiveData> receive_archive(std::string_view archive_path) override {
        requested_archives.push_back(std::string(archive_path));
        return nullptr;
    }

    [[nodiscard]] bool is_mounted_archive(std::string_view) const override {
        return false;
    }

    [[nodiscard]] const smgpc::assets::GameAssetPathResolver &path_resolver() const override {
        return resolver;
    }

    smgpc::assets::GameAssetPathResolver resolver {
        smgpc::assets::GameAssetPathResolverConfiguration {
            .game_root = "/tmp",
            .version = "RMGK01",
            .language = "KrKorean",
            .is_widescreen = true,
        }
    };
    std::vector<std::string> requested_files {};
    std::vector<std::string> requested_archives {};
};

}  // namespace

$test("Game::create_default_game_service rejects null dependencies") {
    auto logger = std::make_shared<RecordingLogger>();
    auto renderer_service = std::make_shared<FakeRendererService>(std::vector<bool> {});
    auto asset_service = std::make_shared<FakeGameAssetService>();
    bool threw_for_renderer = false;
    bool threw_for_assets = false;
    bool threw_for_logger = false;

    try {
        (void)smgpc::game::create_default_game_service(nullptr, asset_service, logger);
    } catch (const std::invalid_argument &) {
        threw_for_renderer = true;
    }

    try {
        (void)smgpc::game::create_default_game_service(renderer_service, nullptr, logger);
    } catch (const std::invalid_argument &) {
        threw_for_assets = true;
    }

    try {
        (void)smgpc::game::create_default_game_service(renderer_service, asset_service, nullptr);
    } catch (const std::invalid_argument &) {
        threw_for_logger = true;
    }

    $pc_port_require(threw_for_renderer);
    $pc_port_require(threw_for_assets);
    $pc_port_require(threw_for_logger);
}

$test("Game::create_default_game_service builds with generic asset service") {
    auto logger = std::make_shared<RecordingLogger>();
    auto renderer_service = std::make_shared<FakeRendererService>(std::vector<bool> {false});
    auto asset_service = std::make_shared<FakeGameAssetService>();
    auto game = smgpc::game::create_default_game_service(renderer_service, asset_service, logger);

    $pc_port_require(game != nullptr);
}
