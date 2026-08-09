#pragma once

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Scene/SceneFunction.hpp"
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
            _runtime->player_system().attach_actor(*this);
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
        smgpc::runtime::RuntimeContext *_runtime = nullptr;
        bool _initialized = false;
        std::size_t _post_placement_count = 0U;
    };

}  // namespace smgpc::test
