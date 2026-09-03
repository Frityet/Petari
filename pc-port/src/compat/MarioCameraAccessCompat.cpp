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
// PlayerUtil, MarioCollision, MarioSwim, MarioJump, and MarioActorGravity source
// units are not enabled in the native player slice. See the source correspondence
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

CubeCameraArea* Mario::getCameraCubeCode() const {
    if (isSwimming()) {
        bool isSurface = mSwim->mIsOnSurface || mSwim->mIsSwimmingAtSurface;

        if (isSurface) {
            TVec3f gravity(*getGravityVec());
            gravity.scale(100.0f);
            TVec3f pos(mPosition);
            pos += gravity;
            return reinterpret_cast< CubeCameraArea* >(MR::getAreaObj("CubeCamera", pos));
        }
    }
    else if (mMovementStates.jumping && isRising()) {
        TVec3f gravity(*getGravityVec());
        gravity.scale(100.0f);
        TVec3f pos(mPosition);
        pos += gravity;
        return reinterpret_cast< CubeCameraArea* >(MR::getAreaObj("CubeCamera", pos));
    }

    return reinterpret_cast< CubeCameraArea* >(MR::getAreaObj("CubeCamera", mPosition));
}

bool Mario::isSwimming() const {
    if (isStatusActive(MarioStatus_Swim)) {
        return true;
    }
    return isStatusActive(MarioStatus_Foo);
}

bool Mario::isRising() const {
    if (getPlayerMode() == 4 || getPlayerMode() == 6) {
        if (_16C.dot(*getGravityVec()) < 0.0f) {
            return true;
        }

        return false;
    }

    return mJumpVec.dot(*getGravityVec()) < 0.0f;
}

GravityInfo* MarioActor::getGravityInfo() const {
    return mGravityInfo;
}

bool MarioActor::isAnimationRun(const char* pName) const {
    return mMario->isAnimationRun(pName);
}
