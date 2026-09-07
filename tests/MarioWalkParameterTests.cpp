#include "MarioWalkParameterTests.hpp"

#include "Game/Player/MarioActor.hpp"
#include "Game/Player/MarioConst.hpp"

#include <stdexcept>
#include <string>
#include <string_view>

namespace smgpc::tests {
    namespace {
        void require(bool condition, std::string_view message) {
            if (!condition) {
                throw std::runtime_error(std::string(message));
            }
        }

        class ScopedWalkParameters {
        public:
            explicit ScopedWalkParameters(MarioActor& actor)
                : mario(*actor.mMario), table(*actor.mConst->getTable()), saved_table(table),
                  saved_states(mario.mMovementStates), saved_ground(mario.mGroundPolygon), saved_status(mario._97C),
                  saved_speed(mario.mWalkSpeed), saved_blend(mario._3F4), saved_reduction(mario._2D0),
                  saved_ground_time(mario._3CE), saved_reflect_time(mario._3F8), saved_spin_time(mario._3FA),
                  saved_spin_cooldown(mario._3FC), saved_dash_time(mario._434), saved_floor(mario._960),
                  saved_speed_index(mario.mTargetWalkSpeedIndex), saved_sink_time(mario.mSinkTimer) {
            }

            ~ScopedWalkParameters() {
                table = saved_table;
                mario.mMovementStates = saved_states;
                mario.mGroundPolygon = saved_ground;
                mario._97C = saved_status;
                mario.mWalkSpeed = saved_speed;
                mario._3F4 = saved_blend;
                mario._2D0 = saved_reduction;
                mario._3CE = saved_ground_time;
                mario._3F8 = saved_reflect_time;
                mario._3FA = saved_spin_time;
                mario._3FC = saved_spin_cooldown;
                mario._434 = saved_dash_time;
                mario._960 = saved_floor;
                mario.mTargetWalkSpeedIndex = saved_speed_index;
                mario.mSinkTimer = saved_sink_time;
            }

            void reset_inputs() {
                mario.mMovementStates = {};
                mario.mGroundPolygon = nullptr;
                mario._97C = nullptr;
                mario.mWalkSpeed = 0.5F;
                mario._3F4 = 0.0F;
                mario._2D0 = 0.0F;
                mario._3CE = 30;
                mario._3F8 = 0;
                mario._3FA = 0;
                mario._3FC = 0;
                mario._434 = 0;
                mario._960 = 0;
                mario.mTargetWalkSpeedIndex = 7;
                mario.mSinkTimer = 0;
            }

            Mario& mario;
            MarioConstTable& table;

        private:
            const MarioConstTable saved_table;
            const Mario::MovementStates saved_states;
            Triangle* const saved_ground;
            MarioState* const saved_status;
            const float saved_speed;
            const float saved_blend;
            const float saved_reduction;
            const u16 saved_ground_time;
            const u16 saved_reflect_time;
            const u16 saved_spin_time;
            const u16 saved_spin_cooldown;
            const u16 saved_dash_time;
            const u16 saved_floor;
            const u8 saved_speed_index;
            const u8 saved_sink_time;
        };
    }

