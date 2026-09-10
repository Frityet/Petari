#include "Game/Scene/GameSceneFunction.hpp"
#include "Game/Scene/GameScene.hpp"
#include "scene/GameSceneBinding.hpp"
#include "Game/Util/SequenceUtil.hpp"

namespace {
    GameScene* getGameScene() NO_INLINE {
        return &smgpc::scene::require_game_scene();
    }
};  // namespace

namespace GameSceneFunction {
    void notifyEndScenarioStarter() {
        smgpc::scene::dispatch_game_scene_action(smgpc::scene::GameSceneAction::EndScenarioStarter);
    }

    void requestPlayMovieDemo() {
        smgpc::scene::dispatch_game_scene_action(smgpc::scene::GameSceneAction::PlayMovie);
    }

    void requestStartGameOverDemo() {
        smgpc::scene::dispatch_game_scene_action(smgpc::scene::GameSceneAction::StartGameOver);
    }

    void requestEndGameOverDemo() {
        smgpc::scene::dispatch_game_scene_action(smgpc::scene::GameSceneAction::EndGameOver);
    }

    void requestEndMissDemo() {
        smgpc::scene::dispatch_game_scene_action(smgpc::scene::GameSceneAction::EndMiss);
    }

    void requestPowerStarGetDemo() {
        smgpc::scene::dispatch_game_scene_action(smgpc::scene::GameSceneAction::PowerStarGet);
    }

    void requestGrandStarGetDemo() {
        smgpc::scene::dispatch_game_scene_action(smgpc::scene::GameSceneAction::GrandStarGet);
    }

    void requestShowGalaxyMap() {
        smgpc::scene::dispatch_game_scene_action(smgpc::scene::GameSceneAction::ShowGalaxyMap);
    }

    void requestStaffRoll() {
        smgpc::scene::dispatch_game_scene_action(smgpc::scene::GameSceneAction::StaffRoll);
    }

    bool isExecScenarioOpeningCamera() {
        return smgpc::scene::query_game_scene(smgpc::scene::GameSceneQuery::ScenarioOpeningCamera);
    }

    bool isExecScenarioStarter() {
        return smgpc::scene::query_game_scene(smgpc::scene::GameSceneQuery::ScenarioStarter);
    }

    bool isExecStageClearDemo() {
        return smgpc::scene::query_game_scene(smgpc::scene::GameSceneQuery::StageClearDemo);
    }

    void activateDraw3D() {
        GameScene* pGameScene = ::getGameScene();

        pGameScene->mDraw3D = true;
    }

    void deactivateDraw3D() {
        GameScene* pGameScene = ::getGameScene();

        pGameScene->mDraw3D = false;
    }
};  // namespace GameSceneFunction

namespace MR {
    void requestStartGameOverDemo() { GameSceneFunction::requestStartGameOverDemo(); }
    void requestEndGameOverDemo() { GameSceneFunction::requestEndGameOverDemo(); }
    void requestEndMissDemo() { GameSceneFunction::requestEndMissDemo(); }
}
