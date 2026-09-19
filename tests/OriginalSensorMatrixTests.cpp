#include "compat/MetrowerksStdCompat.hpp"
#include "compat/Cp932Literal.hpp"
#include "app/Application.hpp"
#include "app/OriginalGameApplication.hpp"
#include "Game/Camera/CameraContext.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/SensorHitChecker.hpp"
#include "Game/Player/Mario.hpp"
#include "Game/Player/MarioAccess.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Scene/GameScene.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Screen/StarPointerController.hpp"
#include "Game/Screen/StarPointerDirector.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/GameSystemObjHolder.hpp"
#include "Game/System/GameSystemSceneController.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/StarPointerUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "scene/NameObjChildOwner.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

#include <aurora/allocation.hpp>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unistd.h>
#include <vector>

namespace {
    void require(bool value, const char* message) {
        if (!value) throw std::runtime_error(message);
    }

    void near(float actual, float expected, const char* message, float tolerance = 0.0001F) {
        if (!std::isfinite(actual) || std::abs(actual - expected) > tolerance) {
            std::fprintf(stderr, "%s: actual=%g expected=%g\n", message, actual, expected);
            throw std::runtime_error(message);
        }
    }

    void near(const TVec3f& actual, const TVec3f& expected, const char* message) {
        near(actual.x, expected.x, message);
        near(actual.y, expected.y, message);
        near(actual.z, expected.z, message);
    }

    void verify_directional_scale() {
        TPos3f matrix;
        matrix.makeTrans(11, 12, 13);
        matrix.setScale(2, 3, 5);
        TVec3f translation;
        matrix.getTrans(translation);
        near(translation, TVec3f(11, 12, 13), "Original setScale preserves affine translation");

        // The direction is the scale basis Y axis. For this basis the two
        // perpendicular axes are (0,-.8,.6) and (-1,0,0), respectively.
        MR::scaleMtxToDir(&matrix, TVec3f(0, 0.6F, 0.8F), TVec3f(2, 3, 5));
        const float expected[3][4]{{5, 0, 0, 0}, {0, 2.36F, 0.48F, 0}, {0, 0.48F, 2.64F, 0}};
        for (unsigned row = 0; row < 3; ++row)
            for (unsigned column = 0; column < 4; ++column)
                near(matrix.mMtx[row][column], expected[row][column], "Directional scale agrees with analytic basis scaling");
        TVec3f transformed;
        matrix.mult(TVec3f(0, 0.6F, 0.8F), transformed);
        near(transformed, TVec3f(0, 1.8F, 2.4F), "Authored scale Y acts along the supplied direction");

        MR::scaleMtxToDir(&matrix, TVec3f(0, 2, 0), TVec3f(2, 3, 5));
        near(matrix.mMtx[0][0], 5, "Perpendicular basis preserves scale Z");
        near(matrix.mMtx[1][1], 12, "Original helper preserves the supplied direction magnitude");
        near(matrix.mMtx[2][2], 2, "Perpendicular basis preserves scale X");
        MR::scaleMtxToDir(&matrix, TVec3f(0, 0, 1), TVec3f(1, 1, 1));
        for (unsigned row = 0; row < 3; ++row)
            for (unsigned column = 0; column < 3; ++column)
                near(matrix.mMtx[row][column], row == column ? 1 : 0, "Axis fallback retains identity for unit scale");
    }

    void verify_orthogonalize() {
        TPos3f matrix;
        matrix.makeTrans(11, 12, 13);
        matrix.setXYZDir(TVec3f(99, -17, 6), TVec3f(1, 2, 3), TVec3f(0, 0, 4));
        MR::orthogonalize(&matrix);
        TVec3f x, y, z, translation;
        matrix.getXYZDir(x, y, z);
        matrix.getTrans(translation);
        const float inverse_root5 = 1.0F / std::sqrt(5.0F);
        near(x, TVec3f(2 * inverse_root5, -inverse_root5, 0), "Orthogonalize reconstructs X from Y cross Z");
        near(y, TVec3f(inverse_root5, 2 * inverse_root5, 0), "Orthogonalize reconstructs Y with original handedness");
        near(z, TVec3f(0, 0, 4), "Orthogonalize restores the original Z magnitude");
        near(translation, TVec3f(11, 12, 13), "Orthogonalize preserves translation");
        near(x.dot(y), 0, "Reconstructed axes are perpendicular");
    }

