#pragma once

#include "Game/Camera/CameraTargetObj.hpp"
#include "camera/StageStartCamera.hpp"

#include <optional>

namespace smgpc::camera {

    void validate_original_camera_target(const StageCameraTargetState &target);

    // A typed boundary for the original camera controllers. Missing source
    // geometry remains unavailable when a controller requests it.
    class PublishedCameraTarget final : public CameraTargetObj {
    public:
        PublishedCameraTarget();
        void publish(const StageCameraTargetState &state);

        const TVec3f &getPosition() const override { return _position; }
        const TVec3f &getUpVec() const override { return _up; }
        const TVec3f &getFrontVec() const override { return _front; }
        const TVec3f &getSideVec() const override { return _side; }
        const TVec3f &getLastMove() const override { return _last_move; }
        const TVec3f &getGroundPos() const override;
        const TVec3f &getGravityVector() const override;
        bool isJumping() const override { return _jumping; }
        bool isFastRise() const override { return _fast_rise; }
        bool isFastDrop() const override { return _fast_drop; }

    private:
        TVec3f _position{0.0F, 0.0F, 0.0F};
        TVec3f _up{0.0F, 1.0F, 0.0F};
        TVec3f _front{0.0F, 0.0F, 1.0F};
        TVec3f _side{1.0F, 0.0F, 0.0F};
        TVec3f _last_move{0.0F, 0.0F, 0.0F};
        std::optional<TVec3f> _ground;
        std::optional<TVec3f> _gravity;
        bool _jumping = false;
        bool _fast_rise = false;
        bool _fast_drop = false;
    };

}  // namespace smgpc::camera
