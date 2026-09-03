#include "Game/Player/MarioAccess.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioAnimator.hpp"
#include "Game/Util/LiveActorUtil.hpp"

namespace MarioAccess {
    void changeAnimationE(const char* pAnimName, const char* pChar2) {
        // unused
        getPlayerActor();
        if (getPlayerActor()->_B91) {
            return;
        }

        if (getPlayerActor()->_468 == 0) {
            getPlayerActor()->getMario()->stopAnimationUpperForce();
        }

        MR::startBck(getPlayerActor(), pAnimName, pChar2);

        getPlayerActor()->setBlink(pAnimName);
        getPlayerActor()->mMarioAnim->closeCallback();
        getPlayerActor()->mMarioAnim->entryCallback(pAnimName);
    }
}
