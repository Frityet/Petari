#include "Game/Util/CameraUtil.hpp"
#include "Game/Camera/CameraAnim.hpp"
#include "Game/Camera/CameraCalc.hpp"
#include "Game/Camera/CameraContext.hpp"
#include "Game/Camera/CameraDirector.hpp"
#include "Game/Camera/CameraHolder.hpp"
#include "Game/Camera/CameraParamChunk.hpp"
#include "Game/Camera/CameraPoseParam.hpp"
#include "Game/Camera/CameraRegisterHolder.hpp"
#include "Game/Camera/CameraTargetArg.hpp"
#include "Game/LiveActor/ActorCameraInfo.hpp"
#include "Game/LiveActor/MirrorCamera.hpp"
#include "Game/Map/WaterAreaHolder.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include <cstdio>
#include <cstring>
#include "revolution/gx/GXEnum.h"
#include "revolution/mtx.h"

namespace MR {
    bool normalizeOrZero(TVec3f*);
    f32 tan(f32);
};

namespace {
    const char* sLauncherCameraName = "大砲";
    const char* sLauncherFlightCameraName = "大砲飛行";

    char* createRegisterName(const NameObj*, u32);

    void calcNormalizedScreenPosToScreenPos(TVec3f* pResult, const TVec3f& rNormalizedScreenPos) {
        f32 width = MR::getScreenWidth();
        f32 height = MR::getScreenHeight();
        pResult->set((0.5f * width) + (0.5f * (rNormalizedScreenPos.x * width)),
                     (0.5f * height) + (0.5f * (rNormalizedScreenPos.y * height)), rNormalizedScreenPos.z);
    }
};  // namespace

namespace MR {
    bool calcScreenPosition(TVec2f* pResult, const TVec3f& rViewMtxMult) {
        TVec3f normalizedScreenPos;
        TVec3f screenPos;
        TVec3f multedViewMtx;
        getSceneObj< CameraContext >(SceneObj_CameraContext)->getViewMtx()->mult(rViewMtxMult, multedViewMtx);
        bool ret = calcNormalizedScreenPositionFromView(&normalizedScreenPos, multedViewMtx);
        ::calcNormalizedScreenPosToScreenPos(&screenPos, normalizedScreenPos);
        pResult->x = screenPos.x;
        pResult->y = screenPos.y;
        return ret;
    }

    bool calcScreenPosition(TVec3f* pResult, const TVec3f& rViewMtxMult) {
        TVec3f normalizedScreenPos;
        TVec3f multedViewMtx;
        getSceneObj< CameraContext >(SceneObj_CameraContext)->getViewMtx()->mult(rViewMtxMult, multedViewMtx);
        bool ret = calcNormalizedScreenPositionFromView(&normalizedScreenPos, multedViewMtx);
        ::calcNormalizedScreenPosToScreenPos(pResult, normalizedScreenPos);
        return ret;
    }

    bool calcNormalizedScreenPosition(TVec3f* pResult, const TVec3f& rViewMtxMult) {
        TVec3f multedViewMtx;
        getSceneObj< CameraContext >(SceneObj_CameraContext)->getViewMtx()->mult(rViewMtxMult, multedViewMtx);
        return calcNormalizedScreenPositionFromView(pResult, multedViewMtx);
    }

    bool calcNormalizedScreenPositionFromView(TVec3f* pResult, const TVec3f& rViewMtxMult) {
        TProj3f* pProj = &getSceneObj< CameraContext >(SceneObj_CameraContext)->mProjection;
        TVec4f pos(rViewMtxMult.x * pProj->mMtx[0][0] + rViewMtxMult.z * pProj->mMtx[0][2],
                   rViewMtxMult.y * pProj->mMtx[1][1] + rViewMtxMult.z * pProj->mMtx[1][2], rViewMtxMult.z * pProj->mMtx[2][2] + pProj->mMtx[2][3],
                   -rViewMtxMult.z);
        pResult->scale(1.0f / pos.w, *pos.toTVec3());
        pResult->y = -pResult->y;

        if (1.0f < __fabs(pResult->x)) {
            return false;
        }

        if (1.0f < __fabs(pResult->y)) {
            return false;
        }

        return pResult->z <= 0.0f;
    }

