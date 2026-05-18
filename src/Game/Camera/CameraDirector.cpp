#include "Game/Camera/CameraDirector.hpp"
#include "Game/Boss/BossStinkBug.hpp"
#include "Game/Camera/CameraCover.hpp"
#include "Game/Camera/CameraHolder.hpp"
#include "Game/Camera/CameraLocalUtil.hpp"
#include "Game/Camera/CameraMan.hpp"
#include "Game/Camera/CameraManEvent.hpp"
#include "Game/Camera/CameraManGame.hpp"
#include "Game/Camera/CameraManPause.hpp"
#include "Game/Camera/CameraManSubjective.hpp"
#include "Game/Camera/CameraParamChunk.hpp"
#include "Game/Camera/CameraParamChunkHolder.hpp"
#include "Game/Camera/CameraPoseParam.hpp"
#include "Game/Camera/CameraRailHolder.hpp"
#include "Game/Camera/CameraRegisterHolder.hpp"
#include "Game/Camera/CameraRotChecker.hpp"
#include "Game/Camera/CameraShaker.hpp"
#include "Game/Camera/CameraTargetHolder.hpp"
#include "Game/Camera/CameraTargetMtx.hpp"
#include "Game/Camera/CameraViewInterpolator.hpp"
#include "Game/Camera/GameCameraCreator.hpp"
#include "Game/Camera/OnlyCamera.hpp"
#include "Game/LiveActor/ActorCameraInfo.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/SequenceUtil.hpp"
#include <cstring>

namespace {
    static f32 sDefaultFovy = 45.0f;
    static const char* sTalkCameraName = "共通会話カメラ";
    static const char* sStartAnimCameraName = "スタートアニメカメラ";
    static const char* sSubjectiveCameraName = "主観カメラ";
    static s32 sUpdateCounter;
};  // namespace

void CameraPoseParam::copyFrom(const CameraPoseParam& rOther) {
    mWatchUpVec.set< f32 >(rOther.mWatchUpVec);
    mWatchPos.set< f32 >(rOther.mWatchPos);
    mUpVec.set< f32 >(rOther.mUpVec);
    mPos.set< f32 >(rOther.mPos);
    mFovy = rOther.mFovy;
    mGlobalOffset.set< f32 >(rOther.mGlobalOffset);
    mLocalOffset.set< f32 >(rOther.mLocalOffset);
    mFrontOffset = rOther.mFrontOffset;
    mUpperOffset = rOther.mUpperOffset;
    mRoll = rOther.mRoll;
}

char* CameraParamChunkID_Tmp::getBuffer(u32 size) {
    return &mBuffer[0];
}

bool CameraMan::isInterpolationOff() const {
    return false;
}

bool CameraMan::isCollisionOff() const {
    return false;
}

bool CameraMan::isZeroFrameMoveOff() const {
    return false;
}

bool CameraMan::isSubjectiveCameraOff() const {
    return false;
}

bool CameraMan::isCorrectingErpPositionOff() const {
    return false;
}

bool CameraMan::isEnableToReset() const {
    return false;
}

bool CameraMan::isEnableToRoundLeft() const {
    return false;
}

bool CameraMan::isEnableToRoundRight() const {
    return false;
}

void CameraMan::roundLeft() {
}

void CameraMan::roundRight() {
}

