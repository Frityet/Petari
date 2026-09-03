#include "camera/StageStartCamera.hpp"
#include "camera/OriginalGameCamera.hpp"

#include "resource/BcsvTable.hpp"
#include "runtime/RuntimeServices.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <exception>
#include <stdexcept>

namespace smgpc::camera {
    namespace {

        [[nodiscard]] float dot(const CameraParamVec3 &lhs, const CameraParamVec3 &rhs) {
            return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
        }

        [[nodiscard]] float length(const CameraParamVec3 &value) {
            return std::sqrt(dot(value, value));
        }

        [[nodiscard]] std::optional<CameraParamVec3> normalized(const CameraParamVec3 &value) {
            const auto magnitude = length(value);
            if (!std::isfinite(magnitude) || magnitude <= 0.000001F) {
                return std::nullopt;
            }
            return CameraParamVec3{value.x / magnitude, value.y / magnitude, value.z / magnitude};
        }

        [[nodiscard]] CameraParamVec3 camera_vec(const std::array<f32, 3U> &value) {
            return {.x = value[0], .y = value[1], .z = value[2]};
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
        try {
            const auto controller = OriginalGameCamera(zone_transform, camera_param, target,
                                                       default_fovy_degrees, state);
            const auto result = controller.calculation();
            const auto up = normalized(result.pose.up);
            if (!up.has_value()) {
                return std::nullopt;
            }
            return result;
        } catch (const std::invalid_argument &) {
            return std::nullopt;
        }
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