    void verify_quaternion_turn() {
        const TQuat4f identity(0, 0, 0, 1);
        for (const float rate : {0.0F, 0.5F, 1.0F, -0.5F, 1.5F}) {
            TQuat4f result;
            MR::turnQuatYDirRate(&result, identity, TVec3f(3, 0, 0), rate);
            TVec3f y;
            result.getYDir(y);
            const float angle = rate * 1.57079632679F;
            near(y, TVec3f(std::sin(angle), std::cos(angle), 0), "Quaternion turn retains authored rates without clamping");
            auto alias = identity;
            MR::turnQuatYDirRate(&alias, alias, TVec3f(3, 0, 0), rate);
            require(alias.x == result.x && alias.y == result.y && alias.z == result.z && alias.w == result.w,
                    "Original turn supports destination/source aliasing");
        }
        const float half_root = std::sqrt(0.5F);
        const TQuat4f yaw(0, half_root, 0, half_root);
        TQuat4f turned;
        MR::turnQuatYDirRate(&turned, yaw, TVec3f(1, 0, 0), 1);
        TVec3f front;
        turned.getZDir(front);
        near(front, TVec3f(0, -1, 0), "Correction multiplies on the world side and preserves the existing roll");
        for (const auto target : {TVec3f(0, -1, 0), TVec3f(0, 0, 0)}) {
            MR::turnQuatYDirRate(&turned, yaw, target, 1);
            require(turned.x == yaw.x && turned.y == yaw.y && turned.z == yaw.z && turned.w == yaw.w,
                    "Parallel-degenerate cross branch preserves original orientation");
        }
        MR::turnQuatYDirRate(&turned, TQuat4f(0, 0, 0, 2), TVec3f(1, 0, 0), 1);
        near(turned.squared(), 4, "Rate helper does not silently normalize its input quaternion");
    }

    void verify_matrix_turn() {
        TPos3f matrix;
        matrix.makeTrans(11, 12, 13);
        MR::turnMtxToYDirRate(&matrix, TVec3f(1, 0, 0), 0.5F);
        TVec3f y, translation;
        matrix.getYDir(y);
        matrix.getTrans(translation);
        const float half_root = std::sqrt(0.5F);
        near(y, TVec3f(half_root, half_root, 0), "Matrix helper turns its actual Y basis by the requested rate");
        near(translation, TVec3f(11, 12, 13), "Matrix turn preserves translation through quaternion conversion");
    }

#ifndef NDEBUG
    struct Receipt {
        HitSensor* receiver;
        HitSensor* sender;
        TVec3f sender_position;
        u32 message;
    };

    class Receiver final : public LiveActor {
    public:
        Receiver(const char* name, std::vector<Receipt>& receipts) : LiveActor(name), receipts(receipts) {
            initHitSensor(1);
            auto* sensor = MR::addHitSensorEnemy(this, "body", 4, 10, TVec3f(0, 0, 0));
            sensor->validateBySystem();
        }
        bool receiveMessage(u32 message, HitSensor* sender, HitSensor* receiver) override {
            receipts.push_back({receiver, sender, sender->mPosition, message});
            if (on_message) on_message();
            return accepts;
        }
        std::vector<Receipt>& receipts;
        std::function<void()> on_message;
        bool accepts = true;
    };