CameraDirector::CameraDirector(const char* pName) : NameObj(pName) {
    mUsedTarget = nullptr;
    mStack = new CameraManStack();
    mOnlyCamera = new OnlyCamera("OnlyCamera");
    mPoseParam1 = new CameraPoseParam();
    mPoseParam2 = new CameraPoseParam();
    mHolder = new CameraHolder("カメラホルダー");
    mChunkHolder = new CameraParamChunkHolder(mHolder, "パラメータ");
    mCameraCreator = new GameCameraCreator(mChunkHolder);
    mRailHolder = new CameraRailHolder("カメラレール管理");
    mRegisterHolder = new CameraRegisterHolder("カメラレジスタ");
    mTargetHolder = new CameraTargetHolder();
    mShaker = new CameraShaker("振動");
    mViewInterpolator = new CameraViewInterpolator();
    mCover = new CameraCover("CameraCover");
    mRotChecker = new CameraRotChecker();
    mCameraManGame = new CameraManGame(mHolder, mChunkHolder, "ゲームカメラマン");
    mCameraManEvent = new CameraManEvent(mHolder, mChunkHolder, "イベントカメラマン");
    mCameraManPause = new CameraManPause("ポーズカメラマン");
    mCameraManSubjective = new CameraManSubjective("主観カメラマン");
    _58 = false;
    mEndEventAtLandingCount = 0;
    _170 = true;
    _174 = 0;
    mStartCameraCreated = false;
    mTargetMatrix = new CameraTargetMtx("カメラターゲットダミー");
    mRequestCameraManReset = false;
    _1B1 = false;
    mIsSubjectiveCamera = false;
    _1B3 = false;
    _1B4 = 0;
    _1BC = -100.0f;
    _1F0 = false;
    _1F1 = true;
    _1F2 = false;

    MR::connectToSceneCamera(this);
    push(mCameraManGame);
    _180.identity();
    mTargetMatrix->mMatrix.setInline(_180);
    setInterpolation(0);
    mCameraManSubjective->owned(this);
    _1C0.identity();
    MR::createCenterScreenBlur();
}

void CameraDirector::init(const JMapInfoIter& rIter) {
}

void CameraDirector::movement() {
    sUpdateCounter++;

    backLastMtx();
    mTargetHolder->movement();
    updateCameraMan();
    calcPose();
    createViewMtx();
    getCurrentCameraMan()->mMatrix.setInline(MR::getCameraInvViewMtx());
    mPoseParam2->copyFrom(*getCurrentCameraMan()->mPoseParam);
    calcSubjective();
    mShaker->movement();
    checkStartCondition();
    checkEndOfEventCamera();
    mRotChecker->update();

    mRequestCameraManReset = false;
    _1B1 = false;
}

void CameraDirector::setTarget(CameraTargetObj* pTarget) {
    mTargetHolder->set(pTarget);
}

CameraTargetObj* CameraDirector::getTarget() {
    return mTargetHolder->get();
}

void CameraDirector::push(CameraMan* pMan) {
    if (mStack->mCount != 0) {
        CameraMan* man = getCurrentCameraMan();
        man->deactivate(this);
    }

    mStack->mElements[mStack->mCount++] = pMan;

    pMan->owned(this);
    pMan->activate(this);
}

CameraMan* CameraDirector::pop() {
    CameraMan* man = mStack->mElements[--mStack->mCount];

    man->deactivate(this);
    man->released(this);

    if (mStack->mCount != 0) {
        CameraMan* newMan = getCurrentCameraMan();
        newMan->activate(this);
    }

    return man;
}

void CameraDirector::backLastMtx() {
    if (_1F0) {
        MR::setCameraViewMtx(_1C0, false, false, TVec3f(0.0f, 0.0f, 0.0f));
        TPos3f* invView = MR::getCameraInvViewMtx();

        TVec3f invTrans;
        invView->getTrans(invTrans);
    }
}

CameraMan* CameraDirector::getCurrentCameraMan() const {
    return mStack->mElements[mStack->mCount - 1];
}

void CameraDirector::updateCameraMan() {
    if (mRequestCameraManReset) {
        resetCameraMan();
    }

    if (_1B1) {
        CameraMan* man = getCurrentCameraMan();
        man->_15 = true;
    }

    getCurrentCameraMan()->movement();
    controlCameraSE();
}

void CameraDirector::calcPose() {
    switchAntiOscillation();

    if (getCurrentCameraMan()->isCollisionOff()) {
        mViewInterpolator->_7C = true;
    }

    if (getCurrentCameraMan()->isCorrectingErpPositionOff()) {
        mViewInterpolator->_8A = false;
    }

    if (getCurrentCameraMan()->isZeroFrameMoveOff()) {
        mOnlyCamera->_3C = true;
    }

    mOnlyCamera->calcPose(getCurrentCameraMan());
    mPoseParam1->copyFrom(*mOnlyCamera->mPoseParam);
}

