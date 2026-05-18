#include "Game/Util/ScreenUtil.hpp"

#include "Game/Screen/CaptureScreenDirector.hpp"
#include "Game/compat/RuntimeContext.hpp"

namespace MR {
    namespace {
        constexpr auto cCircleWipeName = "円ワイプ";
        constexpr auto cFadeWipeName = "フェードワイプ";
        constexpr auto cWhiteFadeWipeName = "白フェードワイプ";

        smgpc::game::WipeService* scene_wipe() {
            if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
                return &runtime->scene_wipe();
            }

            return nullptr;
        }

        smgpc::game::WipeService* system_wipe() {
            if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
                return &runtime->system_wipe();
            }

            return nullptr;
        }

        CaptureScreenDirector* capture_screen_director() {
            if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
                return &runtime->capture_screen_director();
            }

            return nullptr;
        }
    }  // namespace

    void closeWipeCircle(s32 frameCount) {
        if (auto* wipe = scene_wipe()) {
            wipe->close(cCircleWipeName, frameCount);
        }
    }

    void forceOpenWipeCircle() {
        if (auto* wipe = scene_wipe()) {
            wipe->force_open(cCircleWipeName);
        }
    }

    void forceCloseWipeCircle() {
        if (auto* wipe = scene_wipe()) {
            wipe->force_close(cCircleWipeName);
        }
    }

    void closeWipeFade(s32 frameCount) {
        if (auto* wipe = scene_wipe()) {
            wipe->close(cFadeWipeName, frameCount);
        }
    }

    void forceOpenWipeFade() {
        if (auto* wipe = scene_wipe()) {
            wipe->force_open(cFadeWipeName);
        }
    }

    void forceCloseWipeFade() {
        if (auto* wipe = scene_wipe()) {
            wipe->force_close(cFadeWipeName);
        }
    }

    void closeWipeWhiteFade(s32 frameCount) {
        if (auto* wipe = scene_wipe()) {
            wipe->close(cWhiteFadeWipeName, frameCount);
        }
    }

    void forceOpenWipeWhiteFade() {
        if (auto* wipe = scene_wipe()) {
            wipe->force_open(cWhiteFadeWipeName);
        }
    }

    void forceCloseWipeWhiteFade() {
        if (auto* wipe = scene_wipe()) {
            wipe->force_close(cWhiteFadeWipeName);
        }
    }

    bool isWipeActive() {
        if (auto* wipe = scene_wipe()) {
            return wipe->is_active();
        }

        return false;
    }

    bool isWipeBlank() {
        if (auto* wipe = scene_wipe()) {
            return wipe->is_blank();
        }

        return false;
    }

    bool isWipeOpen() {
        if (auto* wipe = scene_wipe()) {
            return wipe->is_open();
        }

        return false;
    }

    void closeSystemWipeCircle(s32 frameCount) {
        if (auto* wipe = system_wipe()) {
            wipe->close(cCircleWipeName, frameCount);
        }
    }

    void openSystemWipeFade(s32 frameCount) {
        if (auto* wipe = system_wipe()) {
            wipe->open(cFadeWipeName, frameCount);
        }
    }

    void closeSystemWipeFade(s32 frameCount) {
        if (auto* wipe = system_wipe()) {
            wipe->close(cFadeWipeName, frameCount);
        }
    }

    void forceOpenSystemWipeFade() {
        if (auto* wipe = system_wipe()) {
            wipe->force_open(cFadeWipeName);
        }
    }

    void openSystemWipeWhiteFade(s32 frameCount) {
        if (auto* wipe = system_wipe()) {
            wipe->open(cWhiteFadeWipeName, frameCount);
        }
    }

    void closeSystemWipeWhiteFade(s32 frameCount) {
        if (auto* wipe = system_wipe()) {
            wipe->close(cWhiteFadeWipeName, frameCount);
        }
    }

    void forceCloseSystemWipeWhiteFade() {
        if (auto* wipe = system_wipe()) {
            wipe->force_close(cWhiteFadeWipeName);
        }
    }

    bool isSystemWipeActive() {
        if (auto* wipe = system_wipe()) {
            return wipe->is_active();
        }

        return false;
    }

    void startToCaptureScreen(const char* pName) {
        if (auto* director = capture_screen_director()) {
            director->requestCaptureTiming(pName);
        }
    }

    void endToCaptureScreen(const char* pName) {
        if (auto* director = capture_screen_director()) {
            director->invalidateCaptureTiming(pName);
        }
    }

    void captureScreenIfAllow(const char* pName) {
        if (auto* director = capture_screen_director()) {
            director->captureIfAllow(pName);
        }
    }

    const ResTIMG* getScreenResTIMG() {
        if (auto* director = capture_screen_director()) {
            return director->getResTIMG();
        }

        return nullptr;
    }

    u8* getScreenTexImage() {
        if (auto* director = capture_screen_director()) {
            return director->getTexImage();
        }

        return nullptr;
    }

    void closeSystemWipeCircleWithCaptureScreen(s32 frameCount) {
        startToCaptureScreen("GameScreen");
        closeSystemWipeCircle(frameCount);
    }

    void closeSystemWipeFadeWithCaptureScreen(s32 frameCount) {
        startToCaptureScreen("GameScreen");
        closeSystemWipeFade(frameCount);
    }

    void deactivateDefaultGameLayout() {
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->game_layout().deactivate_default_game_layout();
        }
    }

    void forceOffImageEffect() {
    }

    void setImageEffectControlAuto() {
    }

    void openWipeCircle(s32 frameCount) {
        if (auto* wipe = scene_wipe()) {
            wipe->open(cCircleWipeName, frameCount);
        }
    }

    void openWipeFade(s32 frameCount) {
        if (auto* wipe = scene_wipe()) {
            wipe->open(cFadeWipeName, frameCount);
        }
    }

    void openWipeWhiteFade(s32 frameCount) {
        if (auto* wipe = scene_wipe()) {
            wipe->open(cWhiteFadeWipeName, frameCount);
        }
    }
}  // namespace MR
