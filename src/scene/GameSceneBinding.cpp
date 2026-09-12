#include "scene/GameSceneBinding.hpp"

#include "Game/Scene/GameScene.hpp"
#include "Game/Scene/GameScenePauseControl.hpp"
#include "Game/Scene/GameSceneScenarioOpeningCameraState.hpp"
#include "Game/Screen/GamePauseSequence.hpp"
#include "Game/System/PauseButtonCheckerInGame.hpp"
#include "Game/Util/Functor.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "scene/SceneLifetimeBinding.hpp"
#include "scene/OriginalSceneSupport.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include <aurora/exception.hpp>
#include <exception>
#include <stdexcept>

namespace smgpc::scene {
    namespace {
        GameSceneBinding *current_binding = nullptr;

        bool unclaimed_child(const NameObj *object, const void *) noexcept {
            return !current_scene_obj_holder_binding_owns(object) &&
                   !compat::name_obj_runtime_ownership_is_claimed(object);
        }
    }

    GameSceneBinding::GameSceneBinding(GameScene &scene)
        : _scene(&scene),
          _marker(compat::mark_name_obj_runtime_registrations()) {
        const compat::JkrHostAllocationScope host;
        if (current_binding)
            aurora::throw_host_exception<std::logic_error>("Retire the active GameScene before binding another");
        _lifetime = std::make_unique<SceneLifetimeBinding>(scene, [](void *context) noexcept {
            static_cast<GameSceneBinding *>(context)->retire();
        }, this);
        current_binding = this;
    }

    void prepare_game_scene_retirement(GameScene& scene) noexcept {
        prepare_original_scene_support_retirement(scene);
        if (current_binding && current_binding->_scene == &scene)
            current_binding->prepare_retirement();
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

}
