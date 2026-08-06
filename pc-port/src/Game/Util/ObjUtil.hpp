#pragma once

#include "Game/Scene/SceneFunction.hpp"

class LiveActor;
class LayoutActor;
class NameObj;
class JMapInfoIter;
class StageSwitchCtrl;

namespace MR {
    class FunctorBase;

    void requestMovementOn(NameObj* pObj);
    void requestMovementOff(NameObj* pObj);
    void connectToSceneMapObj(LiveActor* pActor);
    void connectToSceneMapObjMovement(NameObj* pObj);
    void connectToSceneNoSilhouettedMapObj(LiveActor* pActor);
    void connectToSceneItemStrongLight(LiveActor* pActor);
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
    bool tryRumblePad(const void* pSource, const char* pPatternName, s32 channel);
    void shakeCameraStrong();
    void shakeCameraNormal();
    void shakeCameraWeak();
    void declarePowerStarCoin100();
    void declareStarPiece(const NameObj* pObj, s32 num);
    void listenNameObjStageSwitchOnAppear(const NameObj*, const StageSwitchCtrl*, const MR::FunctorBase&);
    void listenNameObjStageSwitchOnOffAppear(const NameObj*, const StageSwitchCtrl*, const MR::FunctorBase&, const MR::FunctorBase&);
    void listenNameObjStageSwitchOnA(const NameObj*, const StageSwitchCtrl*, const MR::FunctorBase&);
    void listenNameObjStageSwitchOnOffA(const NameObj*, const StageSwitchCtrl*, const MR::FunctorBase&, const MR::FunctorBase&);
    void listenNameObjStageSwitchOnB(const NameObj*, const StageSwitchCtrl*, const MR::FunctorBase&);
    void listenNameObjStageSwitchOffB(const NameObj*, const StageSwitchCtrl*, const MR::FunctorBase&);
    void listenNameObjStageSwitchOnOffB(const NameObj*, const StageSwitchCtrl*, const MR::FunctorBase&, const MR::FunctorBase&);
}  // namespace MR
