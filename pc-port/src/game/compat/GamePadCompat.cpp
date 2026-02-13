#include "compat/GamePadCompat.hpp"

#include "RenderWindow.hpp"
#include "compat/RuntimeContext.hpp"

namespace {

constexpr int ENTER_KEY = 257;
constexpr int A_KEY = 90;
constexpr int B_KEY = 88;

}  // namespace

namespace smgpc::game::compat {

bool test_core_pad_button_a() {
    const auto *renderer = runtime_context().renderer_service;
    return renderer != nullptr && (renderer->is_key_down(ENTER_KEY) || renderer->is_key_down(A_KEY));
}

bool test_core_pad_button_b() {
    const auto *renderer = runtime_context().renderer_service;
    return renderer != nullptr && (renderer->is_key_down(ENTER_KEY) || renderer->is_key_down(B_KEY));
}

}  // namespace smgpc::game::compat