void CameraDirector::calcSubjective() {
    JMath::gekko_ps_copy12(&_1C0, MR::getCameraViewMtx());
    _1F0 = true;

    if (MR::isDemoActive()) {
        MR::stopPlayerFpView();
    }

    if (mIsSubjectiveCamera) {
        _1B4++;

        if (_1B4 > 20) {
            _1B4 = 20;
        }
    } else {
        _1B4--;

        if (_1B4 <= 0) {
            _1B4 = 0;

            if (_1B3) {
                if (_1BC >= 0.0f) {
                    MR::setNearZ(_1BC);
                }

                _1BC = -100.0f;
                MR::turnOnDOFInSubjective();
                _1B3 = false;
            }
        }
    }

    if (!_1B3) {
        return;
    }

    TPos3f curInvView;
    JMath::gekko_ps_copy12(&curInvView, MR::getCameraInvViewMtx());

    mCameraManSubjective->movement();

    TPos3f subjectiveMtx;
    calcViewMtxFromPoseParam(&subjectiveMtx, mCameraManSubjective->mPoseParam);

    s32 blendFrame = _1B4;
    if (blendFrame > 20) {
        blendFrame = 20;
    }

    f32 posRate;
    if (_1B4 >= 20) {
        posRate = 1.0f;
    } else {
        posRate = 0.5f + 0.5f * MR::cos(3.1415927f + (3.1415927f * blendFrame) / 20.0f);
    }

    TVec3f curTrans;
    curInvView.getTrans(curTrans);
    TVec3f blendedTrans = curTrans * (1.0f - posRate) + mCameraManSubjective->mPoseParam->mPos * posRate;

    s32 quatFrame = _1B4 - 10;
    if (quatFrame < 0) {
        quatFrame = 0;
    }

    f32 quatRate = 0.5f + 0.5f * MR::cos(3.1415927f + (3.1415927f * quatFrame) / 10.0f);

    TQuat4f curQuat;
    TQuat4f subjectiveQuat;
    TQuat4f blendedQuat;
    curInvView.getQuat(curQuat);
    subjectiveMtx.getQuat(subjectiveQuat);
    blendedQuat.slerp(curQuat, subjectiveQuat, quatRate);

    subjectiveMtx.zeroTrans();
    subjectiveMtx.setQuat(blendedQuat);
    subjectiveMtx.setTrans(blendedTrans);
    subjectiveMtx.invert(subjectiveMtx);

    MR::setCameraViewMtx(subjectiveMtx, false, false, TVec3f(0.0f, 0.0f, 0.0f));
}

bool CameraDirector::isInterpolationOff() {
    bool off = false;

    CameraMan* man = getCurrentCameraMan();

    if (man->isInterpolationOff() || mViewInterpolator->_9) {
        off = true;
    }

    return off;
}

void CameraDirector::switchAntiOscillation() {
    if (isInterpolationOff()) {
        mViewInterpolator->_8 = false;
    } else {
        mViewInterpolator->_8 = true;
    }
}

void CameraDirector::createViewMtx() {
    TPos3f view;
    calcViewMtxFromPoseParam(&view, mPoseParam1);

    CameraPoseParam* poseParam = mPoseParam1;
    CameraTargetObj* target = mUsedTarget;
    TVec3f& rWatchPos = poseParam->mWatchPos;

    CameraMan* man = getCurrentCameraMan();
    f32 fovy = CameraLocalUtil::getFovy(man);
    mViewInterpolator->updateCameraMtx(reinterpret_cast< MtxPtr >(&view), rWatchPos, target, fovy);
}

void CameraDirector::checkStartCondition() {
    if (_170 && getCurrentCameraMan() == mCameraManGame && _174++ > 30 && mTargetHolder->isMoving()) {
        _170 = false;
        mCameraManGame->endStartPosCamera();
    }
}

