#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Screen/StarPointerTarget.hpp"
#include "Game/Util/StarPointerUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "runtime/RuntimeServices.hpp"

#include <aurora/wpad.hpp>

#include <array>
#include <iostream>
#include <stdexcept>

int main() {
    auto& input = aurora::wpad_service();
    const auto saved_input = input;
    try {
        input.clear();
        LiveActor actor("Pointer device boundary");
        actor.mPosition.set(10.0F, 20.0F, 30.0F);
        actor.initActorStarPointerTarget(40.0F, &actor.mPosition, nullptr, TVec3f(1.0F, 2.0F, 3.0F));
        auto* target = actor.mStarPointerTarget;
        if (target == nullptr || target->mRadius3d != 40.0F ||
            MR::getStarPointerLastPointedPort(&actor) != &target->mLastPointedChannel ||
            *MR::getStarPointerLastPointedPort(&actor) != WPAD_CHAN0) {
            throw std::runtime_error("original target construction did not expose its owned fields");
        }
        TVec3f world;
        target->calcPosition(&world);
        if (world.x != 11.0F || world.y != 22.0F || world.z != 33.0F) {
            throw std::runtime_error("position-bound target lost its original offset");
        }
        Mtx matrix = {{0.0F, -1.0F, 0.0F, 100.0F},
                      {1.0F, 0.0F, 0.0F, 200.0F},
                      {0.0F, 0.0F, 1.0F, 300.0F}};
        target->mMtx = matrix;
        target->calcPosition(&world);
        if (world.x != 8.0F || world.y != 21.0F || world.z != 33.0F) {
            throw std::runtime_error("target offset did not follow its retained matrix basis");
        }
        target->mPosition = nullptr;
        target->calcPosition(&world);
        if (world.x != 98.0F || world.y != 201.0F || world.z != 303.0F) {
            throw std::runtime_error("matrix-bound target did not use the original matrix translation");
        }
        smgpc::runtime::StarPointerService service;
        MR::setStarPointerTargetRadius3d(&actor, -8.0F);
        if (!service.has_target(actor) || !MR::isExistStarPointerTarget(&actor) || target->mRadius3d != -8.0F) {
            throw std::runtime_error("native service diverged from the original target state");
        }
        const auto queries = std::array{
            MR::isStarPointerPointing2P,
            MR::isStarPointerPointing2POnPressButton,
            MR::isStarPointerPointing2POnTriggerButton,
        };
        input.set_pointer(WPAD_CHAN0, 10.0F, 20.0F, true);
        input.set_button_mask(WPAD_CHAN0, WPAD_BUTTON_A);
        for (const auto query : queries) {
            if (query(&actor, "Pointer", true, true)) {
                throw std::runtime_error("disconnected channel inherited player-one pointing");
            }
        }
        input.set_connected(WPAD_CHAN1, true);
        for (const auto query : queries) {
            bool rejected = false;
            try {
                (void)query(&actor, "Pointer", true, true);
            } catch (const std::logic_error&) {
                rejected = true;
            }
            if (!rejected) {
                throw std::runtime_error("connected pointing fabricated an absent result");
            }
        }
        input.set_connected(WPAD_CHAN1, false);
        for (const auto query : queries) {
            if (query(&actor, nullptr, false, false)) {
                throw std::runtime_error("disconnected channel retained pointing state");
            }
        }
        smgpc::compat::release_actor_runtime_state(&actor);
        if (actor.mStarPointerTarget != nullptr || service.has_target(actor)) {
            throw std::runtime_error("actor retirement retained a pointer target");
        }
        input = saved_input;
        std::cout << "StarPointer device boundary tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        input = saved_input;
        std::cerr << "StarPointer device boundary tests failed: " << error.what() << '\n';
        return 1;
    }
}
