#include "render/core/FrameButtonState.hpp"
#include "render/core/RenderTypes.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/WPadOwnership.hpp"
#include "Game/System/WPad.hpp"
#include "Game/System/WPadButton.hpp"
#include <aurora/exception.hpp>
#include <aurora/wpad.hpp>

#include <iostream>
#include <stdexcept>

namespace {
    using Button = smgpc::render::core::InputButton;
    using Buttons = smgpc::render::core::FrameButtonState<Button>;
    constexpr auto a = Button::CORE_PAD_A;
    constexpr auto b = Button::CORE_PAD_B;
    constexpr Buttons::Source enter = 40;
    constexpr Buttons::Source space = 44;
    constexpr Buttons::Source mouse_left = 0x80000001U;
    constexpr Buttons::Source second_keyboard_enter = (Buttons::Source{1} << 32U) | enter;

    void require(bool value, const char* message) {
        if (!value) aurora::throw_host_exception<std::runtime_error>(message);
    }

    void sample(Buttons& buttons, smgpc::compat::WPadOwnership& owner,
                u32 hold, u32 trigger, u32 release, const char* message) {
        auto& input = aurora::wpad_service();
        input.begin_frame();
        input.set_device_type(0, aurora::WpadDeviceType::Freestyle);
        input.set_connected(0, true);
        input.set_button_mask(0, (buttons.is_pressed(a) ? WPAD_BUTTON_A : 0U) |
                                (buttons.is_pressed(b) ? WPAD_BUTTON_B : 0U));
        owner.update_samples();
        const auto& original = *owner.pad(0).mButton;
        const auto observed_hold = (original.testButtonA() ? WPAD_BUTTON_A : 0U) |
                                   (original.testButtonB() ? WPAD_BUTTON_B : 0U);
        const auto observed_trigger = (original.testTriggerA() ? WPAD_BUTTON_A : 0U) |
                                      (original.testTriggerB() ? WPAD_BUTTON_B : 0U);
        require(observed_hold == hold && observed_trigger == trigger &&
                    owner.pad(0).getKPadStatus(0)->release == release &&
                    original.isChangeAnyState() == ((trigger | release) != 0), message);
    }

