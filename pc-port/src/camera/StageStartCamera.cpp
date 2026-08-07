#include "camera/StageStartCamera.hpp"

#include "resource/BcsvTable.hpp"
#include "runtime/RuntimeServices.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <exception>

namespace smgpc::camera {
    namespace {

        [[nodiscard]] CameraParamVec3 add(const CameraParamVec3 &lhs, const CameraParamVec3 &rhs) {
            return {.x = lhs.x + rhs.x, .y = lhs.y + rhs.y, .z = lhs.z + rhs.z};
        }

        [[nodiscard]] CameraParamVec3 subtract(const CameraParamVec3 &lhs, const CameraParamVec3 &rhs) {
            return {.x = lhs.x - rhs.x, .y = lhs.y - rhs.y, .z = lhs.z - rhs.z};
        }

        [[nodiscard]] CameraParamVec3 scale(const CameraParamVec3 &value, float factor) {
            return {.x = value.x * factor, .y = value.y * factor, .z = value.z * factor};
        }

        [[nodiscard]] float dot(const CameraParamVec3 &lhs, const CameraParamVec3 &rhs) {
            return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
        }

        [[nodiscard]] CameraParamVec3 cross(const CameraParamVec3 &lhs, const CameraParamVec3 &rhs) {
            return {
                .x = lhs.y * rhs.z - lhs.z * rhs.y,
                .y = lhs.z * rhs.x - lhs.x * rhs.z,
                .z = lhs.x * rhs.y - lhs.y * rhs.x,
            };
        }

        [[nodiscard]] float length(const CameraParamVec3 &value) {
            return std::sqrt(dot(value, value));
        }

        [[nodiscard]] std::optional<CameraParamVec3> normalized(const CameraParamVec3 &value) {
            const auto magnitude = length(value);
            if (magnitude <= 0.000001F) {
                return std::nullopt;
            }
            return scale(value, 1.0F / magnitude);
        }

        [[nodiscard]] CameraParamVec3 camera_vec(const std::array<f32, 3U> &value) {
            return {.x = value[0], .y = value[1], .z = value[2]};
        }

        [[nodiscard]] std::array<f32, 3U> stage_vec(const CameraParamVec3 &value) {
            return {value.x, value.y, value.z};
        }

        [[nodiscard]] CameraParamVec3 transform_vector(const smgpc::scene::StageZoneTransform &transform,
                                                        const CameraParamVec3 &value) {
            return camera_vec(transform.transform_vector(stage_vec(value)));
        }

    }  // namespace

    std::string_view stage_start_camera_status_name(StageStartCameraResolveStatus status) {
        switch (status) {
        case StageStartCameraResolveStatus::Resolved:
            return "resolved";
        case StageStartCameraResolveStatus::StartInfoNotFound:
            return "start_info_not_found";
        case StageStartCameraResolveStatus::CameraArchiveUnavailable:
            return "camera_archive_unavailable";
        case StageStartCameraResolveStatus::CameraParamUnavailable:
            return "camera_param_unavailable";
        case StageStartCameraResolveStatus::CameraChunkNotFound:
            return "camera_chunk_not_found";
        case StageStartCameraResolveStatus::UnsupportedCameraType:
            return "unsupported_camera_type";
        case StageStartCameraResolveStatus::InvalidCameraBasis:
            return "invalid_camera_basis";
        }
        return "unknown";
    }

    std::string make_start_camera_key(std::int32_t camera_id) {
        auto buffer = std::array<char, 16U>{};
        std::snprintf(buffer.data(), buffer.size(), "s:%04x", static_cast<unsigned int>(static_cast<std::uint16_t>(camera_id)));
        return buffer.data();
    }

    std::optional<CameraParamChunk> load_start_camera_chunk(const smgpc::resource::RarcArchive &archive,
                                                             std::string_view camera_key) {
        if (!archive.contains_resource("CameraParam.bcam")) {
            return std::nullopt;
        }
        const auto table = smgpc::resource::BcsvTable::from_bytes(archive.resource_data("CameraParam.bcam"));
        const auto chunks = load_camera_param_chunks(table);
        return find_camera_param_chunk(chunks, camera_key);
    }

