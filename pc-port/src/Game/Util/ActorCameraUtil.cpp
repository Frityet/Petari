#include "Game/Util/ActorCameraUtil.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/CameraUtil.hpp"

namespace MR {
    void initActorCameraProgrammable(const LiveActor* pActor) {
        if (pActor != nullptr) {
            MR::declareEventCameraProgrammable(pActor->mName);
        }
    }

    void startActorCameraProgrammable(const LiveActor* pActor, s32 frames) {
        if (pActor != nullptr) {
            MR::startGlobalEventCameraNoTarget(pActor->mName, frames);
        }
    }

    void endActorCameraProgrammable(const LiveActor* pActor, s32 frames, bool endForce) {
        if (pActor != nullptr) {
            MR::endGlobalEventCamera(pActor->mName, frames, endForce);
        }
    }

    void setProgrammableCameraParam(const LiveActor* pActor, const TVec3f& rWPoint, const TVec3f& rEye, const TVec3f& rUpVec) {
        if (pActor != nullptr) {
            MR::setProgrammableCameraParam(pActor->mName, rWPoint, rEye, rUpVec, true);
        }
    }

    void setProgrammableCameraParamFovy(const LiveActor* pActor, f32 fovy) {
        if (pActor != nullptr) {
            MR::setProgrammableCameraParamFovy(pActor->mName, fovy);
        }
    }
}  // namespace MR
