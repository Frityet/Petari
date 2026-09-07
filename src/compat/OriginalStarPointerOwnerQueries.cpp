#include "Game/Screen/StarPointerDirector.hpp"
#include "Game/Screen/StarPointerController.hpp"
#include "Game/Screen/StarPointerLayout.hpp"
#include "Game/System/StarPointerOnOffController.hpp"
#include "Game/Util/StarPointerUtil.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "compat/StarPointerDepthOwnership.hpp"
#include <aurora/exception.hpp>
#include <stdexcept>

namespace {
StarPointerDirector* getStarPointerDirector() { return StarPointerFunction::getStarPointerDirector(); }
StarPointerLayout* getStarPointerLayout(s32 channel) {
    auto* director = getStarPointerDirector();
    if (!director->mStarPointerLayouts)
        aurora::throw_host_exception<std::logic_error>("Pointer layout query requires actual initialized resources.");
    return director->getStarPointerLayout(channel);
}
StarPointerController* getStarPointerController(s32 channel) { return getStarPointerDirector()->getStarPointerController(channel); }
StarPointerOnOffController* getStarPointerOnOffController() { return &smgpc::compat::require_star_pointer_depth().modes(); }
} // namespace

namespace MR {
    void initStarPointerGameScene() {
        StarPointerFunction::getStarPointerDirector()->init();
    }

    void setStarPointerModeBase() {
        ::getStarPointerOnOffController()->setStateToBase(nullptr);
    }

    void setStarPointerCameraMtxAtGameScene() {
        ::getStarPointerDirector()->setGameSceneCameraMtx();
    }

    bool isStarPointerValid(s32 channel) {
        return ::getStarPointerLayout(channel)->mIsPointerValid && MR::isConnectedWPad(channel);
    }

    bool requestPointerGuidanceNoInformation() {
        return ::getStarPointerDirector()->mGuidance->request1PGuidance(nullptr, true);
    }

    bool isExistStarPointerGuidance() {
        return ::getStarPointerDirector()->mGuidance->isExistGuidanceOrFrame();
    }

    bool isExistStarPointerGuidanceFrame1P() {
        return ::getStarPointerDirector()->mGuidance->isExistFrame1P();
    }

    void activeStarPointerGuidance() {
        if (::getStarPointerDirector()->mGuidance != nullptr) {
            ::getStarPointerDirector()->mGuidance->active();
        }
    }

    void deactiveStarPointerGuidance() {
        if (::getStarPointerDirector()->mGuidance != nullptr) {
            ::getStarPointerDirector()->mGuidance->deactive();
        }
    }

    void tryShowTimeoutedStarPointerGuidance() {
        if (::getStarPointerDirector()->mGuidance != nullptr) {
            ::getStarPointerDirector()->mGuidance->tryResetTimeout();
        }
    }

    f32 getStarPointerRadius(s32 channel) {
        if (::getStarPointerLayout(channel) == nullptr) {
            return 10.0f;
        }
        return ::getStarPointerLayout(channel)->mRadius;
    }

    TVec2f* getStarPointerScreenPosition(s32 channel) {
        return &::getStarPointerController(channel)->mPastInfo.mPos;
    }

    TVec2f getStarPointerScreenPositionOrEdge(s32 channel) {
        TVec2f pos(::getStarPointerController(channel)->mPastInfo.mPos);
        StarPointerFunction::forceInsideScreenEdge(&pos);
        return pos;
    }

    f32 getStarPointerScreenSpeed(s32 channel) {
        return ::getStarPointerController(channel)->mScreenSpeed;
    }

    bool isStarPointerInScreen(s32 channel) {
        return ::getStarPointerController(channel)->isInScreen();
    }

    bool isStarPointerInScreenAnyPort(s32* pInScreenChannel) {
        for (s32 channel = 0; channel < StarPointerFunction::getNumStarPointer(); channel++) {
            if (isStarPointerInScreen(channel)) {
                if (pInScreenChannel != nullptr) {
                    *pInScreenChannel = channel;
                }
                return true;
            }
        }
        if (pInScreenChannel != nullptr) {
            *pInScreenChannel = -1;
        }
        return false;
    }

    TVec2f* getStarPointerScreenVelocity(s32 channel) {
        return &::getStarPointerController(channel)->mScreenVel;
    }

    void getStarPointerWorldVelocityDirection(TVec3f* pVel, s32 channel) {
        pVel->set(::getStarPointerController(channel)->mWorldVel);
    }

    bool tryStartStarPointerCommandStream(const LiveActor* pActor, const TVec3f* pPos, s32 channel, bool b) {
        return ::getStarPointerLayout(channel)->startCommandStream(pActor, pPos, b);
    }

