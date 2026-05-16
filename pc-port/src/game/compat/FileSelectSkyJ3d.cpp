#include "FileSelectSkyJ3d.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "layout/Binary.hpp"
#include "layout/J3dTexture.hpp"

namespace smgpc::game::compat {
namespace {

    struct ProjectedVertex {
        float x{};
        float y{};
        float z{};
        float u{};
        float v{};
        float q{1.0F};
        float u_secondary{};
        float v_secondary{};
        float q_secondary{1.0F};
        std::uint32_t color{};
    };

    struct CameraVertex {
        float x{};
        float y{};
        float z{};
        float u{};
        float v{};
        float q{1.0F};
        float u_secondary{};
        float v_secondary{};
        float q_secondary{1.0F};
        std::uint32_t color{};
    };

    struct ClippedPolygon {
        std::array< CameraVertex, 16U > vertices{};
        std::size_t count{};
    };

    struct Vec3 {
        float x{};
        float y{};
        float z{};
    };

    struct GeneratedTexCoord {
        float s{};
        float t{};
        float q{1.0F};
    };

    struct Camera {
        Vec3 eye{};
        Vec3 target{};
        float fovy_degrees{};
    };

    struct FrustumClipParameters {
        float near_z{};
        float tan_half_fovy{};
        float aspect{};
    };

    enum class FrustumPlane {
        Near,
        Left,
        Right,
        Bottom,
        Top,
    };

    struct BckKey {
        float frame{};
        float value{};
        float tangent_in{};
        float tangent_out{};
    };

    struct BckTrack {
        bool valid{};
        std::vector< BckKey > keys{};
    };

    struct BckJoint {
        std::array< BckTrack, 3U > scale{};
        std::array< BckTrack, 3U > rotation{};
        std::array< BckTrack, 3U > translation{};
    };

    struct ParsedBck {
        std::vector< BckJoint > joints{};
        float frame_max{};
    };

    struct BtkMaterialAnimation {
        std::string material_name{};
        std::uint16_t material_id{assets::layout::J3D_NO_TEXTURE_INDEX};
        std::uint8_t texture_matrix_index{0xFFU};
        assets::layout::J3dVec3 center{0.5F, 0.5F, 0.5F};
        BckTrack scale_x{};
        BckTrack scale_y{};
        BckTrack rotation{};
        BckTrack translation_x{};
        BckTrack translation_y{};
    };

    struct ParsedBtk {
        std::vector< BtkMaterialAnimation > materials{};
        float frame_max{};
    };

    struct MaterialTrace {
        std::size_t accepted_triangle_count{};
        float min_x{std::numeric_limits< float >::max()};
        float min_y{std::numeric_limits< float >::max()};
        float max_x{-std::numeric_limits< float >::max()};
        float max_y{-std::numeric_limits< float >::max()};
    };

    struct MaterialTextureSelection {
        std::size_t texture_map_slot{};
        std::size_t texture_coordinate_slot{};
    };

    struct Mat34 {
        std::array< std::array< float, 4U >, 3U > m{};
    };

    struct Mat44 {
        std::array< std::array< float, 4U >, 4U > m{};
    };

    [[nodiscard]] Mat34 identity_matrix() {
        return Mat34{{
            std::array< float, 4U >{1.0F, 0.0F, 0.0F, 0.0F},
            std::array< float, 4U >{0.0F, 1.0F, 0.0F, 0.0F},
            std::array< float, 4U >{0.0F, 0.0F, 1.0F, 0.0F},
        }};
    }

    [[nodiscard]] Mat34 multiply_matrix(const Mat34& lhs, const Mat34& rhs) {
        Mat34 result{};
        for (std::size_t row = 0U; row < 3U; ++row) {
            for (std::size_t column = 0U; column < 3U; ++column) {
                result.m[row][column] = lhs.m[row][0U] * rhs.m[0U][column] + lhs.m[row][1U] * rhs.m[1U][column] +
                                        lhs.m[row][2U] * rhs.m[2U][column];
            }
            result.m[row][3U] = lhs.m[row][0U] * rhs.m[0U][3U] + lhs.m[row][1U] * rhs.m[1U][3U] +
                                lhs.m[row][2U] * rhs.m[2U][3U] + lhs.m[row][3U];
        }
        return result;
    }

    [[nodiscard]] Mat44 identity_matrix44() {
        return Mat44{{
            std::array< float, 4U >{1.0F, 0.0F, 0.0F, 0.0F},
            std::array< float, 4U >{0.0F, 1.0F, 0.0F, 0.0F},
            std::array< float, 4U >{0.0F, 0.0F, 1.0F, 0.0F},
            std::array< float, 4U >{0.0F, 0.0F, 0.0F, 1.0F},
        }};
    }

    [[nodiscard]] Mat44 mat44_from_mat34(const Mat34& matrix) {
        auto result = identity_matrix44();
        for (std::size_t row = 0U; row < 3U; ++row) {
            for (std::size_t column = 0U; column < 4U; ++column) {
                result.m[row][column] = matrix.m[row][column];
            }
        }
        return result;
    }

    [[nodiscard]] Mat44 mat44_from_effect_matrix(const std::array< float, 16U >& values) {
        Mat44 result{};
        for (std::size_t row = 0U; row < 4U; ++row) {
            for (std::size_t column = 0U; column < 4U; ++column) {
                result.m[row][column] = values[row * 4U + column];
            }
        }
        return result;
    }

    [[nodiscard]] Mat44 multiply_matrix(const Mat44& lhs, const Mat44& rhs) {
        Mat44 result{};
        for (std::size_t row = 0U; row < 4U; ++row) {
            for (std::size_t column = 0U; column < 4U; ++column) {
                result.m[row][column] = lhs.m[row][0U] * rhs.m[0U][column] + lhs.m[row][1U] * rhs.m[1U][column] +
                                        lhs.m[row][2U] * rhs.m[2U][column] + lhs.m[row][3U] * rhs.m[3U][column];
            }
        }
        return result;
    }

    [[nodiscard]] Mat34 multiply_matrix(const Mat34& lhs, const Mat44& rhs) {
        Mat34 result{};
        for (std::size_t row = 0U; row < 3U; ++row) {
            for (std::size_t column = 0U; column < 4U; ++column) {
                result.m[row][column] = lhs.m[row][0U] * rhs.m[0U][column] + lhs.m[row][1U] * rhs.m[1U][column] +
                                        lhs.m[row][2U] * rhs.m[2U][column] + lhs.m[row][3U] * rhs.m[3U][column];
            }
        }
        return result;
    }

    [[nodiscard]] assets::layout::J3dVec3 transform_point(const Mat34& matrix, const assets::layout::J3dVec3& point) {
        return assets::layout::J3dVec3{
            .x = matrix.m[0U][0U] * point.x + matrix.m[0U][1U] * point.y + matrix.m[0U][2U] * point.z + matrix.m[0U][3U],
            .y = matrix.m[1U][0U] * point.x + matrix.m[1U][1U] * point.y + matrix.m[1U][2U] * point.z + matrix.m[1U][3U],
            .z = matrix.m[2U][0U] * point.x + matrix.m[2U][1U] * point.y + matrix.m[2U][2U] * point.z + matrix.m[2U][3U],
        };
    }

    [[nodiscard]] Mat34 transpose_rotation(const Mat34& matrix) {
        Mat34 result = identity_matrix();
        for (std::size_t row = 0U; row < 3U; ++row) {
            for (std::size_t column = 0U; column < 3U; ++column) {
                result.m[row][column] = matrix.m[column][row];
            }
        }
        return result;
    }

    [[nodiscard]] Mat44 invert_affine_matrix(const Mat34& matrix) {
        const float a00 = matrix.m[0U][0U];
        const float a01 = matrix.m[0U][1U];
        const float a02 = matrix.m[0U][2U];
        const float a10 = matrix.m[1U][0U];
        const float a11 = matrix.m[1U][1U];
        const float a12 = matrix.m[1U][2U];
        const float a20 = matrix.m[2U][0U];
        const float a21 = matrix.m[2U][1U];
        const float a22 = matrix.m[2U][2U];
        const float determinant = a00 * (a11 * a22 - a12 * a21) - a01 * (a10 * a22 - a12 * a20) + a02 * (a10 * a21 - a11 * a20);
        if (std::fabs(determinant) <= 0.000001F || !std::isfinite(determinant)) {
            return identity_matrix44();
        }

        const float inv_det = 1.0F / determinant;
        auto result = identity_matrix44();
        result.m[0U][0U] = (a11 * a22 - a12 * a21) * inv_det;
        result.m[0U][1U] = (a02 * a21 - a01 * a22) * inv_det;
        result.m[0U][2U] = (a01 * a12 - a02 * a11) * inv_det;
        result.m[1U][0U] = (a12 * a20 - a10 * a22) * inv_det;
        result.m[1U][1U] = (a00 * a22 - a02 * a20) * inv_det;
        result.m[1U][2U] = (a02 * a10 - a00 * a12) * inv_det;
        result.m[2U][0U] = (a10 * a21 - a11 * a20) * inv_det;
        result.m[2U][1U] = (a01 * a20 - a00 * a21) * inv_det;
        result.m[2U][2U] = (a00 * a11 - a01 * a10) * inv_det;

        const float tx = matrix.m[0U][3U];
        const float ty = matrix.m[1U][3U];
        const float tz = matrix.m[2U][3U];
        result.m[0U][3U] = -(result.m[0U][0U] * tx + result.m[0U][1U] * ty + result.m[0U][2U] * tz);
        result.m[1U][3U] = -(result.m[1U][0U] * tx + result.m[1U][1U] * ty + result.m[1U][2U] * tz);
        result.m[2U][3U] = -(result.m[2U][0U] * tx + result.m[2U][1U] * ty + result.m[2U][2U] * tz);
        return result;
    }

    [[nodiscard]] Mat34 rotation_x_matrix(float radians) {
        const float cosine = std::cos(radians);
        const float sine = std::sin(radians);
        return Mat34{{
            std::array< float, 4U >{1.0F, 0.0F, 0.0F, 0.0F},
            std::array< float, 4U >{0.0F, cosine, -sine, 0.0F},
            std::array< float, 4U >{0.0F, sine, cosine, 0.0F},
        }};
    }

    [[nodiscard]] Mat34 rotation_y_matrix(float radians) {
        const float cosine = std::cos(radians);
        const float sine = std::sin(radians);
        return Mat34{{
            std::array< float, 4U >{cosine, 0.0F, sine, 0.0F},
            std::array< float, 4U >{0.0F, 1.0F, 0.0F, 0.0F},
            std::array< float, 4U >{-sine, 0.0F, cosine, 0.0F},
        }};
    }

    void apply_scale(Mat34* matrix, const assets::layout::J3dVec3& scale) {
        if (matrix == nullptr) {
            return;
        }
        for (std::size_t row = 0U; row < 3U; ++row) {
            matrix->m[row][0U] *= scale.x;
            matrix->m[row][1U] *= scale.y;
            matrix->m[row][2U] *= scale.z;
        }
    }

    [[nodiscard]] float j3d_rotation_to_radians(std::int16_t rotation) {
        constexpr float SHORT_TO_RAD = 3.14159265358979323846F / 32768.0F;
        return static_cast< float >(rotation) * SHORT_TO_RAD;
    }

    [[nodiscard]] Mat34 j3d_texture_matrix(float scale_x, float scale_y, float rotation, float translation_x, float translation_y,
                                           const assets::layout::J3dVec3& center, bool use_old_layout) {
        const float sine = std::sin(rotation);
        const float cosine = std::cos(rotation);
        const float cx = scale_x * cosine;
        const float sx = scale_x * sine;
        const float sy = scale_y * sine;
        const float cy = scale_y * cosine;
        if (use_old_layout) {
            return Mat34{{
                std::array< float, 4U >{cx, -sx, 0.0F, (-cx * center.x + sx * center.y) + center.x + translation_x},
                std::array< float, 4U >{sy, cy, 0.0F, (-sy * center.x - cy * center.y) + center.y + translation_y},
                std::array< float, 4U >{0.0F, 0.0F, 1.0F, 0.0F},
            }};
        }

        return Mat34{{
            std::array< float, 4U >{cx, -sx, (-cx * center.x + sx * center.y) + center.x + translation_x, 0.0F},
            std::array< float, 4U >{sy, cy, (-sy * center.x - cy * center.y) + center.y + translation_y, 0.0F},
            std::array< float, 4U >{0.0F, 0.0F, 1.0F, 0.0F},
        }};
    }

    [[nodiscard]] Mat34 j3d_texture_matrix(const assets::layout::J3dTextureSrt& srt, const assets::layout::J3dVec3& center, bool use_old_layout) {
        return j3d_texture_matrix(srt.scale_x, srt.scale_y, j3d_rotation_to_radians(srt.rotation), srt.translation_x, srt.translation_y, center,
                                  use_old_layout);
    }

    [[nodiscard]] Mat34 j3d_projection_q_matrix(bool use_old_layout) {
        if (use_old_layout) {
            return Mat34{{
                std::array< float, 4U >{0.5F, 0.0F, 0.0F, 0.5F},
                std::array< float, 4U >{0.0F, -0.5F, 0.0F, 0.5F},
                std::array< float, 4U >{0.0F, 0.0F, 1.0F, 0.0F},
            }};
        }

        return Mat34{{
            std::array< float, 4U >{0.5F, 0.0F, 0.5F, 0.0F},
            std::array< float, 4U >{0.0F, -0.5F, 0.5F, 0.0F},
            std::array< float, 4U >{0.0F, 0.0F, 1.0F, 0.0F},
        }};
    }

    [[nodiscard]] Mat34 pose_matrix(const assets::layout::J3dVec3& scale, const std::array< float, 3U >& rotation,
                                    const assets::layout::J3dVec3& translation) {
        const float sx = std::sin(rotation[0U]);
        const float cx = std::cos(rotation[0U]);
        const float sy = std::sin(rotation[1U]);
        const float cy = std::cos(rotation[1U]);
        const float sz = std::sin(rotation[2U]);
        const float cz = std::cos(rotation[2U]);
        const float cxsz = cx * sz;
        const float sxcz = sx * cz;
        const float sxsz = sx * sz;
        const float cxcz = cx * cz;

        Mat34 matrix{{
            std::array< float, 4U >{cz * cy, sxcz * sy - cxsz, cxcz * sy + sxsz, translation.x},
            std::array< float, 4U >{sz * cy, cxsz * sy + sxcz, sxsz * sy - cxcz, translation.y},
            std::array< float, 4U >{-sy, cy * sx, cy * cx, translation.z},
        }};
        apply_scale(&matrix, scale);
        return matrix;
    }

    [[nodiscard]] Mat34 joint_matrix(const assets::layout::J3dJoint& joint) {
        return pose_matrix(joint.scale,
                           std::array< float, 3U >{
                               j3d_rotation_to_radians(joint.rotation[0U]),
                               j3d_rotation_to_radians(joint.rotation[1U]),
                               j3d_rotation_to_radians(joint.rotation[2U]),
                           },
                           joint.translation);
    }

    [[nodiscard]] float jmath_cos_short_from_float(float value) {
        const auto angle = static_cast< std::int16_t >(value);
        const auto index = static_cast< std::uint16_t >(angle) >> 2U;
        const float radians = (static_cast< float >(index) * 6.28318530717958647692F) / 16384.0F;
        return std::cos(radians);
    }

    [[nodiscard]] Mat34 file_select_sky_base_matrix(float frame) {
        const float yaw = frame * 0.001F;
        const float step = std::fabs((3.14159265358979323846F * frame) / 3000.0F);
        const float pitch = (3.0F * (((1.0F - jmath_cos_short_from_float(8.0F * step)) * 0.5F) * 3.14159265358979323846F)) * 0.25F;

        return transpose_rotation(multiply_matrix(rotation_y_matrix(yaw), rotation_x_matrix(pitch)));
    }

