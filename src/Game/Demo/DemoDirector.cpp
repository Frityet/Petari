#include "Game/Demo/DemoDirector.hpp"
#include "Game/Demo/DemoCastGroupHolder.hpp"
#include "Game/Demo/DemoFunction.hpp"
#include "Game/Demo/DemoSimpleCastHolder.hpp"
#include "Game/Demo/DemoStartRequestHolder.hpp"
#include "Game/Player/MarioAccess.hpp"
#include "Game/Scene/SceneNameObjMovementController.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/JMapIdInfo.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/StarPointerUtil.hpp"

namespace DemoStartRequestUtil {
    void startDemo(DemoStartRequestHolder*);
    void popStartDemoRequest(DemoStartRequestHolder*);
    bool isExistStartDemoRequest(const DemoStartRequestHolder*);
    NameObj* getDemoStarter(const DemoStartInfo&);
};

namespace MR {
    bool isCameraInterpolatingNearlyEnd();
};

DemoDirector::DemoDirector(const char* pName) : NameObj(pName) {
    mIsActive = false;
    mExecutor = nullptr;
    _14 = false;
    _18 = nullptr;
    _20 = new DemoSimpleCastHolder(0x200, 0x40, 0x80);
    mResourceHolder = nullptr;
    mStartReqHolder = new DemoStartRequestHolder();
    _2C = nullptr;
    _30 = nullptr;
    _34 = -1;
    _38 = true;
    MR::connectToScene(this, MR::MovementType_DemoDirector, -1, -1, -1);

    _18 = new DemoCastGroupHolder();
    _18->initWithoutIter();

    _1C = new DemoCastGroupHolder();
    _1C->initWithoutIter();

    mResourceHolder = DemoFunction::loadDemoArchive();
}

void DemoDirector::movement() {
    if (mExecutor != nullptr && !_14) {
        mExecutor->movement();
    }

    if (!tryStartDemoRequested() && _14 && MR::isCameraInterpolatingNearlyEnd()) {
        _14 = false;
        doDemoEndRequest();
    }
}

void DemoDirector::startDemoProgrammable(NameObj* pStarter, const char* pDemoName, bool useCinemaFrame, s32 movementType) {
    startDemo(pStarter, pDemoName, useCinemaFrame, movementType);
}

void DemoDirector::startDemoTimeKeep(NameObj* pStarter, const char* pDemoName, s32 movementType, bool useCinemaFrame, const char* pPartName) {
    startDemo(pStarter, pDemoName, useCinemaFrame, movementType);
    startDemoExecutor(pStarter, pDemoName, movementType, pPartName);
}

void DemoDirector::startDemoExecutor(NameObj* pStarter, const char* pDemoName, s32 movementType, const char* pPartName) {
    mExecutor = DemoFunction::findDemoExecutor(pDemoName);
    if (pPartName != nullptr) {
        mExecutor->startPart(pStarter, pDemoName, pPartName, movementType);
    }
    else {
        mExecutor->start(pStarter, pDemoName, movementType);
    }
}

char* DemoDirector::getCurrentDemoName() const {
    if (mIsActive) {
        return const_cast< char* >(_30);
    }
    return nullptr;
}

void DemoDirector::endDemo(NameObj* pStarter, const char*, bool waitCamera) {
    if (DemoStartRequestUtil::isExistStartDemoRequest(mStartReqHolder)) {
        MR::sendMsgToAllLiveActor(0x70, nullptr);
        mExecutor = nullptr;
        doDemoEndRequest();
        startDemoRequested();
    }
    else if (waitCamera && !MR::isCameraInterpolatingNearlyEnd() && _34 != MR::MovementControlType_3) {
        _14 = true;
        MR::sendMsgToAllLiveActor(0x70, nullptr);
        mExecutor = nullptr;
        MR::getSceneNameObjMovementController()->requestStopSceneOverwrite(pStarter);
    }
    else {
        MR::sendMsgToAllLiveActor(0x70, nullptr);
        mExecutor = nullptr;
        doDemoEndRequest();
    }
}

bool DemoDirector::isExistTimeKeepDemo(const char* pDemoName) const {
    for (s32 i = 0; i < _18->mObjectCount; i++) {
        if (MR::isName(_18->getCastGroup(i), pDemoName)) {
            return true;
        }
    }
    return false;
}

bool DemoDirector::registerDemoCast(LiveActor* pActor, const JMapInfoIter& rIter) {
    if (!MR::isValidInfo(rIter)) {
        return false;
    }

    s32 demoGroupID = -1;
    if (!MR::getJMapInfoDemoGroupID(rIter, &demoGroupID)) {
        return false;
    }

    JMapIdInfo info(MR::getDemoGroupID(rIter), rIter);
    if (_18->tryRegisterDemoActor(pActor, rIter, info)) {
        return true;
    }

    return _1C->tryRegisterDemoActor(pActor, rIter, info);
}

bool DemoDirector::registerDemoCast(LiveActor* pActor, const char* pName, const JMapInfoIter& rIter) {
    if (_18->tryRegisterDemoActor(pActor, pName, rIter)) {
        return true;
    }
    return _1C->tryRegisterDemoActor(pActor, pName, rIter);
}

void DemoDirector::registerDemoSimpleCast(LiveActor* pActor) {
    _20->registerActor(pActor);
}

void DemoDirector::registerDemoSimpleCast(LayoutActor* pActor) {
    _20->registerActor(pActor);
}

void DemoDirector::registerDemoSimpleCast(NameObj* pObj) {
    _20->registerNameObj(pObj);
}

bool DemoDirector::tryStartDemoRequested() {
    if (!mStartReqHolder->isExistRequest()) {
        return false;
    }

    if (!MR::canStartDemo()) {
        return false;
    }

    startDemoRequested();
    return true;
}

void DemoDirector::startDemo(NameObj* pStarter, const char* pDemoName, bool useCinemaFrame, s32 movementType) {
    _2C = pStarter;
    mIsActive = true;
    _30 = pDemoName;
    _34 = movementType;
    _38 = useCinemaFrame;
    _20->movementOnAllCasts();
    MR::initStarPieceGetCSSound();
}

void DemoDirector::startDemoRequested() {
    DemoStartRequestUtil::startDemo(mStartReqHolder);

    const DemoStartInfo* info = mStartReqHolder->getCurrentInfo();
    NameObj* starter = DemoStartRequestUtil::getDemoStarter(*info);
    startDemo(starter, info->mDemoName, info->_2C == 0, info->_24);

    DemoStartRequestUtil::popStartDemoRequest(mStartReqHolder);
}

void DemoDirector::doDemoEndRequest() {
    MR::getSceneNameObjMovementController()->requestPlaySceneFor(static_cast< MR::MovementControlType >(_34), _2C);
    MR::activateDefaultGameLayout();
    MR::endStarPointerMode(_2C);
    if (_38) {
        MR::tryFrameToScreenCinemaFrame();
    }

    if (_34 == MR::MovementControlType_2 || _34 == MR::MovementControlType_3) {
        MarioAccess::endRemoteDemo(nullptr);
    }

    mIsActive = false;
}

DemoDirector::~DemoDirector() {}