    bool tryEndStarPointerCommandStream(const LiveActor* pActor, s32 channel) {
        StarPointerLayout* layout = ::getStarPointerLayout(channel);
        if (layout->isCommandStream(pActor)) {
            layout->endCommandStream(pActor);
            return true;
        }
        return false;
    }

    bool isStarPointerCommandStream(const LiveActor* pActor, s32 channel) {
        return ::getStarPointerLayout(channel)->isCommandStream(pActor);
    }

    bool requestBigBubbleGuidance() {
        return ::getStarPointerDirector()->mGuidance->request1PGuidance("PointerGuidance_BigBubble", true);
    }

    bool requestMarioLauncherGuidance() {
        return ::getStarPointerDirector()->mGuidance->request1PGuidance("PointerGuidance_MarioLauncher", true);
    }

    bool requestFileSelectGuidance() {
        return ::getStarPointerDirector()->mGuidance->request1PGuidance("System_FileSelect008", true);
    }

    bool requestFileSelectCopyGuidance() {
        return ::getStarPointerDirector()->mGuidance->request1PGuidance("System_FileSelect002", false);
    }

    bool requestStarPieceLectureGuidance() {
        return ::getStarPointerDirector()->mGuidance->request1PGuidance("PointerGuidance_StarPieceLecture", false);
    }

    void startStarPointerModeTitle(void* pRequester) {
        ::getStarPointerOnOffController()->setStateToTitle(pRequester);
    }

    void startStarPointerModeFileSelect(void* pRequester) {
        ::getStarPointerOnOffController()->setStateToFileSelect(pRequester);
    }

    void startStarPointerModeGame(void* pRequester) {
        ::getStarPointerOnOffController()->incModeCounter(pRequester, StarPointerMode_Game);
    }

    void startStarPointerModeDemo(void* pRequester) {
        ::getStarPointerOnOffController()->incModeCounter(pRequester, StarPointerMode_Demo);
    }

    void startStarPointerModeDemoWithStarPointer(void* pRequester) {
        ::getStarPointerOnOffController()->incModeCounter(pRequester, StarPointerMode_DemoWithStarPointer);
    }

    void startStarPointerModeDemoWithHandPointerFinger(void* pRequester) {
        ::getStarPointerOnOffController()->incModeCounter(pRequester, StarPointerMode_DemoWithHandPointerFinger);
    }

    void startStarPointerModeDemoMarioDeath(void* pRequester) {
        ::getStarPointerOnOffController()->incModeCounter(pRequester, StarPointerMode_DemoMarioDeath);
    }

    void startStarPointerModeMarioLauncher(void* pRequester) {
        ::getStarPointerOnOffController()->incModeCounter(pRequester, StarPointerMode_MarioLauncher);
    }

    void startStarPointerModeHomeButton(void* pRequester) {
        ::getStarPointerOnOffController()->incModeCounter(pRequester, StarPointerMode_HomeButton);
    }

    void startStarPointerModeChooseYesNo(void* pRequester) {
        ::getStarPointerOnOffController()->incModeCounter(pRequester, StarPointerMode_ChooseYesNo);
    }

    void startStarPointerModePauseMenu(void* pRequester) {
        ::getStarPointerOnOffController()->incModeCounter(pRequester, StarPointerMode_PauseMenu);
    }

    void startStarPointerModeScenarioSelectScene(void* pRequester) {
        ::getStarPointerOnOffController()->incModeCounter(pRequester, StarPointerMode_ScenarioSelectScene);
    }

    void startStarPointerModeBlueStar(void* pRequester) {
        ::getStarPointerOnOffController()->incModeCounter(pRequester, StarPointerMode_BlueStar);
    }

    void startStarPointerModePowerStarGetDemo(void* pRequester) {
        ::getStarPointerOnOffController()->incModeCounter(pRequester, StarPointerMode_PowerStarGetDemo);
    }

    void startStarPointerModeStarPieceTarget(void* pRequester) {
        ::getStarPointerOnOffController()->incModeCounter(pRequester, StarPointerMode_StarPieceTarget);
    }

    void startStarPointerModeSphereSelectorFinger(void* pRequester) {
        ::getStarPointerOnOffController()->incModeCounter(pRequester, StarPointerMode_SphereSelectorFinger);
    }

    void startStarPointerModeSphereSelectorOnReaction(void* pRequester) {
        ::getStarPointerOnOffController()->incModeCounter(pRequester, StarPointerMode_SphereSelectorOnReaction);
    }

    void startStarPointerModeEnding(void* pRequester) {
        ::getStarPointerOnOffController()->incModeCounter(pRequester, StarPointerMode_Base);
    }

    void startStarPointerModeCommandStream(void* pRequester) {
        ::getStarPointerOnOffController()->incModeCounter(pRequester, StarPointerMode_CommandStream);
    }

