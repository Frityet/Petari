#pragma once

#include "Game/Scene/SceneFunction.hpp"
#include "JSystem/JGeometry/TVec.hpp"

class LiveActor;
class LayoutActor;
class NameObj;
class JMapInfoIter;
class StageSwitchCtrl;

namespace MR {
    void connectToSceneMapObjDecorationMovement(NameObj* pObj);
    class FunctorBase;

    bool isInWater(const TVec3f&);
    bool isInDeath(const TVec3f&);
    bool isInDarkMatter(const TVec3f&);
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
    bool tryRumblePad(const void*, const char*, s32);
    bool tryRumblePadVeryStrongLong(const void*, s32);
    bool tryRumblePadVeryStrong(const void*, s32);
    bool tryRumblePadStrong(const void*, s32);
    bool tryRumblePadMiddle(const void*, s32);
    bool tryRumblePadWeak(const void*, s32);
    bool tryRumblePadVeryWeak(const void*, s32);
    bool tryRumbleDefaultHit(const void*, s32);
    void shakeCameraVeryStrong();
    void shakeCameraStrong();
    void shakeCameraNormalStrong();
    void shakeCameraNormal();
    void shakeCameraNormalWeak();
    void shakeCameraWeak();
    void shakeCameraVeryWeak();
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
