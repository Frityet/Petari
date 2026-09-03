#include "CameraTargetTestSupport.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Camera/CameraTargetMtx.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Scene/PlacementStateChecker.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "camera/CameraAnimation.hpp"
#include "camera/CameraParam.hpp"
#include "camera/EventCamera.hpp"
#include "camera/OriginalGameCamera.hpp"
#include "camera/StageStartCamera.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/CameraUtilCompat.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "resource/BcsvTable.hpp"
#include "runtime/RuntimeServices.hpp"
#include "runtime/SceneScheduler.hpp"
#include "scene/StageHostService.hpp"
#include "scene/StagePlacementResolver.hpp"
#include "scene/SceneTransitionRequestService.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void require_near(float actual, float expected, float tolerance, std::string_view message) {
        if (std::fabs(actual - expected) > tolerance) {
            throw std::runtime_error(std::string(message) + ": actual=" + std::to_string(actual) +
                                     ";expected=" + std::to_string(expected));
        }
    }

    class CameraPlayerFixture final : public LiveActor {
    public:
        explicit CameraPlayerFixture(smgpc::runtime::PlayerSystemService &player)
            : LiveActor("Stage camera player fixture"), _player(player) {
            _player.attach_actor(*this);
            _player.set_camera_target(std::make_unique<smgpc::tests::CameraTargetFixture>([this] {
                return camera_state;
            }));
            _player.advance_camera_target(0U);
        }
        ~CameraPlayerFixture() override { _player.detach_actor(this); }

        smgpc::camera::StageCameraTargetState camera_state{
            .ground_position = smgpc::camera::CameraParamVec3{},
            .gravity = smgpc::camera::CameraParamVec3{0.0F, -1.0F, 0.0F}};

    private:
        smgpc::runtime::PlayerSystemService &_player;
    };

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

    void write_bcsv_field(std::vector<std::uint8_t> &bytes, std::size_t index, std::string_view name,
                          std::uint16_t offset, smgpc::resource::BcsvFieldType type) {
        const auto field_offset = 0x10U + index * 0x0cU;
        write_be32(bytes, field_offset, smgpc::resource::jmap_hash(name));
        write_be32(bytes, field_offset + 0x04U, 0xffffffffU);
        write_be16(bytes, field_offset + 0x08U, offset);
        bytes[field_offset + 0x0aU] = 0U;
        bytes[field_offset + 0x0bU] = static_cast<std::uint8_t>(type);
    }

    void write_inline_string(std::vector<std::uint8_t> &bytes, std::size_t offset, std::string_view value) {
        for (auto index = std::size_t{}; index < value.size(); ++index) {
            bytes[offset + index] = static_cast<std::uint8_t>(value[index]);
        }
        bytes[offset + value.size()] = 0U;
    }

    JMapInfo make_start_info(s32 mario_no, s32 camera_id, const std::array<float, 3U> &position,
                             const std::array<float, 3U> &rotation,
                             std::string_view object_name = "Mario",
                             std::string_view object_type = {}) {
        auto names = std::vector<std::string_view>{
            "MarioNo", "Camera_id", "pos_x", "pos_y", "pos_z", "dir_x", "dir_y", "dir_z", "name",
        };
        if (!object_type.empty()) {
            names.push_back("type");
        }
        const auto field_count = names.size();
        constexpr auto cEntrySize = 80U;
        const auto data_offset = 0x10U + field_count * 0x0cU;
        auto bytes = std::vector<std::uint8_t>(data_offset + cEntrySize, 0U);
        write_be32(bytes, 0x00U, 1U);
        write_be32(bytes, 0x04U, static_cast<std::uint32_t>(field_count));
        write_be32(bytes, 0x08U, static_cast<std::uint32_t>(data_offset));
        write_be32(bytes, 0x0cU, cEntrySize);
        for (auto field = std::size_t{}; field < field_count; ++field) {
            const auto field_type = field < 2U ? smgpc::resource::BcsvFieldType::Int32 :
                                    field < 8U ? smgpc::resource::BcsvFieldType::Float :
                                                 smgpc::resource::BcsvFieldType::InlineString;
            const auto field_offset = field == 9U ? 48U : field * 4U;
            write_bcsv_field(bytes, field, names[field], static_cast<std::uint16_t>(field_offset),
                             field_type);
        }
        write_be32(bytes, data_offset + 0U, static_cast<std::uint32_t>(mario_no));
        write_be32(bytes, data_offset + 4U, static_cast<std::uint32_t>(camera_id));
        for (auto axis = std::size_t{}; axis < 3U; ++axis) {
            write_be_float(bytes, data_offset + 8U + axis * 4U, position[axis]);
            write_be_float(bytes, data_offset + 20U + axis * 4U, rotation[axis]);
        }
        write_inline_string(bytes, data_offset + 32U, object_name);
        if (!object_type.empty()) {
            write_inline_string(bytes, data_offset + 48U, object_type);
        }
        return JMapInfo::from_bcsv(bytes);
    }

    smgpc::resource::BcsvTable make_camera_migration_table() {
        constexpr auto cFieldCount = 4U;
        constexpr auto cEntryCount = 2U;
        constexpr auto cEntrySize = 64U;
        constexpr auto cDataOffset = 0x10U + cFieldCount * 0x0cU;
        auto bytes = std::vector<std::uint8_t>(cDataOffset + cEntryCount * cEntrySize, 0U);
        write_be32(bytes, 0x00U, cEntryCount);
        write_be32(bytes, 0x04U, cFieldCount);
        write_be32(bytes, 0x08U, cDataOffset);
        write_be32(bytes, 0x0cU, cEntrySize);
        write_bcsv_field(bytes, 0U, "version", 0U, smgpc::resource::BcsvFieldType::UInt32);
        write_bcsv_field(bytes, 1U, "num1", 4U, smgpc::resource::BcsvFieldType::Int32);
        write_bcsv_field(bytes, 2U, "id", 8U, smgpc::resource::BcsvFieldType::InlineString);
        write_bcsv_field(bytes, 3U, "camtype", 16U, smgpc::resource::BcsvFieldType::InlineString);
        for (auto row = std::size_t{}; row < cEntryCount; ++row) {
            const auto entry = cDataOffset + row * cEntrySize;
            write_be32(bytes, entry, row == 0U ? 0x30015U : 0x30016U);
            write_be32(bytes, entry + 4U, 1U);
            write_inline_string(bytes, entry + 8U, row == 0U ? "s:0001" : "s:0002");
            write_inline_string(bytes, entry + 16U, "CAM_TYPE_XZ_PARA");
        }
        return smgpc::resource::BcsvTable::from_bytes(bytes);
    }

    [[nodiscard]] std::vector<std::uint8_t> make_guide_animation() {
        constexpr auto cHeaderSize = std::size_t{0x20U};
        constexpr auto cComponentTableSize = std::size_t{8U * 8U};
        constexpr auto cValueOffset = cHeaderSize + cComponentTableSize;
        constexpr auto cValueCount = std::size_t{16U};
        auto bytes = std::vector<std::uint8_t>(
            cValueOffset + 4U + cValueCount * sizeof(float), 0U);
        std::copy_n("ANDO", 4U, bytes.begin());
        std::copy_n("CANM", 4U, bytes.begin() + 4U);
        write_be32(bytes, 0x08U, 1U);
        write_be32(bytes, 0x10U, 1U);
        write_be32(bytes, 0x18U, 1U);
        write_be32(bytes, 0x1cU, cComponentTableSize);
        for (auto component = std::size_t{}; component < 8U; ++component) {
            const auto offset = cHeaderSize + component * 8U;
            write_be32(bytes, offset, 2U);
            write_be32(bytes, offset + 4U,
                       static_cast<std::uint32_t>(component * 2U));
        }
        write_be32(bytes, cValueOffset,
                   static_cast<std::uint32_t>(cValueCount * sizeof(float)));
        const auto values = std::array<float, cValueCount>{
            0.0F, 10.0F, 100.0F, 100.0F, 500.0F, 500.0F,
            0.0F, 10.0F, 100.0F, 100.0F, 0.0F,   0.0F,
            0.0F, 0.0F,  45.0F,  45.0F,
        };
        for (auto index = std::size_t{}; index < values.size(); ++index) {
            write_be_float(bytes, cValueOffset + 4U + index * 4U,
                           values[index]);
        }
        return bytes;
    }

    [[nodiscard]] smgpc::camera::ResolvedStageStartCamera
    make_resolved_start_camera(float marker) {
        auto result = smgpc::camera::ResolvedStageStartCamera{};
        result.start_info.camera_id = static_cast<s32>(marker);
        result.camera_key = "synthetic:" + std::to_string(marker);
        result.camera_param.camera_type = "CAM_TYPE_XZ_PARA";
        result.calculation.pose = smgpc::camera::CameraPose{
            .eye = {marker, 100.0F, 200.0F},
            .watch = {marker + 10.0F, 100.0F, 200.0F},
            .up = {0.0F, 1.0F, 0.0F},
        };
        return result;
    }

    [[nodiscard]] bool same_pose(const smgpc::camera::CameraPose &lhs,
                                 const smgpc::camera::CameraPose &rhs) {
        return lhs.eye.x == rhs.eye.x && lhs.eye.y == rhs.eye.y &&
               lhs.eye.z == rhs.eye.z && lhs.watch.x == rhs.watch.x &&
               lhs.watch.y == rhs.watch.y && lhs.watch.z == rhs.watch.z &&
               lhs.up.x == rhs.up.x && lhs.up.y == rhs.up.y &&
               lhs.up.z == rhs.up.z &&
               lhs.fovy_degrees == rhs.fovy_degrees &&
               lhs.aspect_ratio == rhs.aspect_ratio &&
               lhs.near_clip == rhs.near_clip &&
               lhs.far_clip == rhs.far_clip &&
               lhs.projection_offset_x == rhs.projection_offset_x &&
               lhs.projection_offset_y == rhs.projection_offset_y;
    }

    [[nodiscard]] smgpc::camera::ResolvedStageStartCamera
    make_tracking_camera() {
        auto result = make_resolved_start_camera(1.0F);
        result.camera_param.general.dist = 600.0F;
        result.camera_param.general.angle_a = 0.4F;
        result.camera_param.general.angle_b = 0.25F;
        result.camera_param.extra.w_offset = {};
        result.camera_param.extra.l_offset = 100.0F;
        result.camera_param.extra.l_offset_v = 60.0F;
        result.camera_param.extra.fovy = 63.0F;
        result.camera_param.extra.flags = 1U << 1U;
        result.camera_param.extra.v_pan_use = 0;
        result.target.position = {10.0F, 20.0F, 30.0F};
        const auto initial = smgpc::camera::calculate_stage_camera_pose(
            result.start_info.zone_transform, result.camera_param,
            result.target);
        require(initial.has_value(), "the authored tracking fixture must resolve");
        result.calculation = *initial;
        result.calculation.pose.aspect_ratio = 16.0F / 9.0F;
        result.calculation.pose.near_clip = 50.0F;
        result.calculation.pose.far_clip = 200000.0F;
        result.calculation.pose.projection_offset_x = 0.01F;
        result.calculation.pose.projection_offset_y = -0.02F;
        return result;
    }

    void test_rigid_zone_matrix_and_start_selection() {
        const auto parent = smgpc::scene::StageZoneTransform::from_translation_rotation({10.0F, 20.0F, 30.0F},
                                                                                        {0.0F, 0.0F, 90.0F});
        const auto local = smgpc::scene::StageZoneTransform::from_translation_rotation({2.0F, 3.0F, 4.0F},
                                                                                       {15.0F, 30.0F, 45.0F});
        const auto combined = parent.concatenated(local);
        const auto point = std::array<float, 3U>{5.0F, 6.0F, 7.0F};
        const auto sequential = parent.transform_point(local.transform_point(point));
        const auto direct = combined.transform_point(point);
        for (auto axis = std::size_t{}; axis < 3U; ++axis) {
            require_near(direct[axis], sequential[axis], 0.0001F,
                         "zone composition must multiply rigid matrices instead of adding Euler angles");
        }

        const auto child_transform = smgpc::scene::StageZoneTransform::from_translation_rotation(
            {10.0F, 20.0F, 30.0F}, {0.0F, 90.0F, 0.0F});
        auto tables = std::vector<smgpc::scene::StagePlacementTable>{};
        tables.push_back({
            .stage_name = "RootGalaxy",
            .zone_name = "RootGalaxy",
            .category = "start",
            .layer_name = "common",
            .table_name = "startinfo",
            .archive_path = "/StageData/RootGalaxy.arc",
            .table_path = "jmp/start/common/startinfo",
            .jmap_info = make_start_info(0, 11, {}, {}),
            .zone_id = 0,
            .layer_id = 0,
            .archive_entry_order = 1U,
        });
        tables.push_back({
            .stage_name = "ChildZone",
            .zone_name = "ChildZone",
            .category = "start",
            .layer_name = "common",
            .table_name = "startinfo",
            .archive_path = "/StageData/ChildZone.arc",
            .table_path = "jmp/start/common/startinfo",
            .jmap_info = make_start_info(1, 22, {}, {}),
            .zone_id = 5,
            .layer_id = 0,
            .archive_entry_order = 2U,
            .zone_transform = child_transform,
        });
        tables.push_back({
            .stage_name = "ChildZone",
            .zone_name = "ChildZone",
            .category = "start",
            .layer_name = "layera",
            .table_name = "startinfo",
            .archive_path = "/StageData/ChildZone.arc",
            .table_path = "jmp/start/layera/startinfo",
            .jmap_info = make_start_info(0, 78, {1.0F, 2.0F, 3.0F}, {}),
            .zone_id = 5,
            .layer_id = 1,
            .archive_entry_order = 3U,
            .zone_transform = child_transform,
        });

        auto selected = smgpc::scene::select_stage_start_info(tables, 0, 5);
        require(selected.has_value(), "selection should find the requested MarioNo in the requested zone");
        require(selected->object_name == "Mario" && selected->camera_id == 78 &&
                    selected->layer_name == "layera" &&
                    selected->archive_path == "/StageData/ChildZone.arc",
                "selection should preserve the actor, layer order, and selected child-zone archive");
        tables.clear();
        const auto selected_iter = selected->iter();
        const char *selected_name = nullptr;
        auto selected_mario_no = s32{-1};
        require(selected_iter.isValid() && selected_iter.getValue("name", &selected_name) &&
                    selected_name != nullptr && std::string_view(selected_name) == "Mario" &&
                    selected_iter.getValue("MarioNo", &selected_mario_no) && selected_mario_no == 0 &&
                    std::string_view(selected_iter.mInfo->getName()) == "startinfo" &&
                    selected_iter.mInfo->getPlacedZoneId() == 5,
                "selected StartInfo must own its exact JMap row and zone metadata after source tables die");
        require_near(selected->world_position[0], 13.0F, 0.0001F, "child-zone transform should rotate StartInfo X");
        require_near(selected->world_position[1], 22.0F, 0.0001F, "child-zone transform should preserve StartInfo Y");
        require_near(selected->world_position[2], 29.0F, 0.0001F, "child-zone transform should rotate StartInfo Z");
        require_near(selected->world_front[0], 1.0F, 0.0001F, "child-zone orientation should rotate StartInfo front");
        require_near(selected->world_up[1], 1.0F, 0.0001F, "child-zone orientation should preserve StartInfo up");
        auto iter_position = std::array<float, 3U>{};
        require(selected_iter.getValue("pos_x", &iter_position[0]) &&
                    selected_iter.getValue("pos_y", &iter_position[1]) &&
                    selected_iter.getValue("pos_z", &iter_position[2]),
                "the retained StartInfo row must expose its transformed position to exact Game init");
        for (auto axis = std::size_t{}; axis < 3U; ++axis) {
            require_near(iter_position[axis], selected->world_position[axis], 0.0001F,
                         "the retained JMap row and derived world position must agree");
        }

        auto typed_tables = std::vector<smgpc::scene::StagePlacementTable>{};
        typed_tables.push_back({
            .stage_name = "RootGalaxy",
            .zone_name = "RootGalaxy",
            .category = "start",
            .layer_name = "common",
            .table_name = "startinfo",
            .archive_path = "/StageData/RootGalaxy.arc",
            .table_path = "jmp/start/common/startinfo",
            .jmap_info = make_start_info(0, 11, {}, {}, "Mario", "MarioTypeOverride"),
            .zone_id = 0,
            .layer_id = 0,
        });
        const auto typed = smgpc::scene::select_stage_start_info(typed_tables, 0, 0);
        require(typed.has_value() && typed->object_name == "MarioTypeOverride",
                "StartInfo creator selection must prefer retail type over name");
    }

    void test_camera_version_key_and_xz_parallel_pose() {
        require(smgpc::camera::make_start_camera_key(78) == "s:004e", "start camera IDs should use lowercase four-digit hexadecimal");
        require(smgpc::camera::make_start_camera_key(-1) == "s:ffff", "negative camera IDs should retain the original u16 cast");

        const auto migrated = smgpc::camera::load_camera_param_chunks(make_camera_migration_table());
        require(migrated.size() == 2U && migrated[0].general.num1 == 0 && migrated[1].general.num1 == 1,
                "XZ_PARA num1 should migrate only camera versions older than 0x30016");

        auto chunk = smgpc::camera::CameraParamChunk{};
        chunk.version = 0x30016U;
        chunk.camera_type = "CAM_TYPE_XZ_PARA";
        chunk.general.angle_a = -0.3436119854450226F;
        chunk.general.angle_b = -0.3595139980316162F;
        chunk.general.dist = 1179.19921875F;
        chunk.extra.l_offset_v = 219.7265625F;
        chunk.extra.roll = 2.799999952316284F;
        chunk.extra.fovy = 60.0F;
        chunk.extra.w_offset = {};
        chunk.extra.flags = 1U << 2U;

        const auto target = smgpc::camera::StageCameraTargetState{
            .position = {14459.978515625F, -12791.11328125F, 6059.91162109375F},
            .up = {-0.0834537025F, -0.9381827940F, -0.3359293446F},
            .front = {0.9576820939F, -0.1686928920F, 0.2332117388F},
        };
        const auto calculated = smgpc::camera::calculate_stage_camera_pose({}, chunk, target);
        require(calculated.has_value(), "XZ_PARA should produce a camera pose");
        require_near(calculated->pose.watch.x, 14441.641520F, 0.5F, "XZ_PARA watch X should include the immediate local offset");
        require_near(calculated->pose.watch.y, -12997.256962F, 0.5F, "XZ_PARA watch Y should include the immediate local offset");
        require_near(calculated->pose.watch.z, 5986.099021F, 0.5F, "XZ_PARA watch Z should include the immediate local offset");
        require_near(calculated->pose.eye.x, 13402.355347F, 0.5F, "XZ_PARA eye X should match the decompiled polar calculation");
        require_near(calculated->pose.eye.y, -13412.122057F, 0.5F, "XZ_PARA eye Y should match the decompiled polar calculation");
        require_near(calculated->pose.eye.z, 5614.236143F, 0.5F, "XZ_PARA eye Z should match the decompiled polar calculation");
        require_near(calculated->pose.up.x, 0.424968546F, 0.001F, "roll should be folded into CameraPose up X");
        require_near(calculated->pose.up.y, -0.881984091F, 0.001F, "roll should be folded into CameraPose up Y");
        require_near(calculated->pose.up.z, -0.203729719F, 0.001F, "roll should be folded into CameraPose up Z");
        require_near(calculated->pose.fovy_degrees, 45.0F, 0.0001F,
                     "flag.nofovy clear should select the director default despite its name");

        chunk.extra.flags |= 1U << 1U;
        const auto explicit_fovy = smgpc::camera::calculate_stage_camera_pose({}, chunk, target);
        require(explicit_fovy.has_value() && explicit_fovy->pose.fovy_degrees == 60.0F,
                "flag.nofovy set should select the BCAM fovy despite its name");

        auto invalid_target = target;
        invalid_target.front = {};
        require(!smgpc::camera::calculate_stage_camera_pose({}, chunk, invalid_target).has_value(),
                "a degenerate target basis must remain unavailable instead of selecting a guessed direction");
    }

    void test_original_controller_height_and_lifetime() {
        const auto live_objects = smgpc::compat::name_obj_runtime_state_count();
        auto param = smgpc::camera::CameraParamChunk{};
        param.camera_type = "CAM_TYPE_XZ_PARA";
        param.general.dist = 600.0F;
        param.general.angle_a = 0.0F;
        param.general.angle_b = 0.0F;
        param.extra.w_offset = {};
        param.extra.v_pan_use = 1;
        param.extra.upper = 1.0F;
        param.extra.lower = 1.0F;
        auto target = smgpc::camera::StageCameraTargetState{};
        {
            auto controller = smgpc::camera::OriginalGameCamera({}, param, target);
            require(smgpc::compat::name_obj_runtime_state_count() > live_objects,
                    "the backend must own actual registered Game camera objects");
            target.position.y = 100.0F;
            target.last_move.y = 100.0F;
            const auto first = controller.calc(target);
            const auto second = controller.calc(target);
            require_near(first.pose.watch.y, 5.0F, 0.001F,
                         "original grounded HeightArrange must chase at its 0.05 rate");
            require_near(second.pose.watch.y, 9.75F, 0.001F,
                         "the second calculation must retain original HeightArrange history");

            controller.reset({});
            target.jumping = true;
            const auto airborne = controller.calc(target);
            require_near(airborne.pose.watch.y, 0.0F, 0.001F,
                         "original jump handling must retain the takeoff watch height");
            target.jumping = false;
            const auto landed = controller.calc(target);
            require_near(landed.pose.watch.y, 5.0F, 0.001F,
                         "landing must resume the original grounded height chase");

            param.extra.upper = 0.3F;
            param.extra.lower = 0.1F;
            auto slow_rise = smgpc::camera::OriginalGameCamera({}, param, {});
            auto fast_rise = smgpc::camera::OriginalGameCamera({}, param, {});
            target.position.y = 1000.0F;
            target.jumping = true;
            target.fast_rise = false;
            const auto slow_rise_pose = slow_rise.calc(target).pose;
            target.fast_rise = true;
            const auto fast_rise_pose = fast_rise.calc(target).pose;
            require(fast_rise_pose.watch.y > slow_rise_pose.watch.y + 100.0F,
                    "published fast-rise state must reach original HeightArrange rise-delay handling");

            auto slow_drop = smgpc::camera::OriginalGameCamera({}, param, {});
            auto fast_drop = smgpc::camera::OriginalGameCamera({}, param, {});
            target.position.y = -1000.0F;
            target.last_move.y = -100.0F;
            target.fast_rise = false;
            target.fast_drop = false;
            const auto slow_drop_pose = slow_drop.calc(target).pose;
            target.fast_drop = true;
            const auto fast_drop_pose = fast_drop.calc(target).pose;
            require(fast_drop_pose.watch.y < slow_drop_pose.watch.y - 100.0F,
                    "published fast-drop state must reach original HeightArrange drop-delay handling");
        }
        require(smgpc::compat::name_obj_runtime_state_count() == live_objects,
                "retiring original controllers must release their registered camera/height/target owners");

        param.extra.v_pan_use = 0;
        param.extra.flags = 1U << 2U;
        param.extra.l_offset = 10.0F;
        param.extra.l_offset_v = 20.0F;
        target = {};
        target.up = {0.0F, 2.0F, 0.0F};
        target.front = {0.0F, 0.0F, 3.0F};
        const auto calculation = smgpc::camera::calculate_stage_camera_pose({}, param, target);
        require(calculation.has_value(), "the original camera should accept the published basis");
        require_near(calculation->state.local_offset.y, 40.0F, 0.001F,
                     "the host boundary must not normalize the original target's up offset");
        require_near(calculation->state.local_offset.z, 30.0F, 0.001F,
                     "the host boundary must not normalize the original target's front offset");
        require(smgpc::compat::name_obj_runtime_state_count() == live_objects,
                "one-shot start calculation must also retire its original objects");
    }

    void test_original_manager_safe_basis_and_deferred_reset() {
        auto param = smgpc::camera::CameraParamChunk{};
        param.camera_type = "CAM_TYPE_XZ_PARA";
        param.general.dist = 600.0F;
        param.general.angle_b = 1.5707F;
        param.extra.w_offset = {};
        param.extra.v_pan_use = 0;
        auto target = smgpc::camera::StageCameraTargetState{};
        auto controller = smgpc::camera::OriginalGameCamera({}, param, target);
        const auto first = controller.calculation().pose;
        const auto dot = [](const auto &a, const auto &b) {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        };
        auto forward = smgpc::camera::CameraParamVec3{
            first.watch.x - first.eye.x, first.watch.y - first.eye.y,
            first.watch.z - first.eye.z};
        const auto length = std::sqrt(dot(forward, forward));
        forward = {forward.x / length, forward.y / length, forward.z / length};
        require_near(dot(first.up, forward), 0.0F, 0.001F,
                     "the original manager must repair raw up parallel to the view direction");
        require_near(dot(first.up, first.up), 1.0F, 0.001F,
                     "the original safe-pose step must publish a normalized non-degenerate basis");
        require(std::abs(first.up.z) > 0.99F,
                "near-vertical initialization must use the original manager's forward-Z baseline");
        target.front = {1.0F, 0.0F, 0.0F};
        controller.reset_manager(target);
        const auto reset_pose = controller.calc(target).pose;
        require(std::abs(reset_pose.up.x) > 0.99F,
                "explicit manager reset must use the original target-based pose seed for safe-up recovery");

        auto camera = smgpc::runtime::CameraSystemService{};
        const auto resolved = make_tracking_camera();
        const auto owner = camera.set_authored_game_camera(resolved);
        target = resolved.target;
        target.last_move = {15.0F, 0.0F, 0.0F};
        camera.set_game_camera_target(owner, target);
        camera.begin_frame(1U);
        const auto before_reset = *camera.game_camera_pose();
        camera.begin_frame(1U);
        require(same_pose(*camera.game_camera_pose(), before_reset),
                "a repeated camera movement phase must not accumulate the original manager's local offset twice");
        camera.pause_on_camera_director();
        camera.pause_on_camera_director();
        camera.reset_camera_man();
        camera.reset_camera_man();
        camera.begin_frame(2U);
        require(same_pose(*camera.game_camera_pose(), before_reset) &&
                    camera.camera_director_pause_count() == 1U,
                "repeated pause-on must be idempotent and a queued reset must not calculate while paused");
        camera.pause_off_camera_director();
        require(!camera.is_camera_director_paused(),
                "one pause-off must resume after repeated pause-on, as the original movement flag does");
        camera.begin_frame(3U);
        require_near(camera.game_camera_pose()->watch.z - target.position.z,
                     27.1F, 0.0001F,
                     "coalesced reset must retain offset history and run one reset followed by one calc");
        camera.begin_frame(4U);
        require_near(camera.game_camera_pose()->watch.z - target.position.z,
                     34.39F, 0.0001F,
                     "the deferred reset request must clear after its successful calculation");
        camera.clear_stage_start_camera(owner);
    }

    void test_base_programmable_camera_priority_and_request_defaults() {
        auto camera = smgpc::runtime::CameraSystemService{};
        auto base = smgpc::camera::CameraPose{};
        base.eye.x = 10.0F;
        camera.set_game_camera_pose(base);
        require(camera.effective_camera_pose().has_value() && camera.effective_camera_pose()->eye.x == 10.0F,
                "the persistent game camera should be effective without an event camera");

        camera.declare_event_camera_programmable("demo");
        camera.start_global_event_camera_no_target("demo");
        require(camera.effective_camera_pose().has_value() && camera.effective_camera_pose()->eye.x == 10.0F,
                "an event without a programmed pose should not erase the game camera");
        (void)camera.set_programmable_camera_param("demo", {1.0F, 2.0F, 3.0F}, {20.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, false);
        require(camera.effective_camera_pose().has_value() && camera.effective_camera_pose()->eye.x == 20.0F,
                "an active programmable event camera should override the game camera");
        camera.end_global_event_camera("demo");
        require(camera.effective_camera_pose().has_value() && camera.effective_camera_pose()->eye.x == 10.0F,
                "ending the event camera should reveal the persistent game camera again");
        camera.clear_game_camera_pose();
        require(!camera.effective_camera_pose().has_value(), "clearing a stage should release its game camera pose");

        const auto host_request = smgpc::scene::StageHostRequest{};
        require(host_request.start_id == 0 && host_request.start_zone_id == 0,
                "stage requests should preserve the original default start ID and root zone");
    }

    void test_runaway_tico_start_camera_handoff() {
        auto camera = smgpc::runtime::CameraSystemService{};
        const auto resolved = make_tracking_camera();
        const auto start_pose = resolved.calculation.pose;
        const auto owner_generation = camera.set_stage_start_camera(resolved);
        const auto *retained = camera.stage_start_camera();
        require(retained != nullptr && retained->camera_key == resolved.camera_key &&
                    camera.game_camera_pose().has_value() &&
                    same_pose(*camera.game_camera_pose(), start_pose),
                "stage camera ownership must retain the complete resolved camera and publish its pose");

        auto camera_override =
            smgpc::compat::ScopedCameraSystemServiceOverride(camera);
        require(!MR::isStartPosCameraEnd() &&
                    camera.start_position_camera_zero_interpolation_frames() ==
                        5U,
                "retail stage-camera creation must start active with the false-call five-calculation window");
        camera.begin_frame(0U);
        require(camera.start_position_camera_zero_interpolation_frames() == 5U,
                "a missing target must not consume a camera-calculation countdown step");
        camera.set_game_camera_target(owner_generation, resolved.target);
        camera.begin_frame(1U);
        require(camera.start_position_camera_zero_interpolation_frames() == 4U,
                "the false-call zero-interpolation window must count down once per camera frame");

        // RunawayTico Guide0 ends this camera before starting DemoMeetTico.
        MR::endStartPosCamera();
        require(MR::isStartPosCameraEnd() &&
                    camera.stage_start_camera() == retained &&
                    camera.game_camera_pose().has_value() &&
                    same_pose(*camera.game_camera_pose(), start_pose) &&
                    camera.start_position_camera_zero_interpolation_frames() ==
                        0U,
                "Guide0 end must clear the start flag/count while preserving the current game pose");
        MR::endStartPosCamera();
        require(MR::isStartPosCameraEnd() &&
                    camera.stage_start_camera() == retained,
                "retail start-camera end must be repeatable while ownership remains live");

        auto player = smgpc::runtime::PlayerSystemService{};
        auto camera_player = CameraPlayerFixture(player);
        camera_player.camera_state.position = {1000.0F, 0.0F, 0.0F};
        Mtx player_matrix{
            {1.0F, 0.0F, 0.0F, 1000.0F},
            {0.0F, 1.0F, 0.0F, 0.0F},
            {0.0F, 0.0F, 1.0F, 0.0F},
        };
        player.set_base_matrix(player_matrix);
        camera.declare_event_camera_animation(
            0, "DemoMeetTico",
            smgpc::camera::CameraAnimation::from_bytes(make_guide_animation()));
        camera.start_event_camera(
            0, "DemoMeetTico",
            smgpc::camera::EventCameraTarget::target_player(player), 0, 1.0F);
        const auto guide_pose = camera.effective_camera_pose();
        require(guide_pose.has_value() && same_pose(*guide_pose, start_pose) &&
                    camera.active_event_camera_key().has_value(),
                "DemoMeetTico must retain the visible pose until its first camera phase");

        MR::startStartPosCamera(false);
        require(camera.start_position_camera_zero_interpolation_frames() == 5U,
                "a false restart under an event camera must seed the retail countdown");
        camera.begin_frame(2U);
        require(camera.start_position_camera_zero_interpolation_frames() == 5U &&
                    camera.effective_camera_pose().has_value() &&
                    !same_pose(*camera.effective_camera_pose(), start_pose) &&
                    camera.active_event_camera_key().has_value() &&
                    camera.active_event_camera_key()->name == "DemoMeetTico",
                "the game-camera countdown must not advance while an event camera owns the effective pose");
        MR::endStartPosCamera();
        const auto guide_pose_before_restart = camera.effective_camera_pose();

        // RunawayTico Guide1 part 5 restores start-pos first, then ends CANM.
        MR::startStartPosCamera(true);
        require(!MR::isStartPosCameraEnd() &&
                    camera.stage_start_camera() == retained &&
                    camera.game_camera_pose().has_value() &&
                    same_pose(*camera.game_camera_pose(), start_pose) &&
                    camera.start_position_camera_zero_interpolation_frames() ==
                        0U &&
                    camera.effective_camera_pose().has_value() &&
                    guide_pose_before_restart.has_value() &&
                    same_pose(*camera.effective_camera_pose(),
                              *guide_pose_before_restart),
                "Guide1 true start must preserve the current game pose underneath the still-active event camera");
        MR::startStartPosCamera(true);
        require(camera.start_position_camera_zero_interpolation_frames() == 0U,
                "a repeated true start must retain the zero interpolation count");
        camera.end_event_camera(0, "DemoMeetTico", true, -1);
        require(camera.effective_camera_pose().has_value() &&
                    same_pose(*camera.effective_camera_pose(), *guide_pose_before_restart),
                "ending DemoMeetTico must retain the last rendered view until the original finish interpolation advances");
        const auto copied_game_pose = *camera.game_camera_pose();
        require_near(copied_game_pose.eye.x, guide_pose_before_restart->eye.x, 0.0001F,
                     "positive finish with resetView must seed the returning game manager from the latest OnlyCamera position");
        require_near(copied_game_pose.fovy_degrees, 40.0F, 0.0001F,
                     "the director copies OnlyCamera's untouched pose FOV before the game manager reapplies its authored FOV");
        require_near(copied_game_pose.aspect_ratio, start_pose.aspect_ratio, 0.0001F,
                     "the returning manager must keep the game scene's projection geometry");

        MR::startStartPosCamera(false);
        require(camera.start_position_camera_zero_interpolation_frames() == 5U,
                "a false restart must reproduce the exact five-calculation retail window");
        camera.begin_frame(3U);
        require(camera.start_position_camera_zero_interpolation_frames() == 4U,
                "a false restart must resume its countdown from five");
        MR::startStartPosCamera(false);
        require(camera.start_position_camera_zero_interpolation_frames() == 5U,
                "a repeated false restart must reseed the retail countdown");
        camera.clear_stage_start_camera(owner_generation);
    }

    void test_authored_game_camera_target_tracking() {
        auto camera = smgpc::runtime::CameraSystemService{};
        const auto resolved = make_tracking_camera();
        const auto initial = resolved.calculation.pose;
        const auto owner = camera.set_authored_game_camera(resolved);
        const auto *retained = camera.stage_start_camera();
        require(camera.is_start_position_camera_end() &&
                    camera.start_position_camera_zero_interpolation_frames() == 0U,
                "normal authored tracking must not enter start-position mode");
        camera.begin_frame(1U);
        require(same_pose(*camera.game_camera_pose(), initial),
                "an authored owner without a published target must retain its initial pose");

        auto target = resolved.target;
        target.position.x += 50.0F;
        target.position.y -= 20.0F;
        target.position.z += 80.0F;
        camera.set_game_camera_target(owner, target);
        require(same_pose(*camera.game_camera_pose(), initial),
                "target publication must not advance the camera before its frame");
        camera.begin_frame(2U);
        const auto tracked = *camera.game_camera_pose();
        require_near(tracked.eye.x, initial.eye.x + 50.0F, 0.0001F,
                     "authored eye X must track target translation");
        require_near(tracked.eye.y, initial.eye.y - 20.0F, 0.0001F,
                     "authored eye Y must track target translation");
        require_near(tracked.eye.z, initial.eye.z + 80.0F, 0.0001F,
                     "authored eye Z must track target translation");
        require_near(tracked.watch.x, initial.watch.x + 50.0F, 0.0001F,
                     "authored watch point must follow target translation");
        require(tracked.aspect_ratio == initial.aspect_ratio &&
                    tracked.near_clip == initial.near_clip &&
                    tracked.far_clip == initial.far_clip &&
                    tracked.projection_offset_x == initial.projection_offset_x &&
                    tracked.projection_offset_y == initial.projection_offset_y &&
                    tracked.fovy_degrees == 63.0F &&
                    camera.stage_start_camera() == retained &&
                    same_pose(retained->calculation.pose, initial) &&
                    retained->target.position.x == resolved.target.position.x,
                "tracking must retain authored projection and immutable start metadata");
        camera.set_game_camera_target(owner, std::nullopt);
        camera.begin_frame(3U);
        require(same_pose(*camera.game_camera_pose(), tracked),
                "withdrawing a target must freeze the last calculated authored pose");
        camera.clear_stage_start_camera(owner);
    }

    void test_authored_game_camera_offset_accumulation() {
        auto camera = smgpc::runtime::CameraSystemService{};
        const auto resolved = make_tracking_camera();
        const auto owner = camera.set_authored_game_camera(resolved);
        auto target = resolved.target;
        target.last_move = {15.0F, 0.0F, 0.0F};
        camera.set_game_camera_target(owner, target);
        camera.begin_frame(1U);
        require_near(camera.game_camera_pose()->watch.y - target.position.y,
                     6.0F, 0.0001F,
                     "retail offset interpolation must advance by speed * 0.1 / 15");
        require_near(camera.game_camera_pose()->watch.z - target.position.z,
                     10.0F, 0.0001F,
                     "retail front offset must use the same first-frame interpolation");
        camera.begin_frame(2U);
        require_near(camera.game_camera_pose()->watch.y - target.position.y,
                     11.4F, 0.0001F,
                     "vertical local offset must accumulate from its prior frame");
        require_near(camera.game_camera_pose()->watch.z - target.position.z,
                     19.0F, 0.0001F,
                     "front local offset must accumulate instead of resetting each frame");

        target.front = {1.0F, 0.0F, 0.0F};
        camera.set_game_camera_target(owner, target);
        camera.begin_frame(3U);
        const auto pose = *camera.game_camera_pose();
        require_near(pose.watch.x - target.position.x, 10.0F, 0.0001F,
                     "turning must converge toward the new target front");
        require_near(pose.watch.y - target.position.y, 16.26F, 0.0001F,
                     "turning must preserve accumulated vertical offset state");
        require_near(pose.watch.z - target.position.z, 17.1F, 0.0001F,
                     "turning must decay the old front offset from its retained value");
        camera.clear_stage_start_camera(owner);
    }

    void test_player_target_camera_phase_ownership() {
        auto camera = smgpc::runtime::CameraSystemService{};
        auto player = smgpc::runtime::PlayerSystemService{};
        auto actor = CameraPlayerFixture(player);
        auto event_player = smgpc::runtime::PlayerSystemService{};
        auto event_actor = CameraPlayerFixture(event_player);
        auto* target = static_cast<smgpc::tests::CameraTargetFixture*>(player.camera_target());
        auto* event_target = static_cast<smgpc::tests::CameraTargetFixture*>(event_player.camera_target());
        const auto owner = camera.set_authored_game_camera(make_tracking_camera());
        camera.set_game_camera_target_player(owner, player);
        require(target->movement_count == 1U,
                "binding the player target must not advance its original movement state");
        camera.begin_frame(1U);
        camera.begin_frame(1U);
        (void)player.camera_target_state();
        (void)player.camera_target_state();
        require(target->movement_count == 2U,
                "repeated frame requests and pose reads must advance the selected target only once");
        camera.pause_on_camera_director();
        camera.begin_frame(2U);
        require(target->movement_count == 2U,
                "pausing the director must also pause its target movement");
        camera.pause_off_camera_director();
        camera.begin_frame(3U);
        require(target->movement_count == 3U,
                "unpausing must resume the retained target without replaying skipped frames");

        camera.declare_event_camera_animation(
            0, "target-phase-event",
            smgpc::camera::CameraAnimation::from_bytes(make_guide_animation()));
        camera.start_event_camera(
            0, "target-phase-event",
            smgpc::camera::EventCameraTarget::target_player(event_player), 0);
        require(event_target->movement_count == 1U,
                "event requests must sample the retained target without moving it");
        camera.begin_frame(4U);
        require(target->movement_count == 3U && event_target->movement_count == 2U,
                "an event must advance its selected target and suspend the game target");
        camera.start_event_camera(
            0, "target-phase-event", smgpc::camera::EventCameraTarget::retain(), 0);
        camera.begin_frame(5U);
        require(target->movement_count == 3U && event_target->movement_count == 3U,
                "a no-target event request must retain the selected target and its state");
        camera.end_event_camera(0, "target-phase-event", true, -1);
        camera.begin_frame(6U);
        require(target->movement_count == 4U && event_target->movement_count == 3U,
                "ending an event must resume the retained game target");

        player.detach_actor(&actor);
        require(player.camera_target() == nullptr && !player.camera_target_state().has_value(),
                "actor retirement must withdraw its camera target before another camera phase");
        const auto last_pose = *camera.game_camera_pose();
        camera.begin_frame(7U);
        camera.begin_frame(8U);
        camera.begin_frame(9U);
        require(same_pose(*camera.game_camera_pose(), last_pose),
                "an absent player target must retain its last pose across subsequent camera phases without retaining its pointer");
        player.attach_actor(actor);
        player.set_camera_target(std::make_unique<smgpc::tests::CameraTargetFixture>([&actor] {
            return actor.camera_state;
        }));
        require(!player.camera_target_state().has_value(),
                "a replacement target must not publish uninitialized cached vectors");
        target = static_cast<smgpc::tests::CameraTargetFixture*>(player.camera_target());
        camera.begin_frame(10U);
        require(target->movement_count == 1U && player.camera_target_state().has_value(),
                "a replacement player target must initialize at its next camera phase");
        camera.clear_stage_start_camera(owner);
    }

    void test_authored_game_camera_priority_and_restart() {
        auto camera = smgpc::runtime::CameraSystemService{};
        const auto resolved = make_tracking_camera();
        const auto owner = camera.set_authored_game_camera(resolved);
        auto target = resolved.target;
        target.last_move = {15.0F, 0.0F, 0.0F};
        camera.set_game_camera_target(owner, target);
        camera.begin_frame(1U);
        auto prior = *camera.game_camera_pose();

        camera.pause_on_camera_director();
        target.position.x += 200.0F;
        camera.set_game_camera_target(owner, target);
        camera.begin_frame(2U);
        require(same_pose(*camera.game_camera_pose(), prior),
                "director pause must freeze target tracking and interpolation");
        camera.pause_off_camera_director();
        camera.begin_frame(3U);
        require_near(camera.game_camera_pose()->watch.x, target.position.x,
                     0.0001F, "unpausing must consume the newest published target");
        require_near(camera.game_camera_pose()->watch.z - target.position.z,
                     19.0F, 0.0001F,
                     "unpausing must resume interpolation without advancing during pause");
        prior = *camera.game_camera_pose();

        auto player = smgpc::runtime::PlayerSystemService{};
        auto camera_player = CameraPlayerFixture(player);
        camera_player.camera_state.position = {1000.0F, 0.0F, 0.0F};
        Mtx player_matrix{{1.0F, 0.0F, 0.0F, 1000.0F},
                          {0.0F, 1.0F, 0.0F, 0.0F},
                          {0.0F, 0.0F, 1.0F, 0.0F}};
        player.set_base_matrix(player_matrix);
        camera.declare_event_camera_animation(
            0, "tracking-event",
            smgpc::camera::CameraAnimation::from_bytes(make_guide_animation()));
        camera.start_event_camera(
            0, "tracking-event",
            smgpc::camera::EventCameraTarget::target_player(player), 0);
        camera.begin_frame(4U);
        require(same_pose(*camera.game_camera_pose(), prior) &&
                    !same_pose(*camera.effective_camera_pose(), prior),
                "an event camera must own the effective pose while game state remains frozen");
        camera.end_event_camera(0, "tracking-event", false, -1);
        camera.begin_frame(5U);
        require_near(camera.game_camera_pose()->watch.z - target.position.z,
                     34.39F, 0.0001F,
                     "event reactivation must reset then calculate from the retained manager offset");
        prior = *camera.game_camera_pose();

        camera.declare_event_camera_programmable("tracking-programmable");
        camera.start_global_event_camera_no_target("tracking-programmable");
        (void)camera.set_programmable_camera_param(
            "tracking-programmable", {0.0F, 0.0F, 0.0F},
            {500.0F, 100.0F, 200.0F}, {0.0F, 1.0F, 0.0F}, false);
        camera.begin_frame(6U);
        require(same_pose(*camera.game_camera_pose(), prior) &&
                    !same_pose(*camera.effective_camera_pose(), prior),
                "a programmable camera must not feed its pose into authored game state");
        camera.end_global_event_camera("tracking-programmable");
        camera.begin_frame(7U);
        require_near(camera.game_camera_pose()->watch.z - target.position.z,
                     46.8559F, 0.0001F,
                     "programmable reactivation must reset then calculate without clearing manager state");

        camera.end_start_position_camera();
        camera.begin_frame(8U);
        require(camera.game_camera_pose().has_value() && camera.is_start_position_camera_end(),
                "ending start-position mode must preserve normal game-camera calculations");
        require_near(camera.game_camera_pose()->watch.z - target.position.z,
                     52.17031F, 0.0001F,
                     "the end-start flag must not reset or suspend retained local offsets");
        const auto before_start_toggle = *camera.game_camera_pose();
        camera.start_start_position_camera(true);
        require(same_pose(*camera.game_camera_pose(), before_start_toggle),
                "start toggle must preserve the manager pose without an immediate reset");
        camera.begin_frame(9U);
        require_near(camera.game_camera_pose()->watch.z - target.position.z,
                     56.95328F, 0.0001F,
                     "the start flag alone must continue local interpolation from the previous frame");
        camera.clear_stage_start_camera(owner);
    }

    void test_authored_game_camera_owner_retirement() {
        const auto throws_logic_error = [](auto &&call) {
            try {
                call();
            } catch (const std::logic_error &) {
                return true;
            }
            return false;
        };
        auto camera = smgpc::runtime::CameraSystemService{};
        const auto resolved = make_tracking_camera();
        const auto old_owner = camera.set_authored_game_camera(resolved);
        auto target = resolved.target;
        target.position.x += 500.0F;
        camera.set_game_camera_target(old_owner, target);
        camera.begin_frame(1U);
        const auto tracked = *camera.game_camera_pose();

        auto unsupported = resolved;
        unsupported.camera_param.camera_type = "CAM_TYPE_FOLLOW";
        require(throws_logic_error([&] {
                    (void)camera.set_authored_game_camera(unsupported);
                }) && same_pose(*camera.game_camera_pose(), tracked),
                "an unsupported authored camera must fail before replacing a valid owner");
        auto invalid_target = target;
        invalid_target.last_move.x = std::numeric_limits<float>::quiet_NaN();
        require(throws_logic_error([&] {
                    camera.set_game_camera_target(old_owner, invalid_target);
                }),
                "invalid target publication must fail without poisoning the retained target");
        camera.begin_frame(2U);
        require(same_pose(*camera.game_camera_pose(), tracked),
                "a rejected target must leave the previously valid target usable");

        const auto owner = camera.set_authored_game_camera(resolved);
        require(throws_logic_error([&] {
                    camera.set_game_camera_target(old_owner, target);
                }),
                "a stale stage may not publish into a newer camera owner");
        camera.clear_stage_start_camera(old_owner);
        camera.begin_frame(3U);
        require(same_pose(*camera.game_camera_pose(), resolved.calculation.pose),
                "replacement ownership must discard the previous target and ignore stale teardown");
        camera.set_game_camera_target(owner, target);
        camera.clear_stage_start_camera(owner);
        camera.begin_frame(4U);
        require(!camera.game_camera_pose().has_value() &&
                    camera.stage_start_camera() == nullptr &&
                    throws_logic_error([&] {
                        camera.set_game_camera_target(owner, target);
                    }),
                "matching retirement must discard every authored target and calculation");

        const auto manual_owner = camera.set_authored_game_camera(resolved);
        camera.set_game_camera_target(manual_owner, target);
        const auto manual_pose = make_resolved_start_camera(999.0F).calculation.pose;
        camera.set_game_camera_pose(manual_pose);
        camera.begin_frame(5U);
        camera.clear_stage_start_camera(manual_owner);
        require(same_pose(*camera.game_camera_pose(), manual_pose) &&
                    throws_logic_error([&] {
                        camera.set_game_camera_target(manual_owner, target);
                    }),
                "manual camera publication must discard tracked state and its owner generation");

        const auto cleared_owner = camera.set_authored_game_camera(resolved);
        camera.set_game_camera_target(cleared_owner, target);
        camera.clear_game_camera_pose();
        camera.begin_frame(6U);
        require(!camera.game_camera_pose().has_value() &&
                    throws_logic_error([&] {
                        camera.set_game_camera_target(cleared_owner, target);
                    }),
                "explicit game-camera clearing must remove retained target state");
    }

    void test_start_camera_errors_and_scene_generations() {
        const auto throws_logic_error = [](auto &&call) {
            try {
                call();
            } catch (const std::logic_error &) {
                return true;
            }
            return false;
        };

        require(throws_logic_error([] { MR::startStartPosCamera(true); }) &&
                    throws_logic_error([] { MR::endStartPosCamera(); }) &&
                    throws_logic_error([] { (void)MR::isStartPosCameraEnd(); }),
                "MR start-camera calls must reject a missing camera service explicitly");

        auto camera = smgpc::runtime::CameraSystemService{};
        const auto generic_pose = make_resolved_start_camera(-100.0F)
                                      .calculation.pose;
        camera.set_game_camera_pose(generic_pose);
        {
            auto camera_override =
                smgpc::compat::ScopedCameraSystemServiceOverride(camera);
            require(throws_logic_error([] { MR::startStartPosCamera(true); }) &&
                        throws_logic_error([] { MR::endStartPosCamera(); }) &&
                        throws_logic_error(
                            [] { (void)MR::isStartPosCameraEnd(); }) &&
                        camera.game_camera_pose().has_value() &&
                        same_pose(*camera.game_camera_pose(), generic_pose),
                    "an absent stage owner must fail without poisoning an unrelated base pose");
        }

        auto invalid = make_resolved_start_camera(999.0F);
        invalid.calculation.pose.eye.x =
            std::numeric_limits<float>::quiet_NaN();
        auto rejected_invalid = false;
        try {
            (void)camera.set_stage_start_camera(std::move(invalid));
        } catch (const std::invalid_argument &) {
            rejected_invalid = true;
        }
        require(rejected_invalid && camera.stage_start_camera() == nullptr &&
                    camera.game_camera_pose().has_value() &&
                    same_pose(*camera.game_camera_pose(), generic_pose),
                "an invalid restore candidate must leave prior camera state untouched");

        for (auto generation = 0U; generation < 2U; ++generation) {
            const auto resolved =
                make_resolved_start_camera(200.0F + 100.0F * generation);
            const auto expected_pose = resolved.calculation.pose;
            const auto owner_generation =
                camera.set_stage_start_camera(resolved);
            {
                auto camera_override =
                    smgpc::compat::ScopedCameraSystemServiceOverride(camera);
                MR::endStartPosCamera();
                MR::startStartPosCamera(true);
                require(camera.game_camera_pose().has_value() &&
                            same_pose(*camera.game_camera_pose(), expected_pose),
                        "each stage generation must restore only its own retained pose");
            }
            camera.clear_stage_start_camera(owner_generation);
            require(camera.stage_start_camera() == nullptr &&
                        !camera.game_camera_pose().has_value(),
                    "stage teardown must return the longer-lived camera service to baseline");

            auto camera_override =
                smgpc::compat::ScopedCameraSystemServiceOverride(camera);
            require(throws_logic_error([] { MR::startStartPosCamera(true); }) &&
                        camera.stage_start_camera() == nullptr &&
                        !camera.game_camera_pose().has_value(),
                    "a failed post-teardown restore must not poison the next scene generation");
        }

        const auto old_generation = camera.set_stage_start_camera(
            make_resolved_start_camera(600.0F));
        const auto replacement = make_resolved_start_camera(650.0F);
        const auto replacement_pose = replacement.calculation.pose;
        const auto new_generation =
            camera.set_stage_start_camera(replacement);
        camera.clear_stage_start_camera(old_generation);
        require(camera.stage_start_camera() != nullptr &&
                    camera.stage_start_camera()->camera_key ==
                        replacement.camera_key &&
                    camera.game_camera_pose().has_value() &&
                    same_pose(*camera.game_camera_pose(), replacement_pose),
                "a stale StageHost teardown must not clear a newer stage-camera generation");
        camera.clear_stage_start_camera(new_generation);
        require(camera.stage_start_camera() == nullptr &&
                    !camera.game_camera_pose().has_value(),
                "the matching replacement owner must still release its generation");

        const auto stale = make_resolved_start_camera(700.0F);
        const auto stale_owner_generation =
            camera.set_stage_start_camera(stale);
        camera.set_game_camera_pose(generic_pose);
        camera.clear_stage_start_camera(stale_owner_generation);
        auto camera_override =
            smgpc::compat::ScopedCameraSystemServiceOverride(camera);
        require(camera.stage_start_camera() == nullptr &&
                    throws_logic_error([] { MR::startStartPosCamera(true); }) &&
                    camera.game_camera_pose().has_value() &&
                    same_pose(*camera.game_camera_pose(), generic_pose),
                "a new generic base-camera owner must invalidate stale stage restore state");
    }

    void test_retail_placement_zone_scene_object() {
        auto holder = SceneObjHolder{};
        auto binding = smgpc::scene::SceneObjHolderBinding(holder);
        auto *checker = dynamic_cast<PlacementStateChecker *>(
            holder.create(SceneObj_PlacementStateChecker));
        require(checker != nullptr && MR::getCurrentPlacementZoneId() == -1,
                "the exact placement-state SceneObj must begin outside a placement scope");
        MR::setCurrentPlacementZoneId(5);
        require(MR::getCurrentPlacementZoneId() == 5,
                "the generalized SceneUtil bridge must delegate placement-zone state to the exact SceneObj");
        MR::clearCurrentPlacementZoneId();
        require(MR::getCurrentPlacementZoneId() == -1,
                "clearing a retail placement scope must restore the absent zone sentinel");
    }

    void test_optional_real_disc_heavensdoor_camera() {
        const auto *disc_path = std::getenv("SMGPC_REAL_DISC");
        if (disc_path == nullptr || disc_path[0] == '\0') {
            std::cout << "[skip] real-disc camera test (set SMGPC_REAL_DISC)\n";
            return;
        }

        aurora_dvd_close();
        require(aurora_dvd_open(disc_path), "SMGPC_REAL_DISC should point to a readable SMG image");
        struct DiscCloseGuard {
            ~DiscCloseGuard() {
                aurora_dvd_close();
            }
        } close_guard;
        DVDInit();

        auto scheduler = smgpc::runtime::SceneScheduler{};
        const auto scheduler_binding = smgpc::runtime::SceneSchedulerBinding(scheduler);
        auto dvd = smgpc::runtime::DvdFileSystemService{"/"};
        auto demo = smgpc::compat::DemoSceneRuntime(dvd, {});
        auto event_scene_holder = SceneObjHolder{};
        auto event_scene_binding = smgpc::scene::SceneObjHolderBinding(event_scene_holder);
        require(event_scene_holder.create(SceneObj_AreaObjContainer) != nullptr &&
                    event_scene_holder.create(SceneObj_PlanetGravityManager) != nullptr,
                "the original stage view and matrix target require real area and gravity scene registries");
        const auto root = smgpc::camera::resolve_stage_start_camera(dvd, "HeavensDoorGalaxy", 1, 0, 0);
        require(root.status == smgpc::camera::StageStartCameraResolveStatus::Resolved && root.camera.has_value(),
                "real scenario 1 should resolve its root StartInfo camera");
        require(root.camera->start_info.zone_id == 0 && root.camera->start_info.layer_name == "layera" &&
                    root.camera->start_info.object_name == "Mario" &&
                    root.camera->start_info.camera_id == 78 && root.camera->camera_key == "s:004e" &&
                    root.camera->camera_param.camera_type == "CAM_TYPE_XZ_PARA",
                "real scenario data should select root LayerA MarioNo 0 and s:004e");
        require_near(root.camera->calculation.pose.eye.x, 13402.355347F, 0.5F,
                     "real-disc XZ_PARA should use transformed StartInfo up while gravity is unavailable");
        const auto &param = root.camera->camera_param;
        std::cout << "[authored-camera] zone=" << root.camera->start_info.zone_id
                  << ";key=" << root.camera->camera_key
                  << ";type=" << param.camera_type
                  << ";num1=" << param.general.num1
                  << ";v_pan_use=" << param.extra.v_pan_use
                  << ";dist=" << param.general.dist
                  << ";angle_a=" << param.general.angle_a
                  << ";angle_b=" << param.general.angle_b
                  << ";l_offset=" << param.extra.l_offset
                  << ";l_offset_v=" << param.extra.l_offset_v
                  << ";w_offset=" << param.extra.w_offset.x << ','
                  << param.extra.w_offset.y << ',' << param.extra.w_offset.z
                  << ";roll=" << param.extra.roll
                  << ";fovy=" << param.extra.fovy
                  << ";flags=" << param.extra.flags << '\n';
        require_near(param.general.dist, 1179.19921875F, 0.0001F,
                     "the retained authored distance must match the real StartInfo camera");
        require_near(param.general.angle_a, -0.3436119854450226F, 0.0001F,
                     "the retained authored horizontal angle must match the real StartInfo camera");
        require_near(param.general.angle_b, -0.3595139980316162F, 0.0001F,
                     "the retained authored vertical angle must match the real StartInfo camera");
        require_near(param.extra.l_offset_v, 219.7265625F, 0.0001F,
                     "the retained authored target-up offset must match the real StartInfo camera");

        auto camera = smgpc::runtime::CameraSystemService{};
        const auto real_start_pose = root.camera->calculation.pose;
        const auto owner_generation =
            camera.set_stage_start_camera(*root.camera);
        {
            auto camera_override =
                smgpc::compat::ScopedCameraSystemServiceOverride(camera);
            MR::endStartPosCamera();
            require(camera.stage_start_camera() != nullptr &&
                        camera.stage_start_camera()->camera_key == "s:004e" &&
                        camera.game_camera_pose().has_value() &&
                        same_pose(*camera.game_camera_pose(), real_start_pose),
                    "Guide0 must retain the current real HeavensDoor pose when clearing start mode");
            MR::startStartPosCamera(true);
            require(camera.game_camera_pose().has_value() &&
                        same_pose(*camera.game_camera_pose(), real_start_pose) &&
                        camera.start_position_camera_zero_interpolation_frames() ==
                            0U,
                    "Guide1 true start must preserve the current real-stage pose exactly");
        }
        camera.clear_stage_start_camera(owner_generation);
        require(camera.stage_start_camera() == nullptr &&
                    !camera.game_camera_pose().has_value(),
                "real-stage teardown must release all retained start-camera state");

        const auto event_tables = std::array{
            smgpc::scene::StagePlacementTable{
                .category = "camera",
                .table_name = "CameraParam.bcam",
                .archive_path = root.camera->start_info.archive_path,
                .table_path = "CameraParam.bcam",
                .zone_id = 0,
            },
        };
        const auto event_catalog = smgpc::camera::EventCameraCatalog::from_stage_tables(dvd, event_tables);
        const auto *pipe_camera = event_catalog.find(0, "土管固有出現054");
        require(pipe_camera != nullptr && pipe_camera->camera_param.camera_type == "CAM_TYPE_XZ_PARA",
                "the real root pipe event must supply an authored XZ_PARA controller");
        camera.attach_event_camera_catalog(event_catalog);
        camera.declare_event_camera(0, "土管固有出現054");
        auto event_target = CameraTargetMtx("PersistentEventPauseTarget");
        event_target.mMatrix.mMtx[0][3] = 100.0F;
        event_target.mMatrix.mMtx[1][3] = 200.0F;
        event_target.mMatrix.mMtx[2][3] = 300.0F;
        camera.start_event_camera(0, "土管固有出現054",
                                 smgpc::camera::EventCameraTarget::target_matrix(event_target), 0);
        camera.begin_frame(99U);
        const auto event_before_pause = *camera.active_event_camera_pose();
        camera.pause_on_camera_director();
        camera.pause_on_camera_director();
        event_target.mMatrix.mMtx[0][3] += 75.0F;
        event_target.mMatrix.mMtx[1][3] += 50.0F;
        camera.begin_frame(100U);
        camera.begin_frame(101U);
        require(same_pose(*camera.active_event_camera_pose(), event_before_pause),
                "director pause must freeze persistent original XZ event state while its real target moves");
        camera.pause_off_camera_director();
        camera.begin_frame(102U);
        const auto event_after_pause = *camera.active_event_camera_pose();
        require_near(event_after_pause.watch.x, event_before_pause.watch.x + 75.0F, 0.001F,
                     "unpaused original XZ event must consume the newest target translation");
        require_near(event_after_pause.eye.x, event_before_pause.eye.x + 75.0F, 0.001F,
                     "persistent original XZ event eye must resume with the same target translation");
        camera.end_event_camera(0, "土管固有出現054", true, -1);
        camera.detach_event_camera_catalog(event_catalog);

        const auto child = smgpc::scene::resolve_stage_start_info(dvd, "HeavensDoorGalaxy", 1, 0, 5);
        require(child.has_value() && child->zone_name == "HeavensDoorMysteriousZone" && child->camera_id == 999 &&
                    child->archive_path.find("HeavensDoorMysteriousZone.arc") != std::string::npos,
                "a child-zone start must retain its own zone archive rather than the root archive");
        const auto child_iter = child->iter();
        const char *child_object_name = nullptr;
        auto child_camera_id = s32{-1};
        require(child_iter.isValid() && child_iter.getValue("name", &child_object_name) &&
                    child_object_name != nullptr && std::string_view(child_object_name) == child->object_name &&
                    child_iter.getValue("Camera_id", &child_camera_id) && child_camera_id == child->camera_id &&
                    child_iter.mInfo->getPlacedZoneId() == 5,
                "a resolved real-disc StartInfo must retain its exact actor row and placed-zone identity");
        const auto &child_archive = dvd.archive_for_path(child->archive_path);
        const auto child_chunk = smgpc::camera::load_start_camera_chunk(child_archive,
                                                                        smgpc::camera::make_start_camera_key(child->camera_id));
        require(child_chunk.has_value() && child_chunk->id == "s:03e7" &&
                    child_chunk->camera_type == "CAM_TYPE_EYEPOS_FIX",
                "child-zone camera lookup must load s:03e7 from the selected child archive");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
        bool owns_scene = false;
    };

}  // namespace

