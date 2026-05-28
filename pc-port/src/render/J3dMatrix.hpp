#pragma once

#include "render/J3dMaterialRuntime.hpp"

#include <revolution.h>

namespace smgpc::render {

    [[nodiscard]] J3dMatrix3x4 j3d_matrix_from_mtx(MtxPtr matrix);
    [[nodiscard]] J3dMatrix3x4 j3d_rotation_matrix(float axis_x, float axis_y, float axis_z, float radians);
    [[nodiscard]] J3dMatrix3x4 j3d_concat_matrix(const J3dMatrix3x4 &left, const J3dMatrix3x4 &right);
    [[nodiscard]] J3dMatrix3x4 j3d_invert_orthonormal_matrix(const J3dMatrix3x4 &matrix);
    [[nodiscard]] J3dMatrix3x4 j3d_invert_affine_matrix(const J3dMatrix3x4 &matrix);
    [[nodiscard]] J3dMatrix3x4 j3d_apply_matrix_scale(const J3dMatrix3x4 &matrix, float scale_x, float scale_y, float scale_z);
    [[nodiscard]] J3dMatrix3x4 j3d_remove_matrix_scale(const J3dMatrix3x4 &matrix, float scale_x, float scale_y, float scale_z);

}  // namespace smgpc::render