    bool calcWorldPositionFromScreen(TVec3f* pResult, const TVec2f& rScreenPos, f32 depth) {
        f32 width = MR::getScreenWidth();
        f32 height = MR::getScreenHeight();
        TVec2f centerScreenPos(rScreenPos.x - (0.5f * width), rScreenPos.y - (0.5f * height));
        return calcWorldPositionFromCenterScreen(pResult, centerScreenPos, depth);
    }

    bool calcWorldPositionFromCenterScreen(TVec3f* pResult, const TVec2f& rCenterScreenPos, f32 depth) {
        f32 screenHeight = getScreenHeight();
        f32 defaultDepth = (0.5f * screenHeight) / MR::tan(0.5f * (0.017453292f * getFovy()));

        if (depth < 0.0f) {
            depth = defaultDepth;
        }

        f32 scale = depth / defaultDepth;
        TVec3f viewPos(rCenterScreenPos.x * scale, -rCenterScreenPos.y * scale, -depth);
        TVec3f worldPos;
        TPos3f invViewMtx = *getSceneObj< CameraContext >(SceneObj_CameraContext)->getInvViewMtx();
        invViewMtx.mult(viewPos, worldPos);

        if (pResult) {
            *pResult = worldPos;
        }

        return true;
    }

    bool calcWorldRayDirectionFromScreen(TVec3f* pResult, const TVec2f& a2) {
        bool ret = calcWorldPositionFromScreen(pResult, a2, -1.0f);
        pResult->sub(getCamPos());
        return ret;
    }

    f32 calcCameraDistanceZ(const TVec3f& rPos) {
        const TPos3f* pViewMtx = getSceneObj< CameraContext >(SceneObj_CameraContext)->getViewMtx();
        return __fabs((rPos.x * pViewMtx->mMtx[2][0]) + (rPos.y * pViewMtx->mMtx[2][1]) + (rPos.z * pViewMtx->mMtx[2][2]) + pViewMtx->mMtx[2][3]);
    }

    void loadProjectionMtx() {
        GXSetProjection(getSceneObj< CameraContext >(SceneObj_CameraContext)->mProjection, (GXProjectionType) nullptr);
    }

    void loadViewMtx() {
        PSMTXCopy((MtxPtr)getSceneObj< CameraContext >(SceneObj_CameraContext)->getViewMtx(), j3dSys.mViewMtx);
    }

    const MtxPtr getCameraViewMtx() {
        return (MtxPtr)getSceneObj< CameraContext >(SceneObj_CameraContext)->getViewMtx();
    }

    TPos3f* getCameraInvViewMtx() {
        return const_cast< TPos3f* >(getSceneObj< CameraContext >(SceneObj_CameraContext)->getInvViewMtx());
    }

    TProj3f* getCameraProjectionMtx() {
        return &getSceneObj< CameraContext >(SceneObj_CameraContext)->mProjection;
    }

    void setCameraViewMtx(const TPos3f& a1, bool a2, bool a3, const TVec3f& a4) {
        getSceneObj< CameraContext >(SceneObj_CameraContext)->setViewMtx(a1, a2, a3, a4);
    }

    f32 getAspect() {
        return MR::getSceneObj< CameraContext >(SceneObj_CameraContext)->getAspect();
    }

    f32 getNearZ() {
        return MR::getSceneObj< CameraContext >(SceneObj_CameraContext)->mNearZ;
    }

    f32 getFarZ() {
        return MR::getSceneObj< CameraContext >(SceneObj_CameraContext)->mFarZ;
    }

    f32 getFovy() {
        return MR::getSceneObj< CameraContext >(SceneObj_CameraContext)->mFovy;
    }

    void setNearZ(f32 nearZ) {
        MR::getSceneObj< CameraContext >(SceneObj_CameraContext)->setNearZ(nearZ);
    }

    void setFovy(f32 fovy) {
        MR::getSceneObj< CameraContext >(SceneObj_CameraContext)->setFovy(fovy);
    }

