#include "render/core/FrameButtonState.hpp"
#include "render/core/RenderTypes.hpp"
#include "OriginalStageResourceProcessFixture.hpp"
#include "Game/System/WPadHolder.hpp"
#include "Game/System/WPad.hpp"
#include "Game/System/WPadButton.hpp"
#include "runtime/DebugWpadInputScript.hpp"
#include "runtime/DebugWpadInputFile.hpp"
#include <aurora/exception.hpp>
#include <aurora/wpad.hpp>

#include <iostream>
#include <stdexcept>
#include <limits>
#include <filesystem>
#include <fstream>
#include <unistd.h>

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

    void sample(Buttons& buttons, WPadHolder& owner,
                u32 hold, u32 trigger, u32 release, const char* message) {
        auto& input = aurora::wpad_service();
        input.begin_frame();
        input.set_device_type(0, aurora::WpadDeviceType::Freestyle);
        input.set_connected(0, true);
        input.set_button_mask(0, (buttons.is_pressed(a) ? WPAD_BUTTON_A : 0U) |
                                (buttons.is_pressed(b) ? WPAD_BUTTON_B : 0U));
        aurora::wpad_service().dispatch_callbacks();
        owner.update();
        const auto& original = *owner.getWPad(0)->mButton;
        const auto observed_hold = (original.testButtonA() ? WPAD_BUTTON_A : 0U) |
                                   (original.testButtonB() ? WPAD_BUTTON_B : 0U);
        const auto observed_trigger = (original.testTriggerA() ? WPAD_BUTTON_A : 0U) |
                                      (original.testTriggerB() ? WPAD_BUTTON_B : 0U);
        require(observed_hold == hold && observed_trigger == trigger &&
                    owner.getWPad(0)->getKPadStatus(0)->release == release &&
                    original.isChangeAnyState() == ((trigger | release) != 0), message);
    }

    void test_controller_samples() {
        auto& owner = *MR::getGameSystemObjHolder()->mWPadHolder;
        aurora::wpad_service().clear();
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

#ifndef NDEBUG
    void test_shared_controller_script() {
        using Script = smgpc::runtime::DebugWpadInputScript;
        using Pointer = smgpc::render::core::InputPointerState;
        float stick_x = 0.25F;
        float stick_y = -0.5F;
        const Script script{
            " 2-4: A + B ; 3:C; 7-:PLUS",
            "2-4:10.5,20;3:90,80,false;5-:6,7,off"};
        require(script.button_span_count() == 3 && script.pointer_span_count() == 3,
                "shared parser retains every valid button and pointer span");
        for (std::uint64_t frame = 0; frame != 9; ++frame) {
            u32 mask = WPAD_BUTTON_Z;
            Pointer pointer{1, 2, true};
            const auto applied = script.apply(frame, mask, pointer, stick_x, stick_y);
            u32 expected = WPAD_BUTTON_Z;
            if (frame >= 2 && frame <= 4) expected |= WPAD_BUTTON_A | WPAD_BUTTON_B;
            if (frame == 3) expected |= WPAD_BUTTON_C;
            if (frame >= 7) expected |= WPAD_BUTTON_PLUS;
            require(mask == expected && applied.buttons == (expected != WPAD_BUTTON_Z),
                    "button ranges are inclusive, overlap by OR, and preserve physical input");
            if (frame == 3) {
                require(pointer.x == 90 && pointer.y == 80 && !pointer.valid,
                        "the last active pointer span wins, including explicit invalidity");
            } else if (frame >= 2 && frame <= 4) {
                require(pointer.x == 10.5F && pointer.y == 20 && pointer.valid,
                        "pointer coordinates and default validity survive parsing");
            } else if (frame >= 5) {
                require(pointer.x == 6 && pointer.y == 7 && !pointer.valid,
                        "open-ended pointer spans preserve false validity");
            } else {
                require(pointer.x == 1 && pointer.y == 2 && pointer.valid,
                        "inactive scripts preserve physical pointer input");
            }
            require(applied.pointer == (frame >= 2), "pointer activity reports actual span application");
        }
        u32 mask = 0;
        Pointer pointer;
        require(script.apply(std::numeric_limits<std::uint64_t>::max(), mask, pointer, stick_x, stick_y).buttons &&
                    mask == WPAD_BUTTON_PLUS,
                "open-ended button ranges include the final representable frame");
        const Script empty;
        mask = WPAD_BUTTON_B;
        const auto inactive = empty.apply(3, mask, pointer, stick_x, stick_y);
        require(!inactive.buttons && !inactive.pointer && mask == WPAD_BUTTON_B,
                "an absent script leaves physical input unchanged");
        require(!inactive.stick && stick_x == 0.25F && stick_y == -0.5F,
                "an absent stick script preserves the physical stick");
        const Script stick_script{{}, {}, "2-4:1:-1;3:0:0;7-:-0.5:0.25"};
        require(stick_script.stick_span_count() == 3, "stick scripts use the shared frame-range syntax");
        require(stick_script.apply(2, mask, pointer, stick_x, stick_y).stick && stick_x == 1 && stick_y == -1,
                "stick axes accept inclusive normalized endpoints");
        require(stick_script.apply(3, mask, pointer, stick_x, stick_y).stick && stick_x == 0 && stick_y == 0,
                "the last overlapping stick span can explicitly center the stick");
        require(stick_script.apply(std::numeric_limits<std::uint64_t>::max(), mask, pointer, stick_x, stick_y).stick &&
                    stick_x == -0.5F && stick_y == 0.25F,
                "open-ended stick ranges preserve their normalized coordinates");
        for (const char* invalid : {"2-1:0:0", "bad:0:0", "2:1.01:0", "2:0:-1.01", "2:nan:0", "2:0:inf", "2:0,0", "2:0:0:0"}) {
            bool rejected = false;
            try { (void)Script{{}, {}, invalid}; }
            catch (const std::invalid_argument&) { rejected = true; }
            require(rejected, "invalid scripted stick ranges, coordinates and syntax are rejected");
        }

        for (const char* invalid : {"6-2:A", "nope:A", "9:unknown", "9:A+unknown", "9:A+", "18446744073709551616:A"}) {
            bool rejected = false;
            try { (void)Script{invalid}; } catch (const std::invalid_argument&) { rejected = true; }
            require(rejected, "malformed button spans must fail instead of silently losing intended input");
        }
        for (const char* invalid : {"bad", "5-2:0,0", "6:not-a-number,7", "1:nan,0", "1:0,inf", "1:0,0,maybe", "1:0,0,true,false"}) {
            bool rejected = false;
            try { (void)Script{{}, invalid}; } catch (const std::invalid_argument&) { rejected = true; }
            require(rejected, "malformed pointer spans and nonfinite coordinates must fail explicitly");
        }

        auto& owner = *MR::getGameSystemObjHolder()->mWPadHolder;
        aurora::wpad_service().clear();
        const Script held{"3-5:A", {}, "3-5:0.5:-0.25"};
        for (std::uint64_t frame = 0; frame != 8; ++frame) {
            auto& input = aurora::wpad_service();
            input.begin_frame();
            input.set_device_type(0, aurora::WpadDeviceType::Freestyle);
            input.set_connected(0, true);
            mask = WPAD_BUTTON_B;
            stick_x = stick_y = 0.0F;
            const auto applied = held.apply(frame, mask, pointer, stick_x, stick_y);
            input.set_button_mask(0, mask);
            input.set_sub_stick(0, stick_x, stick_y);
            aurora::wpad_service().dispatch_callbacks();
            owner.update();
            const auto& original = *owner.getWPad(0)->mButton;
            const bool active = frame >= 3 && frame <= 5;
            require(applied.buttons == active && original.testButtonA() == active && original.testButtonB(),
                    "script is applied before original WPad sampling and keeps physical B held");
            const auto& stick = owner.getWPad(0)->getKPadStatus(0)->ex_status.fs.stick;
            require(applied.stick == active && stick.x == (active ? 0.5F : 0.0F) && stick.y == (active ? -0.25F : 0.0F),
                    "scripted Nunchuk axes reach the original KPAD record and release to physical input");
            require(original.testTriggerA() == (frame == 3),
                    "a scripted hold has one original trigger and subsequent non-trigger held samples");
            require((owner.getWPad(0)->getKPadStatus(0)->release & WPAD_BUTTON_A) == (frame == 6 ? WPAD_BUTTON_A : 0),
                    "leaving a script range creates the normal original release edge");
        }
        std::cout << "Shared debug controller script: parsing, overlap, pointer precedence and original WPad hold/trigger/release passed\n";
    }
    void test_live_controller_file() {
        namespace fs = std::filesystem;
        using smgpc::runtime::DebugWpadInputFile;
        const auto path = fs::temp_directory_path() / ("petari-controller-file-" + std::to_string(getpid()) + ".json");
        const auto pending = fs::path(path.string() + ".pending");
        require(!fs::exists(path) && !fs::exists(pending), "test owns fresh controller files");
        struct Cleanup { fs::path a, b; ~Cleanup() { std::error_code e; fs::remove(a, e); fs::remove(b, e); } } cleanup{path, pending};
        auto replace = [&](const std::string& text) {
            { std::ofstream output(pending, std::ios::binary); output << text; require(bool(output), "write controller update"); }
            fs::rename(pending, path);
        };
        u32 mask = WPAD_BUTTON_B;
        smgpc::render::core::InputPointerState pointer{12, 34, true};
        float x = 0.25F, y = -0.5F;
        DebugWpadInputFile disabled;
        require(!disabled.apply(0, mask, pointer, x, y).stick && mask == WPAD_BUTTON_B && x == .25F && pointer.x == 12,
                "disabled live source preserves physical input without opening a file");
        replace(R"({"buttons":"2-4:A","pointer":"2-4:100,200,false","stick":"2-4:1:-1"})");
        DebugWpadInputFile file(path.string());
        auto applied = file.apply(2, mask, pointer, x, y);
        require(file.revision() == 1 && applied.buttons && applied.pointer && applied.stick &&
                mask == (WPAD_BUTTON_A | WPAD_BUTTON_B) && pointer.x == 100 && !pointer.valid && x == 1 && y == -1,
                "an atomic revision applies every input channel through the shared parser");
        file.apply(3, mask, pointer, x, y);
        require(file.revision() == 1, "unchanged bytes do not create phantom input revisions");
        replace(R"({"buttons":"","pointer":"","stick":"3-:0:0"})");
        mask = WPAD_BUTTON_B; pointer = {12, 34, true};
        applied = file.apply(3, mask, pointer, x, y);
        require(file.revision() == 2 && !applied.buttons && !applied.pointer && applied.stick &&
                mask == WPAD_BUTTON_B && pointer.x == 12 && x == 0 && y == 0,
                "replacement releases scripted buttons/pointer and can explicitly center the stick");
        for (const auto& invalid : {std::string("{}"), std::string("{"),
                std::string(R"({"buttons":"5:A+typo","pointer":"","stick":"5-:1:0"})"),
                std::string(R"({"buttons":"","pointer":"1:nan,0","stick":""})"),
                std::string(R"({"buttons":"","pointer":"","stick":"3-:4:0"})"), std::string(65537, ' ')}) {
            replace(invalid);
            mask = WPAD_BUTTON_B; x = .25F; y = -.5F;
            bool rejected = false;
            try { file.apply(5, mask, pointer, x, y); } catch (const std::exception&) { rejected = true; }
            require(rejected && file.revision() == 2 && mask == WPAD_BUTTON_B && x == .25F && y == -.5F,
                    "invalid revisions cannot partially publish controls or replace prior parser state");
        }
        fs::remove(path);
        bool rejected = false;
        try { file.apply(6, mask, pointer, x, y); } catch (const std::exception&) { rejected = true; }
        require(rejected && file.revision() == 2, "a vanished explicit replay source fails rather than holding stale input silently");
        std::cout << "Live controller file: atomic revisions, release, bounded reads and failed-update rollback passed\n";
    }

#endif
}

int main() {
    return smgpc::test::run_stage_resource_process("frame-button-state", [] {
        test_controller_samples();
        test_debug_input();
#ifndef NDEBUG
        test_shared_controller_script();
        test_live_controller_file();
#endif
        std::cout << "Frame input: quick key/mouse taps, physical aliases, repeat, focus reset and original WPad edges passed\n";
    });
}
