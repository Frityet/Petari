#include "Game/Player/MarioAccess.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Util/PlayerUtil.hpp"

// PlayerUtil remains excluded from the Game archive. These wrappers use the
// accessors provided by the complete original MarioAccess source unit.
namespace MR {

    Triangle* getPlayerGroundingPolygon() {
        return MarioAccess::getGroundingPolygon(0);
    }

    TVec3f* getPlayerLastMove() {
        return MarioAccess::getLastMove();
    }

    bool isPlayerFlying() {
        return MarioAccess::isFlying();
    }

    bool isPlayerInBind() {
        return MarioAccess::isInRush();
    }

    bool isPlayerInWaterMode() {
        return MarioAccess::isInWaterMode();
    }

    bool isPlayerOnWaterSurface() {
        return MarioAccess::isOnWaterSurface();
    }

    u16 getPlayerMovementTimer() {
        return MarioAccess::getPlayerActor()->_378;
    }

    CubeCameraArea* getCameraCube() {
        return MarioAccess::getCameraCubeCode();
    }

}  // namespace MR

namespace MR {
    bool isActorOnPlayer(const LiveActor* pActor) {
        return MarioAccess::isOnActor(pActor);
    }
}

namespace MR {
    bool isOnPlayer(const LiveActor* pActor) {
        return isActorOnPlayer(pActor);
    }
}
