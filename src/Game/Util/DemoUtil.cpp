#include "Game/Util/DemoUtil.hpp"
#include "Game/Demo/DemoDirector.hpp"
#include "Game/Demo/DemoFunction.hpp"
#include "Game/Demo/DemoPlayerKeeper.hpp"
#include "Game/Demo/DemoStartRequestHolder.hpp"
#include "Game/NPC/TalkDirector.hpp"
#include "Game/Scene/GameSceneFunction.hpp"
#include "Game/Scene/SceneNameObjMovementController.hpp"
#include "Game/Util/TalkUtil.hpp"

namespace DemoStartRequestUtil {
    void startDemoSystem(NameObj*, const char*, s32, DemoStartInfo::DemoType, DemoStartInfo::CinemaFrameType, DemoStartInfo::StarPointerType,
                         DemoStartInfo::DeleteEffectType, const char*);
    void startDemoSystem(LiveActor*, const char*, s32, DemoStartInfo::DemoType, DemoStartInfo::CinemaFrameType, DemoStartInfo::StarPointerType,
                         DemoStartInfo::DeleteEffectType, const char*);
    void startDemoSystem(LayoutActor*, const char*, s32, DemoStartInfo::DemoType, DemoStartInfo::CinemaFrameType, DemoStartInfo::StarPointerType,
                         DemoStartInfo::DeleteEffectType, const char*);
    bool requestStartDemo(LiveActor*, const char*, const Nerve*, const Nerve*, s32, DemoStartInfo::DemoType, DemoStartInfo::CinemaFrameType,
                          DemoStartInfo::StarPointerType, DemoStartInfo::DeleteEffectType);
    bool requestStartDemo(LayoutActor*, const char*, const Nerve*, const Nerve*, s32, DemoStartInfo::DemoType, DemoStartInfo::CinemaFrameType,
                          DemoStartInfo::StarPointerType, DemoStartInfo::DeleteEffectType);
    bool requestStartDemo(NerveExecutor*, LiveActor*, const char*, const Nerve*, const Nerve*, s32, DemoStartInfo::DemoType,
                          DemoStartInfo::CinemaFrameType, DemoStartInfo::StarPointerType, DemoStartInfo::DeleteEffectType);
    bool requestStartTimeKeepDemo(LiveActor*, const char*, const char*, const Nerve*, const Nerve*, s32, DemoStartInfo::DemoType,
                                  DemoStartInfo::CinemaFrameType, DemoStartInfo::StarPointerType, DemoStartInfo::DeleteEffectType);
    bool requestStartTimeKeepDemo(NerveExecutor*, LiveActor*, const char*, const char*, const Nerve*, const Nerve*, s32, DemoStartInfo::DemoType,
                                  DemoStartInfo::CinemaFrameType, DemoStartInfo::StarPointerType, DemoStartInfo::DeleteEffectType);
    bool requestStartTimeKeepDemo(NameObj*, const char*, const char*, s32, DemoStartInfo::DemoType, DemoStartInfo::CinemaFrameType,
                                  DemoStartInfo::StarPointerType, DemoStartInfo::DeleteEffectType);
};

namespace MR {
    bool tryRegisterDemoCast(LiveActor* pActor, const JMapInfoIter& rIter) {
        return DemoFunction::getDemoDirector()->registerDemoCast(pActor, rIter);
    }

    void registerDemoActionFunctor(const LiveActor* pActor, const MR::FunctorBase& rFunctor, const char* pName) {
        DemoFunction::registerDemoActionFunctorFunction(pActor, rFunctor, pName);
    }

    void registerDemoActionNerve(const LiveActor* pActor, const Nerve* pNerve, const char* pName) {
        DemoFunction::registerDemoActionNerveFunction(pActor, pNerve, pName);
    }

    bool tryRegisterDemoActionFunctor(const LiveActor* pActor, const MR::FunctorBase& rFunctor, const char* pName) {
        if (!DemoFunction::isRegisteredDemoActionFunctor(pActor)) {
            return false;
        } else {
            DemoFunction::registerDemoActionFunctorFunction(pActor, rFunctor, pName);
            return true;
        }
    }

