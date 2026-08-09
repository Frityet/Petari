#include "Game/Gravity/PlanetGravityManager.hpp"
#include "Game/Map/StageSwitch.hpp"
#include "Game/Map/SwitchWatcher.hpp"
#include "Game/Map/SwitchWatcherHolder.hpp"
#include "Game/Player/MarioHolder.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/nameobj/NameObjFactory.hpp"

#include <array>
#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    [[nodiscard]] std::string read_file(std::string_view path) {
        auto stream = std::ifstream(std::string(path), std::ios::binary);
        if (!stream.is_open()) {
            throw std::runtime_error("could not open source-order file: " + std::string(path));
        }
        return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
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

        auto *gravity_manager = MR::createSceneObj(SceneObj_PlanetGravityManager);
        require(gravity_manager != nullptr &&
                    MR::getSceneObj<PlanetGravityManager>(
                        SceneObj_PlanetGravityManager) == gravity_manager &&
                    std::string_view(gravity_manager->getName()) == "重力",
                "SceneObj 0x32 must be the exact scene-owned PlanetGravityManager");
        require(MR::createSceneObj(SceneObj_PlanetGravityManager) == gravity_manager,
                "repeated gravity-manager creation must return the scene singleton");

        require(MR::createSceneObj(SceneObj_CollisionDirector) == nullptr &&
                    !MR::isExistSceneObj(SceneObj_CollisionDirector),
                "an unsupported SceneObj factory entry must remain absent");
        require(MR::createSceneObj(SceneObj_MiiFacePartsHolder) == nullptr &&
                    !MR::isExistSceneObj(SceneObj_MiiFacePartsHolder),
                "the Mii holder must remain absent until real character-model construction and drawing exist");
    }

    void test_mario_holder_precedes_real_actor_creation() {
        auto holder = SceneObjHolder{};
        const auto binding = smgpc::scene::SceneObjHolderBinding(holder);

        require(!MR::isExistSceneObj(SceneObj_MarioHolder) &&
                    holder.getObj(SceneObj_MarioHolder) == nullptr,
                "MarioHolder must remain absent until the scene explicitly creates it");

        auto *created = MR::createSceneObj(SceneObj_MarioHolder);
        auto *mario_holder = dynamic_cast<MarioHolder *>(created);
        require(mario_holder != nullptr && MR::isExistSceneObj(SceneObj_MarioHolder),
                "SceneObj 0x14 must construct the exact MarioHolder");
        require(MR::getMarioHolder() == mario_holder &&
                    holder.getObj(SceneObj_MarioHolder) == mario_holder,
                "the retail MarioHolder accessor must return the active scene-owned object");
        require(std::string_view(mario_holder->getName()) == "マリオ保持" &&
                    mario_holder->getMarioActor() == nullptr,
                "MarioHolder must exist with its exact name and a null actor before real MarioActor init2");
        require(MR::createSceneObj(SceneObj_MarioHolder) == mario_holder &&
                    mario_holder->getMarioActor() == nullptr,
                "repeated creation must preserve the same empty holder until the real actor registers itself");
    }

    void test_stage_host_creates_mario_holder_before_start_preflight() {
        const auto source = read_file("src/scene/StageHostScene.cpp");
        const auto required_objects = source.find("constexpr auto required_scene_objects");
        const auto mario_holder = source.find("SceneObj_MarioHolder", required_objects);
        const auto create_loop = source.find("for (const auto id : required_scene_objects)", mario_holder);
        const auto placement_init_call = source.find("init_placement_roots();", create_loop);
        const auto placement_init = source.find("void StageHostScene::init_placement_roots()", placement_init_call);
        const auto start_preflight = source.find("preflight_stage_start_or_throw();", placement_init);
        const auto start_construction = source.find("construct_stage_start_root();", start_preflight);

        require(required_objects != std::string::npos && mario_holder != std::string::npos &&
                    create_loop != std::string::npos && placement_init_call != std::string::npos &&
                    placement_init != std::string::npos && start_preflight != std::string::npos &&
                    start_construction != std::string::npos && required_objects < mario_holder &&
                    mario_holder < create_loop && create_loop < placement_init_call &&
                    placement_init_call < placement_init && placement_init < start_preflight &&
                    start_preflight < start_construction,
                "StageHost must create SceneObj_MarioHolder before StartInfo preflight and construction");
    }

    void test_holder_does_not_install_a_player_creator() {
        const auto mario = smgpc::scene::nameobj::describe_name_obj_creator_support("Mario");
        const auto mario_actor = smgpc::scene::nameobj::describe_name_obj_creator_support("MarioActor");

        require(!smgpc::scene::nameobj::can_create_name_obj("Mario") &&
                    !smgpc::scene::nameobj::can_create_name_obj("MarioActor") &&
                    mario.kind == smgpc::scene::nameobj::NameObjCreatorSupportKind::NotLinked &&
                    mario_actor.kind == smgpc::scene::nameobj::NameObjCreatorSupportKind::NotLinked &&
                    mario.reason == "retail_creator_not_linked" &&
                    mario_actor.reason == "retail_creator_not_linked",
                "MarioHolder must not enable either player factory entry before the real closure exists");
    }

    void test_bindings_are_single_scene_and_isolated() {
        auto first_holder = SceneObjHolder{};
        {
            const auto first_binding = smgpc::scene::SceneObjHolderBinding(first_holder);
            require(MR::createSceneObj(SceneObj_StageSwitchContainer) != nullptr,
                    "the first scene should create its own supported object");
            require(MR::createSceneObj(SceneObj_PlanetGravityManager) != nullptr,
                    "the first scene should create its own gravity manager");

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
        require(!MR::isExistSceneObj(SceneObj_PlanetGravityManager) &&
                    MR::getSceneObj<PlanetGravityManager>(
                        SceneObj_PlanetGravityManager) == nullptr,
                "a later scene must not inherit the previous scene's gravity manager");
    }

    void test_switch_watcher_holder_retires_children_before_recreation() {
        const auto baseline =
            smgpc::compat::name_obj_runtime_state_count();

        for (auto generation = 0; generation < 2; ++generation) {
            {
                auto holder = SceneObjHolder{};
                const auto binding =
                    smgpc::scene::SceneObjHolderBinding(holder);
                auto *watcher_holder = dynamic_cast<SwitchWatcherHolder *>(
                    MR::createSceneObj(SceneObj_SwitchWatcherHolder));
                require(watcher_holder != nullptr,
                        "the scene must create its real SwitchWatcherHolder");

                watcher_holder->addSwitchWatcher(new SwitchWatcher(nullptr));
                watcher_holder->addSwitchWatcher(new SwitchWatcher(nullptr));
                require(smgpc::compat::name_obj_runtime_state_count() ==
                            baseline + 3U,
                        "SwitchWatcherHolder did not retain exactly its two registered children");
            }

            require(smgpc::compat::name_obj_runtime_state_count() == baseline,
                    "SwitchWatcherHolder teardown left registered children before scene recreation");
        }
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
        TestCase{"MarioHolder precedes real actor creation", test_mario_holder_precedes_real_actor_creation},
        TestCase{"StageHost creates MarioHolder before StartInfo", test_stage_host_creates_mario_holder_before_start_preflight},
        TestCase{"MarioHolder does not install a player creator", test_holder_does_not_install_a_player_creator},
        TestCase{"bindings are single-scene and isolated", test_bindings_are_single_scene_and_isolated},
        TestCase{"SwitchWatcher holder retires children before recreation",
                 test_switch_watcher_holder_retires_children_before_recreation},
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
