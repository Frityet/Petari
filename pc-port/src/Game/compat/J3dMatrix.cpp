#include "J3dMatrix.hpp"

#include <cmath>

namespace smgpc::game {

    J3dMatrix3x4 j3d_matrix_from_mtx(MtxPtr matrix) {
        if (matrix == nullptr) {
            return {};
        }

        return J3dMatrix3x4{{
            matrix[0][0],
            matrix[0][1],
            matrix[0][2],
            matrix[0][3],
            matrix[1][0],
            matrix[1][1],
            matrix[1][2],
            matrix[1][3],
            matrix[2][0],
            matrix[2][1],
            matrix[2][2],
            matrix[2][3],
        }};
    }

    J3dMatrix3x4 j3d_rotation_matrix(float axis_x, float axis_y, float axis_z, float radians) {
        const auto length = std::sqrt((axis_x * axis_x) + (axis_y * axis_y) + (axis_z * axis_z));
        if (length == 0.0F) {
            return {};
        }

        const auto x = axis_x / length;
        const auto y = axis_y / length;
        const auto z = axis_z / length;
        const auto sin = std::sin(radians);
        const auto cos = std::cos(radians);
        const auto one_minus_cos = 1.0F - cos;

        return J3dMatrix3x4{{
            (one_minus_cos * x * x) + cos,
            (one_minus_cos * x * y) - (sin * z),
            (one_minus_cos * x * z) + (sin * y),
            0.0F,
            (one_minus_cos * y * x) + (sin * z),
            (one_minus_cos * y * y) + cos,
            (one_minus_cos * y * z) - (sin * x),
            0.0F,
            (one_minus_cos * z * x) - (sin * y),
            (one_minus_cos * z * y) + (sin * x),
            (one_minus_cos * z * z) + cos,
            0.0F,
        }};
    }

    J3dMatrix3x4 j3d_concat_matrix(const J3dMatrix3x4& left, const J3dMatrix3x4& right) {
        auto result = J3dMatrix3x4{};
        for (auto row = 0U; row < 3U; ++row) {
            for (auto column = 0U; column < 3U; ++column) {
                result.m[row * 4U + column] = (left.m[row * 4U + 0U] * right.m[column + 0U]) +
                                              (left.m[row * 4U + 1U] * right.m[column + 4U]) +
                                              (left.m[row * 4U + 2U] * right.m[column + 8U]);
            }
            result.m[row * 4U + 3U] = (left.m[row * 4U + 0U] * right.m[3U]) +
                                      (left.m[row * 4U + 1U] * right.m[7U]) +
                                      (left.m[row * 4U + 2U] * right.m[11U]) + left.m[row * 4U + 3U];
        }

        return result;
    }

    J3dMatrix3x4 j3d_invert_orthonormal_matrix(const J3dMatrix3x4& matrix) {
        auto result = J3dMatrix3x4{{
            matrix.m[0U],
            matrix.m[4U],
            matrix.m[8U],
            0.0F,
            matrix.m[1U],
            matrix.m[5U],
            matrix.m[9U],
            0.0F,
            matrix.m[2U],
            matrix.m[6U],
            matrix.m[10U],
            0.0F,
        }};

        result.m[3U] = -((result.m[0U] * matrix.m[3U]) + (result.m[1U] * matrix.m[7U]) + (result.m[2U] * matrix.m[11U]));
        result.m[7U] = -((result.m[4U] * matrix.m[3U]) + (result.m[5U] * matrix.m[7U]) + (result.m[6U] * matrix.m[11U]));
        result.m[11U] = -((result.m[8U] * matrix.m[3U]) + (result.m[9U] * matrix.m[7U]) + (result.m[10U] * matrix.m[11U]));

        return result;
    }

    J3dMatrix3x4 j3d_apply_matrix_scale(const J3dMatrix3x4& matrix, float scale_x, float scale_y, float scale_z) {
        return J3dMatrix3x4{{
            matrix.m[0U] * scale_x,
            matrix.m[1U] * scale_y,
            matrix.m[2U] * scale_z,
            matrix.m[3U],
            matrix.m[4U] * scale_x,
            matrix.m[5U] * scale_y,
            matrix.m[6U] * scale_z,
            matrix.m[7U],
            matrix.m[8U] * scale_x,
            matrix.m[9U] * scale_y,
            matrix.m[10U] * scale_z,
            matrix.m[11U],
        }};
    }

}  // namespace smgpc::game
