#include "Game/LiveActor/ClippingDirector.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/LodCtrl.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/LiveActorUtil.hpp"
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

    void test_game_source_boundary_is_exact() {
        require(readFile("../src/Game/LiveActor/LodCtrl.cpp") == readFile("src/Game/LiveActor/LodCtrl.cpp"),
                "the PC Game LodCtrl source must be byte-identical to the root decomp source");
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
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };

}  // namespace

int main() {
    constexpr auto tests = std::array{
        TestCase{"Game source boundary is exact", test_game_source_boundary_is_exact},
        TestCase{"director must be explicitly scene-owned", test_director_must_be_explicitly_scene_owned},
        TestCase{"exact update uses real host state", test_exact_update_uses_real_host_state},
        TestCase{"missing resources and shadows stay absent", test_missing_resources_and_shadows_stay_absent},
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
