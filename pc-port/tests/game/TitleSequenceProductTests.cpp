#include "game/Game/Screen/LayoutActor.hpp"
#include "game/Game/Screen/TitleSequenceProduct.hpp"
#include "game/compat/RuntimeContext.hpp"
#include "game/layout/LayoutArchiveLoader.hpp"
#include "game/layout/LayoutRuntimeActor.hpp"
#include "render/RenderWindow.hpp"
#include "tests/TestHarness.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>

namespace {

class DummyRenderer final : public smgpc::render::Renderer {
public:
    void on_frame_enter() override {
    }

    void draw() override {
    }

    void on_frame_exit() override {
    }
};

class FakeRendererService final : public smgpc::render::IRendererService {
public:
    smgpc::render::Renderer &renderer() override {
        return _renderer;
    }

    bool poll_events() override {
        return true;
    }

    void render_frame() override {
    }

    void capture_next_frame(std::filesystem::path) override {
    }

    [[nodiscard]] std::optional<std::filesystem::path> poll_completed_capture() override {
        return std::nullopt;
    }

    [[nodiscard]] bool is_key_down(int key) const override {
        return _down_keys.contains(key);
    }

    [[nodiscard]] std::pair<std::uint16_t, std::uint16_t> framebuffer_size() const override {
        return {640U, 480U};
    }

    void set_key_down(int key, bool down) {
        if (down) {
            _down_keys.insert(key);
        } else {
            _down_keys.erase(key);
        }
    }

private:
    DummyRenderer _renderer {};
    std::unordered_set<int> _down_keys {};
};

class TestLayoutActor final : public LayoutActor {
public:
    explicit TestLayoutActor(std::shared_ptr<smgpc::game::layout::LayoutRuntimeActor> runtime_actor)
        : LayoutActor("TestLayout", std::move(runtime_actor)) {
    }
};

[[nodiscard]] smgpc::assets::layout::PaneDefinition make_root_pane() {
    smgpc::assets::layout::PaneDefinition pane {};
    pane.type = smgpc::assets::layout::PaneType::Pane;
    pane.name = "Root";
    pane.visible = true;
    pane.alpha = 255U;
    pane.scale = {.x = 1.0F, .y = 1.0F};
    pane.size = {.x = 640.0F, .y = 480.0F};
    return pane;
}

[[nodiscard]] smgpc::assets::layout::BrlanAnimation make_animation(std::string name, std::uint16_t frame_size, bool loop) {
    return smgpc::assets::layout::BrlanAnimation {
        .name = std::move(name),
        .frame_size = frame_size,
        .loop = loop,
        .tracks = {},
    };
}

[[nodiscard]] std::shared_ptr<smgpc::game::layout::LayoutRuntimeActor> make_logo_actor() {
    auto resource = std::make_shared<smgpc::game::layout::LayoutArchiveData>();
    resource->layout.center_origin = true;
    resource->layout.size = {.x = 640.0F, .y = 480.0F};
    resource->layout.root_pane = 0;
    resource->layout.panes.push_back(make_root_pane());
    resource->animations_by_name.emplace("appear", make_animation("Appear", 1U, false));
    resource->animations_by_name.emplace("wait", make_animation("Wait", 120U, true));
    resource->animations_by_name.emplace("decide", make_animation("Decide", 4U, false));
    resource->animations_by_name.emplace("reactiona", make_animation("ReactionA", 3U, false));
    resource->animations_by_name.emplace("reactionb", make_animation("ReactionB", 3U, false));
    return std::make_shared<smgpc::game::layout::LayoutRuntimeActor>(resource);
}

[[nodiscard]] std::shared_ptr<smgpc::game::layout::LayoutRuntimeActor> make_press_start_actor() {
    auto resource = std::make_shared<smgpc::game::layout::LayoutArchiveData>();
    resource->layout.center_origin = true;
    resource->layout.size = {.x = 640.0F, .y = 480.0F};
    resource->layout.root_pane = 0;
    resource->layout.panes.push_back(make_root_pane());
    resource->animations_by_name.emplace("appear", make_animation("Appear", 1U, false));
    resource->animations_by_name.emplace("wait", make_animation("Wait", 120U, true));
    resource->animations_by_name.emplace("end", make_animation("End", 4U, false));
    resource->animations_by_name.emplace("buttonreaction", make_animation("ButtonReaction", 3U, false));
    return std::make_shared<smgpc::game::layout::LayoutRuntimeActor>(resource);
}

void advance_frames(TitleSequenceProduct *sequence, int frame_count) {
    for (int frame = 0; frame < frame_count; ++frame) {
        sequence->update();
    }
}

}  // namespace

$test("TitleSequenceProduct reaches logo display flow without input") {
    TestLayoutActor logo_actor(make_logo_actor());
    TestLayoutActor press_start_actor(make_press_start_actor());
    TitleSequenceProduct sequence(&logo_actor, &press_start_actor);

    FakeRendererService renderer {};
    smgpc::game::compat::set_runtime_context(smgpc::game::compat::RuntimeContext {
        .asset_service = nullptr,
        .renderer_service = &renderer,
        .logger = nullptr,
        .is_widescreen = true,
    });

    sequence.appear();
    advance_frames(&sequence, 80);

    $pc_port_require(sequence.isActive());
    $pc_port_require(not logo_actor.isDead());
    $pc_port_require(not press_start_actor.isDead());
}

$test("TitleSequenceProduct transitions to dead after enter press") {
    TestLayoutActor logo_actor(make_logo_actor());
    TestLayoutActor press_start_actor(make_press_start_actor());
    TitleSequenceProduct sequence(&logo_actor, &press_start_actor);

    FakeRendererService renderer {};
    smgpc::game::compat::set_runtime_context(smgpc::game::compat::RuntimeContext {
        .asset_service = nullptr,
        .renderer_service = &renderer,
        .logger = nullptr,
        .is_widescreen = true,
    });

    sequence.appear();
    advance_frames(&sequence, 80);

    renderer.set_key_down(257, true);
    advance_frames(&sequence, 20);
    renderer.set_key_down(257, false);
    advance_frames(&sequence, 10);

    $pc_port_require(not sequence.isActive());

    sequence.update();
    $pc_port_require(logo_actor.isDead());
    $pc_port_require(press_start_actor.isDead());
}
