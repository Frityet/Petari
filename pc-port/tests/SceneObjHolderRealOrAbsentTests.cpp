#include "Game/Map/StageSwitch.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

#include <array>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void test_no_holder_means_no_scene_object() {
        require(MR::getSceneObjHolder() == nullptr,
                "the process must not synthesize a SceneObjHolder when no scene owns one");
        require(MR::createSceneObj(SceneObj_StageSwitchContainer) == nullptr,
                "SceneObj creation must remain absent without an active scene holder");
        require(!MR::isExistSceneObj(SceneObj_StageSwitchContainer),
                "SceneObj existence must be false without an active scene holder");

        auto unbound_holder = SceneObjHolder{};
        require(unbound_holder.create(SceneObj_StageSwitchContainer) == nullptr,
                "an unbound holder must not become an alternate process-global scene");
    }

    void test_bound_holder_requires_explicit_real_creation() {
        auto holder = SceneObjHolder{};
        const auto binding = smgpc::scene::SceneObjHolderBinding(holder);

        require(MR::getSceneObjHolder() == &holder,
                "the active holder must be the holder owned by the bound scene");
        require(MR::getSceneObj<StageSwitchContainer>(SceneObj_StageSwitchContainer) == nullptr,
                "getSceneObj must not fabricate an object that the scene has not created");
        require(!MR::isExistSceneObj(SceneObj_StageSwitchContainer),
                "an uncreated supported object must report absent");

        auto *created = MR::createSceneObj(SceneObj_StageSwitchContainer);
        require(created != nullptr && MR::isExistSceneObj(SceneObj_StageSwitchContainer),
                "explicit creation must install the real supported stage-switch holder");
        require(MR::getSceneObj<StageSwitchContainer>(SceneObj_StageSwitchContainer) == created,
                "getSceneObj must return the exact scene-owned object after creation");
        require(MR::createSceneObj(SceneObj_StageSwitchContainer) == created,
                "repeated creation must return the scene's existing object");

        require(MR::createSceneObj(SceneObj_CollisionDirector) == nullptr &&
                    !MR::isExistSceneObj(SceneObj_CollisionDirector),
                "an unsupported SceneObj factory entry must remain absent");
    }

    void test_bindings_are_single_scene_and_isolated() {
        auto first_holder = SceneObjHolder{};
        {
            const auto first_binding = smgpc::scene::SceneObjHolderBinding(first_holder);
            require(MR::createSceneObj(SceneObj_StageSwitchContainer) != nullptr,
                    "the first scene should create its own supported object");

            auto second_holder = SceneObjHolder{};
            auto rejected_parallel_binding = false;
            try {
                const auto second_binding = smgpc::scene::SceneObjHolderBinding(second_holder);
                (void)second_binding;
            } catch (const std::logic_error &) {
                rejected_parallel_binding = true;
            }
            require(rejected_parallel_binding,
                    "a second holder must not silently replace the active scene's holder");
            require(MR::getSceneObjHolder() == &first_holder,
                    "a rejected binding must leave the active scene holder unchanged");
        }

        require(MR::getSceneObjHolder() == nullptr,
                "destroying the scene binding must leave no process-global holder behind");

        auto second_holder = SceneObjHolder{};
        const auto second_binding = smgpc::scene::SceneObjHolderBinding(second_holder);
        require(!MR::isExistSceneObj(SceneObj_StageSwitchContainer) &&
                    MR::getSceneObj<StageSwitchContainer>(SceneObj_StageSwitchContainer) == nullptr,
                "a later scene must not inherit objects from the previous holder");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };

}  // namespace

int main() {
    constexpr auto tests = std::array{
        TestCase{"no holder means no scene object", test_no_holder_means_no_scene_object},
        TestCase{"bound holder requires explicit real creation", test_bound_holder_requires_explicit_real_creation},
        TestCase{"bindings are single-scene and isolated", test_bindings_are_single_scene_and_isolated},
    };

    auto failures = 0;
    for (const auto &test : tests) {
        try {
            test.run();
            std::cout << "[ok] " << test.name << '\n';
        } catch (const std::exception &error) {
            ++failures;
            std::cerr << "[fail] " << test.name << ": " << error.what() << '\n';
        }
    }

    if (failures != 0) {
        std::cerr << failures << " SceneObjHolder real-or-absent test(s) failed\n";
        return 1;
    }

    std::cout << tests.size() << " SceneObjHolder real-or-absent test(s) passed\n";
    return 0;
}