    [[nodiscard]] Mat34 file_select_sky_actor_matrix(float frame) {
        constexpr float ACTOR_SCALE = 0.8F;
        auto base = file_select_sky_base_matrix(frame);
        apply_scale(&base, assets::layout::J3dVec3{.x = ACTOR_SCALE, .y = ACTOR_SCALE, .z = ACTOR_SCALE});
        return base;
    }

    [[nodiscard]] Vec3 to_vec3(const assets::layout::J3dVec3& value) {
        return Vec3{
            .x = value.x,
            .y = value.y,
            .z = value.z,
        };
    }

    [[nodiscard]] GeneratedTexCoord to_generated_texcoord(const assets::layout::J3dVec2& value) {
        return GeneratedTexCoord{
            .s = value.x,
            .t = value.y,
            .q = 1.0F,
        };
    }

    [[nodiscard]] Vec3 add(Vec3 lhs, Vec3 rhs) {
        return Vec3{
            .x = lhs.x + rhs.x,
            .y = lhs.y + rhs.y,
            .z = lhs.z + rhs.z,
        };
    }

    [[nodiscard]] Vec3 subtract(Vec3 lhs, Vec3 rhs) {
        return Vec3{
            .x = lhs.x - rhs.x,
            .y = lhs.y - rhs.y,
            .z = lhs.z - rhs.z,
        };
    }

    [[nodiscard]] float dot(Vec3 lhs, Vec3 rhs) {
        return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
    }

    [[nodiscard]] Vec3 cross(Vec3 lhs, Vec3 rhs) {
        return Vec3{
            .x = lhs.y * rhs.z - lhs.z * rhs.y,
            .y = lhs.z * rhs.x - lhs.x * rhs.z,
            .z = lhs.x * rhs.y - lhs.y * rhs.x,
        };
    }

    [[nodiscard]] Vec3 normalize(Vec3 value) {
        const float length = std::sqrt(dot(value, value));
        if (length <= 0.000001F || !std::isfinite(length)) {
            return Vec3{.x = 0.0F, .y = 0.0F, .z = 1.0F};
        }

        return Vec3{
            .x = value.x / length,
            .y = value.y / length,
            .z = value.z / length,
        };
    }

    [[nodiscard]] std::size_t item_array_index_for_selected_file(std::size_t selected_file_index) {
        constexpr std::array< std::size_t, 6U > INDEX_ORDER{1U, 2U, 4U, 6U, 5U, 3U};
        const auto file_number = std::min(selected_file_index, INDEX_ORDER.size() - 1U) + 1U;
        const auto found = std::find(INDEX_ORDER.begin(), INDEX_ORDER.end(), file_number);
        return found == INDEX_ORDER.end() ? 0U : static_cast< std::size_t >(std::distance(INDEX_ORDER.begin(), found));
    }

    [[nodiscard]] Vec3 file_select_base_position_for_selected_file(std::size_t selected_file_index) {
        constexpr std::array< float, 6U > ITEM_THETA_OFFSET_DEGREES{10.0F, -10.0F, 0.0F, 0.0F, 0.0F, 0.0F};
        constexpr float DEG_TO_RAD = 0.017453292519943295F;
        constexpr float RING_RADIUS = 5000.0F;
        constexpr float RING_PITCH_RADIANS = 0.3490658700466156F;

        const auto item_index = item_array_index_for_selected_file(selected_file_index);
        const float angle = -(static_cast< float >(item_index) + 4.0F) * (3.14159265358979323846F / 3.0F) -
                            ITEM_THETA_OFFSET_DEGREES[item_index] * DEG_TO_RAD;
        const Vec3 raw{
            .x = RING_RADIUS * std::cos(angle),
            .y = 0.0F,
            .z = RING_RADIUS * std::sin(angle),
        };

        const float cosine = std::cos(RING_PITCH_RADIANS);
        const float sine = std::sin(RING_PITCH_RADIANS);
        return Vec3{
            .x = raw.x,
            .y = raw.y * cosine - raw.z * sine,
            .z = raw.y * sine + raw.z * cosine,
        };
    }

    [[nodiscard]] Camera file_select_camera(std::size_t selected_file_index, FileSelectSkyCameraMode mode) {
        if (mode == FileSelectSkyCameraMode::Title) {
            return Camera{
                .eye = Vec3{.x = 0.0F, .y = 15800.0F, .z = 15000.0F},
                .target = Vec3{.x = 0.0F, .y = 15800.0F, .z = 0.0F},
                .fovy_degrees = 60.0F,
            };
        }

        if (mode == FileSelectSkyCameraMode::Far) {
            return Camera{
                .eye = Vec3{.x = 0.0F, .y = 0.0F, .z = 15000.0F},
                .target = Vec3{.x = 0.0F, .y = 800.0F, .z = 0.0F},
                .fovy_degrees = 40.0F,
            };
        }

        const auto base = file_select_base_position_for_selected_file(selected_file_index);
        const auto target = add(base, Vec3{.x = 0.0F, .y = 1100.0F, .z = 0.0F});
        return Camera{
            .eye = add(target, Vec3{.x = 0.0F, .y = 0.0F, .z = 4800.0F}),
            .target = target,
            .fovy_degrees = 50.0F,
        };
    }

    [[nodiscard]] bool is_finite_vertex(const ProjectedVertex& vertex) {
        return std::isfinite(vertex.x) && std::isfinite(vertex.y) && std::isfinite(vertex.z) && std::isfinite(vertex.u) &&
               std::isfinite(vertex.v) && std::isfinite(vertex.q) && std::isfinite(vertex.u_secondary) && std::isfinite(vertex.v_secondary) &&
               std::isfinite(vertex.q_secondary);
    }

    [[nodiscard]] bool is_finite_camera_vertex(const CameraVertex& vertex) {
        return std::isfinite(vertex.x) && std::isfinite(vertex.y) && std::isfinite(vertex.z) && std::isfinite(vertex.u) &&
               std::isfinite(vertex.v) && std::isfinite(vertex.q) && std::isfinite(vertex.u_secondary) && std::isfinite(vertex.v_secondary) &&
               std::isfinite(vertex.q_secondary);
    }

    [[nodiscard]] float clamp_float(float value, float low, float high) {
        return std::max(low, std::min(high, value));
    }

    [[nodiscard]] std::uint8_t clamp_u8(float value) {
        return static_cast< std::uint8_t >(std::lround(clamp_float(value, 0.0F, 255.0F)));
    }

    [[nodiscard]] std::uint8_t interpolate_color_channel(std::uint32_t lhs, std::uint32_t rhs, std::uint32_t shift, float t) {
        const float left = static_cast< float >((lhs >> shift) & 0xFFU);
        const float right = static_cast< float >((rhs >> shift) & 0xFFU);
        return clamp_u8(left + (right - left) * t);
    }

    [[nodiscard]] std::uint32_t interpolate_color(std::uint32_t lhs, std::uint32_t rhs, float t) {
        return render::layout::pack_abgr(interpolate_color_channel(lhs, rhs, 0U, t),
                                         interpolate_color_channel(lhs, rhs, 8U, t),
                                         interpolate_color_channel(lhs, rhs, 16U, t),
                                         interpolate_color_channel(lhs, rhs, 24U, t));
    }

    [[nodiscard]] CameraVertex interpolate_camera_vertex(const CameraVertex& lhs, const CameraVertex& rhs, float t) {
        return CameraVertex{
            .x = lhs.x + (rhs.x - lhs.x) * t,
            .y = lhs.y + (rhs.y - lhs.y) * t,
            .z = lhs.z + (rhs.z - lhs.z) * t,
            .u = lhs.u + (rhs.u - lhs.u) * t,
            .v = lhs.v + (rhs.v - lhs.v) * t,
            .q = lhs.q + (rhs.q - lhs.q) * t,
            .u_secondary = lhs.u_secondary + (rhs.u_secondary - lhs.u_secondary) * t,
            .v_secondary = lhs.v_secondary + (rhs.v_secondary - lhs.v_secondary) * t,
            .q_secondary = lhs.q_secondary + (rhs.q_secondary - lhs.q_secondary) * t,
            .color = interpolate_color(lhs.color, rhs.color, t),
        };
    }

    [[nodiscard]] GeneratedTexCoord perspective_correct_layout_texcoord(float s, float t, float q, float camera_z) {
        if (std::fabs(q) <= 0.000001F || camera_z <= 0.000001F || !std::isfinite(camera_z)) {
            return GeneratedTexCoord{
                .s = s,
                .t = t,
                .q = q,
            };
        }

        const float reciprocal_z = 1.0F / camera_z;
        return GeneratedTexCoord{
            .s = s * reciprocal_z,
            .t = t * reciprocal_z,
            .q = q * reciprocal_z,
        };
    }

    [[nodiscard]] float frustum_plane_distance(const CameraVertex& vertex, FrustumPlane plane, const FrustumClipParameters& parameters) {
        const float horizontal_limit = vertex.z * parameters.tan_half_fovy * parameters.aspect;
        const float vertical_limit = vertex.z * parameters.tan_half_fovy;
        switch (plane) {
        case FrustumPlane::Near:
            return vertex.z - parameters.near_z;
        case FrustumPlane::Left:
            return vertex.x + horizontal_limit;
        case FrustumPlane::Right:
            return horizontal_limit - vertex.x;
        case FrustumPlane::Bottom:
            return vertex.y + vertical_limit;
        case FrustumPlane::Top:
            return vertical_limit - vertex.y;
        }
        return -1.0F;
    }

    [[nodiscard]] ClippedPolygon clip_polygon_to_plane(const ClippedPolygon& polygon, FrustumPlane plane,
                                                       const FrustumClipParameters& parameters) {
        ClippedPolygon clipped{};
        const auto emit_vertex = [&](const CameraVertex& vertex) {
            if (clipped.count < clipped.vertices.size()) {
                clipped.vertices[clipped.count] = vertex;
                ++clipped.count;
            }
        };

        if (polygon.count == 0U) {
            return clipped;
        }

        for (std::size_t index = 0U; index < polygon.count; ++index) {
            const auto& previous = polygon.vertices[(index + polygon.count - 1U) % polygon.count];
            const auto& current = polygon.vertices[index];
            const float previous_distance = frustum_plane_distance(previous, plane, parameters);
            const float current_distance = frustum_plane_distance(current, plane, parameters);
            const bool previous_inside = previous_distance >= -0.0001F;
            const bool current_inside = current_distance >= -0.0001F;

            if (current_inside && !previous_inside) {
                const float denominator = previous_distance - current_distance;
                const float t = std::fabs(denominator) <= 0.000001F ? 0.0F : previous_distance / denominator;
                auto clipped_vertex = interpolate_camera_vertex(previous, current, t);
                emit_vertex(clipped_vertex);
            } else if (!current_inside && previous_inside) {
                const float denominator = previous_distance - current_distance;
                const float t = std::fabs(denominator) <= 0.000001F ? 0.0F : previous_distance / denominator;
                auto clipped_vertex = interpolate_camera_vertex(previous, current, t);
                emit_vertex(clipped_vertex);
            }

            if (current_inside) {
                emit_vertex(current);
            }
        }

        return clipped;
    }

    [[nodiscard]] ClippedPolygon clip_triangle_to_frustum(const std::array< CameraVertex, 3U >& triangle,
                                                          const FrustumClipParameters& parameters) {
        ClippedPolygon polygon{};
        polygon.vertices[0U] = triangle[0U];
        polygon.vertices[1U] = triangle[1U];
        polygon.vertices[2U] = triangle[2U];
        polygon.count = triangle.size();

        for (const auto plane : {FrustumPlane::Near, FrustumPlane::Left, FrustumPlane::Right, FrustumPlane::Bottom, FrustumPlane::Top}) {
            polygon = clip_polygon_to_plane(polygon, plane, parameters);
            if (polygon.count < 3U) {
                break;
            }
        }

        for (std::size_t index = 0U; index < polygon.count; ++index) {
            polygon.vertices[index].z = std::max(polygon.vertices[index].z, parameters.near_z);
        }

        return polygon;
    }

    [[nodiscard]] render::layout::TextureRef texture_ref(const assets::layout::J3dTexture& texture) {
        return render::layout::TextureRef{
            .id = static_cast< std::uint64_t >(reinterpret_cast< std::uintptr_t >(&texture.image)),
            .rgba8 = texture.image.rgba8.data(),
            .width = texture.image.width,
            .height = texture.image.height,
            .wrap_s = texture.wrap_s,
            .wrap_t = texture.wrap_t,
        };
    }

    [[nodiscard]] render::layout::TextureRef empty_texture_ref() {
        return render::layout::TextureRef{};
    }

    [[nodiscard]] std::vector< MaterialTextureSelection > material_texture_selections(
        const assets::layout::J3dMaterial& material,
        const std::vector< assets::layout::J3dTexture >& textures,
        std::size_t max_selections) {
        constexpr std::uint8_t GX_TEXMAP_NULL = 0xFFU;
        constexpr std::uint8_t GX_TEXCOORD_NULL = 0xFFU;
        std::vector< MaterialTextureSelection > selections{};
        selections.reserve(std::min< std::size_t >(max_selections, 4U));

        const auto append_selection = [&](MaterialTextureSelection selection) {
            if (selections.size() >= max_selections) {
                return;
            }
            const auto duplicate = std::find_if(selections.begin(), selections.end(), [&](const MaterialTextureSelection& existing) {
                return existing.texture_map_slot == selection.texture_map_slot &&
                       existing.texture_coordinate_slot == selection.texture_coordinate_slot;
            });
            if (duplicate == selections.end()) {
                selections.push_back(selection);
            }
        };

        const auto texture_slot_is_usable = [&](std::size_t slot) {
            return slot < material.texture_indices.size() && material.texture_indices[slot] != assets::layout::J3D_NO_TEXTURE_INDEX &&
                   material.texture_indices[slot] < textures.size() && !textures[material.texture_indices[slot]].image.empty();
        };

        const std::size_t stage_count = material.tev_stage_count == 0U ? material.tev_orders.size()
                                                                       : std::min< std::size_t >(material.tev_stage_count, material.tev_orders.size());
        for (std::size_t stage = 0U; stage < stage_count; ++stage) {
            const auto& order = material.tev_orders[stage];
            if (!order.valid || order.texture_map == GX_TEXMAP_NULL || order.texture_coordinate == GX_TEXCOORD_NULL) {
                continue;
            }

            const auto texture_map_slot = static_cast< std::size_t >(order.texture_map);
            if (texture_slot_is_usable(texture_map_slot)) {
                append_selection(MaterialTextureSelection{
                    .texture_map_slot = texture_map_slot,
                    .texture_coordinate_slot = static_cast< std::size_t >(order.texture_coordinate),
                });
                if (selections.size() >= max_selections) {
                    return selections;
                }
            }
        }

        const std::size_t indirect_stage_count =
            std::min< std::size_t >(material.indirect_texture_stage_count, material.indirect_texture_orders.size());
        for (std::size_t stage = 0U; stage < indirect_stage_count; ++stage) {
            const auto& order = material.indirect_texture_orders[stage];
            if (!order.valid || order.texture_map == GX_TEXMAP_NULL || order.texture_coordinate == GX_TEXCOORD_NULL) {
                continue;
            }

            const auto texture_map_slot = static_cast< std::size_t >(order.texture_map);
            if (texture_slot_is_usable(texture_map_slot)) {
                append_selection(MaterialTextureSelection{
                    .texture_map_slot = texture_map_slot,
                    .texture_coordinate_slot = static_cast< std::size_t >(order.texture_coordinate),
                });
                if (selections.size() >= max_selections) {
                    return selections;
                }
            }
        }

        for (std::size_t slot = 0U; slot < material.texture_indices.size(); ++slot) {
            if (texture_slot_is_usable(slot)) {
                append_selection(MaterialTextureSelection{
                    .texture_map_slot = slot,
                    .texture_coordinate_slot = slot,
                });
                if (selections.size() >= max_selections) {
                    return selections;
                }
            }
        }

        return selections;
    }