    bool tryRegisterDemoActionNerve(const LiveActor* pActor, const Nerve* pNerve, const char* pName) {
        if (!DemoFunction::isRegisteredDemoActionNerve(pActor)) {
            return false;
        } else {
            DemoFunction::registerDemoActionNerveFunction(pActor, pNerve, pName);
            return true;
        }
    }

    bool tryRegisterDemoCast(LiveActor* pActor, const char* pName, const JMapInfoIter& rIter) {
        return DemoFunction::getDemoDirector()->registerDemoCast(pActor, pName, rIter);
    }

    void registerDemoCast(LiveActor* pActor, const char* pName, const JMapInfoIter& rIter) {
        DemoFunction::getDemoDirector()->registerDemoCast(pActor, pName, rIter);
    }

    void initTalkAnimDemoCast(LiveActor* pActor, const JMapInfoIter& rIter, const char* pName1, const char* pName2) {
        DemoFunction::getDemoDirector()->registerDemoCast(pActor, pName1, rIter);
        DemoFunction::tryCreateDemoTalkAnimCtrlForActorDirect(pActor, pName1, pName2, nullptr);
    }

    void registerDemoActionFunctorDirect(const LiveActor* pActor, const MR::FunctorBase& rFunctor, const char* pName1, const char* pName2) {
        DemoFunction::registerDemoActionFunctorFunction(pActor, rFunctor, pName1, pName2);
    }

    bool tryRegisterDemoActionFunctorDirect(const LiveActor* pActor, const MR::FunctorBase& rFunctor, const char* pName1, const char* pName2) {
        if (!DemoFunction::isRegisteredDemoActionFunctor(pActor, pName1)) {
            return false;
        } else {
            DemoFunction::registerDemoActionFunctorFunction(pActor, rFunctor, pName1, pName2);
            return true;
        }
    }

    bool tryStartDemoRegistered(LiveActor* pActor, const char* pName) {
        return DemoFunction::tryStartDemoRegistered(pActor, pName);
    }

    bool tryStartDemoRegisteredMarioPuppetable(LiveActor* pActor, const char* pName) {
        return DemoFunction::tryStartDemoRegisteredMarioPuppetable(pActor, pName);
    }

    void registerDemoSimpleCastAll(LiveActor* pActor) {
        DemoFunction::registerDemoSimpleCastAllFunction(pActor);
    }

    void registerDemoSimpleCastAll(LayoutActor* pLayoutActor) {
        DemoFunction::registerDemoSimpleCastAllFunction(pLayoutActor);
    }

    void registerDemoSimpleCastAll(NameObj* pObj) {
        DemoFunction::registerDemoSimpleCastAllFunction(pObj);
    }

    bool isDemoCast(const LiveActor* pActor, const char* pName) {
        DemoExecutor* pExecutor = DemoFunction::findDemoExecutor(pActor);
        if (!pExecutor) {
            return false;
        }
        if (pName) {
            return DemoFunction::isRegisteredDemoCast(pActor, pName);
        } else {
            return DemoFunction::isDemoCast(pExecutor, pActor);
        }
    }

    bool isRegisteredDemoActionAppear(const LiveActor* pActor) {
        return DemoFunction::isRegisteredDemoActionAppear(pActor);
    }

    bool isRegisteredDemoActionNerve(const LiveActor* pActor) {
        return DemoFunction::isRegisteredDemoActionNerve(pActor);
    }

    bool tryStartDemo(LiveActor* pActor, const char* pName) {
        if (!MR::canStartDemo()) {
            return false;
        }

        DemoStartRequestUtil::startDemoSystem(pActor, pName, MR::MovementControlType_1, DemoStartInfo::DemoType_Programmable,
                                              DemoStartInfo::CinemaFrame_On, DemoStartInfo::StarPointer_Default, DemoStartInfo::DeleteEffect_Keep,
                                              nullptr);
        return true;
    }

    bool tryStartDemo(LayoutActor* pActor, const char* pName) {
        if (!MR::canStartDemo()) {
            return false;
        }

        DemoStartRequestUtil::startDemoSystem(pActor, pName, MR::MovementControlType_1, DemoStartInfo::DemoType_Programmable,
                                              DemoStartInfo::CinemaFrame_On, DemoStartInfo::StarPointer_Default, DemoStartInfo::DeleteEffect_Keep,
                                              nullptr);
        return true;
    }

