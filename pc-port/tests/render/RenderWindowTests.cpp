#include "common/Logger.hpp"
#include "render/RenderWindow.hpp"
#include "tests/TestHarness.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

class NullLogger final : public smgpc::logging::ILogger {
public:
    void write(std::FILE *, std::string_view, int, smgpc::logging::Level, smgpc::logging::Category, std::string_view) override {
    }
};

class FakeRenderer final : public smgpc::render::Renderer {
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

class FakeWindow final : public smgpc::render::Window {
public:
    explicit FakeWindow(std::vector<bool> poll_results)
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
        capture_requests.push_back(std::move(output_path));
    }

    [[nodiscard]] std::optional<std::filesystem::path> poll_completed_capture() override {
        if (completed_captures.empty()) {
            return std::nullopt;
        }

        auto output_path = completed_captures.front();
        completed_captures.erase(completed_captures.begin());
        return output_path;
    }

    [[nodiscard]] bool is_key_down(int key) const override {
        return key == 10;
    }

    [[nodiscard]] std::pair<std::uint16_t, std::uint16_t> framebuffer_size() const override {
        return {640U, 480U};
    }

    FakeRenderer renderer_instance {};
    int render_frame_calls {};
    std::vector<std::filesystem::path> capture_requests {};
    std::vector<std::filesystem::path> completed_captures {};

private:
    std::vector<bool> _poll_results {};
    std::size_t _poll_index {};
};

class FakeWindowFactory final : public smgpc::render::IWindowFactory {
public:
    explicit FakeWindowFactory(std::unique_ptr<smgpc::render::Window> window)
        : _window(std::move(window)) {
    }

    [[nodiscard]] std::unique_ptr<smgpc::render::Window> create(const smgpc::render::WindowConfiguration &configuration) const override {
        ++create_calls;
        last_width = configuration.width;
        last_height = configuration.height;
        last_title = configuration.title;
        return std::move(_window);
    }

    mutable int create_calls {};
    mutable int last_width {};
    mutable int last_height {};
    mutable std::string last_title {};

private:
    mutable std::unique_ptr<smgpc::render::Window> _window {};
};

}  // namespace

$test("Render::create_default_window_factory requires logger") {
    bool threw = false;

    try {
        (void)smgpc::render::create_default_window_factory(nullptr);
    } catch (const std::invalid_argument &) {
        threw = true;
    }

    $pc_port_require(threw);
}

$test("Render::create_default_window_factory returns factory") {
    auto logger = std::make_shared<NullLogger>();
    auto factory = smgpc::render::create_default_window_factory(logger);

    $pc_port_require(factory != nullptr);
}

$test("Render::create_default_renderer_service requires non-null dependencies") {
    auto logger = std::make_shared<NullLogger>();
    bool threw_for_factory = false;
    bool threw_for_logger = false;

    try {
        (void)smgpc::render::create_default_renderer_service(nullptr, smgpc::render::WindowConfiguration {.width = 1, .height = 1, .title = "x"}, logger);
    } catch (const std::invalid_argument &) {
        threw_for_factory = true;
    }

    auto fake_factory = std::make_shared<FakeWindowFactory>(std::make_unique<FakeWindow>(std::vector<bool> {false}));
    try {
        (void)smgpc::render::create_default_renderer_service(fake_factory, smgpc::render::WindowConfiguration {.width = 1, .height = 1, .title = "x"}, nullptr);
    } catch (const std::invalid_argument &) {
        threw_for_logger = true;
    }

    $pc_port_require(threw_for_factory);
    $pc_port_require(threw_for_logger);
}

$test("Render::create_default_renderer_service throws when factory returns null window") {
    auto logger = std::make_shared<NullLogger>();
    auto null_window_factory = std::make_shared<FakeWindowFactory>(nullptr);
    bool threw = false;

    try {
        (void)smgpc::render::create_default_renderer_service(null_window_factory, smgpc::render::WindowConfiguration {.width = 800, .height = 600, .title = "null-window"}, logger);
    } catch (const std::runtime_error &) {
        threw = true;
    }

    $pc_port_require(threw);
}

$test("Render renderer service delegates window behavior") {
    auto logger = std::make_shared<NullLogger>();
    auto *window_ptr = new FakeWindow(std::vector<bool> {true, false});
    auto fake_window = std::unique_ptr<smgpc::render::Window>(window_ptr);
    auto fake_factory = std::make_shared<FakeWindowFactory>(std::move(fake_window));

    auto service = smgpc::render::create_default_renderer_service(fake_factory, smgpc::render::WindowConfiguration {.width = 320, .height = 240, .title = "UnitTest"}, logger);

    $pc_port_require(service != nullptr);
    $pc_port_require_eq(fake_factory->create_calls, 1);
    $pc_port_require_eq(fake_factory->last_width, 320);
    $pc_port_require_eq(fake_factory->last_height, 240);
    $pc_port_require_eq(fake_factory->last_title, std::string("UnitTest"));

    $pc_port_require(service->poll_events());
    $pc_port_require(not service->poll_events());
    service->render_frame();
    service->render_frame();
    $pc_port_require_eq(window_ptr->render_frame_calls, 2);
    service->capture_next_frame("captures/frame_000001.ppm");
    $pc_port_require_eq(window_ptr->capture_requests.size(), static_cast<std::size_t>(1));
    $pc_port_require_eq(window_ptr->capture_requests.front(), std::filesystem::path("captures/frame_000001.ppm"));

    window_ptr->completed_captures.push_back("captures/frame_000001.ppm");
    const auto completed_capture = service->poll_completed_capture();
    $pc_port_require(completed_capture.has_value());
    $pc_port_require_eq(*completed_capture, std::filesystem::path("captures/frame_000001.ppm"));
    $pc_port_require(not service->poll_completed_capture().has_value());

    auto &renderer = service->renderer();
    renderer.on_frame_enter();
    renderer.draw();
    renderer.on_frame_exit();

    $pc_port_require(service->is_key_down(10));
    $pc_port_require(not service->is_key_down(11));
    const auto [width, height] = service->framebuffer_size();
    $pc_port_require_eq(width, static_cast<std::uint16_t>(640));
    $pc_port_require_eq(height, static_cast<std::uint16_t>(480));

    $pc_port_require_eq(window_ptr->renderer_instance.enter_calls, 1);
    $pc_port_require_eq(window_ptr->renderer_instance.draw_calls, 1);
    $pc_port_require_eq(window_ptr->renderer_instance.exit_calls, 1);
}
