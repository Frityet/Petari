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