    void verify_sensor_messages() {
        const auto domain = smgpc::scene::current_scene_allocation_domain();
        require(domain != nullptr, "Sensors require the actual original scene allocation domain");
        auto* checker = static_cast<SensorHitChecker*>(MR::getSceneObjHolder()->getObj(SceneObj_SensorHitChecker));
        require(checker != nullptr, "Original scene owns its SensorHitChecker");
        const auto actors_before = smgpc::compat::actor_runtime_state_count();
        const auto names_before = smgpc::compat::name_obj_runtime_state_count();
        const auto sensors_before = checker->mCharacterGroup->mSensorCount;
        std::vector<Receipt> receipts;
        receipts.reserve(32);
        smgpc::scene::NameObjChildOwner actors;
        std::array<Receiver*, 4> owners{};
        actors.capture_construction_children([&] {
            const smgpc::compat::JkrAllocationScope game(domain);
            for (auto& owner : owners) owner = new Receiver("sensor utility probe", receipts);
            owners[0]->initBinder(10, 0, 6);
        });
        auto* sender = owners[0]->getSensor("body");
        auto* first = owners[1]->getSensor("body");
        auto* second = owners[2]->getSensor("body");
        auto* third = owners[3]->getSensor("body");
        require(checker->mCharacterGroup->mSensorCount == sensors_before + 4 && sender->mSensorGroup == checker->mCharacterGroup,
                "Probe sensors join the original typed scene group");
        sender->mPosition.set(10, 20, 30);
        first->mPosition.set(5, 7, 9);
        second->mPosition.set(-4, -6, -8);

        using Send = bool (*)(HitSensor*, HitSensor*);
        const std::array<std::pair<Send, u32>, 3> messages{{
            {MR::sendMsgEnemyAttackFlipWeak, ACTMES_ENEMY_ATTACK_FLIP_WEAK},
            {MR::sendMsgEnemyAttackFlipWeakJump, ACTMES_ENEMY_ATTACK_FLIP_WEAK_JUMP},
            {MR::sendMsgToEnemyAttackBlow, ACTMES_TO_ENEMY_ATTACK_BLOW},
        }};
        for (const auto& [send, message] : messages) {
            for (const bool accepts : {false, true}) {
                receipts.clear();
                owners[1]->accepts = accepts;
                require(send(first, sender) == accepts && receipts.size() == 1 && receipts[0].message == message &&
                            receipts[0].receiver == first && receipts[0].sender == sender,
                        "Attack wrapper forwards actual sensors, message and receiver result");
                near(receipts[0].sender_position, TVec3f(10, 20, 30), "Ordinary wrapper preserves sender position");
            }
        }

        receipts.clear();
        owners[1]->accepts = false;
        owners[1]->on_message = [&] {
            near(sender->mPosition, TVec3f(3, 10, 5), "Directional wrapper exposes receiver minus raw direction");
            require(MR::sendMsgEnemyAttackFlipMaximumToDir(second, sender, TVec3f(1, 2, 3)),
                    "Nested directional wrapper returns its own receiver result");
            near(sender->mPosition, TVec3f(3, 10, 5), "Nested wrapper restores the enclosing directional position");
        };
        require(!MR::sendMsgEnemyAttackFlipToDir(first, sender, TVec3f(2, -3, 4)),
                "Outer directional wrapper retains a rejected receiver result");
        owners[1]->on_message = nullptr;
        require(receipts.size() == 2 && receipts[0].message == ACTMES_ENEMY_ATTACK_FLIP &&
                    receipts[1].message == ACTMES_ENEMY_ATTACK_FLIP_MAXIMUM,
                "Directional wrappers preserve their distinct original messages");
        near(receipts[1].sender_position, TVec3f(-5, -8, -11), "Nested wrapper uses its own receiver and direction");
        near(sender->mPosition, TVec3f(10, 20, 30), "Directional dispatch restores the actual sender position");
        near(owners[0]->mPosition, TVec3f(0, 0, 0), "Directional dispatch never substitutes actor translation");

        auto* binder = owners[0]->mBinder;
        receipts.clear();
        require(!MR::sendMsgEnemyAttackToBindedSensor(owners[0], sender) && receipts.empty(),
                "An empty original Binder reports no accepted messages");
        const HitSensor* contacts[]{second, first, second, third, first, third};
        for (unsigned index = 0; index < std::size(contacts); ++index)
            binder->mPlane[index].mParentTriangle.mSensor = const_cast<HitSensor*>(contacts[index]);
        binder->mPlaneNum = std::size(contacts);
        owners[1]->accepts = false;
        owners[2]->accepts = true;
        owners[3]->accepts = false;
        require(MR::sendMsgEnemyAttackToBindedSensor(owners[0], sender) && receipts.size() == 3,
                "Original Binder contact sorting sends once to each unique sensor and combines receiver results");
        for (const auto& receipt : receipts)
            require(receipt.message == ACTMES_ENEMY_ATTACK && receipt.sender == sender,
                    "Every bound sensor receives the original enemy-attack message and sender");
        require(receipts[0].receiver != receipts[1].receiver && receipts[1].receiver != receipts[2].receiver &&
                    receipts[0].receiver != receipts[2].receiver,
                "Repeated contact planes do not duplicate sensor dispatch");
        receipts.clear();
        owners[2]->accepts = false;
        require(!MR::sendMsgEnemyAttackToBindedSensor(owners[0], sender) && receipts.size() == 3,
                "All-rejected bound messages still visit every unique sensor");
        actors.clear();
        require(checker->mCharacterGroup->mSensorCount == sensors_before &&
                    smgpc::compat::actor_runtime_state_count() == actors_before &&
                    smgpc::compat::name_obj_runtime_state_count() == names_before,
                "Actor retirement releases sensors, group membership, Binder and native ownership");
    }