    void setShakeOffset(f32 a1, f32 a2) {
        MR::getSceneObj< CameraContext >(SceneObj_CameraContext)->setShakeOffset(a1, a2);
    }

    const TVec3f getCamPos() {
        TPos3f viewMtx = *MR::getSceneObj< CameraContext >(SceneObj_CameraContext)->getInvViewMtx();
        TVec3f pos;
        MR::extractMtxTrans(viewMtx.toMtxPtr(), &pos);
        return pos;
    }

    TVec3f getCamXdir() {
        TPos3f viewMtx = *MR::getSceneObj< CameraContext >(SceneObj_CameraContext)->getInvViewMtx();
        TVec3f dir;
        viewMtx.getXDir(dir);
        MR::normalizeOrZero(&dir);
        return dir;
    }

    TVec3f getCamYdir() {
        TPos3f viewMtx = *MR::getSceneObj< CameraContext >(SceneObj_CameraContext)->getInvViewMtx();
        TVec3f dir;
        viewMtx.getYDir(dir);
        MR::normalizeOrZero(&dir);
        return dir;
    }

    TVec3f getCamZdir() {
        TPos3f viewMtx = *MR::getSceneObj< CameraContext >(SceneObj_CameraContext)->getInvViewMtx();
        TVec3f dir;
        viewMtx.getZDir(dir);
        MR::normalizeOrZero(&dir);
        return -dir;
    }

    void createMirrorCamera() {
        MR::createSceneObj(SceneObj_MirrorCamera);
    }

    bool isExistMirrorCamera() {
        return MR::isExistSceneObj(SceneObj_MirrorCamera);
    }

    const MtxPtr getMirrorCameraViewMtx() {
        return getMirrorCamera()->mViewMtx;
    }

    const MtxPtr getMirrorModelTexMtx() {
        return getMirrorCamera()->mModelTexMtx;
    }

    void completeCameraParameters() {
        MR::getCameraDirector()->closeCreatingCameraChunk();
    }

    void resetCameraMan() {
        MR::getCameraDirector()->requestToResetCameraMan();
    }

    void startCameraInterpolation(u32 intr) {
        MR::getCameraDirector()->setInterpolation(intr);
    }

    void declareEventCamera(const ActorCameraInfo* pInfo, const char* pEventName) {
        MR::getCameraDirector()->declareEvent(pInfo->mZoneID, pEventName);
    }

    void endEventCamera(const ActorCameraInfo* pInfo, const char* pEventName, bool a3, s32 a4) {
        MR::getCameraDirector()->endEvent(pInfo->mZoneID, pEventName, a3, a4);
    }

    void endEventCameraAtLanding(const ActorCameraInfo* pInfo, const char* pName, s32 a3) {
        getCameraDirector()->endEventAtLanding(pInfo->mZoneID, pName, a3);
    }

    void declareGlobalEventCameraAbyss(const char* pEventName) {
        MR::getCameraDirector()->declareEvent(0, pEventName);
        CameraParamChunkEvent* chunk = MR::getCameraDirector()->getEventParameter(0, pEventName);

        if (chunk) {
            chunk->setCameraType("CAM_TYPE_EYEPOS_FIX_THERE", MR::getCameraDirector()->mHolder);
            chunk->mGeneralParam->mNum1 = 1;
            chunk->_64 = true;
        }
    }

    bool isEventCameraActive() {
        return getCameraDirector()->isEventCameraActive();
    }

    bool isEventCameraActive(const ActorCameraInfo* pInfo, const char* pEventName) {
        return getCameraDirector()->isEventCameraActive(pInfo->mZoneID, pEventName);
    }

    void declareGlobalEventCamera(const char* pEventName) {
        getCameraDirector()->declareEvent(0, pEventName);
    }

    void endGlobalEventCamera(const char* pEventName, s32 a2, bool a3) {
        getCameraDirector()->endEvent(0, pEventName, a3, a2);
    }

