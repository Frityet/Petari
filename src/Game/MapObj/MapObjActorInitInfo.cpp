#include "Game/MapObj/MapObjActor.hpp"
#include "Game/Util.hpp"

// scheduling issues with the paired single set
MapObjActorInitInfo::MapObjActorInitInfo() {
    mSetDefaultPosition = 0;
    mConnectToScene = 0;
    mInitBinder = 0;
    mHasEffect = 0;
    mHasSensors = 0;
    mHasShadows = 0;
    mCalcGravity = 0;
    mProjectMapMtx = 0;
    mInitFur = 0;
    mHasRailMover = 0;
    mHasRotator = 0;
    mHasRailRotator = 0;
    _C = 0;
    _D = 0;
    mUsesRailPosture = 0;
    mDoesBaseMtxFollowTarget = 0;
    mIsAffectedByScale = 0;
    mUseMirrorReflection = 0;
    mModelName = 0;
    mHioNode = 0;
    _1C = 0;
    mSensorSize = 0.0f;
    mSensorOffset.setPSZeroVec();
    mHitSensorCB = 0;
    mBinderRadius = 0.0f;
    mBinderCenterY = 0.0f;
    mEffectName = 0;
    mSound = 0;
    mSoundPos = 0;
    _48 = 0;
    mNerve = 0;
    mGroupClipping = 0;
    mClippingRadius = 0.0f;
    mFarClipping = 0.0f;
    _5C = -1;
    mColorChangeArg = -1;
    mTextureChangeArg = -1;
    mNoAppearRiddleSE = 0;
    _6C = 0;
    _70 = 0.0f;
    _74 = 0;
    mShadowName = 0;
    mShadowLength = -1.0f;
    _80 = 0;
    mDummyChangeTexture = 0;
    _88 = -1;
    _8C = 0;
    mNoUseLOD = 0;
}

void MapObjActorInitInfo::setupHioNode(const char* pName) {
    mHioNode = pName;
}

void MapObjActorInitInfo::setupDefaultPos() {
    mSetDefaultPosition = true;
}

void MapObjActorInitInfo::setupModelName(const char* pName) {
    mModelName = pName;
}

void MapObjActorInitInfo::setupConnectToScene() {
    mConnectToScene = true;
}

void MapObjActorInitInfo::setupBinder(f32 binderRadius, f32 binderCenterY) {
    mBinderRadius = binderRadius;
    mInitBinder = true;
    mBinderCenterY = binderCenterY;
}

void MapObjActorInitInfo::setupEffect(const char* pName) {
    mEffectName = pName;
    mHasEffect = true;
}

void MapObjActorInitInfo::setupSound(s32 id) {
    mSound = id;
}

void MapObjActorInitInfo::setupSoundPos(TVec3f* pPos) {
    mSoundPos = pPos;
}

void MapObjActorInitInfo::setupNoAppearRiddleSE() {
    mNoAppearRiddleSE = false;
}

void MapObjActorInitInfo::setupHitSensor() {
    mHasSensors = true;
}

void MapObjActorInitInfo::setupHitSensorCallBack() {
    mHasSensors = true;
    mHitSensorCB = true;
}

void MapObjActorInitInfo::setupHitSensorParam(u16 type, f32 size, const TVec3f& rOffset) {
    _1C = type;
    mSensorSize = size;
    mSensorOffset.x = rOffset.x;
    mSensorOffset.y = rOffset.y;
    mSensorOffset.z = rOffset.z;
}

void MapObjActorInitInfo::setupNerve(const Nerve* pNerve) {
    mNerve = pNerve;
}

void MapObjActorInitInfo::setupShadow(const char* pName) {
    mShadowName = pName;
    mHasShadows = true;
}

void MapObjActorInitInfo::setupGroupClipping(s32 groupClipping) {
    mGroupClipping = groupClipping;
}

void MapObjActorInitInfo::setupClippingRadius(f32 radius) {
    mClippingRadius = radius;
}

void MapObjActorInitInfo::setupFarClipping(f32 clipping) {
    mFarClipping = clipping;
}

void MapObjActorInitInfo::setupProjmapMtx(bool updateMtx) {
    _48 = updateMtx;
    mProjectMapMtx = true;
}

void MapObjActorInitInfo::setupRailMover() {
    mHasRailMover = true;
}

void MapObjActorInitInfo::setupRotator() {
    mHasRotator = true;
}

void MapObjActorInitInfo::setupRailRotator() {
    mHasRailRotator = true;
}

void MapObjActorInitInfo::setupSeesaw1AxisRotator(const char* pName, f32 speed) {
    _6C = pName;
    _C = true;
    _70 = speed;
}

void MapObjActorInitInfo::setupRailPosture() {
    mUsesRailPosture = true;
}

void MapObjActorInitInfo::setupBaseMtxFollowTarget() {
    mDoesBaseMtxFollowTarget = true;
}

void MapObjActorInitInfo::setupAffectedScale() {
    mIsAffectedByScale = true;
}

void MapObjActorInitInfo::setupSeAppear() {
    mNoAppearRiddleSE = true;
}

void MapObjActorInitInfo::setupMirrorReflection(bool useMirrorReflection) {
    _74 = useMirrorReflection;
    mUseMirrorReflection = true;
}

void MapObjActorInitInfo::setupPrepareChangeDummyTexture(const char* pTextureName) {
    mDummyChangeTexture = pTextureName;
}

void MapObjActorInitInfo::setupNoUseLodCtrl() {
    mNoUseLOD = true;
}