    void verify_retail_retention() {
        auto* actor = MarioAccess::getPlayerActor();
        require(actor && actor->mMario, "Retention regression requires the actual scene MarioActor and Mario");
        auto& mario = *actor->mMario;
        require(!mario.isSwimming() && !mario.isAnimationRun(CP932("その場足踏み")) &&
                    !mario.isAnimationRun(CP932("ターンブレーキ")),
                "Retention threshold fixture requires an ordinary original animation");
        const auto movement = mario.mMovementStates;
        const auto flags = mario._10;
        const auto draw = mario.mDrawStates;
        const auto stick = mario.mStickPos;
        const auto angle = mario._2B4;
        const auto landing = mario._3CE;
        const auto counter = mario._40E;
        const auto weak_turn = mario._3D4;
        const auto retained_ground = mario._29C;
        const auto retained_gravity = mario._2A8;
        // Only the helper's scalar/vector state is varied. Its real actor,
        // animator, gravity and scene owners remain in place, and every field
        // is restored before normal GameSystem execution resumes, even on fail.
        struct Restore { std::function<void()> apply; ~Restore() { apply(); } } restore{[&] {
            mario.mMovementStates = movement;
            mario._10 = flags;
            mario.mDrawStates = draw;
            mario.mStickPos = stick;
            mario._2B4 = angle;
            mario._3CE = landing;
            mario._40E = counter;
            mario._3D4 = weak_turn;
            mario._29C = retained_ground;
            mario._2A8 = retained_gravity;
        }};
        const auto exercise = [&](float previous_angle, unsigned landing_frames, unsigned countdown, bool retained) {
            mario.mMovementStates.jumping = false;
            mario._10._A = false;
            mario._10._B = false;
            mario.mDrawStates._9 = false;
            mario.mDrawStates._16 = false;
            mario.mDrawStates._D = false;
            mario.mStickPos.set(1, 0, 1);
            mario._2B4 = previous_angle;
            mario._3CE = landing_frames;
            mario._40E = countdown;
            mario._3D4 = 11;
            mario._29C.set(11, 22, 33);
            mario._2A8.set(44, 55, 66);
            TVec3f output(12, 34, 56);
            require(!mario.retainMoveDir(1, 0, &output), "Retail retention helper returns false on both state branches");
            near(output, TVec3f(12, 34, 56), "Retail retention updates turn state without replacing the output vector");
            require(mario.mDrawStates._16 && !mario.mDrawStates._9 &&
                        (mario.mDrawStates_WORD & 0x200) != 0 && (mario.mDrawStates_WORD & 0x00400000) == 0,
                    "Retail retention marks DrawStates mask0x200, not the unrelated mask0x00400000");
            near(mario._2B4, 0, "Retention stores the actual current stick angle");
            require(mario._40E == (retained ? (countdown ? countdown - 1 : 0) : 30),
                    "Retail 0.06/initial0.1 thresholds choose countdown versus reset");
            require(bool(mario.mDrawStates._D) == (retained && countdown == 0),
                    "Retained movement requests its draw flag only when the countdown expires");
            require(mario._3D4 == (retained || countdown ? 11 : 0),
                    "Only a reset with an expired countdown clears accumulated weak turn time");
            if (!retained || countdown) {
                near(mario._29C, mario._368, "Retention captures the real original ground direction");
                near(mario._2A8, *mario.getGravityVec(), "Retention captures the real original gravity owner");
            }
        };
        exercise(.04F, 3, 0, true);
        exercise(.08F, 3, 0, false);
        exercise(.08F, 1, 0, true);
        exercise(.04F, 3, 7, true);
    }