    std::optional<StageCameraPoseCalculation> calculate_stage_camera_pose(
        const smgpc::scene::StageZoneTransform &zone_transform, const CameraParamChunk &camera_param,
        const StageCameraTargetState &target, const StageCameraCalculationState &state, float default_fovy_degrees) {
        if (camera_param.camera_type != "CAM_TYPE_XZ_PARA") {
            return std::nullopt;
        }

        const auto target_front = normalized(target.front);
        const auto target_up = normalized(target.up);
        if (!target_front.has_value() || !target_up.has_value()) {
            return std::nullopt;
        }
        const auto desired_local_offset = add(scale(*target_front, camera_param.extra.l_offset),
                                              scale(*target_up, camera_param.extra.l_offset_v));
        const auto local_offset_factor = camera_param.is_l_offset_erp_off()
                                             ? 1.0F
                                             : std::min(1.0F, length(target.last_move) * (0.1F / 15.0F));

        auto result = StageCameraPoseCalculation{.state = state};
        result.state.local_offset = add(state.local_offset,
                                        scale(subtract(desired_local_offset, state.local_offset), local_offset_factor));

        const auto distance = std::max(camera_param.general.dist, 300.0F);
        const auto angle_a = camera_param.general.angle_a + result.state.round_angle_radians;
        const auto angle_b = camera_param.general.angle_b;
        const auto cos_b = std::cos(angle_b);
        const auto local_eye_offset = CameraParamVec3{
            .x = -distance * cos_b * std::cos(angle_a),
            .y = distance * std::sin(angle_b),
            .z = distance * cos_b * std::sin(angle_a),
        };
        const auto world_eye_offset = transform_vector(zone_transform, local_eye_offset);
        const auto global_offset = transform_vector(zone_transform, camera_param.extra.w_offset);
        const auto watch = add(target.position, add(result.state.local_offset, global_offset));
        const auto eye = add(watch, world_eye_offset);
        const auto raw_up = normalized(transform_vector(zone_transform, {0.0F, 1.0F, 0.0F}));
        const auto forward = normalized(subtract(watch, eye));
        if (!raw_up.has_value() || !forward.has_value()) {
            return std::nullopt;
        }
        const auto right = normalized(cross(*forward, *raw_up));
        if (!right.has_value()) {
            return std::nullopt;
        }
        const auto corrected_up = normalized(cross(*right, *forward));
        if (!corrected_up.has_value()) {
            return std::nullopt;
        }
        const auto rolled_up = normalized(add(scale(*corrected_up, std::cos(camera_param.extra.roll)),
                                              scale(*right, -std::sin(camera_param.extra.roll))));
        if (!rolled_up.has_value()) {
            return std::nullopt;
        }

        result.pose.eye = eye;
        result.pose.watch = watch;
        result.pose.up = *rolled_up;
        result.pose.fovy_degrees = camera_param.is_on_use_fovy() ? camera_param.extra.fovy : default_fovy_degrees;
        return result;
    }

    StageStartCameraResolveResult resolve_stage_start_camera(smgpc::runtime::DvdFileSystemService &dvd,
                                                              std::string_view stage_name, std::int32_t scenario_no,
                                                              std::int32_t start_id, std::int32_t start_zone_id,
                                                              float default_fovy_degrees) {
        const auto start_info = smgpc::scene::resolve_stage_start_info(dvd, stage_name, scenario_no, start_id, start_zone_id);
        if (!start_info.has_value()) {
            return {
                .status = StageStartCameraResolveStatus::StartInfoNotFound,
                .detail = "no active StartInfo matched the requested start and zone",
            };
        }

        return resolve_stage_start_camera(dvd, *start_info, default_fovy_degrees);
    }

    StageStartCameraResolveResult resolve_stage_start_camera(smgpc::runtime::DvdFileSystemService &dvd,
                                                              const smgpc::scene::StageStartInfo &start_info,
                                                              float default_fovy_degrees) {

        const auto camera_key = make_start_camera_key(start_info.camera_id);
        if (start_info.archive_path.empty()) {
            return {
                .status = StageStartCameraResolveStatus::CameraArchiveUnavailable,
                .detail = "selected StartInfo has no zone archive path",
            };
        }

        try {
            const auto &archive = dvd.archive_for_path(start_info.archive_path);
            if (!archive.contains_resource("CameraParam.bcam")) {
                return {
                    .status = StageStartCameraResolveStatus::CameraParamUnavailable,
                    .detail = "selected start zone does not contain CameraParam.bcam",
                };
            }
            const auto camera_param = load_start_camera_chunk(archive, camera_key);
            if (!camera_param.has_value()) {
                return {
                    .status = StageStartCameraResolveStatus::CameraChunkNotFound,
                    .detail = "CameraParam.bcam does not contain " + camera_key,
                };
            }

            const auto target_up = normalized(camera_vec(start_info.world_up));
            const auto target_front = normalized(camera_vec(start_info.world_front));
            if (!target_up.has_value() || !target_front.has_value()) {
                return {
                    .status = StageStartCameraResolveStatus::InvalidCameraBasis,
                    .detail = "selected StartInfo has a degenerate orientation basis",
                };
            }
            const auto target = StageCameraTargetState{
                .position = camera_vec(start_info.world_position),
                .up = *target_up,
                .front = *target_front,
            };
            const auto calculation = calculate_stage_camera_pose(start_info.zone_transform, *camera_param, target, {},
                                                                  default_fovy_degrees);
            if (!calculation.has_value()) {
                return {
                    .status = camera_param->camera_type == "CAM_TYPE_XZ_PARA"
                                  ? StageStartCameraResolveStatus::InvalidCameraBasis
                                  : StageStartCameraResolveStatus::UnsupportedCameraType,
                    .detail = camera_param->camera_type == "CAM_TYPE_XZ_PARA"
                                  ? "start camera basis is degenerate"
                                  : "unsupported start camera type " + camera_param->camera_type,
                };
            }

            return {
                .status = StageStartCameraResolveStatus::Resolved,
                .camera = ResolvedStageStartCamera{
                    .start_info = start_info,
                    .camera_param = *camera_param,
                    .camera_key = camera_key,
                    .target = target,
                    .calculation = *calculation,
                },
                .detail = "resolved from selected StartInfo zone archive",
            };
        } catch (const std::exception &error) {
            return {
                .status = StageStartCameraResolveStatus::CameraArchiveUnavailable,
                .detail = error.what(),
            };
        }
    }

}  // namespace smgpc::camera
