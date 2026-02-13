#include "ServiceProvider.hpp"
#include "common/Logger.hpp"
#include "render/RenderWindow.hpp"
#include "tests/TestHarness.hpp"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

class NullLogger final : public smgpc::logging::ILogger {
public:
    void write(
        std::FILE *,
        std::string_view,
        int,
        smgpc::logging::Level,
        smgpc::logging::Category,
        std::string_view) override {
    }
};

class FakeInputSnapshot final : public smgpc::render::IInputSnapshot {
public:
    void set_key_state(int key, bool down) {
        if (key < 0 || key >= static_cast<int>(_keys.size())) {
            return;
        }
        _keys[static_cast<std::size_t>(key)] = down;
    }

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

    FakeInputSnapshot &snapshot() {
        return _snapshot;
    }

private:
    mutable FakeInputSnapshot _snapshot {};
};

class FakeWindow final : public smgpc::render::IWindowService {
public:
    explicit FakeWindow(std::vector<bool> poll_results)
        : _poll_results(std::move(poll_results)) {
    }

    bool poll_events() override {
        if (_poll_index >= _poll_results.size()) {
            return false;
        }
        return _poll_results[_poll_index++];
    }

    [[nodiscard]] bool should_close() const override {
        return _poll_index >= _poll_results.size();
    }

    [[nodiscard]] bool is_focused() const override {
        return true;
    }

    [[nodiscard]] bool is_minimized() const override {
        return false;
    }

    [[nodiscard]] smgpc::render::FramebufferInfo framebuffer_size() const override {
        return {640U, 480U};
    }

    [[nodiscard]] smgpc::render::NativeWindowHandle native_handle() const override {
        return {nullptr, nullptr};
    }

private:
    std::vector<bool> _poll_results {};
    std::size_t _poll_index {};
};

}  // namespace

$test("Render::create_default_window_factory returns factory") {
    NullLogger logger {};
    auto factory = smgpc::render::create_default_window_factory(smgpc::di::DependencyReference<smgpc::logging::ILogger>{logger});

    $pc_port_require(factory != nullptr);
}

$test("Render::create_default_input_service requires platform-specific window type") {
    NullLogger logger {};
    auto window = FakeWindow(std::vector<bool> {});
    bool threw = false;

    try {
        (void)smgpc::render::create_default_input_service(
            smgpc::di::DependencyReference<smgpc::render::IWindowService>{window},
            smgpc::di::DependencyReference<smgpc::logging::ILogger>{logger});
    } catch (const std::invalid_argument &) {
        threw = true;
    }

    $pc_port_require(threw);
}

$test("Render::create_default_renderer_engine builds and proxies window state") {
    NullLogger logger {};
    FakeWindow window(std::vector<bool> {});
    FakeInputService input_service {};

    auto engine = smgpc::render::create_default_renderer_engine(
        smgpc::di::DependencyReference<smgpc::render::IWindowService>{window},
        smgpc::di::DependencyReference<smgpc::render::IInputService>{input_service},
        smgpc::di::DependencyReference<smgpc::logging::ILogger>{logger});

    $pc_port_require(engine != nullptr);
    const auto framebuffer = engine->framebuffer_size();
    $pc_port_require_eq(framebuffer.width, static_cast<std::uint16_t>(640));
    $pc_port_require_eq(framebuffer.height, static_cast<std::uint16_t>(480));
    $pc_port_require(not engine->poll_completed_capture().has_value());
}
