#include <aurora/exception.hpp>
#include "scene/SceneInitializationState.hpp"

#include "Game/System/GameSystem.hpp"
#include "Game/Util/SingletonHolder.hpp"

#include <exception>
#include <stdexcept>

namespace smgpc::scene {
    namespace {
        SceneInitializationBinding *sCurrentBinding = nullptr;

        GameSystemSceneController &require_controller() {
            auto *system = SingletonHolder<GameSystem>::get();
            if (system == nullptr || system->mSceneController == nullptr) {
                aurora::throw_host_exception<std::logic_error>(
                    "Scene initialization requires the original GameSystem scene controller.");
            }
            return *system->mSceneController;
        }

        SceneInitializationBinding &require_binding() {
            if (sCurrentBinding == nullptr) {
                aurora::throw_host_exception<std::logic_error>(
                    "Scene initialization state requires an active scene owner.");
            }
            return *sCurrentBinding;
        }

        void validate_state(SceneInitializeState state) {
            if (state < SceneInitializeState_NotInit || state > SceneInitializeState_End) {
                aurora::throw_host_exception<std::invalid_argument>("Unknown retail scene initialization state.");
            }
        }
    }  // namespace

    SceneInitializationBinding::SceneInitializationBinding() {
        if (sCurrentBinding != nullptr) {
            aurora::throw_host_exception<std::logic_error>("A scene initialization owner is already bound.");
        }
        _controller = &require_controller();
        sCurrentBinding = this;
    }

    SceneInitializationBinding::~SceneInitializationBinding() {
        if (sCurrentBinding != this || _scope != nullptr) {
            std::terminate();
        }
        sCurrentBinding = nullptr;
    }

    void SceneInitializationBinding::complete() {
        if (sCurrentBinding != this || _scope != nullptr || _controller != &require_controller()) {
            aurora::throw_host_exception<std::logic_error>(
                "Scene initialization can only complete after its placement scopes finish.");
        }
        _controller->setSceneInitializeState(SceneInitializeState_End);
    }

    SceneInitializationScope::SceneInitializationScope(SceneInitializeState state)
        : _binding(&require_binding()), _previous_scope(_binding->_scope),
          _previous_state(SceneInitializeState_NotInit) {
        validate_state(state);
        if (_binding->_controller != &require_controller()) {
            aurora::throw_host_exception<std::logic_error>("The scene controller changed during its placement lifetime.");
        }
        _previous_state = _binding->_controller->mSceneInitializeState;
        _binding->_controller->setSceneInitializeState(state);
        _binding->_scope = this;
    }

    SceneInitializationScope::~SceneInitializationScope() {
        if (sCurrentBinding != _binding || _binding->_scope != this) {
            std::terminate();
        }
        _binding->_controller->setSceneInitializeState(_previous_state);
        _binding->_scope = _previous_scope;
    }

    SceneInitializeState current_scene_initialization_state() {
        return require_controller().mSceneInitializeState;
    }
}  // namespace smgpc::scene