void CameraDirector::startEvent(s32 zoneID, const char* pName, const CameraTargetArg& rTargetArg, s32 a4) {
    mViewInterpolator->_A = false;
    removeEndEventAtLanding(zoneID, pName);

    if (getCurrentCameraMan() != mCameraManEvent) {
        if (mStack->mCount != 0) {
            CameraMan* gameMan = mCameraManGame;

            if (getCurrentCameraMan() == gameMan) {
                mCameraManEvent->mPoseParam->copyFrom(*gameMan->mPoseParam);
                mCameraManEvent->mMatrix.setInline(MR::getCameraInvViewMtx());
            }
        }

        push(mCameraManEvent);
    }

    mCameraManEvent->start(zoneID, pName, rTargetArg, a4);
}

void CameraDirector::endEvent(s32 zoneID, const char* pName, bool a3, s32 a4) {
    if (getCurrentCameraMan() == mCameraManEvent) {
        mCameraManEvent->end(zoneID, pName, a4);

        if (!mCameraManEvent->isActive()) {
            pop();

            if (!mViewInterpolator->_9 && a3 && getCurrentCameraMan() == mCameraManGame) {
                mCameraManGame->mPoseParam->copyFrom(*mPoseParam1);
                mCameraManGame->mMatrix.setInline(MR::getCameraInvViewMtx());
            }

            mViewInterpolator->_A = true;
        }
    }
}

void CameraDirector::endEventAtLanding(s32 zoneID, const char* pName, s32 interpolateFrame) {
    if (getCurrentCameraMan() == mCameraManEvent) {
        mEndEventsAtLanding[mEndEventAtLandingCount].mZoneID = zoneID;
        strcpy(mEndEventsAtLanding[mEndEventAtLandingCount].mName, pName);
        mEndEventsAtLanding[mEndEventAtLandingCount].mInterpolateFrame = interpolateFrame;
        mEndEventAtLandingCount++;
    }
}

CameraParamChunkEvent* CameraDirector::getEventParameter(s32 zoneID, const char* pName) {
    CameraParamChunkID_Tmp chunkID = CameraParamChunkID_Tmp();
    chunkID.createEventID(zoneID, pName);

    return reinterpret_cast< CameraParamChunkEvent* >(mChunkHolder->getChunk(chunkID));
}

void CameraDirector::requestToResetCameraMan() {
    mRequestCameraManReset = true;
}

void CameraDirector::setInterpolation(u32 a1) {
    mViewInterpolator->setInterpolation(a1);

    if (a1 == 0 && !_170) {
        mViewInterpolator->_7C = true;
        mCover->cover(2);
    }
}

void CameraDirector::cover(u32 a1) {
    mCover->cover(a1);
}

void CameraDirector::closeCreatingCameraChunk() {
    mCameraCreator->scanArea();
    mCameraCreator->scanStartPos();
    createStartAnimCamera();
    createTalkCamera();
    createSubjectiveCamera();
    mCameraManGame->closeCreatingCameraChunk();
    mChunkHolder->loadCameraParameters();
    mChunkHolder->sort();
    _170 = true;
    mCameraManGame->startStartPosCamera(false);
}

void CameraDirector::initCameraCodeCollection(const char* a1, s32 a2) {
    mCameraCreator->initCameraCodeCollection(a1, a2);
}

void CameraDirector::registerCameraCode(u32 code) {
    mCameraCreator->registerCameraCode(code);
}

void CameraDirector::termCameraCodeCollection() {
    mCameraCreator->termCameraCodeCollection();
}

void CameraDirector::declareEvent(s32 zoneID, const char* pName) {
    CameraParamChunkID_Tmp chunkID = CameraParamChunkID_Tmp();
    chunkID.createEventID(zoneID, pName);

    mChunkHolder->createChunk(chunkID, nullptr);
}

void CameraDirector::started() {
    _170 = false;
    mCameraManGame->endStartPosCamera();
}

