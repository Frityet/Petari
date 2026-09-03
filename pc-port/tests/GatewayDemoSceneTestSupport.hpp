#pragma once

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/GatewayDemoScene.hpp"

#include <cstddef>
#include <stdexcept>

namespace smgpc::test {

    // Data/visual route proofs still cross the real Gateway player boundary.
    // This deliberately small external actor has an observable init and scene
    // registration, but owns no Gateway placements and receives its
    // initAfterPlacement only from GatewayDemoScene::finalize_placements.
    class GatewayPlayerSentinel final : public LiveActor {
    public:
        GatewayPlayerSentinel(
            smgpc::runtime::RuntimeContext &runtime,
            const smgpc::scene::GatewayDemoScene &scene)
            : LiveActor("Gateway external player sentinel"),
              _runtime(&runtime) {
            _runtime->player_system().attach_actor(*this, smgpc::runtime::PlayerActorBridge{
                .read_camera_target = &GatewayPlayerSentinel::read_camera_target});
            try {
                init(scene.player_start_iter());
            } catch (...) {
                _runtime->player_system().detach_actor(this);
                throw;
            }
        }

        ~GatewayPlayerSentinel() override {
            if (_runtime != nullptr &&
                _runtime->player_system().attached_actor() == this) {
                _runtime->player_system().detach_actor(this);
            }
        }

        GatewayPlayerSentinel(const GatewayPlayerSentinel &) = delete;
        GatewayPlayerSentinel &operator=(const GatewayPlayerSentinel &) = delete;

        void init(const JMapInfoIter &iter) override {
            if (!iter.isValid()) {
                throw std::logic_error(
                    "Gateway player sentinel requires the exact retained StartInfo row.");
            }
            if (!iter.getValue("pos_x", &mPosition.x) ||
                !iter.getValue("pos_y", &mPosition.y) ||
                !iter.getValue("pos_z", &mPosition.z)) {
                throw std::logic_error(
                    "Gateway player sentinel StartInfo is missing its position.");
            }
            calcAndSetBaseMtx();
            MR::connectToScene(
                this, MR::MovementType_Player, MR::CalcAnimType_Player,
                MR::DrawBufferType_Player, MR::DrawType_Player);
            _initialized = true;
        }

        void initAfterPlacement() override {
            if (!_initialized) {
                throw std::logic_error(
                    "Gateway player sentinel postpass preceded its init boundary.");
            }
            ++_post_placement_count;
        }

        [[nodiscard]] std::size_t post_placement_count() const noexcept {
            return _post_placement_count;
        }

    private:
        // This fixture has no Mario movement states. Its explicit camera
        // capability describes the sentinel's actual stationary actor basis.
        static smgpc::camera::StageCameraTargetState read_camera_target(const LiveActor &actor) {
            const auto &matrix = smgpc::compat::actor_base_matrix(&actor).m;
            return {
                .position = {actor.mPosition.x, actor.mPosition.y, actor.mPosition.z},
                .up = {matrix[1U], matrix[5U], matrix[9U]},
                .front = {matrix[2U], matrix[6U], matrix[10U]},
                .last_move = {actor.mVelocity.x, actor.mVelocity.y, actor.mVelocity.z},
                .jumping = false,
                .fast_rise = false,
                .fast_drop = false,
                .side = smgpc::camera::CameraParamVec3{matrix[0U], matrix[4U], matrix[8U]}};
        }

        smgpc::runtime::RuntimeContext *_runtime = nullptr;
        bool _initialized = false;
        std::size_t _post_placement_count = 0U;
    };

}  // namespace smgpc::test
