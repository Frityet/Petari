#include "compat/MetrowerksStdCompat.hpp"

#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"

#include <math_types.hpp>

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    bool near(f32 actual, f32 expected, f32 tolerance = 0.00001F) {
        return std::fabs(actual - expected) <= tolerance;
    }

    void test_vector_decomposition_and_projection() {
        auto direction = TVec3f{3.0F, 4.0F, 0.0F};
        auto scalar = 0.0F;
        MR::separateScalarAndDirection(&scalar, &direction, direction);
        require(near(scalar, 5.0F) && direction.epsilonEquals(TVec3f{0.6F, 0.8F, 0.0F}, 0.00001F),
                "scalar/direction decomposition must remain safe when source and destination alias");

        auto foot = TVec3f{};
        auto parameter = MR::calcPerpendicFootToLineInside(&foot, TVec3f{4.0F, 3.0F, 0.0F},
                                                           TVec3f{0.0F, 0.0F, 0.0F},
                                                           TVec3f{10.0F, 0.0F, 0.0F});
        require(near(parameter, 0.4F) && foot.epsilonEquals(TVec3f{4.0F, 0.0F, 0.0F}, 0.00001F),
                "inside-line projection must return the retail normalized segment parameter");

        parameter = MR::calcPerpendicFootToLineInside(&foot, TVec3f{-2.0F, 3.0F, 0.0F},
                                                       TVec3f{0.0F, 0.0F, 0.0F},
                                                       TVec3f{10.0F, 0.0F, 0.0F});
        require(parameter == 0.0F && foot.epsilonEquals(TVec3f{0.0F, 0.0F, 0.0F}, 0.0F),
                "inside-line projection must clamp before the segment tip");

        parameter = MR::calcPerpendicFootToLineInside(&foot, TVec3f{12.0F, -3.0F, 0.0F},
                                                       TVec3f{0.0F, 0.0F, 0.0F},
                                                       TVec3f{10.0F, 0.0F, 0.0F});
        require(parameter == 1.0F && foot.epsilonEquals(TVec3f{10.0F, 0.0F, 0.0F}, 0.0F),
                "inside-line projection must clamp beyond the segment tail");

        parameter = MR::calcPerpendicFootToLineInside(&foot, TVec3f{4.0F, 3.0F, 2.0F},
                                                       TVec3f{1.0F, 1.0F, 1.0F},
                                                       TVec3f{1.0F, 1.0F, 1.0F});
        require(std::isnan(parameter) && std::isnan(foot.x) && std::isnan(foot.y) && std::isnan(foot.z),
                "a degenerate retail segment must propagate its undefined normalized parameter");
    }

    void test_gravity_scalar_math_surface() {
        require(MR::getMaxElement(TVec3f{-8.0F, -2.0F, -4.0F}) == -2.0F,
                "maximum-vector-element lookup must preserve signed values");

        const auto nan = std::numeric_limits<f32>::quiet_NaN();
        require(MR::getMaxElement(TVec3f{nan, 3.0F, 2.0F}) == 3.0F,
                "maximum-vector-element lookup must follow retail ordering when x is NaN");
        require(MR::getMaxElement(TVec3f{4.0F, nan, 2.0F}) == 2.0F,
                "maximum-vector-element lookup must follow retail ordering when y is NaN");
        require(std::isnan(MR::getMaxElement(TVec3f{4.0F, 3.0F, nan})),
                "maximum-vector-element lookup must select a NaN z after unordered comparisons");
        require(std::signbit(MR::getMaxElement(TVec3f{0.0F, -0.0F, -1.0F})),
                "an x/y maximum tie must select y exactly as the retail comparison tree does");
        require(near(MR::cosDegree(60.0F), 0.5F) && near(MR::sqrt(81.0F), 9.0F) && MR::sqrt(-4.0F) == -4.0F,
                "degree cosine and host square root must preserve the retail utility contract");
        require(FLOAT_MAX > 3.0e38F && FLOAT_ZERO == 0.0F && near(PI_180 * 180.0F, PI, 0.000001F),
                "math_types constants must have one canonical compatibility definition");
        require(gZeroVec.x == 0.0F && gZeroVec.y == 0.0F && gZeroVec.z == 0.0F,
                "the excluded retail MathUtil translation unit must retain its canonical zero-vector provider");
    }

    void test_vector_gravity_helpers() {
        require(TVec3f{0.001F, 0.0F, 0.0F}.isZero() && !TVec3f{0.002F, 0.0F, 0.0F}.isZero(),
                "TVec3f::isZero must use the original squared epsilon threshold");

        auto orthogonal = TVec3f{};
        orthogonal.killElement2(TVec3f{3.0F, 4.0F, 5.0F}, TVec3f{0.0F, 1.0F, 0.0F});
        require(orthogonal.epsilonEquals(TVec3f{3.0F, 0.0F, 5.0F}, 0.0F),
                "killElement2 must remove the component parallel to its normalized direction");
    }

    void test_rotation_and_prescale() {
        auto position = TPos3f{};
        position.makeTrans(4.0F, 5.0F, 6.0F);
        require(position.mMtx[0][0] == 1.0F && position.mMtx[1][1] == 1.0F &&
                    position.mMtx[2][2] == 1.0F && position.mMtx[0][3] == 4.0F &&
                    position.mMtx[1][3] == 5.0F && position.mMtx[2][3] == 6.0F,
                "TPos3f translation construction must reset the affine basis and retain all three offsets");
        position.makeRotate(TVec3f{1.0F, 0.0F, 0.0F}, PI * 0.5F);
        require(near(position.mMtx[0][0], 1.0F) && near(position.mMtx[1][1], 0.0F) &&
                    near(position.mMtx[1][2], -1.0F) && near(position.mMtx[2][1], 1.0F) &&
                    near(position.mMtx[2][2], 0.0F) && position.mMtx[0][3] == 0.0F &&
                    position.mMtx[1][3] == 0.0F && position.mMtx[2][3] == 0.0F,
                "TPos3f axis-angle construction must use the retail rotation signs and clear translation");

        Mtx matrix{};
        MR::makeMtxRotate(matrix, TVec3f{0.0F, 90.0F, 0.0F});

        Mtx explicitlyQuantized{};
        const auto rotation = TVec3f{12.345F, -67.891F, 23.456F};
        MR::makeMtxRotate(matrix, rotation);
        MR::makeMtxRotate(explicitlyQuantized, static_cast<s16>(rotation.x * DEGREE_TO_S16),
                          static_cast<s16>(rotation.y * DEGREE_TO_S16),
                          static_cast<s16>(rotation.z * DEGREE_TO_S16));
        for (auto row = 0; row < 3; ++row) {
            for (auto column = 0; column < 4; ++column) {
                require(matrix[row][column] == explicitlyQuantized[row][column],
                        "floating-point makeMtxRotate must use the retail short-angle path");
            }
        }

        MR::makeMtxRotate(matrix, TVec3f{0.0F, 90.0F, 0.0F});
        matrix[0][3] = 7.0F;
        matrix[1][3] = 8.0F;
        matrix[2][3] = 9.0F;
        MR::preScaleMtx(matrix, TVec3f{2.0F, 3.0F, 4.0F});

        require(near(matrix[0][0], 0.0F) && near(matrix[0][2], 4.0F) && near(matrix[2][0], -2.0F) &&
                    near(matrix[2][2], 0.0F),
                "preScaleMtx must scale matrix columns in local-axis order");
        require(matrix[0][3] == 7.0F && matrix[1][3] == 8.0F && matrix[2][3] == 9.0F,
                "preScaleMtx must preserve affine translation");

        Mtx identity = {{1.0F, 0.0F, 0.0F, 4.0F},
                        {0.0F, 1.0F, 0.0F, 5.0F},
                        {0.0F, 0.0F, 1.0F, 6.0F}};
        MR::preScaleMtx(identity, 2.0F);
        require(identity[0][0] == 2.0F && identity[1][1] == 2.0F && identity[2][2] == 2.0F &&
                    identity[0][3] == 4.0F,
                "uniform preScaleMtx must delegate to the generalized three-axis implementation");
    }
}  // namespace

int main() {
    try {
        test_vector_decomposition_and_projection();
        test_gravity_scalar_math_surface();
        test_vector_gravity_helpers();
        test_rotation_and_prescale();
        std::cout << "gravity math foundation tests passed\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "gravity math foundation tests failed: " << exception.what() << '\n';
        return 1;
    }
}
