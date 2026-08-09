#include "compat/MetrowerksStdCompat.hpp"

#include "Game/LiveActor/Nerve.hpp"
#include "Game/Effect/SimpleEffectObj.hpp"
#include "Game/LiveActor/ActorStateBase.hpp"
#include "Game/LiveActor/ActorStateKeeper.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/PartsModel.hpp"
#include "Game/LiveActor/RailRider.hpp"
#include "Game/Map/ActorAppearSwitchListener.hpp"
#include "Game/Map/RailPart.hpp"
#include "Game/Map/StageSwitch.hpp"
#include "Game/Map/SwitchWatcher.hpp"
#include "Game/MapObj/CollisionBlocker.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/NameObj/NameObjArchiveListCollector.hpp"
#include "Game/NameObj/NameObjFactory.hpp"
#include "Game/NPC/TalkMessageCtrl.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/System/GameDataHolder.hpp"
#include "Game/System/SaveDataHandleSequence.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/FixedPosition.hpp"
#include "Game/Util/GravityUtil.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/RailUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include "JSystem/JGeometry/TBox.hpp"
#include "JSystem/JGeometry/TMatrix.hpp"
#include "JSystem/JGeometry/TUtil.hpp"
#include "runtime/RuntimeServices.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/SceneScheduler.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/TplTexture.hpp"
#include "scene/StageCollisionService.hpp"
#include "scene/StageHostScene.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/nameobj/NameObjFactory.hpp"
#include "compat/ActorMotionCompat.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/GameGravityCompat.hpp"
#include "compat/PlayerUtilCompat.hpp"
#include "compat/SaveDataHandleSequenceCompat.hpp"

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
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <map>
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

    template <typename Fn>
    void require_logic_error(Fn&& fn, std::string_view message) {
        auto rejected = false;
        try {
            fn();
        } catch (const std::logic_error&) {
            rejected = true;
        }
        require(rejected, message);
    }

    void write_be32(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint32_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 24U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value >> 16U);
        bytes[offset + 2U] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 3U] = static_cast<std::uint8_t>(value);
    }

    void write_be16(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint16_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value);
    }

    void write_be_float(std::vector<std::uint8_t> &bytes, std::size_t offset, float value) {
        write_be32(bytes, offset, std::bit_cast<std::uint32_t>(value));
    }

    void write_bcsv_field(std::vector<std::uint8_t> &bytes, std::size_t index, std::string_view name, std::uint16_t offset,
                          smgpc::resource::BcsvFieldType type) {
        const auto field_offset = 0x10U + index * 0x0cU;
        write_be32(bytes, field_offset, smgpc::resource::jmap_hash(name));
        write_be32(bytes, field_offset + 0x04U, 0xffffffffU);
        write_be16(bytes, field_offset + 0x08U, offset);
        bytes[field_offset + 0x0aU] = 0U;
        bytes[field_offset + 0x0bU] = static_cast<std::uint8_t>(type);
    }

    JMapInfo make_fieldless_jmap(std::uint32_t entry_count) {
        auto bytes = std::vector<std::uint8_t>(0x10U, 0U);
        write_be32(bytes, 0x00U, entry_count);
        write_be32(bytes, 0x08U, 0x10U);
        return JMapInfo::from_bcsv(bytes);
    }

    std::vector<std::uint8_t> make_single_triangle_kcl(float thickness = 2.0F,
                                                       std::uint16_t attribute = 7U) {
        constexpr auto position_offset = 0x38U;
        constexpr auto normal_offset = 0x44U;
        constexpr auto prism_offset = 0x74U;
        constexpr auto octree_offset = 0x84U;
        auto bytes = std::vector<std::uint8_t>(0x88U, 0U);
        write_be32(bytes, 0x00U, position_offset);
        write_be32(bytes, 0x04U, normal_offset);
        // KCL stores this address 0x10 bytes before the first prism.
        write_be32(bytes, 0x08U, prism_offset - 0x10U);
        write_be32(bytes, 0x0cU, octree_offset);
        write_be_float(bytes, 0x10U, thickness);

        const auto write_vec3 = [&](std::size_t offset, float x, float y, float z) {
            write_be_float(bytes, offset, x);
            write_be_float(bytes, offset + 4U, y);
            write_be_float(bytes, offset + 8U, z);
        };
        write_vec3(position_offset, 0.0F, 0.0F, 0.0F);
        write_vec3(normal_offset + 0x00U, 0.0F, 1.0F, 0.0F);
        write_vec3(normal_offset + 0x0cU, 1.0F, 0.0F, 0.0F);
        write_vec3(normal_offset + 0x18U, 0.0F, 0.0F, 1.0F);
        constexpr auto diagonal = 0.70710678118F;
        write_vec3(normal_offset + 0x24U, diagonal, 0.0F, diagonal);

        write_be_float(bytes, prism_offset, diagonal);
        write_be16(bytes, prism_offset + 4U, 0U);
        write_be16(bytes, prism_offset + 6U, 0U);
        write_be16(bytes, prism_offset + 8U, 1U);
        write_be16(bytes, prism_offset + 10U, 2U);
        write_be16(bytes, prism_offset + 12U, 3U);
        write_be16(bytes, prism_offset + 14U, attribute);
        return bytes;
    }

    JMapInfo make_open_rail_path_info() {
        constexpr auto data_offset = 0x1cU;
        constexpr auto entry_size = 8U;
        auto bytes = std::vector<std::uint8_t>(data_offset + entry_size, 0U);
        write_be32(bytes, 0x00U, 1U);
        write_be32(bytes, 0x04U, 1U);
        write_be32(bytes, 0x08U, data_offset);
        write_be32(bytes, 0x0cU, entry_size);
        write_bcsv_field(bytes, 0U, "closed", 0U, smgpc::resource::BcsvFieldType::InlineString);
        bytes[data_offset + 0U] = 'O';
        bytes[data_offset + 1U] = 'P';
        bytes[data_offset + 2U] = 'E';
        bytes[data_offset + 3U] = 'N';
        return JMapInfo::from_bcsv(bytes);
    }

    JMapInfo make_linear_rail_point_info(std::uint32_t entry_count = 3U) {
        constexpr auto field_count = 10U;
        constexpr auto entry_size = field_count * 4U;
        constexpr auto data_offset = 0x10U + field_count * 0x0cU;
        constexpr auto field_names = std::array<std::string_view, field_count>{
            "pnt0_x", "pnt0_y", "pnt0_z", "pnt1_x", "pnt1_y", "pnt1_z", "pnt2_x", "pnt2_y", "pnt2_z", "id",
        };

        auto bytes = std::vector<std::uint8_t>(data_offset + entry_count * entry_size, 0U);
        write_be32(bytes, 0x00U, entry_count);
        write_be32(bytes, 0x04U, field_count);
        write_be32(bytes, 0x08U, data_offset);
        write_be32(bytes, 0x0cU, entry_size);
        for (auto field = 0U; field < field_count; ++field) {
            const auto type = field + 1U == field_count ? smgpc::resource::BcsvFieldType::Int32 : smgpc::resource::BcsvFieldType::Float;
            write_bcsv_field(bytes, field, field_names[field], static_cast<std::uint16_t>(field * 4U), type);
        }

        for (auto entry = 0U; entry < entry_count; ++entry) {
            const auto entry_offset = data_offset + entry * entry_size;
            const auto x = static_cast<float>(entry * 10U);
            write_be_float(bytes, entry_offset + 0U * 4U, x);
            write_be_float(bytes, entry_offset + 3U * 4U, x);
            write_be_float(bytes, entry_offset + 6U * 4U, x);
            write_be32(bytes, entry_offset + 9U * 4U, entry);
        }
        return JMapInfo::from_bcsv(bytes);
    }

    JMapInfo make_demo_rabbit_placement_info() {
        constexpr auto field_count = 4U;
        constexpr auto entry_count = 3U;
        constexpr auto entry_size = field_count * 4U;
        constexpr auto data_offset = 0x10U + field_count * 0x0cU;
        constexpr auto field_names = std::array<std::string_view, field_count>{
            "DemoGroupId", "CastId", "MessageId", "CommonPath_ID",
        };

        auto bytes = std::vector<std::uint8_t>(data_offset + entry_count * entry_size, 0U);
        write_be32(bytes, 0x00U, entry_count);
        write_be32(bytes, 0x04U, field_count);
        write_be32(bytes, 0x08U, data_offset);
        write_be32(bytes, 0x0cU, entry_size);
        for (auto field = 0U; field < field_count; ++field) {
            write_bcsv_field(bytes, field, field_names[field], static_cast<std::uint16_t>(field * 4U),
                             smgpc::resource::BcsvFieldType::Int32);
        }

        for (auto entry = 0U; entry < entry_count; ++entry) {
            const auto entry_offset = data_offset + entry * entry_size;
            write_be32(bytes, entry_offset + 0U * 4U, 0U);
            write_be32(bytes, entry_offset + 1U * 4U, entry);
            write_be32(bytes, entry_offset + 2U * 4U, entry == 0U ? 0U : 0xffffffffU);
            write_be32(bytes, entry_offset + 3U * 4U, entry == 0U ? 0U : 0xffffffffU);
        }

        auto info = JMapInfo::from_bcsv(bytes);
        info.setRailInfo(0, make_open_rail_path_info(), make_linear_rail_point_info(5U), 0);
        return info;
    }

    JMapInfo make_demo_cast_sentinel_placement_info() {
        constexpr auto field_count = 2U;
        constexpr auto entry_count = 2U;
        constexpr auto entry_size = field_count * 4U;
        constexpr auto data_offset = 0x10U + field_count * 0x0cU;
        constexpr auto field_names = std::array<std::string_view, field_count>{
            "DemoGroupId", "CastId",
        };

        auto bytes = std::vector<std::uint8_t>(data_offset + entry_count * entry_size, 0U);
        write_be32(bytes, 0x00U, entry_count);
        write_be32(bytes, 0x04U, field_count);
        write_be32(bytes, 0x08U, data_offset);
        write_be32(bytes, 0x0cU, entry_size);
        for (auto field = 0U; field < field_count; ++field) {
            write_bcsv_field(bytes, field, field_names[field], static_cast<std::uint16_t>(field * 4U),
                             smgpc::resource::BcsvFieldType::Int32);
        }

        // Gateway Rosetta has a valid group and the optional CastId sentinel.
        write_be32(bytes, data_offset + 0U * entry_size + 0U * 4U, 0U);
        write_be32(bytes, data_offset + 0U * entry_size + 1U * 4U, 0xffffffffU);
        // A missing group remains invalid even when CastId is present.
        write_be32(bytes, data_offset + 1U * entry_size + 0U * 4U, 0xffffffffU);
        write_be32(bytes, data_offset + 1U * entry_size + 1U * 4U, 0U);
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

    void test_game_pad_compat_requires_runtime_context() {
        require(smgpc::runtime::RuntimeContext::try_instance() == nullptr,
                "the strict GamePad boundary test must run without an active title runtime");
        require_logic_error(
            [] { static_cast<void>(MR::testCorePadButtonA(WPAD_CHAN0)); },
            "GamePad button queries must not manufacture neutral input without an active runtime context");
        require_logic_error(
            [] { static_cast<void>(MR::testSubPadStickTriggerRight(WPAD_CHAN0)); },
            "GamePad stick-edge queries must not bypass the active runtime context");
        require_logic_error(
            [] { static_cast<void>(MR::getPlayerStickX()); },
            "player stick queries must not manufacture neutral input without an active runtime context");
        require_logic_error(
            [] {
                auto direction = TVec3f{};
                MR::calcWorldStickDirectionXZ(&direction, WPAD_CHAN0);
            },
            "world-stick projection must not manufacture a camera basis without an active runtime context");
    }

    void test_aurora_wpad_sub_stick_edges() {
        auto service = aurora::WpadService{};
        service.set_connected(WPAD_CHAN0, true);

        service.begin_frame();
        service.set_sub_stick(WPAD_CHAN0, 0.2F, -0.2F);
        require(service.sub_stick_hold(WPAD_CHAN0) == aurora::WpadStickNone &&
                    service.sub_stick_trigger(WPAD_CHAN0) == aurora::WpadStickNone,
                "the retail ±0.2 Nunchuk threshold must remain exclusive");

        service.set_sub_stick(WPAD_CHAN0, 0.75F, 0.5F);
        require(service.sub_stick_hold(WPAD_CHAN0) == (aurora::WpadStickRight | aurora::WpadStickUp) &&
                    service.sub_stick_trigger(WPAD_CHAN0) == (aurora::WpadStickRight | aurora::WpadStickUp) &&
                    service.sub_stick_release(WPAD_CHAN0) == aurora::WpadStickNone,
                "crossing the retail Nunchuk threshold must produce directional hold and trigger bits");

        service.begin_frame();
        service.set_sub_stick(WPAD_CHAN0, 1.0F, 0.25F);
        require(service.sub_stick_trigger(WPAD_CHAN0) == aurora::WpadStickNone &&
                    service.sub_stick_release(WPAD_CHAN0) == aurora::WpadStickNone,
                "remaining beyond the Nunchuk threshold must not retrigger an edge");

        service.begin_frame();
        service.set_sub_stick(WPAD_CHAN0, -0.75F, -0.5F);
        require(service.sub_stick_trigger(WPAD_CHAN0) == (aurora::WpadStickLeft | aurora::WpadStickDown) &&
                    service.sub_stick_release(WPAD_CHAN0) == (aurora::WpadStickRight | aurora::WpadStickUp),
                "reversing the Nunchuk stick must release old directions and trigger new directions in one frame");
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

        const auto inline_source = TVec3f{2.0F, 4.0F, 6.0F};
        require(inline_source.scaleInline(0.5F).epsilonEquals(TVec3f{1.0F, 2.0F, 3.0F}, 0.0F) &&
                    inline_source.epsilonEquals(TVec3f{2.0F, 4.0F, 6.0F}, 0.0F),
                "scaleInline should return a scaled copy without mutating its source");
        auto inline_zero = inline_source;
        inline_zero.zeroInline();
        require(inline_zero.epsilonEquals(TVec3f{}, 0.0F), "zeroInline should preserve the original in-place zeroing surface");

        auto identity_quat = TQuat4f{};
        auto quat_front = TVec3f{};
        identity_quat.getZDir(quat_front);
        require(sizeof(identity_quat) == 16U && quat_front.epsilonEquals(TVec3f{0.0F, 0.0F, 1.0F}, 0.0F),
                "TQuat4f should retain its four-float layout and identity forward vector");

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

    void test_gx_ia8_channel_order() {
        auto block = std::array<std::uint8_t, 32U>{};
        block[0U] = 0x12U;
        block[1U] = 0x34U;

        const auto texture = smgpc::resource::decode_raw_gx_texture(
            std::span<const std::uint8_t>(block.data(), block.size()), 4U, 4U,
            smgpc::resource::TplTextureFormat::IA8);
        require(texture.rgba.size() == 4U * 4U * 4U,
                "IA8 decoding should produce one RGBA texel per source texel");
        require(texture.rgba[0U] == 0x34U && texture.rgba[1U] == 0x34U &&
                    texture.rgba[2U] == 0x34U && texture.rgba[3U] == 0x12U,
                "GX IA8 stores alpha before intensity in texture memory");
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

        auto drawable = NameObj("drawable");
        scheduler.connect_name_obj(drawable, -1, -1, -1, 7);
        require(scheduler.is_draw_connected(drawable),
                "new scheduler registrations must begin connected to draw");
        scheduler.disconnect_draw(drawable);
        require(!scheduler.is_draw_connected(drawable),
                "temporary draw disconnect must not remove the scheduler registration");
        scheduler.connect_draw(drawable);
        require(scheduler.is_draw_connected(drawable),
                "temporary draw connect must restore the existing scheduler registration");
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
        explicit CurrentSceneObjHolderGuard(SceneObjHolder &holder) : _binding(holder) {
        }

    private:
        smgpc::scene::SceneObjHolderBinding _binding;
    };

    void test_stage_switch_zone_identity_and_edges() {
        auto holder = SceneObjHolder{};
        const auto holder_guard = CurrentSceneObjHolderGuard(holder);
        require(MR::createSceneObj(SceneObj_StageSwitchContainer) != nullptr,
                "the retail stage-switch holder must be created explicitly for the scene");

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
        require(MR::isEqualString("Steam", "Steam") && !MR::isEqualString("Steam", "Smoke"),
                "host string equality should retain the retail strcmp semantics");

        auto camera = smgpc::runtime::CameraSystemService{};
        camera.set_game_camera_pose(smgpc::camera::CameraPose{});
        camera.set_shake_projection_dimensions(608.0F, 456.0F);
        camera.begin_frame(42U);
        camera.request_weak_shake();
        camera.request_strong_shake();
        const auto events = camera.shake_request_events();

        require(events.size() == 2U && events[0].kind == smgpc::runtime::CameraSystemService::ShakeRequestKind::Weak &&
                    events[1].kind == smgpc::runtime::CameraSystemService::ShakeRequestKind::Strong && events[0].frame_index == 42U &&
                    events[1].frame_index == 42U,
                "camera shake events should retain their kind and frame");
        camera.begin_frame(43U);
        const auto shaken = camera.effective_camera_pose();
        const auto remaining = 24.0F;
        const auto raw_offset = (0.2F + 6.0F) * std::sin(12.566371F * remaining / 25.0F) *
                                std::sin(1.5707964F * remaining / 25.0F);
        require(shaken.has_value() && std::abs(shaken->projection_offset_y - raw_offset * 30.0F / 456.0F) < 0.000001F,
                "camera shake should apply the retail damped sine and EFB-height projection scaling");
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

    void test_demo_rabbit_factory_is_absent() {
        require(!smgpc::scene::nameobj::can_create_name_obj("DemoRabbit"),
                "DemoRabbit must remain absent until every required default NPC capability is real");
        require(NameObjFactory::getCreator("DemoRabbit") == nullptr,
                "an actor that cannot initialize must not expose a factory constructor");

        auto placement = make_demo_rabbit_placement_info();
        for (auto row = 0; row < placement.getNumEntries(); ++row) {
            auto archives = NameObjArchiveListCollector{};
            NameObjFactory::getMountObjectArchiveList(&archives, "DemoRabbit", JMapInfoIter(&placement, row));
            require(archives.mCount == 0,
                    "an absent DemoRabbit factory must not advertise actor-specific archive preload support");
        }
    }

    void test_demo_cast_requires_scene_definition() {
        auto placement = make_demo_cast_sentinel_placement_info();
        auto actor = LiveActor("optional-cast-id");

        require_logic_error(
            [&] { (void)MR::tryRegisterDemoCast(&actor, JMapInfoIter(&placement, 0)); },
            "demo-cast registration without a scene-owned DemoDirector runtime must fail explicitly");
        require(!smgpc::compat::has_registered_demo_cast(&actor),
                "missing-runtime registration must not retain orphan cast state");

        require_logic_error(
            [&] { (void)MR::tryRegisterDemoCast(&actor, JMapInfoIter(&placement, 1)); },
            "missing placement metadata must not hide an absent scene-owned DemoDirector runtime");
        require(!smgpc::compat::has_registered_demo_cast(&actor),
                "a second missing-runtime registration must not create registry state");
    }

    void test_story_event_spin_entitlement_boundary() {
        auto player = smgpc::runtime::PlayerSystemService{};
        player.reset_stage_state();
        auto actor = LiveActor("story-event-player");
        player.attach_actor(actor);
        const auto player_context = smgpc::compat::ScopedPlayerSystemServiceOverride{player};

        require_logic_error(
            [&] { MR::onGameEventFlagEnableToSpinAndStarPointer(); },
            "spin entitlement must be unavailable while no retail save sequence is backed");
        require(!player.is_swing_permitted(),
                "an unavailable story write must not grant swing permission as a partial side effect");
        require_logic_error(
            [] { static_cast<void>(MR::isOnGameEventFlagEndTicoGuideDemo()); },
            "story-event queries must be unavailable while no retail save sequence is backed");
    }

    void test_star_piece_group_factory_is_absent_without_real_director() {
        require(!smgpc::scene::nameobj::can_create_name_obj("StarPieceGroup") &&
                    !smgpc::scene::nameobj::can_create_name_obj("StarPieceFlow"),
                "StarPiece group factories must remain absent until the real StarPieceDirector closure is linked");
        require(NameObjFactory::getCreator("StarPieceGroup") == nullptr && NameObjFactory::getCreator("StarPieceFlow") == nullptr,
                "unsupported StarPiece placements must not expose a partial creator");

        auto archives = NameObjArchiveListCollector{};
        NameObjFactory::getMountObjectArchiveList(&archives, "StarPieceGroup", JMapInfoIter());
        require(archives.mCount == 0,
                "an absent StarPiece group factory must not synthesize archive requests");
    }

    void test_stage_host_preserves_placement_appearance_state() {
        auto placement = smgpc::scene::StagePlacementObject{};
        require(smgpc::scene::should_apply_host_appear(nullptr),
                "an explicit non-placement stage root should retain the requested host-level appear pass");
        require(!smgpc::scene::should_apply_host_appear(&placement),
                "a placement root should retain the appeared/dead state chosen by its own initialization");
        require(smgpc::scene::should_apply_host_appear(&placement, true),
                "an explicit placement-backed root should still receive its requested host appear call");

        auto explicit_root = LiveActor("explicit-root");
        auto placement_root = LiveActor("placement-root");
        if (smgpc::scene::should_apply_host_appear(nullptr)) {
            explicit_root.makeActorAppeared();
        }
        if (smgpc::scene::should_apply_host_appear(&placement)) {
            placement_root.makeActorAppeared();
        }
        auto explicit_placement_root = LiveActor("explicit-placement-root");
        if (smgpc::scene::should_apply_host_appear(&placement, true)) {
            explicit_placement_root.makeActorAppeared();
        }
        require(!explicit_root.isDead() && placement_root.isDead() && !explicit_placement_root.isDead(),
                "the generic host appear policy should not revive a placement actor that initialized dead");
    }

    void test_kcl_collision_service_queries_and_binder_resolution() {
        auto collision = smgpc::scene::StageCollisionService{};
        constexpr auto identity = std::array<float, 12U>{
            1.0F, 0.0F, 0.0F, 0.0F,
            0.0F, 1.0F, 0.0F, 0.0F,
            0.0F, 0.0F, 1.0F, 0.0F,
        };
        const auto kcl = make_single_triangle_kcl();
        require(collision.add_kcl(kcl, identity, "native-test.kcl"),
                "a valid big-endian KCL prism should reconstruct into a host collision triangle");
        collision.build();
        require(collision.stats().mesh_count == 1U && collision.stats().triangle_count == 1U &&
                    collision.stats().rejected_triangle_count == 0U,
                "the KCL service should report its accepted resource and triangle counts");

        auto hit = smgpc::scene::StageCollisionHit{};
        require(collision.line_cast(TVec3f{0.25F, 1.0F, 0.25F}, TVec3f{0.0F, -2.0F, 0.0F}, &hit) &&
                    hit.position.epsilonEquals(TVec3f{0.25F, 0.0F, 0.25F}, 0.0001F) &&
                    hit.normal.epsilonEquals(TVec3f{0.0F, 1.0F, 0.0F}, 0.0001F) && hit.attribute == 7U,
                "line queries should return the reconstructed KCL position, normal, and attribute");
        require(!collision.line_cast(TVec3f{0.25F, -1.0F, 0.25F}, TVec3f{0.0F, 2.0F, 0.0F}, &hit),
                "KCL line queries should reject travel from the prism's back side toward its front");
        require(collision.line_cast(TVec3f{0.25F, 1.0F, 0.25F}, TVec3f{0.0F, -1.0F, 0.0F}, &hit) &&
                    std::abs(hit.fraction - 1.0F) < 0.0001F,
                "KCL line queries should accept an arrow ending exactly on the face");
        require(collision.line_cast(TVec3f{-0.005F, 1.0F, 0.25F}, TVec3f{0.0F, -2.0F, 0.0F}, &hit) &&
                    !collision.line_cast(TVec3f{-0.011F, 1.0F, 0.25F}, TVec3f{0.0F, -2.0F, 0.0F}, &hit),
                "KCL arrows should preserve the original 0.01-unit physical edge tolerance");

        const auto contacts = collision.sphere_contacts(TVec3f{0.25F, 0.25F, 0.25F}, 0.5F);
        require(!contacts.empty() && contacts.front().penetration > 0.24F && contacts.front().attribute == 7U,
                "sphere queries should expose penetrating KCL contacts to the generalized binder");
        const auto move = collision.move_sphere(TVec3f{0.25F, 0.75F, 0.25F}, TVec3f{0.0F, -0.5F, 0.0F}, 0.5F);
        const auto resolved_center = TVec3f{0.25F, 0.75F, 0.25F} + move.displacement;
        require(!move.contacts.empty() && resolved_center.y >= 0.5F && resolved_center.y < 2.0F,
                "binder motion should stop a moving sphere at the KCL surface instead of passing through it");

        const auto moving_away = collision.move_sphere(TVec3f{0.25F, 0.25F, 0.25F},
                                                        TVec3f{0.0F, 0.25F, 0.0F}, 0.5F);
        require(!moving_away.contacts.empty() && moving_away.displacement.y > 0.25F,
                "binder motion should resolve an initial overlap before preserving movement away from the face");

        auto duplicate_collision = smgpc::scene::StageCollisionService{};
        require(duplicate_collision.add_kcl(kcl, identity, "duplicate-a.kcl") &&
                    duplicate_collision.add_kcl(kcl, identity, "duplicate-b.kcl"),
                "the native fixture should permit coincident KCL sources");
        duplicate_collision.build();
        const auto duplicate_move = duplicate_collision.move_sphere(TVec3f{0.25F, 0.75F, 0.25F},
                                                                     TVec3f{0.0F, -0.5F, 0.0F}, 0.5F);
        require(duplicate_move.displacement.epsilonEquals(move.displacement, 0.0001F),
                "coincident triangles should use Binder's component extrema instead of summing duplicate reactions");
        const auto capped_move = duplicate_collision.move_sphere(TVec3f{0.25F, 0.75F, 0.25F},
                                                                  TVec3f{0.0F, -0.5F, 0.0F}, 0.5F, 1U);
        require(capped_move.contacts.size() == 1U &&
                    capped_move.displacement.epsilonEquals(move.displacement, 0.0001F),
                "Binder's third init argument should cap stored planes without changing a coincident-face reaction");

        auto ordered_collision = smgpc::scene::StageCollisionService{};
        constexpr auto raised = std::array<float, 12U>{
            1.0F, 0.0F, 0.0F, 0.0F,
            0.0F, 1.0F, 0.0F, 0.2F,
            0.0F, 0.0F, 1.0F, 0.0F,
        };
        require(ordered_collision.add_kcl(make_single_triangle_kcl(2.0F, 7U), identity, "first.kcl") &&
                    ordered_collision.add_kcl(make_single_triangle_kcl(2.0F, 9U), raised, "second.kcl"),
                "the native fixture should permit overlapping KCL sources at different depths");
        ordered_collision.build();
        const auto first_ordered_contact =
            ordered_collision.sphere_contacts(TVec3f{0.25F, 0.25F, 0.25F}, 0.5F, 1U);
        require(first_ordered_contact.size() == 1U && first_ordered_contact.front().attribute == 7U,
                "a full collision-plane array should use deterministic source order rather than deepest-first order");

        auto retry_collision = smgpc::scene::StageCollisionService{};
        constexpr auto wall = std::array<float, 12U>{
            0.0F, -4.0F, 0.0F, 10.0F,
            4.0F, 0.0F, 0.0F, 0.0F,
            0.0F, 0.0F, 4.0F, 0.0F,
        };
        require(retry_collision.add_kcl(kcl, identity, "retry-floor.kcl") &&
                    retry_collision.add_kcl(make_single_triangle_kcl(100.0F), wall, "retry-wall.kcl"),
                "the native fixture should permit orthogonal floor and wall KCL sources");
        retry_collision.build();
        const auto capped_retry = retry_collision.move_sphere(
            TVec3f{0.25F, 0.25F, 0.25F}, TVec3f{70.0F, 0.0F, 0.0F}, 0.5F, 1U);
        const auto retry_center = TVec3f{0.25F, 0.25F, 0.25F} + capped_retry.displacement;
        require(capped_retry.contacts.size() == 1U && retry_center.x > 23.0F && retry_center.x < 24.0F,
                "a full Binder plane array should still detect and stop at a projected-retry face without storing it");
        require(retry_center.y > 1.0F && retry_center.y < 2.0F,
                "an unstored projected-retry face must not contribute another collision reaction");

        auto thin_collision = smgpc::scene::StageCollisionService{};
        const auto thin_kcl = make_single_triangle_kcl(0.2F);
        require(thin_collision.add_kcl(thin_kcl, identity, "thin.kcl"),
                "a thin KCL prism should remain a valid collision resource");
        thin_collision.build();
        require(thin_collision.sphere_contacts(TVec3f{0.25F, 0.25F, 0.25F}, 0.5F).empty(),
                "a sphere penetration deeper than the KCL thickness should be rejected");
        const auto thickness_boundary = thin_collision.sphere_contacts(TVec3f{0.25F, 0.3F, 0.25F}, 0.5F);
        require(!thickness_boundary.empty() &&
                    std::abs(thickness_boundary.front().penetration - 0.2F) < 0.0001F,
                "the inclusive KCL thickness boundary should retain radius minus signed face distance");

        const auto edge_contact = collision.sphere_contacts(TVec3f{0.75F, 0.1F, 0.75F}, 0.5F);
        constexpr auto expected_edge_penetration = 0.25355339F;
        require(!edge_contact.empty() &&
                    std::abs(edge_contact.front().penetration - expected_edge_penetration) < 0.0001F &&
                    edge_contact.front().reaction_normal.epsilonEquals(TVec3f{0.0F, 1.0F, 0.0F}, 0.0001F),
                "edge contacts should use KCHitSphere's face-axis square-root correction and face normal");

        const auto point_contact = collision.sphere_contacts(TVec3f{0.25F, -1.0F, 0.25F}, 0.0F);
        require(!point_contact.empty() && std::abs(point_contact.front().penetration - 1.0F) < 0.0001F,
                "a zero-radius point behind the face but inside the extruded prism slab should collide");
        require(collision.sphere_contacts(TVec3f{0.0F, -1.0F, 0.25F}, 0.0F).empty(),
                "a zero-radius point exactly on a prism edge should retain KCHitSphere's strict rejection");
        const auto zero_depth = collision.sphere_contacts(TVec3f{0.25F, 0.5F, 0.25F}, 0.5F);
        require(!zero_depth.empty() && std::abs(zero_depth.front().penetration) < 0.0001F,
                "a face-interior sphere exactly tangent to KCL should remain a zero-depth hit");

        auto scaled_collision = smgpc::scene::StageCollisionService{};
        constexpr auto scale_four = std::array<float, 12U>{
            4.0F, 0.0F, 0.0F, 0.0F,
            0.0F, 4.0F, 0.0F, 0.0F,
            0.0F, 0.0F, 4.0F, 0.0F,
        };
        require(scaled_collision.add_kcl(kcl, scale_four, "scaled.kcl"),
                "a uniformly scaled KCL prism should remain loadable");
        scaled_collision.build();
        const auto scaled_depth = scaled_collision.sphere_contacts(TVec3f{1.0F, -4.0F, 1.0F}, 0.0F);
        require(!scaled_depth.empty() && std::abs(scaled_depth.front().penetration - 4.0F) < 0.0001F,
                "KCL header thickness should scale into world space with its collision transform");
        require(scaled_collision.line_cast(TVec3f{-0.03F, 4.0F, 1.0F}, TVec3f{0.0F, -8.0F, 0.0F}) &&
                    !scaled_collision.line_cast(TVec3f{-0.041F, 4.0F, 1.0F}, TVec3f{0.0F, -8.0F, 0.0F}),
                "KCHitArrow's 0.01 local-unit edge allowance should scale with transformed KCL");

        auto affine_collision = smgpc::scene::StageCollisionService{};
        constexpr auto shear_x_by_y = std::array<float, 12U>{
            1.0F, 1.0F, 0.0F, 0.0F,
            0.0F, 1.0F, 0.0F, 0.0F,
            0.0F, 0.0F, 1.0F, 0.0F,
        };
        require(affine_collision.add_kcl(kcl, shear_x_by_y, "affine.kcl"),
                "an affine-transformed KCL prism should remain loadable");
        affine_collision.build();
        require(!affine_collision.sphere_contacts(TVec3f{0.5F, -1.9F, 0.25F}, 0.0F).empty() &&
                    affine_collision.sphere_contacts(TVec3f{0.5F, -2.1F, 0.25F}, 0.0F).empty(),
                "KCL slab thickness should use transformed-plane separation under non-uniform affine transforms");

        collision.activate();
        require(smgpc::scene::StageCollisionService::active() == &collision,
                "the scene collision boundary should publish the current stage service");

        auto zero_binder = LiveActor("zero-binder-test");
        zero_binder.makeActorAppeared();
        zero_binder.mPosition.set(0.25F, -1.0F, 0.25F);
        zero_binder.initBinder(0.0F, 0.0F, 0U);
        smgpc::compat::integrate_live_actor_velocity(zero_binder);
        require(smgpc::compat::has_actor_binder(&zero_binder) && zero_binder.mBindedGround &&
                    zero_binder.mPosition.y > 1.0F,
                "a zero-radius Binder should remain explicit and resolve a strict-interior point-prism hit");

        auto matrix_binder = LiveActor("matrix-binder-test");
        matrix_binder.makeActorAppeared();
        matrix_binder.mPosition.set(0.25F, 2.0F, 0.25F);
        matrix_binder.setBaseMatrix(smgpc::render::J3dMatrix3x4{{
            1.0F, 0.0F, 0.0F, 0.25F,
            0.0F, -2.0F, 0.0F, 2.0F,
            0.0F, 0.0F, -1.0F, 0.25F,
        }});
        matrix_binder.initBinder(0.5F, 2.0F, 1U);
        smgpc::compat::integrate_live_actor_velocity(matrix_binder);
        require(matrix_binder.mBindedGround && matrix_binder.mPosition.y > 3.0F,
                "Binder offset should use the unscaled base-matrix Y direction instead of inferred anti-gravity");

        auto negative_scale_binder = LiveActor("negative-scale-binder-test");
        negative_scale_binder.makeActorAppeared();
        negative_scale_binder.mPosition.set(0.25F, -2.0F, 0.25F);
        negative_scale_binder.mScale.set(1.0F, -1.0F, 1.0F);
        negative_scale_binder.calcAndSetBaseMtx();
        negative_scale_binder.initBinder(0.5F, 2.0F, 1U);
        smgpc::compat::integrate_live_actor_velocity(negative_scale_binder);
        require(negative_scale_binder.mBindedGround && negative_scale_binder.mPosition.y > -1.0F,
                "host model-scale sign should not invert the scale-free Binder offset basis");

        collision.deactivate();

        auto collapsed_axis_collision = smgpc::scene::StageCollisionService{};
        constexpr auto positive_x_wall = std::array<float, 12U>{
            0.0F, 4.0F, 0.0F, 0.0F,
            -4.0F, 0.0F, 0.0F, 0.0F,
            0.0F, 0.0F, 4.0F, 0.0F,
        };
        require(collapsed_axis_collision.add_kcl(kcl, positive_x_wall, "collapsed-axis-wall.kcl"),
                "the native fixture should permit a positive-X wall");
        collapsed_axis_collision.build();
        collapsed_axis_collision.activate();
        auto collapsed_axis_binder = LiveActor("collapsed-axis-binder-test");
        collapsed_axis_binder.makeActorAppeared();
        collapsed_axis_binder.mPosition.set(2.0F, -1.0F, 1.0F);
        collapsed_axis_binder.mRotation.set(0.0F, 0.0F, 90.0F);
        collapsed_axis_binder.mScale.zero();
        collapsed_axis_binder.calcAndSetBaseMtx();
        collapsed_axis_binder.initBinder(0.5F, 2.0F, 1U);
        smgpc::compat::integrate_live_actor_velocity(collapsed_axis_binder);
        require(collapsed_axis_binder.mBindedWall && collapsed_axis_binder.mPosition.x > 3.0F,
                "a collapsed host scale should reconstruct Binder's rotated scale-free Y basis");
        collapsed_axis_collision.deactivate();
        require(smgpc::scene::StageCollisionService::active() == nullptr,
                "stage teardown should clear the active collision boundary");
    }

    void test_original_rail_part_geometry() {
        auto linear = RailPart{};
        linear.init(TVec3f{0.0F, 0.0F, 0.0F}, TVec3f{0.0F, 0.0F, 0.0F}, TVec3f{10.0F, 0.0F, 0.0F},
                    TVec3f{10.0F, 0.0F, 0.0F});

        auto position = TVec3f{};
        linear.calcPos(&position, 0.25F);
        require(position.epsilonEquals(TVec3f{2.5F, 0.0F, 0.0F}, 0.00001F) && std::fabs(linear.getTotalLength() - 10.0F) < 0.00001F,
                "linear rail parts should preserve original position and arc-length behavior");
        require(std::fabs(linear.getParam(5.0F) - 0.5F) < 0.00001F &&
                    std::fabs(linear.getNearestParam(TVec3f{7.0F, 4.0F, 0.0F}, 0.1F) - 0.7F) < 0.00001F,
                "linear rail coordinates should map consistently to parameters and nearest points");

        auto bezier = RailPart{};
        bezier.initForBezier(TVec3f{0.0F, 0.0F, 0.0F}, TVec3f{0.0F, 10.0F, 0.0F}, TVec3f{10.0F, 10.0F, 0.0F},
                             TVec3f{10.0F, 0.0F, 0.0F});
        bezier.calcPos(&position, 0.5F);
        require(position.epsilonEquals(TVec3f{5.0F, 7.5F, 0.0F}, 0.00001F) && bezier.getTotalLength() > 10.0F,
                "Bezier rail parts should preserve original cubic evaluation and positive arc length");

        auto placement = make_fieldless_jmap(1U);
        placement.setRailInfo(0, make_open_rail_path_info(), make_linear_rail_point_info(), 0);
        auto actor = LiveActor("rail-test");
        actor.initRailRider(JMapInfoIter(&placement, 0));
        require(actor.mRailRider != nullptr && std::fabs(MR::getRailTotalLength(&actor) - 20.0F) < 0.00001F &&
                    MR::getRailPos(&actor).epsilonEquals(TVec3f{0.0F, 0.0F, 0.0F}, 0.00001F),
                "LiveActor rail ownership should consume attached CommonPath metadata and start at the first point");

        MR::setRailCoordSpeed(&actor, 5.0F);
        MR::moveRailRider(&actor);
        require(MR::getRailPos(&actor).epsilonEquals(TVec3f{5.0F, 0.0F, 0.0F}, 0.00001F),
                "RailRider movement should advance by source-compatible world-space arc length");
        MR::moveCoordToNearestPos(&actor, TVec3f{13.0F, 4.0F, 0.0F});
        require(MR::getRailPos(&actor).epsilonEquals(TVec3f{13.0F, 0.0F, 0.0F}, 0.00001F),
                "RailRider nearest-position projection should select the correct path part");
    }

    void test_fixed_position_and_parts_model_surface() {
        auto scheduler = smgpc::runtime::SceneScheduler{};
        const auto scheduler_binding = smgpc::runtime::SceneSchedulerBinding{scheduler};
        auto host = LiveActor("fixed-position-host");
        host.mPosition.set(10.0F, 20.0F, 30.0F);
        host.mRotation.set(0.0F, 90.0F, 0.0F);
        host.mScale.set(2.0F, 3.0F, 4.0F);
        host.calcAndSetBaseMtx();
        host.makeActorAppeared();

        auto fixed = FixedPosition(&host, TVec3f{1.0F, 2.0F, 3.0F}, TVec3f{0.0F, 0.0F, 0.0F});
        fixed.calc();
        auto world = TVec3f{};
        fixed.copyTrans(&world);
        require(world.epsilonEquals(TVec3f{22.0F, 26.0F, 28.0F}, 0.00001F),
                "FixedPosition should apply the host TRS matrix to its local translation");

        auto axisX = TVec3f{};
        auto axisY = TVec3f{};
        auto axisZ = TVec3f{};
        fixed.mMtx.getXDir(axisX);
        fixed.mMtx.getYDir(axisY);
        fixed.mMtx.getZDir(axisZ);
        require(std::fabs(axisX.length() - 1.0F) < 0.00001F && std::fabs(axisY.length() - 1.0F) < 0.00001F &&
                    std::fabs(axisZ.length() - 1.0F) < 0.00001F,
                "FixedPosition should remove inherited host scale by default");

        host.mPosition.set(100.0F, 200.0F, 300.0F);
        host.mRotation.zero();
        host.mScale.set(1.0F);
        host.calcAndSetBaseMtx();
        fixed.setLocalTrans(TVec3f{4.0F, 5.0F, 6.0F});
        fixed.calc();
        fixed.copyTrans(&world);
        require(world.epsilonEquals(TVec3f{104.0F, 205.0F, 306.0F}, 0.00001F),
                "FixedPosition should retain a live reference to the host base matrix");

        auto* part = MR::createPartsModelNoSilhouettedMapObj(&host, "fixed-position-part", "", nullptr);
        scheduler.connect_name_obj(*part, -1, -1, -1, 7);
        part->initFixedPosition(TVec3f{0.0F, 70.0F, 0.0F}, TVec3f{0.0F, 0.0F, 0.0F}, nullptr);
        part->calcAnim();
        require(part->mPosition.epsilonEquals(TVec3f{100.0F, 270.0F, 300.0F}, 0.00001F),
                "PartsModel should expose Coin's fixed-position surface and update its actor position");

        host.mPosition.set(110.0F, 220.0F, 330.0F);
        host.calcAndSetBaseMtx();
        part->calcAnim();
        require(part->mPosition.epsilonEquals(TVec3f{110.0F, 290.0F, 330.0F}, 0.00001F),
                "PartsModel should continue following host motion after construction");

        MR::hideModel(&host);
        part->movement();
        require(part->mIsDead && !scheduler.is_draw_connected(*part) && !MR::isClipped(part),
                "PartsModel should leave only the draw path while its host is hidden");
        MR::showModel(&host);
        part->movement();
        require(!part->mIsDead && scheduler.is_draw_connected(*part) && !MR::isClipped(part),
                "PartsModel should reconnect to draw without changing clipping when its host returns");

        auto* ownedFixedPosition = part->mFixedPos;
        part->mFixedPos = nullptr;
        delete ownedFixedPosition;
        delete part;
    }

    void test_coin_math_and_gravity_surface() {
        auto resized = TVec3f{3.0F, 4.0F, 0.0F};
        const auto oldLength = resized.setLength(10.0F);
        require(std::fabs(oldLength - 5.0F) < 0.00001F && resized.epsilonEquals(TVec3f{6.0F, 8.0F, 0.0F}, 0.00001F),
                "TVec3f::setLength should return the old length and preserve direction");

        auto zeroLength = TVec3f{};
        require(zeroLength.setLength(10.0F) == 0.0F && zeroLength.epsilonEquals(TVec3f{}, 0.0F),
                "TVec3f::setLength should leave a zero vector stable");

        auto normalized = TVec3f{};
        require(MR::normalizeOrZero(&normalized) && normalized.epsilonEquals(TVec3f{}, 0.0F),
                "normalizeOrZero should report and preserve the zero case");
        require(!MR::normalizeOrZero(TVec3f{0.0F, 3.0F, 4.0F}, &normalized) &&
                    normalized.epsilonEquals(TVec3f{0.0F, 0.6F, 0.8F}, 0.00001F),
                "normalizeOrZero should produce a unit vector for a nonzero source");

        auto verticalAxis = TVec3f{};
        MR::makeAxisVerticalZX(&verticalAxis, TVec3f{0.0F, -1.0F, 0.0F});
        require(verticalAxis.epsilonEquals(TVec3f{0.0F, 0.0F, 1.0F}, 0.00001F),
                "makeAxisVerticalZX should prefer projected world Z");
        MR::makeAxisVerticalZX(&verticalAxis, TVec3f{0.0F, 0.0F, 1.0F});
        require(verticalAxis.epsilonEquals(TVec3f{1.0F, 0.0F, 0.0F}, 0.00001F),
                "makeAxisVerticalZX should fall back to projected world X for a parallel Z axis");

        auto rotated = TVec3f{};
        MR::rotateVecDegree(&rotated, TVec3f{1.0F, 0.0F, 0.0F}, TVec3f{0.0F, 1.0F, 0.0F}, 90.0F);
        require(rotated.epsilonEquals(TVec3f{0.0F, 0.0F, -1.0F}, 0.00001F),
                "rotateVecDegree should use the original right-handed axis rotation");

        auto rebound = TVec3f{4.0F, -10.0F, 2.0F};
        require(MR::calcReboundVelocity(&rebound, TVec3f{0.0F, 1.0F, 0.0F}, 0.6F, 0.5F) &&
                    rebound.epsilonEquals(TVec3f{2.0F, 6.0F, 1.0F}, 0.00001F),
                "calcReboundVelocity should independently scale normal and tangent velocity");
        auto simpleRebound = TVec3f{0.0F, -10.0F, 0.0F};
        require(MR::calcReboundVelocity(&simpleRebound, TVec3f{0.0F, 1.0F, 0.0F}, 0.6F) &&
                    simpleRebound.epsilonEquals(TVec3f{0.0F, 6.0F, 0.0F}, 0.00001F),
                "the simple rebound overload should apply restitution along the contact normal");
        const auto separating = rebound;
        require(!MR::calcReboundVelocity(&rebound, TVec3f{0.0F, 1.0F, 0.0F}, 0.6F) &&
                    rebound.epsilonEquals(separating, 0.0F),
                "calcReboundVelocity should not alter velocity already leaving the surface");

        const auto randomBase = TVec3f{10.0F, -5.0F, 3.0F};
        auto randomized = TVec3f{};
        MR::addRandomVector(&randomized, randomBase, 0.0F);
        require(randomized.epsilonEquals(randomBase, 0.0F), "addRandomVector should preserve its base when the range is zero");
        MR::addRandomVector(&randomized, randomBase, 2.0F);
        const auto randomDelta = randomized - randomBase;
        require(randomDelta.x >= -2.0F && randomDelta.x < 2.0F && randomDelta.y >= -2.0F && randomDelta.y < 2.0F &&
                    randomDelta.z >= -2.0F && randomDelta.z < 2.0F,
                "addRandomVector should keep each perturbation in the original half-open range");

        auto actor = LiveActor("coin-gravity-test");
        actor.mGravity.set(0.0F, -4.0F, 0.0F);
        auto gravity = TVec3f{9.0F, 9.0F, 9.0F};
        require_logic_error([&] { (void)MR::calcGravityVector(&actor, &gravity, nullptr, nullptr); },
                            "a missing scene-owned gravity system must be explicitly unavailable");
        require(gravity.epsilonEquals(TVec3f{9.0F, 9.0F, 9.0F}, 0.0F),
                "an unavailable gravity system must not invent or overwrite a destination vector");

        auto plainObject = NameObj("positional-gravity-test");
        gravity.set(9.0F, 9.0F, 9.0F);
        require_logic_error(
            [&] { (void)MR::calcGravityVector(&plainObject, TVec3f{1.0F, 2.0F, 3.0F}, &gravity, nullptr, nullptr); },
            "a positional query must reject absence of the scene-owned gravity system");
        require(gravity.epsilonEquals(TVec3f{9.0F, 9.0F, 9.0F}, 0.0F),
                "an unavailable positional query must leave its destination untouched");

        auto gravity_holder = SceneObjHolder{};
        const auto gravity_binding =
            smgpc::scene::SceneObjHolderBinding(gravity_holder);
        require(MR::createSceneObj(SceneObj_PlanetGravityManager) != nullptr,
                "the grounded fallback test requires the exact scene-owned gravity manager");
        actor.mGravity.zero();
        actor.mBindedGround = true;
        actor.mGroundNormal.set(0.0F, 2.0F, 0.0F);
        MR::calcGravityOrZero(&actor);
        require(actor.mGravity.epsilonEquals(TVec3f{0.0F, -1.0F, 0.0F}, 0.00001F),
                "calcGravityOrZero should retain the original grounded-normal fallback");
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

    void test_spine_uses_retail_two_phase_transition() {
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
        require(spine.getCurrentNerve() == &second,
                "the retail post-execution phase should commit the queued nerve in the same update");
        require(spine.mStep == 0, "committing a queued nerve should reset its step before the next update");

        spine.update();
        require(state.second_executions == 1, "queued nerve should execute on the next update");
        require(spine.getCurrentNerve() == &second, "executed nerve should become current after its first tick");
        require(spine.mStep == 1, "the second nerve should advance exactly once after its first execution");
    }

    class ActorStateProbe final : public ActorStateBaseInterface {
    public:
        explicit ActorStateProbe(const char* name) : ActorStateBaseInterface(name) {
            mIsDead = true;
        }

        void appear() override {
            ActorStateBaseInterface::appear();
            ++appear_count;
        }

        void kill() override {
            ActorStateBaseInterface::kill();
            ++kill_count;
        }

        void control() override {
            ++control_count;
        }

        int appear_count = 0;
        int kill_count = 0;
        int control_count = 0;
    };

    void test_actor_state_keeper_uses_exact_state_lifecycle() {
        const auto first_nerve = SpineProbeFirstNerve{};
        const auto second_nerve = SpineProbeSecondNerve{};
        auto first = ActorStateProbe{"first"};
        auto second = ActorStateProbe{"second"};
        auto keeper = ActorStateKeeper{2};

        keeper.addState(&first, &first_nerve, "first");
        keeper.addState(&second, &second_nerve, "second");
        require(!keeper.updateCurrentState(), "an exact state keeper starts without an active state");

        keeper.startState(&first_nerve);
        require(!first.mIsDead && first.appear_count == 1,
                "starting a registered state should invoke its real appear lifecycle once");
        require(!keeper.updateCurrentState() && first.control_count == 1,
                "updating an active real state should execute its control path");

        keeper.endState(&first_nerve);
        require(first.mIsDead && first.kill_count == 1,
                "ending a live registered state should invoke its real kill lifecycle once");
        keeper.startState(&second_nerve);
        require(!second.mIsDead && second.appear_count == 1,
                "the next registered state should own the active lifecycle");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };
}  // namespace

int main() {
    const auto tests = std::array{
        TestCase{"revolution headers and input defaults", test_revolution_headers_and_input_defaults},
        TestCase{"GamePad compatibility requires runtime context", test_game_pad_compat_requires_runtime_context},
        TestCase{"Aurora WPAD sub-stick edges", test_aurora_wpad_sub_stick_edges},
        TestCase{"JGeometry host layout and math", test_jgeometry_host_layout_and_math},
        TestCase{"Aurora VI retrace/framebuffer state", test_aurora_vi_retrace_and_framebuffer_state},
        TestCase{"Aurora DVD requires disc image", test_aurora_dvd_requires_disc_image},
        TestCase{"Aurora OS cache and GX copy smoke", test_aurora_os_cache_and_gx_copy_smoke},
        TestCase{"GX IA8 channel order", test_gx_ia8_channel_order},
        TestCase{"Aurora NAND storage smoke", test_aurora_nand_storage_smoke},
        TestCase{"player visibility can be restored", test_player_visibility_can_be_restored},
        TestCase{"scene scheduler registration scope cleanup", test_scene_scheduler_registration_scope_cleanup},
        TestCase{"stage switch zone identity and edges", test_stage_switch_zone_identity_and_edges},
        TestCase{"CollisionBlocker sensor lifecycle", test_collision_blocker_sensor_lifecycle},
        TestCase{"SimpleEffectObj host compatibility", test_simple_effect_host_compatibility},
        TestCase{"rail info ownership and per-entry lookup", test_rail_info_ownership_and_per_entry_lookup},
        TestCase{"DemoRabbit factory is absent", test_demo_rabbit_factory_is_absent},
        TestCase{"demo cast requires scene definition", test_demo_cast_requires_scene_definition},
        TestCase{"story-event spin entitlement boundary", test_story_event_spin_entitlement_boundary},
        TestCase{"StarPieceGroup factory absent without real director",
                 test_star_piece_group_factory_is_absent_without_real_director},
        TestCase{"stage host preserves placement appearance state", test_stage_host_preserves_placement_appearance_state},
        TestCase{"KCL collision queries and binder resolution", test_kcl_collision_service_queries_and_binder_resolution},
        TestCase{"original rail part geometry", test_original_rail_part_geometry},
        TestCase{"FixedPosition and PartsModel surface", test_fixed_position_and_parts_model_surface},
        TestCase{"Coin math and gravity surface", test_coin_math_and_gravity_surface},
        TestCase{"Spine retail two-phase transition", test_spine_uses_retail_two_phase_transition},
        TestCase{"ActorStateKeeper exact lifecycle", test_actor_state_keeper_uses_exact_state_lifecycle},
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