    bool tryStartDemoWithoutCinemaFrame(LiveActor* pActor, const char* pName) {
        if (!MR::canStartDemo()) {
            return false;
        }

        DemoStartRequestUtil::startDemoSystem(pActor, pName, MR::MovementControlType_1, DemoStartInfo::DemoType_Programmable,
                                              DemoStartInfo::CinemaFrame_Off, DemoStartInfo::StarPointer_Default, DemoStartInfo::DeleteEffect_Keep,
                                              nullptr);
        return true;
    }

    bool tryStartDemoWithoutCinemaFrameValidStarPointer(LiveActor* pActor, const char* pName) {
        if (!MR::canStartDemo()) {
            return false;
        }

        DemoStartRequestUtil::startDemoSystem(pActor, pName, MR::MovementControlType_1, DemoStartInfo::DemoType_Programmable,
                                              DemoStartInfo::CinemaFrame_Off, DemoStartInfo::StarPointer_StarPointer,
                                              DemoStartInfo::DeleteEffect_Keep, nullptr);
        return true;
    }

    bool tryStartDemoWithoutCinemaFrameValidHandPointerFinger(NameObj* pObj, const char* pName) {
        if (!MR::canStartDemo()) {
            return false;
        }

        DemoStartRequestUtil::startDemoSystem(pObj, pName, MR::MovementControlType_1, DemoStartInfo::DemoType_Programmable,
                                              DemoStartInfo::CinemaFrame_Off, DemoStartInfo::StarPointer_HandPointerFinger,
                                              DemoStartInfo::DeleteEffect_Keep, nullptr);
        return true;
    }

    bool tryStartDemoWithoutCinemaFrameValidHandPointerFinger(LayoutActor* pActor, const char* pName) {
        if (!MR::canStartDemo()) {
            return false;
        }

        DemoStartRequestUtil::startDemoSystem(pActor, pName, MR::MovementControlType_1, DemoStartInfo::DemoType_Programmable,
                                              DemoStartInfo::CinemaFrame_Off, DemoStartInfo::StarPointer_HandPointerFinger,
                                              DemoStartInfo::DeleteEffect_Keep, nullptr);
        return true;
    }

    bool tryStartDemoMarioPuppetable(LiveActor* pActor, const char* pName) {
        if (!MR::canStartDemo()) {
            return false;
        }

        DemoStartRequestUtil::startDemoSystem(pActor, pName, MR::MovementControlType_2, DemoStartInfo::DemoType_Programmable,
                                              DemoStartInfo::CinemaFrame_On, DemoStartInfo::StarPointer_Default, DemoStartInfo::DeleteEffect_Keep,
                                              nullptr);
        return true;
    }

    bool tryStartDemoMarioPuppetableWithoutCinemaFrame(LiveActor* pActor, const char* pName) {
        if (!MR::canStartDemo()) {
            return false;
        }

        DemoStartRequestUtil::startDemoSystem(pActor, pName, MR::MovementControlType_2, DemoStartInfo::DemoType_Programmable,
                                              DemoStartInfo::CinemaFrame_Off, DemoStartInfo::StarPointer_Default, DemoStartInfo::DeleteEffect_Keep,
                                              nullptr);
        return true;
    }

    bool tryStartTimeKeepDemo(NameObj* pObj, const char* pName, const char* pPartName) {
        if (!MR::canStartDemo()) {
            return false;
        }

        if (pPartName != nullptr) {
            DemoFunction::findDemoExecutor(pName)->startDemoSystemPart(pPartName, MR::MovementControlType_1);
        } else {
            DemoStartRequestUtil::startDemoSystem(pObj, pName, MR::MovementControlType_1, DemoStartInfo::DemoType_TimeKeep,
                                                  DemoStartInfo::CinemaFrame_On, DemoStartInfo::StarPointer_Default,
                                                  DemoStartInfo::DeleteEffect_Keep, nullptr);
        }

        return true;
    }

