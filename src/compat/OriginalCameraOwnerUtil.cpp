// Original camera owner helpers from the reference CameraLocalUtil.cpp and
// CameraUtil.cpp. Existing scoped host target binding remains in its owner.
#include "Game/Camera/Camera.hpp"
#include "Game/Camera/CameraDirector.hpp"
#include "Game/Camera/CameraLocalUtil.hpp"
#include "Game/Camera/CameraMan.hpp"
#include "Game/Camera/CameraRegisterHolder.hpp"
#include "Game/Player/MarioAccess.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"

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

namespace MR {
    void startCameraInterpolation(u32 time) {
        MR::getCameraDirector()->setInterpolation(time);
    }

    bool isPlayerNeedBrakingCamera() {
        return MarioAccess::isNeedBrakingCamera();
    }

    void startSubjectiveCamera(s32 camType) {
        getCameraDirector()->startSubjectiveCamera(camType);
    }

    void endSubjectiveCamera(s32 camType) {
        getCameraDirector()->endSubjectiveCamera(camType);
    }

    bool isFirstPersonCameraOK() {
        return !MR::isPlayerDisableFpView() && getCameraDirector()->isEnableToControl();
    }

    bool isPossibleToShiftToFirstPersonCamera() {
        bool ret = false;
        if (isFirstPersonCameraOK() && !getCameraDirector()->mIsStartCameraActive) {
            ret = true;
        }
        return ret;
    }

    bool isCameraPossibleToRoundLeft() {
        return getCameraDirector()->isEnableToRoundLeft();
    }

    bool isCameraPossibleToRoundRight() {
        return getCameraDirector()->isEnableToRoundRight();
    }

    bool isCameraControlNG() {
        bool ret = true;
        if (getCameraDirector()->mIsCameraNG == false && isFpViewChangingFailure() == false) {
            ret = false;
        }
        return ret;
    }

} // namespace MR