    void verify_original_mario_walk_parameters(MarioActor& actor) {
        require(actor.mMario != nullptr && actor.mConst != nullptr && actor.mMario->_95C != nullptr,
                "walking parameter checks require an initialized original Mario actor");
        ScopedWalkParameters parameters(actor);
        auto& mario = parameters.mario;
        auto& table = parameters.table;
        table.mItemDashRatio = 1.5F;
        table.mInertiaStandardStop = 0.25F;
        table.mInertiaStandardMax = 0.5F;
        table.mInertiaStartSpin = 0.75F;
        table.mInertiaOverSpeed = 0.875F;
        table.mInertiaStop = 0.0625F;
        table.mInertiaTurnSlip = 0.125F;
        table.mInertiaTurning = 0.1875F;
        table.mInertiaJumpFinish = 0.3125F;
        table.mInertiaSquat = 0.4375F;
        table.mInertiaReflectSlip = 0.5625F;
        table.mInertiaTornadoBrake = 0.6875F;
        table.mInertiaTornadoAccel = 0.8125F;
        table.mInertiaIceStandardStop = 0.625F;
        table.mInertiaIceStandardMax = 0.75F;
        table.mInertiaIceStop = 0.9375F;
        table.mInertiaSlipStandardStop = 0.5F;
        table.mInertiaSlipStandardMax = 0.75F;
        table.mInertiaSlipStop = 0.3125F;
        table.mStartSpinTime = 3;

        parameters.reset_inputs();
        require(mario.getTargetWalkSpeed() == 1.0F, "the maximum authored walking band must select unit target speed");
        mario.mSinkTimer = 128;
        mario._2D0 = 0.25F;
        mario._434 = 1;
        require(mario.getTargetWalkSpeed() == 0.5625F,
                "target speed must combine sink depth, movement reduction, and item dash in the original order");
        mario.mMovementStates._A = true;
        require(mario.getTargetWalkSpeed() == 0.0F, "squat must override the other target speed modifiers");
        parameters.reset_inputs();
        mario.mSinkTimer = 255;
        require(mario.getTargetWalkSpeed() == 1.0F / 256.0F, "sink depth must retain the original 256-step scale");
        parameters.reset_inputs();
        mario._2D0 = 1.25F;
        require(mario.getTargetWalkSpeed() == -0.25F, "the target helper must not add an authored-reduction clamp");

        parameters.reset_inputs();
        require(mario.decideInertia(0.5F) == 0.375F, "normal inertia must interpolate the two walking-speed endpoints");
        mario._3F4 = 0.5F;
        require(mario.decideInertia(0.5F) == 0.5625F, "start-spin blending must retain its original interpolation weight");
        parameters.reset_inputs();
        require(mario.decideInertia(0.0F) == 0.0625F, "idle input must select stop inertia");
        mario.mMovementStates._10 = true;
        require(mario.decideInertia(0.0F) == 0.125F, "turn-slip braking must override idle stop inertia");
        mario.mMovementStates._4 = true;
        require(mario.decideInertia(0.0F) == 0.1875F, "turning must override turn-slip braking");
        mario._3CE = 29;
        require(mario.decideInertia(0.0F) == 0.3125F, "recent landing must override the turn-braking modes");
        mario.mMovementStates._A = true;
        require(mario.decideInertia(0.0F) == 0.4375F, "squat must retain the last zero-input braking priority");

        parameters.reset_inputs();
        mario.mMovementStates._F = true;
        require(mario.decideInertia(0.25F) == 0.6875F && mario.decideInertia(0.75F) == 0.8125F,
                "tornado inertia must distinguish braking from acceleration");
        parameters.reset_inputs();
        mario._3F8 = 2;
        require(mario.decideInertia(0.5F) == 0.5625F && mario._3F8 == 1,
                "reflection inertia must consume exactly one frame of its timer");
        parameters.reset_inputs();
        mario.mWalkSpeed = 0.0F;
        mario._3CE = 11;
        mario.mMovementStates._C = true;
        require(mario.decideInertia(1.0F) == 0.75F && mario.mWalkSpeed == 0.08F && mario._3FA == 2,
                "a qualified start spin must latch its original minimum speed and consume its first timer frame");
        mario._3FA = 1;
        require(mario.decideInertia(1.0F) == 0.75F && mario._3FA == 0 && mario._3FC == 59,
                "start-spin completion must create and immediately advance the sixty-frame cooldown");

        parameters.reset_inputs();
        mario.mMovementStates._34 = true;
        require(mario.decideInertia(1.5F) == 0.75F && mario.decideInertia(0.0F) == 0.9375F,
                "ice inertia must use its original helper and clamp full-stick input there");
        mario.mWalkSpeed = 1.25F;
        require(mario.decideInertia(0.5F) == 0.875F, "overspeed must precede the ice-ground flag branch");
        parameters.reset_inputs();
        mario.mMovementStates._35 = true;
        require(mario.decideInertia(0.5F) == 0.625F && mario.decideInertia(0.0F) == 0.3125F,
                "slippery-ground inertia must delegate to the original speed and stop formulas");
    }
}
