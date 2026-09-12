#include "Game/Scene/GameScenePauseControl.hpp"
#if defined(TARGET_PC)
#include "compat/DisabledObjectAudio.hpp"
#endif
#include "Game/AudioLib/AudSystem.hpp"
#include "Game/AudioLib/AudWrap.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/Screen/GamePauseSequence.hpp"
#include "Game/System/PauseButtonCheckerInGame.hpp"

namespace {
    NEW_NERVE(GameScenePauseControlNormal, GameScenePauseControl, Normal);
};  // namespace

GameScenePauseControl::GameScenePauseControl(GameScene* pScene) : NerveExecutor("GameSceneポーズ制御") {
    mScene = pScene;
    mPauseChecker = nullptr;
    mPauseMenuOff = false;
    mPauseMenuNerve = nullptr;
    initNerve(&GameScenePauseControlNormal::sInstance);
    mPauseChecker = new PauseButtonCheckerInGame();
}

void GameScenePauseControl::registerNervePauseMenu(const Nerve* pNerve) {
    mPauseMenuNerve = pNerve;
}

void GameScenePauseControl::requestPauseMenuOff() {
    mPauseMenuOff = true;
}

void GameScenePauseControl::exeNormal() {
    tryStartPauseMenu();

    if (mPauseMenuOff) {
#if defined(TARGET_PC)
        if constexpr (aurora::audio::DisabledObjectAudio::enabled()) {
            AudWrap::getSystem()->exitPauseMenu();
        }
#else
        AudWrap::getSystem()->exitPauseMenu();
#endif
        mScene->setNerveAfterPauseMenu();
        mPauseMenuOff = false;
        mScene->mPauseSeq->deactivate();
    }
}

bool GameScenePauseControl::tryStartPauseMenu() {
    if (mScene->isPermitToPauseMenu()) {
        mPauseChecker->update();

        if (mPauseChecker->isPermitToMinusPause()) {
            mScene->mPauseSeq->startPause(GamePauseSequence::ActivePause);
            mScene->setNerve(mPauseMenuNerve);
            return true;
        }

        if (mPauseChecker->isPermitToPlusPause()) {
            mScene->mPauseSeq->startPause(GamePauseSequence::ActivePause);
            mScene->setNerve(mPauseMenuNerve);
            return true;
        }
    }

    return false;
}

GameScenePauseControl::~GameScenePauseControl() {
}
