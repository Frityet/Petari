#include "game/Game/Util/GamePadUtil.hpp"
#include "game/compat/GamePadCompat.hpp"
#include "game/compat/RuntimeContext.hpp"
#include "render/RendererService.hpp"
#include "tests/TestHarness.hpp"

#include <array>
#include <cstddef>
#include <cstdlib>

namespace {

constexpr int ENTER_KEY = 257;
constexpr int UP_KEY = 265;
constexpr int DOWN_KEY = 264;

class FakeInputSnapshot final : public smgpc::render::IInputSnapshot {
public:
    void set_key_down(int key, bool down) {
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

    void set_key_down(int key, bool down) {
        _snapshot.set_key_down(key, down);
    }

private:
    mutable FakeInputSnapshot _snapshot {};
};

}  // namespace

$test("GamePadUtil A trigger fires only on rising edge") {
    FakeInputService input_service {};
    smgpc::game::compat::set_runtime_context(smgpc::game::compat::RuntimeContext {
        .input_service = &input_service,
    });

    input_service.set_key_down(ENTER_KEY, false);
    $pc_port_require(!MR::testCorePadTriggerA(0));

    input_service.set_key_down(ENTER_KEY, true);
    $pc_port_require(MR::testCorePadTriggerA(0));
    $pc_port_require(!MR::testCorePadTriggerA(0));

    input_service.set_key_down(ENTER_KEY, false);
    $pc_port_require(!MR::testCorePadTriggerA(0));

    input_service.set_key_down(ENTER_KEY, true);
    $pc_port_require(MR::testCorePadTriggerA(0));

    smgpc::game::compat::set_runtime_context(smgpc::game::compat::RuntimeContext {});
}

$test("GamePadUtil synthetic A trigger frames fire once") {
    FakeInputService input_service {};
    smgpc::game::compat::set_runtime_context(smgpc::game::compat::RuntimeContext {
        .input_service = &input_service,
    });

    setenv("SMGPC_SYNTHETIC_A_TRIGGER_FRAMES", "10, 12", 1);
    smgpc::game::compat::begin_input_frame(9U);
    $pc_port_require(!MR::testCorePadTriggerA(WPAD_CHAN0));

    smgpc::game::compat::begin_input_frame(10U);
    $pc_port_require(MR::testCorePadTriggerA(WPAD_CHAN0));
    $pc_port_require(!MR::testCorePadTriggerA(WPAD_CHAN0));

    smgpc::game::compat::begin_input_frame(11U);
    $pc_port_require(!MR::testCorePadTriggerA(WPAD_CHAN0));

    smgpc::game::compat::begin_input_frame(12U);
    $pc_port_require(MR::testCorePadTriggerA(WPAD_CHAN0));
    unsetenv("SMGPC_SYNTHETIC_A_TRIGGER_FRAMES");

    smgpc::game::compat::set_runtime_context(smgpc::game::compat::RuntimeContext {});
}

$test("GamePadUtil directional triggers fire only on rising edge") {
    FakeInputService input_service {};
    smgpc::game::compat::set_runtime_context(smgpc::game::compat::RuntimeContext {
        .input_service = &input_service,
    });

    input_service.set_key_down(UP_KEY, false);
    input_service.set_key_down(DOWN_KEY, false);
    $pc_port_require(!MR::testCorePadTriggerUp(WPAD_CHAN0));
    $pc_port_require(!MR::testCorePadTriggerDown(WPAD_CHAN0));

    input_service.set_key_down(UP_KEY, true);
    $pc_port_require(MR::testCorePadTriggerUp(WPAD_CHAN0));
    $pc_port_require(!MR::testCorePadTriggerUp(WPAD_CHAN0));

    input_service.set_key_down(UP_KEY, false);
    $pc_port_require(!MR::testCorePadTriggerUp(WPAD_CHAN0));

    input_service.set_key_down(DOWN_KEY, true);
    $pc_port_require(MR::testCorePadTriggerDown(WPAD_CHAN0));
    $pc_port_require(!MR::testCorePadTriggerDown(WPAD_CHAN0));

    smgpc::game::compat::set_runtime_context(smgpc::game::compat::RuntimeContext {});
}