    void declareGlobalEventCameraFixedThere(const char* pEventName, bool a2, f32 a3) {
        getCameraDirector()->declareEvent(0, pEventName);
        CameraParamChunkEvent* pChunk = getCameraDirector()->getEventParameter(0, pEventName);
        if (pChunk) {
            pChunk->setCameraType("CAM_TYPE_EYEPOS_FIX_THERE", getCameraDirector()->mHolder);
            pChunk->mExParam.mLOffsetV = a3;
            if (a2) {
                pChunk->mGeneralParam->mNum1 = 1;
            }
            pChunk->_64 = true;
        }
    }

    void declareGlobalEventCameraDead(const char* pEventName, f32 dist, s32 num1, s32 num2) {
        getCameraDirector()->declareEvent(0, pEventName);
        CameraParamChunkEvent* pChunk = getCameraDirector()->getEventParameter(0, pEventName);

        if (pChunk) {
            pChunk->setCameraType("CAM_TYPE_DEAD", getCameraDirector()->mHolder);
            pChunk->mGeneralParam->mDist = dist;
            pChunk->mGeneralParam->mNum1 = num1;
            pChunk->mGeneralParam->mNum2 = num2;
            pChunk->setLOfsErpOff(true);
            TVec3f offset(0.0f, 0.0f, 0.0f);
            pChunk->mExParam.mWOffset.set(offset);
            pChunk->mExParam.mLOffsetV = 100.0f;
            pChunk->_64 = true;
        }
    }

    void declareEventCameraAnim(const ActorCameraInfo* pInfo, const char* pName, void* pData) {
        getCameraDirector()->declareEvent(pInfo->mZoneID, pName);
        CameraParamChunkEvent* pChunk = getCameraDirector()->getEventParameter(pInfo->mZoneID, pName);

        if (pChunk) {
            pChunk->setCameraType("CAM_TYPE_ANIM", getCameraDirector()->mHolder);
            pChunk->mGeneralParam->mNum1 = reinterpret_cast< s32 >(pData);
            pChunk->mGeneralParam->mDist = 1.0f;
            pChunk->mGeneralParam->mNum2 = CameraAnim::getAnimFrame(reinterpret_cast< u8* >(pData));
            pChunk->_64 = true;
        }
    }

    bool isAnimCameraEnd(const ActorCameraInfo* pInfo, const char* pName) {
        return getCameraDirector()->isAnimCameraEnd(pInfo->mZoneID, pName);
    }

    s32 getAnimCameraFrame(const ActorCameraInfo* pInfo, const char* pName) {
        CameraParamChunkEvent* pChunk = getCameraDirector()->getEventParameter(pInfo->mZoneID, pName);

        if (pChunk) {
            const char* pCameraType = "CAM_TYPE_ANIM";
            u8 cameraTypeIndex = pChunk->mCameraTypeIndex;
            CameraDirector* pDirector = getCameraDirector();
            s32 animCameraIndex = pDirector->mHolder->getIndexOf(pCameraType);

            if (cameraTypeIndex == animCameraIndex) {
                return pChunk->mGeneralParam->mNum2;
            }
        }

        return 0;
    }

    void pauseOnAnimCamera(const ActorCameraInfo* pInfo, const char* pName) {
        getCameraDirector()->pauseOnAnimCamera(pInfo->mZoneID, pName);
    }

    void pauseOffAnimCamera(const ActorCameraInfo* pInfo, const char* pName) {
        getCameraDirector()->pauseOffAnimCamera(pInfo->mZoneID, pName);
    }

    void declareBlackHoleCamera(const char* pEventName) {
        getCameraDirector()->declareEvent(0, pEventName);
        CameraParamChunkEvent* pChunk = getCameraDirector()->getEventParameter(0, pEventName);
        if (pChunk) {
            pChunk->setCameraType("CAM_TYPE_BLACK_HOLE", getCameraDirector()->mHolder);
            pChunk->_64 = true;
            pChunk->mEnableErpFrame = 1;
            pChunk->mExParam.mCamInt = 0xf0;
            pChunk->setCollisionOff(true);
        }
    }

    void startBlackHoleCamera(const char* pEventName, const TVec3f& a2, const TVec3f& a3) {
        CameraParamChunkEvent* pChunk = getCameraDirector()->getEventParameter(0, pEventName);
        if (pChunk) {
            pChunk->mGeneralParam->mWPoint.set(a2);
            pChunk->mGeneralParam->mAxis.set(a3);
            CameraTargetArg stack_8 = CameraTargetArg();
            getCameraDirector()->startEvent(0, pEventName, stack_8, -1);
        }
    }

