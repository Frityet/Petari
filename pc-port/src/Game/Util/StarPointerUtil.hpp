#pragma once

#include <revolution/types.h>

class LayoutActor;

namespace MR {
    bool isStarPointerPointingPane(const LayoutActor* pLayout, const char* pPaneName, s32, bool, const char*);
    bool isStarPointerPointingPaneForMeterLayout(const LayoutActor* pLayout, const char* pPaneName, s32, bool, const char*);
    void startStarPointerModeTitle(void* host);
    void startStarPointerModeFileSelect(void* host);
    void requestStarPointerModeSaveLoad(void* host);
    void activeStarPointerGuidance();
    void deactiveStarPointerGuidance();
    bool requestFileSelectGuidance();
    bool requestFileSelectCopyGuidance();
}  // namespace MR
