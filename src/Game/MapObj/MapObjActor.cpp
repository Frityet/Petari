#include "Game/MapObj/MapObjActor.hpp"
#include "Game/LiveActor/LodCtrl.hpp"
#include "Game/LiveActor/MaterialCtrl.hpp"
#include "Game/LiveActor/ModelObj.hpp"
#include "Game/AudioLib/AudAnmSoundObject.hpp"
#include "Game/MapObj/MapPartsRailGuideDrawer.hpp"
#include "Game/MapObj/MapPartsRailMover.hpp"
#include "Game/MapObj/MapPartsRailPosture.hpp"
#include "Game/MapObj/MapPartsRailRotator.hpp"
#include "Game/MapObj/MapPartsRotator.hpp"
#include "Game/MapObj/StageEffectDataTable.hpp"
#include "Game/Util.hpp"
#include "Game/Util/FurMulti.hpp"

#include <cstdio>
#include <cstring>

namespace MR {
    bool getMapPartsArgMovePosture(s32*, const LiveActor*);
    bool getMapPartsArgRailGuideType(s32*, const LiveActor*);
};

namespace NrvMapObjActor {
    NEW_NERVE(HostTypeWait, MapObjActor, Wait);
    NEW_NERVE(HostTypeMove, MapObjActor, Move);
    NEW_NERVE(HostTypeDone, MapObjActor, Done);
};  // namespace NrvMapObjActor

namespace {
    const char* cBrkNameColorChange = "ColorChange";
    const char* cBtpNameTexChange = "TexChange";
    const char* cBtkNameTexChange = "TexChange";
    const char* cEffectNameAppear = "Appear";
    const char* cBckNameMove = "Move";
    const char* cFollowJointName = "Move";
    const char* cEffectNameBreak = "Break";
    const char* cBckNameBreak = "Break";
};  // namespace

class MapPartsSeesaw1AxisRotator : public MapPartsRotatorBase {
public:
    MapPartsSeesaw1AxisRotator(LiveActor*, const char*, f32);

    virtual ~MapPartsSeesaw1AxisRotator();
    virtual void init(const JMapInfoIter&);
    virtual bool isWorking() const;
    virtual void start();
    virtual void end();
    virtual bool receiveMsg(u32);
    virtual void control();
    virtual const TMtx34f& getRotateMtx() const;
    virtual bool isMoving() const;

    u8 _18[0x50];
    f32 mAngularSpeed;  // 0x68
    f32 mForcedAngle;   // 0x6C
    u8 _70[0x14];
};

class MapPartsSeesaw2AxisRotator : public MapPartsRotatorBase {
public:
    MapPartsSeesaw2AxisRotator(LiveActor*, const char*, f32);

    virtual ~MapPartsSeesaw2AxisRotator();
    virtual void init(const JMapInfoIter&);
    virtual bool isWorking() const;
    virtual void start();
    virtual void end();
    virtual bool receiveMsg(u32);
    virtual void control();
    virtual const TMtx34f& getRotateMtx() const;
    virtual bool isMoving() const;

    u8 _18[0xB8];
};

MapObjActor::MapObjActor(const char* pName) : LiveActor(pName) {
    mObjectName = 0;
    mPlanetLodCtrl = 0;
    mBloomModel = 0;
    mModelObj = 0;
    mMatrixSetter = 0;
    mRailMover = 0;
    mRotator = 0;
    mRailRotator = 0;
    mRailPosture = 0;
    mRailGuideDrawer = 0;
    _B4 = 0;
    _B5 = 0;
    _B6 = 0;
    mWaitNrv = &NrvMapObjActor::HostTypeWait::sInstance;
    mMoveNrv = &NrvMapObjActor::HostTypeMove::sInstance;
    mDoneNrv = &NrvMapObjActor::HostTypeDone::sInstance;
}

MapObjActor::MapObjActor(const char* pName, const char* pObjName) : LiveActor(pName) {
    mObjectName = pObjName;
    mPlanetLodCtrl = 0;
    mBloomModel = 0;
    mModelObj = 0;
    mMatrixSetter = 0;
    mRailMover = 0;
    mRotator = 0;
    mRailRotator = 0;
    mRailPosture = 0;
    mRailGuideDrawer = 0;
    _B4 = 0;
    _B6 = 0;
    mWaitNrv = &NrvMapObjActor::HostTypeWait::sInstance;
    mMoveNrv = &NrvMapObjActor::HostTypeMove::sInstance;
    mDoneNrv = &NrvMapObjActor::HostTypeDone::sInstance;
}

