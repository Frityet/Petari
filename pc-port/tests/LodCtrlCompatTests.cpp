#include "Game/LiveActor/ClippingDirector.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/LodCtrl.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

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

    [[nodiscard]] std::string readFile(std::string_view path) {
        auto stream = std::ifstream(std::string(path), std::ios::binary);
        require(stream.is_open(), std::string("could not open source-boundary file: ") + std::string(path));
        return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
    }

    [[nodiscard]] std::string normalizeLodCtrlNoInlinePlacement(std::string source) {
        constexpr auto replacements = std::array{
            std::pair{
                "    void LodFuntionCall(LodCtrl* pCtrl, void (*pFunc)(LiveActor*)) NO_INLINE {",
                "    NO_INLINE void LodFuntionCall(LodCtrl* pCtrl, void (*pFunc)(LiveActor*)) {"},
            std::pair{
                "    void LodFuntionCall(LodCtrl* pCtrl, void (*pFunc)(LiveActor*, T), T arg) NO_INLINE {",
                "    NO_INLINE void LodFuntionCall(LodCtrl* pCtrl, void (*pFunc)(LiveActor*, T), T arg) {"},
        };
        for (const auto& [suffix_form, prefix_form] : replacements) {
            if (const auto offset = source.find(suffix_form); offset != std::string::npos) {
                source.replace(offset, std::char_traits<char>::length(suffix_form), prefix_form);
            }
        }
        return source;
    }

    void test_game_source_boundary_retains_only_gcc_attribute_relocation() {
        require(normalizeLodCtrlNoInlinePlacement(readFile("../src/Game/LiveActor/LodCtrl.cpp")) ==
                    readFile("src/Game/LiveActor/LodCtrl.cpp"),
                "the PC Game LodCtrl source may differ only by the GCC-required NO_INLINE prefix placement");
        require(readFile("../include/Game/LiveActor/LodCtrl.hpp") == readFile("src/Game/LiveActor/LodCtrl.hpp"),
                "the PC Game LodCtrl header must be byte-identical to the root decomp header");
    }

    void test_director_must_be_explicitly_scene_owned() {
        auto actor = LiveActor("lod-scene-owner-test");

        auto rejected_without_scene = false;
        try {
            const auto ctrl = LodCtrl(&actor, JMapInfoIter{});
            (void)ctrl;
        } catch (const std::logic_error&) {
            rejected_without_scene = true;
        }
        require(rejected_without_scene,
                "LodCtrl construction must reject an absent scene instead of fabricating a director");

        auto holder = SceneObjHolder{};
        const auto binding = smgpc::scene::SceneObjHolderBinding(holder);

        auto rejected_without_director = false;
        try {
            const auto ctrl = LodCtrl(&actor, JMapInfoIter{});
            (void)ctrl;
        } catch (const std::logic_error&) {
            rejected_without_director = true;
        }
        require(rejected_without_director,
                "binding a scene must not implicitly create a ClippingDirector for LodCtrl");

        auto* created = dynamic_cast< ClippingDirector* >(MR::createSceneObj(SceneObj_ClippingDirector));
        require(created != nullptr && MR::getClippingDirector() == created,
                "explicit creation must return the active scene's real ClippingDirector");

        const auto ctrl = LodCtrl(&actor, JMapInfoIter{});
        require(ctrl.mActor == &actor && ctrl._8 == &actor && ctrl._0 == 2000.0F && ctrl._4 == 3000.0F,
                "a scene-backed LodCtrl must retain the retail constructor state");
    }

    void test_exact_update_uses_real_host_state() {
        auto holder = SceneObjHolder{};
        const auto binding = smgpc::scene::SceneObjHolderBinding(holder);
        require(MR::createSceneObj(SceneObj_ClippingDirector) != nullptr,
                "the update test requires an explicitly created scene director");

        auto actor = LiveActor("lod-update-test");
        actor.makeActorAppeared();
        auto ctrl = LodCtrl(&actor, JMapInfoIter{});
        ctrl.validate();

        auto high = false;
        auto middle = false;
        auto low = false;
        auto hidden = true;
        ctrl.setViewCtrlPtr(&high, &middle, &low, &hidden);
        ctrl.update();
        require(MR::isHiddenModel(&actor) && ctrl._8 == nullptr,
                "the recovered no-submodel update path must hide the actual high model");

        hidden = false;
        ctrl.update();
        require(!MR::isHiddenModel(&actor) && ctrl._8 == &actor,
                "clearing the real view flag must restore the actual high model");

        ctrl.invalidate();
        hidden = true;
        ctrl.update();
        require(!MR::isHiddenModel(&actor) && ctrl._8 == &actor && ctrl._18 == 0,
                "an invalid controller must preserve the recovered early-return behavior");
    }

    void test_missing_resources_and_shadows_stay_absent() {
        constexpr auto missing_model = "SMGPC_LodCtrl_RealOrAbsent_Missing_5A6F2C8D";
        require(!LodCtrlFunction::isExistLodLowModel(missing_model),
                "a missing Low archive must remain absent instead of producing a fallback model");

        auto holder = SceneObjHolder{};
        const auto binding = smgpc::scene::SceneObjHolderBinding(holder);
        require(MR::createSceneObj(SceneObj_ClippingDirector) != nullptr,
                "the resource test requires an explicitly created scene director");

        auto actor = LiveActor("lod-resource-test");
        actor.initModelManagerWithAnm(missing_model, nullptr, false);
        actor.makeActorAppeared();
        auto ctrl = LodCtrl(&actor, JMapInfoIter{});
        ctrl.createLodModel(-1, -1, -1);
        require(ctrl._10 == nullptr && ctrl._14 == nullptr && ctrl._18 == 0,
                "only real Middle/Low archives may create LodCtrl submodels");

        auto rejected_shadow_sync = false;
        try {
            ctrl.offSyncShadowHost();
        } catch (const std::logic_error&) {
            rejected_shadow_sync = true;
        }
        require(rejected_shadow_sync && ctrl._1A != 0,
                "missing real shadow ownership must reject explicitly without reporting fake state");

        const auto lod_baseline = smgpc::compat::actor_lod_ctrl_runtime_state_count();
        auto rejected_npc_lod = false;
        try {
            (void)MR::createLodCtrlNPC(&actor, JMapInfoIter{});
        } catch (const std::logic_error&) {
            rejected_npc_lod = true;
        }
        require(rejected_npc_lod &&
                    smgpc::compat::actor_lod_ctrl_runtime_state_count() == lod_baseline,
                "NPC LodCtrl construction must reject a missing real shadow without retaining partial ownership");
    }

    void test_npc_lod_owns_controller_and_synchronizes_all_shadows() {
        auto holder = SceneObjHolder{};
        const auto binding = smgpc::scene::SceneObjHolderBinding(holder);
        require(MR::createSceneObj(SceneObj_ClippingDirector) != nullptr,
                "the NPC LOD test requires an explicitly created scene director");

        const auto lod_baseline = smgpc::compat::actor_lod_ctrl_runtime_state_count();
        {
            auto actor = LiveActor("lod-npc-owner-test");
            actor.initModelManagerWithAnm("SMGPC_LodCtrl_RealOrAbsent_Missing_5A6F2C8D", nullptr, false);
            actor.makeActorAppeared();
            actor.initShadowControllerList(2U);
            auto& primary = smgpc::compat::add_actor_shadow_controller(
                &actor, "primary", smgpc::compat::ActorShadowControllerKind::VolumeSphere,
                75.0F);
            auto& secondary = smgpc::compat::add_actor_shadow_controller(
                &actor, "secondary", smgpc::compat::ActorShadowControllerKind::VolumeSphere,
                50.0F);
            require(primary.visible_sync_host && secondary.visible_sync_host,
                    "every real shadow controller must default to host visibility synchronization");

            auto* ctrl = MR::createLodCtrlNPC(&actor, JMapInfoIter{});
            require(ctrl != nullptr && ctrl->_1A == 0 && !primary.visible_sync_host &&
                        !secondary.visible_sync_host &&
                        smgpc::compat::actor_lod_ctrl_runtime_state_count() == lod_baseline + 1U,
                    "NPC LOD creation must atomically adopt its controller and disable host shadow visibility sync");

            ctrl->kill();
            require(primary.visible_sync_host && secondary.visible_sync_host,
                    "killing an independently synchronized NPC LOD must restore every host shadow visibility sync");
            ctrl->appear();
            require(!primary.visible_sync_host && !secondary.visible_sync_host,
                    "appearing an independently synchronized NPC LOD must disable every host shadow visibility sync again");
            ctrl->kill();
            ctrl->validate();
            require(!primary.visible_sync_host && !secondary.visible_sync_host &&
                        ctrl->_18 != 0,
                    "validating an NPC LOD must appear it and retain independent shadow visibility");
        }
        require(smgpc::compat::actor_lod_ctrl_runtime_state_count() == lod_baseline,
                "LiveActor release must destroy its adopted NPC LodCtrl");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };

}  // namespace

int main() {
    constexpr auto tests = std::array{
        TestCase{"Game source boundary retains only GCC attribute relocation", test_game_source_boundary_retains_only_gcc_attribute_relocation},
        TestCase{"director must be explicitly scene-owned", test_director_must_be_explicitly_scene_owned},
        TestCase{"exact update uses real host state", test_exact_update_uses_real_host_state},
        TestCase{"missing resources and shadows stay absent", test_missing_resources_and_shadows_stay_absent},
        TestCase{"NPC LOD ownership and shadow sync", test_npc_lod_owns_controller_and_synchronizes_all_shadows},
    };

    auto failures = 0;
    for (const auto& test : tests) {
        try {
            test.run();
            std::cout << "[ok] " << test.name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[fail] " << test.name << ": " << error.what() << '\n';
        }
    }

    if (failures != 0) {
        std::cerr << failures << " LodCtrl real-or-absent test(s) failed\n";
        return 1;
    }

    std::cout << tests.size() << " LodCtrl real-or-absent test(s) passed\n";
    return 0;
}
