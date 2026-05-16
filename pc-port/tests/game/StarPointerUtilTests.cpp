#include "game/Game/Screen/LayoutActor.hpp"
#include "game/Game/Util/StarPointerUtil.hpp"
#include "game/compat/RuntimeContext.hpp"
#include "game/layout/LayoutRuntimeActor.hpp"
#include "render/RendererService.hpp"
#include "tests/TestHarness.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace {

class FakeInputSnapshot final : public smgpc::render::IInputSnapshot {
public:
    void set_cursor(double x, double y) {
        _cursor = smgpc::render::core::CursorPosition {
            .x = x,
            .y = y,
        };
    }

    [[nodiscard]] bool is_key_down(int key) const override {
        if (key < 0 || key >= static_cast<int>(_keys.size())) {
            return false;
        }
        return _keys[static_cast<std::size_t>(key)];
    }

    [[nodiscard]] std::optional<smgpc::render::core::CursorPosition> cursor_position() const override {
        return _cursor;
    }

private:
    std::array<bool, 1024> _keys {};
    std::optional<smgpc::render::core::CursorPosition> _cursor {};
};

class FakeInputService final : public smgpc::render::IInputService {
public:
    [[nodiscard]] const smgpc::render::IInputSnapshot &snapshot() const override {
        return _snapshot;
    }

    void set_cursor(double x, double y) {
        _snapshot.set_cursor(x, y);
    }

private:
    mutable FakeInputSnapshot _snapshot {};
};

class FakeRendererEngine final : public smgpc::render::IRendererEngine {
public:
    [[nodiscard]] smgpc::render::FrameContext begin_frame() override {
        return {
            .frame_index = 0U,
            .frame_time_seconds = 0.0,
            .frame_delta_seconds = 0.0,
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
    }

    void request_capture(smgpc::render::RenderCaptureRequest) override {
    }

    [[nodiscard]] std::optional<std::filesystem::path> poll_completed_capture() override {
        return std::nullopt;
    }

    [[nodiscard]] smgpc::render::FramebufferInfo framebuffer_size() const override {
        return {200U, 100U};
    }
};

class TestLayoutActor final : public LayoutActor {
public:
    explicit TestLayoutActor(std::shared_ptr<smgpc::game::layout::LayoutRuntimeActor> runtime_actor)
        : LayoutActor("TestLayout", std::move(runtime_actor)) {
    }
};

[[nodiscard]] std::shared_ptr<smgpc::game::layout::LayoutRuntimeActor> make_button_actor() {
    auto resource = std::make_shared<smgpc::game::layout::LayoutArchiveData>();
    resource->layout.center_origin = false;
    resource->layout.size = {.x = 100.0F, .y = 50.0F};
    resource->layout.root_pane = 0;

    smgpc::assets::layout::PaneDefinition pane {};
    pane.type = smgpc::assets::layout::PaneType::Pane;
    pane.name = "Button";
    pane.visible = true;
    pane.location_adjust = false;
    pane.alpha = 255U;
    pane.translate = {.x = 10.0F, .y = -10.0F, .z = 0.0F};
    pane.scale = {.x = 1.0F, .y = 1.0F};
    pane.size = {.x = 50.0F, .y = 20.0F};
    resource->layout.panes.push_back(std::move(pane));

    return std::make_shared<smgpc::game::layout::LayoutRuntimeActor>(resource);
}

}  // namespace

$test("StarPointerUtil hit tests against pane bounds") {
    FakeInputService input_service {};
    FakeRendererEngine renderer_engine {};
    TestLayoutActor actor(make_button_actor());

    smgpc::game::compat::set_runtime_context(smgpc::game::compat::RuntimeContext {
        .renderer_engine = &renderer_engine,
        .input_service = &input_service,
    });

    input_service.set_cursor(40.0, 40.0);
    $pc_port_require(MR::isStarPointerPointingPane(&actor, "Button", 0, true, "weak"));

    input_service.set_cursor(180.0, 40.0);
    $pc_port_require(!MR::isStarPointerPointingPane(&actor, "Button", 0, true, "weak"));
}
