#pragma once

#include "Game/Scene/IntermissionScene.hpp"
#include "Game/NameObj/NameObjRegister.hpp"
#include "Game/Scene/PlayTimerScene.hpp"
#include "Game/Scene/ScenarioSelectScene.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include <memory>
#include <stdexcept>

namespace smgpc::test {
// Construct the original process/scene owners without booting disc services.
// Tests publish their real SceneObjHolder and executor through this Scene.
class OriginalSceneControllerFixture final {
public:
    explicit OriginalSceneControllerFixture(const std::shared_ptr<compat::JkrHeapRuntime>& heaps)
        : domain(compat::JkrAllocationDomain::create(heaps, 1U << 20)), scene("original scene fixture") {
        if (SingletonHolder<GameSystem>::get() || SingletonHolder<NameObjRegister>::get())
            throw std::logic_error("Scene test requires exclusive original GameSystem ownership");
        compat::JkrAllocationScope game(domain);
        SingletonHolder<GameSystem>::init();
        SingletonHolder<GameSystem>::get()->mSceneController = new GameSystemSceneController();
        SingletonHolder<NameObjRegister>::init();
        SingletonHolder<NameObjRegister>::get()->setCurrentHolder(controller().mObjHolder);
        controller().mScene = &scene;
    }
    ~OriginalSceneControllerFixture() {
        auto* system = SingletonHolder<GameSystem>::release();
        if (!system) return;
        auto* owner = system->mSceneController;
        owner->mScene = nullptr;
        delete owner->mScenarioSelectScene;
        delete owner->mPlayTimerScene;
        delete owner->mIntermissionScene;
        delete SingletonHolder<NameObjRegister>::release();
        delete owner;
        system->mSceneController = nullptr;
        delete system;
        // Original constructor arrays and raw child records retire with their
        // actual Game arena, after scenes and singleton borrowers are gone.
    }
    GameSystemSceneController& controller() { return *SingletonHolder<GameSystem>::get()->mSceneController; }
    std::shared_ptr<compat::JkrAllocationDomain> domain;
    Scene scene;
};
}