    void declareLauncherCamera() {
        if (getCameraDirector()->getEventParameter(0, sLauncherCameraName)) {
            return;
        }

        getCameraDirector()->declareEvent(0, sLauncherCameraName);
        CameraParamChunkEvent* pChunk = getCameraDirector()->getEventParameter(0, sLauncherCameraName);

        if (pChunk) {
            pChunk->setCameraType("CAM_TYPE_DPD", getCameraDirector()->mHolder);
            pChunk->mGeneralParam->mDist = 120.0f;
            pChunk->mGeneralParam->mNum1 = 1;
            pChunk->mGeneralParam->mAngleA = 1.0471976f;
            pChunk->mGeneralParam->mAngleB = 0.5235988f;
            pChunk->mGeneralParam->mWPoint.z = 0.0f;
            pChunk->mGeneralParam->mWPoint.x = 0.05f;
            pChunk->mGeneralParam->mWPoint.y = 0.99f;
            pChunk->mGeneralParam->mUp.zero();
            pChunk->mGeneralParam->mNum2 = 0;
            pChunk->_64 = true;
        }
    }

    void endLauncherCamera() {
        getCameraDirector()->endEvent(0, sLauncherCameraName, true, -1);
    }

    void setLauncherCameraAngle(f32 angleB, f32 angleA, f32 wPointZ, f32 upX) {
        CameraParamChunkEvent* pChunk = getCameraDirector()->getEventParameter(0, sLauncherCameraName);

        if (pChunk) {
            pChunk->mGeneralParam->mAngleA = angleA;
            pChunk->mGeneralParam->mAngleB = angleB;
            pChunk->mGeneralParam->mWPoint.z = wPointZ;

            if (upX < 0.0f) {
                pChunk->mGeneralParam->mNum2 = 0;
            }
            else {
                pChunk->mGeneralParam->mNum2 = 1;
                pChunk->mGeneralParam->mUp.x = upX;
            }
        }
    }

    void declareLauncherFlightCamera() {
        if (getCameraDirector()->getEventParameter(0, sLauncherFlightCameraName)) {
            return;
        }

        getCameraDirector()->declareEvent(0, sLauncherFlightCameraName);
        CameraParamChunkEvent* pChunk = getCameraDirector()->getEventParameter(0, sLauncherFlightCameraName);

        if (pChunk) {
            pChunk->setCameraType("CAM_TYPE_OBJ_PARALLEL", getCameraDirector()->mHolder);
            TVec3f offset(0.0f, 0.0f, 0.0f);
            pChunk->mExParam.mWOffset.set(offset);
            pChunk->mExParam.mLOffset = 0.0f;
            pChunk->mGeneralParam->mDist = 900.0f;
            pChunk->mGeneralParam->mAngleA = 1.4f;
            pChunk->mGeneralParam->mAngleB = 3.1415927f;
            pChunk->setCollisionOff(true);
            pChunk->_64 = true;
        }
    }

    void endLauncherFlightCamera() {
        getCameraDirector()->endEvent(0, sLauncherFlightCameraName, true, -1);
    }

    bool isActiveLauncherCamera() {
        return getCameraDirector()->isEventCameraActive(0, sLauncherCameraName);
    }

    bool isActiveLauncherFlightCamera() {
        return getCameraDirector()->isEventCameraActive(0, sLauncherFlightCameraName);
    }

    void startSubjectiveCamera(s32 a1) {
        getCameraDirector()->startSubjectiveCamera(a1);
    }

    void endSubjectiveCamera(s32 a1) {
        getCameraDirector()->endSubjectiveCamera(a1);
    }

    void declareEventCameraProgrammable(const char* pEventName) {
        getCameraDirector()->declareEvent(0, pEventName);
        CameraParamChunkEvent* pChunk = getCameraDirector()->getEventParameter(0, pEventName);
        if (pChunk) {
            pChunk->setCameraType("CAM_TYPE_POINT_FIX", getCameraDirector()->mHolder);
            pChunk->_64 = true;
        }
    }