    [[nodiscard]] std::optional< render::layout::TriangleTextureCombineMode > secondary_texture_combine_mode_from_environment() {
        const char* value = std::getenv("SMGPC_FILE_SELECT_LIVE_J3D_SKY_SECONDARY_MODE");
        if (value == nullptr || value[0] == '\0') {
            return std::nullopt;
        }

        const auto mode = std::string_view(value);
        if (mode == "none" || mode == "0") {
            return render::layout::TriangleTextureCombineMode::None;
        }
        if (mode == "multiply" || mode == "mul") {
            return render::layout::TriangleTextureCombineMode::Multiply;
        }
        if (mode == "add" || mode == "additive") {
            return render::layout::TriangleTextureCombineMode::Add;
        }
        if (mode == "j3d" || mode == "tev" || mode == "j3d_tev") {
            return render::layout::TriangleTextureCombineMode::J3dTevColorStages;
        }
        return render::layout::TriangleTextureCombineMode::Screen;
    }

    [[nodiscard]] std::size_t triangle_limit_from_environment() {
        const char* value = std::getenv("SMGPC_FILE_SELECT_LIVE_J3D_SKY_TRIANGLE_LIMIT");
        if (value == nullptr || value[0] == '\0') {
            return std::numeric_limits< std::size_t >::max();
        }

        char* end = nullptr;
        const unsigned long parsed = std::strtoul(value, &end, 10);
        if (end == value || *end != '\0' || parsed == 0UL) {
            return std::numeric_limits< std::size_t >::max();
        }

        return static_cast< std::size_t >(parsed);
    }

    [[nodiscard]] float frame_offset_from_environment() {
        const char* value = std::getenv("SMGPC_FILE_SELECT_LIVE_J3D_SKY_FRAME_OFFSET");
        if (value == nullptr || value[0] == '\0') {
            return 0.0F;
        }

        char* end = nullptr;
        const float parsed = std::strtof(value, &end);
        if (end == value || *end != '\0' || !std::isfinite(parsed)) {
            return 0.0F;
        }

        return parsed;
    }

    [[nodiscard]] std::optional< float > float_from_environment(const char* name) {
        const char* value = std::getenv(name);
        if (value == nullptr || value[0] == '\0') {
            return std::nullopt;
        }

        char* end = nullptr;
        const float parsed = std::strtof(value, &end);
        if (end == value || *end != '\0' || !std::isfinite(parsed)) {
            return std::nullopt;
        }

        return parsed;
    }

    void apply_camera_overrides_from_environment(Camera* camera) {
        if (camera == nullptr) {
            return;
        }

        if (const auto value = float_from_environment("SMGPC_FILE_SELECT_LIVE_J3D_SKY_EYE_X")) {
            camera->eye.x = *value;
        }
        if (const auto value = float_from_environment("SMGPC_FILE_SELECT_LIVE_J3D_SKY_EYE_Y")) {
            camera->eye.y = *value;
        }
        if (const auto value = float_from_environment("SMGPC_FILE_SELECT_LIVE_J3D_SKY_EYE_Z")) {
            camera->eye.z = *value;
        }
        if (const auto value = float_from_environment("SMGPC_FILE_SELECT_LIVE_J3D_SKY_TARGET_X")) {
            camera->target.x = *value;
        }
        if (const auto value = float_from_environment("SMGPC_FILE_SELECT_LIVE_J3D_SKY_TARGET_Y")) {
            camera->target.y = *value;
        }
        if (const auto value = float_from_environment("SMGPC_FILE_SELECT_LIVE_J3D_SKY_TARGET_Z")) {
            camera->target.z = *value;
        }
        if (const auto value = float_from_environment("SMGPC_FILE_SELECT_LIVE_J3D_SKY_FOVY")) {
            camera->fovy_degrees = *value;
        }
    }

    [[nodiscard]] float environment_float_or(const char* name, float fallback) {
        if (const auto value = float_from_environment(name)) {
            return *value;
        }

        return fallback;
    }

    [[nodiscard]] float sky_material_alpha_scale_from_environment(const assets::layout::J3dMaterial& material, bool additive, float opaque_alpha_scale,
                                                                  float additive_alpha_scale) {
        if (additive) {
            return additive_alpha_scale;
        }
        if (material.name == "Space_Mat_v") {
            return environment_float_or("SMGPC_FILE_SELECT_LIVE_J3D_SKY_SPACE_ALPHA_SCALE", opaque_alpha_scale);
        }
        if (material.name == "EarthNightMat_v") {
            return environment_float_or("SMGPC_FILE_SELECT_LIVE_J3D_SKY_EARTH_ALPHA_SCALE", opaque_alpha_scale);
        }
        return opaque_alpha_scale;
    }

    [[nodiscard]] bool environment_flag(const char* name) {
        const char* value = std::getenv(name);
        return value != nullptr && value[0] != '\0' && value[0] != '0';
    }

    [[nodiscard]] float animation_frame(float frame, float frame_max) {
        if (frame_max <= 0.0F) {
            return frame;
        }

        float wrapped = std::fmod(frame, frame_max);
        if (wrapped < 0.0F) {
            wrapped += frame_max;
        }
        return wrapped;
    }

    [[nodiscard]] BckTrack read_bck_float_track(std::span< const std::byte > bytes, std::size_t values_offset, std::size_t values_count,
                                                std::uint16_t count, std::uint16_t index, std::uint16_t tangent_type) {
        BckTrack track{};
        if (count == 0U || index >= values_count) {
            return track;
        }

        track.valid = true;
        if (count == 1U) {
            const float value = assets::layout::binary::read_f32_be(bytes, values_offset + static_cast< std::size_t >(index) * 4U);
            track.keys.push_back(BckKey{
                .frame = 0.0F,
                .value = value,
                .tangent_in = 0.0F,
                .tangent_out = 0.0F,
            });
            return track;
        }

        const std::size_t stride = tangent_type == 0U ? 3U : 4U;
        track.keys.reserve(count);
        for (std::size_t key_index = 0U; key_index < count; ++key_index) {
            const auto value_index = static_cast< std::size_t >(index) + key_index * stride;
            if (value_index + stride - 1U >= values_count) {
                track.valid = false;
                track.keys.clear();
                return track;
            }

            const float tangent_in = assets::layout::binary::read_f32_be(bytes, values_offset + (value_index + 2U) * 4U);
            track.keys.push_back(BckKey{
                .frame = assets::layout::binary::read_f32_be(bytes, values_offset + value_index * 4U),
                .value = assets::layout::binary::read_f32_be(bytes, values_offset + (value_index + 1U) * 4U),
                .tangent_in = tangent_in,
                .tangent_out = tangent_type == 0U ? tangent_in
                                                  : assets::layout::binary::read_f32_be(bytes, values_offset + (value_index + 3U) * 4U),
            });
        }
        return track;
    }

    [[nodiscard]] BckTrack read_bck_rotation_track(std::span< const std::byte > bytes, std::size_t values_offset, std::size_t values_count,
                                                   std::uint16_t count, std::uint16_t index, std::uint16_t tangent_type, float rotation_scale) {
        BckTrack track{};
        if (count == 0U || index >= values_count) {
            return track;
        }

        const auto read_rotation_value = [&](std::size_t value_index) {
            const auto raw = static_cast< std::int16_t >(assets::layout::binary::read_u16_be(bytes, values_offset + value_index * 2U));
            return static_cast< float >(raw) * rotation_scale;
        };

        track.valid = true;
        if (count == 1U) {
            const float value = read_rotation_value(index);
            track.keys.push_back(BckKey{
                .frame = 0.0F,
                .value = value,
                .tangent_in = 0.0F,
                .tangent_out = 0.0F,
            });
            return track;
        }

        const std::size_t stride = tangent_type == 0U ? 3U : 4U;
        track.keys.reserve(count);
        for (std::size_t key_index = 0U; key_index < count; ++key_index) {
            const auto value_index = static_cast< std::size_t >(index) + key_index * stride;
            if (value_index + stride - 1U >= values_count) {
                track.valid = false;
                track.keys.clear();
                return track;
            }

            const float tangent_in = read_rotation_value(value_index + 2U);
            track.keys.push_back(BckKey{
                .frame = static_cast< float >(static_cast< std::int16_t >(
                    assets::layout::binary::read_u16_be(bytes, values_offset + value_index * 2U))),
                .value = read_rotation_value(value_index + 1U),
                .tangent_in = tangent_in,
                .tangent_out = tangent_type == 0U ? tangent_in : read_rotation_value(value_index + 3U),
            });
        }
        return track;
    }

    [[nodiscard]] std::string read_name_table_name(std::span< const std::byte > bytes, std::size_t table_offset, std::size_t index) {
        if (!assets::layout::binary::has_bytes(bytes, table_offset, 4U)) {
            return {};
        }

        const auto count = static_cast< std::size_t >(assets::layout::binary::read_u16_be(bytes, table_offset));
        if (index >= count) {
            return {};
        }

        const auto entry_offset = table_offset + 4U + index * 4U;
        if (!assets::layout::binary::has_bytes(bytes, entry_offset, 4U)) {
            return {};
        }

        const auto name_offset = table_offset + static_cast< std::size_t >(assets::layout::binary::read_u16_be(bytes, entry_offset + 2U));
        if (!assets::layout::binary::has_bytes(bytes, name_offset, 1U)) {
            return {};
        }

        return assets::layout::binary::read_c_string(bytes, name_offset);
    }

    [[nodiscard]] std::optional< ParsedBck > parse_bck_animation(std::span< const std::byte > bytes) {
        if (bytes.empty()) {
            return std::nullopt;
        }
        if (!assets::layout::binary::has_bytes(bytes, 0U, 0x40U) || !assets::layout::binary::fourcc_equals(bytes, 0U, "J3D1") ||
            !assets::layout::binary::fourcc_equals(bytes, 4U, "bck1") || !assets::layout::binary::fourcc_equals(bytes, 0x20U, "ANK1")) {
            return std::nullopt;
        }

        constexpr std::size_t ANK_OFFSET = 0x20U;
        const auto section_size = static_cast< std::size_t >(assets::layout::binary::read_u32_be(bytes, ANK_OFFSET + 4U));
        if (section_size < 0x28U || !assets::layout::binary::has_bytes(bytes, ANK_OFFSET, section_size)) {
            return std::nullopt;
        }

        const auto rotation_shift = assets::layout::binary::read_u8(bytes, ANK_OFFSET + 0x09U);
        const auto frame_max = static_cast< float >(assets::layout::binary::read_u16_be(bytes, ANK_OFFSET + 0x0AU));
        const auto joint_count = static_cast< std::size_t >(assets::layout::binary::read_u16_be(bytes, ANK_OFFSET + 0x0CU));
        const auto scale_count = static_cast< std::size_t >(assets::layout::binary::read_u16_be(bytes, ANK_OFFSET + 0x0EU));
        const auto rotation_count = static_cast< std::size_t >(assets::layout::binary::read_u16_be(bytes, ANK_OFFSET + 0x10U));
        const auto translation_count = static_cast< std::size_t >(assets::layout::binary::read_u16_be(bytes, ANK_OFFSET + 0x12U));
        const auto table_offset = ANK_OFFSET + static_cast< std::size_t >(assets::layout::binary::read_u32_be(bytes, ANK_OFFSET + 0x14U));
        const auto scale_offset = ANK_OFFSET + static_cast< std::size_t >(assets::layout::binary::read_u32_be(bytes, ANK_OFFSET + 0x18U));
        const auto rotation_offset = ANK_OFFSET + static_cast< std::size_t >(assets::layout::binary::read_u32_be(bytes, ANK_OFFSET + 0x1CU));
        const auto translation_offset = ANK_OFFSET + static_cast< std::size_t >(assets::layout::binary::read_u32_be(bytes, ANK_OFFSET + 0x20U));

        if (!assets::layout::binary::has_bytes(bytes, table_offset, joint_count * 0x36U) ||
            !assets::layout::binary::has_bytes(bytes, scale_offset, scale_count * 4U) ||
            !assets::layout::binary::has_bytes(bytes, rotation_offset, rotation_count * 2U) ||
            !assets::layout::binary::has_bytes(bytes, translation_offset, translation_count * 4U)) {
            return std::nullopt;
        }

        ParsedBck animation{
            .joints = std::vector< BckJoint >(joint_count),
            .frame_max = frame_max,
        };
        const float rotation_scale = (static_cast< float >(1U << rotation_shift) * 3.14159265358979323846F) / 32768.0F;
        for (std::size_t joint_index = 0U; joint_index < joint_count; ++joint_index) {
            const auto joint_offset = table_offset + joint_index * 0x36U;
            for (std::size_t axis = 0U; axis < 3U; ++axis) {
                const auto axis_offset = joint_offset + axis * 0x12U;
                const auto scale_track_offset = axis_offset;
                animation.joints[joint_index].scale[axis] = read_bck_float_track(
                    bytes,
                    scale_offset,
                    scale_count,
                    assets::layout::binary::read_u16_be(bytes, scale_track_offset + 0U),
                    assets::layout::binary::read_u16_be(bytes, scale_track_offset + 2U),
                    assets::layout::binary::read_u16_be(bytes, scale_track_offset + 4U));

                const auto rotation_track_offset = axis_offset + 0x06U;
                animation.joints[joint_index].rotation[axis] = read_bck_rotation_track(
                    bytes,
                    rotation_offset,
                    rotation_count,
                    assets::layout::binary::read_u16_be(bytes, rotation_track_offset + 0U),
                    assets::layout::binary::read_u16_be(bytes, rotation_track_offset + 2U),
                    assets::layout::binary::read_u16_be(bytes, rotation_track_offset + 4U),
                    rotation_scale);

                const auto translation_track_offset = axis_offset + 0x0CU;
                animation.joints[joint_index].translation[axis] = read_bck_float_track(
                    bytes,
                    translation_offset,
                    translation_count,
                    assets::layout::binary::read_u16_be(bytes, translation_track_offset + 0U),
                    assets::layout::binary::read_u16_be(bytes, translation_track_offset + 2U),
                    assets::layout::binary::read_u16_be(bytes, translation_track_offset + 4U));
            }
        }

        return animation;
    }