void CameraDirector::setTargetActor(const LiveActor* pActor) {
    mTargetHolder->set(pActor);
}

void CameraDirector::setTargetPlayer(const MarioActor* pActor) {
    mTargetHolder->set(pActor);
}

bool CameraDirector::isRotatingHard() const {
    return mRotChecker->_30;
}

bool CameraDirector::isSubjectiveCamera() const {
    return mIsSubjectiveCamera;
}

bool CameraDirector::isEnableToControl() const {
    bool enable = false;

    if (!getCurrentCameraMan()->isSubjectiveCameraOff()) {
        bool change = true;
        bool equals = getCurrentCameraMan() == mCameraManEvent;

        if (equals && !isEventCameraActive(0, sSubjectiveCameraName)) {
            change = false;
        }

        if (change) {
            enable = true;
        }
    }

    return enable;
}

bool CameraDirector::isEnableToRoundLeft() const {
    return getCurrentCameraMan()->isEnableToRoundLeft();
}

bool CameraDirector::isEnableToRoundRight() const {
    return getCurrentCameraMan()->isEnableToRoundRight();
}

bool CameraDirector::isEnableToReset() const {
    return getCurrentCameraMan()->isEnableToReset();
}

bool CameraDirector::isEventCameraActive(s32 zoneID, const char* pName) const {
    if (getCurrentCameraMan() == mCameraManEvent) {
        return mCameraManEvent->isEventActive(zoneID, pName);
    }

    return false;
}

bool CameraDirector::isEventCameraActive() const {
    return getCurrentCameraMan() == mCameraManEvent;
}

void CameraDirector::startStartPosCamera(bool a1) {
    _170 = true;
    mCameraManGame->startStartPosCamera(a1);
}

bool CameraDirector::isInterpolatingNearlyEnd() const {
    if (getCurrentCameraMan() == mCameraManEvent && mCameraManEvent->doesNextChunkHaveInterpolation()) {
        return false;
    }

    return mViewInterpolator->isInterpolatingNearlyEnd();
}

bool CameraDirector::isForceCameraChange() const {
    return mViewInterpolator->_9;
}

f32 CameraDirector::getDefaultFovy() const {
    return sDefaultFovy;
}

void CameraDirector::startStartAnimCamera() {
    if (mStartCameraCreated) {
        ActorCameraInfo info = ActorCameraInfo(-1, 0);
        CameraTargetArg targetArg = CALL_INLINE_FUNC(CameraTargetArg, mTargetMatrix);

        MR::startEventCamera(&info, sStartAnimCameraName, targetArg, 0);
    }
}

bool CameraDirector::isStartAnimCameraEnd() const {
    if (mStartCameraCreated) {
        return isAnimCameraEnd(0, sStartAnimCameraName);
    }

    return true;
}

u32 CameraDirector::getStartAnimCameraFrame() const {
    if (mStartCameraCreated) {
        return getAnimCameraFrame(0, sStartAnimCameraName);
    }

    return 0;
}

void CameraDirector::endStartAnimCamera() {
    ActorCameraInfo info = ActorCameraInfo(-1, 0);
    MR::endEventCamera(&info, sStartAnimCameraName, true, 0);
}

// FIXME: Erroneously-ordered lwz instruction.
void CameraDirector::startTalkCamera(const TVec3f& rPosition, const TVec3f& rUp, f32 axisX, f32 axisY, s32 a5) {
    CameraParamChunkID_Tmp chunkID = CameraParamChunkID_Tmp();
    chunkID.createEventID(0, sTalkCameraName);

    CameraParamChunk* chunk = mChunkHolder->getChunk(chunkID);

    if (chunk != nullptr) {
        chunk->mGeneralParam->mWPoint.set< f32 >(rPosition);
        chunk->mGeneralParam->mUp.set< f32 >(rUp);

        CameraGeneralParam* generalParam = chunk->mGeneralParam;
        generalParam->mAxis.x = axisX;
        generalParam->mAxis.y = axisY;
        generalParam->mAxis.z = 0.0f;

        CameraTargetArg targetArg = CALL_INLINE_FUNC_NO_ARG(CameraTargetArg);

        MR::setCameraTargetToPlayer(&targetArg);
        startEvent(0, sTalkCameraName, targetArg, a5);
    }
}

