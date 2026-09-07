#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/StarPointerUtil.hpp"

#include <aurora/wpad.hpp>

#include <array>
#include <iostream>
#include <stdexcept>

int main() {
    auto& input = aurora::wpad_service();
    const auto saved_input = input;
    try {
        input.clear();
        LiveActor actor("Pointer device boundary");
        const auto queries = std::array{
            MR::isStarPointerPointing2P,
            MR::isStarPointerPointing2POnPressButton,
            MR::isStarPointerPointing2POnTriggerButton,
        };
        input.set_pointer(WPAD_CHAN0, 10.0F, 20.0F, true);
        input.set_button_mask(WPAD_CHAN0, WPAD_BUTTON_A);
        for (const auto query : queries) {
            if (query(&actor, "Pointer", true, true)) {
                throw std::runtime_error("disconnected channel inherited player-one pointing");
            }
        }
        input.set_connected(WPAD_CHAN1, true);
        for (const auto query : queries) {
            bool rejected = false;
            try {
                (void)query(&actor, "Pointer", true, true);
            } catch (const std::logic_error&) {
                rejected = true;
            }
            if (!rejected) {
                throw std::runtime_error("connected pointing fabricated an absent result");
            }
        }
        input.set_connected(WPAD_CHAN1, false);
        for (const auto query : queries) {
            if (query(&actor, nullptr, false, false)) {
                throw std::runtime_error("disconnected channel retained pointing state");
            }
        }
        input = saved_input;
        std::cout << "StarPointer device boundary tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        input = saved_input;
        std::cerr << "StarPointer device boundary tests failed: " << error.what() << '\n';
        return 1;
    }
}