    [[nodiscard]] std::optional< ParsedBtk > parse_btk_animation(std::span< const std::byte > bytes) {
        if (bytes.empty()) {
            return std::nullopt;
        }
        if (!assets::layout::binary::has_bytes(bytes, 0U, 0x80U) || !assets::layout::binary::fourcc_equals(bytes, 0U, "J3D1") ||
            !assets::layout::binary::fourcc_equals(bytes, 4U, "btk1") || !assets::layout::binary::fourcc_equals(bytes, 0x20U, "TTK1")) {
            return std::nullopt;
        }

        constexpr std::size_t TTK_OFFSET = 0x20U;
        const auto section_size = static_cast< std::size_t >(assets::layout::binary::read_u32_be(bytes, TTK_OFFSET + 4U));
        if (section_size < 0x60U || !assets::layout::binary::has_bytes(bytes, TTK_OFFSET, section_size)) {
            return std::nullopt;
        }

        const auto rotation_shift = assets::layout::binary::read_u8(bytes, TTK_OFFSET + 0x09U);
        const auto frame_max = static_cast< float >(assets::layout::binary::read_u16_be(bytes, TTK_OFFSET + 0x0AU));
        const auto track_count = static_cast< std::size_t >(assets::layout::binary::read_u16_be(bytes, TTK_OFFSET + 0x0CU));
        const auto scale_count = static_cast< std::size_t >(assets::layout::binary::read_u16_be(bytes, TTK_OFFSET + 0x0EU));
        const auto rotation_count = static_cast< std::size_t >(assets::layout::binary::read_u16_be(bytes, TTK_OFFSET + 0x10U));
        const auto translation_count = static_cast< std::size_t >(assets::layout::binary::read_u16_be(bytes, TTK_OFFSET + 0x12U));
        const auto table_offset = TTK_OFFSET + static_cast< std::size_t >(assets::layout::binary::read_u32_be(bytes, TTK_OFFSET + 0x14U));
        const auto material_id_offset = TTK_OFFSET + static_cast< std::size_t >(assets::layout::binary::read_u32_be(bytes, TTK_OFFSET + 0x18U));
        const auto name_table_offset = TTK_OFFSET + static_cast< std::size_t >(assets::layout::binary::read_u32_be(bytes, TTK_OFFSET + 0x1CU));
        const auto tex_mtx_id_offset = TTK_OFFSET + static_cast< std::size_t >(assets::layout::binary::read_u32_be(bytes, TTK_OFFSET + 0x20U));
        const auto center_offset = TTK_OFFSET + static_cast< std::size_t >(assets::layout::binary::read_u32_be(bytes, TTK_OFFSET + 0x24U));
        const auto scale_offset = TTK_OFFSET + static_cast< std::size_t >(assets::layout::binary::read_u32_be(bytes, TTK_OFFSET + 0x28U));
        const auto rotation_offset = TTK_OFFSET + static_cast< std::size_t >(assets::layout::binary::read_u32_be(bytes, TTK_OFFSET + 0x2CU));
        const auto translation_offset = TTK_OFFSET + static_cast< std::size_t >(assets::layout::binary::read_u32_be(bytes, TTK_OFFSET + 0x30U));

        if (track_count == 0U || track_count % 3U != 0U) {
            return std::nullopt;
        }

        const auto material_count = track_count / 3U;
        if (!assets::layout::binary::has_bytes(bytes, table_offset, track_count * 0x12U) ||
            !assets::layout::binary::has_bytes(bytes, material_id_offset, material_count * 2U) ||
            !assets::layout::binary::has_bytes(bytes, tex_mtx_id_offset, material_count) ||
            !assets::layout::binary::has_bytes(bytes, center_offset, material_count * 12U) ||
            !assets::layout::binary::has_bytes(bytes, scale_offset, scale_count * 4U) ||
            !assets::layout::binary::has_bytes(bytes, rotation_offset, rotation_count * 2U) ||
            !assets::layout::binary::has_bytes(bytes, translation_offset, translation_count * 4U)) {
            return std::nullopt;
        }

        ParsedBtk animation{
            .materials = std::vector< BtkMaterialAnimation >(material_count),
            .frame_max = frame_max,
        };
        const float rotation_scale = (static_cast< float >(1U << rotation_shift) * 3.14159265358979323846F) / 32768.0F;

        const auto read_scale_track = [&](std::size_t entry_offset) {
            return read_bck_float_track(bytes,
                                        scale_offset,
                                        scale_count,
                                        assets::layout::binary::read_u16_be(bytes, entry_offset + 0U),
                                        assets::layout::binary::read_u16_be(bytes, entry_offset + 2U),
                                        assets::layout::binary::read_u16_be(bytes, entry_offset + 4U));
        };
        const auto read_rotation_track = [&](std::size_t entry_offset) {
            return read_bck_rotation_track(bytes,
                                           rotation_offset,
                                           rotation_count,
                                           assets::layout::binary::read_u16_be(bytes, entry_offset + 6U),
                                           assets::layout::binary::read_u16_be(bytes, entry_offset + 8U),
                                           assets::layout::binary::read_u16_be(bytes, entry_offset + 10U),
                                           rotation_scale);
        };
        const auto read_translation_track = [&](std::size_t entry_offset) {
            return read_bck_float_track(bytes,
                                        translation_offset,
                                        translation_count,
                                        assets::layout::binary::read_u16_be(bytes, entry_offset + 12U),
                                        assets::layout::binary::read_u16_be(bytes, entry_offset + 14U),
                                        assets::layout::binary::read_u16_be(bytes, entry_offset + 16U));
        };

        for (std::size_t material_index = 0U; material_index < material_count; ++material_index) {
            const auto entry_x = table_offset + (material_index * 3U + 0U) * 0x12U;
            const auto entry_y = table_offset + (material_index * 3U + 1U) * 0x12U;
            const auto entry_rotation = table_offset + (material_index * 3U + 2U) * 0x12U;
            auto& material = animation.materials[material_index];
            material.material_name = read_name_table_name(bytes, name_table_offset, material_index);
            material.material_id = assets::layout::binary::read_u16_be(bytes, material_id_offset + material_index * 2U);
            material.texture_matrix_index = assets::layout::binary::read_u8(bytes, tex_mtx_id_offset + material_index);
            const auto center = center_offset + material_index * 12U;
            material.center = assets::layout::J3dVec3{
                .x = assets::layout::binary::read_f32_be(bytes, center + 0U),
                .y = assets::layout::binary::read_f32_be(bytes, center + 4U),
                .z = assets::layout::binary::read_f32_be(bytes, center + 8U),
            };
            material.scale_x = read_scale_track(entry_x);
            material.scale_y = read_scale_track(entry_y);
            material.rotation = read_rotation_track(entry_rotation);
            material.translation_x = read_translation_track(entry_x);
            material.translation_y = read_translation_track(entry_y);
        }

        return animation;
    }

    [[nodiscard]] std::uint32_t multiply_color(const assets::layout::J3dColor& vertex, const assets::layout::J3dColor& material, float alpha_scale) {
        return render::layout::pack_abgr(
            clamp_u8(static_cast< float >(vertex.r) * static_cast< float >(material.r) / 255.0F),
            clamp_u8(static_cast< float >(vertex.g) * static_cast< float >(material.g) / 255.0F),
            clamp_u8(static_cast< float >(vertex.b) * static_cast< float >(material.b) / 255.0F),
            clamp_u8(static_cast< float >(vertex.a) * static_cast< float >(material.a) * alpha_scale / 255.0F));
    }

    [[nodiscard]] std::uint8_t channel_source_component(const assets::layout::J3dColorChannelInfo& channel, std::uint8_t vertex_component,
                                                        std::uint8_t material_component) {
        constexpr std::uint8_t GX_SRC_REG = 0U;
        constexpr std::uint8_t GX_SRC_VTX = 1U;

        if (!channel.valid || channel.material_source == GX_SRC_REG) {
            return material_component;
        }
        if (channel.material_source == GX_SRC_VTX) {
            return vertex_component;
        }
        return material_component;
    }

    [[nodiscard]] std::uint32_t raster_color(const assets::layout::J3dColor& vertex, const assets::layout::J3dMaterial& material, float alpha_scale,
                                             bool ignore_material_color) {
        // J3D color channel slots are loaded as COLOR0, COLOR1, ALPHA0, ALPHA1; TEV order GX_COLOR0A0 uses slots 0 and 2.
        const auto& color_channel = material.color_channels[0U];
        const auto& alpha_channel = material.color_channels[2U].valid ? material.color_channels[2U] : color_channel;
        if (!color_channel.valid && !alpha_channel.valid) {
            return multiply_color(vertex, ignore_material_color ? assets::layout::J3dColor{} : material.material_color, alpha_scale);
        }

        const assets::layout::J3dColor material_color = ignore_material_color ? assets::layout::J3dColor{} : material.material_color;
        return render::layout::pack_abgr(channel_source_component(color_channel, vertex.r, material_color.r),
                                         channel_source_component(color_channel, vertex.g, material_color.g),
                                         channel_source_component(color_channel, vertex.b, material_color.b),
                                         clamp_u8(static_cast< float >(
                                                      channel_source_component(alpha_channel, vertex.a, material_color.a)) *
                                                  alpha_scale));
    }

    [[nodiscard]] std::uint32_t pack_tev_k_color(const assets::layout::J3dTevKColor& color) {
        if (!color.valid) {
            return render::layout::pack_abgr(0U, 0U, 0U, 255U);
        }
        return render::layout::pack_abgr(color.r, color.g, color.b, color.a);
    }

    [[nodiscard]] std::uint32_t pack_tev_color_s10(const assets::layout::J3dTevColorS10& color) {
        if (!color.valid) {
            return render::layout::pack_abgr(0U, 0U, 0U, 255U);
        }
        return render::layout::pack_abgr(clamp_u8(static_cast< float >(color.r)),
                                         clamp_u8(static_cast< float >(color.g)),
                                         clamp_u8(static_cast< float >(color.b)),
                                         clamp_u8(static_cast< float >(color.a)));
    }

    [[nodiscard]] render::core::RenderJ3dTevSwapChannels core_swap_channels(const assets::layout::J3dMaterial& material, std::uint8_t table_index) {
        render::core::RenderJ3dTevSwapChannels channels{};
        if (static_cast< std::size_t >(table_index) >= material.tev_swap_mode_tables.size()) {
            return channels;
        }

        const auto& table = material.tev_swap_mode_tables[table_index];
        if (!table.valid) {
            return channels;
        }

        for (std::size_t index = 0U; index < channels.channels.size(); ++index) {
            channels.channels[index] = table.channels[index] <= 3U ? table.channels[index] : static_cast< std::uint8_t >(index);
        }
        return channels;
    }

    [[nodiscard]] render::core::RenderJ3dAlphaCompare core_alpha_compare(const assets::layout::J3dAlphaCompare& alpha_compare) {
        if (!alpha_compare.valid) {
            return {};
        }

        return render::core::RenderJ3dAlphaCompare{
            .valid = true,
            .comp0 = alpha_compare.comp0,
            .ref0 = alpha_compare.ref0,
            .op = alpha_compare.op,
            .comp1 = alpha_compare.comp1,
            .ref1 = alpha_compare.ref1,
        };
    }

    [[nodiscard]] render::core::RenderJ3dIndirectTextureMatrix core_indirect_texture_matrix(
        const assets::layout::J3dIndirectTextureMatrix& matrix) {
        if (!matrix.valid) {
            return {};
        }

        return render::core::RenderJ3dIndirectTextureMatrix{
            .valid = true,
            .values = matrix.values,
            .scale_exponent = matrix.scale_exponent,
        };
    }

    [[nodiscard]] render::core::RenderJ3dIndirectTevStage core_indirect_tev_stage(const assets::layout::J3dIndirectTevStage& stage) {
        if (!stage.valid) {
            return {};
        }

        return render::core::RenderJ3dIndirectTevStage{
            .valid = true,
            .ind_stage = stage.ind_stage,
            .format = stage.format,
            .bias = stage.bias,
            .matrix = stage.matrix,
            .wrap_s = stage.wrap_s,
            .wrap_t = stage.wrap_t,
            .add_prev = stage.add_prev,
            .alpha = stage.alpha,
        };
    }

    [[nodiscard]] render::layout::TriangleTevStage triangle_tev_stage_from_j3d(const assets::layout::J3dTevStageInfoRaw& stage) {
        return render::layout::TriangleTevStage{
            .color_args =
                {
                    .a = stage.color_args.a,
                    .b = stage.color_args.b,
                    .c = stage.color_args.c,
                    .d = stage.color_args.d,
                },
            .color_op =
                {
                    .op = stage.color_op.op,
                    .bias = stage.color_op.bias,
                    .scale = stage.color_op.scale,
                    .clamp = stage.color_op.clamp,
                    .output_register = stage.color_op.output_register,
                },
        };
    }

    [[nodiscard]] std::uint8_t populate_triangle_tev_stages(
        std::array< render::layout::TriangleTevStage, 2U >* pStages,
        const assets::layout::J3dMaterial& material) {
        if (pStages == nullptr) {
            return 0U;
        }

        std::uint8_t count = 0U;
        const auto stage_count = std::min< std::size_t >(material.tev_stage_count, pStages->size());
        for (std::size_t stage_index = 0U; stage_index < stage_count; ++stage_index) {
            if (!material.tev_stages[stage_index].valid) {
                continue;
            }
            (*pStages)[count] = triangle_tev_stage_from_j3d(material.tev_stages[stage_index]);
            ++count;
        }
        return count;
    }

    [[nodiscard]] render::layout::TriangleTextureCombineMode default_secondary_texture_mode_for_j3d_tev(std::uint8_t tev_stage_count) {
        return tev_stage_count > 1U ? render::layout::TriangleTextureCombineMode::J3dTevColorStages :
                                      render::layout::TriangleTextureCombineMode::None;
    }

    [[nodiscard]] render::core::RenderBlendMode core_blend_mode(const assets::layout::J3dBlendMode& blend, bool additive) {
        if (blend.valid && blend.type == 0U) {
            return render::core::RenderBlendMode::Opaque;
        }
        return additive ? render::core::RenderBlendMode::Additive : render::core::RenderBlendMode::Alpha;
    }

    [[nodiscard]] render::core::RenderCullMode core_cull_mode(std::uint8_t cull_mode) {
        if (cull_mode == 1U) {
            return render::core::RenderCullMode::Front;
        }
        if (cull_mode == 2U) {
            return render::core::RenderCullMode::Back;
        }
        return render::core::RenderCullMode::None;
    }

    [[nodiscard]] render::core::RenderDepthMode core_depth_mode(const assets::layout::J3dZMode& z_mode) {
        if (!z_mode.valid || z_mode.compare_enable == 0U) {
            return render::core::RenderDepthMode::Always;
        }
        if (z_mode.function == 1U) {
            return render::core::RenderDepthMode::Less;
        }
        if (z_mode.function == 3U) {
            return render::core::RenderDepthMode::LessEqual;
        }
        if (z_mode.function == 7U) {
            return render::core::RenderDepthMode::Always;
        }
        return render::core::RenderDepthMode::LessEqual;
    }

    [[nodiscard]] render::core::RenderTriangleTextureCombineMode core_secondary_texture_mode(render::layout::TriangleTextureCombineMode mode) {
        switch (mode) {
        case render::layout::TriangleTextureCombineMode::Multiply:
            return render::core::RenderTriangleTextureCombineMode::Multiply;
        case render::layout::TriangleTextureCombineMode::Add:
            return render::core::RenderTriangleTextureCombineMode::Add;
        case render::layout::TriangleTextureCombineMode::Screen:
            return render::core::RenderTriangleTextureCombineMode::Screen;
        case render::layout::TriangleTextureCombineMode::J3dTevColorStages:
            return render::core::RenderTriangleTextureCombineMode::J3dTevColorStages;
        case render::layout::TriangleTextureCombineMode::None:
        default:
            return render::core::RenderTriangleTextureCombineMode::None;
        }
    }

    [[nodiscard]] render::core::RenderTextureRef core_texture_ref(const render::layout::TextureRef& texture) {
        return render::core::RenderTextureRef{
            .id = texture.id,
            .rgba8 = texture.rgba8,
            .width = texture.width,
            .height = texture.height,
            .wrap_s = texture.wrap_s,
            .wrap_t = texture.wrap_t,
        };
    }

    [[nodiscard]] render::core::RenderJ3dTevStage core_tev_stage_from_j3d(const assets::layout::J3dTevStageInfoRaw& stage) {
        return render::core::RenderJ3dTevStage{
            .color_args =
                {
                    .a = stage.color_args.a,
                    .b = stage.color_args.b,
                    .c = stage.color_args.c,
                    .d = stage.color_args.d,
                },
            .color_op =
                {
                    .op = stage.color_op.op,
                    .bias = stage.color_op.bias,
                    .scale = stage.color_op.scale,
                    .clamp = stage.color_op.clamp,
                    .output_register = stage.color_op.output_register,
                },
            .alpha_args =
                {
                    .a = stage.alpha_args.a,
                    .b = stage.alpha_args.b,
                    .c = stage.alpha_args.c,
                    .d = stage.alpha_args.d,
                },
            .alpha_op =
                {
                    .op = stage.alpha_op.op,
                    .bias = stage.alpha_op.bias,
                    .scale = stage.alpha_op.scale,
                    .clamp = stage.alpha_op.clamp,
                    .output_register = stage.alpha_op.output_register,
                },
        };
    }