    bool tryStartTimeKeepDemoMarioPuppetable(NameObj* pObj, const char* pName, const char* pPartName) {
        if (!MR::canStartDemo()) {
            return false;
        }

        DemoExecutor* pExecutor = DemoFunction::findDemoExecutor(pName);

        if (pPartName != nullptr) {
            pExecutor->startDemoSystemPart(pPartName, MR::MovementControlType_2);
        } else {
            DemoStartInfo::DeleteEffectType deleteEffect = DemoStartInfo::DeleteEffect_Keep;

            if (pExecutor->mPlayerKeeper != nullptr && pExecutor->mPlayerKeeper->isExistPosName()) {
                deleteEffect = DemoStartInfo::DeleteEffect_Delete;
            }

            DemoStartRequestUtil::startDemoSystem(pObj, pName, MR::MovementControlType_2, DemoStartInfo::DemoType_TimeKeep,
                                                  DemoStartInfo::CinemaFrame_On, DemoStartInfo::StarPointer_Default, deleteEffect, nullptr);
        }

        return true;
    }

    bool tryStartTimeKeepDemoMarioPuppetable(LiveActor* pActor, const char* pName, const char* pPartName) {
        if (!MR::canStartDemo()) {
            return false;
        }

        DemoExecutor* pExecutor = DemoFunction::findDemoExecutor(pName);

        if (pPartName != nullptr) {
            pExecutor->startDemoSystemPart(pPartName, MR::MovementControlType_2);
        } else {
            DemoStartInfo::DeleteEffectType deleteEffect = DemoStartInfo::DeleteEffect_Keep;

            if (pExecutor->mPlayerKeeper != nullptr && pExecutor->mPlayerKeeper->isExistPosName()) {
                deleteEffect = DemoStartInfo::DeleteEffect_Delete;
            }

            DemoStartRequestUtil::startDemoSystem(pActor, pName, MR::MovementControlType_2, DemoStartInfo::DemoType_TimeKeep,
                                                  DemoStartInfo::CinemaFrame_On, DemoStartInfo::StarPointer_Default, deleteEffect, nullptr);
        }

        return true;
    }

    bool requestStartDemo(LiveActor* pActor, const char* pName, const Nerve* pStartNerve, const Nerve* pEndNerve) {
        return DemoStartRequestUtil::requestStartDemo(pActor, pName, pStartNerve, pEndNerve, MR::MovementControlType_1,
                                                      DemoStartInfo::DemoType_Programmable, DemoStartInfo::CinemaFrame_On,
                                                      DemoStartInfo::StarPointer_Default, DemoStartInfo::DeleteEffect_Keep);
    }

    bool requestStartDemoWithoutCinemaFrame(LiveActor* pActor, const char* pName, const Nerve* pStartNerve, const Nerve* pEndNerve) {
        return DemoStartRequestUtil::requestStartDemo(pActor, pName, pStartNerve, pEndNerve, MR::MovementControlType_1,
                                                      DemoStartInfo::DemoType_Programmable, DemoStartInfo::CinemaFrame_Off,
                                                      DemoStartInfo::StarPointer_Default, DemoStartInfo::DeleteEffect_Keep);
    }

    bool requestStartDemoWithoutCinemaFrame(LayoutActor* pActor, const char* pName, const Nerve* pStartNerve, const Nerve* pEndNerve) {
        return DemoStartRequestUtil::requestStartDemo(pActor, pName, pStartNerve, pEndNerve, MR::MovementControlType_1,
                                                      DemoStartInfo::DemoType_Programmable, DemoStartInfo::CinemaFrame_Off,
                                                      DemoStartInfo::StarPointer_Default, DemoStartInfo::DeleteEffect_Keep);
    }

    bool requestStartDemoMarioPuppetable(LiveActor* pActor, const char* pName, const Nerve* pStartNerve, const Nerve* pEndNerve) {
        return DemoStartRequestUtil::requestStartDemo(pActor, pName, pStartNerve, pEndNerve, MR::MovementControlType_2,
                                                      DemoStartInfo::DemoType_Programmable, DemoStartInfo::CinemaFrame_On,
                                                      DemoStartInfo::StarPointer_Default, DemoStartInfo::DeleteEffect_Keep);
    }

