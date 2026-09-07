#include "compat/MetrowerksStdCompat.hpp"

#include "Game/Util/MathUtil.hpp"

#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    bool near(float actual, float expected, float tolerance = 0.00002F) {
        return std::fabs(actual - expected) <= tolerance;
    }

    void require_vector(const TVec3f& actual, const TVec3f& expected, std::string_view message,
                        float tolerance = 0.00002F) {
        require(near(actual.x, expected.x, tolerance) && near(actual.y, expected.y, tolerance) &&
                    near(actual.z, expected.z, tolerance), message);
    }

    void require_basis(const TQuat4f& quaternion) {
        auto side = TVec3f{};
        auto up = TVec3f{};
        auto front = TVec3f{};
        quaternion.getXDir(side);
        quaternion.getYDir(up);
        quaternion.getZDir(front);
        require(near(side.length(), 1.0F) && near(up.length(), 1.0F) && near(front.length(), 1.0F) &&
                    near(side.dot(up), 0.0F) && near(up.dot(front), 0.0F) && near(front.dot(side), 0.0F),
                "quaternion blends must retain a finite orthonormal actor basis");
        require_vector(side.cross(up), front, "quaternion blends must retain the handedness of the actor basis");
    }

    void test_quaternion_world_rotation_and_aliasing() {
        const auto half_root = std::sqrt(0.5F);
        const auto pitch = TQuat4f{half_root, 0.0F, 0.0F, half_root};
        const auto yaw = TQuat4f{0.0F, half_root, 0.0F, half_root};
        auto result = yaw;
        result.mult(pitch);
        auto front = TVec3f{};
        result.getZDir(front);
        require_vector(front, TVec3f{1.0F, 0.0F, 0.0F},
                       "mult(rotation) must apply a world rotation on the left of the current orientation");
        require(near(result.x, 0.5F) && near(result.y, 0.5F) && near(result.z, 0.5F) && near(result.w, 0.5F),
                "a quarter pitch after a quarter yaw must retain the ordered Hamilton product");

        auto alias_left = pitch;
        alias_left.mult(alias_left, yaw);
        auto alias_right = yaw;
        alias_right.mult(pitch, alias_right);
        require(alias_left.x == result.x && alias_left.y == result.y && alias_left.z == result.z &&
                    alias_left.w == result.w && alias_right.x == result.x && alias_right.y == result.y &&
                    alias_right.z == result.z && alias_right.w == result.w,
                "both quaternion multiplication input aliases must preserve every input component");
        alias_left.set(alias_left);
        require(alias_left.w == result.w, "quaternion copy must support self assignment");
    }

    void test_quaternion_normalization_and_slerp() {
        auto quaternion = TQuat4f{0.001F, 0.0F, 0.0F, 0.0F};
        quaternion.normalize();
        require(quaternion.x == 0.0F && quaternion.y == 0.0F && quaternion.z == 0.0F && quaternion.w == 1.0F,
                "quaternions below the retail squared epsilon must normalize to identity");
        quaternion.normalize(TQuat4f{0.0F, 3.0F, 0.0F, 4.0F});
        require(near(quaternion.y, 0.6F) && near(quaternion.w, 0.8F),
                "separate-source quaternion normalization must preserve the original component ratios");

        const auto source = TQuat4f{0.0F, 0.0F, 0.0F, 2.0F};
        const auto target = TQuat4f{0.0F, 3.0F, 0.0F, 0.0F};
        quaternion = source;
        quaternion.slerp(target, 0.5F);
        require(near(quaternion.x, 0.0F) && near(quaternion.y, std::sqrt(0.5F)) &&
                    near(quaternion.z, 0.0F) && near(quaternion.w, std::sqrt(0.5F)),
                "slerp must normalize both nonunit endpoints before interpolating a quarter turn");
        quaternion = source;
        quaternion.slerp(target, 0.0F);
        require(quaternion.y == 0.0F && quaternion.w == 1.0F,
                "slerp at zero must return the normalized source");
        quaternion = source;
        quaternion.slerp(target, 1.0F);
        require(quaternion.y == 1.0F && quaternion.w == 0.0F,
                "slerp at one must return the normalized target");
        quaternion = source;
        quaternion.slerp(target, 1.5F);
        require(near(quaternion.y, std::sqrt(0.5F)) && near(quaternion.w, -std::sqrt(0.5F)),
                "slerp must retain authored rates beyond one instead of clamping the turn");
        quaternion = source;
        quaternion.slerp(target, -0.5F);
        require(near(quaternion.y, -std::sqrt(0.5F)) && near(quaternion.w, std::sqrt(0.5F)),
                "slerp must retain negative authored rates");

        const auto quarter = std::sqrt(0.5F);
        auto positive = TQuat4f{};
        positive.slerp(TQuat4f{0.0F, quarter, 0.0F, quarter}, 0.5F);
        auto negative = TQuat4f{};
        negative.slerp(TQuat4f{0.0F, -quarter, 0.0F, -quarter}, 0.5F);
        require(positive.x == negative.x && positive.y == negative.y && positive.z == negative.z &&
                    positive.w == negative.w && negative.y > 0.0F && negative.w > 0.0F,
                "a negative quaternion dot must select the same short turn as its positive-hemisphere equivalent");

        quaternion = TQuat4f{};
        quaternion.slerp(TQuat4f{0.0F, 0.001953125F, 0.0F, 0.9999980926513671875F}, 0.25F);
        require(quaternion.y == 0.00048828125F && quaternion.w == 0.999999523162841796875F &&
                    quaternion.squared() < 0.9999995F,
                "the epsilon branch must preserve its unnormalized linear result");
        quaternion = TQuat4f{};
        quaternion.slerp(TQuat4f{0.0F, 0.00390625F, 0.0F, 0.99999237060546875F}, 0.25F);
        require(quaternion.w > 1.0001F && quaternion.squared() > 1.0003F,
                "a dot outside the retail epsilon must use table-based spherical weights without final normalization");

        auto alias = TQuat4f{0.0F, 3.0F, 0.0F, 4.0F};
        alias.slerp(alias, 0.25F);
        require(near(alias.y, 0.6F) && near(alias.w, 0.8F),
                "in-place slerp must copy both endpoints before overwriting an aliased target");
        auto separate = TQuat4f{};
        separate.slerp(source, target, 0.25F);
        alias = source;
        alias.slerp(alias, target, 0.25F);
        require(alias.x == separate.x && alias.y == separate.y && alias.z == separate.z && alias.w == separate.w,
                "the three-argument overload must preserve source aliasing while delegating to recovered slerp");
        alias = target;
        alias.slerp(source, alias, 0.25F);
        require(alias.x == 0.0F && alias.y == 0.0F && alias.z == 0.0F && alias.w == 1.0F,
                "the three-argument overload must retain the retail copy-source-before-target alias ordering");
    }

    void test_quaternion_matrix_assignment() {
        auto matrix = TPos3f{};
        matrix.makeTrans(4.0F, 5.0F, 6.0F);
        const auto quaternion = TQuat4f{0.0F, 1.0F, 0.0F, 1.0F};
        matrix.setQuat(quaternion);
        require(matrix.mMtx[0][0] == -1.0F && matrix.mMtx[0][2] == 2.0F && matrix.mMtx[1][1] == 1.0F &&
                    matrix.mMtx[2][0] == -2.0F && matrix.mMtx[2][2] == -1.0F,
                "quaternion-to-matrix assignment must use the original components without implicit normalization");
        require(matrix.mMtx[0][3] == 4.0F && matrix.mMtx[1][3] == 5.0F && matrix.mMtx[2][3] == 6.0F,
                "setQuat must preserve the existing affine translation");
        matrix.makeQuat(quaternion);
        require(matrix.mMtx[0][3] == 0.0F && matrix.mMtx[1][3] == 0.0F && matrix.mMtx[2][3] == 0.0F,
                "makeQuat must clear affine translation before writing the rotation");
        matrix.setQT(TQuat4f{}, TVec3f{7.0F, 8.0F, 9.0F});
        require(matrix.mMtx[0][0] == 1.0F && matrix.mMtx[1][1] == 1.0F && matrix.mMtx[2][2] == 1.0F &&
                    matrix.mMtx[0][3] == 7.0F && matrix.mMtx[1][3] == 8.0F && matrix.mMtx[2][3] == 9.0F,
                "setQT must assign the recovered rotation and translation together");
    }

    void test_camera_direction_rotation_and_transform() {
        auto rotation = TQuat4f{};
        rotation.setRotate(TVec3f{0.0F, 0.0F, 4.0F}, TVec3f{3.0F, 0.0F, 0.0F});
        auto output = TVec3f{};
        const auto input = TVec3f{0.0F, 0.0F, 2.0F};
        rotation.transform(input, output);
        require_vector(output, TVec3f{2.0F, 0.0F, 0.0F},
                       "camera direction rotation must accept nonunit directions and preserve vector magnitude");
        auto alias = input;
        rotation.transform(alias, alias);
        require_vector(alias, output, "quaternion transform must support a separate destination aliased to its source");
        alias = input;
        rotation.transform(alias);
        require_vector(alias, output, "the one-vector transform must share the original Hamilton-product semantics");
        rotation.setRotate(TVec3f{1.0F, 0.0F, 0.0F}, TVec3f{-1.0F, 0.0F, 0.0F});
        require(rotation.x == 0.0F && rotation.y == 0.0F && rotation.z == 0.0F && rotation.w == 1.0F,
                "the recovered two-direction rotation must retain the original antiparallel identity branch");
        rotation.setRotate(TVec3f{}, TVec3f{1.0F, 0.0F, 0.0F});
        require(rotation.w == 1.0F, "a zero direction must use the original small-cross-product identity branch");

        rotation.set(0.0F, 0.0F, 0.0F, 2.0F);
        rotation.transform(TVec3f{1.0F, 2.0F, 3.0F}, output);
        require_vector(output, TVec3f{4.0F, 8.0F, 12.0F},
                       "quaternion transform must preserve raw nonunit quaternion scaling");
        require(!MR::isNan(TVec3f{INFINITY, 0.0F, -INFINITY}) && MR::isNan(TVec3f{0.0F, NAN, 0.0F}),
                "camera pose validation must identify NaN components without reclassifying infinity as NaN");
    }

    void test_camera_trigonometric_tables() {
        const auto sample = static_cast<float>(std::atan(341.0 / 1024.0));
        const auto half_pi = PI * 0.5F;
        require(JMAATan2(1.0F, 3.0F) == sample && JMAATan2(3.0F, 1.0F) == half_pi - sample &&
                    JMAATan2(3.0F, -1.0F) == half_pi + sample && JMAATan2(1.0F, -3.0F) == PI - sample &&
                    JMAATan2(-1.0F, -3.0F) == -PI + sample && JMAATan2(-3.0F, -1.0F) == -half_pi - sample &&
                    JMAATan2(-3.0F, 1.0F) == -half_pi + sample && JMAATan2(-1.0F, 3.0F) == -sample,
                "camera polar angles must preserve the original quantized atan table in all eight octants");
        require(JMAATan2(0.0F, 0.0F) == 0.0F && !std::signbit(JMAATan2(-0.0F, 1.0F)) &&
                    JMAATan2(-0.0F, -1.0F) == PI && JMAATan2(1.0F, 0.0F) == half_pi &&
                    JMAATan2(-1.0F, 0.0F) == -half_pi,
                "atan quadrants must retain the retail axis and signed-zero decisions");
        require(JMAATan2(1.0F, 1.0F) == PI * 0.25F && JMAATan2(-1.0F, 1.0F) == -PI * 0.25F,
                "equal direction components must use the separate atan endpoint cell");
        const auto cell_boundary = 171.5F / 1024.0F;
        require(JMath::sAtanTable.get_(std::nextafter(cell_boundary, 0.0F), 1.0F) ==
                    static_cast<float>(std::atan(171.0 / 1024.0)) &&
                    JMath::sAtanTable.get_(cell_boundary, 1.0F) == static_cast<float>(std::atan(172.0 / 1024.0)),
                "atan ratio lookup must round half a table cell upward");
        require(std::fabs(JMAATan2(1.0F, 3.0F) - std::atan2(1.0F, 3.0F)) > 0.0002F,
                "the retail table angle must remain distinguishable from a host libm replacement");

        require(std::bit_cast<std::uint32_t>(JMath::sSinCosTable.table[6000].a1) == 0x3F3EBC1BU &&
                    std::bit_cast<std::uint32_t>(JMath::sSinCosTable.table[6000].b1) == 0xBF2AC083U &&
                    std::bit_cast<std::uint32_t>(JMath::sSinCosTable.table[12287].b1) == 0xB9C8FE41U,
                "sin/cos samples must retain double angle evaluation with the original float-rounded two-pi constant");
        const auto degree_boundary = std::bit_cast<float>(std::uint32_t{0x3DE0FFFFU});
        require(JMASinDegree(degree_boundary) == JMath::sSinCosTable.table[5].a1 &&
                    JMACosDegree(degree_boundary) == JMath::sSinCosTable.table[5].b1 &&
                    JMASinDegree(-degree_boundary) == -JMath::sSinCosTable.table[5].a1,
                "degree lookup must multiply by the original index scale directly at cell boundaries");
        require(JMASinLap(0.25F) == JMath::sSinCosTable.table[4096].a1 &&
                    JMASinLap(-0.25F) == -JMath::sSinCosTable.table[4096].a1 &&
                    JMACosLap(1.25F) == JMath::sSinCosTable.table[4096].b1,
                "lap lookup must preserve direct table indexing, sign, and full-turn wrapping");
    }

    void test_actor_up_then_front_blend() {
        auto quaternion = TQuat4f{};
        // DemoRabbit::control uses these exact rates. The geometric result is
        // a 9-degree pitch followed by an 18-degree turn about the new up axis.
        MR::blendQuatUpFront(&quaternion, TVec3f{0.0F, 0.0F, 1.0F}, TVec3f{1.0F, 0.0F, 0.0F}, 0.1F, 0.2F);
        const auto pitch = PI / 20.0F;
        const auto yaw = PI / 10.0F;
        auto up = TVec3f{};
        auto front = TVec3f{};
        quaternion.getYDir(up);
        quaternion.getZDir(front);
        require_vector(up, TVec3f{0.0F, std::cos(pitch), std::sin(pitch)},
                       "actor up blending must rotate by its fraction of the angle to gravity");
        require_vector(front, TVec3f{std::sin(yaw), -std::sin(pitch) * std::cos(yaw),
                                    std::cos(pitch) * std::cos(yaw)},
                       "front blending must use the up axis produced by the preceding gravity rotation");
        require_basis(quaternion);

        const auto source = quaternion;
        auto separate = TQuat4f{};
        MR::blendQuatUpFront(&separate, source, TVec3f{0.0F, 1.0F, 0.0F}, TVec3f{0.0F, 0.0F, 1.0F}, 0.1F, 0.2F);
        MR::blendQuatUpFront(&quaternion, TVec3f{0.0F, 1.0F, 0.0F}, TVec3f{0.0F, 0.0F, 1.0F}, 0.1F, 0.2F);
        require(quaternion.x == separate.x && quaternion.y == separate.y && quaternion.z == separate.z &&
                    quaternion.w == separate.w,
                "the separate-source and in-place actor quaternion overloads must have identical semantics");

        quaternion = TQuat4f{};
        MR::blendQuatUpFront(&quaternion, TVec3f{0.0F, 1.0F, 0.0F}, TVec3f{1.0F, 0.0F, 0.0F}, 0.0F, 1.5F);
        quaternion.getZDir(front);
        require_vector(front, TVec3f{std::sqrt(0.5F), 0.0F, -std::sqrt(0.5F)},
                       "actor rotation rates must retain the retail extrapolation contract");

        const auto before_zero = quaternion;
        MR::blendQuatUpFront(&quaternion, TVec3f{}, TVec3f{}, 0.1F, 0.2F);
        require(near(quaternion.x, before_zero.x) && near(quaternion.y, before_zero.y) &&
                    near(quaternion.z, before_zero.z) && near(quaternion.w, before_zero.w),
                "zero target directions must preserve the source orientation");
        require_basis(quaternion);

        quaternion = TQuat4f{};
        MR::blendQuatUpFront(&quaternion, TVec3f{0.0F, -1.0F, 0.0F}, TVec3f{0.0F, 0.0F, 1.0F}, 0.1F, 0.0F);
        quaternion.getYDir(up);
        require(near(up.y, std::cos(PI * 0.1F), 0.0004F),
                "opposite gravity must use the retail perturbation and begin a fractional half turn");
        require_basis(quaternion);

        quaternion = TQuat4f{};
        MR::blendQuatUpFront(&quaternion, TVec3f{0.0F, 1.0F, 0.0F}, TVec3f{0.0F, 0.0F, -1.0F}, 0.0F, 0.2F);
        quaternion.getZDir(front);
        require(near(front.z, std::cos(PI * 0.2F), 0.0004F),
                "opposite facing must use the retail perturbation instead of sticking at the old front");
        require_basis(quaternion);
    }

    void test_spherical_vector_blend() {
        auto result = TVec3f{};
        require(MR::vecBlendSphere(TVec3f{2.0F, 0.0F, 0.0F}, TVec3f{0.0F, 4.0F, 0.0F}, &result, 0.5F),
                "orthogonal vectors must have a valid spherical blend");
        require_vector(result, TVec3f{3.0F / std::sqrt(5.0F), 6.0F / std::sqrt(5.0F), 0.0F},
                       "spherical interpolation must weight the original magnitudes before restoring blended length");
        require(near(result.length(), 3.0F), "spherical interpolation must restore interpolated vector length");

        require(MR::vecBlendSphere(TVec3f{}, TVec3f{0.0F, 4.0F, 0.0F}, &result, 0.5F),
                "one zero input must take the successful normalized-linear branch");
        require_vector(result, TVec3f{0.0F, 2.0F, 0.0F}, "a zero endpoint must retain interpolated magnitude");
        require(MR::vecBlendSphere(TVec3f{}, TVec3f{}, &result, 0.5F),
                "two zero inputs must remain a successful zero blend");
        require_vector(result, TVec3f{}, "two zero vectors must stay zero");

        const auto small_angle = 0.05F;
        const auto small_target = TVec3f{4.0F * std::cos(small_angle), 4.0F * std::sin(small_angle), 0.0F};
        require(MR::vecBlendSphere(TVec3f{2.0F, 0.0F, 0.0F}, small_target, &result, 0.25F),
                "angles under a tenth radian must use the normalized-linear path");
        const auto weighted_x = 0.75F + 0.25F * std::cos(small_angle);
        const auto weighted_y = 0.25F * std::sin(small_angle);
        const auto scale = 2.5F / std::hypot(weighted_x, weighted_y);
        require_vector(result, TVec3f{weighted_x * scale, weighted_y * scale, 0.0F},
                       "the near-parallel branch must blend normalized directions independently of endpoint lengths");

        result.set(3.0F, 4.0F, 5.0F);
        require(!MR::vecBlendSphere(TVec3f{1.0F, 0.0F, 0.0F}, TVec3f{-1.0F, 0.0F, 0.0F}, &result, 0.5F),
                "exactly opposite directions must report their ambiguous great-circle path");
        require_vector(result, TVec3f{3.0F, 4.0F, 5.0F}, "a failed opposite-direction blend must leave output untouched");
        require(MR::vecBlendSphere(TVec3f{1.0F, 0.0F, 0.0F}, TVec3f{-std::cos(0.01F), std::sin(0.01F), 0.0F},
                                  &result, 0.5F),
                "near-opposite directions must not be rejected by a widened cosine threshold");
        require(result.y > 0.999F && near(result.length(), 1.0F),
                "near-opposite blending must keep the real turn direction finite");

        auto alias = TVec3f{2.0F, 0.0F, 0.0F};
        require(MR::vecBlendSphere(alias, TVec3f{0.0F, 4.0F, 0.0F}, &alias, 0.5F),
                "spherical blending must allow the output to alias the source");
        require_vector(alias, TVec3f{3.0F / std::sqrt(5.0F), 6.0F / std::sqrt(5.0F), 0.0F},
                       "in-place spherical blending must preserve original input magnitude");
    }

    void test_axis_rotation_snap_and_sign() {
        auto result = TVec3f{};
        const auto source = TVec3f{2.0F, 0.0F, 0.0F};
        const auto target = TVec3f{0.0F, 4.0F, 0.0F};
        const auto axis = TVec3f{0.0F, 0.0F, 1.0F};
        MR::vecRotAxis(source, target, axis, &result, PI * 0.5F);
        require_vector(result, target, "a reachable target must copy the target including its magnitude");
        MR::vecRotAxis(source, target, axis, &result, PI * 0.25F);
        require_vector(result, TVec3f{std::sqrt(2.0F), std::sqrt(2.0F), 0.0F},
                       "a bounded rotation must retain source magnitude until reaching the target");
        MR::vecRotAxis(source, -target, axis, &result, PI * 0.25F);
        require_vector(result, TVec3f{std::sqrt(2.0F), -std::sqrt(2.0F), 0.0F},
                       "the cross product must select a negative turn around the supplied axis");
        MR::vecRotAxis(source, -source, axis, &result, PI * 0.25F);
        require_vector(result, TVec3f{std::sqrt(2.0F), std::sqrt(2.0F), 0.0F},
                       "opposite directions must use the positive authored rotation axis");
        MR::vecRotAxis(TVec3f{}, target, axis, &result, 0.1F);
        require_vector(result, target, "a zero source must copy the target");
        MR::vecRotAxis(source, TVec3f{}, axis, &result, 0.1F);
        require_vector(result, TVec3f{}, "a zero target must zero the output");
        result = source;
        MR::vecRotAxis(result, target, axis, &result, PI * 0.25F);
        require_vector(result, TVec3f{std::sqrt(2.0F), std::sqrt(2.0F), 0.0F},
                       "the SDK matrix rotation must preserve in-place vector calls");
    }

    void test_original_angle_boundaries() {
        require(MR::normalizeAngleAbs(TWO_PI) == TWO_PI &&
                    MR::normalizeAngleAbs(2.0F * TWO_PI) == TWO_PI,
                "positive full turns retain the original inclusive upper endpoint");
        require(MR::normalizeAngleAbs(-TWO_PI) == 0.0F &&
                    std::signbit(MR::normalizeAngleAbs(-0.0F)),
                "negative full turns and signed zero retain original wrapping semantics");
        require(near(MR::blendAngle(0.25F, TWO_PI - 0.25F, 0.5F), TWO_PI),
                "angle interpolation crosses the wrap along the short arc");
        require(near(MR::blendAngle(0.5F, 1.5F, 0.25F), 0.75F),
                "ordinary angle interpolation retains its requested weight");
        require(near(MR::convergeRadian(1.0F, 1.125F, 0.25F), 1.125F),
                "angle convergence must stop at its target without overshoot");
        require(near(MR::convergeRadian(TWO_PI - 0.125F, 0.125F, 0.25F), 0.125F),
                "angle convergence follows the target across a full turn");
        TVec2f direction(3.0F, 4.0F);
        MR::normalize(&direction);
        require(near(direction.x, 0.6F) && near(direction.y, 0.8F),
                "two-dimensional normalization uses the shared original vector path");
    }

    void test_near_parallel_angle_table() {
        MR::initAcosTable();
        // 0.999 selects retail table entry 242, whose ratio is 12737/12750.
        const auto expected = static_cast<float>(std::acos(12737.0 / 12750.0));
        require(near(MR::acosEx(0.999F), expected, 0.0000001F),
                "near-parallel acos must use the retail table grid");
        require(near(MR::acosEx(-0.999F), PI - expected, 0.0000003F),
                "near-opposite acos must reflect the same retail grid around pi");
        require(MR::diffAngleAbs(TVec3f{2.0F, 0.0F, 0.0F}, TVec3f{4.0F, 0.0F, 0.0F}) == 0.0F &&
                    MR::diffAngleAbs(TVec3f{2.0F, 0.0F, 0.0F}, TVec3f{-4.0F, 0.0F, 0.0F}) == PI,
                "exact parallel and opposite angle clamps must precede table lookup");
        require(near(JMath::sAsinAcosTable.mTable[512], static_cast<float>(std::asin(0.5)), 0.0000001F),
                "the shared asin table must sample index divided by 1024 in double precision");
        require(near(JMath::sAsinAcosTable.mTable[1023], static_cast<float>(std::asin(1023.0 / 1024.0)), 0.0000001F),
                "the last regular asin sample must remain below the unit endpoint");
        const auto acos_half = PI * 0.5F - static_cast<float>(std::asin(511.0 / 1024.0));
        require(JMath::sAsinAcosTable.acos_(0.5F) == acos_half && JGeometry::TUtil<float>::acos(0.5F) == acos_half,
                "the shared acos paths must select the original truncated 1023.5-scaled table index");
    }
}  // namespace

int main() {
    try {
        test_quaternion_world_rotation_and_aliasing();
        test_quaternion_normalization_and_slerp();
        test_quaternion_matrix_assignment();
        test_camera_direction_rotation_and_transform();
        test_camera_trigonometric_tables();
        test_actor_up_then_front_blend();
        test_spherical_vector_blend();
        test_axis_rotation_snap_and_sign();
        test_near_parallel_angle_table();
        test_original_angle_boundaries();
        std::cout << "game math rotation tests passed\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "game math rotation tests failed: " << exception.what() << '\n';
        return 1;
    }
}