void MapObjActor::init(const JMapInfoIter& rIter) {
    if (mObjectName) {
        return;
    }

    MR::getObjectName(&mObjectName, rIter);
}

void MapObjActor::initAfterPlacement() {
    if (!mMatrixSetter) {
        return;
    }

    if (!_B4) {
        return;
    }

    mMatrixSetter->updateMtxUseBaseMtx();
}

void MapObjActor::appear() {
    LiveActor::appear();

    if (mBloomModel) {
        mBloomModel->appear();
    }

    if (MR::isExistEffectKeeper(this)) {
        const char* appearEffectName = cEffectNameAppear;
        if (MR::isRegisteredEffect(this, appearEffectName)) {
            MR::emitEffect(this, appearEffectName);
        }
    }

    if (!MR::isEqualString(mObjectName, "DarkHopperRotateStepA")) {
        const char* startSound = MR::StageEffect::getStartSe(mObjectName);

        if (startSound) {
            MR::startSound(this, startSound);
        }
    }

    if (_B6) {
        MR::startSystemSE("SE_SY_READ_RIDDLE_S");
    }
}

void MapObjActor::kill() {
    if (MR::isValidSwitchDead(this)) {
        MR::onSwitchDead(this);
    }

    if (mModelObj) {
        mModelObj->kill();
    }

    if (mBloomModel) {
        mBloomModel->kill();
    }

    LiveActor::kill();
}

