#include "compat/GamePadCompat.hpp"

#include "RendererService.hpp"

#include "compat/RuntimeContext.hpp"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <limits>

namespace {

constexpr int ENTER_KEY = 257;
constexpr int A_KEY = 90;
constexpr int B_KEY = 88;
constexpr int LEFT_KEY = 263;
constexpr int RIGHT_KEY = 262;
constexpr int UP_KEY = 265;
constexpr int DOWN_KEY = 264;

std::uint64_t sCurrentInputFrame = 0U;
std::uint64_t sLastSyntheticAFrame = std::numeric_limits<std::uint64_t>::max();

[[nodiscard]] bool is_key_down(int key) {
    const auto &input = smgpc::game::compat::runtime_context().input_service;
    return input && input->is_key_down(key);
}

[[nodiscard]] bool test_key_trigger(int key, bool *pWasPressed) {
    const bool pressed = is_key_down(key);
    const bool triggered = pressed && !*pWasPressed;
    *pWasPressed = pressed;
    return triggered;
}

[[nodiscard]] bool is_separator(char ch) {
    return ch == ',' || ch == ';' || ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}

[[nodiscard]] bool test_synthetic_a_trigger() {
    if (sLastSyntheticAFrame == sCurrentInputFrame) {
        return false;
    }

    const char *value = std::getenv("SMGPC_SYNTHETIC_A_TRIGGER_FRAMES");
    if (value == nullptr || value[0] == '\0') {
        return false;
    }

    const char *cursor = value;
    while (*cursor != '\0') {
        while (is_separator(*cursor)) {
            ++cursor;
        }
        if (*cursor == '\0') {
            break;
        }

        errno = 0;
        char *end = nullptr;
        const unsigned long long parsed = std::strtoull(cursor, &end, 10);
        if (end == cursor) {
            while (*cursor != '\0' && !is_separator(*cursor)) {
                ++cursor;
            }
            continue;
        }

        if (errno == 0 && parsed == sCurrentInputFrame) {
            sLastSyntheticAFrame = sCurrentInputFrame;
            return true;
        }

        cursor = end;
    }

    return false;
}

}  // namespace

namespace smgpc::game::compat {

void begin_input_frame(std::uint64_t frame_index) {
    sCurrentInputFrame = frame_index;
}

bool test_core_pad_button_a() {
    return is_key_down(ENTER_KEY) || is_key_down(A_KEY);
}

bool test_core_pad_button_b() {
    return is_key_down(ENTER_KEY) || is_key_down(B_KEY);
}

bool test_core_pad_trigger_a() {
    static bool sWasPressed = false;
    const bool pressed = is_key_down(ENTER_KEY) || is_key_down(A_KEY);
    const bool triggered = test_synthetic_a_trigger() || (pressed && !sWasPressed);
    sWasPressed = pressed;
    return triggered;
}

bool test_system_trigger_b() {
    static bool sWasPressed = false;
    return test_key_trigger(B_KEY, &sWasPressed);
}

bool test_core_pad_trigger_left() {
    static bool sWasPressed = false;
    return test_key_trigger(LEFT_KEY, &sWasPressed);
}

bool test_core_pad_trigger_right() {
    static bool sWasPressed = false;
    return test_key_trigger(RIGHT_KEY, &sWasPressed);
}

bool test_core_pad_trigger_up() {
    static bool sWasPressed = false;
    return test_key_trigger(UP_KEY, &sWasPressed);
}

bool test_core_pad_trigger_down() {
    static bool sWasPressed = false;
    return test_key_trigger(DOWN_KEY, &sWasPressed);
}

}  // namespace smgpc::game::compat
