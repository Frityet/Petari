#include "resource/TextEncoding.hpp"
#include "SceneExecutionFixture.hpp"
#include "Game/Gravity/PlanetGravityManager.hpp"
#include "Game/Map/CollisionDirector.hpp"
#include "Game/Map/StageSwitch.hpp"
#include "Game/Map/SwitchWatcher.hpp"
#include "Game/Map/SwitchWatcherHolder.hpp"
#include "Game/Player/MarioHolder.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/ShareUtil.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
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

    struct SceneFixture {
        std::shared_ptr<smgpc::compat::JkrHeapRuntime> heaps = smgpc::compat::JkrHeapRuntime::create(16U << 20);
        smgpc::runtime::SceneScheduler scheduler;
        smgpc::runtime::SceneSchedulerBinding scheduler_binding{scheduler};
        smgpc::test::SceneExecutionFixture scene{
            scheduler, smgpc::compat::JkrAllocationDomain::create(heaps, 8U << 20)};
    };

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
        SceneFixture fixture;
        auto& holder = fixture.scene.holder();
        auto& binding = fixture.scene.objects();
        require(smgpc::scene::current_scene_initialization_state() == SceneInitializeState_Init &&
                    !MR::isInitializeStatePlacementSomething() && !MR::isInitializeStateEnd(),
                "a fresh scene holder did not own the retail Init phase");

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
                    smgpc::resource::decode_cp932(gravity_manager->getName()) == "重力",
                "SceneObj 0x32 must be the exact scene-owned PlanetGravityManager");
        require(MR::createSceneObj(SceneObj_PlanetGravityManager) == gravity_manager,
                "repeated gravity-manager creation must return the scene singleton");

        auto* resources = dynamic_cast<ResourceShare*>(MR::createSceneObj(SceneObj_ResourceShare));
        require(resources != nullptr &&
                    MR::getSceneObj<ResourceShare>(SceneObj_ResourceShare) == resources &&
                    MR::createSceneObj(SceneObj_ResourceShare) == resources &&
                    smgpc::resource::decode_cp932(resources->getName()) == "資源共有機構" &&
                    resources->_14 == 0U && resources->_C != nullptr && resources->_10 != nullptr &&
                    resources->_C != resources->_10,
                "ResourceShare must be the original scene singleton with its distinct original buffers and empty count");
        auto& scene_heap = smgpc::scene::current_scene_allocation_domain()->heap();
        require(JKRHeap::findFromRoot(resources) == &scene_heap &&
                    JKRHeap::findFromRoot(resources->_C) == &scene_heap &&
                    JKRHeap::findFromRoot(resources->_10) == &scene_heap,
                "the original ResourceShare object and both buffers must share the scene arena lifetime");

        auto* collision = dynamic_cast<CollisionDirector*>(MR::createSceneObj(SceneObj_CollisionDirector));
        require(collision && MR::getCollisionDirector() == collision &&
                    MR::createSceneObj(SceneObj_CollisionDirector) == collision,
                "the active original CollisionDirector must be the scene's exact singleton");
        require(MR::createSceneObj(SceneObj_EventDirector) == nullptr &&
                    !MR::isExistSceneObj(SceneObj_EventDirector),
                "an unsupported SceneObj factory entry must remain absent");
        require(MR::createSceneObj(SceneObj_MiiFacePartsHolder) == nullptr &&
                    !MR::isExistSceneObj(SceneObj_MiiFacePartsHolder),
                "the Mii holder must remain absent until real character-model construction and drawing exist");
        fixture.scene.complete_initialization();
        binding.complete_initialization();
        require(MR::isInitializeStateEnd() && !MR::isInitializeStatePlacementSomething(),
                "successful scene completion did not persist the retail End phase");
    }

    void test_mario_holder_precedes_real_actor_creation() {
        SceneFixture fixture;
        auto& holder = fixture.scene.holder();

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
        require(smgpc::resource::decode_cp932(mario_holder->getName()) == "マリオ保持" &&
                    mario_holder->getMarioActor() == nullptr,
                "MarioHolder must exist with its exact name and a null actor before real MarioActor init2");
        require(MR::createSceneObj(SceneObj_MarioHolder) == mario_holder &&
                    mario_holder->getMarioActor() == nullptr,
                "repeated creation must preserve the same empty holder until the real actor registers itself");
    }

    void test_planet_catalog_precedes_authored_data_resolution() {
        const auto host = read_file("src/scene/StageInitializationService.cpp");
        const auto host_environment =
            host.find("void StageInitializationService::load_stage_files()");
        const auto host_catalog =
            host.find("_planet_map_catalog =", host_environment);
        const auto host_data = host.find("_authored_data =", host_catalog);

        const auto gateway = read_file("src/scene/GatewayDemoScene.cpp");
        const auto gateway_constructor = gateway.find("explicit Impl(");
        const auto gateway_session =
            gateway.find("_stage_session_binding =", gateway_constructor);
        const auto gateway_catalog =
            gateway.find("_planet_map_catalog =", gateway_session);
        const auto gateway_data =
            gateway.find("_authored_data =", gateway_catalog);
        const auto gateway_placements =
            gateway.find("_authored_placements =", gateway_data);
        const auto gateway_preload = gateway.find(
            "_authored_placements->preload()", gateway_placements);
        const auto gateway_finalize = gateway.find(
            "void finalize_placements(LiveActor &player)", gateway_preload);
        const auto gateway_construct = gateway.find(
            "_authored_placements->instantiate()", gateway_finalize);
        const auto gateway_collision_prepass = gateway.find(
            "_collision.build()", gateway_construct);
        const auto gateway_scene_postpass = gateway.find(
            "_scene_binding->init_after_placement()", gateway_collision_prepass);
        const auto gateway_player_postpass = gateway.find(
            "_runtime->name_obj_lifecycle().init_after_placement(player)",
            gateway_scene_postpass);
        const auto gateway_player_sync = gateway.find(
            "_runtime->player_system().synchronize_attached_actor()",
            gateway_player_postpass);
        const auto gateway_placement_postpass = gateway.find(
            "_authored_placements->init_after_placement()",
            gateway_player_sync);
        const auto gateway_collision_validation = gateway.find(
            "validate_planet_collision()", gateway_placement_postpass);
        const auto gateway_collision_rebuild = gateway.find(
            "_collision.build()", gateway_collision_validation);
        const auto gateway_retire = gateway.find("void retire() noexcept");
        const auto gateway_clear_placements = gateway.find(
            "_authored_placements->clear()", gateway_retire);
        const auto gateway_clear_collision = gateway.find(
            "_collision.clear()", gateway_clear_placements);
        const auto gateway_deactivate_collision = gateway.find(
            "_collision.deactivate()", gateway_clear_collision);

        require(host_environment != std::string::npos &&
                    host_catalog != std::string::npos &&
                    host_data != std::string::npos &&
                    host_environment < host_catalog &&
                    host_catalog < host_data &&
                    gateway_constructor != std::string::npos &&
                    gateway_session != std::string::npos &&
                    gateway_catalog != std::string::npos &&
                    gateway_data != std::string::npos &&
                    gateway_placements != std::string::npos &&
                    gateway_preload != std::string::npos &&
                    gateway_finalize != std::string::npos &&
                    gateway_construct != std::string::npos &&
                    gateway_collision_prepass != std::string::npos &&
                    gateway_scene_postpass != std::string::npos &&
                    gateway_player_postpass != std::string::npos &&
                    gateway_player_sync != std::string::npos &&
                    gateway_placement_postpass != std::string::npos &&
                    gateway_collision_validation != std::string::npos &&
                    gateway_collision_rebuild != std::string::npos &&
                    gateway_retire != std::string::npos &&
                    gateway_clear_placements != std::string::npos &&
                    gateway_clear_collision != std::string::npos &&
                    gateway_deactivate_collision != std::string::npos &&
                    gateway_constructor < gateway_session &&
                    gateway_session < gateway_catalog &&
                    gateway_catalog < gateway_data &&
                    gateway_data < gateway_placements &&
                    gateway_placements < gateway_preload &&
                    gateway_preload < gateway_finalize &&
                    gateway_finalize < gateway_construct &&
                    gateway_construct < gateway_collision_prepass &&
                    gateway_collision_prepass < gateway_scene_postpass &&
                    gateway_scene_postpass < gateway_player_postpass &&
                    gateway_player_postpass < gateway_player_sync &&
                    gateway_player_sync < gateway_placement_postpass &&
                    gateway_placement_postpass < gateway_collision_validation &&
                    gateway_collision_validation < gateway_collision_rebuild &&
                    gateway_retire < gateway_clear_placements &&
                    gateway_clear_placements < gateway_clear_collision &&
                    gateway_clear_collision < gateway_deactivate_collision,
                "Gateway must stop after preload, finalize around one external-player postpass, and retire placements before collision");
    }

    void test_model_changing_archive_path_uses_retail_mounted_prefix() {
        const auto uses_retail_flag = [](std::string_view path) {
            const auto source = read_file(path);
            const auto call = source.find(
                "MR::makeObjectArchiveFileNameFromPrefix(");
            const auto call_end = source.find(")) {", call);
            if (call == std::string::npos || call_end == std::string::npos) {
                return false;
            }
            const auto invocation = source.substr(call, call_end - call);
            return invocation.find("true") != std::string::npos &&
                   invocation.find("false") == std::string::npos;
        };

        require(
            uses_retail_flag("src/scene/AuthoredPlacementInstantiator.cpp") &&
                uses_retail_flag("src/scene/NameObjLifecycleService.cpp"),
            "model-changing rank and preload must request the mounted-object `%s%02d` archive path");
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
        {
            SceneFixture first;
            auto& first_holder = first.scene.holder();
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

        SceneFixture second;
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
                SceneFixture fixture;
                const auto scene_baseline = smgpc::compat::name_obj_runtime_state_count();
                auto *watcher_holder = dynamic_cast<SwitchWatcherHolder *>(
                    MR::createSceneObj(SceneObj_SwitchWatcherHolder));
                require(watcher_holder != nullptr,
                        "the scene must create its real SwitchWatcherHolder");

                watcher_holder->addSwitchWatcher(new SwitchWatcher(nullptr));
                watcher_holder->addSwitchWatcher(new SwitchWatcher(nullptr));
                require(smgpc::compat::name_obj_runtime_state_count() ==
                            scene_baseline + 3U,
                        "SwitchWatcherHolder did not retain exactly its two registered children");
                fixture.scene.complete_initialization();
                fixture.scene.objects().complete_initialization();
            }

            require(smgpc::compat::name_obj_runtime_state_count() == baseline,
                    "SwitchWatcherHolder teardown left registered children before scene recreation");
        }
    }

    void test_switch_watcher_adoption_failure_releases_raw_child() {
        const auto baseline =
            smgpc::compat::name_obj_runtime_state_count();
        {
            SceneFixture fixture;
            auto watcher_holder = SwitchWatcherHolder{};
            fixture.scene.complete_initialization();
            fixture.scene.objects().complete_initialization();
            fixture.scene.retire();
            require(!MR::getSceneObjHolder() && watcher_holder.mExecutorIdx == -1 &&
                        smgpc::compat::name_obj_runtime_state_count() == baseline + 1U,
                    "scene retirement must detach the externally owned watcher before testing absent-owner adoption");
            auto rejected_without_scene_owner = false;
            try {
                watcher_holder.addSwitchWatcher(new SwitchWatcher(nullptr));
            } catch (const std::logic_error& error) {
                rejected_without_scene_owner = std::string_view(error.what()).find(
                    "without an active SceneObjHolder binding") != std::string_view::npos;
            }
            require(rejected_without_scene_owner &&
                        watcher_holder.mSwitchWatcher.size() == 0 &&
                        smgpc::compat::name_obj_runtime_state_count() ==
                            baseline + 1U,
                    "failed SwitchWatcher adoption leaked its raw-new child registration");
        }
        require(smgpc::compat::name_obj_runtime_state_count() == baseline,
                "standalone SwitchWatcherHolder fixture did not restore the registry baseline");
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
        TestCase{"planet catalog precedes authored data resolution",
                 test_planet_catalog_precedes_authored_data_resolution},
        TestCase{"model-changing archive path uses retail mounted prefix",
                 test_model_changing_archive_path_uses_retail_mounted_prefix},
        TestCase{"MarioHolder does not install a player creator", test_holder_does_not_install_a_player_creator},
        TestCase{"bindings are single-scene and isolated", test_bindings_are_single_scene_and_isolated},
        TestCase{"SwitchWatcher holder retires children before recreation",
                 test_switch_watcher_holder_retires_children_before_recreation},
        TestCase{"SwitchWatcher adoption failure releases raw child",
                 test_switch_watcher_adoption_failure_releases_raw_child},
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