void MapObjActor::initialize(const JMapInfoIter& rIter, const MapObjActorInitInfo& rInfo) {
    bool isConnectedWithRail = MR::isConnectedWithRail(rIter);

    if (rInfo.mSetDefaultPosition) {
        MR::initDefaultPos(this, rIter);
    }

    bool needsDifferedDLBuffer = rInfo.mProjectMapMtx || rInfo.mUseMirrorReflection || rInfo.mDummyChangeTexture != nullptr;

    if (rInfo.mModelName != nullptr) {
        mObjectName = rInfo.mModelName;
    }

    initModelManagerWithAnm(mObjectName, nullptr, needsDifferedDLBuffer);

    if (rInfo.mDummyChangeTexture != nullptr) {
        MR::initDLMakerChangeTex(this, rInfo.mDummyChangeTexture);
        MR::newDifferedDLBuffer(this);
    }

    bool hasCollision = MR::isExistCollisionResource(this, mObjectName);
    connectToScene(rInfo);

    if (rInfo._8C) {
        MR::initLightCtrl(this);
    }

    if (rInfo.mProjectMapMtx) {
        mMatrixSetter = MR::initDLMakerProjmapEffectMtxSetter(this);
        MR::newDifferedDLBuffer(this);
        _B4 = rInfo._48;
    }

    if (rInfo.mUseMirrorReflection) {
        MR::initMirrorReflection(this);
        _B5 = rInfo._74;

        TPos3f baseMtx;
        baseMtx.setInline(getBaseMtx());
        MR::setMirrorReflectionInfoFromMtxYUp(baseMtx);
    }

    if (rInfo.mInitBinder) {
        initBinder(rInfo.mBinderRadius, rInfo.mBinderCenterY, 0);
    }

    if (rInfo.mHasEffect) {
        initEffectKeeper(0, rInfo.mEffectName, false);
    }

    if (rInfo.mSound > 0) {
        bool hasSoundPos = rInfo.mSoundPos != nullptr;
        initSound(rInfo.mSound, hasSoundPos);

        if (hasSoundPos) {
            mSoundObject->setTrans(rInfo.mSoundPos);
        }
    }

    if (rInfo.mNoAppearRiddleSE) {
        _B6 = true;
    }

    if (rInfo.mHasShadows && rInfo.mShadowLength != 0.0f) {
        if (rInfo.mShadowName != nullptr) {
            MR::initShadowFromCSV(this, rInfo.mShadowName);
        }
        else {
            MR::initShadowFromCSV(this, "Shadow");
        }

        if (rInfo.mShadowLength != -1.0f) {
            MR::setShadowDropLength(this, nullptr, rInfo.mShadowLength);
        }
    }

    if (rInfo.mCalcGravity) {
        MR::onCalcGravity(this);
    }

    if (rInfo.mDoesBaseMtxFollowTarget) {
        MR::addBaseMatrixFollowTarget(this, rIter, nullptr, nullptr);
    }

    if (rInfo.mNerve != nullptr) {
        initNerve(rInfo.mNerve);
    }

    if (rInfo.mHasSensors) {
        initHitSensor(1);

        f32 sensorSize = rInfo.mSensorSize;
        TVec3f sensorOffset;
        sensorOffset.set< f32 >(rInfo.mSensorOffset);

        if (rInfo.mIsAffectedByScale) {
            sensorSize *= mScale.x;
            sensorOffset.x *= mScale.x;
            sensorOffset.y *= mScale.y;
            sensorOffset.z *= mScale.z;
        }

        if (rInfo.mHitSensorCB) {
            MR::addHitSensorCallbackMapObj(this, "body", rInfo._1C, sensorSize);
        }
        else {
            MR::addHitSensorMapObj(this, "body", rInfo._1C, sensorSize, sensorOffset);
        }
    }

    if (hasCollision) {
        if (!rInfo.mHasSensors) {
            initHitSensor(1);
            MR::addBodyMessageSensorMapObj(this);
        }

        HitSensor* sensor = getSensor("body");
        MtxPtr jointMtx = nullptr;

        if (MR::isExistJoint(this, cFollowJointName)) {
            jointMtx = MR::getJointMtx(this, cFollowJointName);
        }

        MR::initCollisionParts(this, mObjectName, sensor, jointMtx);
        MR::tryCreateCollisionMoveLimit(this, sensor);
    }

    if (isConnectedWithRail) {
        initRailRider(rIter);
    }

    if (rInfo.mHasRailMover && isConnectedWithRail) {
        mRailMover = new MapPartsRailMover(this);
        mRailMover->init(rIter);
    }

    if (rInfo.mHasRotator) {
        mRotator = new MapPartsRotator(this);
        mRotator->init(rIter);
    }

    if (rInfo.mHasRailRotator) {
        mRailRotator = new MapPartsRailRotator(this);
        mRailRotator->init(rIter);
    }

    if (rInfo._C) {
        mRotator = new MapPartsSeesaw1AxisRotator(this, rInfo._6C, rInfo._70);
        mRotator->init(rIter);
    }

    if (rInfo._D) {
        mRotator = new MapPartsSeesaw2AxisRotator(this, rInfo._6C, rInfo._70);
        mRotator->init(rIter);
    }

    if (rInfo.mUsesRailPosture && isConnectedWithRail) {
        s32 movePosture = 0;
        MR::getMapPartsArgMovePosture(&movePosture, this);

        if (movePosture != 0) {
            mRailPosture = new MapPartsRailPosture(this);
            mRailPosture->init(rIter);
        }
    }

    if (isConnectedWithRail) {
        s32 railGuideType = 0;
        MR::getMapPartsArgRailGuideType(&railGuideType, this);

        if (railGuideType != 0) {
            mRailGuideDrawer = MR::createMapPartsRailGuideDrawer(this, "RailPoint", rIter);
        }
    }

    MR::tryStartAllAnim(this, mObjectName);

    if (rInfo.mColorChangeArg > -1) {
        MR::startBrk(this, cBrkNameColorChange);
        MR::setBrkFrameAndStop(this, static_cast< f32 >(rInfo.mColorChangeArg));
    }

    if (rInfo.mTextureChangeArg > -1) {
        if (MR::isExistBtp(this, cBtpNameTexChange)) {
            MR::startBtp(this, cBtpNameTexChange);
            MR::setBtpFrameAndStop(this, static_cast< f32 >(rInfo.mTextureChangeArg));
        }

        if (MR::isExistBtk(this, cBtpNameTexChange)) {
            MR::startBtk(this, cBtkNameTexChange);
            MR::setBtkFrameAndStop(this, static_cast< f32 >(rInfo.mTextureChangeArg));
        }
    }

    f32 clippingRadius[1];
    clippingRadius[0] = -1.0f;
    if (rInfo.mClippingRadius > 0.0f) {
        clippingRadius[0] = rInfo.mClippingRadius;
    }
    else {
        MR::calcModelBoundingRadius(clippingRadius, this);
    }

    if (rInfo.mIsAffectedByScale) {
        clippingRadius[0] *= mScale.x;
    }

    MR::setClippingTypeSphere(this, clippingRadius[0]);

    if (MR::isValidInfo(rIter) && rInfo.mGroupClipping > 0) {
        MR::setGroupClipping(this, rIter, rInfo.mGroupClipping);
    }

    if (rInfo.mFarClipping != 0.0f) {
        MR::setClippingFar(this, rInfo.mFarClipping);
    }

    if (!rInfo.mNoUseLOD) {
        if (LodCtrlFunction::isExistLodLowModel(mObjectName)) {
            mPlanetLodCtrl = MR::createLodCtrlPlanet(this, rIter, -1.0f, rInfo._88);

            if (rInfo.mColorChangeArg > -1 && MR::isExistBrk(this, cBrkNameColorChange)) {
                MR::startBrk(mPlanetLodCtrl->_14, cBrkNameColorChange);
                MR::setBrkFrameAndStop(mPlanetLodCtrl->_14, static_cast< f32 >(rInfo.mColorChangeArg));
            }

            if (rInfo.mTextureChangeArg > -1) {
                if (MR::isExistBtp(this, cBtpNameTexChange)) {
                    MR::startBtp(mPlanetLodCtrl->_14, cBtpNameTexChange);
                    MR::setBtpFrameAndStop(mPlanetLodCtrl->_14, static_cast< f32 >(rInfo.mTextureChangeArg));
                }

                if (MR::isExistBtk(this, cBtpNameTexChange)) {
                    MR::startBtk(mPlanetLodCtrl->_14, cBtkNameTexChange);
                    MR::setBtkFrameAndStop(mPlanetLodCtrl->_14, static_cast< f32 >(rInfo.mTextureChangeArg));
                }
            }
        }
    }

    if (MR::isExistSubModel(mObjectName, "Bloom")) {
        char bloomModelName[0x100];
        snprintf(bloomModelName, sizeof(bloomModelName), "%sBloom", mObjectName);
        mBloomModel = MR::createModelObjBloomModel(mName, bloomModelName, getBaseMtx());
        mBloomModel->mPosition.set< f32 >(mPosition);
        MR::calcModelBoundingRadius(clippingRadius, this);
        MR::setClippingFarMax(mBloomModel);
        MR::setClippingTypeSphere(mBloomModel, clippingRadius[0]);
    }

    tryCreateBreakModel(rInfo);
    makeSubModels(rIter, rInfo);

    if (rInfo.mInitFur) {
        MR::initMultiFur(this, rInfo._5C);
    }

    makeActorDead();

    if (MR::useStageSwitchWriteA(this, rIter)) {
        initCaseUseSwitchA(rInfo);
    }
    else {
        initCaseNoUseSwitchA(rInfo);
    }

    if (MR::useStageSwitchWriteB(this, rIter)) {
        initCaseUseSwitchB(rInfo);
    }
    else {
        initCaseNoUseSwitchB(rInfo);
    }

    MR::useStageSwitchWriteDead(this, rIter);

    if (MR::useStageSwitchReadAppear(this, rIter)) {
        MR::syncStageSwitchAppear(this);

        if (rInfo.mNoAppearRiddleSE) {
            _B6 = true;
        }

        makeActorDead();
    }

    MR::useStageSwitchSleep(this, rIter);

    if (MR::tryRegisterDemoCast(this, rIter)) {
        if (mModelObj != nullptr) {
            MR::tryRegisterDemoCast(mModelObj, rIter);
        }

        if (MR::isRegisteredDemoActionAppear(this)) {
            if (rInfo.mNoAppearRiddleSE) {
                _B6 = true;
            }

            makeActorDead();
        }
    }
}