    struct Probe {
        bool exercised = false;
        unsigned pointer_moving = 0;
        unsigned pointer_stationary = 0;
        unsigned pointer_second_parallel = 0;

        void verify_pointer_plane(GameSystem& system, std::uint64_t frame) {
            require(system.mObjHolder && system.mObjHolder->mStarPointerDirector,
                    "Pointer probe requires the actual original system director");
            auto* director = system.mObjHolder->mStarPointerDirector;
            require(StarPointerFunction::getStarPointerDirector() == director,
                    "Pointer utilities resolve the actual process owner");
            const auto* pointer = director->getStarPointerController(0);
            auto* camera = static_cast<CameraContext*>(MR::getSceneObjHolder()->getObj(SceneObj_CameraContext));
            require(camera && &MR::getCameraInvViewMtx() == &camera->mViewInv,
                    "Projection uses the actual scene camera context");
            if (!pointer->mPastInfo.mInScreen) return;
            const auto position = pointer->mPastInfo.mPos;
            const auto velocity = pointer->mScreenVel;
            const float speed = std::hypot(velocity.x, velocity.y);
            if (speed != 0 && speed < 2) return;

            static_assert(!std::is_reference_v<decltype(position.subInline(velocity))>);
            const TVec2f& previous = position.subInline(velocity);
            near(previous.x, position.x - velocity.x, "Native subtraction returns a live value for the previous pointer X");
            near(previous.y, position.y - velocity.y, "Native subtraction returns a live value for the previous pointer Y");

            // Independent double-precision pinhole geometry. Unnormalized rays
            // intersect the plane directly, so this does not repeat the donor's
            // world-point subtraction and normalized-ray calculation.
            using Vector = std::array<double, 3>;
            const auto dot = [](const Vector& a, const Vector& b) {
                return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
            };
            const auto unit = [&](Vector value) {
                const double length = std::sqrt(dot(value, value));
                // Fused multiply/subtract may leave sub-picometre residuals
                // when two identical plane intersections cancel at O2.
                if (length < 1.0e-9) value.fill(0);
                else for (auto& component : value) component /= length;
                return value;
            };
            const auto native = [](const Vector& value) { return TVec3f(value[0], value[1], value[2]); };
            const auto& matrix = camera->mViewInv.mMtx;
            const Vector right{matrix[0][0], matrix[1][0], matrix[2][0]};
            const Vector up{matrix[0][1], matrix[1][1], matrix[2][1]};
            const Vector back{matrix[0][2], matrix[1][2], matrix[2][2]};
            const double focal = MR::getScreenHeight() * 0.5 /
                std::tan(camera->mFovy * 3.14159265358979323846 / 360.0);
            const auto ray = [&](const TVec2f& screen) {
                const double x = (screen.x - MR::getScreenWidth() * 0.5) / focal;
                const double y = -(screen.y - MR::getScreenHeight() * 0.5) / focal;
                return Vector{right[0] * x + up[0] * y - back[0],
                              right[1] * x + up[1] * y - back[1],
                              right[2] * x + up[2] * y - back[2]};
            };
            const Vector ray_a = ray(position), ray_b = ray(previous);
            const TVec3f base(matrix[0][3] - back[0] * 1000, matrix[1][3] - back[1] * 1000,
                             matrix[2][3] - back[2] * 1000);
            const Vector offset{double(base.x) - matrix[0][3], double(base.y) - matrix[1][3],
                                double(base.z) - matrix[2][3]};
            const Vector oblique = unit({back[0] + right[0] * .35 + up[0] * .2,
                                         back[1] + right[1] * .35 + up[1] * .2,
                                         back[2] + right[2] * .35 + up[2] * .2});
            const auto compare = [&](const TVec3f& result, const Vector& expected, const char* message) {
                near(result.x, expected[0], message, .003F);
                near(result.y, expected[1], message, .003F);
                near(result.z, expected[2], message, .003F);
            };
            for (const Vector normal : {back, oblique}) {
                const double a = dot(offset, normal) / dot(ray_a, normal);
                const double b = dot(offset, normal) / dot(ray_b, normal);
                const Vector expected = unit({ray_a[0] * a - ray_b[0] * b,
                                              ray_a[1] * a - ray_b[1] * b,
                                              ray_a[2] * a - ray_b[2] * b});
                TVec3f result(12, 34, 56);
                require(MR::calcStarPointerWorldVelocityDirectionOnPlane(&result, base, native(normal), 0),
                        "Actual pointer rays intersect a facing or oblique plane");
                compare(result, expected, "Pointer plane direction agrees with independent ray intersections");
                near(result.x * normal[0] + result.y * normal[1] + result.z * normal[2], 0,
                     "Pointer direction lies in the supplied plane", .003F);
                TVec3f reversed;
                require(MR::calcStarPointerWorldVelocityDirectionOnPlane(&reversed, base,
                            TVec3f(-normal[0] * 3, -normal[1] * 3, -normal[2] * 3), 0),
                        "Plane normal need not be unit length or face the camera");
                compare(reversed, expected, "Normal sign and magnitude preserve the geometric direction");
            }
            const auto cross_up = [&](const Vector& ray_value) {
                return unit(Vector{ray_value[1] * up[2] - ray_value[2] * up[1],
                                   ray_value[2] * up[0] - ray_value[0] * up[2],
                                   ray_value[0] * up[1] - ray_value[1] * up[0]});
            };
            for (const Vector normal : {cross_up(ray_a), Vector{0, 0, 0}}) {
                TVec3f result(12, 34, 56);
                require(!MR::calcStarPointerWorldVelocityDirectionOnPlane(&result, base, native(normal), 0),
                        "Parallel first ray and zero normal preserve the original failure result");
                near(result, TVec3f(12, 34, 56), "Failed pointer-plane query leaves the output untouched");
            }
            const Vector second_parallel = cross_up(ray_b);
            if (std::abs(dot(unit(ray_a), second_parallel)) > .005) {
                TVec3f result(12, 34, 56);
                require(!MR::calcStarPointerWorldVelocityDirectionOnPlane(&result, base, native(second_parallel), 0),
                        "A parallel previous ray fails after the current ray passes the first test");
                near(result, TVec3f(12, 34, 56), "Second-ray failure also preserves the output");
                ++pointer_second_parallel;
            }
            require(pointer->mPastInfo.mPos.x == position.x && pointer->mPastInfo.mPos.y == position.y &&
                        pointer->mScreenVel.x == velocity.x && pointer->mScreenVel.y == velocity.y,
                    "Projection only reads original controller inputs");
            if (speed == 0) ++pointer_stationary;
            else ++pointer_moving;
            if ((speed == 0 && pointer_stationary == 1) || (speed != 0 && pointer_moving == 1)) {
                std::fprintf(stderr, "[pointer-plane-probe] frame=%llu pos=%g,%g velocity=%g,%g fovy=%g PASS actual camera geometry\n",
                    static_cast<unsigned long long>(frame), position.x, position.y, velocity.x, velocity.y, camera->mFovy);
            }
        }

