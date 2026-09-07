#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Screen/StarPointerTarget.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "Game/Util/StarPointerUtil.hpp"

#include <stdexcept>

namespace {
    bool query_unavailable_pointing_channel(s32 channel) {
        // The original pointing core rejects a disconnected WPad before any
        // target hit test, touch transition, shooting suppression, or rumble.
        if (!MR::isConnectedWPad(channel)) {
            return false;
        }
        throw std::logic_error(
            "Connected actor pointing requires the original StarPointer controller and layout owners.");
    }
} // namespace

namespace MR {
    bool isStarPointerPointing2P(const LiveActor*, const char*, bool, bool) {
        return query_unavailable_pointing_channel(WPAD_CHAN1);
    }

    bool isStarPointerPointing2POnPressButton(const LiveActor*, const char*, bool, bool) {
        return query_unavailable_pointing_channel(WPAD_CHAN1);
    }

    bool isStarPointerPointing2POnTriggerButton(const LiveActor*, const char*, bool, bool) {
        return query_unavailable_pointing_channel(WPAD_CHAN1);
    }

    // Original StarPointerUtil.cpp body; the actor retains its actual target.
    s32* getStarPointerLastPointedPort(const LiveActor* pActor) {
        return &pActor->mStarPointerTarget->mLastPointedChannel;
    }
} // namespace MR