bool MapObjActor::isObjectName(const char* pName) const {
    return MR::isEqualString(pName, mObjectName);
}

void MapObjActor::connectToScene(const MapObjActorInitInfo& rInfo) {
    if (rInfo.mConnectToScene) {
        if (MR::isExistCollisionResource(this, mObjectName)) {
            s32 type = rInfo._5C;

            if (type == 1) {
                MR::connectToSceneCollisionMapObjStrongLight(this);
            } else if (type == 2) {
                MR::connectToSceneCollisionMapObjWeakLight(this);
            } else {
                MR::connectToSceneCollisionMapObj(this);
            }
        } else if (rInfo._5C == 1) {
            MR::connectToSceneMapObjStrongLight(this);
        } else {
            MR::connectToSceneMapObj(this);
        }
    }
}

void MapObjActor::initCaseUseSwitchA(const MapObjActorInitInfo&) {
    setNerve(mWaitNrv);
}

void MapObjActor::initCaseNoUseSwitchA(const MapObjActorInitInfo&) {
}

void MapObjActor::initCaseUseSwitchB(const MapObjActorInitInfo& rInfo) {
    MR::listenStageSwitchOnOffB(this, MR::Functor(this, &MapObjActor::startMapPartsFunctions), MR::Functor(this, &MapObjActor::endMapPartsFunctions));
}