void CameraDirector::endTalkCamera(bool a1, s32 a2) {
    endEvent(0, sTalkCameraName, a1, a2);
}

void CameraDirector::startSubjectiveCamera(s32 a1) {
    _170 = false;
    mCameraManGame->endStartPosCamera();
    mIsSubjectiveCamera = true;

    if (!_1B3) {
        _1B3 = true;
        _1B4 = 0;

        mCameraManSubjective->activate(this);
        f32 nearZ = MR::getNearZ();
        _1BC = nearZ;

        MR::setNearZ(10.0f);
        MR::turnOffDOFInSubjective();
    }

    if (_1B4 < 20) {
        MR::startCenterScreenBlur(20, 15.0f, 80, 5, 10);
    }
}

void CameraDirector::endSubjectiveCamera(s32 a1) {
    bool bVar1 = a1 == 0 || a1 == 1;

    if (mIsSubjectiveCamera == true) {
        mIsSubjectiveCamera = false;

        if (!bVar1) {
            MR::startCenterScreenBlur(_1B4, 15.0f, 80, 5, 10);
        }
    }

    if (_1B3 && bVar1) {
        _1B4 = 0;
    }
}

bool CameraDirector::isAnimCameraEnd(s32 zoneID, const char* pName) const {
    if (getCurrentCameraMan() == mCameraManEvent) {
        return mCameraManEvent->isAnimCameraEnd(zoneID, pName);
    }

    return true;
}

u32 CameraDirector::getAnimCameraFrame(s32 zoneID, const char* pName) const {
    if (getCurrentCameraMan() == mCameraManEvent) {
        return mCameraManEvent->getAnimCameraFrame(zoneID, pName);
    }

    return 0;
}

void CameraDirector::pauseOnAnimCamera(s32 zoneID, const char* pName) {
    if (getCurrentCameraMan() == mCameraManEvent) {
        mCameraManEvent->pauseOnAnimCamera(zoneID, pName);
    }
}

void CameraDirector::pauseOffAnimCamera(s32 zoneID, const char* pName) {
    if (getCurrentCameraMan() == mCameraManEvent) {
        mCameraManEvent->pauseOffAnimCamera(zoneID, pName);
    }
}

void CameraDirector::zoomInGameCamera() {
    mCameraManGame->zoomIn();
}

void CameraDirector::zoomOutGameCamera() {
    mCameraManGame->zoomOut();
}

void CameraDirector::checkEndOfEventCamera() {
    if (mEndEventAtLandingCount != 0 && mTargetHolder->isOnGround()) {
        for (u32 i = 0; i < mEndEventAtLandingCount; i++) {
            endEvent(mEndEventsAtLanding[i].mZoneID, mEndEventsAtLanding[i].mName, true, mEndEventsAtLanding[i].mInterpolateFrame);
        }

        mEndEventAtLandingCount = 0;
    }
}