        void after_frame(GameSystem& system, std::uint64_t frame) {
            auto* controller = system.mSceneController;
            if (!controller || controller->mSceneInitializeState != SceneInitializeState_End ||
                controller->getCurrentSceneForExecute() != controller->mScene ||
                !dynamic_cast<GameScene*>(controller->mScene)) return;
            const aurora::allocation::HostAllocationScope host;
            verify_pointer_plane(system, frame);
            if (exercised) return;
            verify_sensor_messages();
            verify_sensor_messages();
            verify_retail_retention();
            exercised = true;
            std::fprintf(stderr, "[sensor-matrix-probe] PASS original sensor dispatch, nested directions, Binder deduplication and retirement twice; frame=%llu\n",
                         static_cast<unsigned long long>(frame));
            std::fprintf(stderr, "[retail-retention-probe] PASS actual Mario owner: flags, countdown, 0.06/initial0.1 thresholds and restored state\n");
        }
    };
#endif
}

int main(int argc, char* argv[]) try {
    verify_directional_scale();
    verify_orthogonalize();
    verify_quaternion_turn();
    verify_matrix_turn();
    std::puts("PASS four original matrix/quaternion utility cases");
    if (argc == 1) return 0;
    require(argc == 2 && std::strcmp(argv[1], "--original-sensors") == 0, "Expected --original-sensors or no argument");
#ifdef NDEBUG
    require(false, "Original-process sensor diagnostic requires a debug build");
#else
    const char* disc = std::getenv("SMGPC_REAL_DISC");
    require(disc && *disc, "SMGPC_REAL_DISC must name the real disc image");
    const auto save = std::filesystem::temp_directory_path() / ("petari-original-sensor-matrix-" + std::to_string(getpid()));
    require(!std::filesystem::exists(save), "Diagnostic must start with a fresh native console directory");
    setenv("SMGPC_SAVE_DIR", save.c_str(), 1);
    unsetenv("SMGPC_NAND_DIR");
    unsetenv("SMGPC_DEBUG_WPAD_BUTTON_SCRIPT");
    unsetenv("SMGPC_DEBUG_WPAD_POINTER_SCRIPT");
    unsetenv("SMGPC_DEBUG_WPAD_STICK_SCRIPT");
    unsetenv("SMGPC_DEBUG_WPAD_INPUT_FILE");
    std::string pointer_script = "0-59:260,180;";
    for (unsigned frame = 60; frame < 90; ++frame) {
        pointer_script += std::to_string(frame) + ":" + std::to_string(268 + 8 * (frame - 60)) + "," +
            std::to_string(184 + 4 * (frame - 60)) + ";";
    }
    pointer_script += "90-:500,300";
    setenv("SMGPC_DEBUG_WPAD_POINTER_SCRIPT", pointer_script.c_str(), 1);
    smgpc::app::BootstrapConfiguration configuration{
        .window_width = 640, .window_height = 456, .window_title = "Original sensor/matrix integration",
        .arguments = {"original-sensor-matrix-test", "--original", "--stage", "HeavensDoorGalaxy", "--scenario", "1", "--max-frames", "120"},
        .disc_image = disc,
    };
    auto logger = smgpc::logging::create_default_logger();
    smgpc::app::ensure_disc_image_open(configuration, *logger);
    struct Disc { ~Disc() { smgpc::app::close_disc_image(); } } disc_lifetime;
    Probe probe;
    const smgpc::app::OriginalGameDebugObserver observer{
        .context = &probe,
        .after_frame = +[](void* context, GameSystem& system, std::uint64_t frame) {
            static_cast<Probe*>(context)->after_frame(system, frame);
        },
    };
    require(smgpc::app::run_original_game(configuration, *logger, observer) == 0 && probe.exercised &&
                probe.pointer_moving >= 5 && probe.pointer_stationary >= 5 && probe.pointer_second_parallel >= 1,
            "OriginalProcess completed sensors and actual moving/stationary pointer-plane checks");
    std::printf("PASS original-process sensors and pointer plane integration; moving=%u stationary=%u second-parallel=%u\n",
                probe.pointer_moving, probe.pointer_stationary, probe.pointer_second_parallel);
#endif
    return 0;
} catch (const std::exception& error) {
    std::fprintf(stderr, "FAIL original sensor/matrix utilities: %s\n", error.what());
    return 1;
}
