#pragma once

#include <cstdint>

namespace smgpc::game::compat {

void begin_input_frame(std::uint64_t frame_index);
[[nodiscard]] bool test_core_pad_button_a();
[[nodiscard]] bool test_core_pad_button_b();
[[nodiscard]] bool test_core_pad_trigger_a();
[[nodiscard]] bool test_system_trigger_b();
[[nodiscard]] bool test_core_pad_trigger_left();
[[nodiscard]] bool test_core_pad_trigger_right();
[[nodiscard]] bool test_core_pad_trigger_up();
[[nodiscard]] bool test_core_pad_trigger_down();

}  // namespace smgpc::game::compat
