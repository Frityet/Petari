#pragma once

#include <revolution/types.h>

struct ResTIMG;

namespace MR {
    void closeWipeCircle(s32 frameCount);
    void forceOpenWipeCircle();
    void forceCloseWipeCircle();
    void closeWipeFade(s32 frameCount);
    void forceOpenWipeFade();
    void forceCloseWipeFade();
    void closeWipeWhiteFade(s32 frameCount);
    void forceOpenWipeWhiteFade();
    void forceCloseWipeWhiteFade();
    bool isWipeActive();
    bool isWipeBlank();
    bool isWipeOpen();
    void closeSystemWipeCircle(s32 frameCount);
    void openSystemWipeFade(s32 frameCount);
    void closeSystemWipeFade(s32 frameCount);
    void forceOpenSystemWipeFade();
    void openSystemWipeWhiteFade(s32 frameCount);
    void closeSystemWipeWhiteFade(s32 frameCount);
    void forceCloseSystemWipeWhiteFade();
    bool isSystemWipeActive();
    void startToCaptureScreen(const char* pName);
    void endToCaptureScreen(const char* pName);
    void captureScreenIfAllow(const char* pName);
    const ResTIMG* getScreenResTIMG();
    u8* getScreenTexImage();
    void closeSystemWipeCircleWithCaptureScreen(s32 frameCount);
    void closeSystemWipeFadeWithCaptureScreen(s32 frameCount);
    void deactivateDefaultGameLayout();
    void openWipeCircle(s32 frameCount);
    void openWipeFade(s32 frameCount);
    void openWipeWhiteFade(s32 frameCount);
}  // namespace MR
