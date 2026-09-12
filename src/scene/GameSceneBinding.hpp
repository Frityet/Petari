#pragma once

#include "compat/ActorRuntimeRegistry.hpp"
#include <memory>

class GameScene;
class GameScenePauseControl;
class GameSceneScenarioOpeningCameraState;
class PauseButtonCheckerInGame;
namespace MR { class FunctorBase; }

namespace smgpc::scene {
    class SceneLifetimeBinding;
    // Borrows an actual original GameScene. Native services never substitute
    // another Scene subclass for the original nerve and sequence fields.
    class GameSceneBinding final {
    public:
        explicit GameSceneBinding(GameScene &scene);
        ~GameSceneBinding();
        GameSceneBinding(const GameSceneBinding &) = delete;
        GameSceneBinding &operator=(const GameSceneBinding &) = delete;

        // Snapshot raw children while the derived object is still alive.
        // Their destructors run at the Scene base retirement boundary.
        void prepare_retirement() noexcept;

    private:
        friend void prepare_game_scene_retirement(GameScene&) noexcept;
        void retire() noexcept;
        GameScene *_scene;
        std::unique_ptr<SceneLifetimeBinding> _lifetime;
        compat::NameObjRuntimeRegistrationMarker _marker;
        GameScenePauseControl *_pause_control = nullptr;
        GameSceneScenarioOpeningCameraState *_opening_camera = nullptr;
        PauseButtonCheckerInGame *_pause_checker = nullptr;
        MR::FunctorBase *_window_callback = nullptr;
        bool _prepared = false;
    };

    void prepare_game_scene_retirement(GameScene&) noexcept;
}
