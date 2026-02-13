#include "compat/GamePadCompat.hpp"

#include "RendererService.hpp"

#include "compat/RuntimeContext.hpp"

namespace {

constexpr int ENTER_KEY = 257;
constexpr int A_KEY = 90;
constexpr int B_KEY = 88;

}  // namespace

namespace smgpc::game::compat {

bool test_core_pad_button_a() {
    const auto &input = runtime_context().input_service;
    return input && (input->is_key_down(ENTER_KEY) || input->is_key_down(A_KEY));
}

bool test_core_pad_button_b() {
    const auto &input = runtime_context().input_service;
    return input && (input->is_key_down(ENTER_KEY) || input->is_key_down(B_KEY));
}

}  // namespace smgpc::game::compat