void MapObjActor::initCaseNoUseSwitchB(const MapObjActorInitInfo& rInfo) {
    MapObjActorUtil::startAllMapPartsFunctions(this);
}

void MapObjActor::control() {
    if (!tryEmitWaitEffect()) {
        tryDeleteWaitEffect();
    }

    if (mPlanetLodCtrl) {
        mPlanetLodCtrl->update();
    }

    if (mRailPosture) {
        mRailPosture->movement();
    }

    if (mRailMover) {
        mRailMover->movement();

        if (mRailMover->isWorking()) {
            mPosition.set< f32 >(mRailMover->_28);
            mRailMover->tryResetPositionRepeat();
        }
    }

    if (mRotator) {
        mRotator->movement();
        if (mRotator->isOnReverse()) {
            const char* startSound = MR::StageEffect::getStartSe(mObjectName);

            if (startSound) {
                MR::startSound(this, startSound);
            }
        }
    }

    if (mRailRotator) {
        mRailRotator->movement();
    }

    if (!mRailMover && !mRotator && !mRailRotator && !MR::isEqualString(mObjectName, "OceanRingRuinsMove")) {
        const char* movingSound = MR::StageEffect::getMovingSe(mObjectName);
        if (movingSound) {
            MR::startLevelSound(this, movingSound);
        }
    }

    if (mRailGuideDrawer) {
        mRailGuideDrawer->movement();
    }
}

void MapObjActor::calcAndSetBaseMtx() {
    updateProjmapMtx();

    if (MR::isExistMirrorCamera() && _B5) {
        MR::setMirrorReflectionInfoFromModel(this);
    }

    bool v3 = 1;
    bool v4 = 1;
    bool v5 = 0;

    if (mRotator && mRotator->isWorking()) {
        v5 = true;
    }

    if (!v5) {
        bool v7 = 0;

        if (mRailRotator && mRailRotator->isWorking()) {
            v7 = 1;
        }

        if (!v7) {
            v4 = 0;
        }
    }

    if (!v4) {
        bool v9 = 0;

        if (mRailPosture && mRailPosture->isWorking()) {
            v9 = 1;
        }

        if (!v9) {
            v3 = 0;
        }
    }

    if (!v3) {
        LiveActor::calcAndSetBaseMtx();
    } else {
        TPos3f mtx;
        mtx.identity();

        if (mRailPosture && mRailPosture->isWorking()) {
            mtx.concat(mRailPosture->_18);
        }

        if (mRotator && mRotator->isWorking()) {
            mtx.concat(mRotator->getRotateMtx());
        }

        if (mRailRotator && mRailRotator->isWorking()) {
            mtx.concat(mRailRotator->_5C);
        }

        mtx.mMtx[0][3] = mPosition.x;
        mtx.mMtx[1][3] = mPosition.y;
        mtx.mMtx[2][3] = mPosition.z;
        MR::setBaseTRMtx(this, mtx);
    }
}

void MapObjActor::startClipped() {
    tryEmitWaitEffect();
    LiveActor::startClipped();
}

void MapObjActor::endClipped() {
    LiveActor::endClipped();
    tryDeleteWaitEffect();
}

