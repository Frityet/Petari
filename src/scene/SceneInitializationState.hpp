#pragma once

#include "Game/System/GameSystemSceneController.hpp"

namespace smgpc::scene {

    class SceneInitializationScope;

    // Native placement scopes borrow the actual process scene controller.
    // This lifetime record does not publish a second initialization state.
    class SceneInitializationBinding final {
    public:
        SceneInitializationBinding();
        ~SceneInitializationBinding();

        SceneInitializationBinding(const SceneInitializationBinding &) = delete;
        SceneInitializationBinding &operator=(const SceneInitializationBinding &) = delete;

        void complete();

    private:
        friend class SceneInitializationScope;
        friend SceneInitializeState current_scene_initialization_state();

        GameSystemSceneController *_controller = nullptr;
        SceneInitializationScope *_scope = nullptr;
    };

    // Placement operations borrow the scene's state and restore it even when
    // an original constructor or post-placement callback throws.
    class SceneInitializationScope final {
    public:
        explicit SceneInitializationScope(SceneInitializeState state);
        ~SceneInitializationScope();

        SceneInitializationScope(const SceneInitializationScope &) = delete;
        SceneInitializationScope &operator=(const SceneInitializationScope &) = delete;

    private:
        SceneInitializationBinding *_binding;
        SceneInitializationScope *_previous_scope;
        SceneInitializeState _previous_state;
    };

    [[nodiscard]] SceneInitializeState current_scene_initialization_state();

}  // namespace smgpc::scene