    void setProgrammableCameraParam(const char* pEventName, const TVec3f& rWPoint, const TVec3f& a3, const TVec3f& rUpVec, bool doZeroWOffset) {
        CameraParamChunkEvent* pChunk = getCameraDirector()->getEventParameter(0, pEventName);
        if (pChunk) {
            pChunk->mGeneralParam->mWPoint.set(rWPoint);
            crossToPolar(rWPoint, a3, (f32*)pChunk->mGeneralParam, &pChunk->mGeneralParam->mAxis.x, &pChunk->mGeneralParam->mAxis.y);
            pChunk->mGeneralParam->mUp.set(rUpVec);
            pChunk->setLOfsErpOff(doZeroWOffset);
            if (!doZeroWOffset) {
                pChunk->mExParam.mWOffset.zero();
            }
        }
    }

    void setProgrammableCameraParamFovy(const char* pEventName, f32 fovy) {
        CameraParamChunkEvent* pChunk = getCameraDirector()->getEventParameter(0, pEventName);
        if (pChunk) {
            pChunk->mExParam.mFovy = fovy;
            pChunk->setUseFovy(true);
        }
    }

    u32 getEventCameraFrames(const ActorCameraInfo* pInfo, const char* pEventName) {
        CameraParamChunkEvent* pChunk = getCameraDirector()->getEventParameter(pInfo->mZoneID, pEventName);
        if (pChunk) {
            return pChunk->mEvFrame;
        } else {
            return 0;
        }
    }
};  // namespace MR

namespace {
    char* createRegisterName(const NameObj* pObj, u32 index) {
        char buf[0x100];
        snprintf(buf, sizeof(buf), "%s-%d", pObj->mName, index);
        char* pName = new char[strlen(buf) + 1];
        strcpy(pName, buf);
        return pName;
    }
};  // namespace

namespace MR {
    void declareCameraRegisterMtx(const NameObj* pObj, u32 a2, MtxPtr mtx) {
        char* pName = createRegisterName(pObj, a2);
        getCameraDirector()->mRegisterHolder->declareMtxReg(pName, mtx);
    }

    void declareCameraRegisterVec(const NameObj* pObj, u32 a2, TVec3f* pVec) {
        char* pName = createRegisterName(pObj, a2);
        getCameraDirector()->mRegisterHolder->declareVecReg(pName, pVec);
    }

    void startStartPosCamera(bool a1) {
        getCameraDirector()->startStartPosCamera(a1);
    }

    void endStartPosCamera() {
        getCameraDirector()->started();
    }

    bool isStartPosCameraEnd() {
        return !getCameraDirector()->_170;
    }

    bool hasStartAnimCamera() {
        return getCameraDirector()->mStartCameraCreated;
    }

    void startStartAnimCamera() {
        getCameraDirector()->startStartAnimCamera();
    }

