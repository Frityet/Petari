#include "NativeHeapFixture.hpp"
#include "SceneExecutionFixture.hpp"
#include "OriginalSceneControllerFixture.hpp"
#include "Game/LiveActor/LiveActorGroup.hpp"
#include "Game/Map/SphereSelector.hpp"
#include "Game/Map/SphereSelectorHandle.hpp"
#include "Game/NameObj/NameObjFactory.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "Game/Util/StarPointerUtil.hpp"
#include "Game/Demo/DemoDirector.hpp"
#include "Game/Demo/DemoSimpleCastHolder.hpp"
#include "camera/CameraDirectorRuntime.hpp"
#include "runtime/RuntimeServices.hpp"
#include "runtime/SceneScheduler.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

static_assert(std::is_base_of_v<LiveActor, SphereSelectorHandle>);

namespace {

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    template <typename Function>
    void require_logic_error(Function &&function, std::string_view expected_text,
                             std::string_view message) {
        try {
            function();
        } catch (const std::logic_error &error) {
            require(std::string_view(error.what()).find(expected_text) != std::string_view::npos,
                    message);
            return;
        }
        throw std::runtime_error(std::string(message));
    }

    void test_scene_obj_6f_is_exact_and_synchronous() {
        require(MR::createSceneObj(SceneObj_SphereSelector) == nullptr,
                "SceneObj 0x6F must remain absent without a scene-owned holder");

        const auto heaps = smgpc::test::create_native_root_heap(16U << 20);
        auto scheduler = smgpc::runtime::SceneScheduler{};
        const auto active_scheduler = smgpc::runtime::SceneSchedulerBinding(scheduler);
        smgpc::test::OriginalSceneControllerFixture original(heaps);
        auto scene = smgpc::test::SceneExecutionFixture(
            scheduler, smgpc::test::create_native_solid_heap(heaps, 4U << 20),
            &original.scene);
        auto *created = MR::createSceneObj(SceneObj_SphereSelector);
        auto *selector = dynamic_cast<SphereSelector *>(created);
        require(selector != nullptr && MR::isDead(selector),
                "SceneObj 0x6F must synchronously initialize the exact dead SphereSelector");
        require(selector->mSphereGroup != nullptr &&
                    selector->mSphereGroup->getObjNum() == 0 &&
                    selector->mHandle == nullptr,
                "synchronous SphereSelector init must create its real empty retail target group");
        require(MR::createSceneObj(SceneObj_SphereSelector) == selector,
                "SceneObj 0x6F must retain one scene-owned singleton");
    }

    void test_generalized_pointer_and_layout_contract() {
        auto pointer = smgpc::runtime::StarPointerService{};
        auto requester = 0;
        auto other_requester = 0;
        pointer.begin_frame(77U);
        pointer.push_mode(&requester,
                          smgpc::runtime::StarPointerMode::SphereSelectorFinger);
        pointer.push_mode(&requester,
                          smgpc::runtime::StarPointerMode::SphereSelectorReaction);
        pointer.push_mode(&other_requester,
                          smgpc::runtime::StarPointerMode::DocumentViewer);
        require(pointer.mode_request_count(&requester) == 2U &&
                    pointer.mode() == smgpc::runtime::StarPointerMode::DocumentViewer,
                "retail-priority requester modes must preserve both SphereSelector requests");

        pointer.pop_mode(&other_requester);
        require(pointer.mode() ==
                    smgpc::runtime::StarPointerMode::SphereSelectorReaction,
                "ending a higher-priority owner must reveal the Sphere reaction request");
        pointer.pop_mode(&requester);
        require(pointer.mode_request_count(&requester) == 1U &&
                    pointer.mode() ==
                        smgpc::runtime::StarPointerMode::SphereSelectorFinger,
                "retail end-mode must pop the requester's most recent request");
        pointer.clear_mode_requests(&requester);
        require(pointer.mode_request_count(&requester) == 0U &&
                    pointer.mode() == smgpc::runtime::StarPointerMode::None,
                "scene-owner cleanup must remove every stale requester entry");

        auto layouts = smgpc::runtime::GameLayoutService{};
        require(layouts.is_default_game_layout_active(),
                "the default game layout must begin active");
        layouts.deactivate_default_game_layout();
        require(!layouts.is_default_game_layout_active(),
                "Sphere selection must be able to deactivate the default layout");
        layouts.activate_default_game_layout();
        require(layouts.is_default_game_layout_active(),
                "Sphere teardown must restore the default layout through the inverse operation");

        require(smgpc::camera::current_original_camera_context() == nullptr,
                "the absent-camera query must have no original CameraContext owner");
        require_logic_error(
            [] { (void)MR::getCameraViewMtx(); }, "Camera state is unavailable",
            "the Sphere camera dependency must not manufacture a view matrix");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };

}  // namespace

int main() {
    constexpr auto tests = std::array{
        TestCase{"SceneObj 0x6F exact synchronous init",
                 test_scene_obj_6f_is_exact_and_synchronous},
        TestCase{"generalized pointer/layout real-or-absent contract",
                 test_generalized_pointer_and_layout_contract},
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
        std::cerr << failures << " SphereSelector real-or-absent test(s) failed\n";
        return 1;
    }
    std::cout << tests.size()
              << " SphereSelector real-or-absent test(s) passed\n";
    return 0;
}
