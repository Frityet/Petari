#include "Game/LiveActor/LiveActorGroup.hpp"
#include "Game/Map/SphereSelector.hpp"
#include "Game/Map/SphereSelectorHandle.hpp"
#include "Game/NameObj/NameObjFactory.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "Game/Util/StarPointerUtil.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "runtime/RuntimeServices.hpp"
#include "runtime/SceneScheduler.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/StagePlacementResolver.hpp"
#include "scene/nameobj/NameObjFactory.hpp"

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

    [[nodiscard]] std::optional<std::filesystem::path> find_real_disc() {
        if (const auto *configured = std::getenv("SMGPC_REAL_DISC");
            configured != nullptr && configured[0] != '\0') {
            return std::filesystem::path(configured);
        }

        auto error = std::error_code{};
        auto directory = std::filesystem::current_path(error);
        if (error) {
            return std::nullopt;
        }
        while (true) {
            for (const auto name : {"RMGK01.iso", "RMGK01.wbfs"}) {
                const auto candidate = directory / name;
                if (std::filesystem::is_regular_file(candidate, error) && !error) {
                    return candidate;
                }
                error.clear();
            }
            const auto parent = directory.parent_path();
            if (parent == directory || parent.empty()) {
                break;
            }
            directory = parent;
        }
        return std::nullopt;
    }

    void test_scene_obj_6f_is_exact_and_synchronous() {
        require(MR::createSceneObj(SceneObj_SphereSelector) == nullptr,
                "SceneObj 0x6F must remain absent without a scene-owned holder");

        auto holder = SceneObjHolder{};
        const auto binding = smgpc::scene::SceneObjHolderBinding(holder);
        auto *created = MR::createSceneObj(SceneObj_SphereSelector);
        auto *selector = dynamic_cast<SphereSelector *>(created);
        require(selector != nullptr && selector->isDead(),
                "SceneObj 0x6F must synchronously initialize the exact dead SphereSelector");
        require(selector->mSphereGroup != nullptr &&
                    selector->mSphereGroup->mObjectCount == 0 &&
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

        require_logic_error(
            [] { (void)MR::getStarPointerScreenPositionOrEdge(WPAD_CHAN0); },
            "active game runtime",
            "a pointer-edge query must not manufacture input without the active runtime");
        require_logic_error(
            [] { (void)MR::getCameraViewMtx(); }, "Camera state is unavailable",
            "the Sphere camera dependency must not manufacture a view matrix");
    }

    void test_handle_factory_stays_absent_at_remaining_audio_boundaries() {
        const auto support =
            smgpc::scene::nameobj::describe_name_obj_creator_support(
                "SphereSelectorHandle");
        require(
            support.kind == smgpc::scene::nameobj::NameObjCreatorSupportKind::RuntimeClosureUnavailable &&
                support.reason ==
                    "me_and_multi_stage_bgm_playback_runtime_unavailable" &&
                NameObjFactory::getCreator("SphereSelectorHandle") == nullptr,
            "SphereSelectorHandle must remain absent until its reachable ME and multi-stage-BGM paths have real playback");

        require_logic_error(
            [] {
                (void)MR::startAtmosphereLevelSE(
                    "SE_AT_LV_ASTRO_DOME_WIND_1", 100, -1);
            },
            "active RuntimeContext",
            "the mandatory retail level-sound call must fail explicitly instead of becoming a silent event");
    }

    void test_real_file_select_row_exact_init_and_messages() {
        const auto disc_path = find_real_disc();
        if (!disc_path.has_value()) {
            std::cout
                << "[skip] real RMGK01 SphereSelectorHandle row (set SMGPC_REAL_DISC or place RMGK01.iso in a workspace ancestor)\n";
            return;
        }

        aurora_dvd_close();
        const auto disc_path_string = disc_path->string();
        require(aurora_dvd_open(disc_path_string.c_str()),
                "the selected real-disc fixture must be a readable SMG image");
        struct DiscCloseGuard {
            ~DiscCloseGuard() {
                aurora_dvd_close();
            }
        } close_guard;
        DVDInit();

        auto dvd = smgpc::runtime::DvdFileSystemService{"/"};
        const auto placements =
            smgpc::scene::resolve_stage_placement_objects(dvd, "FileSelect", 1);
        const auto row = std::ranges::find_if(placements, [](const auto &placement) {
            return placement.object_name == "SphereSelectorHandle";
        });
        if (row != placements.end()) {
            std::cout << "[info] handle source: zone=" << row->zone_name
                      << ";layer=" << row->layer_name
                      << ";table=" << row->table_path
                      << ";arg0=" << row->object_args[0]
                      << ";factory=" << row->factory_supported
                      << ";reason=" << row->support_reason << '\n';
        }
        require(row != placements.end() && row->stage_name == "FileSelect" &&
                    row->zone_name == "FileSelect" && row->layer_name == "common" &&
                    row->table_path == "jmp/placement/common/objinfo" &&
                    row->object_args[0] == 0 && !row->factory_supported &&
                    row->support_reason ==
                        "me_and_multi_stage_bgm_playback_runtime_unavailable",
                "the test must use the real RMGK01 FileSelect handle row and retain its precise blocker");

        auto holder = SceneObjHolder{};
        const auto binding = smgpc::scene::SceneObjHolderBinding(holder);
        require(MR::createSceneObj(SceneObj_MessageSensorHolder) != nullptr,
                "the exact message contract requires the retail scene message sensor");
        auto demo_runtime = smgpc::compat::DemoSceneRuntime{dvd, placements};
        auto handle = SphereSelectorHandle{"FileSelect SphereSelectorHandle"};
        const auto iter = JMapInfoIter(&row->jmap_info, row->jmap_entry_index);
        handle.init(iter);

        auto *selector =
            MR::getSceneObj<SphereSelector>(SceneObj_SphereSelector);
        require(handle.isDead() && !handle.mIsFileSelect && selector != nullptr &&
                    selector->isDead() && selector->mHandle == &handle &&
                    selector->mSphereGroup->mObjectCount == 1 &&
                    selector->mSphereGroup->getActor(0) == &handle,
                "exact placement init must synchronously bind the real handle into SceneObj 0x6F");
        require(demo_runtime.simple_cast_registration_count(&handle) == 1U,
                "exact placement init must retain the retail simple demo-cast registration");

        require(SphereSelectorFunction::isMsgSelectStart(
                    ACTMES_SPHERE_SELECTOR_SELECT_START) &&
                    SphereSelectorFunction::isMsgSelectEnd(
                        ACTMES_SPHERE_SELECTOR_SELECT_END) &&
                    SphereSelectorFunction::isMsgConfirmStart(
                        ACTMES_SPHERE_SELECTOR_CONFIRM_START) &&
                    SphereSelectorFunction::isMsgConfirmCancel(
                        ACTMES_SPHERE_SELECTOR_CONFIRM_CANCEL) &&
                    SphereSelectorFunction::isMsgConfirmed(
                        ACTMES_SPHERE_SELECTOR_CONFIRMED) &&
                    SphereSelectorFunction::isMsgTargetSelected(
                        ACTMES_SPHERE_SELECTOR_TARGET_SELECTED) &&
                    !SphereSelectorFunction::isMsgTargetSelected(0xFFFFFFFFU),
                "SphereSelector message classifiers must match the retail 0xE0-0xE5 contract");

        const auto *message_sensor = MR::getMessageSensor();
        const auto target_selected = MR::sendSimpleMsgToActor(
            ACTMES_SPHERE_SELECTOR_TARGET_SELECTED, &handle);
        require(message_sensor != nullptr && target_selected &&
                    !handle.isHolding() && handle.getNerveStep() == -1,
                "the real message sensor must queue Wait-to-Hold at the retail deferred-nerve boundary");
        require(MR::sendSimpleMsgToActor(
                    ACTMES_SPHERE_SELECTOR_CONFIRM_START, &handle) &&
                    !handle.isHolding() && handle.getNerveStep() == -1,
                "the real FileSelect-row confirm-start must replace the pending nerve through the exact message path");
        require(MR::sendSimpleMsgToActor(
                    ACTMES_SPHERE_SELECTOR_CONFIRM_CANCEL, &handle) &&
                    MR::sendSimpleMsgToActor(
                        ACTMES_SPHERE_SELECTOR_SELECT_END, &handle) &&
                    !MR::sendSimpleMsgToActor(0xFFFFFFFFU, &handle),
                "confirm-cancel/select-end must be accepted and an unknown message rejected");

        auto scheduler = smgpc::runtime::SceneScheduler{};
        const auto marker = scheduler.registration_marker();
        scheduler.connect_name_obj(handle, MR::MovementType_Environment,
                                   MR::CalcAnimType_MapObj, -1, -1);
        const auto registrations = scheduler.remove_registrations_since(marker);
        require(registrations.size() == 1U &&
                    registrations.front().kind ==
                        smgpc::runtime::SceneEntryKind::NameObj &&
                    registrations.front().name_obj == &handle &&
                    registrations.front().live_actor == &handle,
                "the generalized scheduler must retain the exact handle's LiveActor identity");

        handle.mRotateSpeed = 1.0F;
        require_logic_error(
            [&handle] { handle.playRotateSE(); },
            "active RuntimeContext",
            "the exact runtime path must require the active concrete audio service");

        std::cout << "[info] real-disc fixture: " << disc_path_string
                  << "; SphereSelectorHandle row=" << row->jmap_entry_index << '\n';
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
        TestCase{"factory stays absent at ME/multi-BGM boundaries",
                 test_handle_factory_stays_absent_at_remaining_audio_boundaries},
        TestCase{"real FileSelect row exact init/message contract",
                 test_real_file_select_row_exact_init_and_messages},
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
