#include "Game/LiveActor/LiveActor.hpp"
#include "runtime/RuntimeServices.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    class CenterPlayer final : public LiveActor {
    public:
        CenterPlayer() : LiveActor("center-position-owner") {}
        TVec3f center{40.0F, 50.0F, 60.0F};
    };

    class MatrixPlayer final : public LiveActor {
    public:
        MatrixPlayer() : LiveActor("matrix-update-owner") {}

        void calcAndSetBaseMtx() override {
            ++matrix_updates;
            matrix[0][3] = mPosition.x;
            matrix[1][3] = mPosition.y;
            matrix[2][3] = mPosition.z;
        }
        void calcAnim() override {
            calcAndSetBaseMtx();
        }
        MtxPtr getBaseMtx() const override {
            return const_cast<MtxPtr>(matrix);
        }

        Mtx matrix{{1.0F, 0.0F, 0.0F, -3.0F},
                   {0.0F, 1.0F, 0.0F, -5.0F},
                   {0.0F, 0.0F, 1.0F, -7.0F}};
        unsigned matrix_updates = 0;
    };

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }
}

int main() {
    auto passed = 0;
    auto player = smgpc::runtime::PlayerSystemService{};
    require(player.actor_center_position() == nullptr,
            "an unattached player must not manufacture a center");
    auto center_actor = CenterPlayer{};
    center_actor.mPosition.set(1.0F, 2.0F, 3.0F);
    player.attach_actor(center_actor);
    require(player.actor_center_position() == nullptr,
            "an actor without center capability must not substitute its translation");
    const auto center_bridge = smgpc::runtime::PlayerActorBridge{
        .read_center_position = [](LiveActor& actor) {
            return &static_cast<CenterPlayer&>(actor).center;
        },
    };
    player.attach_actor(center_actor, center_bridge);
    auto* center = player.actor_center_position();
    require(center == &center_actor.center && center != &center_actor.mPosition,
            "the player center must preserve the owner's actual field address");
    center_actor.center.set(7.0F, 8.0F, 9.0F);
    require(player.actor_center_position() == center && center->x == 7.0F && center->y == 8.0F,
            "the stable center pointer must expose owner updates without synchronization");
    center->z = 12.0F;
    require(center_actor.center.z == 12.0F,
            "the bridge's mutable pointer must write through to its owner");
    auto replacement_actor = CenterPlayer{};
    player.attach_actor(replacement_actor, center_bridge);
    require(player.actor_center_position() == &replacement_actor.center,
            "replacing the player must expose the new owner's center");
    player.detach_actor(&replacement_actor);
    require(player.actor_center_position() == nullptr,
            "detaching a player must not retain its center pointer");
    player.attach_actor(center_actor, center_bridge);
    player.clear_stage_state();
    require(player.actor_center_position() == nullptr,
            "clearing the stage must remove the center capability");
    ++passed;

    auto matrix_actor = MatrixPlayer{};
    matrix_actor.mPosition.set(10.0F, 20.0F, 30.0F);
    matrix_actor.mVelocity.set(1.0F, 2.0F, 3.0F);
    matrix_actor.mGravity.set(0.0F, 0.0F, -1.0F);
    player.attach_actor(matrix_actor);
    player.synchronize_attached_actor();
    player.synchronize_attached_actor();
    require(matrix_actor.matrix_updates == 0,
            "attaching and publishing player state must never execute actor matrix callbacks");
    require(player.position()[0] == 10.0F && player.position()[1] == 20.0F &&
                player.velocity()[2] == 3.0F && player.gravity()[2] == -1.0F &&
                player.has_base_matrix() && player.base_matrix()[3] == -3.0F &&
                player.base_matrix()[7] == -5.0F && player.base_matrix()[11] == -7.0F,
            "the snapshot must preserve current physics and the matrix actually produced by its owner");
    matrix_actor.calcAnim();
    player.synchronize_attached_actor();
    require(matrix_actor.matrix_updates == 1 && player.base_matrix()[3] == 10.0F &&
                player.base_matrix()[7] == 20.0F && player.base_matrix()[11] == 30.0F,
            "publishing after the fixture actor animation phase must retain its one matrix update");
    matrix_actor.mPosition.x = 40.0F;
    player.synchronize_attached_actor();
    require(matrix_actor.matrix_updates == 1 && player.position()[0] == 40.0F &&
                player.base_matrix()[3] == 10.0F,
            "a new physics position must not force animation ahead of its scheduled phase");
    player.detach_actor(&matrix_actor);
    player.synchronize_attached_actor();
    require(matrix_actor.matrix_updates == 1,
            "detached snapshots cannot retain an actor callback");
    ++passed;

    std::cout << "Player actor bridge tests passed: " << passed << "/2\n";
    return 0;
}
