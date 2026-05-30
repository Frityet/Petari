#pragma once

#include "Game/Scene/SceneFunction.hpp"

class LiveActor;
class LayoutActor;
class NameObj;
class JMapInfoIter;

namespace MR {
    class FunctorBase;

    void requestMovementOn(NameObj* pObj);
    void requestMovementOff(NameObj* pObj);
    void connectToSceneMapObj(LiveActor* pActor);
    void connectToSceneMapObjMovement(NameObj* pObj);
    void connectToSceneAreaObj(NameObj* pObj);
    void connectToSceneNpc(LiveActor* pActor);
    void connectToSceneNpcMovement(NameObj* pObj);
    void connectToSceneLayout(LayoutActor* pLayout);
    void connectToSceneLayoutDecoration(LayoutActor* pLayout);
    void connectToSceneTalkLayout(LayoutActor* pLayout);
    void connectToSceneLayoutOnPause(LayoutActor* pLayout);
    bool isExistResourceInArc(const char* pArcName, const char* pResourceName);
    bool tryRumblePadStrong(const void* pSource, s32 channel);
    bool tryRumblePadWeak(const void* pSource, s32 channel);
    void shakeCameraNormal();
    bool useStageSwitchReadA(LiveActor* pActor, const JMapInfoIter& rIter);
    bool useStageSwitchReadB(LiveActor* pActor, const JMapInfoIter& rIter);
    bool useStageSwitchReadAppear(LiveActor* pActor, const JMapInfoIter& rIter);
    void useStageSwitchSleep(LiveActor* pActor, const JMapInfoIter& rIter);
    bool useStageSwitchWriteA(LiveActor* pActor, const JMapInfoIter& rIter);
    bool useStageSwitchWriteB(LiveActor* pActor, const JMapInfoIter& rIter);
    bool useStageSwitchWriteDead(LiveActor* pActor, const JMapInfoIter& rIter);
    bool needStageSwitchReadA(LiveActor* pActor, const JMapInfoIter& rIter);
    bool needStageSwitchReadB(LiveActor* pActor, const JMapInfoIter& rIter);
    bool needStageSwitchReadAppear(LiveActor* pActor, const JMapInfoIter& rIter);
    bool needStageSwitchWriteA(LiveActor* pActor, const JMapInfoIter& rIter);
    bool needStageSwitchWriteB(LiveActor* pActor, const JMapInfoIter& rIter);
    bool needStageSwitchWriteDead(LiveActor* pActor, const JMapInfoIter& rIter);
    bool isValidSwitchA(const LiveActor* pActor);
    bool isValidSwitchB(const LiveActor* pActor);
    bool isValidSwitchAppear(const LiveActor* pActor);
    bool isValidSwitchDead(const LiveActor* pActor);
    bool isOnSwitchA(const LiveActor* pActor);
    bool isOnSwitchB(const LiveActor* pActor);
    bool isOnSwitchAppear(const LiveActor* pActor);
    bool isOnStageSwitch(s32 switchId);
    void onSwitchA(LiveActor* pActor);
    void onSwitchB(LiveActor* pActor);
    void onSwitchDead(LiveActor* pActor);
    void onStageSwitchById(s32 switchId);
    void offSwitchA(LiveActor* pActor);
    void offSwitchB(LiveActor* pActor);
    void offSwitchDead(LiveActor* pActor);
    void offStageSwitchById(s32 switchId);
    void listenStageSwitchOnAppear(LiveActor* pActor, const MR::FunctorBase& rFunctor);
    void listenStageSwitchOnOffAppear(LiveActor* pActor, const MR::FunctorBase& rFunctor1, const MR::FunctorBase& rFunctor2);
    void listenStageSwitchOnA(LiveActor* pActor, const MR::FunctorBase& rFunctor);
    void listenStageSwitchOnOffA(LiveActor* pActor, const MR::FunctorBase& rFunctor1, const MR::FunctorBase& rFunctor2);
    void listenStageSwitchOnB(LiveActor* pActor, const MR::FunctorBase& rFunctor);
    void listenStageSwitchOffB(LiveActor* pActor, const MR::FunctorBase& rFunctor);
    void listenStageSwitchOnOffB(LiveActor* pActor, const MR::FunctorBase& rFunctor1, const MR::FunctorBase& rFunctor2);
}  // namespace MR