    [[nodiscard]] std::uint8_t populate_core_j3d_tev_stages(std::array< render::core::RenderJ3dTevStage, 4U >* pStages,
                                                            const assets::layout::J3dMaterial& material) {
        if (pStages == nullptr) {
            return 0U;
        }

        std::uint8_t count = 0U;
        const auto stage_count = std::min< std::size_t >(material.tev_stage_count, pStages->size());
        for (std::size_t stage_index = 0U; stage_index < stage_count; ++stage_index) {
            if (!material.tev_stages[stage_index].valid) {
                continue;
            }
            (*pStages)[count] = core_tev_stage_from_j3d(material.tev_stages[stage_index]);
            ++count;
        }
        return count;
    }

    [[nodiscard]] bool tev_stage_args_equal(const render::layout::TriangleTevStageArgs& lhs,
                                            const render::layout::TriangleTevStageArgs& rhs) {
        return lhs.a == rhs.a && lhs.b == rhs.b && lhs.c == rhs.c && lhs.d == rhs.d;
    }

    [[nodiscard]] bool tev_stage_op_equal(const render::layout::TriangleTevStageOp& lhs,
                                          const render::layout::TriangleTevStageOp& rhs) {
        return lhs.op == rhs.op && lhs.bias == rhs.bias && lhs.scale == rhs.scale && lhs.clamp == rhs.clamp &&
               lhs.output_register == rhs.output_register;
    }

    [[nodiscard]] bool tev_stage_equal(const render::layout::TriangleTevStage& lhs,
                                       const render::layout::TriangleTevStage& rhs) {
        return tev_stage_args_equal(lhs.color_args, rhs.color_args) && tev_stage_op_equal(lhs.color_op, rhs.color_op);
    }

    [[nodiscard]] bool tev_stage_array_equal(const std::array< render::layout::TriangleTevStage, 2U >& lhs,
                                             const std::array< render::layout::TriangleTevStage, 2U >& rhs) {
        return tev_stage_equal(lhs[0U], rhs[0U]) && tev_stage_equal(lhs[1U], rhs[1U]);
    }

    [[nodiscard]] bool is_triangle_culled(const ProjectedVertex& v0, const ProjectedVertex& v1, const ProjectedVertex& v2, std::uint8_t cull_mode) {
        constexpr std::uint8_t GX_CULL_NONE = 0U;
        constexpr std::uint8_t GX_CULL_FRONT = 1U;
        constexpr std::uint8_t GX_CULL_BACK = 2U;
        constexpr std::uint8_t GX_CULL_ALL = 3U;

        if (cull_mode == GX_CULL_NONE || cull_mode == 0xFFU) {
            return false;
        }
        if (cull_mode == GX_CULL_ALL) {
            return true;
        }

        const float signed_area = (v1.x - v0.x) * (v2.y - v0.y) - (v1.y - v0.y) * (v2.x - v0.x);
        if (std::fabs(signed_area) <= 0.0001F) {
            return true;
        }

        const bool front_facing = signed_area < 0.0F;
        if (cull_mode == GX_CULL_FRONT) {
            return front_facing;
        }
        if (cull_mode == GX_CULL_BACK) {
            return !front_facing;
        }
        return false;
    }

}  // namespace

assets::AssetResult< FileSelectSkyJ3d > FileSelectSkyJ3d::parse(std::span< const std::byte > bdlBytes, std::span< const std::byte > bckBytes,
                                                                std::span< const std::byte > btkBytes) {
    auto model = assets::layout::parse_j3d_model(bdlBytes);
    if (!model) {
        return model.failure();
    }

    FileSelectSkyJ3d sky{};
    sky._model = std::move(*model);
    const auto copy_track = [](const BckTrack& source_track) {
        AnimationTrack track{};
        track.valid = source_track.valid;
        track.keys.reserve(source_track.keys.size());
        for (const auto& key : source_track.keys) {
            track.keys.push_back(AnimationKey{
                .frame = key.frame,
                .value = key.value,
                .tangent_in = key.tangent_in,
                .tangent_out = key.tangent_out,
            });
        }
        return track;
    };
    if (const auto animation = parse_bck_animation(bckBytes)) {
        sky._jointAnimationFrameMax = animation->frame_max;
        sky._jointAnimations.reserve(animation->joints.size());
        for (const auto& source_joint : animation->joints) {
            JointAnimation joint_animation{};
            for (std::size_t axis = 0U; axis < 3U; ++axis) {
                joint_animation.scale[axis] = copy_track(source_joint.scale[axis]);
                joint_animation.rotation[axis] = copy_track(source_joint.rotation[axis]);
                joint_animation.translation[axis] = copy_track(source_joint.translation[axis]);
            }
            sky._jointAnimations.push_back(std::move(joint_animation));
        }
    }
    if (const auto animation = parse_btk_animation(btkBytes)) {
        sky._textureMatrixAnimationFrameMax = animation->frame_max;
        sky._textureMatrixAnimations.reserve(animation->materials.size());
        for (const auto& source_material : animation->materials) {
            if (source_material.texture_matrix_index == 0xFFU) {
                continue;
            }

            auto material_index = static_cast< std::size_t >(assets::layout::J3D_NO_TEXTURE_INDEX);
            if (!source_material.material_name.empty()) {
                const auto found =
                    std::find_if(sky._model.materials.begin(), sky._model.materials.end(), [&](const assets::layout::J3dMaterial& material) {
                        return material.name == source_material.material_name;
                    });
                if (found != sky._model.materials.end()) {
                    material_index = static_cast< std::size_t >(std::distance(sky._model.materials.begin(), found));
                }
            }
            if (material_index == static_cast< std::size_t >(assets::layout::J3D_NO_TEXTURE_INDEX) &&
                source_material.material_id < sky._model.materials.size()) {
                material_index = source_material.material_id;
            }
            if (material_index >= sky._model.materials.size()) {
                continue;
            }

            sky._textureMatrixAnimations.push_back(TextureMatrixAnimation{
                .material_index = material_index,
                .texture_matrix_index = source_material.texture_matrix_index,
                .center = source_material.center,
                .scale_x = copy_track(source_material.scale_x),
                .scale_y = copy_track(source_material.scale_y),
                .rotation = copy_track(source_material.rotation),
                .translation_x = copy_track(source_material.translation_x),
                .translation_y = copy_track(source_material.translation_y),
            });
        }
    }
    return sky;
}

std::size_t FileSelectSkyJ3d::triangleCount() const {
    std::size_t count = 0U;
    for (const auto& shape : _model.shapes) {
        count += shape.triangles.size();
    }
    return count;
}

bool FileSelectSkyJ3d::empty() const {
    return triangleCount() == 0U || _model.materials.empty() || _model.textures.empty();
}

void FileSelectSkyJ3d::appendDrawCommands(render::layout::LayoutDrawList* pDrawList, float frame, std::size_t selectedFileIndex,
                                          bool useNearCamera) const {
    appendDrawCommands(pDrawList, frame, selectedFileIndex, useNearCamera ? FileSelectSkyCameraMode::Near : FileSelectSkyCameraMode::Far);
}

void FileSelectSkyJ3d::appendDrawCommands(render::layout::LayoutDrawList* pDrawList, float frame, std::size_t selectedFileIndex,
                                          FileSelectSkyCameraMode cameraMode) const {
    if (pDrawList == nullptr || empty()) {
        return;
    }

    constexpr float OUTPUT_WIDTH = 836.0F;
    constexpr float OUTPUT_HEIGHT = 456.0F;
    constexpr float NEAR_Z = 64.0F;

    std::vector< ProjectedTriangle > projected{};
    projected.reserve(triangleCount());
    const auto triangle_limit = triangle_limit_from_environment();
    const float sky_frame = frame + frame_offset_from_environment();
    const bool debug_solid = environment_flag("SMGPC_FILE_SELECT_LIVE_J3D_SKY_SOLID");
    const bool debug_trace = environment_flag("SMGPC_FILE_SELECT_LIVE_J3D_SKY_TRACE");
    const bool debug_trace_materials = environment_flag("SMGPC_FILE_SELECT_LIVE_J3D_SKY_TRACE_MATERIALS");
    std::size_t accepted_triangle_count = 0U;
    float accepted_min_x = std::numeric_limits< float >::max();
    float accepted_min_y = std::numeric_limits< float >::max();
    float accepted_max_x = -std::numeric_limits< float >::max();
    float accepted_max_y = -std::numeric_limits< float >::max();
    std::vector< MaterialTrace > material_traces(debug_trace_materials ? _model.materials.size() : 0U);
    const float opaque_alpha_scale = environment_float_or("SMGPC_FILE_SELECT_LIVE_J3D_SKY_OPAQUE_ALPHA_SCALE", 0.01F);
    const float additive_alpha_scale = environment_float_or("SMGPC_FILE_SELECT_LIVE_J3D_SKY_ADDITIVE_ALPHA_SCALE", 0.92F);
    const auto requested_secondary_texture_mode = secondary_texture_combine_mode_from_environment();

    std::vector< Mat34 > joint_matrices{};
    joint_matrices.reserve(_model.joints.size());
    const auto sample_animation_track = [](const AnimationTrack& track, float frame_position) {
        if (!track.valid || track.keys.empty()) {
            return 0.0F;
        }
        if (track.keys.size() == 1U || frame_position <= track.keys.front().frame) {
            return track.keys.front().value;
        }
        for (std::size_t index = 1U; index < track.keys.size(); ++index) {
            const auto& right_key = track.keys[index];
            if (frame_position > right_key.frame) {
                continue;
            }

            const auto& left_key = track.keys[index - 1U];
            const float span = right_key.frame - left_key.frame;
            if (span <= 0.0001F) {
                return right_key.value;
            }

            const float rate = clamp_float((frame_position - left_key.frame) / span, 0.0F, 1.0F);
            const float rate2 = rate * rate;
            const float rate3 = rate2 * rate;
            const float h00 = 2.0F * rate3 - 3.0F * rate2 + 1.0F;
            const float h10 = rate3 - 2.0F * rate2 + rate;
            const float h01 = -2.0F * rate3 + 3.0F * rate2;
            const float h11 = rate3 - rate2;
            return h00 * left_key.value + h10 * span * left_key.tangent_out + h01 * right_key.value +
                   h11 * span * right_key.tangent_in;
        }
        return track.keys.back().value;
    };
    const auto set_axis = [](assets::layout::J3dVec3* value, std::size_t axis, float axis_value) {
        if (axis == 0U) {
            value->x = axis_value;
        } else if (axis == 1U) {
            value->y = axis_value;
        } else {
            value->z = axis_value;
        }
    };
    const auto animated_joint_matrix = [&](std::size_t joint_index) {
        const auto& joint = _model.joints[joint_index];
        if (joint_index >= _jointAnimations.size()) {
            return joint_matrix(joint);
        }

        const auto frame_position = animation_frame(sky_frame, _jointAnimationFrameMax);
        const auto& animation = _jointAnimations[joint_index];
        auto scale = joint.scale;
        std::array< float, 3U > rotation{
            j3d_rotation_to_radians(joint.rotation[0U]),
            j3d_rotation_to_radians(joint.rotation[1U]),
            j3d_rotation_to_radians(joint.rotation[2U]),
        };
        auto translation = joint.translation;

        for (std::size_t axis = 0U; axis < 3U; ++axis) {
            if (animation.scale[axis].valid) {
                set_axis(&scale, axis, sample_animation_track(animation.scale[axis], frame_position));
            }
            if (animation.rotation[axis].valid) {
                rotation[axis] = sample_animation_track(animation.rotation[axis], frame_position);
            }
            if (animation.translation[axis].valid) {
                set_axis(&translation, axis, sample_animation_track(animation.translation[axis], frame_position));
            }
        }

        return pose_matrix(scale, rotation, translation);
    };

    for (std::size_t joint_index = 0U; joint_index < _model.joints.size(); ++joint_index) {
        const auto& joint = _model.joints[joint_index];
        auto local = animated_joint_matrix(joint_index);
        if (joint.parent_index != assets::layout::J3D_NO_JOINT_INDEX && joint.parent_index < joint_matrices.size()) {
            local = multiply_matrix(joint_matrices[joint.parent_index], local);
        }
        joint_matrices.push_back(local);
    }
    const auto actor_matrix = file_select_sky_actor_matrix(sky_frame);
    for (auto& matrix : joint_matrices) {
        matrix = multiply_matrix(actor_matrix, matrix);
    }
    const auto inverse_actor_base_matrix = invert_affine_matrix(file_select_sky_base_matrix(sky_frame));

    auto camera = file_select_camera(selectedFileIndex, cameraMode);
    apply_camera_overrides_from_environment(&camera);
    const auto forward = normalize(subtract(camera.target, camera.eye));
    const auto right = normalize(cross(forward, Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F}));
    const auto up = normalize(cross(right, forward));
    const float focal_y = (OUTPUT_HEIGHT * 0.5F) / std::tan((camera.fovy_degrees * 0.017453292519943295F) * 0.5F);
    const float focal_x = focal_y;
    const FrustumClipParameters clip_parameters{
        .near_z = NEAR_Z,
        .tan_half_fovy = std::tan((camera.fovy_degrees * 0.017453292519943295F) * 0.5F),
        .aspect = OUTPUT_WIDTH / OUTPUT_HEIGHT,
    };

    const auto world_position_for_vertex = [&](const assets::layout::J3dVertex& vertex) {
        if (vertex.draw_matrix_index >= _model.draw_matrices.size()) {
            return vertex.position;
        }

        const auto& draw_matrix = _model.draw_matrices[vertex.draw_matrix_index];
        if (draw_matrix.weighted || draw_matrix.index >= joint_matrices.size()) {
            return vertex.position;
        }

        return transform_point(joint_matrices[draw_matrix.index], vertex.position);
    };

