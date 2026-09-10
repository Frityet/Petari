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
    enum class GameSceneAction {
        EndScenarioStarter, PlayMovie, StartGameOver, EndGameOver, EndMiss,
        PowerStarGet, GrandStarGet, ShowGalaxyMap, StaffRoll
    };
    enum class GameSceneQuery { ScenarioOpeningCamera, ScenarioStarter, StageClearDemo };

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
        [[nodiscard]] GameScene &scene() const;
        [[nodiscard]] GameScene *scene_if_active() const noexcept;
        void dispatch(GameSceneAction action);
        [[nodiscard]] bool query(GameSceneQuery query) const;

    private:
        void retire() noexcept;
        GameScene *_scene;
        // The actual owner installs these delegates. API users can link and
        // validate a missing owner without pulling in its construction graph.
        void (*_dispatch)(GameScene &, GameSceneAction);
        bool (*_query)(const GameScene &, GameSceneQuery);
        std::unique_ptr<SceneLifetimeBinding> _lifetime;
        compat::NameObjRuntimeRegistrationMarker _marker;
        GameScenePauseControl *_pause_control = nullptr;
        GameSceneScenarioOpeningCameraState *_opening_camera = nullptr;
        PauseButtonCheckerInGame *_pause_checker = nullptr;
        MR::FunctorBase *_window_callback = nullptr;
        bool _prepared = false;
    };

    [[nodiscard]] GameScene *current_game_scene() noexcept;
    [[nodiscard]] GameScene &require_game_scene();
    void dispatch_game_scene_action(GameSceneAction action);
    [[nodiscard]] bool query_game_scene(GameSceneQuery query);
}
