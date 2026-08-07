#include "Game/Util/SequenceUtil.hpp"
#include "Game/Scene/GameSceneFunction.hpp"
#include "Game/System/GalaxyMoveArgument.hpp"
#include "Game/System/GameSequenceFunction.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/JMapIdInfo.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "Game/Util/SystemUtil.hpp"

namespace MR {
    void requestChangeScene(const char* pSceneName) {
        GameSequenceFunction::requestChangeScene(pSceneName);
    }

    void requestChangeSceneTitle() {
        GameSequenceFunction::requestGalaxyMove(GalaxyMoveArgument(7, nullptr, 1, nullptr));
    }

    void requestChangeStageInGameAfterLoadingGameData() {
        GameSequenceFunction::requestGalaxyMove(GalaxyMoveArgument(6, nullptr, 1, nullptr));
    }

    void requestChangeStageAfterStageClear() {
        GameSequenceFunction::requestGalaxyMove(GalaxyMoveArgument(4, nullptr, 1, nullptr));
    }

    void requestChangeStageAfterMiss() {
        JMapIdInfo restartIdInfo = *getPlayerRestartIdInfo();

        if (isGalaxyAnyCometAppearInCurrentStage()) {
            const JMapIdInfo& rInitializeStartIdInfo = getInitializeStartIdInfo();
            restartIdInfo._0 = rInitializeStartIdInfo._0;
            restartIdInfo.mZoneID = rInitializeStartIdInfo.mZoneID;
        }

        GameSystemSceneController* pSceneController = SingletonHolder< GameSystem >::get()->mSceneController;
        GalaxyMoveArgument moveArgument(5, pSceneController->mCurrSceneControlInfo.mStage, pSceneController->getCurrentScenarioNo(), &restartIdInfo);
        moveArgument._C = pSceneController->getCurrentSelectedScenarioNo();
        GameSequenceFunction::requestGalaxyMove(moveArgument);
    }

    void requestChangeStageInGameMoving(const char* pStageName, s32 scenarioNo, const JMapIdInfo& rIdInfo) {
        GameSequenceFunction::requestGalaxyMove(GalaxyMoveArgument(0, pStageName, scenarioNo, &rIdInfo));
    }

    void requestChangeStageInGameMoving(const char* pStageName, s32 scenarioNo) {
        GameSequenceFunction::requestGalaxyMove(GalaxyMoveArgument(0, pStageName, scenarioNo, &getInitializeStartIdInfo()));
    }

    void requestChangeSceneAfterGameOver() {
        resetSystemAndGameStatus();
        GameSequenceFunction::requestGalaxyMove(GalaxyMoveArgument(7, nullptr, 1, nullptr));
    }

    void requestChangeSceneAfterBoot() {
        GameSequenceFunction::requestGalaxyMove(GalaxyMoveArgument(7, nullptr, 1, nullptr));
    }

    void requestChangeStageGoBackAstroDome() {
        GameSequenceFunction::requestGalaxyMove(GalaxyMoveArgument(3, nullptr, 1, nullptr));
    }

    void requestStartScenarioSelect(const char* pStageName) {
        GameSequenceFunction::requestGalaxyMove(GalaxyMoveArgument(2, pStageName, -1, nullptr));
    }

    void requestStartScenarioSelectForComet(const char* pStageName, s32 scenarioNo) {
        GameSequenceFunction::requestGalaxyMove(GalaxyMoveArgument(2, pStageName, scenarioNo, nullptr));
    }

    bool hasRetryGalaxySequence() {
        return GameSequenceFunction::hasRetryGalaxySequence();
    }

    bool isExecScenarioStarter() {
        return GameSceneFunction::isExecScenarioStarter();
    }

    void requestPowerStarGetDemo() {
        GameSceneFunction::requestPowerStarGetDemo();
    }

    void requestGrandStarGetDemo() {
        GameSceneFunction::requestGrandStarGetDemo();
    }

    void requestStartGameOverDemo() {
        GameSceneFunction::requestStartGameOverDemo();
    }

    void requestEndGameOverDemo() {
        GameSceneFunction::requestEndGameOverDemo();
    }

    void requestEndMissDemo() {
        GameSceneFunction::requestEndMissDemo();
    }

    void requestShowGalaxyMap() {
        GameSceneFunction::requestShowGalaxyMap();
    }

    void executeOnWelcomeAndRetry() {
        startGalaxyCometEvent();
        requestForceAppearHPMeter();
    }

    void requestGoToAstroGalaxy(s32 marioStartId) {
        JMapIdInfo idInfo(marioStartId, 0);
        GameSequenceFunction::requestGalaxyMove(GalaxyMoveArgument(0, "AstroGalaxy", -1, &idInfo));
    }

    void requestGoToAstroDomeFromAstroGalaxy(s32 scenarioNo, s32 marioStartId) {
        JMapIdInfo idInfo(marioStartId, 0);
        GameSequenceFunction::requestGalaxyMove(GalaxyMoveArgument(0, "AstroDome", scenarioNo, &idInfo));
    }
}  // namespace MR
