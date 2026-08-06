#include "Game/LiveActor/Nerve.hpp"
#include "Game/Effect/SimpleEffectObj.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/ActorAppearSwitchListener.hpp"
#include "Game/Map/StageSwitch.hpp"
#include "Game/Map/SwitchWatcher.hpp"
#include "Game/MapObj/CollisionBlocker.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/System/StorySequenceExecutor.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include "JSystem/JGeometry/TBox.hpp"
#include "JSystem/JGeometry/TMatrix.hpp"
#include "JSystem/JGeometry/TUtil.hpp"
#include "runtime/RuntimeServices.hpp"
#include "runtime/SceneScheduler.hpp"

#include <RVLFaceLib.h>
#include <aurora/dvd.h>
#include <aurora/nand.hpp>
#include <aurora/wpad.hpp>
#include <dolphin/dvd.h>
#include <dolphin/gx.h>
#include <dolphin/os.h>
#include <dolphin/vi.h>
#include <revolution.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void write_be32(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint32_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 24U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value >> 16U);
        bytes[offset + 2U] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 3U] = static_cast<std::uint8_t>(value);
    }

    JMapInfo make_fieldless_jmap(std::uint32_t entry_count) {
        auto bytes = std::vector<std::uint8_t>(0x10U, 0U);
        write_be32(bytes, 0x00U, entry_count);
        write_be32(bytes, 0x08U, 0x10U);
        return JMapInfo::from_bcsv(bytes);
    }

    int g_pre_retrace = -1;
    int g_post_retrace = -1;

    void on_pre_retrace(u32 count) {
        g_pre_retrace = static_cast<int>(count);
    }

    void on_post_retrace(u32 count) {
        g_post_retrace = static_cast<int>(count);
    }

    void test_revolution_headers_and_input_defaults() {
        static_assert(sizeof(u8) == 1U);
        static_assert(sizeof(u16) == 2U);
        static_assert(sizeof(u32) == 4U);
        static_assert(sizeof(s32) == 4U);
        static_assert(NAND_MAX_PATH == 64U);
        static_assert(RFL_NAME_LEN == 10U);

        aurora::wpad_service().clear();
        auto type = u32{0xFFFFFFFFU};
        require(WPADProbe(0, &type) == FALSE, "WPADProbe should report disconnected before Aurora receives controller state");
        require(type == 0U, "WPADProbe should zero the controller type when disconnected");

        auto status = KPADStatus{};
        require(KPADRead(0, &status, 1U) == 0, "KPADRead should not synthesize samples before Aurora receives controller state");

        auto &wpad = aurora::wpad_service();
        wpad.begin_frame();
        wpad.set_button_mask(WPAD_CHAN0, WPAD_BUTTON_A | WPAD_BUTTON_UP);
        wpad.set_pointer(WPAD_CHAN0, 123.0F, 234.0F, true);
        wpad.set_distance_to_display(WPAD_CHAN0, 1.0F);
        require(WPADProbe(WPAD_CHAN0, &type) == TRUE, "WPADProbe should report Aurora-fed controller state");
        require(KPADRead(WPAD_CHAN0, &status, 1U) == 1, "KPADRead should expose Aurora-fed controller samples");
        require((status.hold & WPAD_BUTTON_A) != 0U && (status.hold & WPAD_BUTTON_UP) != 0U,
                "KPADRead should preserve held WPAD buttons");
        require(status.dpd_valid_fg == 1 && status.pos.x == 123.0F && status.pos.y == 234.0F,
                "KPADRead should preserve pointer coordinates");
        WPADDisconnect(WPAD_CHAN0);
        require(WPADProbe(WPAD_CHAN0, nullptr) == FALSE, "WPADDisconnect should clear Aurora controller state");
    }

    void test_jgeometry_host_layout_and_math() {
        static_assert(std::is_same_v<TVec3f, JGeometry::TVec3<f32>>);
        static_assert(sizeof(TVec3f) == sizeof(Vec));
        static_assert(sizeof(TVec3f) == 12U);
        static_assert(alignof(TVec3f) == alignof(Vec));
        static_assert(sizeof(TPos3f) == sizeof(Mtx));
        static_assert(sizeof(TBox3f) == 24U);

        auto vector = TVec3f{3.0F, 4.0F, 0.0F};
        require(vector.squared() == 25.0F && vector.squared(TVec3f{3.0F, 0.0F, 0.0F}) == 16.0F,
                "JGeometry vectors should expose source-compatible squared magnitudes and distances");
        require(vector.normalize() == 5.0F && vector.epsilonEquals(TVec3f{0.6F, 0.8F, 0.0F}, 0.00001F),
                "JGeometry vector normalization should return the original length");

        auto zero = TVec3f{};
        require(zero.normalize() == 0.0F && zero.squared() == 0.0F, "normalizing a zero vector should remain finite and zero");

        auto scale_add = TVec3f{1.0F, 2.0F, 3.0F};
        scale_add.scaleAdd(2.0F, TVec3f{4.0F, 5.0F, 6.0F}, scale_add);
        require(scale_add.epsilonEquals(TVec3f{9.0F, 12.0F, 15.0F}, 0.0F),
                "scaleAdd should remain correct when the destination aliases its base operand");
        require(TVec3f{3.0F, 4.0F, 5.0F}.killElement(TVec3f{0.0F, 1.0F, 0.0F}).epsilonEquals(TVec3f{3.0F, 0.0F, 5.0F}, 0.0F),
                "killElement should remove the component parallel to a normalized direction");
        require(JGeometry::TUtil<f32>::clamp(-1.0F, 0.0F, 2.0F) == 0.0F &&
                    JGeometry::TUtil<f32>::clamp(3.0F, 0.0F, 2.0F) == 2.0F,
                "TUtil clamp should preserve the source boundary behavior");

        auto lhs = TPos3f{};
        lhs.identity();
        lhs.mMtx[0][0] = 0.0F;
        lhs.mMtx[0][1] = -1.0F;
        lhs.mMtx[1][0] = 1.0F;
        lhs.mMtx[1][1] = 0.0F;
        lhs.setTrans(10.0F, 20.0F, 0.0F);

        auto rhs = TPos3f{};
        rhs.identity();
        rhs.setTrans(2.0F, 3.0F, 4.0F);

        auto expected = TPos3f{};
        expected.concat(lhs, rhs);
        require(expected.toMtxPtr() == expected.mMtx && expected.mMtx[0][3] == 7.0F && expected.mMtx[1][3] == 22.0F &&
                    expected.mMtx[2][3] == 4.0F,
                "TPos3f concat should compose affine translation and expose its writable Mtx view");

        const auto matrix_equal = [](const TPos3f &left, const TPos3f &right) {
            for (int row = 0; row < 3; ++row) {
                for (int column = 0; column < 4; ++column) {
                    if (left.mMtx[row][column] != right.mMtx[row][column]) {
                        return false;
                    }
                }
            }
            return true;
        };

        auto alias_lhs = lhs;
        alias_lhs.concat(alias_lhs, rhs);
        auto alias_rhs = rhs;
        alias_rhs.concat(lhs, alias_rhs);
        require(matrix_equal(alias_lhs, expected) && matrix_equal(alias_rhs, expected),
                "TPos3f concat should support destination aliasing either input operand");

        auto local = TVec3f{2.0F, 3.0F, 4.0F};
        auto world = TVec3f{};
        lhs.mult(local, world);
        require(world.epsilonEquals(TVec3f{7.0F, 22.0F, 4.0F}, 0.0F), "TPos3f mult should apply rotation and translation");
        lhs.multTranspose(world, world);
        require(world.epsilonEquals(local, 0.0F), "TPos3f multTranspose should be writable and safe for an aliased source/destination");

        auto box = TBox3f{};
        box.set(TVec3f{-1.0F, -2.0F, -3.0F}, TVec3f{1.0F, 2.0F, 3.0F});
        require(box.intersectsPoint(TVec3f{0.0F, 0.0F, 0.0F}) && !box.intersectsPoint(TVec3f{1.0F, 0.0F, 0.0F}),
                "TBox3f point tests should retain the original half-open maximum boundary");
    }

    void test_aurora_vi_retrace_and_framebuffer_state() {
        VIInit();
        VIConfigure(&GXNtsc480IntDf);
        require(VIGetTvFormat() == VI_NTSC, "VI should expose NTSC from GXNtsc480IntDf");
        require(VIGetScanMode() == VI_INTERLACE, "VI should expose interlaced scan mode");
        require(VIGetRetraceCount() == 0U, "VI retrace count should reset to zero");

        auto framebuffer = std::array<std::uint8_t, 32U>{};
        VISetNextFrameBuffer(framebuffer.data());
        require(VIGetNextFrameBuffer() == framebuffer.data(), "VI next framebuffer should be stored");

        g_pre_retrace = -1;
        g_post_retrace = -1;
        require(VISetPreRetraceCallback(on_pre_retrace) == nullptr, "first VI pre callback install should return null");
        require(VISetPostRetraceCallback(on_post_retrace) == nullptr, "first VI post callback install should return null");
        VIWaitForRetrace();
        require(VIGetRetraceCount() == 1U, "VIWaitForRetrace should advance the retrace count");
        require(VIGetCurrentFrameBuffer() == framebuffer.data(), "VIWaitForRetrace should publish the next framebuffer");
        require(g_pre_retrace == 0 && g_post_retrace == 1, "VI retrace callbacks should receive pre/post counts");
        VISetPreRetraceCallback(nullptr);
        VISetPostRetraceCallback(nullptr);
    }

    void test_aurora_dvd_requires_disc_image() {
        aurora_dvd_close();
        require(!aurora_dvd_open(nullptr), "aurora_dvd_open should reject a null disc path");
        require(!aurora_dvd_open("/definitely/not/a/smg/disc.rvz"), "aurora_dvd_open should reject missing disc images");
        DVDInit();
        require(DVDGetDriveStatus() == DVD_STATE_NO_DISK, "DVD should report no disk until Aurora opens a disc image");
        require(DVDConvertPathToEntrynum("/ObjectData") < 0, "DVD path lookup should fail without an open disc image");

        auto file = DVDFileInfo{};
        require(DVDOpen("/ObjectData/Mario.arc", &file) == FALSE, "DVDOpen should fail without an open disc image");
    }

    void test_aurora_os_cache_and_gx_copy_smoke() {
        auto cache_bytes = std::array<std::uint8_t, 64U>{};
        DCFlushRange(cache_bytes.data(), static_cast<u32>(cache_bytes.size()));
        DCInvalidateRange(cache_bytes.data(), static_cast<u32>(cache_bytes.size()));
        DCZeroRange(cache_bytes.data(), static_cast<u32>(cache_bytes.size()));
        require(cache_bytes[0] == 0U && cache_bytes.back() == 0U, "DCZeroRange should clear its byte range");

        alignas(32) auto fifo = std::array<std::uint8_t, 32U * 1024U>{};
        require(GXInit(fifo.data(), static_cast<u32>(fifo.size())) != nullptr, "GXInit should return a FIFO object");
        GXSetCopyClear(GXColor{.r = 0U, .g = 0U, .b = 0U, .a = 255U}, GX_MAX_Z24);
        GXSetDispCopySrc(0U, 0U, 640U, 456U);
        GXSetDispCopyDst(640U, 456U);
        GXCopyDisp(nullptr, GX_TRUE);
    }

    void test_aurora_nand_storage_smoke() {
        auto nand = aurora::NandFileSystem{};
        const auto payload = std::array<std::uint8_t, 4U>{1U, 2U, 3U, 4U};
        nand.write_file("save/banner.bin", std::span<const std::uint8_t>(payload), 0x3CU, 0U);

        const auto normalized = nand.normalize_path("save/banner.bin");
        require(normalized.starts_with(aurora::NandFileSystem::title_data_root()),
                "relative NAND paths should live under the title data root");
        require(nand.exists("save/banner.bin"), "NAND write should make the file visible");

        const auto readback = nand.read_file("save/banner.bin");
        require(readback.has_value() && *readback == std::vector<std::uint8_t>(payload.begin(), payload.end()),
                "NAND readback should match written bytes");
        require(nand.rename("save/banner.bin", "save/banner-new.bin") == NAND_RESULT_OK, "NAND rename should succeed");
        require(!nand.exists("save/banner.bin") && nand.exists("save/banner-new.bin"), "NAND rename should move the file");
        require(nand.erase("save/banner-new.bin"), "NAND erase should remove the file");

        const auto check = nand.check(1U, 1U);
        require(check.result == NAND_RESULT_OK && check.free_blocks > 0U && check.free_inodes > 0U,
                "NAND quota check should report free space");
    }

    void test_player_visibility_can_be_restored() {
        auto player = smgpc::runtime::PlayerSystemService{};
        require(!player.is_player_hidden(), "player visibility should default to shown");
        player.hide_player();
        require(player.is_player_hidden(), "hide_player should mark the player hidden");
        player.show_player();
        require(!player.is_player_hidden(), "show_player should restore player visibility");
    }

    void test_scene_scheduler_registration_scope_cleanup() {
        auto scheduler = smgpc::runtime::SceneScheduler{};
        auto persistent = NameObj("persistent");
        auto scene_local_actor = LiveActor("scene-local");

        scheduler.connect_name_obj(persistent, 0, -1, -1, -1);
        const auto marker = scheduler.registration_marker();
        scheduler.connect_name_obj(persistent, 1, -1, -1, -1);
        scheduler.connect_name_obj(scene_local_actor, 0, -1, -1, -1);

        const auto removed_scene = scheduler.remove_registrations_since(marker);
        require(removed_scene.size() == 1U && removed_scene.front().name == "scene-local",
                "scene cleanup should remove registrations made after its marker");
        require(removed_scene.front().live_actor == &scene_local_actor,
                "scene cleanup should identify movement-only LiveActor registrations");

        const auto removed_persistent = scheduler.remove_registrations_since(0U);
        require(removed_persistent.size() == 1U && removed_persistent.front().name == "persistent",
                "scene cleanup should preserve registrations made before its marker");
    }

    class CountingSwitchListener final : public SwitchEventListener {
    public:
        void listenSwitchOnEvent() override {
            ++on_count;
        }

        void listenSwitchOffEvent() override {
            ++off_count;
        }

        int on_count = 0;
        int off_count = 0;
    };

    class CurrentSceneObjHolderGuard {
    public:
        explicit CurrentSceneObjHolderGuard(SceneObjHolder &holder) {
            MR::setCurrentSceneObjHolder(&holder);
        }

        ~CurrentSceneObjHolderGuard() {
            MR::setCurrentSceneObjHolder(nullptr);
        }
    };

    void test_stage_switch_zone_identity_and_edges() {
        auto holder = SceneObjHolder{};
        const auto holder_guard = CurrentSceneObjHolderGuard(holder);

        auto zone_one_info = JMapInfo{};
        zone_one_info.setPlacedZoneId(1);
        auto zone_two_info = JMapInfo{};
        zone_two_info.setPlacedZoneId(2);
        const auto zone_one_iter = JMapInfoIter(&zone_one_info, 0);
        const auto zone_two_iter = JMapInfoIter(&zone_two_info, 0);

        auto local_zone_one = SwitchIdInfo(7, zone_one_iter);
        auto local_zone_two = SwitchIdInfo(7, zone_two_iter);
        auto global_zone_one = SwitchIdInfo(1007, zone_one_iter);
        auto global_zone_two = SwitchIdInfo(1007, zone_two_iter);

        auto *container = MR::getSceneObj<StageSwitchContainer>(SceneObj_StageSwitchContainer);
        container->createAndAddZone(local_zone_one);
        container->createAndAddZone(local_zone_two);

        require(!StageSwitchFunction::isOnSwitchBySwitchIdInfo(local_zone_one) &&
                    !StageSwitchFunction::isOnSwitchBySwitchIdInfo(local_zone_two),
                "same-numbered local switches should start off in separate zone banks");

        auto ctrl = StageSwitchCtrl(JMapInfoIter{});
        ctrl.mSW_A = &local_zone_one;
        auto watcher = SwitchWatcher(&ctrl);
        auto listener = CountingSwitchListener{};
        watcher.addSwitchListener(&listener, 1U);
        watcher.movement();

        StageSwitchFunction::onSwitchBySwitchIdInfo(local_zone_one);
        require(StageSwitchFunction::isOnSwitchBySwitchIdInfo(local_zone_one), "local switch should turn on in its placed zone");
        require(!StageSwitchFunction::isOnSwitchBySwitchIdInfo(local_zone_two),
                "turning on a local switch must not affect the same number in another zone");
        watcher.movement();
        watcher.movement();
        require(listener.on_count == 1 && listener.off_count == 0, "watcher should emit one rising edge and no duplicate steady-state edge");

        StageSwitchFunction::onSwitchBySwitchIdInfo(global_zone_one);
        require(StageSwitchFunction::isOnSwitchBySwitchIdInfo(global_zone_two),
                "global switches should share their bank across placement zones");

        StageSwitchFunction::offSwitchBySwitchIdInfo(local_zone_one);
        watcher.movement();
        require(listener.on_count == 1 && listener.off_count == 1, "watcher should emit one falling edge after the switch turns off");
    }

    void test_collision_blocker_sensor_lifecycle() {
        auto blocker = CollisionBlocker("CollisionBlocker");
        blocker.init(JMapInfoIter{});

        const auto* sensor = blocker.getSensor("eye");
        require(sensor != nullptr, "CollisionBlocker should create its original eye sensor");
        require(sensor->mType == ATYPE_EYE && sensor->mGroupSize == 4U && sensor->mRadius == 50.0F,
                "CollisionBlocker eye sensor should preserve the original type, group size, and radius");
        require(!blocker.isDead(), "CollisionBlocker should appear after initialization");

        blocker.forceBreak();
        require(blocker.isDead(), "CollisionBlocker forceBreak should kill the actor");
    }

    void test_simple_effect_host_compatibility() {
        auto steam = SimpleEffectObj("Steam");
        const auto* first_offset = steam.getClippingCenterOffset();
        const auto* second_offset = steam.getClippingCenterOffset();

        require(first_offset != nullptr && first_offset == second_offset,
                "SimpleEffectObj clipping offset should use persistent host storage");
        require(first_offset->x == 0.0F && first_offset->y == 0.0F && first_offset->z == 0.0F,
                "SimpleEffectObj clipping offset should preserve the original zero value");
        require(MR::isEqualString("Steam", "Steam") && !MR::isEqualString("Steam", "Smoke") && !MR::isEqualString(nullptr, "Steam"),
                "original string equality should be null-safe on the host");

        auto camera = smgpc::runtime::CameraSystemService{};
        camera.begin_frame(42U);
        camera.request_weak_shake();
        camera.request_strong_shake();
        const auto events = camera.shake_request_events();

        require(camera.weak_shake_request_count() == 1U && camera.strong_shake_request_count() == 1U,
                "weak and strong camera shake requests should remain distinct");
        require(events.size() == 2U && events[0].kind == smgpc::runtime::CameraSystemService::ShakeRequestKind::Weak &&
                    events[1].kind == smgpc::runtime::CameraSystemService::ShakeRequestKind::Strong && events[0].frame_index == 42U &&
                    events[1].frame_index == 42U,
                "camera shake events should retain their kind and frame");
    }

    void test_rail_info_ownership_and_per_entry_lookup() {
        auto placement_info = [] {
            auto placement = make_fieldless_jmap(2U);
            placement.setRailInfo(0, make_fieldless_jmap(2U), make_fieldless_jmap(3U), 1);
            placement.setRailInfo(1, make_fieldless_jmap(1U), make_fieldless_jmap(4U), 0);
            return placement;
        }();

        auto retained_copy = placement_info;
        placement_info = JMapInfo{};

        auto path_iter = JMapInfoIter{};
        const JMapInfo *point_info = nullptr;
        MR::getRailInfo(&path_iter, &point_info, JMapInfoIter(&retained_copy, 0));
        require(path_iter.isValid() && path_iter.mIndex == 1,
                "rail header iterator should retain the matched CommonPathInfo row");
        require(point_info != nullptr && point_info->getNumEntries() == 3,
                "rail point metadata should outlive the resolver's temporary tables");

        MR::getRailInfo(&path_iter, &point_info, JMapInfoIter(&retained_copy, 1));
        require(path_iter.isValid() && path_iter.mIndex == 0,
                "each placement row should retain its own CommonPathInfo association");
        require(point_info != nullptr && point_info->getNumEntries() == 4,
                "each placement row should retain its own CommonPathPointInfo table");
    }

    class EnvironmentVariableGuard {
    public:
        explicit EnvironmentVariableGuard(const char *name) : _name(name) {
            if (const auto *value = std::getenv(_name.c_str())) {
                _old_value = value;
            }
        }

        ~EnvironmentVariableGuard() {
            if (_old_value.has_value()) {
                setenv(_name.c_str(), _old_value->c_str(), 1);
            } else {
                unsetenv(_name.c_str());
            }
        }

        void set(const char *value) const {
            setenv(_name.c_str(), value, 1);
        }

        void unset() const {
            unsetenv(_name.c_str());
        }

    private:
        std::string _name;
        std::optional<std::string> _old_value;
    };

    void test_heavensdoor_route_is_picturebook_handoff_only() {
        auto boot = EnvironmentVariableGuard("SMGPC_DEMO_BOOT");
        auto route = EnvironmentVariableGuard("SMGPC_DEMO_ROUTE");
        boot.set("heavensdoor_bunny");
        route.unset();

        const auto initial = StorySequenceExecutor::makeInitialStageRequest();
        require(initial.mStageName == "FileSelect", "SMGPC_DEMO_BOOT should not bypass file select");

        boot.unset();
        route.set("heavensdoor_after_picturebook");
        auto &executor = smgpc::game::story_sequence_executor();
        require(executor.shouldRouteToHeavensDoorBunnyDemoAfterPictureBook(),
                "heavensdoor_after_picturebook should arm the picturebook handoff");
        executor.requestHeavensDoorBunnyDemoAfterPictureBook();
        const auto pending = executor.takePendingStageRequest();
        require(pending.has_value(), "picturebook handoff should create a pending stage request");
        require(pending->mStageName == "HeavensDoorGalaxy", "picturebook handoff should request HeavensDoorGalaxy");
        require(pending->mScenarioNo == 1, "picturebook handoff should request scenario 1");
    }

    struct SpineProbeState {
        int first_executions = 0;
        int second_executions = 0;
        const Nerve *second_nerve = nullptr;
    };

    class SpineProbeFirstNerve final : public Nerve {
    public:
        void execute(Spine *spine) const override {
            auto *state = static_cast<SpineProbeState *>(spine->mExecutor);
            ++state->first_executions;
            spine->setNerve(state->second_nerve);
        }
    };

    class SpineProbeSecondNerve final : public Nerve {
    public:
        void execute(Spine *spine) const override {
            auto *state = static_cast<SpineProbeState *>(spine->mExecutor);
            ++state->second_executions;
        }
    };

    void test_spine_pending_nerve_runs_next_tick() {
        const auto first = SpineProbeFirstNerve{};
        const auto second = SpineProbeSecondNerve{};
        auto state = SpineProbeState{
            .second_nerve = &second,
        };
        auto spine = Spine(&state, &first);

        require(spine.getCurrentNerve() == &first, "Spine should start on its initial nerve");
        spine.update();
        require(state.first_executions == 1, "first nerve should execute on the first update");
        require(state.second_executions == 0, "queued nerve should not execute in the same update");
        require(spine.getCurrentNerve() == &first, "queued nerve should not appear current before it executes");

        spine.update();
        require(state.second_executions == 1, "queued nerve should execute on the next update");
        require(spine.getCurrentNerve() == &second, "executed nerve should become current after its first tick");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };
}  // namespace