    const auto transform_texcoord = [&](const assets::layout::J3dVertex& vertex, std::size_t material_index, std::size_t texture_coordinate_slot,
                                        Vec3 world_position, Vec3 camera_position) {
        constexpr std::uint8_t GX_TG_POS = 0U;
        constexpr std::uint8_t GX_TG_TEX0 = 4U;
        constexpr std::uint8_t GX_TEXMTX0 = 30U;
        constexpr std::uint8_t GX_TEXMTX9 = 57U;
        constexpr std::uint8_t GX_IDENTITY = 60U;

        if (material_index >= _model.materials.size() || texture_coordinate_slot >= _model.materials[material_index].texture_coord_generators.size()) {
            return to_generated_texcoord(vertex.texcoord);
        }

        const auto& material = _model.materials[material_index];
        std::size_t texture_matrix_slot = 0U;
        const auto& generator = material.texture_coord_generators[texture_coordinate_slot];
        if (generator.valid) {
            if (generator.matrix == GX_IDENTITY) {
                return to_generated_texcoord(vertex.texcoord);
            }
            if (generator.matrix < GX_TEXMTX0 || generator.matrix > GX_TEXMTX9 ||
                ((generator.matrix - GX_TEXMTX0) % 3U) != 0U) {
                return to_generated_texcoord(vertex.texcoord);
            }
            texture_matrix_slot = static_cast< std::size_t >((generator.matrix - GX_TEXMTX0) / 3U);
            if (texture_matrix_slot >= material.texture_matrices.size()) {
                return to_generated_texcoord(vertex.texcoord);
            }
        }

        const auto texture_coordinate_from_matrix = [](const Mat34& matrix, Vec3 source, bool preserve_q) {
            const float s = matrix.m[0U][0U] * source.x + matrix.m[0U][1U] * source.y + matrix.m[0U][2U] * source.z + matrix.m[0U][3U];
            const float t = matrix.m[1U][0U] * source.x + matrix.m[1U][1U] * source.y + matrix.m[1U][2U] * source.z + matrix.m[1U][3U];
            const float q = matrix.m[2U][0U] * source.x + matrix.m[2U][1U] * source.y + matrix.m[2U][2U] * source.z + matrix.m[2U][3U];
            return GeneratedTexCoord{
                .s = s,
                .t = t,
                .q = preserve_q ? q : 1.0F,
            };
        };

        assets::layout::J3dVec3 center{0.5F, 0.5F, 0.5F};
        float scale_x = 1.0F;
        float scale_y = 1.0F;
        float rotation = 0.0F;
        float translation_x = 0.0F;
        float translation_y = 0.0F;
        bool has_transform = false;
        const auto& static_matrix = material.texture_matrices[texture_matrix_slot];
        if (static_matrix.valid) {
            center = static_matrix.center;
            scale_x = static_matrix.srt.scale_x;
            scale_y = static_matrix.srt.scale_y;
            rotation = j3d_rotation_to_radians(static_matrix.srt.rotation);
            translation_x = static_matrix.srt.translation_x;
            translation_y = static_matrix.srt.translation_y;
            has_transform = true;
        }

        const auto found = std::find_if(
            _textureMatrixAnimations.begin(), _textureMatrixAnimations.end(), [&](const TextureMatrixAnimation& animation) {
                return animation.material_index == material_index && animation.texture_matrix_index == texture_matrix_slot;
            });
        if (found == _textureMatrixAnimations.end() && !has_transform) {
            return to_generated_texcoord(vertex.texcoord);
        }

        if (found != _textureMatrixAnimations.end()) {
            const float frame_position = animation_frame(sky_frame, _textureMatrixAnimationFrameMax);
            center = found->center;
            if (found->scale_x.valid) {
                scale_x = sample_animation_track(found->scale_x, frame_position);
            }
            if (found->scale_y.valid) {
                scale_y = sample_animation_track(found->scale_y, frame_position);
            }
            if (found->rotation.valid) {
                rotation = sample_animation_track(found->rotation, frame_position);
            }
            if (found->translation_x.valid) {
                translation_x = sample_animation_track(found->translation_x, frame_position);
            }
            if (found->translation_y.valid) {
                translation_y = sample_animation_track(found->translation_y, frame_position);
            }
        }

        const auto mode = static_matrix.valid ? static_cast< std::uint8_t >(static_matrix.info & 0x3FU) : static_cast< std::uint8_t >(0U);
        const bool is_projection_mode = mode == 2U || mode == 3U || mode == 4U || mode == 5U || mode == 8U || mode == 9U || mode == 10U || mode == 11U;
        if (generator.valid && generator.source == GX_TG_POS && static_matrix.valid && is_projection_mode) {
            const bool use_old_layout = mode == 2U || mode == 3U || mode == 4U || mode == 5U || mode == 6U || mode == 10U;
            const bool use_q_matrix = mode == 7U || mode == 8U || mode == 9U || mode == 10U || mode == 11U;
            const bool use_effect_matrix = mode == 2U || mode == 3U || mode == 4U || mode == 5U || mode == 8U || mode == 9U || mode == 10U || mode == 11U;
            const bool use_camera_source = mode == 3U || mode == 9U;
            auto texture_matrix = j3d_texture_matrix(scale_x, scale_y, rotation, translation_x, translation_y, center, use_old_layout);
            if (use_q_matrix) {
                texture_matrix = multiply_matrix(texture_matrix, mat44_from_mat34(j3d_projection_q_matrix(use_old_layout)));
            }
            if (use_effect_matrix) {
                const auto effect_matrix = multiply_matrix(mat44_from_effect_matrix(static_matrix.effect_matrix), inverse_actor_base_matrix);
                texture_matrix = multiply_matrix(texture_matrix, effect_matrix);
            }
            return texture_coordinate_from_matrix(texture_matrix,
                                                  use_camera_source ? camera_position : world_position,
                                                  static_matrix.projection != 0U &&
                                                      !environment_flag("SMGPC_TITLE_WORLD_J3D_FORCE_TEXQ_ONE"));
        }

        if (mode != 0U) {
            return to_generated_texcoord(vertex.texcoord);
        }

        if (generator.valid && generator.source != GX_TG_TEX0) {
            return to_generated_texcoord(vertex.texcoord);
        }

        const float sine = std::sin(rotation);
        const float cosine = std::cos(rotation);
        const float cx = scale_x * cosine;
        const float sx = scale_x * sine;
        const float sy = scale_y * sine;
        const float cy = scale_y * cosine;
        const float center_x = center.x;
        const float center_y = center.y;
        return GeneratedTexCoord{
            .s = cx * vertex.texcoord.x - sx * vertex.texcoord.y + (-cx * center_x + sx * center_y) + center_x + translation_x,
            .t = sy * vertex.texcoord.x + cy * vertex.texcoord.y + (-sy * center_x - cy * center_y) + center_y + translation_y,
            .q = 1.0F,
        };
    };

    const auto camera_vertex = [&](const assets::layout::J3dVertex& vertex, const assets::layout::J3dMaterial& material,
                                   std::size_t material_index, std::size_t texture_coordinate_slot,
                                   std::size_t secondary_texture_coordinate_slot, float alpha_scale) {
        const auto world = to_vec3(world_position_for_vertex(vertex));
        const auto camera_relative = subtract(world, camera.eye);
        const float camera_x = dot(camera_relative, right);
        const float camera_y = dot(camera_relative, up);
        const float camera_z = dot(camera_relative, forward);
        const Vec3 camera_position{
            .x = camera_x,
            .y = camera_y,
            .z = camera_z,
        };
        const auto texcoord = transform_texcoord(vertex, material_index, texture_coordinate_slot, world, camera_position);
        const auto secondary_texcoord = transform_texcoord(vertex, material_index, secondary_texture_coordinate_slot, world, camera_position);
        return CameraVertex{
            .x = camera_x,
            .y = camera_y,
            .z = camera_z,
            .u = texcoord.s,
            .v = texcoord.t,
            .q = texcoord.q,
            .u_secondary = secondary_texcoord.s,
            .v_secondary = secondary_texcoord.t,
            .q_secondary = secondary_texcoord.q,
            .color = debug_solid ? render::layout::pack_abgr(255U, 64U, 64U, 230U) : multiply_color(vertex.color, material.material_color, alpha_scale),
        };
    };

    const auto project_camera_vertex = [&](const CameraVertex& vertex) {
        const auto texcoord = perspective_correct_layout_texcoord(vertex.u, vertex.v, vertex.q, vertex.z);
        const auto secondary_texcoord = perspective_correct_layout_texcoord(vertex.u_secondary, vertex.v_secondary, vertex.q_secondary, vertex.z);
        return ProjectedVertex{
            .x = OUTPUT_WIDTH * 0.5F + (vertex.x * focal_x) / vertex.z,
            .y = OUTPUT_HEIGHT * 0.5F - (vertex.y * focal_y) / vertex.z,
            .z = vertex.z,
            .u = texcoord.s,
            .v = texcoord.t,
            .q = texcoord.q,
            .u_secondary = secondary_texcoord.s,
            .v_secondary = secondary_texcoord.t,
            .q_secondary = secondary_texcoord.q,
            .color = vertex.color,
        };
    };

    for (const auto& shape : _model.shapes) {
        if (shape.material_index >= _model.materials.size()) {
            continue;
        }

        const auto& material = _model.materials[shape.material_index];
        const auto texture_selections = material_texture_selections(material, _model.textures, 2U);
        const auto texture = !texture_selections.empty() ? texture_ref(_model.textures[material.texture_indices[texture_selections[0U].texture_map_slot]]) :
                                                           empty_texture_ref();
        const std::size_t texture_coordinate_slot = !texture_selections.empty() ? texture_selections[0U].texture_coordinate_slot : 0U;
        std::array< render::layout::TriangleTevStage, 2U > tev_stages{};
        const auto tev_stage_count = populate_triangle_tev_stages(&tev_stages, material);
        const auto secondary_texture_mode = requested_secondary_texture_mode.value_or(default_secondary_texture_mode_for_j3d_tev(tev_stage_count));
        const bool has_secondary_texture =
            texture_selections.size() > 1U && secondary_texture_mode != render::layout::TriangleTextureCombineMode::None;
        const std::uint8_t effective_tev_stage_count =
            has_secondary_texture && secondary_texture_mode == render::layout::TriangleTextureCombineMode::J3dTevColorStages ? tev_stage_count : 0U;
        const auto secondary_texture = has_secondary_texture ?
                                           texture_ref(_model.textures[material.texture_indices[texture_selections[1U].texture_map_slot]]) :
                                           empty_texture_ref();
        const std::size_t secondary_texture_coordinate_slot =
            has_secondary_texture ? texture_selections[1U].texture_coordinate_slot : texture_coordinate_slot;
        const bool additive = material.blend.valid ?
                                  (material.blend.type == 1U && material.blend.destination_factor == 1U) :
                                  (material.name.find("Halo") != std::string::npos || material.name.find("Sky") != std::string::npos ||
                                   material.name.find("Sun") != std::string::npos);
        const float material_alpha_scale = sky_material_alpha_scale_from_environment(material, additive, opaque_alpha_scale, additive_alpha_scale);
        for (const auto& triangle : shape.triangles) {
            if (projected.size() >= triangle_limit) {
                break;
            }

            const auto camera_v0 =
                camera_vertex(triangle.v0, material, shape.material_index, texture_coordinate_slot, secondary_texture_coordinate_slot, material_alpha_scale);
            const auto camera_v1 =
                camera_vertex(triangle.v1, material, shape.material_index, texture_coordinate_slot, secondary_texture_coordinate_slot, material_alpha_scale);
            const auto camera_v2 =
                camera_vertex(triangle.v2, material, shape.material_index, texture_coordinate_slot, secondary_texture_coordinate_slot, material_alpha_scale);
            if (!is_finite_camera_vertex(camera_v0) || !is_finite_camera_vertex(camera_v1) || !is_finite_camera_vertex(camera_v2)) {
                continue;
            }

            const auto clipped = clip_triangle_to_frustum(std::array< CameraVertex, 3U >{camera_v0, camera_v1, camera_v2}, clip_parameters);
            if (clipped.count < 3U) {
                continue;
            }

            for (std::size_t clipped_index = 1U; clipped_index + 1U < clipped.count; ++clipped_index) {
                if (projected.size() >= triangle_limit) {
                    break;
                }

                const auto v0 = project_camera_vertex(clipped.vertices[0U]);
                const auto v1 = project_camera_vertex(clipped.vertices[clipped_index]);
                const auto v2 = project_camera_vertex(clipped.vertices[clipped_index + 1U]);
                if (!is_finite_vertex(v0) || !is_finite_vertex(v1) || !is_finite_vertex(v2)) {
                    continue;
                }
                if (is_triangle_culled(v0, v1, v2, material.cull_mode)) {
                    continue;
                }

                const float min_x = std::min({v0.x, v1.x, v2.x});
                const float max_x = std::max({v0.x, v1.x, v2.x});
                const float min_y = std::min({v0.y, v1.y, v2.y});
                const float max_y = std::max({v0.y, v1.y, v2.y});
                if (max_x < -2048.0F || min_x > OUTPUT_WIDTH + 2048.0F || max_y < -2048.0F || min_y > OUTPUT_HEIGHT + 2048.0F ||
                    std::max({std::fabs(min_x), std::fabs(max_x), std::fabs(min_y), std::fabs(max_y)}) > 100000.0F) {
                    continue;
                }
                ++accepted_triangle_count;
                accepted_min_x = std::min(accepted_min_x, min_x);
                accepted_min_y = std::min(accepted_min_y, min_y);
                accepted_max_x = std::max(accepted_max_x, max_x);
                accepted_max_y = std::max(accepted_max_y, max_y);
                if (debug_trace_materials && shape.material_index < material_traces.size()) {
                    auto& trace = material_traces[shape.material_index];
                    ++trace.accepted_triangle_count;
                    trace.min_x = std::min(trace.min_x, min_x);
                    trace.min_y = std::min(trace.min_y, min_y);
                    trace.max_x = std::max(trace.max_x, max_x);
                    trace.max_y = std::max(trace.max_y, max_y);
                }

                projected.push_back(ProjectedTriangle{
                    .quad =
                        render::layout::QuadCommand{
                            .x0 = min_x,
                            .y0 = min_y,
                            .x1 = max_x,
                            .y1 = max_y,
                            .use_custom_vertices = true,
                            .x_tl = v0.x,
                            .y_tl = v0.y,
                            .x_tr = v1.x,
                            .y_tr = v1.y,
                            .x_bl = v2.x,
                            .y_bl = v2.y,
                            .x_br = v2.x,
                            .y_br = v2.y,
                            .use_custom_tex_coords = true,
                            .u_tl = v0.u,
                            .v_tl = v0.v,
                            .q_tl = v0.q,
                            .u_tr = v1.u,
                            .v_tr = v1.v,
                            .q_tr = v1.q,
                            .u_bl = v2.u,
                            .v_bl = v2.v,
                            .q_bl = v2.q,
                            .u_br = v2.u,
                            .v_br = v2.v,
                            .q_br = v2.q,
                            .u_tl_secondary = v0.u_secondary,
                            .v_tl_secondary = v0.v_secondary,
                            .q_tl_secondary = v0.q_secondary,
                            .u_tr_secondary = v1.u_secondary,
                            .v_tr_secondary = v1.v_secondary,
                            .q_tr_secondary = v1.q_secondary,
                            .u_bl_secondary = v2.u_secondary,
                            .v_bl_secondary = v2.v_secondary,
                            .q_bl_secondary = v2.q_secondary,
                            .u_br_secondary = v2.u_secondary,
                            .v_br_secondary = v2.v_secondary,
                            .q_br_secondary = v2.q_secondary,
                            .color_tl = v0.color,
                            .color_tr = v1.color,
                            .color_bl = v2.color,
                            .color_br = v2.color,
                            .blend_mode = additive ? render::layout::BlendMode::Additive : render::layout::BlendMode::Alpha,
                            .tev_color0 = pack_tev_k_color(material.tev_k_colors[0U]),
                            .tev_color1 = pack_tev_color_s10(material.tev_colors[0U]),
                            .texture = texture,
                            .mask_texture = secondary_texture,
                        },
                    .secondary_texture_mode =
                        has_secondary_texture ? secondary_texture_mode : render::layout::TriangleTextureCombineMode::None,
                    .tev_stage_count = effective_tev_stage_count,
                    .tev_stages = tev_stages,
                    .depth = (v0.z + v1.z + v2.z) / 3.0F,
                });
            }
        }
        if (projected.size() >= triangle_limit) {
            break;
        }
    }

    std::sort(projected.begin(), projected.end(), [](const ProjectedTriangle& lhs, const ProjectedTriangle& rhs) {
        return lhs.depth > rhs.depth;
    });