    bool requestStartDemoMarioPuppetable(NerveExecutor* pExecutor, LiveActor* pActor, const char* pName, const Nerve* pStartNerve,
                                         const Nerve* pEndNerve) {
        return DemoStartRequestUtil::requestStartDemo(pExecutor, pActor, pName, pStartNerve, pEndNerve, MR::MovementControlType_2,
                                                      DemoStartInfo::DemoType_Programmable, DemoStartInfo::CinemaFrame_On,
                                                      DemoStartInfo::StarPointer_Default, DemoStartInfo::DeleteEffect_Keep);
    }

    bool requestStartDemoMarioPuppetableWithoutCinemaFrame(LiveActor* pActor, const char* pName, const Nerve* pStartNerve,
                                                           const Nerve* pEndNerve) {
        return DemoStartRequestUtil::requestStartDemo(pActor, pName, pStartNerve, pEndNerve, MR::MovementControlType_2,
                                                      DemoStartInfo::DemoType_Programmable, DemoStartInfo::CinemaFrame_Off,
                                                      DemoStartInfo::StarPointer_Default, DemoStartInfo::DeleteEffect_Keep);
    }

    bool requestStartTimeKeepDemo(LiveActor* pActor, const char* pName, const Nerve* pStartNerve, const Nerve* pEndNerve,
                                  const char* pPartName) {
        return DemoStartRequestUtil::requestStartTimeKeepDemo(pActor, pName, pPartName, pStartNerve, pEndNerve, MR::MovementControlType_1,
                                                              DemoStartInfo::DemoType_TimeKeep, DemoStartInfo::CinemaFrame_On,
                                                              DemoStartInfo::StarPointer_Default, DemoStartInfo::DeleteEffect_Keep);
    }

    bool requestStartTimeKeepDemo(NameObj* pObj, const char* pName, const char* pPartName) {
        return DemoStartRequestUtil::requestStartTimeKeepDemo(pObj, pName, pPartName, MR::MovementControlType_1,
                                                              DemoStartInfo::DemoType_TimeKeep, DemoStartInfo::CinemaFrame_On,
                                                              DemoStartInfo::StarPointer_Default, DemoStartInfo::DeleteEffect_Keep);
    }

    bool requestStartTimeKeepDemoMarioPuppetable(LiveActor* pActor, const char* pName, const Nerve* pStartNerve, const Nerve* pEndNerve,
                                                 const char* pPartName) {
        return DemoStartRequestUtil::requestStartTimeKeepDemo(pActor, pName, pPartName, pStartNerve, pEndNerve, MR::MovementControlType_2,
                                                              DemoStartInfo::DemoType_TimeKeep, DemoStartInfo::CinemaFrame_On,
                                                              DemoStartInfo::StarPointer_Default, DemoStartInfo::DeleteEffect_Keep);
    }

    bool requestStartTimeKeepDemoMarioPuppetable(NerveExecutor* pExecutor, LiveActor* pActor, const char* pName, const Nerve* pStartNerve,
                                                 const Nerve* pEndNerve, const char* pPartName) {
        return DemoStartRequestUtil::requestStartTimeKeepDemo(pExecutor, pActor, pName, pPartName, pStartNerve, pEndNerve,
                                                              MR::MovementControlType_2, DemoStartInfo::DemoType_TimeKeep,
                                                              DemoStartInfo::CinemaFrame_On, DemoStartInfo::StarPointer_Default,
                                                              DemoStartInfo::DeleteEffect_Keep);
    }

    bool requestStartTimeKeepDemoMarioPuppetable(NameObj* pObj, const char* pName, const char* pPartName) {
        return DemoStartRequestUtil::requestStartTimeKeepDemo(pObj, pName, pPartName, MR::MovementControlType_2,
                                                              DemoStartInfo::DemoType_TimeKeep, DemoStartInfo::CinemaFrame_On,
                                                              DemoStartInfo::StarPointer_Default, DemoStartInfo::DeleteEffect_Keep);
    }

