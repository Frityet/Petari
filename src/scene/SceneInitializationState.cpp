#include <aurora/exception.hpp>
#include "scene/SceneInitializationState.hpp"

#include <exception>
#include <stdexcept>

namespace smgpc::scene {
    namespace {
        SceneInitializationBinding *sCurrentBinding = nullptr;

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
        sCurrentBinding = this;
    }

    SceneInitializationBinding::~SceneInitializationBinding() {
        if (sCurrentBinding != this || _scope != nullptr) {
            std::terminate();
        }
        sCurrentBinding = nullptr;
    }

    void SceneInitializationBinding::complete() {
        if (sCurrentBinding != this || _scope != nullptr) {
            aurora::throw_host_exception<std::logic_error>(
                "Scene initialization can only complete after its placement scopes finish.");
        }
        _state = SceneInitializeState_End;
    }

    SceneInitializationScope::SceneInitializationScope(SceneInitializeState state)
        : _binding(&require_binding()), _previous_scope(_binding->_scope),
          _previous_state(_binding->_state) {
        validate_state(state);
        _binding->_state = state;
        _binding->_scope = this;
    }

    SceneInitializationScope::~SceneInitializationScope() {
        if (sCurrentBinding != _binding || _binding->_scope != this) {
            std::terminate();
        }
        _binding->_state = _previous_state;
        _binding->_scope = _previous_scope;
    }

    SceneInitializeState current_scene_initialization_state() {
        return require_binding()._state;
    }

    void set_scene_initialization_state(SceneInitializeState state) {
        auto &binding = require_binding();
        validate_state(state);
        binding._state = state;
    }
}  // namespace smgpc::scene