    bool isStartAnimCameraEnd() {
        return getCameraDirector()->isStartAnimCameraEnd();
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

    bool isFirstPersonCamera() {
        return getCameraDirector()->isSubjectiveCamera();
    }

    bool isPossibleToShiftToFirstPersonCamera() {
        bool canControl = false;
        bool ret = false;

        if (!isPlayerDisableFpView() && getCameraDirector()->isEnableToControl()) {
            canControl = true;
        }

        if (canControl && !getCameraDirector()->_170) {
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
        if (getCameraDirector()->_1F2 == false && isFpViewChangingFailure() == false) {
            ret = false;
        }
        return ret;
    }

    void startTalkCamera(const TVec3f& rPosition, const TVec3f& rUp, f32 axisX, f32 axisY, s32 a5) {
        getCameraDirector()->startTalkCamera(rPosition, rUp, axisX, axisY, a5);
    }

    void endTalkCamera(bool a1, s32 a2) {
        getCameraDirector()->endTalkCamera(a1, a2);
    }

    void pauseOnCameraDirector() {
        requestMovementOff(getCameraDirector());
    }

    void pauseOffCameraDirector() {
        requestMovementOn(getCameraDirector());
    }

    TVec3f* getCameraWatchPos() {
        return &getCameraDirector()->mPoseParam1->mWatchPos;
    }

    void zoomInTargetGameCamera() {
        getCameraDirector()->zoomInGameCamera();
    }

    void zoomOutTargetGameCamera() {
        getCameraDirector()->zoomOutGameCamera();
    }

    bool isCameraInWater() {
        return WaterAreaFunction::isCameraInWaterForCameraUtil();
    }

    void resetCameraLocalOffset() {
        CameraDirector* pDirector = getCameraDirector();
        pDirector->_1B1 = true;
    }

    void overlayWithPreviousScreen(u32 a1) {
        getCameraDirector()->cover(a1);
    }

    bool isSubjectiveCameraOnForObjClipping() {
        return getCameraDirector()->_1B4 > 0;
    }

    void setGameCameraTargetToPlayer() {
        CameraTargetArg camTarget = CameraTargetArg();
        setCameraTargetToPlayer(&camTarget);
        camTarget.setTarget();
    }

    void setGameCameraTarget(const CameraTargetArg& rCamTarget) {
        rCamTarget.setTarget();
    }

    void changeEventCameraTarget(const ActorCameraInfo* pInfo, const char* pEvetName, const CameraTargetArg& rCamTarget) {
        rCamTarget.setTarget();
    }

    void startEventCameraNoTarget(const ActorCameraInfo* pInfo, const char* pName, s32 a3) {
        CameraTargetArg camTarget = CameraTargetArg();
        getCameraDirector()->startEvent(pInfo->mZoneID, pName, camTarget, a3);
    }

    void startEventCameraTargetPlayer(const ActorCameraInfo* pInfo, const char* pName, s32 a3) {
        CameraTargetArg camTarget = CameraTargetArg();
        setCameraTargetToPlayer(&camTarget);
        getCameraDirector()->startEvent(pInfo->mZoneID, pName, camTarget, a3);
    }

    void startEventCamera(const ActorCameraInfo* pInfo, const char* pName, const CameraTargetArg& rCamTarget, s32 a4) {
        getCameraDirector()->startEvent(pInfo->mZoneID, pName, rCamTarget, a4);
    }

    void startGlobalEventCameraNoTarget(const char* pName, s32 a2) {
        CameraTargetArg camTarget = CameraTargetArg();
        getCameraDirector()->startEvent(0, pName, camTarget, a2);
    }

    void startGlobalEventCameraTargetPlayer(const char* pName, s32 a2) {
        CameraTargetArg camTarget = CameraTargetArg();
        setCameraTargetToPlayer(&camTarget);
        getCameraDirector()->startEvent(0, pName, camTarget, a2);
    }

    void startGlobalEventCamera(const char* pName, const CameraTargetArg& rCamTarget, s32 a3) {
        getCameraDirector()->startEvent(0, pName, rCamTarget, a3);
    }

    void startEventCameraAnim(const ActorCameraInfo* pInfo, const char* pEventName, const CameraTargetArg& rCamTarget, s32 a4, f32 a5) {
        CameraParamChunkEvent* pChunk = getCameraDirector()->getEventParameter(pInfo->mZoneID, pEventName);
        if (pChunk) {
            pChunk->mGeneralParam->mDist = a5;
        }
        getCameraDirector()->startEvent(pInfo->mZoneID, pEventName, rCamTarget, a4);
    }

    void startLauncherCamera(const CameraTargetArg& rCamTarget) {
        getCameraDirector()->startEvent(0, sLauncherCameraName, rCamTarget, 0);
    }

    void startLauncherFlightCamera(s32 frame) {
        const char* pCameraName = sLauncherFlightCameraName;
        CameraTargetArg camTarget = CameraTargetArg();
        setCameraTargetToPlayer(&camTarget);
        getCameraDirector()->startEvent(0, pCameraName, camTarget, frame);
    }

    void cleanEventCameraTarget_temporally() {
        CameraTargetArg camTarget = CameraTargetArg();
        setCameraTargetToPlayer(&camTarget);
        camTarget.setTarget();
    }
};  // namespace MR