void CameraDirector::controlCameraSE() {
    _1F2 = false;

    if (MR::isPlayerDead()) {
        return;
    }

    if (mIsSubjectiveCamera) {
        if (MR::testCorePadTriggerLeft(WPAD_CHAN0) || MR::testCorePadTriggerRight(WPAD_CHAN0) || MR::testFpViewStartTrigger()) {
            if (isPlayableCameraSE(false)) {
                MR::startSystemSE("SE_SY_CAMERA_NG", -1, -1);
                _1F2 = true;
            }
        }

        return;
    }

    if (CameraLocalUtil::testCameraPadTriggerRoundLeft()) {
        if (getCurrentCameraMan()->isEnableToRoundLeft()) {
            getCurrentCameraMan()->roundLeft();

            if (isPlayableCameraSE(false)) {
                MR::startSystemSE("SE_SY_CAMERA_MOVE", -1, -1);
            }
        } else if (isPlayableCameraSE(false)) {
            MR::startSystemSE("SE_SY_CAMERA_NG", -1, -1);
            _1F2 = true;
        }
    }

    if (CameraLocalUtil::testCameraPadTriggerRoundRight()) {
        if (getCurrentCameraMan()->isEnableToRoundRight()) {
            getCurrentCameraMan()->roundRight();

            if (isPlayableCameraSE(false)) {
                MR::startSystemSE("SE_SY_CAMERA_MOVE", -1, -1);
            }
        } else if (isPlayableCameraSE(false)) {
            MR::startSystemSE("SE_SY_CAMERA_NG", -1, -1);
            _1F2 = true;
        }
    }

    if (CameraLocalUtil::testCameraPadTriggerReset()) {
        if (getCurrentCameraMan()->isEnableToReset()) {
            if (isPlayableCameraSE(false)) {
                MR::startSystemSE("SE_SY_CAMERA_RESET", -1, -1);
                MR::startSystemSE("SE_SY_CAMERA_MOVE", -1, -1);
            }
        } else if (isPlayableCameraSE(false)) {
            MR::startSystemSE("SE_SY_CAMERA_NG", -1, -1);
            _1F2 = true;
        }
    }

    if (MR::isPlayerInBind() && MR::testFpViewStartTrigger() && isPlayableCameraSE(false)) {
        MR::startSystemSE("SE_SY_CAMERA_NG", -1, -1);
        _1F2 = true;
    }

    if (MR::testCorePadTriggerDown(WPAD_CHAN0) && isPlayableCameraSE(false)) {
        MR::startSystemSE("SE_SY_CAMERA_NG", -1, -1);
        _1F2 = true;
    }
}

void CameraDirector::removeEndEventAtLanding(s32 zoneID, const char* pName) {
    for (u32 i = 0; i < mEndEventAtLandingCount; i++) {
        if (mEndEventsAtLanding[i].mZoneID == zoneID && strcmp(mEndEventsAtLanding[i].mName, pName) == 0) {
            u32 lastIdx = mEndEventAtLandingCount - 1;

            if (lastIdx == i) {
                mEndEventAtLandingCount = 0;
                return;
            }

            mEndEventsAtLanding[i].mZoneID = mEndEventsAtLanding[lastIdx].mZoneID;
            strcpy(mEndEventsAtLanding[i].mName, mEndEventsAtLanding[lastIdx].mName);
            mEndEventsAtLanding[i].mInterpolateFrame = mEndEventsAtLanding[lastIdx].mInterpolateFrame;
            mEndEventAtLandingCount--;
            return;
        }
    }
}

void CameraDirector::calcViewMtxFromPoseParam(TPos3f* pMtx, const CameraPoseParam* pPoseParam) {
    TVec3f zDir = pPoseParam->mWatchPos - pPoseParam->mPos;
    MR::normalizeOrZero(&zDir);

    TVec3f xDir;
    PSVECCrossProduct(&pPoseParam->mUpVec, &zDir, &xDir);
    MR::normalizeOrZero(&xDir);

    TVec3f yDir;
    PSVECCrossProduct(&zDir, &xDir, &yDir);
    MR::normalizeOrZero(&yDir);

    TVec3f negXDir = -xDir;
    pMtx->mMtx[0][0] = negXDir.x;
    pMtx->mMtx[1][0] = negXDir.y;
    pMtx->mMtx[2][0] = negXDir.z;
    pMtx->mMtx[0][1] = yDir.x;
    pMtx->mMtx[1][1] = yDir.y;
    pMtx->mMtx[2][1] = yDir.z;
    TVec3f negZDir = -zDir;
    pMtx->mMtx[0][2] = negZDir.x;
    pMtx->mMtx[1][2] = negZDir.y;
    pMtx->mMtx[2][2] = negZDir.z;
    pMtx->setTrans(pPoseParam->mPos);

    TPos3f rollMtx;
    rollMtx.zeroTrans();
    rollMtx.setRotateInline(TVec3f(0.0f, 0.0f, 1.0f), pPoseParam->mRoll);
    pMtx->concat(*pMtx, rollMtx);
}

