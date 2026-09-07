#include "camera/PublishedCameraTarget.hpp"

#include <cmath>
#include <stdexcept>

namespace smgpc::camera {
    namespace {
        TVec3f game_vec(const CameraParamVec3 &value) {
            return {value.x, value.y, value.z};
        }
    }

    void validate_original_camera_target(const StageCameraTargetState &target) {
        const auto finite = [](const CameraParamVec3 &value) {
            return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
        };
        if (!finite(target.position) || !finite(target.up) || !finite(target.front) ||
            !finite(target.last_move) || game_vec(target.up).squared() <= 1.0e-12F ||
            game_vec(target.front).squared() <= 1.0e-12F ||
            (target.ground_position.has_value() && !finite(*target.ground_position)) ||
            (target.gravity.has_value() && !finite(*target.gravity)) ||
            (target.side.has_value() && !finite(*target.side))) {
            throw std::invalid_argument("Original camera target requires finite vectors and a non-degenerate orientation.");
        }
    }

    PublishedCameraTarget::PublishedCameraTarget() : CameraTargetObj("PublishedCameraTarget") {}

    void PublishedCameraTarget::publish(const StageCameraTargetState &state) {
        validate_original_camera_target(state);
        _position = game_vec(state.position);
        _up = game_vec(state.up);
        _front = game_vec(state.front);
        _side = state.side.has_value() ? game_vec(*state.side) : _up.cross(_front);
        _last_move = game_vec(state.last_move);
        _ground = state.ground_position.has_value()
                      ? std::optional<TVec3f>{game_vec(*state.ground_position)} : std::nullopt;
        _gravity = state.gravity.has_value()
                       ? std::optional<TVec3f>{game_vec(*state.gravity)} : std::nullopt;
        _jumping = state.jumping;
        _fast_rise = state.fast_rise;
        _fast_drop = state.fast_drop;
    }

    const TVec3f &PublishedCameraTarget::getGroundPos() const {
        if (!_ground.has_value()) {
            throw std::logic_error("Original camera target has no published ground position.");
        }
        return *_ground;
    }

    const TVec3f &PublishedCameraTarget::getGravityVector() const {
        if (!_gravity.has_value()) {
            throw std::logic_error("Original camera target has no published gravity vector.");
        }
        return *_gravity;
    }

}  // namespace smgpc::camera
