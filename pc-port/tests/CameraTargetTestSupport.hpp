#pragma once

#include "Game/Camera/CameraTargetObj.hpp"
#include "camera/StageStartCamera.hpp"

#include <functional>
#include <stdexcept>
#include <utility>

namespace smgpc::tests {
    // Explicit test data represented by a real owned CameraTargetObj. Tests
    // supply ground/gravity when they query the complete player target state.
    class CameraTargetFixture final : public CameraTargetObj {
    public:
        using Reader = std::function<camera::StageCameraTargetState()>;

        explicit CameraTargetFixture(Reader reader)
            : CameraTargetObj("Explicit camera target fixture"), _read(std::move(reader)) {}

        void movement() override { ++movement_count; }
        const TVec3f& getPosition() const override { return assign(_position, _read().position); }
        const TVec3f& getUpVec() const override { return assign(_up, _read().up); }
        const TVec3f& getFrontVec() const override { return assign(_front, _read().front); }
        const TVec3f& getLastMove() const override { return assign(_last_move, _read().last_move); }
        const TVec3f& getSideVec() const override {
            const auto state = _read();
            if (state.side.has_value()) {
                return assign(_side, *state.side);
            }
            const auto& up = state.up;
            const auto& front = state.front;
            _side.set(up.y * front.z - up.z * front.y,
                      up.z * front.x - up.x * front.z,
                      up.x * front.y - up.y * front.x);
            return _side;
        }
        const TVec3f& getGroundPos() const override {
            const auto state = _read();
            if (!state.ground_position.has_value()) {
                throw std::logic_error("The camera test fixture did not supply ground position.");
            }
            return assign(_ground, *state.ground_position);
        }
        const TVec3f& getGravityVector() const override {
            const auto state = _read();
            if (!state.gravity.has_value()) {
                throw std::logic_error("The camera test fixture did not supply gravity.");
            }
            return assign(_gravity, *state.gravity);
        }
        bool isJumping() const override { return _read().jumping; }
        bool isFastRise() const override { return _read().fast_rise; }
        bool isFastDrop() const override { return _read().fast_drop; }

        unsigned movement_count = 0U;

    private:
        static const TVec3f& assign(TVec3f& result, const camera::CameraParamVec3& value) {
            result.set(value.x, value.y, value.z);
            return result;
        }
        Reader _read;
        mutable TVec3f _position;
        mutable TVec3f _up;
        mutable TVec3f _front;
        mutable TVec3f _side;
        mutable TVec3f _last_move;
        mutable TVec3f _ground;
        mutable TVec3f _gravity;
    };
}
