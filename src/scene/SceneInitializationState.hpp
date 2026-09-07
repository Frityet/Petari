#pragma once

#include "Game/System/GameSystemSceneController.hpp"

namespace smgpc::scene {

    class SceneInitializationScope;

    // One native scene owns the same state that the retail scene controller
    // retains through construction and normal execution. This is independent
    // of the host controller's asynchronous scene-transition state machine.
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
        friend void set_scene_initialization_state(SceneInitializeState state);

        SceneInitializeState _state = SceneInitializeState_Init;
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
    void set_scene_initialization_state(SceneInitializeState state);

}  // namespace smgpc::scene
