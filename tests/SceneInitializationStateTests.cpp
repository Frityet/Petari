#include "Game/Util/SceneUtil.hpp"
#include "scene/SceneInitializationState.hpp"

#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
    using smgpc::scene::SceneInitializationBinding;
    using smgpc::scene::SceneInitializationScope;
    using smgpc::scene::current_scene_initialization_state;

    void require(bool condition, const char *message) {
        if (!condition) throw std::runtime_error(message);
    }

    template <class F> void require_rejected(F action) {
        try {
            action();
        } catch (const std::logic_error &) {
            return;
        }
        throw std::runtime_error("An unowned or conflicting phase operation was accepted.");
    }

    void test_unowned_operations() {
        require_rejected([] { (void)MR::isInitializeStatePlacementSomething(); });
        require_rejected([] { (void)MR::isInitializeStateEnd(); });
        require_rejected([] { MR::setInitializeStatePlacementPlayer(); });
        require_rejected([] { const auto phase = SceneInitializationScope(SceneInitializeState_Placement); });
    }

    void test_original_state_predicates_and_setters() {
        auto scene = SceneInitializationBinding{};
        require(current_scene_initialization_state() == SceneInitializeState_Init,
                "Scene construction did not begin at the original Init phase.");
        constexpr auto phases = std::array{
            SceneInitializeState_NotInit, SceneInitializeState_Init,
            SceneInitializeState_PlacementPlayer, SceneInitializeState_PlacementHighPriority,
            SceneInitializeState_Placement, SceneInitializeState_AfterPlacement,
            SceneInitializeState_End};
        constexpr auto placement = std::array{false, false, true, true, true, false, false};
        for (std::size_t index = 0; index < phases.size(); ++index) {
            const auto phase = SceneInitializationScope(phases[index]);
            require(MR::isInitializeStatePlacementSomething() == placement[index],
                    "PlacementSomething disagreed with the original 2/3/4 predicate.");
            require(MR::isInitializeStateEnd() == (phases[index] == SceneInitializeState_End),
                    "End disagreed with the original state equality query.");
        }
        MR::setInitializeStatePlacementPlayer();
        require(current_scene_initialization_state() == SceneInitializeState_PlacementPlayer,
                "The original player setter did not update the scene owner.");
        MR::setInitializeStatePlacementHighPriority();
        require(current_scene_initialization_state() == SceneInitializeState_PlacementHighPriority,
                "The original high-priority setter did not update the scene owner.");
        MR::setInitializeStatePlacement();
        require(current_scene_initialization_state() == SceneInitializeState_Placement,
                "The original placement setter did not update the scene owner.");
        MR::setInitializeStateAfterPlacement();
        require(current_scene_initialization_state() == SceneInitializeState_AfterPlacement,
                "The original postpass setter did not update the scene owner.");
        scene.complete();
        require(MR::isInitializeStateEnd(), "Completion did not persist End after initialization.");
    }

    void test_nested_failure_and_scene_replacement() {
        {
            auto scene = SceneInitializationBinding{};
            require_rejected([] { const auto duplicate = SceneInitializationBinding{}; });
            require(current_scene_initialization_state() == SceneInitializeState_Init,
                    "Rejected duplicate scene changed the current owner.");
            {
                const auto player = SceneInitializationScope(SceneInitializeState_PlacementPlayer);
                require_rejected([&] { scene.complete(); });
                try {
                    const auto high_priority = SceneInitializationScope(SceneInitializeState_PlacementHighPriority);
                    MR::setInitializeStatePlacement();
                    throw std::runtime_error("original actor init failed");
                } catch (const std::runtime_error &) {
                }
                require(current_scene_initialization_state() == SceneInitializeState_PlacementPlayer,
                        "Exception unwinding did not restore the outer player phase.");
                require_rejected([] { const auto invalid = SceneInitializationScope(static_cast<SceneInitializeState>(7)); });
                require(current_scene_initialization_state() == SceneInitializeState_PlacementPlayer,
                        "Rejected invalid phase changed its parent scope.");
            }
            require(current_scene_initialization_state() == SceneInitializeState_Init && !MR::isInitializeStateEnd(),
                    "Failed placement marked the scene complete.");
            scene.complete();
            {
                const auto postpass = SceneInitializationScope(SceneInitializeState_AfterPlacement);
                require(!MR::isInitializeStateEnd(), "A scoped postpass did not override End.");
            }
            require(MR::isInitializeStateEnd(), "Scoped postpass lost persistent completed state.");
        }
        test_unowned_operations();
        {
            const auto next_scene = SceneInitializationBinding{};
            require(current_scene_initialization_state() == SceneInitializeState_Init && !MR::isInitializeStateEnd(),
                    "The next scene inherited End from a retired generation.");
        }
    }
}

int main() {
    try {
        test_unowned_operations();
        test_original_state_predicates_and_setters();
        test_nested_failure_and_scene_replacement();
        std::cout << "[ok] original scene initialization phases, nesting, failure and retirement\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "[fail] scene initialization: " << error.what() << '\n';
        return 1;
    }
}
