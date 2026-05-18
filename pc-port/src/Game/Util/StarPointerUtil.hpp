#pragma once

#include <revolution/types.h>

class LayoutActor;
class LiveActor;
struct TVec2f;
struct TVec3f;

namespace MR {
    bool isStarPointerPointingPane(const LayoutActor* pLayout, const char* pPaneName, s32, bool, const char*);
    bool isStarPointerPointingPaneForMeterLayout(const LayoutActor* pLayout, const char* pPaneName, s32, bool, const char*);
    void initStarPointerTarget(LiveActor* pActor, f32 radius, const TVec3f& rOffset);
    bool isStarPointerPointing1PWithoutCheckZ(const LiveActor* pActor, const char*, bool, bool);
    bool isStarPointerPointingFileSelect(const LiveActor* pActor);
    bool isExistStarPointerTarget(const LiveActor* pActor);
    void setStarPointerTargetRadius3d(LiveActor* pActor, f32 radius);
    TVec2f* getStarPointerScreenPosition(s32 channel);
    TVec2f* getStarPointerScreenVelocity(s32 channel);
    f32 getStarPointerScreenSpeed(s32 channel);
    bool isStarPointerInScreen(s32 channel);
    void startStarPointerModeTitle(void* host);
    void startStarPointerModeFileSelect(void* host);
    void requestStarPointerModeSaveLoad(void* host);
    void requestStarPointerModePictureBook(void* host);
    void activeStarPointerGuidance();
    void deactiveStarPointerGuidance();
    bool requestFileSelectGuidance();
    bool requestFileSelectCopyGuidance();
}  // namespace MR