    bool requestStartTimeKeepDemoWithoutCinemaFrame(LiveActor* pActor, const char* pName, const Nerve* pStartNerve,
                                                    const Nerve* pEndNerve, const char* pPartName) {
        return DemoStartRequestUtil::requestStartTimeKeepDemo(pActor, pName, pPartName, pStartNerve, pEndNerve, MR::MovementControlType_1,
                                                              DemoStartInfo::DemoType_TimeKeep, DemoStartInfo::CinemaFrame_Off,
                                                              DemoStartInfo::StarPointer_Default, DemoStartInfo::DeleteEffect_Keep);
    }

    bool requestStartDemoRegistered(LiveActor* pActor, const Nerve* pStartNerve, const Nerve* pEndNerve, const char* pPartName) {
        DemoExecutor* pExecutor = DemoFunction::findDemoExecutor(pActor);

        if (pExecutor == nullptr) {
            return false;
        }

        return DemoStartRequestUtil::requestStartTimeKeepDemo(pActor, pExecutor->mName, pPartName, pStartNerve, pEndNerve,
                                                              MR::MovementControlType_1, DemoStartInfo::DemoType_TimeKeep,
                                                              DemoStartInfo::CinemaFrame_On, DemoStartInfo::StarPointer_Default,
                                                              DemoStartInfo::DeleteEffect_Keep);
    }

    bool requestStartDemoRegisteredMarioPuppetable(LiveActor* pActor, const Nerve* pStartNerve, const Nerve* pEndNerve,
                                                   const char* pPartName) {
        DemoExecutor* pExecutor = DemoFunction::findDemoExecutor(pActor);

        if (pExecutor == nullptr) {
            return false;
        }

        return DemoStartRequestUtil::requestStartTimeKeepDemo(pActor, pExecutor->mName, pPartName, pStartNerve, pEndNerve,
                                                              MR::MovementControlType_2, DemoStartInfo::DemoType_TimeKeep,
                                                              DemoStartInfo::CinemaFrame_On, DemoStartInfo::StarPointer_Default,
                                                              DemoStartInfo::DeleteEffect_Keep);
    }

    void endDemo(NameObj* pObj, const char* pName) {
        DemoFunction::getDemoDirector()->endDemo(pObj, pName, false);
    }

    void endDemoWaitCameraInterpolating(NameObj* pObj, const char* pName) {
        DemoFunction::getDemoDirector()->endDemo(pObj, pName, true);
    }

    void initDemoSheetTalkAnim(LiveActor* pActor, const JMapInfoIter& rIter, const char* pName1, const char* pName2, TalkMessageCtrl* pTalkCtrl) {
        tryInitDemoSheetTalkAnim(pActor, rIter, pName1, pName2, pTalkCtrl);
    }

    void initDemoSheetTalkAnimWithMessageName(LiveActor* pActor, const JMapInfoIter& rIter, const char* pName1, const char* pName2,
                                              const char* pMessageName) {
        TVec3f offset(0.0f, 0.0f, 0.0f);
        TalkMessageCtrl* pTalkCtrl = MR::createTalkCtrl(pActor, rIter, pMessageName, offset, nullptr);

        MR::onRootNodeAutomatic(pTalkCtrl);
        MR::tryInitDemoSheetTalkAnim(pActor, rIter, pName1, pName2, pTalkCtrl);
    }

    void initDemoSheetTalkAnimFunctor(LiveActor* pActor, const JMapInfoIter& rIter, const char* pName1, const char* pName2,
                                      TalkMessageCtrl* pTalkCtrl, const MR::FunctorBase& rFunctor) {
        if (tryInitDemoSheetTalkAnim(pActor, rIter, pName1, pName2, pTalkCtrl)) {
            DemoFunction::registerDemoActionFunctorFunction(pActor, rFunctor, pName1, nullptr);
        }
    }

    bool tryInitDemoSheetTalkAnim(LiveActor* pActor, const JMapInfoIter& rIter, const char* pName1, const char* pName2, TalkMessageCtrl* pTalkCtrl) {
        if (!DemoFunction::getDemoDirector()->registerDemoCast(pActor, pName1, rIter)) {
            return false;
        }

        if (pTalkCtrl == nullptr) {
            TVec3f offset(0.0f, 0.0f, 0.0f);
            pTalkCtrl = MR::createTalkCtrl(pActor, rIter, pName2, offset, nullptr);
            MR::onRootNodeAutomatic(pTalkCtrl);
        }

        DemoFunction::registerDemoTalkMessageCtrlDirect(pActor, pTalkCtrl, pName1);
        DemoFunction::tryCreateDemoTalkAnimCtrlForSceneDirect(pActor, pName1, rIter, pName2, nullptr, 0, 0);

        return true;
    }

