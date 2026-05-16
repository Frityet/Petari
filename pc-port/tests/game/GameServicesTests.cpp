#include "ServiceProvider.hpp"
#include "assets/AssetLoader.hpp"
#include "assets/GameAssetService.hpp"
#include "common/Logger.hpp"
#include "game/GameServices.hpp"
#include "render/RenderWindow.hpp"
#include "tests/TestHarness.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

class RecordingLogger final : public smgpc::logging::ILogger {
public:
    void write(std::FILE *, std::string_view, int, smgpc::logging::Level level, smgpc::logging::Category category, std::string_view message) override {
        entries.push_back({.level = level, .category = category, .message = std::string(message)});
    }

    struct LogEntry {
        smgpc::logging::Level level {};
        smgpc::logging::Category category {};
        std::string message {};
    };

    std::vector<LogEntry> entries {};
};

class FakeWindowService final : public smgpc::render::IWindowService {
public:
    bool poll_events() override {
        return false;
    }

    [[nodiscard]] bool should_close() const override {
        return false;
    }

    [[nodiscard]] bool is_focused() const override {
        return true;
    }

    [[nodiscard]] bool is_minimized() const override {
        return false;
    }

    [[nodiscard]] smgpc::render::FramebufferInfo framebuffer_size() const override {
        return {320U, 240U};
    }

    [[nodiscard]] smgpc::render::NativeWindowHandle native_handle() const override {
        return {nullptr, nullptr};
    }
};

class FakeInputSnapshot final : public smgpc::render::IInputSnapshot {
public:
    [[nodiscard]] bool is_key_down(int key) const override {
        if (key < 0 || key >= static_cast<int>(_keys.size())) {
            return false;
        }
        return _keys[static_cast<std::size_t>(key)];
    }

private:
    std::array<bool, 1024> _keys {};
};

class FakeInputService final : public smgpc::render::IInputService {
public:
    [[nodiscard]] const smgpc::render::IInputSnapshot &snapshot() const override {
        return _snapshot;
    }

private:
    FakeInputSnapshot _snapshot {};
};

class FakeRendererEngine final : public smgpc::render::IRendererEngine {
public:
    [[nodiscard]] smgpc::render::FrameContext begin_frame() override {
        ++begin_frame_calls;
        return {
            .frame_index = frame_counter++,
            .frame_time_seconds = static_cast<double>(frame_counter),
            .frame_delta_seconds = 0.0166667,
            .framebuffer = framebuffer_size(),
            .has_focus = true,
            .is_minimized = false,
            .input_snapshot = nullptr,
        };
    }

    void submit(const smgpc::render::RenderCommandBuffer &) override {
    }

    void submit(std::span<const smgpc::render::RenderCommandBuffer>) override {
    }

    void end_frame() override {
        ++end_frame_calls;
    }

    void request_capture(smgpc::render::RenderCaptureRequest request) override {
        capture_requests.push_back(std::move(request.path));
    }

    [[nodiscard]] std::optional<std::filesystem::path> poll_completed_capture() override {
        if (capture_requests.empty()) {
            return std::nullopt;
        }

        auto output_path = capture_requests.front();
        capture_requests.erase(capture_requests.begin());
        return output_path;
    }

    [[nodiscard]] smgpc::render::FramebufferInfo framebuffer_size() const override {
        return {320U, 240U};
    }

    int begin_frame_calls {};
    int end_frame_calls {};
    std::vector<std::filesystem::path> capture_requests {};

private:
    std::uint64_t frame_counter {};
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

$test("Game::create_default_game_service builds with valid dependencies") {
    auto logger = std::make_unique<RecordingLogger>();
    FakeWindowService window_service {};
    FakeInputService input_service {};
    FakeRendererEngine renderer_engine {};
    FakeGameAssetService asset_service {};

    smgpc::di::DependencyReference<smgpc::render::IWindowService> window_service_ref(window_service);
    smgpc::di::DependencyReference<smgpc::render::IInputService> input_service_ref(input_service);
    smgpc::di::DependencyReference<smgpc::render::IRendererEngine> renderer_engine_ref(renderer_engine);
    smgpc::assets::AssetLoader asset_loader{smgpc::di::DependencyReference<smgpc::assets::IGameAssetService>(asset_service)};
    smgpc::di::DependencyReference<smgpc::assets::AssetLoader> asset_loader_ref(asset_loader);
    smgpc::di::DependencyReference<smgpc::logging::ILogger> logger_ref(*logger);

    auto game = smgpc::game::create_default_game_service(
        std::move(window_service_ref),
        std::move(input_service_ref),
        std::move(renderer_engine_ref),
        std::move(asset_loader_ref),
        std::move(logger_ref));

    $pc_port_require(game != nullptr);
}