void MapObjActorUtil::setupInitInfoTypical(MapObjActorInitInfo* pInfo, const char* pObjName) {
    const char* noAppearRiddleSEObjects[] = {
        "AsteroidMoveA",
        "HeavensDoorAppearStepA",
        "HeavensDoorAppearStepAAfter",
        "HeavensDoorInsideRotatePartsA",
        "HeavensDoorInsideRotatePartsB",
        "HeavensDoorInsideRotatePartsC",
        "PeachCastleTownAfterAttack",
        "PeachCastleTownAfterAttack",
        "PeachCastleTownBeforeAttack",
        "PhantomShipPropellerBig",
        "PhantomShipPropellerSmall",
        "StarPieceCluster",
        "RosettaChair",
        "AstroDomeDemoAstroGalaxy",
        "ChallengeBallMoveGroundB",
        "ChallengeBallVanishingRoadA",
        "DarkHopperRotateStepA",
        "DarkHopperPlanetA",
        "PeachCastleTownGate",
        "KoopaShipE",
        "BattleShipElevatorCover",
        "StrongBlock",
    };

    if (MR::isExistString(pObjName, noAppearRiddleSEObjects, 22)) {
        pInfo->mNoAppearRiddleSE = false;
    }

    if (MR::isEqualString("HeavensDoorInsidePlanet", pObjName)) {
        pInfo->mProjectMapMtx = true;
        pInfo->_48 = false;
    }

    const char* shadowNameObjects[] = {
        "UFOSandObstacleA",
        "UFOSandObstacleB",
        "UFOSandObstacleC",
    };

    if (MR::isExistString(pObjName, shadowNameObjects, 3)) {
        pInfo->mShadowName = "Shadow";
    }

    const char* hitSensorObjects[] = {
        "KoopaVS2PartsJoinedMoveStep",
        "KoopaVS2PartsSquareMoveStepA",
        "KoopaVS2PartsSquareMoveStepB",
        "KoopaVS2PartsNarrowRoad",
        "KoopaVS2PartsClipAreaDisplayA",
    };

    if (MR::isExistString(pObjName, hitSensorObjects, 5)) {
        pInfo->mHasSensors = true;
    }

    if (MR::isEqualString("SandUpDownTowerBreakableWallA", pObjName)
        || MR::isEqualString("SandUpDownTowerBreakableWallB", pObjName)) {
        pInfo->_80 = "SandUpDownTowerBreakableWallBreak";
    }

    if (MR::isEqualString("KoopaJrNormalShipA", pObjName)) {
        pInfo->_88 = 0x22;
    }

    if (MR::isEqualString("KoopaStatue", pObjName)) {
        pInfo->_8C = true;
        pInfo->_5C = 2;
    }

    if (MR::isEqualString("DangerSignBoard", pObjName)) {
        pInfo->mHasSensors = true;
        TVec3f offset(0.0f, 200.0f, 0.0f);
        pInfo->setupHitSensorParam(4, 200.0f, offset);
    }
}

void MapObjActorUtil::setupInitInfoColorChangeArg0(MapObjActorInitInfo* pInfo, const JMapInfoIter& rIter) {
    s32 arg = -1;
    MR::getJMapInfoArg0NoInit(rIter, &arg);
    pInfo->mColorChangeArg = arg;
}

void MapObjActorUtil::setupInitInfoTextureChangeArg1(MapObjActorInitInfo* pInfo, const JMapInfoIter& rIter) {
    s32 arg = -1;
    MR::getJMapInfoArg1NoInit(rIter, &arg);
    pInfo->mTextureChangeArg = arg;
}

void MapObjActorUtil::setupInitInfoShadowLengthArg2(MapObjActorInitInfo* pInfo, const JMapInfoIter& rIter) {
    f32 arg = -1.0f;
    MR::getJMapInfoArg2NoInit(rIter, &arg);
    pInfo->mShadowLength = arg;
}

void MapObjActorUtil::setupInitInfoSeesaw(MapObjActorInitInfo* pInfo, const JMapInfoIter& rIter, const char* pName, f32 speed) {
    s32 rotateAxis = -1;
    MR::getMapPartsArgRotateAxis(&rotateAxis, rIter);

    if (rotateAxis == 0) {
        pInfo->_6C = pName;
        pInfo->_C = true;
        pInfo->_70 = speed;
    }
    else {
        pInfo->_6C = pName;
        pInfo->_D = true;
        pInfo->_70 = speed;
    }
}

void MapObjActorUtil::setupInitInfoSimpleMapObj(MapObjActorInitInfo* pInfo) {
    pInfo->mHioNode = "地形オブジェ";
    pInfo->mSetDefaultPosition = true;
    pInfo->mConnectToScene = true;
    pInfo->mHasEffect = true;
    pInfo->mEffectName = 0;
    pInfo->mSound = 4;
    pInfo->mHasShadows = true;
    pInfo->mShadowName = 0;
    pInfo->mGroupClipping = 0x40;
    pInfo->mNoAppearRiddleSE = true;
}

void MapObjActorUtil::setupInitInfoPlanet(MapObjActorInitInfo* pInfo) {
    pInfo->mSetDefaultPosition = true;
    pInfo->mHioNode = "惑星";
    pInfo->mConnectToScene = true;
    pInfo->mHasEffect = true;
    pInfo->mEffectName = 0;
    pInfo->mFarClipping = -1.0f;
    pInfo->mNoAppearRiddleSE = false;
}