bool MapObjActor::tryCreateBreakModel(const MapObjActorInitInfo& rInfo) {
    char buf[0x100];

    if (rInfo._80) {
        snprintf(buf, sizeof(buf), "%s", rInfo._80);
    } else {
        snprintf(buf, sizeof(buf), "%sBreak", mObjectName);
    }

    if (!MR::isExistModel(buf)) {
        return false;
    }

    if (MR::isEqualString(mObjectName, "SandUpDownTowerBreakableWallB")) {
        MtxPtr baseMtx = getBaseMtx();
        mModelObj = MR::createModelObjMapObj("壊れモデル", buf, baseMtx);
    } else {
        MtxPtr baseMtx = getBaseMtx();
        mModelObj = MR::createModelObjMapObjStrongLight("壊れモデル", buf, baseMtx);
    }

    mModelObj->makeActorDead();

    return true;
}

bool MapObjActor::tryEmitWaitEffect() {
    if (!MR::isExistEffectKeeper(this)) {
        return false;
    }

    if (!MR::isRegisteredEffect(this, mObjectName)) {
        return false;
    }

    if (MR::calcCameraDistanceZ(mPosition) > 4000.0f) {
        return false;
    }

    if (MR::isEffectValid(this, mObjectName)) {
        return false;
    }

    MR::emitEffect(this, mObjectName);
    return true;
}

bool MapObjActor::tryDeleteWaitEffect() {
    if (!MR::isExistEffectKeeper(this)) {
        return false;
    }

    if (!MR::isRegisteredEffect(this, mObjectName)) {
        return false;
    }

    if (MR::calcCameraDistanceZ(mPosition) <= 4000.0f) {
        return false;
    }

    if (MR::isNearPlayer(this, 4000.0f)) {
        return false;
    }

    if (!MR::isEffectValid(this, mObjectName)) {
        return false;
    }

    MR::deleteEffect(this, mObjectName);
    return true;
}

void MapObjActor::startMapPartsFunctions() {
    MapObjActorUtil::startAllMapPartsFunctions(this);
}

void MapObjActor::endMapPartsFunctions() {
    MapObjActorUtil::endAllMapPartsFunctions(this);
}

void MapObjActor::pauseMapPartsFunctions() {
    MapObjActorUtil::pauseAllMapPartsFunctions(this);
}

void MapObjActor::setStateWait() {
    setNerve(mWaitNrv);
}

void MapObjActor::updateProjmapMtx() {
    if (!mMatrixSetter) {
        return;
    }

    if (!_B4) {
        return;
    }

    mMatrixSetter->updateMtxUseBaseMtx();
}

void MapObjActor::exeWait() {
    if (MR::isValidSwitchA(this) && MR::isOnSwitchA(this)) {
        MapObjActorUtil::startAllMapPartsFunctions(this);
        setNerve(mMoveNrv);
    }
}

void MapObjActor::exeMove() {
    if (MR::isFirstStep(this)) {
        const char* moveName = cBckNameMove;
        if (MR::isExistBck(this, moveName)) {
            MR::startBck(this, moveName, 0);
        }
    }

    if (MR::isExistBck(this, cBckNameMove) && MR::isBckStopped(this)) {
        setNerve(mDoneNrv);
    }
}

void MapObjActor::exeDone() {
}

void MapObjActorUtil::startAllMapPartsFunctions(const MapObjActor* pActor) {
    if (pActor->mRotator) {
        pActor->mRotator->start();
    }

    if (pActor->mRailMover) {
        pActor->mRailMover->start();
    }

    if (pActor->mRailRotator) {
        pActor->mRailRotator->start();
    }

    if (pActor->mRailPosture) {
        pActor->mRailPosture->start();
    }

    if (pActor->mRailGuideDrawer) {
        pActor->mRailGuideDrawer->start();
    }
}

void MapObjActorUtil::endAllMapPartsFunctions(const MapObjActor* pActor) {
    if (pActor->mRotator) {
        pActor->mRotator->end();
    }

    if (pActor->mRailMover) {
        pActor->mRailMover->end();
    }

    if (pActor->mRailRotator) {
        pActor->mRailRotator->end();
    }

    if (pActor->mRailPosture) {
        pActor->mRailPosture->end();
    }
}

void MapObjActorUtil::pauseAllMapPartsFunctions(const MapObjActor* pActor) {
    if (pActor->mRotator) {
        pActor->mRotator->_14 = 0;
    }

    if (pActor->mRailMover) {
        pActor->mRailMover->_14 = 0;
    }

    if (pActor->mRailRotator) {
        pActor->mRailRotator->_14 = 0;
    }
}

