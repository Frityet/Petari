#include "Game/Util/GamePadUtil.hpp"

#include "RenderWindow.hpp"
#include "compat/RuntimeContext.hpp"

namespace {

constexpr int ENTER_KEY = 257;
constexpr int A_KEY = 90;
constexpr int B_KEY = 88;

}  // namespace

namespace MR {

bool testCorePadButtonA(int channel) {
    (void)channel;

    const auto *renderer = smgpc::game::compat::runtime_context().renderer_service;
    return renderer != nullptr && (renderer->is_key_down(ENTER_KEY) || renderer->is_key_down(A_KEY));
}

bool testCorePadButtonB(int channel) {
    (void)channel;

    const auto *renderer = smgpc::game::compat::runtime_context().renderer_service;
    return renderer != nullptr && (renderer->is_key_down(ENTER_KEY) || renderer->is_key_down(B_KEY));
}

}  // namespace MR