int main() {
    const auto tests = std::array{
        TestCase{"revolution headers and input defaults", test_revolution_headers_and_input_defaults},
        TestCase{"JGeometry host layout and math", test_jgeometry_host_layout_and_math},
        TestCase{"Aurora VI retrace/framebuffer state", test_aurora_vi_retrace_and_framebuffer_state},
        TestCase{"Aurora DVD requires disc image", test_aurora_dvd_requires_disc_image},
        TestCase{"Aurora OS cache and GX copy smoke", test_aurora_os_cache_and_gx_copy_smoke},
        TestCase{"Aurora NAND storage smoke", test_aurora_nand_storage_smoke},
        TestCase{"player visibility can be restored", test_player_visibility_can_be_restored},
        TestCase{"scene scheduler registration scope cleanup", test_scene_scheduler_registration_scope_cleanup},
        TestCase{"stage switch zone identity and edges", test_stage_switch_zone_identity_and_edges},
        TestCase{"CollisionBlocker sensor lifecycle", test_collision_blocker_sensor_lifecycle},
        TestCase{"SimpleEffectObj host compatibility", test_simple_effect_host_compatibility},
        TestCase{"rail info ownership and per-entry lookup", test_rail_info_ownership_and_per_entry_lookup},
        TestCase{"HeavensDoor route is picturebook handoff only", test_heavensdoor_route_is_picturebook_handoff_only},
        TestCase{"Spine pending nerve runs next tick", test_spine_pending_nerve_runs_next_tick},
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
        std::cerr << failures << " Aurora-native test(s) failed\n";
        return 1;
    }

    std::cout << tests.size() << " Aurora-native test(s) passed\n";
    return 0;
}
