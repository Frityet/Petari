#include "compat/MetrowerksStdCompat.hpp"
#include "OriginalSceneControllerFixture.hpp"
#include "SceneExecutionFixture.hpp"
#include "Game/Gravity/PlanetGravity.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/LiveActor/Spine.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/GravityUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {
    unsigned failures;
    void check(bool condition, const char* message) {
        if (!condition) {
            std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    }
    bool same_bits(float a, float b) {
        return std::bit_cast<std::uint32_t>(a) == std::bit_cast<std::uint32_t>(b);
    }
    bool same_bits(const TVec3f& a, const TVec3f& b) {
        return same_bits(a.x, b.x) && same_bits(a.y, b.y) && same_bits(a.z, b.z);
    }
    class UtilityActor final : public LiveActor {
    public:
        UtilityActor() : LiveActor("original utility probe") {}
        void control() override { mFlag.mIsDead = true; }
    };
    class ConstantGravity final : public PlanetGravity {
    public:
        bool calcOwnGravityVector(TVec3f* out, f32* distance, const TVec3f&) const override {
            if (observer) observed_calculation_flag = observer->mFlag.mIsCalcGravity;
            *out = direction;
            *distance = 100.0f;
            return true;
        }
        TVec3f direction{1.0f, 2.0f, 3.0f};
        const LiveActor* observer = nullptr;
        mutable bool observed_calculation_flag = false;
    };
    class IdleNerve final : public Nerve {
        void execute(Spine*) const override {}
    };

    void check_original_binder_integration() {
        // Exercise the public original method under the existing real scene
        // owner. These cases deliberately need no collision-query provider.
        LiveActor actor("original Binder integration probe");
        const TVec3f start{7.0f, -13.0f, 29.0f};
        const TVec3f velocity{0.25f, -1.5f, 2.0f};
        TVec3f expected(start);
        expected += velocity;
        for (bool dead : {false, true}) {
            actor.mPosition = start;
            actor.mVelocity = velocity;
            actor.mFlag.mIsDead = dead;
            actor.updateBinder();
            check(same_bits(actor.mPosition, expected),
                  "public updateBinder integrates one velocity even when called on a dead actor");
            check(same_bits(actor.mVelocity, velocity), "updateBinder does not rewrite velocity");
        }

        actor.initBinder(10.0f, 0.0f, 4);
        actor.mFlag.mIsNoBind = true;
        for (bool dead : {false, true}) {
            actor.mPosition = start;
            actor.mVelocity = velocity;
            actor.mFlag.mIsDead = dead;
            actor.mBinder->mPlaneNum = 3;
            actor.mBinder->_C8 = 2.0f;
            actor.mBinder->_158 = 3.0f;
            actor.mBinder->_1E8 = 4.0f;
            actor.mBinder->mFixReactionVector.set(1.0f, 2.0f, 3.0f);
            actor.updateBinder();
            check(same_bits(actor.mPosition, expected), "disabled binding integrates exactly one original velocity");
            check(actor.mBinder->mPlaneNum == 0 && !actor.mBinder->isBindedGround() &&
                      !actor.mBinder->isBindedWall() && !actor.mBinder->isBindedRoof() &&
                      same_bits(actor.mBinder->mFixReactionVector, TVec3f(0.0f)),
                  "disabled binding clears every previous contact and reaction, including explicit dead-actor calls");
        }

        UtilityActor dies_in_control;
        dies_in_control.mFlag.mIsDead = false;
        dies_in_control.mPosition = start;
        dies_in_control.mVelocity = velocity;
        dies_in_control.movement();
        check(same_bits(dies_in_control.mPosition, start),
              "ordinary movement keeps its original death-after-control guard before integration");

        LiveActor stepping("original per-movement step probe");
        const IdleNerve idle;
        stepping.initNerve(&idle);
        stepping.mFlag.mIsDead = false;
        stepping.mPosition = start;
        stepping.mVelocity = velocity;
        TVec3f accumulated(start);
        for (int step = 1; step <= 8; ++step) {
            stepping.movement();
            accumulated += velocity;
            check(stepping.getNerveStep() == step && same_bits(stepping.mPosition, accumulated),
                  "each original movement call advances exactly one nerve step and one velocity displacement");
        }
        std::cout << "original Binder integration and per-call nerve/displacement checks completed\n";
    }

    void run_gravity_checks() {
        const auto heaps = smgpc::compat::JkrHeapRuntime::create(16U << 20);
        smgpc::test::OriginalSceneControllerFixture original(heaps);
        smgpc::runtime::SceneScheduler scheduler;
        smgpc::runtime::SceneSchedulerBinding active(scheduler);
        const auto domain = smgpc::compat::JkrAllocationDomain::create(heaps, 1U << 20);
        smgpc::test::SceneExecutionFixture scene(scheduler, domain,
                                               &original.scene);
        if (!MR::createSceneObj(SceneObj_PlanetGravityManager))
            throw std::runtime_error("utility tests require the actual scene-owned gravity manager");
        if (!MR::createSceneObj(SceneObj_ClippingDirector))
            throw std::runtime_error("utility actors require the original clipping registration owner");
        check_original_binder_integration();
        UtilityActor actor;
        const TVec3f previous{0.125f, -0.25f, 0.5f};
        actor.mGravity = previous;
        actor.mFlag.mIsDead = false;
        MR::onCalcGravity(&actor);
        check(same_bits(actor.mGravity, previous), "onCalcGravity keeps gravity when the actual manager has no field");
        ConstantGravity field;
        MR::registerGravity(&field);
        field.observer = &actor;
        actor.mFlag.mIsCalcGravity = false;
        MR::onCalcGravity(&actor);
        check(!field.observed_calculation_flag && actor.mFlag.mIsCalcGravity,
              "onCalcGravity calculates before setting its flag, as the original does");

        unsigned changed_components = 0;
        for (unsigned i = 1; i <= 128; ++i) {
            field.direction.set(i * 0.371f, (i % 11 + 1) * -0.827f, (i % 7 + 1) * 0.213f);
            TVec3f expected;
            MR::calcGravityVector(&actor, actor.mPosition, &expected, nullptr, 0);
            actor.mFlag.mIsDead = false;
            MR::onCalcGravity(&actor);
            changed_components += !same_bits(actor.mGravity, expected);
        }
        std::cout << "manager-output bit mismatches: " << changed_components << "/128\n";
        check(changed_components == 0, "onCalcGravity stores the original manager output without a second normalization");

        actor.mGravity = previous;
        actor.mFlag.mIsDead = true;
        actor.mFlag.mIsCalcGravity = false;
        MR::onCalcGravity(&actor);
        check(same_bits(actor.mGravity, previous) && actor.mFlag.mIsCalcGravity,
              "onCalcGravity on a dead actor sets its flag without calculating");
        TVec3f expected;
        MR::calcGravityVector(&actor, actor.mPosition, &expected, nullptr, 0);
        actor.movement();
        check(same_bits(actor.mGravity, expected),
              "LiveActor movement calculates flagged gravity before its original dead-actor return");
        actor.mGravity = previous;
        actor.mFlag.mIsDead = false;
        MR::offCalcGravity(&actor);
        actor.movement();
        check(same_bits(actor.mGravity, previous), "movement leaves gravity unchanged while calculation is disabled");

        for (const auto direction : {TVec3f(0.0f), TVec3f(1.0e-12f, -1.0e-12f, 0.0f)}) {
            field.direction = direction;
            actor.mFlag.mIsDead = false;
            actor.mGravity = previous;
            MR::onCalcGravity(&actor);
            check(same_bits(actor.mGravity, previous), "zero/near-zero manager results retain the original actor gravity");
        }
        field.mActivated = false;
        actor.mFlag.mIsDead = false;
        actor.mGravity = previous;
        MR::onCalcGravity(&actor);
        check(same_bits(actor.mGravity, previous), "inactive fields preserve the original no-field behavior");
    }

    void run_easing_checks() {
        UtilityActor actor;
        const IdleNerve idle;
        actor.initNerve(&idle);
        unsigned mismatches = 0;
        for (s32 maximum : {0, -1, 73}) {
            for (s32 step : {-1, 0, 1, 2, 17, 36, 72, 73, 90}) {
                actor.mSpine->mStep = step;
                const float rate = maximum <= 0 ? 1.0f : MR::clamp(static_cast<float>(step) / maximum, 0.0f, 1.0f);
                const float expected = MR::getEaseInValue(rate, 0.0f, 1.0f, 1.0f);
                mismatches += !same_bits(MR::calcNerveEaseInRate(&actor, maximum), expected);
            }
        }
        std::cout << "table-easing bit mismatches: " << mismatches << "/27\n";
        check(mismatches == 0, "LiveActor ease-in uses the original shared JMath interpolation for clamped nerve steps");
    }
}

int main() {
    try {
        run_gravity_checks();
        run_easing_checks();
        if (failures) return 1;
        std::cout << "original actor utility checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "fixture error: " << error.what() << '\n';
        return 2;
    }
}
