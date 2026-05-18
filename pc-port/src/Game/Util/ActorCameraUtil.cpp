#include "Game/Util/ActorCameraUtil.hpp"

#include "Game/Camera/CameraTargetArg.hpp"
#include "Game/LiveActor/ActorCameraInfo.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/CameraUtil.hpp"

namespace MR {
    void initAnimCamera(const LiveActor* pActor, const ActorCameraInfo*, const char* pCameraName) {
        if (pActor != nullptr && pCameraName != nullptr) {
            MR::declareEventCameraProgrammable(pCameraName);
        }
    }

    void startAnimCameraTargetSelf(const LiveActor* pActor, const ActorCameraInfo*, const char* pCameraName, s32 frames, f32) {
        if (pActor != nullptr && pCameraName != nullptr) {
            MR::startGlobalEventCameraNoTarget(pCameraName, frames);
        }
    }

    void startAnimCameraTargetOther(const LiveActor* pActor, const ActorCameraInfo*, const char* pCameraName, const CameraTargetArg&, s32 frames, f32) {
        if (pActor != nullptr && pCameraName != nullptr) {
            MR::startGlobalEventCameraNoTarget(pCameraName, frames);
        }
    }

    void endAnimCamera(const LiveActor* pActor, const ActorCameraInfo*, const char* pCameraName, s32 frames, bool endForce) {
        if (pActor != nullptr && pCameraName != nullptr) {
            MR::endGlobalEventCamera(pCameraName, frames, endForce);
        }
    }

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
