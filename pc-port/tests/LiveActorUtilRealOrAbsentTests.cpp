#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/LiveActorUtil.hpp"

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void require_unavailable(const std::function<void()>& operation, std::string_view message) {
        auto unavailable = false;
        try {
            operation();
        } catch (const std::logic_error&) {
            unavailable = true;
        }
        require(unavailable, message);
    }
}

int main() {
    auto passed = 0;
    auto actor = LiveActor("animation-absence-test");
    actor.calcAndSetBaseMtx();

    require_unavailable([&] { (void)MR::isBckStopped(&actor); },
                        "an actor without a real BCK must not report a fabricated stopped state");
    require_unavailable([&] { (void)MR::getBckFrameMax(&actor); },
                        "an actor without a real BCK must not report a fabricated frame count");
    ++passed;

    require_unavailable([&] { (void)MR::isBtpStopped(&actor); },
                        "unsupported BTP playback must be explicitly unavailable");
    require_unavailable([&] { (void)MR::isBrkOneTimeAndStopped(&actor); },
                        "an actor without a real BRK must not report a fabricated stopped state");
    require_unavailable([&] { (void)MR::getBrkCtrl(&actor); },
                        "an actor without a real BRK must not expose a fabricated frame controller");
    ++passed;

    require(MR::getJointMtx(&actor, "MarioPosition") == nullptr,
            "a missing model joint must remain absent instead of substituting the actor base matrix");
    require(MR::getJointMtx(nullptr, "MarioPosition") == nullptr,
            "a missing actor must not manufacture a joint matrix");
    ++passed;

    std::cout << "LiveActorUtil real-or-absent tests passed: " << passed << "/3\n";
    return 0;
}
