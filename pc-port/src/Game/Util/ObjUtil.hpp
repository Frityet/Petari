#pragma once

#include "Game/Scene/SceneFunction.hpp"

class LiveActor;
class LayoutActor;
class NameObj;
class JMapInfoIter;

namespace MR {
    void requestMovementOn(NameObj* pObj);
    void requestMovementOff(NameObj* pObj);
    void connectToSceneMapObj(LiveActor* pActor);
    void connectToSceneMapObjMovement(NameObj* pObj);
    void connectToSceneNpc(LiveActor* pActor);
    void connectToSceneLayout(LayoutActor* pLayout);
    void connectToSceneLayoutDecoration(LayoutActor* pLayout);
    void connectToSceneTalkLayout(LayoutActor* pLayout);
    void connectToSceneLayoutOnPause(LayoutActor* pLayout);
    bool isExistResourceInArc(const char* pArcName, const char* pResourceName);
    bool tryRumblePadStrong(const void* pSource, s32 channel);
    bool tryRumblePadWeak(const void* pSource, s32 channel);
    void shakeCameraNormal();
    bool useStageSwitchWriteA(LiveActor* pActor, const JMapInfoIter& rIter);
    void onSwitchA(LiveActor* pActor);
    void offSwitchA(LiveActor* pActor);
}  // namespace MR