    std::vector< render::layout::TriangleBatchCommand > batches{};
    batches.reserve(_model.materials.size());
    for (const auto& triangle : projected) {
        const auto can_append_to_batch = [&](const render::layout::TriangleBatchCommand& batch) {
            return batch.blend_mode == triangle.quad.blend_mode && batch.secondary_texture_mode == triangle.secondary_texture_mode &&
                   batch.tev_color0 == triangle.quad.tev_color0 && batch.tev_color1 == triangle.quad.tev_color1 &&
                   batch.tev_stage_count == triangle.tev_stage_count && tev_stage_array_equal(batch.tev_stages, triangle.tev_stages) &&
                   batch.texture.id == triangle.quad.texture.id &&
                   batch.texture.rgba8 == triangle.quad.texture.rgba8 && batch.texture.width == triangle.quad.texture.width &&
                   batch.texture.height == triangle.quad.texture.height && batch.texture.wrap_s == triangle.quad.texture.wrap_s &&
                   batch.texture.wrap_t == triangle.quad.texture.wrap_t && batch.secondary_texture.id == triangle.quad.mask_texture.id &&
                   batch.secondary_texture.rgba8 == triangle.quad.mask_texture.rgba8 &&
                   batch.secondary_texture.width == triangle.quad.mask_texture.width &&
                   batch.secondary_texture.height == triangle.quad.mask_texture.height &&
                   batch.secondary_texture.wrap_s == triangle.quad.mask_texture.wrap_s &&
                   batch.secondary_texture.wrap_t == triangle.quad.mask_texture.wrap_t;
        };
        if (batches.empty() || !can_append_to_batch(batches.back())) {
            batches.push_back(render::layout::TriangleBatchCommand{
                .blend_mode = triangle.quad.blend_mode,
                .secondary_texture_mode = triangle.secondary_texture_mode,
                .tev_color0 = triangle.quad.tev_color0,
                .tev_color1 = triangle.quad.tev_color1,
                .tev_stage_count = triangle.tev_stage_count,
                .tev_stages = triangle.tev_stages,
                .texture = triangle.quad.texture,
                .secondary_texture = triangle.quad.mask_texture,
            });
        }

        auto& batch = batches.back();
        batch.vertices.push_back(render::layout::TriangleVertex{
            .x = triangle.quad.x_tl,
            .y = triangle.quad.y_tl,
            .u = triangle.quad.u_tl,
            .v = triangle.quad.v_tl,
            .q = triangle.quad.q_tl,
            .u_secondary = triangle.quad.u_tl_secondary,
            .v_secondary = triangle.quad.v_tl_secondary,
            .q_secondary = triangle.quad.q_tl_secondary,
            .color = triangle.quad.color_tl,
        });
        batch.vertices.push_back(render::layout::TriangleVertex{
            .x = triangle.quad.x_tr,
            .y = triangle.quad.y_tr,
            .u = triangle.quad.u_tr,
            .v = triangle.quad.v_tr,
            .q = triangle.quad.q_tr,
            .u_secondary = triangle.quad.u_tr_secondary,
            .v_secondary = triangle.quad.v_tr_secondary,
            .q_secondary = triangle.quad.q_tr_secondary,
            .color = triangle.quad.color_tr,
        });
        batch.vertices.push_back(render::layout::TriangleVertex{
            .x = triangle.quad.x_bl,
            .y = triangle.quad.y_bl,
            .u = triangle.quad.u_bl,
            .v = triangle.quad.v_bl,
            .q = triangle.quad.q_bl,
            .u_secondary = triangle.quad.u_bl_secondary,
            .v_secondary = triangle.quad.v_bl_secondary,
            .q_secondary = triangle.quad.q_bl_secondary,
            .color = triangle.quad.color_bl,
        });
    }

    for (auto& batch : batches) {
        if (!batch.vertices.empty()) {
            pDrawList->push_triangle_batch(std::move(batch));
        }
    }

    if (debug_trace) {
        std::fprintf(stderr,
                     "SMGPC live J3D sky: frame=%.3f accepted=%zu batches=%zu bbox=[%.2f,%.2f]-[%.2f,%.2f]\n",
                     sky_frame,
                     accepted_triangle_count,
                     batches.size(),
                     accepted_min_x,
                     accepted_min_y,
                     accepted_max_x,
                     accepted_max_y);
    }
    if (debug_trace_materials) {
        for (std::size_t material_index = 0U; material_index < material_traces.size(); ++material_index) {
            const auto& trace = material_traces[material_index];
            if (trace.accepted_triangle_count == 0U) {
                continue;
            }
            const auto& material = _model.materials[material_index];
            std::fprintf(stderr,
                         "SMGPC live J3D sky material: index=%zu name=%s accepted=%zu bbox=[%.2f,%.2f]-[%.2f,%.2f]\n",
                         material_index,
                         material.name.c_str(),
                         trace.accepted_triangle_count,
                         trace.min_x,
                         trace.min_y,
                         trace.max_x,
                         trace.max_y);
        }
    }
}

