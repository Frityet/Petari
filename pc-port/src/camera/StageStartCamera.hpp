#pragma once

#include "camera/CameraParam.hpp"
#include "camera/CameraPose.hpp"
#include "resource/RarcArchive.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace smgpc::runtime {
    class DvdFileSystemService;
}  // namespace smgpc::runtime

namespace smgpc::camera {

    struct StageCameraTargetState {
        CameraParamVec3 position{};
        CameraParamVec3 up{0.0F, 1.0F, 0.0F};
        CameraParamVec3 front{0.0F, 0.0F, 1.0F};
        CameraParamVec3 last_move{};
    };

    struct StageCameraCalculationState {
        CameraParamVec3 local_offset{};
        float round_angle_radians = 0.0F;
    };

    struct StageCameraPoseCalculation {
        CameraPose pose{};
        StageCameraCalculationState state{};
    };

    struct ResolvedStageStartCamera {
        smgpc::scene::StageStartInfo start_info{};
        CameraParamChunk camera_param{};
        std::string camera_key;
        StageCameraTargetState target{};
        StageCameraPoseCalculation calculation{};
        bool used_start_orientation_up_fallback = true;
    };

    enum class StageStartCameraResolveStatus {
        Resolved,
        StartInfoNotFound,
        CameraArchiveUnavailable,
        CameraParamUnavailable,
        CameraChunkNotFound,
        UnsupportedCameraType,
    };

    struct StageStartCameraResolveResult {
        StageStartCameraResolveStatus status = StageStartCameraResolveStatus::StartInfoNotFound;
        std::optional<ResolvedStageStartCamera> camera;
        std::string detail;
    };

    [[nodiscard]] std::string_view stage_start_camera_status_name(StageStartCameraResolveStatus status);
    [[nodiscard]] std::string make_start_camera_key(std::int32_t camera_id);
    [[nodiscard]] std::optional<CameraParamChunk> load_start_camera_chunk(const smgpc::resource::RarcArchive &archive,
                                                                          std::string_view camera_key);
    [[nodiscard]] std::optional<StageCameraPoseCalculation> calculate_stage_camera_pose(
        const smgpc::scene::StageZoneTransform &zone_transform, const CameraParamChunk &camera_param,
        const StageCameraTargetState &target, const StageCameraCalculationState &state = {}, float default_fovy_degrees = 45.0F);
    [[nodiscard]] StageStartCameraResolveResult resolve_stage_start_camera(
        smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name, std::int32_t scenario_no,
        std::int32_t start_id = 0, std::int32_t start_zone_id = 0, float default_fovy_degrees = 45.0F);
    [[nodiscard]] StageStartCameraResolveResult resolve_stage_start_camera(
        smgpc::runtime::DvdFileSystemService &dvd, const smgpc::scene::StageStartInfo &start_info,
        float default_fovy_degrees = 45.0F);

}  // namespace smgpc::camera
