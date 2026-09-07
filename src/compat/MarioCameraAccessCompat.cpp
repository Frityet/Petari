#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioAccess.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioHolder.hpp"
#include "Game/Player/MarioState.hpp"
#include "Game/Player/MarioSwim.hpp"
#include "Game/Util/AreaObjUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"

// Verbatim original accessors required by CameraTargetPlayer. Full MarioAccess,
// PlayerUtil and MarioActorGravity source units are not enabled in the Game
// archive. Collision, Swim and Jump own their queries directly. See the source correspondence
// record in notes/original-camera-target-player-20260903.
namespace MarioAccess {

    bool isOnGround(u32 a1) {
        if (a1 != 0) {
            return false;
        }

        if (getPlayerActor()->_934) {
            return MR::isOnGround(getPlayerActor()->_924->mHost);
        }

        return getPlayerActor()->getMovementStates()._1;
    }

    bool isInRush() {
        return getPlayerActor()->_934 || getPlayerActor()->getMario()->isStatusActive(MarioStatus_13);
    }

    bool isFlying() {
        if (getPlayerActor()->getMario()->isStatusActive(MarioStatus_Foo)) {
            return true;
        }

        return getPlayerActor()->getMario()->_10._21;
    }

    CubeCameraArea* getCameraCubeCode() {
        return getPlayerActor()->getMario()->getCameraCubeCode();
    }

    bool isSwimming() {
        return getPlayerActor()->getMario()->isSwimming();
    }

    Triangle* getGroundingPolygon(u32) {
        if (isSwimming()) {
            if (getPlayerActor()->getMovementStates()._2) {
                return getPlayerActor()->getMario()->_45C;
            }

            return nullptr;
        }

        if (!isOnGround(0)) {
            return nullptr;
        }

        if (getPlayerActor()->_934) {
            return &getPlayerActor()->_924->mHost->mBinder->mGroundInfo.mParentTriangle;
        }

        return getPlayerActor()->getMario()->mGroundPolygon;
    }

    TVec3f* getLastMove() {
        return const_cast< TVec3f* >(&getPlayerActor()->getLastMove());
    }

    MtxPtr getBaseMtx() {
        if (getPlayerActor()->_EA5) {
            return getPlayerActor()->_EA8;
        }

        return getPlayerActor()->getBaseMtx();
    }

    bool isOnWaterSurface() {
        if (getPlayerActor()->getMario()->isStatusActive(MarioStatus_Swim)) {
            return getPlayerActor()->getMario()->mSwim->isOnWaterSurface();
        }

        return getPlayerActor()->isAnimationRun("水泳ジェット");
    }

    MarioActor* getPlayerActor() {
        return MR::getMarioHolder()->getMarioActor();
    }

    bool isInWaterMode() {
        if (getPlayerActor()->getMario()->isStatusActive(MarioStatus_Swim)) {
            return true;
        }

        return getPlayerActor()->isAnimationRun("水泳ジェット");
    }

}  // namespace MarioAccess

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

GravityInfo* MarioActor::getGravityInfo() const {
    return mGravityInfo;
}

namespace MarioAccess {
    bool isOnActor(const LiveActor* pActor) {
        if (getPlayerActor()->getMario()->_1C._13) {
            Triangle* marioGroundPolygon = getPlayerActor()->getMario()->mGroundPolygon;
            if (marioGroundPolygon->isValid()) {
                return marioGroundPolygon->mSensor->mHost == pActor;
            }

            return false;
        }

        if (getPlayerActor()->getMario()->_1C._14 || (getPlayerActor()->IsMarioSwimming() && getPlayerActor()->getMovementStates()._2)) {
            Triangle* marioTri = getPlayerActor()->getMario()->_45C;
            if (!marioTri->isValid()) {
                return false;
            }

            return marioTri->mSensor->mHost == pActor;
        }

        return false;
    }
}

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