void FileSelectSkyJ3d::appendJ3dDrawCommands(render::core::RenderCommandBuffer* pCommands, float frame, std::uint16_t framebufferWidth,
                                             std::uint16_t framebufferHeight, std::size_t selectedFileIndex,
                                             FileSelectSkyCameraMode cameraMode) const {
    if (pCommands == nullptr || empty()) {
        return;
    }

    const auto triangle_limit = triangle_limit_from_environment();
    const float sky_frame = frame + frame_offset_from_environment();
    const bool debug_solid = environment_flag("SMGPC_FILE_SELECT_LIVE_J3D_SKY_SOLID");
    const bool debug_trace = environment_flag("SMGPC_FILE_SELECT_LIVE_J3D_SKY_TRACE");
    const char* title_material_filter = std::getenv("SMGPC_TITLE_WORLD_J3D_ONLY_MATERIAL");
    const char* title_material_skip = std::getenv("SMGPC_TITLE_WORLD_J3D_SKIP_MATERIAL");
    const bool ignore_material_color = environment_flag("SMGPC_TITLE_WORLD_J3D_IGNORE_MATERIAL_COLOR");
    const float opaque_alpha_scale = environment_float_or("SMGPC_TITLE_WORLD_J3D_OPAQUE_ALPHA_SCALE", 1.0F);
    const float additive_alpha_scale = environment_float_or("SMGPC_TITLE_WORLD_J3D_ADDITIVE_ALPHA_SCALE", 1.0F);
    const auto requested_secondary_texture_mode = secondary_texture_combine_mode_from_environment();

    const auto sample_animation_track = [](const AnimationTrack& track, float frame_position) {
        if (!track.valid || track.keys.empty()) {
            return 0.0F;
        }
        if (track.keys.size() == 1U || frame_position <= track.keys.front().frame) {
            return track.keys.front().value;
        }
        for (std::size_t index = 1U; index < track.keys.size(); ++index) {
            const auto& right_key = track.keys[index];
            if (frame_position > right_key.frame) {
                continue;
            }

            const auto& left_key = track.keys[index - 1U];
            const float span = right_key.frame - left_key.frame;
            if (span <= 0.0001F) {
                return right_key.value;
            }

            const float rate = clamp_float((frame_position - left_key.frame) / span, 0.0F, 1.0F);
            const float rate2 = rate * rate;
            const float rate3 = rate2 * rate;
            const float h00 = 2.0F * rate3 - 3.0F * rate2 + 1.0F;
            const float h10 = rate3 - 2.0F * rate2 + rate;
            const float h01 = -2.0F * rate3 + 3.0F * rate2;
            const float h11 = rate3 - rate2;
            return h00 * left_key.value + h10 * span * left_key.tangent_out + h01 * right_key.value +
                   h11 * span * right_key.tangent_in;
        }
        return track.keys.back().value;
    };
    const auto set_axis = [](assets::layout::J3dVec3* value, std::size_t axis, float axis_value) {
        if (axis == 0U) {
            value->x = axis_value;
        } else if (axis == 1U) {
            value->y = axis_value;
        } else {
            value->z = axis_value;
        }
    };
    const auto animated_joint_matrix = [&](std::size_t joint_index) {
        const auto& joint = _model.joints[joint_index];
        if (joint_index >= _jointAnimations.size()) {
            return joint_matrix(joint);
        }

        const auto frame_position = animation_frame(sky_frame, _jointAnimationFrameMax);
        const auto& animation = _jointAnimations[joint_index];
        auto scale = joint.scale;
        std::array< float, 3U > rotation{
            j3d_rotation_to_radians(joint.rotation[0U]),
            j3d_rotation_to_radians(joint.rotation[1U]),
            j3d_rotation_to_radians(joint.rotation[2U]),
        };
        auto translation = joint.translation;

        for (std::size_t axis = 0U; axis < 3U; ++axis) {
            if (animation.scale[axis].valid) {
                set_axis(&scale, axis, sample_animation_track(animation.scale[axis], frame_position));
            }
            if (animation.rotation[axis].valid) {
                rotation[axis] = sample_animation_track(animation.rotation[axis], frame_position);
            }
            if (animation.translation[axis].valid) {
                set_axis(&translation, axis, sample_animation_track(animation.translation[axis], frame_position));
            }
        }

        return pose_matrix(scale, rotation, translation);
    };

    std::vector< Mat34 > joint_matrices{};
    joint_matrices.reserve(_model.joints.size());
    for (std::size_t joint_index = 0U; joint_index < _model.joints.size(); ++joint_index) {
        const auto& joint = _model.joints[joint_index];
        auto local = animated_joint_matrix(joint_index);
        if (joint.parent_index != assets::layout::J3D_NO_JOINT_INDEX && joint.parent_index < joint_matrices.size()) {
            local = multiply_matrix(joint_matrices[joint.parent_index], local);
        }
        joint_matrices.push_back(local);
    }
    const auto actor_matrix = file_select_sky_actor_matrix(sky_frame);
    for (auto& matrix : joint_matrices) {
        matrix = multiply_matrix(actor_matrix, matrix);
    }
    const auto inverse_actor_base_matrix = invert_affine_matrix(file_select_sky_base_matrix(sky_frame));

    auto camera = file_select_camera(selectedFileIndex, cameraMode);
    apply_camera_overrides_from_environment(&camera);
    const auto forward = normalize(subtract(camera.target, camera.eye));
    const auto right = normalize(cross(forward, Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F}));
    const auto up = normalize(cross(right, forward));
    const auto world_position_for_vertex = [&](const assets::layout::J3dVertex& vertex) {
        if (vertex.draw_matrix_index >= _model.draw_matrices.size()) {
            return vertex.position;
        }

        const auto& draw_matrix = _model.draw_matrices[vertex.draw_matrix_index];
        if (draw_matrix.weighted || draw_matrix.index >= joint_matrices.size()) {
            return vertex.position;
        }

        return transform_point(joint_matrices[draw_matrix.index], vertex.position);
    };

    const auto transform_texcoord = [&](const assets::layout::J3dVertex& vertex, std::size_t material_index, std::size_t texture_coordinate_slot,
                                        Vec3 world_position, Vec3 camera_position) {
        constexpr std::uint8_t GX_TG_POS = 0U;
        constexpr std::uint8_t GX_TG_TEX0 = 4U;
        constexpr std::uint8_t GX_TEXMTX0 = 30U;
        constexpr std::uint8_t GX_TEXMTX9 = 57U;
        constexpr std::uint8_t GX_IDENTITY = 60U;

        if (material_index >= _model.materials.size() || texture_coordinate_slot >= _model.materials[material_index].texture_coord_generators.size()) {
            return to_generated_texcoord(vertex.texcoord);
        }

        const auto& material = _model.materials[material_index];
        std::size_t texture_matrix_slot = 0U;
        const auto& generator = material.texture_coord_generators[texture_coordinate_slot];
        if (generator.valid) {
            if (generator.matrix == GX_IDENTITY) {
                return to_generated_texcoord(vertex.texcoord);
            }
            if (generator.matrix < GX_TEXMTX0 || generator.matrix > GX_TEXMTX9 || ((generator.matrix - GX_TEXMTX0) % 3U) != 0U) {
                return to_generated_texcoord(vertex.texcoord);
            }
            texture_matrix_slot = static_cast< std::size_t >((generator.matrix - GX_TEXMTX0) / 3U);
            if (texture_matrix_slot >= material.texture_matrices.size()) {
                return to_generated_texcoord(vertex.texcoord);
            }
        }

        const auto texture_coordinate_from_matrix = [](const Mat34& matrix, Vec3 source, bool preserve_q) {
            const float s = matrix.m[0U][0U] * source.x + matrix.m[0U][1U] * source.y + matrix.m[0U][2U] * source.z + matrix.m[0U][3U];
            const float t = matrix.m[1U][0U] * source.x + matrix.m[1U][1U] * source.y + matrix.m[1U][2U] * source.z + matrix.m[1U][3U];
            const float q = matrix.m[2U][0U] * source.x + matrix.m[2U][1U] * source.y + matrix.m[2U][2U] * source.z + matrix.m[2U][3U];
            return GeneratedTexCoord{
                .s = s,
                .t = t,
                .q = preserve_q ? q : 1.0F,
            };
        };

        assets::layout::J3dVec3 center{0.5F, 0.5F, 0.5F};
        float scale_x = 1.0F;
        float scale_y = 1.0F;
        float rotation = 0.0F;
        float translation_x = 0.0F;
        float translation_y = 0.0F;
        bool has_transform = false;
        const auto& static_matrix = material.texture_matrices[texture_matrix_slot];
        if (static_matrix.valid) {
            center = static_matrix.center;
            scale_x = static_matrix.srt.scale_x;
            scale_y = static_matrix.srt.scale_y;
            rotation = j3d_rotation_to_radians(static_matrix.srt.rotation);
            translation_x = static_matrix.srt.translation_x;
            translation_y = static_matrix.srt.translation_y;
            has_transform = true;
        }

        const auto found = std::find_if(
            _textureMatrixAnimations.begin(), _textureMatrixAnimations.end(), [&](const TextureMatrixAnimation& animation) {
                return animation.material_index == material_index && animation.texture_matrix_index == texture_matrix_slot;
            });
        if (found == _textureMatrixAnimations.end() && !has_transform) {
            return to_generated_texcoord(vertex.texcoord);
        }

        if (found != _textureMatrixAnimations.end()) {
            const float frame_position = animation_frame(sky_frame, _textureMatrixAnimationFrameMax);
            center = found->center;
            if (found->scale_x.valid) {
                scale_x = sample_animation_track(found->scale_x, frame_position);
            }
            if (found->scale_y.valid) {
                scale_y = sample_animation_track(found->scale_y, frame_position);
            }
            if (found->rotation.valid) {
                rotation = sample_animation_track(found->rotation, frame_position);
            }
            if (found->translation_x.valid) {
                translation_x = sample_animation_track(found->translation_x, frame_position);
            }
            if (found->translation_y.valid) {
                translation_y = sample_animation_track(found->translation_y, frame_position);
            }
        }

        const auto mode = static_matrix.valid ? static_cast< std::uint8_t >(static_matrix.info & 0x3FU) : static_cast< std::uint8_t >(0U);
        const bool is_projection_mode = mode == 2U || mode == 3U || mode == 4U || mode == 5U || mode == 8U || mode == 9U || mode == 10U || mode == 11U;
        if (generator.valid && generator.source == GX_TG_POS && static_matrix.valid && is_projection_mode) {
            const bool use_old_layout = mode == 2U || mode == 3U || mode == 4U || mode == 5U || mode == 6U || mode == 10U;
            const bool use_q_matrix = mode == 7U || mode == 8U || mode == 9U || mode == 10U || mode == 11U;
            const bool use_effect_matrix = mode == 2U || mode == 3U || mode == 4U || mode == 5U || mode == 8U || mode == 9U || mode == 10U || mode == 11U;
            const bool use_camera_source = mode == 3U || mode == 9U;
            auto texture_matrix = j3d_texture_matrix(scale_x, scale_y, rotation, translation_x, translation_y, center, use_old_layout);
            if (use_q_matrix) {
                texture_matrix = multiply_matrix(texture_matrix, mat44_from_mat34(j3d_projection_q_matrix(use_old_layout)));
            }
            if (use_effect_matrix) {
                const auto effect_matrix = multiply_matrix(mat44_from_effect_matrix(static_matrix.effect_matrix), inverse_actor_base_matrix);
                texture_matrix = multiply_matrix(texture_matrix, effect_matrix);
            }
            return texture_coordinate_from_matrix(texture_matrix, use_camera_source ? camera_position : world_position, static_matrix.projection != 0U);
        }

        if (mode != 0U) {
            return to_generated_texcoord(vertex.texcoord);
        }

        if (generator.valid && generator.source != GX_TG_TEX0) {
            return to_generated_texcoord(vertex.texcoord);
        }

        const float sine = std::sin(rotation);
        const float cosine = std::cos(rotation);
        const float cx = scale_x * cosine;
        const float sx = scale_x * sine;
        const float sy = scale_y * sine;
        const float cy = scale_y * cosine;
        const float center_x = center.x;
        const float center_y = center.y;
        return GeneratedTexCoord{
            .s = cx * vertex.texcoord.x - sx * vertex.texcoord.y + (-cx * center_x + sx * center_y) + center_x + translation_x,
            .t = sy * vertex.texcoord.x + cy * vertex.texcoord.y + (-sy * center_x - cy * center_y) + center_y + translation_y,
            .q = 1.0F,
        };
    };

    const auto make_world_vertex = [&](const assets::layout::J3dVertex& vertex, const assets::layout::J3dMaterial& material,
                                       std::size_t material_index, const std::vector< MaterialTextureSelection >& texture_selections,
                                       float alpha_scale) {
        const auto world_j3d = world_position_for_vertex(vertex);
        const auto world = to_vec3(world_j3d);
        const auto camera_relative = subtract(world, camera.eye);
        const Vec3 camera_position{
            .x = dot(camera_relative, right),
            .y = dot(camera_relative, up),
            .z = dot(camera_relative, forward),
        };
        std::array< GeneratedTexCoord, 4U > texcoords{
            to_generated_texcoord(vertex.texcoord),
            to_generated_texcoord(vertex.texcoord),
            to_generated_texcoord(vertex.texcoord),
            to_generated_texcoord(vertex.texcoord),
        };
        const auto texture_count = std::min< std::size_t >(texcoords.size(), texture_selections.size());
        for (std::size_t texture_index = 0U; texture_index < texture_count; ++texture_index) {
            texcoords[texture_index] = transform_texcoord(vertex, material_index, texture_selections[texture_index].texture_coordinate_slot, world,
                                                          camera_position);
        }
        return render::core::RenderJ3dVertex{
            .x = world_j3d.x,
            .y = world_j3d.y,
            .z = world_j3d.z,
            .u = texcoords[0U].s,
            .v = texcoords[0U].t,
            .q = texcoords[0U].q,
            .u_secondary = texcoords[1U].s,
            .v_secondary = texcoords[1U].t,
            .q_secondary = texcoords[1U].q,
            .u2 = texcoords[2U].s,
            .v2 = texcoords[2U].t,
            .q2 = texcoords[2U].q,
            .u3 = texcoords[3U].s,
            .v3 = texcoords[3U].t,
            .q3 = texcoords[3U].q,
            .color = debug_solid ? render::layout::pack_abgr(255U, 64U, 64U, 230U) : raster_color(vertex.color, material, alpha_scale, ignore_material_color),
        };
    };

    const auto is_finite_world_vertex = [](const render::core::RenderJ3dVertex& vertex) {
        return std::isfinite(vertex.x) && std::isfinite(vertex.y) && std::isfinite(vertex.z) && std::isfinite(vertex.u) &&
               std::isfinite(vertex.v) && std::isfinite(vertex.q) && std::isfinite(vertex.u_secondary) && std::isfinite(vertex.v_secondary) &&
               std::isfinite(vertex.q_secondary);
    };

    render::core::RenderDrawJ3dCommand command{
        .view_id = 0U,
        .framebuffer_width = framebufferWidth,
        .framebuffer_height = framebufferHeight,
        .use_camera = true,
        .camera_eye = {camera.eye.x, camera.eye.y, camera.eye.z},
        .camera_target = {camera.target.x, camera.target.y, camera.target.z},
        .camera_up = {0.0F, 1.0F, 0.0F},
        .camera_fovy_degrees = camera.fovy_degrees,
        .camera_near = 64.0F,
        .camera_far = 1000000.0F,
    };
    command.batches.reserve(_model.shapes.size());

    std::size_t emitted_triangle_count = 0U;
    for (const auto& shape : _model.shapes) {
        if (shape.material_index >= _model.materials.size() || emitted_triangle_count >= triangle_limit) {
            continue;
        }

        const auto& material = _model.materials[shape.material_index];
        if (environment_flag("SMGPC_TITLE_WORLD_J3D_TRACE_MATERIALS")) {
            std::fprintf(stderr,
                         "SMGPC world J3D sky material: index=%u name=%s triangles=%zu\n",
                         shape.material_index,
                         material.name.c_str(),
                         shape.triangles.size());
        }
        if (title_material_filter != nullptr && title_material_filter[0U] != '\0' && material.name != title_material_filter) {
            continue;
        }
        if (title_material_skip != nullptr && title_material_skip[0U] != '\0' && material.name == title_material_skip) {
            continue;
        }
        const auto texture_selections = material_texture_selections(material, _model.textures, 4U);
        const auto texture = !texture_selections.empty() ? texture_ref(_model.textures[material.texture_indices[texture_selections[0U].texture_map_slot]]) :
                                                           empty_texture_ref();
        std::array< render::core::RenderJ3dTevStage, 4U > tev_stages{};
        const auto tev_stage_count = populate_core_j3d_tev_stages(&tev_stages, material);
        const auto secondary_texture_mode = requested_secondary_texture_mode.value_or(
            tev_stage_count > 0U ? render::layout::TriangleTextureCombineMode::J3dTevColorStages : render::layout::TriangleTextureCombineMode::None);
        const bool has_j3d_tev = !texture_selections.empty() && secondary_texture_mode == render::layout::TriangleTextureCombineMode::J3dTevColorStages &&
                                 tev_stage_count > 0U;
        const bool has_secondary_texture = texture_selections.size() > 1U && secondary_texture_mode != render::layout::TriangleTextureCombineMode::None;
        const std::uint8_t effective_tev_stage_count =
            has_j3d_tev ? tev_stage_count : 0U;
        const auto secondary_texture = has_secondary_texture ?
                                           texture_ref(_model.textures[material.texture_indices[texture_selections[1U].texture_map_slot]]) :
                                           empty_texture_ref();
        const bool additive = material.blend.valid ? (material.blend.type == 1U && material.blend.destination_factor == 1U) :
                                                     (material.name.find("Halo") != std::string::npos || material.name.find("Sky") != std::string::npos ||
                                                      material.name.find("Sun") != std::string::npos);
        const float material_alpha_scale = sky_material_alpha_scale_from_environment(material, additive, opaque_alpha_scale, additive_alpha_scale);

        std::array< std::uint8_t, 4U > tev_stage_texture_indices{0U, 1U, 2U, 3U};
        std::array< std::uint8_t, 4U > tev_stage_k_color_selectors{};
        std::array< std::uint8_t, 4U > tev_stage_k_alpha_selectors{};
        std::array< render::core::RenderJ3dTevSwapChannels, 4U > tev_stage_texture_swizzles{};
        std::array< render::core::RenderJ3dTevSwapChannels, 4U > tev_stage_raster_swizzles{};
        for (std::size_t stage_index = 0U; stage_index < tev_stage_texture_indices.size() && stage_index < material.tev_orders.size(); ++stage_index) {
            const auto& order = material.tev_orders[stage_index];
            const auto& swap = material.tev_swap_modes[stage_index];
            tev_stage_k_color_selectors[stage_index] = material.tev_k_color_selectors[stage_index];
            tev_stage_k_alpha_selectors[stage_index] = material.tev_k_alpha_selectors[stage_index];
            tev_stage_texture_swizzles[stage_index] = core_swap_channels(material, swap.valid ? swap.tex_sel : 0U);
            tev_stage_raster_swizzles[stage_index] = core_swap_channels(material, swap.valid ? swap.ras_sel : 0U);
            if (!order.valid) {
                tev_stage_texture_indices[stage_index] = 0U;
                continue;
            }
            const auto found = std::find_if(texture_selections.begin(), texture_selections.end(), [&](const MaterialTextureSelection& selection) {
                return selection.texture_map_slot == static_cast< std::size_t >(order.texture_map) &&
                       selection.texture_coordinate_slot == static_cast< std::size_t >(order.texture_coordinate);
            });
            tev_stage_texture_indices[stage_index] =
                found == texture_selections.end() ? 0U : static_cast< std::uint8_t >(std::distance(texture_selections.begin(), found));
        }
        std::array< std::uint32_t, 4U > tev_colors{};
        std::array< std::uint32_t, 4U > tev_k_colors{};
        for (std::size_t color_index = 0U; color_index < tev_colors.size(); ++color_index) {
            tev_colors[color_index] = pack_tev_color_s10(material.tev_colors[color_index]);
            tev_k_colors[color_index] = pack_tev_k_color(material.tev_k_colors[color_index]);
        }
        std::array< render::core::RenderJ3dIndirectTextureOrder, 4U > indirect_texture_orders{};
        const auto indirect_stage_count =
            std::min< std::size_t >(material.indirect_texture_stage_count, material.indirect_texture_orders.size());
        for (std::size_t stage_index = 0U; stage_index < indirect_stage_count && stage_index < indirect_texture_orders.size(); ++stage_index) {
            const auto& order = material.indirect_texture_orders[stage_index];
            const auto& scale = material.indirect_texture_coord_scales[stage_index];
            const auto found = std::find_if(texture_selections.begin(), texture_selections.end(), [&](const MaterialTextureSelection& selection) {
                return selection.texture_map_slot == static_cast< std::size_t >(order.texture_map) &&
                       selection.texture_coordinate_slot == static_cast< std::size_t >(order.texture_coordinate);
            });
            indirect_texture_orders[stage_index] = render::core::RenderJ3dIndirectTextureOrder{
                .valid = order.valid && found != texture_selections.end(),
                .texture_index = found == texture_selections.end() ? 0U : static_cast< std::uint8_t >(std::distance(texture_selections.begin(), found)),
                .texture_coordinate = order.texture_coordinate,
                .texture_map = order.texture_map,
                .scale_s = scale.valid ? scale.scale_s : 0U,
                .scale_t = scale.valid ? scale.scale_t : 0U,
            };
        }
        std::array< render::core::RenderJ3dIndirectTextureMatrix, 3U > indirect_texture_matrices{};
        for (std::size_t matrix_index = 0U; matrix_index < indirect_texture_matrices.size(); ++matrix_index) {
            indirect_texture_matrices[matrix_index] = core_indirect_texture_matrix(material.indirect_texture_matrices[matrix_index]);
        }
        std::array< render::core::RenderJ3dIndirectTevStage, 4U > indirect_tev_stages{};
        for (std::size_t stage_index = 0U; stage_index < indirect_tev_stages.size(); ++stage_index) {
            indirect_tev_stages[stage_index] = core_indirect_tev_stage(material.indirect_tev_stages[stage_index]);
        }

        render::core::RenderJ3dMaterialBatch batch{
            .blend_mode = core_blend_mode(material.blend, additive),
            .cull_mode = core_cull_mode(material.cull_mode),
            .depth_mode = core_depth_mode(material.z_mode),
            .write_depth = material.z_mode.valid ? material.z_mode.update_enable != 0U : true,
            .secondary_texture_mode = has_j3d_tev ? render::core::RenderTriangleTextureCombineMode::J3dTevColorStages :
                                                     (has_secondary_texture ? core_secondary_texture_mode(secondary_texture_mode) :
                                                                              render::core::RenderTriangleTextureCombineMode::None),
            .tev_color0 = pack_tev_k_color(material.tev_k_colors[0U]),
            .tev_color1 = pack_tev_color_s10(material.tev_colors[0U]),
            .tev_colors = tev_colors,
            .tev_k_colors = tev_k_colors,
            .tev_stage_count = effective_tev_stage_count,
            .tev_stages = tev_stages,
            .tev_stage_k_color_selectors = tev_stage_k_color_selectors,
            .tev_stage_k_alpha_selectors = tev_stage_k_alpha_selectors,
            .tev_stage_texture_swizzles = tev_stage_texture_swizzles,
            .tev_stage_raster_swizzles = tev_stage_raster_swizzles,
            .alpha_compare = core_alpha_compare(material.alpha_compare),
            .indirect_texture_stage_count = static_cast< std::uint8_t >(indirect_stage_count),
            .indirect_texture_orders = indirect_texture_orders,
            .indirect_texture_matrices = indirect_texture_matrices,
            .indirect_tev_stages = indirect_tev_stages,
            .texture_count = static_cast< std::uint8_t >(std::min< std::size_t >(texture_selections.size(), 4U)),
            .tev_stage_texture_indices = tev_stage_texture_indices,
            .texture = core_texture_ref(texture),
            .secondary_texture = core_texture_ref(secondary_texture),
        };
        for (std::size_t texture_index = 0U; texture_index < batch.texture_count; ++texture_index) {
            const auto& selection = texture_selections[texture_index];
            batch.textures[texture_index] = core_texture_ref(texture_ref(_model.textures[material.texture_indices[selection.texture_map_slot]]));
        }
        batch.vertices.reserve(shape.triangles.size() * 3U);

        for (const auto& triangle : shape.triangles) {
            if (emitted_triangle_count >= triangle_limit) {
                break;
            }

            const auto v0 = make_world_vertex(triangle.v0, material, shape.material_index, texture_selections, material_alpha_scale);
            const auto v1 = make_world_vertex(triangle.v1, material, shape.material_index, texture_selections, material_alpha_scale);
            const auto v2 = make_world_vertex(triangle.v2, material, shape.material_index, texture_selections, material_alpha_scale);
            if (!is_finite_world_vertex(v0) || !is_finite_world_vertex(v1) || !is_finite_world_vertex(v2)) {
                continue;
            }

            batch.vertices.push_back(v0);
            batch.vertices.push_back(v1);
            batch.vertices.push_back(v2);
            ++emitted_triangle_count;
        }

        if (!batch.vertices.empty()) {
            if (environment_flag("SMGPC_TITLE_WORLD_J3D_TRACE_TEXCOORDS")) {
                float min_u = std::numeric_limits< float >::infinity();
                float max_u = -std::numeric_limits< float >::infinity();
                float min_v = std::numeric_limits< float >::infinity();
                float max_v = -std::numeric_limits< float >::infinity();
                float min_q = std::numeric_limits< float >::infinity();
                float max_q = -std::numeric_limits< float >::infinity();
                for (const auto& vertex : batch.vertices) {
                    min_u = std::min(min_u, vertex.u);
                    max_u = std::max(max_u, vertex.u);
                    min_v = std::min(min_v, vertex.v);
                    max_v = std::max(max_v, vertex.v);
                    min_q = std::min(min_q, vertex.q);
                    max_q = std::max(max_q, vertex.q);
                }
                std::fprintf(stderr,
                             "SMGPC world J3D texcoords: material=%s tex0 u=[%.6f,%.6f] v=[%.6f,%.6f] q=[%.6f,%.6f]\n",
                             material.name.c_str(),
                             min_u,
                             max_u,
                             min_v,
                             max_v,
                             min_q,
                             max_q);
            }
            command.batches.push_back(std::move(batch));
        }
    }

    if (!command.batches.empty()) {
        pCommands->draw_j3d(command);
    }

    if (debug_trace) {
        std::fprintf(stderr,
                     "SMGPC world J3D sky: frame=%.3f triangles=%zu batches=%zu fb=%ux%u\n",
                     sky_frame,
                     emitted_triangle_count,
                     command.batches.size(),
                     framebufferWidth,
                     framebufferHeight);
    }
}

}  // namespace smgpc::game::compat