    void startTimeKeepDemo(NameObj* pObj, const char* pName, const char* pPartName) {
        if (pPartName != nullptr) {
            DemoFunction::findDemoExecutor(pName)->startDemoSystemPart(pPartName, MR::MovementControlType_1);
        } else {
            DemoStartRequestUtil::startDemoSystem(pObj, pName, MR::MovementControlType_1, DemoStartInfo::DemoType_TimeKeep,
                                                  DemoStartInfo::CinemaFrame_On, DemoStartInfo::StarPointer_Default,
                                                  DemoStartInfo::DeleteEffect_Keep, nullptr);
        }
    }

    void startTimeKeepDemoMarioPuppetable(NameObj* pObj, const char* pName, const char* pPartName) {
        DemoExecutor* pExecutor = DemoFunction::findDemoExecutor(pName);

        if (pPartName != nullptr) {
            pExecutor->startDemoSystemPart(pPartName, MR::MovementControlType_2);
        } else {
            DemoStartInfo::DeleteEffectType deleteEffect = DemoStartInfo::DeleteEffect_Keep;

            if (pExecutor->mPlayerKeeper != nullptr && pExecutor->mPlayerKeeper->isExistPosName()) {
                deleteEffect = DemoStartInfo::DeleteEffect_Delete;
            }

            DemoStartRequestUtil::startDemoSystem(pObj, pName, MR::MovementControlType_2, DemoStartInfo::DemoType_TimeKeep,
                                                  DemoStartInfo::CinemaFrame_On, DemoStartInfo::StarPointer_Default, deleteEffect, nullptr);
        }
    }

    bool isDemoExist(const char* pName) {
        return DemoFunction::getDemoDirector()->isExistTimeKeepDemo(pName);
    }

    bool isDemoActive() {
        return DemoFunction::getDemoDirector()->mIsActive;
    }

    bool isDemoActive(const char* pName) {
        DemoExecutor* pExecutor = DemoFunction::getDemoDirector()->mExecutor;
        if (pExecutor) {
            if (isName(pExecutor, pName)) {
                return true;
            }
        }
        char* pDemoName = DemoFunction::getDemoDirector()->getCurrentDemoName();
        if (!pDemoName) {
            return false;
        } else {
            return isEqualString(pDemoName, pName);
        }
    }

    bool canStartDemo() {
        if (DemoFunction::getDemoDirector()->mIsActive) {
            return false;
        }
        if (isPlayerDead()) {
            return false;
        }
        if (GameSceneFunction::isExecStageClearDemo()) {
            return false;
        } else {
            return !isPlayerConfrontDeath();
        }
    }

    bool isTimeKeepDemoActive() {
        if (DemoFunction::getDemoDirector()->mIsActive == false) {
            return false;
        }
        return DemoFunction::getDemoDirector()->mExecutor != nullptr;
    }

    bool isDemoActiveRegistered(const LiveActor* pActor) {
        DemoExecutor* pExecutor = DemoFunction::findDemoExecutor(pActor);
        if (pExecutor == false) {
            return false;
        } else {
            return DemoFunction::getDemoDirector()->mExecutor == pExecutor;
        }
    }

    bool isDemoPartExist(const LiveActor* pActor, const char* pName) {
        DemoExecutor* pExecutor = DemoFunction::findDemoExecutor(pActor);
        if (pExecutor) {
            return DemoFunction::isExistDemoPart(pExecutor, pName);
        } else {
            return false;
        }
    }

    bool isDemoLastStep() {
        return DemoFunction::isDemoLastPartLastStep();
    }

    bool isDemoPartActive(const char* pName) {
        return DemoFunction::isDemoPartActiveFunction(pName);
    }

    bool isDemoPartStep(const char* pName, s32 a2) {
        if (DemoFunction::isDemoPartActiveFunction(pName) == false) {
            return false;
        } else {
            return DemoFunction::getDemoPartStepFunction(pName) == a2;
        }
    }

