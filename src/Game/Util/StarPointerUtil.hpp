#pragma once

#include <JSystem/JGeometry/TVec.hpp>
#include <revolution/types.h>

class LayoutActor;
class LiveActor;

namespace MR {
    bool isStarPointerPointingPane(const LayoutActor* pLayout, const char* pPaneName, s32, bool, const char*);
    bool isStarPointerPointingPaneForMeterLayout(const LayoutActor* pLayout, const char* pPaneName, s32, bool, const char*);
    void initStarPointerTarget(LiveActor* pActor, f32 radius, const TVec3f& rOffset);
    void initStarPointerTargetAtPos(LiveActor*, f32, const TVec3f*, const TVec3f&);
    void initStarPointerTargetAtMtx(LiveActor*, f32, MtxPtr, const TVec3f&);
    void initStarPointerTargetAtJoint(LiveActor*, const char*, f32, const TVec3f&);
    MtxPtr getStarPointerViewMtx();
    Mtx44Ptr getStarPointerProjMtx();
    bool isStarPointerPointing1PWithoutCheckZ(const LiveActor* pActor, const char*, bool, bool);
    bool isStarPointerPointing2P(const LiveActor* pActor, const char*, bool, bool);
    bool isStarPointerPointing2POnPressButton(const LiveActor*, const char*, bool, bool);
    bool isStarPointerPointing2POnTriggerButton(const LiveActor*, const char*, bool, bool);
    s32* getStarPointerLastPointedPort(const LiveActor*);
    bool isStarPointerPointingFileSelect(const LiveActor* pActor);
    bool isExistStarPointerTarget(const LiveActor* pActor);
    void setStarPointerTargetRadius3d(LiveActor* pActor, f32 radius);
    TVec2f* getStarPointerScreenPosition(s32 channel);
    TVec3f* getStarPointerWorldPosUsingDepth(s32);
    TVec2f getStarPointerScreenPositionOrEdge(s32 channel);
    TVec2f* getStarPointerScreenVelocity(s32 channel);
    f32 getStarPointerScreenSpeed(s32 channel);
    bool isStarPointerInScreen(s32 channel);
    void startStarPointerModeTitle(void* host);
    void startStarPointerModeFileSelect(void* host);
    void startStarPointerModeSphereSelectorFinger(void* host);
    void startStarPointerModeSphereSelectorOnReaction(void* host);
    void startStarPointerModeDemoMarioDeath(void* host);
    void endStarPointerMode(void* host);
    void requestStarPointerModeSaveLoad(void* host);
    void requestStarPointerModePictureBook(void* host);
    void activeStarPointerGuidance();
    void deactiveStarPointerGuidance();
    bool requestFileSelectGuidance();
    bool requestFileSelectCopyGuidance();
}  // namespace MR

namespace MR {
    bool isExistStarPointerGuidance();
    bool isExistStarPointerGuidanceFrame1P();
    bool isStarPointerInScreenAnyPort(s32*);
    void startStarPointerModeChooseYesNo(void*);
    f32 calcPointRadius2D(const TVec3f&, f32);
}

namespace MR {
    bool isStarPointerValid(s32);
    bool isStarPointerModeMarioLauncher();
}

namespace MR {
    void initStarPointerGameScene();
    void createStarPointerLayout();
    void onStarPointerSceneOut();
    void setStarPointerModeBase();
    void setStarPointerCameraMtxAtGameScene();
    void addStarPointerTargetCircle(LayoutActor*, const char*, f32, const TVec2f&, const char*);
    bool isStarPointerPointing(const LiveActor*, s32, bool, const char*);
    bool tryStarPointerCheck(const LiveActor*, bool, const char*);
    bool tryStarPointerCheckWithoutRumble(LiveActor*, bool);
    bool isStarPointerPointingTarget(const LayoutActor*, const char*, s32, bool, const char*);
    bool isStarPointerPointing1P(const LiveActor*, const char*, bool, bool);
    bool isStarPointerPointing1Por2P(const LiveActor*, const char*, bool, bool);
    bool requestTicoSeedGuidanceForce();
    bool requestPointerGuidanceNoInformation();
    void tryShowTimeoutedStarPointerGuidance();
    bool isStarPointerPointing(const TVec3f&, f32, s32, bool);
    f32 getStarPointerRadius(s32);
    void getStarPointerWorldVelocityDirection(TVec3f*, s32);
    void calcStarPointerWorldPointingPos(TVec3f*, const TVec3f&, s32);
    void calcStarPointerWorldPointingPosInsideEdge(TVec3f*, const TVec3f&, s32);
    bool calcStarPointerPosOnPlane(TVec3f*, const TVec3f&, const TVec3f&, s32, bool);
    bool calcStarPointerWorldVelocityDirectionOnPlane(TVec3f*, const TVec3f&, const TVec3f&, s32);
    bool calcStarPointerStrokeRotateMoment(TVec3f*, const TVec3f&, f32, s32);
    bool calcStarPointerScreenDistanceToTarget(const LiveActor*, f32*, s32);
    bool tryStartStarPointerCommandStream(const LiveActor*, const TVec3f*, s32, bool);
    bool tryEndStarPointerCommandStream(const LiveActor*, s32);
    bool isStarPointerCommandStream(const LiveActor*, s32);
    void startStarPointerModeGame(void*);
    void startStarPointerModeDemo(void*);
    void startStarPointerModeDemoWithStarPointer(void*);
    void startStarPointerModeDemoWithHandPointerFinger(void*);
    void startStarPointerModeMarioLauncher(void*);
    void startStarPointerModeHomeButton(void*);
    void startStarPointerModePauseMenu(void*);
    void startStarPointerModeScenarioSelectScene(void*);
    void startStarPointerModeBlueStar(void*);
    void startStarPointerModePowerStarGetDemo(void*);
    void startStarPointerModeStarPieceTarget(void*);
    void startStarPointerModeEnding(void*);
    void startStarPointerModeCommandStream(void*);
    void startStarPointerMode1PInvalid2PValid(void*);
    void requestStarPointerModeErrorWindow(void*);
    void requestStarPointerModePauseMenu(void*);
    void requestStarPointerModeBlueStarReady(void*);
    void requestStarPointerModeBigBubble(void*, const TVec3f&);
    bool isStarPointerModeBlueStarReady();
    bool isStarPointerModeStarPieceTarget();
    bool isStarPointerModeHomeButton();
    bool isStarPointerModeErrorWindow();
    void enableStarPointerShootStarPiece();
    void disableStarPointerShootStarPiece();
    bool isEnableStarPointerShootStarPiece(s32);
    bool isStarPointer2PTransparencyMode();
    bool isStarPointer1PInvalid2PValidMode();
    void setStarPointerDrawSyncToken();
    bool requestBlueStarGuidance();
    bool requestTicoSeedGuidance(s32);
    bool requestBigBubbleGuidance();
    bool requestMarioLauncherGuidance();
    bool requestStarPieceLectureGuidance();
}