    void startStarPointerMode1PInvalid2PValid(void* pRequester) {
        ::getStarPointerOnOffController()->incModeCounter(pRequester, StarPointerMode_1PInvalid2PValid);
    }

    void requestStarPointerModeErrorWindow(void* pRequester) {
        ::getStarPointerOnOffController()->requestMode(pRequester, StarPointerMode_ErrorWindow);
    }

    void requestStarPointerModeSaveLoad(void* pRequester) {
        ::getStarPointerOnOffController()->requestMode(pRequester, StarPointerMode_SaveLoad);
    }

    void requestStarPointerModePictureBook(void* pRequester) {
        ::getStarPointerOnOffController()->requestMode(pRequester, StarPointerMode_PictureBook);
    }

    void requestStarPointerModePauseMenu(void* pRequester) {
        ::getStarPointerOnOffController()->requestMode(pRequester, StarPointerMode_PauseMenu);
    }

    void requestStarPointerModeBlueStarReady(void* pRequester) {
        ::getStarPointerOnOffController()->requestMode(pRequester, StarPointerMode_BlueStarReady);
    }

    void requestStarPointerModeBigBubble(void* pRequester, const TVec3f& rPosition) {
        ::getStarPointerOnOffController()->requestMode(pRequester, StarPointerMode_BigBubble);
        ::getStarPointerDirector()->mNozzleAimPos.set(rPosition);
    }

    bool isStarPointerModeBlueStarReady() {
        return ::getStarPointerOnOffController()->compareMode(StarPointerMode_BlueStarReady);
    }

    bool isStarPointerModeStarPieceTarget() {
        return ::getStarPointerOnOffController()->compareMode(StarPointerMode_StarPieceTarget);
    }

    bool isStarPointerModeMarioLauncher() {
        return ::getStarPointerOnOffController()->isMode(StarPointerMode_MarioLauncher);
    }

    bool isStarPointerModeHomeButton() {
        return ::getStarPointerOnOffController()->isMode(StarPointerMode_HomeButton);
    }

    bool isStarPointerModeErrorWindow() {
        return ::getStarPointerOnOffController()->isMode(StarPointerMode_ErrorWindow);
    }

    void endStarPointerMode(void* pRequester) {
        ::getStarPointerOnOffController()->popState(pRequester);
    }

    void enableStarPointerShootStarPiece() {
        StarPointerDirector* director = ::getStarPointerDirector();
        director->mIsAllowP1StarPieceShot = true;
        director->mIsAllowP2StarPieceShot = true;
    }

    void disableStarPointerShootStarPiece() {
        StarPointerDirector* director = ::getStarPointerDirector();
        director->mIsAllowP1StarPieceShot = false;
        director->mIsAllowP2StarPieceShot = false;
    }

    bool isEnableStarPointerShootStarPiece(s32 channel) {
        StarPointerDirector* director = ::getStarPointerDirector();
        if (channel == WPAD_CHAN0) {
            return director->mIsAllowP1StarPieceShot;
        } else {
            return director->isEnableStarPointerShootStarPiece();
        }
    }

    bool isStarPointer2PTransparencyMode() {
        return ::getStarPointerOnOffController()->compareMode(StarPointerMode_FileSelect) ||
               ::getStarPointerOnOffController()->compareMode(StarPointerMode_ScenarioSelectScene) ||
               ::getStarPointerOnOffController()->compareMode(StarPointerMode_PauseMenu) ||
               ::getStarPointerOnOffController()->compareMode(StarPointerMode_ChooseYesNo) ||
               ::getStarPointerOnOffController()->compareMode(StarPointerMode_PictureBook) ||
               ::getStarPointerOnOffController()->compareMode(StarPointerMode_SphereSelectorOnReaction) ||
               ::getStarPointerOnOffController()->compareMode(StarPointerMode_SphereSelectorFinger) ||
               ::getStarPointerOnOffController()->compareMode(StarPointerMode_SaveLoad) ||
               ::getStarPointerOnOffController()->compareMode(StarPointerMode_StarPieceTarget) ||
               ::getStarPointerOnOffController()->compareMode(StarPointerMode_MarioLauncher);
    }

    bool isStarPointer1PInvalid2PValidMode() {
        return ::getStarPointerOnOffController()->mMode == StarPointerMode_1PInvalid2PValid;
    }

    void setStarPointerDrawSyncToken() {
        StarPointerDirector* director = ::getStarPointerDirector();
        if (director->getStarPointerLayout(WPAD_CHAN0)->mIsPointerValid || director->getStarPointerLayout(WPAD_CHAN1)->mIsPointerValid) {
            ::getStarPointerDirector()->mPeekZ->setDrawSyncToken();
        }
    }

} // namespace MR
