#include "Game/Util/PlayerUtil.hpp"

// Native actor overload delegates to the original PlayerUtil predicate.
namespace MR {
    bool isOnPlayer(const LiveActor* pActor) {
        return isActorOnPlayer(pActor);
    }
}