    bool isDemoPartFirstStep(const char* pName) {
        if (DemoFunction::isDemoPartActiveFunction(pName) == false) {
            return false;
        } else {
            return DemoFunction::getDemoPartStepFunction(pName) == 0;
        }
    }

    bool isDemoPartLastStep(const char *pName) {
        if (DemoFunction::isDemoPartActiveFunction(pName) == false) {
            return false;
        }
        else {
            s32 totalSteps = DemoFunction::getDemoPartTotalStepFunction(pName);
            return totalSteps - 1 == DemoFunction::getDemoPartStepFunction(pName);
        }
    }

    bool isDemoPartLessEqualStep(const char* pName, s32 a2) {
        if (DemoFunction::isDemoPartActiveFunction(pName) == false) {
            return false;
        } else {
            return DemoFunction::getDemoPartStepFunction(pName) <= a2;
        }
    }

    bool isDemoPartGreaterStep(const char* pName, s32 a2) {
        if (DemoFunction::isDemoPartActiveFunction(pName) == false) {
            return false;
        } else {
            return DemoFunction::getDemoPartStepFunction(pName) > a2;
        }
    }

    s32 getDemoPartTotalStep(const char* pName) {
        return DemoFunction::getDemoPartTotalStepFunction(pName);
    }

    f32 calcDemoPartStepRate(const char* pName) {
        s32 totalStep = DemoFunction::getDemoPartTotalStepFunction(pName);
        s32 step = DemoFunction::getDemoPartStepFunction(pName);

        return static_cast< f32 >(step) / static_cast< f32 >(totalStep);
    }

    s32 getDemoPartStep(const char* pName) {
        return DemoFunction::getDemoPartStepFunction(pName);
    }

    void pauseTimeKeepDemo(LiveActor* pActor) {
        DemoFunction::pauseTimeKeepDemo(pActor);
    }

    void resumeTimeKeepDemo(LiveActor* pActor) {
        DemoFunction::resumeTimeKeepDemo(pActor);
    }

    bool isPowerStarGetDemoActive() {
        return GameSceneFunction::isExecStageClearDemo();
    }

    const char* getCurrentDemoPartNameMain(const char* pName) {
        return DemoFunction::getCurrentDemoPartNameMain(pName);
    }

    void startTalkingSequenceWithoutCinemaFrame(NameObj* pObj) {
        DemoStartRequestUtil::startDemoSystem(pObj, "会話", MR::MovementControlType_3, DemoStartInfo::DemoType_Programmable,
                                              DemoStartInfo::CinemaFrame_Off, DemoStartInfo::StarPointer_Default, DemoStartInfo::DeleteEffect_Keep,
                                              nullptr);
    }

    void endTalkingSequence(NameObj* pObj) {
        const char* pName = "会話";
        DemoFunction::getDemoDirector()->endDemo(pObj, pName, false);
    }

    bool isSystemTalking() {
        if (isExistSceneObj(0x19) == false) {
            return false;
        } else {
            return getSceneObj< TalkDirector >(SceneObj_TalkDirector)->isSystemTalking();
        }
    }

    bool isNormalTalking() {
        if (isExistSceneObj(0x19) == false) {
            return false;
        } else {
            return getSceneObj< TalkDirector >(SceneObj_TalkDirector)->isNormalTalking();
        }
    }

    LiveActor* getTalkingActor() {
        if (isExistSceneObj(0x19) == false) {
            return false;
        } else {
            return getSceneObj< TalkDirector >(SceneObj_TalkDirector)->getTalkingActor();
        }
    }

    bool isDemoPartTalk(const char* pName) {
        return DemoFunction::isDemoPartTalk(pName);
    }

    void startTalkingSequence(NameObj* pObj) {
        DemoStartRequestUtil::startDemoSystem(pObj, "会話", MR::MovementControlType_3, DemoStartInfo::DemoType_Programmable,
                                              DemoStartInfo::CinemaFrame_On, DemoStartInfo::StarPointer_Default, DemoStartInfo::DeleteEffect_Keep,
                                              nullptr);
    }
};  // namespace MR
