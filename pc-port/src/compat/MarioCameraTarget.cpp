#include "compat/MarioCameraTarget.hpp"

#include "Game/Player/MarioActor.hpp"
#include "Game/Util/MathUtil.hpp"

namespace smgpc::compat {

    // CameraTargetPlayer reads the actor's camera translation and orientation,
    // which can differ from its collision position and rendered base matrix.
    // Keep the concrete player dependency in callers that own MarioActor.
    camera::StageCameraTargetState mario_camera_target(
        const MarioActor &actor) {
        auto up = TVec3f{};
        auto front = TVec3f{};
        auto side = TVec3f{};
        actor.getUpVec(&up);
        actor.getFrontVec(&front);
        actor.getSideVec(&side);
        if (MR::isNearZero(up)) {
            up.set(0.0F, 1.0F, 0.0F);
        } else {
            MR::normalize(&up);
        }
        const auto &position = actor.getTransForCamera();
        const auto &last_move = actor.getLastMove();
        const auto &ground_position = *actor.getShadowPos();
        auto gravity = TVec3f{};
        actor.getGravityVector(&gravity);
        return {
            .position = {position.x, position.y, position.z},
            .up = {up.x, up.y, up.z},
            .front = {front.x, front.y, front.z},
            .last_move = {last_move.x, last_move.y, last_move.z},
            .ground_position = camera::CameraParamVec3{
                ground_position.x, ground_position.y, ground_position.z},
            .gravity = camera::CameraParamVec3{gravity.x, gravity.y, gravity.z},
            .jumping = actor.isJumping(),
            .fast_rise = actor.isFastRise(),
            .fast_drop = actor.isFastDrop(),
            .side = camera::CameraParamVec3{side.x, side.y, side.z},
        };
    }

}  // namespace smgpc::compat