    void test_controller_samples() {
        auto heaps = smgpc::compat::JkrHeapRuntime::create(2U << 20);
        auto domain = smgpc::compat::JkrAllocationDomain::create(heaps, 512U << 10);
        aurora::wpad_service().clear();
        smgpc::compat::WPadOwnership owner(domain);
        Buttons buttons;
        sample(buttons, owner, 0, 0, 0, "initial controller sample is neutral");

        buttons.begin_poll();
        buttons.press(enter, a);
        buttons.release(enter);
        sample(buttons, owner, WPAD_BUTTON_A, WPAD_BUTTON_A, 0,
               "a complete key tap in one event poll reaches the original A trigger");
        require(buttons.is_pressed(a) && buttons.is_pressed(a), "input reads do not consume a pending tap");
        buttons.begin_poll();
        sample(buttons, owner, 0, 0, WPAD_BUTTON_A, "a released tap is sampled for exactly one frame");
        buttons.begin_poll();
        sample(buttons, owner, 0, 0, 0, "idle polls do not repeat a tap or release edge");

        buttons.begin_poll();
        buttons.press(enter, a);
        sample(buttons, owner, WPAD_BUTTON_A, WPAD_BUTTON_A, 0, "initial held key has one original trigger");
        for (int frame = 0; frame != 3; ++frame) {
            buttons.begin_poll();
            buttons.press(enter, a, true);
            buttons.press(enter, a);
            sample(buttons, owner, WPAD_BUTTON_A, 0, 0,
                   "held, repeated and duplicate key-down events do not retrigger");
        }
        buttons.begin_poll();
        buttons.press(space, a);
        buttons.release(enter);
        buttons.release(enter);
        sample(buttons, owner, WPAD_BUTTON_A, 0, 0, "a held alias survives another alias's repeated release");
        buttons.begin_poll();
        buttons.press(mouse_left, a);
        buttons.release(space);
        sample(buttons, owner, WPAD_BUTTON_A, 0, 0, "keyboard release preserves a mouse alias without a new edge");
        buttons.begin_poll();
        buttons.release(mouse_left);
        sample(buttons, owner, 0, 0, WPAD_BUTTON_A, "the last held physical source releases the logical button");

        buttons.begin_poll();
        buttons.press(mouse_left, a);
        buttons.release(mouse_left);
        sample(buttons, owner, WPAD_BUTTON_A, WPAD_BUTTON_A, 0, "a complete mouse click survives one event poll");
        buttons.begin_poll();
        sample(buttons, owner, 0, 0, WPAD_BUTTON_A, "quick mouse input has the same frame lifetime as a key tap");

        buttons.begin_poll();
        buttons.press(enter, a);
        buttons.press(second_keyboard_enter, a);
        sample(buttons, owner, WPAD_BUTTON_A, WPAD_BUTTON_A, 0, "separate keyboard devices can share a scancode");
        buttons.begin_poll();
        buttons.release(enter);
        sample(buttons, owner, WPAD_BUTTON_A, 0, 0, "releasing one keyboard leaves the second keyboard held");
        buttons.begin_poll();
        buttons.release(second_keyboard_enter);
        sample(buttons, owner, 0, 0, WPAD_BUTTON_A, "the second keyboard releases independently");

        buttons.begin_poll();
        buttons.press(enter, a);
        buttons.press(space, b);
        sample(buttons, owner, WPAD_BUTTON_A | WPAD_BUTTON_B, WPAD_BUTTON_A | WPAD_BUTTON_B, 0,
               "independent logical actions share a poll without overwriting each other");
        buttons.begin_poll();
        buttons.clear();
        sample(buttons, owner, 0, 0, WPAD_BUTTON_A | WPAD_BUTTON_B, "focus loss clears every held physical source");
        buttons.begin_poll();
        buttons.press(enter, a, true);
        buttons.release(space);
        sample(buttons, owner, 0, 0, 0, "late repeat and release events cannot restore a focus-lost hold");
        buttons.begin_poll();
        buttons.press(enter, a);
        buttons.clear();
        sample(buttons, owner, 0, 0, 0, "focus loss also cancels a tap latched earlier in the current batch");

        buttons.begin_poll();
        buttons.press(enter, a);
        buttons.press(enter, b);
        sample(buttons, owner, WPAD_BUTTON_A, WPAD_BUTTON_A, 0, "a held source keeps its original binding until release");
        buttons.begin_poll();
        buttons.release(enter);
        sample(buttons, owner, 0, 0, WPAD_BUTTON_A, "release uses source identity without relying on the current key mapping");
        buttons.begin_poll();
        buttons.press(enter, Button::COUNT);
        require(!buttons.is_pressed(Button::COUNT), "the enum sentinel does not create a logical action");
        sample(buttons, owner, 0, 0, 0, "an unmapped logical action leaves the controller neutral");
    }

    void test_debug_input() {
        using Debug = smgpc::render::core::DebugInput;
        smgpc::render::core::FrameButtonState<Debug> buttons;
        constexpr auto toggle = Debug::CORE_PAD_TOGGLE_FREECAM;
        buttons.begin_poll();
        buttons.press(66, toggle);
        buttons.release(66);
        require(buttons.is_pressed(toggle), "debug actions preserve quick taps too");
        buttons.begin_poll();
        require(!buttons.is_pressed(toggle), "debug taps do not become stuck holds");
    }
}

int main() {
    try {
        test_controller_samples();
        test_debug_input();
        std::cout << "Frame input: quick key/mouse taps, physical aliases, repeat, focus reset and original WPad edges passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
