#include "scene/GameSceneBinding.hpp"

#include "Game/Scene/GameScene.hpp"
#include "Game/Scene/GameScenePauseControl.hpp"
#include "Game/Scene/GameSceneScenarioOpeningCameraState.hpp"
#include "Game/Screen/GamePauseSequence.hpp"
#include "Game/System/PauseButtonCheckerInGame.hpp"
#include "Game/Util/Functor.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "scene/SceneLifetimeBinding.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include <aurora/exception.hpp>
#include <exception>
#include <stdexcept>

namespace smgpc::scene {
    namespace {
        thread_local GameSceneBinding *current_binding = nullptr;

        bool unclaimed_child(const NameObj *object, const void *) noexcept {
            return !current_scene_obj_holder_binding_owns(object) &&
                   !compat::name_obj_runtime_ownership_is_claimed(object);
        }
    }

    GameSceneBinding::GameSceneBinding(GameScene &scene)
        : _scene(&scene),
          _dispatch([](GameScene &scene, GameSceneAction action) {
              switch (action) {
              case GameSceneAction::EndScenarioStarter: scene.notifyEndScenarioStarter(); break;
              case GameSceneAction::PlayMovie: scene.requestPlayMovieDemo(); break;
              case GameSceneAction::StartGameOver: scene.requestStartGameOverDemo(); break;
              case GameSceneAction::EndGameOver: scene.requestEndGameOverDemo(); break;
              case GameSceneAction::EndMiss: scene.requestEndMissDemo(); break;
              case GameSceneAction::PowerStarGet: scene.requestPowerStarGetDemo(); break;
              case GameSceneAction::GrandStarGet: scene.requestGrandStarGetDemo(); break;
              case GameSceneAction::ShowGalaxyMap: scene.requestShowGalaxyMap(); break;
              case GameSceneAction::StaffRoll: scene.requestStaffRoll(); break;
              }
          }),
          _query([](const GameScene &scene, GameSceneQuery query) {
              switch (query) {
              case GameSceneQuery::ScenarioOpeningCamera: return scene.isExecScenarioOpeningCamera();
              case GameSceneQuery::ScenarioStarter: return scene.isExecScenarioStarter();
              case GameSceneQuery::StageClearDemo: return scene.isExecStageClearDemo();
              }
              std::terminate();
          }),
          _marker(compat::mark_name_obj_runtime_registrations()) {
        const compat::JkrHostAllocationScope host;
        if (current_binding)
            aurora::throw_host_exception<std::logic_error>("Retire the active GameScene before binding another");
        _lifetime = std::make_unique<SceneLifetimeBinding>(scene, [](void *context) noexcept {
            static_cast<GameSceneBinding *>(context)->retire();
        }, this);
        current_binding = this;
    }

    GameSceneBinding::~GameSceneBinding() {
        // The Scene must run its original derived destructor before the
        // borrowed services and children are released.
        if (_scene)
            std::terminate();
    }

    void GameSceneBinding::prepare_retirement() noexcept {
        if (_prepared || !_scene)
            return;
        _pause_control = _scene->mPauseCtrl;
        _opening_camera = _scene->mScenarioCamera;
        _pause_checker = _pause_control ? _pause_control->mPauseChecker : nullptr;
        _window_callback = _scene->mPauseSeq ? _scene->mPauseSeq->mWindowMenuFunc : nullptr;
        _prepared = true;
    }

    void GameSceneBinding::retire() noexcept {
        if (!_prepared || current_binding != this)
            std::terminate();
        const compat::JkrHostAllocationScope host;
        // The derived GameScene lifetime has ended. Only cached children and
        // registration identities may be read beyond this boundary.
        _scene = nullptr;
        current_binding = nullptr;
        delete _window_callback;
        _window_callback = nullptr;
        while (auto *child = compat::newest_name_obj_runtime_object_since_if(_marker, unclaimed_child, nullptr))
            delete child;
        delete _opening_camera;
        _opening_camera = nullptr;
        delete _pause_checker;
        _pause_checker = nullptr;
        delete _pause_control;
        _pause_control = nullptr;
        _lifetime.reset();
    }

    GameScene &GameSceneBinding::scene() const {
        if (!scene_if_active())
            aurora::throw_host_exception<std::logic_error>("The original GameScene has retired");
        return *_scene;
    }

    GameScene *GameSceneBinding::scene_if_active() const noexcept {
        return _prepared ? nullptr : _scene;
    }

    void GameSceneBinding::dispatch(GameSceneAction action) {
        _dispatch(scene(), action);
    }

    bool GameSceneBinding::query(GameSceneQuery query) const {
        return _query(scene(), query);
    }

    GameScene *current_game_scene() noexcept {
        return current_binding ? current_binding->scene_if_active() : nullptr;
    }

    void dispatch_game_scene_action(GameSceneAction action) {
        (void)require_game_scene();
        current_binding->dispatch(action);
    }

    bool query_game_scene(GameSceneQuery query) {
        (void)require_game_scene();
        return current_binding->query(query);
    }

    GameScene &require_game_scene() {
        if (auto *scene = current_game_scene())
            return *scene;
        aurora::throw_host_exception<std::logic_error>("This operation requires an active original GameScene");
    }
}
