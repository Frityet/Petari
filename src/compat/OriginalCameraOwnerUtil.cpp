// Original CameraLocalUtil bodies while its full reference translation unit
// remains excluded. CameraUtil itself is compiled from Game/Util/CameraUtil.cpp.
#include "Game/Camera/Camera.hpp"
#include "Game/Camera/CameraDirector.hpp"
#include "Game/Camera/CameraLocalUtil.hpp"
#include "Game/Camera/CameraMan.hpp"
#include "Game/Camera/CameraRegisterHolder.hpp"
#include "Game/Camera/CameraParamChunk.hpp"
#include "Game/Camera/CameraPoseParam.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/MathUtil.hpp"

namespace CameraLocalUtil {
    const MtxPtr getMtxReg(const char* pRegName) {
        return getCameraDirector()->mRegisterHolder->getMtx(pRegName);
    }

    const TVec3f& getVecReg(const char* pRegName) {
        return *getCameraDirector()->mRegisterHolder->getVec(pRegName);
    }

    const char* getDummyVecRegName() {
        return getCameraDirector()->mRegisterHolder->getDummyVecRegName();
    }

    bool isForceCameraChange() {
        return getCameraDirector()->isForceCameraChange();
    }

    CameraDirector* getCameraDirector() {
        return MR::getCameraDirector();
    }

    void setUsedTarget(const CameraMan* pCameraMan, CameraTargetObj* pUsedTarget) {
        pCameraMan->mDirector->mTargetObj = pUsedTarget;
    }

    bool tryCameraReset() {
        if (getCameraDirector()->isEnableToReset()) {
            return testCameraPadButtonReset();
        }

        return false;
    }

    bool tryCameraResetTrigger() {
        if (!getCameraDirector()->isEnableToReset()) {
            return false;
        }

        if (getCameraDirector()->isSubjectiveCamera()) {
            return false;
        }

        return testCameraPadTriggerReset();
    }

    bool makeTowerCameraMtx(TPos3f* pMtx, const TPos3f& rMtx, const TVec3f& rPos, const TVec3f& rUp, const TVec3f& rWatchPos) {
        TVec3f pos = rPos;
        TVec3f up = rUp;
        rMtx.mult(pos, pos);
        rMtx.mult33(up);

        TVec3f diff = rWatchPos - pos;
        TVec3f front = diff.killElement(up);

        if (MR::isNearZero(front)) {
            return false;
        }

        MR::normalize(&front);
        TVec3f side = up.cross(front);
        pMtx->setXDir(side);
        pMtx->setYDir(up);
        pMtx->setZDir(front);
        pMtx->setTrans(rWatchPos);
        return true;
    }

    void arrangeDistanceByFovy(Camera* pCamera, TVec3f pos, f32 offset) {
        // FIXME: regswaps
        // https://decomp.me/scratch/jeret
        TVec3f viewDir = getPos(pCamera) - getWatchPos(pCamera);
        if (MR::isNearZero(viewDir)) {
            return;
        }

        f32 viewDist = viewDir.length();
        MR::normalize(&viewDir);
        TVec3f posDiff = pos - getWatchPos(pCamera);
        f32 projDist = posDiff.dot(viewDir);
        TVec3f projZ = viewDir * projDist;

        f32 dist = projDist + (offset + posDiff.distance(projZ)) / MR::tanDegree(MR::getFovy() * 0.5f);
        if (dist > viewDist) {
            setPos(pCamera, viewDir * dist + getWatchPos(pCamera));
        }
    }

    void arrangeDistanceByPushAndPull(Camera* pCamera, f32 push, f32 pull) {
        TVec3f viewDir = getPos(pCamera) - getWatchPos(pCamera);
        f32 viewDist = viewDir.length();

        if (viewDist < push) {
            if (MR::isNearZero(viewDir)) {
                MR::getCameraInvViewMtx().getZDir(viewDir);
                viewDir.negate();
            }
            viewDir.setLength(push);
        } else if (viewDist > pull) {
            viewDir.setLength(pull);
        }

        setPos(pCamera, getWatchPos(pCamera) + viewDir);
    }

} // namespace CameraLocalUtil