int main() {
    constexpr auto tests = std::array{
        TestCase{"rigid zone matrix and StartInfo selection", test_rigid_zone_matrix_and_start_selection},
        TestCase{"camera migration, key, and XZ_PARA pose", test_camera_version_key_and_xz_parallel_pose},
        TestCase{"original controller height and lifetime", test_original_controller_height_and_lifetime},
        TestCase{"original manager safe basis and deferred reset", test_original_manager_safe_basis_and_deferred_reset},
        TestCase{"base and programmable camera priority", test_base_programmable_camera_priority_and_request_defaults},
        TestCase{"RunawayTico start-camera handoff", test_runaway_tico_start_camera_handoff},
        TestCase{"authored game-camera target tracking", test_authored_game_camera_target_tracking},
        TestCase{"authored game-camera offset accumulation", test_authored_game_camera_offset_accumulation},
        TestCase{"player target camera phase ownership", test_player_target_camera_phase_ownership},
        TestCase{"authored game-camera priority and restart", test_authored_game_camera_priority_and_restart},
        TestCase{"authored game-camera owner retirement", test_authored_game_camera_owner_retirement},
        TestCase{"start-camera errors and scene generations", test_start_camera_errors_and_scene_generations},
        TestCase{"retail placement-zone SceneObj", test_retail_placement_zone_scene_object, true},
        TestCase{"optional real-disc HeavensDoor camera", test_optional_real_disc_heavensdoor_camera, true},
    };

    auto failures = 0;
    for (const auto &test : tests) {
        try {
            auto holder = std::unique_ptr<SceneObjHolder>{};
            auto binding = std::unique_ptr<smgpc::scene::SceneObjHolderBinding>{};
            if (!test.owns_scene) {
                holder = std::make_unique<SceneObjHolder>();
                binding = std::make_unique<smgpc::scene::SceneObjHolderBinding>(*holder);
                require(holder->create(SceneObj_AreaObjContainer) != nullptr,
                        "original stage camera views require the real AreaObj registry");
            }
            test.run();
            std::cout << "[ok] " << test.name << '\n';
        } catch (const std::exception &error) {
            ++failures;
            std::cerr << "[fail] " << test.name << ": " << error.what() << '\n';
        }
    }
    if (failures != 0) {
        std::cerr << failures << " stage-start camera test(s) failed\n";
        return 1;
    }
    std::cout << tests.size() << " stage-start camera test(s) passed\n";
    return 0;
}
