// Original camera owner helpers from the reference CameraLocalUtil.cpp and
// CameraUtil.cpp. Existing scoped host target binding remains in its owner.
#include "Game/Camera/Camera.hpp"
#include "Game/Camera/CameraDirector.hpp"
#include "Game/Camera/CameraLocalUtil.hpp"
#include "Game/Camera/CameraMan.hpp"
#include "Game/Camera/CameraRegisterHolder.hpp"
#include "Game/Camera/CameraParamChunk.hpp"
#include "Game/Camera/CameraPoseParam.hpp"
#include "Game/Camera/CameraTargetArg.hpp"
#include "Game/LiveActor/ActorCameraInfo.hpp"
#include "Game/Player/MarioAccess.hpp"
#include "Game/MapObj/GCapture.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
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
    bool isPlayerDisableFpView() {
        return MarioAccess::isDisableFpView();
    }

    bool isFpViewChangingFailure() {
        return MarioAccess::isFpViewChangingFailure();
    }

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

namespace MR {
    CameraHolder* getCameraHolder() {
        return getCameraDirector()->getHolder();
    }

    void declareGlobalEventCamera(const char* pEventName) {
        getCameraDirector()->declareEvent(0, pEventName);
    }

    void declareGlobalEventCameraAbyss(const char* pEventName) {
        declareGlobalEventCamera(pEventName);
        CameraParamChunkEvent* chunk = MR::getCameraDirector()->getEventParameter(0, pEventName);

        if (chunk != nullptr) {
            chunk->setCameraType("CAM_TYPE_EYEPOS_FIX_THERE", MR::getCameraDirector()->mHolder);
            chunk->mGeneralParam->mNum1 = 1;
            chunk->_64 = true;
        }
    }

    void declareGlobalEventCameraDead(const char* pEventName, f32 dist, s32 time, s32 type) {
        declareGlobalEventCamera(pEventName);
        CameraParamChunkEvent* chunk = getCameraDirector()->getEventParameter(0, pEventName);
        if (chunk != nullptr) {
            chunk->setCameraType("CAM_TYPE_DEAD", getCameraDirector()->mHolder);
            chunk->mGeneralParam->mDist = dist;
            chunk->mGeneralParam->mNum1 = time;
            chunk->mGeneralParam->mNum2 = type;
            chunk->setLOfsErpOff(true);
            chunk->mExParam.setWOffset(TVec3f(0.0f, 0.0f, 0.0f));
            chunk->mExParam.mLOffsetV = 100.0f;
            chunk->_64 = true;
        }
    }

    void declareBlackHoleCamera(const char* pEventName) {
        declareGlobalEventCamera(pEventName);
        CameraParamChunkEvent* chunk = getCameraDirector()->getEventParameter(0, pEventName);
        if (chunk != nullptr) {
            chunk->setCameraType("CAM_TYPE_BLACK_HOLE", getCameraDirector()->mHolder);
            chunk->_64 = true;
            chunk->mEnableErpFrame = true;
            chunk->mExParam.mCamInt = 240;
            chunk->setCollisionOff(true);
        }
    }

    void setGameCameraTargetToPlayer() {
        CameraTargetArg camTarget = CameraTargetArg();
        setCameraTargetToPlayer(&camTarget);
        setGameCameraTarget(camTarget);
    }

    void setGameCameraTarget(const CameraTargetArg& rCamTarget) {
        rCamTarget.setTarget();
    }

    void startGlobalEventCamera(const char* pName, const CameraTargetArg& rCamTarget, s32 frame) {
        getCameraDirector()->startEvent(0, pName, rCamTarget, frame);
    }

    bool hasStartAnimCamera() {
        return getCameraDirector()->mStartCameraCreated;
    }

    void startStartAnimCamera() {
        getCameraDirector()->startStartAnimCamera();
    }

    s32 getStartAnimCameraFrame() {
        return getCameraDirector()->getStartAnimCameraFrame();
    }

    void endStartAnimCamera() {
        getCameraDirector()->endStartAnimCamera();
    }

    bool isCameraInterpolatingNearlyEnd() {
        return getCameraDirector()->isInterpolatingNearlyEnd();
    }

    void resetCameraLocalOffset() {
        getCameraDirector()->requestLocalOffsetReset();
    }

    void overlayWithPreviousScreen(u32 time) {
        getCameraDirector()->cover(time);
    }

    bool isSubjectiveCameraOnForObjClipping() {
        return getCameraDirector()->mSubjectiveFrame > 0;
    }

    const TVec3f& getCameraWatchPos() {
        return getCameraDirector()->mPoseParam1->mWatchPos;
    }

    void zoomInTargetGameCamera() {
        getCameraDirector()->zoomInGameCamera();
    }

    void zoomOutTargetGameCamera() {
        getCameraDirector()->zoomOutGameCamera();
    }

    void startTalkCamera(const TVec3f& rPosition, const TVec3f& rUp, f32 axisX, f32 axisY, s32 frame) {
        getCameraDirector()->startTalkCamera(rPosition, rUp, axisX, axisY, frame);
    }

    void endTalkCamera(bool resetView, s32 frame) {
        getCameraDirector()->endTalkCamera(resetView, frame);
    }

    void pauseOnAnimCamera(const ActorCameraInfo* pInfo, const char* pName) {
        getCameraDirector()->pauseOnAnimCamera(pInfo->mZoneID, pName);
    }

    void pauseOffAnimCamera(const ActorCameraInfo* pInfo, const char* pName) {
        getCameraDirector()->pauseOffAnimCamera(pInfo->mZoneID, pName);
    }
}

namespace MR {
    void stopPlayerFpView() {
        return MarioAccess::stopFpView();
    }

    bool isPlayerGCaptured() {
        if (!MR::isExistSceneObj(SceneObj_GCapture)) {
            return false;
        }
        GCapture* gCapture = static_cast< GCapture* >(MR::getSceneObjHolder()->getObj(SceneObj_GCapture));
        if (gCapture == nullptr) {
            return false;
        }

        return gCapture->_108;
    }

    void cleanEventCameraTarget_temporally() {
        CameraTargetArg camTarget = CameraTargetArg();
        setCameraTargetToPlayer(&camTarget);
        camTarget.setTarget();
    }
}

namespace MR {
    void startBlackHoleCamera(const char* pEventName, const TVec3f& rWPoint, const TVec3f& rPos) {
        CameraParamChunkEvent* chunk = getCameraDirector()->getEventParameter(0, pEventName);
        if (chunk != nullptr) {
            chunk->mGeneralParam->mWPoint.set(rWPoint);
            chunk->mGeneralParam->mAxis.set(rPos);
            startGlobalEventCameraNoTarget(pEventName, -1);
        }
    }

}