void MapObjActorUtil::resumeAllMapPartsFunctions(const MapObjActor* pActor) {
    if (pActor->mRotator) {
        pActor->mRotator->_14 = 1;
    }

    if (pActor->mRailMover) {
        pActor->mRailMover->_14 = 1;
    }

    if (pActor->mRailRotator) {
        pActor->mRailRotator->_14 = 1;
    }
}

bool MapObjActorUtil::isRotatorMoving(const MapObjActor* pActor) {
    return pActor->mRotator->isMoving();
}

bool MapObjActorUtil::isRailMoverWorking(const MapObjActor* pActor) {
    return pActor->mRailMover->isWorking();
}

bool MapObjActorUtil::isRailMoverReachedEnd(const MapObjActor* pActor) {
    return pActor->mRailMover->isReachedEnd();
}

f32 MapObjActorUtil::getSeesaw1AxisAngularSpeed(const MapObjActor* pActor) {
    return static_cast< MapPartsSeesaw1AxisRotator* >(pActor->mRotator)->mAngularSpeed;
}

void MapObjActorUtil::forceRotateSeesaw1Axis(const MapObjActor* pActor, f32 a2) {
    static_cast< MapPartsSeesaw1AxisRotator* >(pActor->mRotator)->mForcedAngle = a2;
}

void MapObjActorUtil::startRotator(const MapObjActor* pActor) {
    pActor->mRotator->start();
}

void MapObjActorUtil::startRailMover(const MapObjActor* pActor) {
    pActor->mRailMover->start();
}

void MapObjActorUtil::endRotator(const MapObjActor* pActor) {
    pActor->mRotator->end();
}

void MapObjActorUtil::pauseRotator(const MapObjActor* pActor) {
    pActor->mRotator->_14 = 0;
}

void MapObjActorUtil::resetRailMoverToInitPos(const MapObjActor* pActor) {
    pActor->mRailMover->resetToInitPos();
}

void MapObjActorUtil::startBreak(MapObjActor* pActor) {
    if (!MapObjActorUtil::tryStartBreak(pActor)) {
        pActor->kill();
    }
}

bool MapObjActorUtil::tryStartBreak(MapObjActor* pActor) {
    const char* stopSe = MR::StageEffect::getStopSe(pActor->mObjectName);
    if (stopSe) {
        MR::startSound(pActor, stopSe);
    }

    const char* breakEffect = cEffectNameBreak;
    if (MR::isRegisteredEffect(pActor, breakEffect)) {
        MR::emitEffect(pActor, breakEffect);
    }

    ModelObj* modelObj = pActor->mModelObj;
    if (modelObj) {
        pActor->mModelObj->appear();
        const char* breakName = (const char*)cBckNameBreak;
        MR::startAllAnim(modelObj, breakName);

        if (MR::isExistBva(pActor, breakName)) {
            MR::startBva(pActor, breakName);
            MR::setBvaFrameAndStop(pActor, 1.0f);
        } else {
            MR::hideModel(pActor);
        }

        MR::invalidateClipping(modelObj);
        return true;
    } else {
        const char* breakName = cBckNameBreak;
        if (MR::isExistBck(pActor, breakName)) {
            MR::startAllAnim(pActor, breakName);
            MR::invalidateClipping(pActor);
            return true;
        }
    }

    return false;
}

bool MapObjActorUtil::isBreakStopped(const MapObjActor* pActor) {
    const LiveActor* actor = pActor->mModelObj;

    if (!pActor->mModelObj && MR::isExistBck(pActor, cBckNameBreak)) {
        actor = pActor;
    }

    if (!actor) {
        return false;
    }

    return MR::isBckOneTimeAndStopped(actor);
}

void MapObjActorUtil::killBloomModel(MapObjActor* pActor) {
    pActor->mBloomModel->kill();
}

void MapObjActorUtil::appearBloomModel(MapObjActor* pActor) {
    pActor->mBloomModel->appear();
    char buf[0x100];
    snprintf(buf, sizeof(buf), "%sBloom", pActor->mObjectName);
    MR::tryStartAllAnim(pActor->mBloomModel, buf);
}