bool CameraDirector::isPlayableCameraSE(bool a1) {
    if (MR::isDemoActive()) {
        return false;
    }

    if (MR::isPowerStarGetDemoActive()) {
        return false;
    }

    if (MR::isStageStateScenarioOpeningCamera()) {
        return false;
    }

    if (MR::isExecScenarioStarter()) {
        return false;
    }

    if (MR::isPlayerDead()) {
        return false;
    }

    if (MR::isEqualStageName("FileSelect")) {
        return false;
    }

    if (MR::isEqualStageName("EpilogueDemoStage")) {
        return false;
    }

    if (a1 && mIsSubjectiveCamera) {
        return false;
    }

    return true;
}

void CameraDirector::resetCameraMan() {
    setInterpolation(0);

    CameraTargetObj* target1;
    CameraTargetObj* target2;
    CameraTargetObj* target3;
    CameraMan* man = getCurrentCameraMan();

    target1 = mTargetHolder->get();
    target2 = mTargetHolder->get();
    target3 = mTargetHolder->get();

    TVec3f newPos = *target3->getPosition() - *target2->getFrontVec() * 800.0f + *target1->getUpVec() * 300.0f;

    CameraLocalUtil::setPos(man, newPos);
    CameraLocalUtil::setWatchPos(man, *mTargetHolder->get()->getPosition());
    CameraLocalUtil::setUpVec(man, *mTargetHolder->get()->getUpVec());
    CameraLocalUtil::setWatchUpVec(man, *mTargetHolder->get()->getUpVec());

    man->deactivate(this);
    man->activate(this);

    mOnlyCamera->_3D = true;
}

void CameraDirector::createStartAnimCamera() {
    void* data = nullptr;
    s32 size = 0;
    MR::getCurrentScenarioStartAnimCameraData(&data, &size);

    if (size > 0) {
        ActorCameraInfo info = ActorCameraInfo(-1, 0);
        MR::declareEventCameraAnim(&info, sStartAnimCameraName, data);
        mStartCameraCreated = true;
    }
}

void CameraDirector::createTalkCamera() {
    const char* name = sTalkCameraName;
    CameraParamChunkID_Tmp chunkID = CameraParamChunkID_Tmp();
    chunkID.createEventID(0, name);

    mChunkHolder->createChunk(chunkID, nullptr);

    const char* name2 = sTalkCameraName;
    CameraParamChunkID_Tmp chunkID2 = CameraParamChunkID_Tmp();
    chunkID2.createEventID(0, name2);

    CameraParamChunk* chunk2 = mChunkHolder->getChunk(chunkID2);

    if (chunk2 != nullptr) {
        chunk2->setCameraType("CAM_TYPE_TALK", mHolder);
        chunk2->_64 = true;
    }
}

void CameraDirector::createSubjectiveCamera() {
    const char* name = sSubjectiveCameraName;
    CameraParamChunkID_Tmp chunkID = CameraParamChunkID_Tmp();
    chunkID.createEventID(0, name);

    mChunkHolder->createChunk(chunkID, nullptr);

    const char* name2 = sSubjectiveCameraName;
    CameraParamChunkID_Tmp chunkID2 = CameraParamChunkID_Tmp();
    chunkID2.createEventID(0, name2);

    CameraParamChunk* chunk2 = mChunkHolder->getChunk(chunkID2);

    if (chunk2 != nullptr) {
        chunk2->setCameraType("CAM_TYPE_SUBJECTIVE", mHolder);
        chunk2->_64 = true;
    }
}

namespace MR {
    CameraDirector* getCameraDirector() {
        return getSceneObj< CameraDirector >(SceneObj_CameraDirector);
    }
};  // namespace MR
