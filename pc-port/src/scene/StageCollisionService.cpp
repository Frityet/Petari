#include "scene/StageCollisionService.hpp"

#include "scene/StagePlacementResolver.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <stdexcept>

namespace smgpc::scene {
    namespace {
        constexpr auto cPi = 3.14159265358979323846F;
        constexpr auto cLeafTriangleCount = std::uint32_t{8U};
        constexpr auto cCollisionSkin = 1.2F;
        // Internal Binder margin queries can reconstruct an exact shell a few
        // ulps outside the face at retail-scale world coordinates. Public
        // exact sphere queries keep a zero margin and therefore zero epsilon.
        constexpr auto cCollisionContactEpsilon = 0.01F;
        constexpr auto cReactionConstraintTolerance = 1.0e-4F;
        constexpr auto cReactionPivotTolerance = 1.0e-8;
        constexpr auto cReactionTieTolerance = 1.0e-6F;
        constexpr auto cArrowEdgeTolerance = 0.01F;

        StageCollisionService* sActiveService = nullptr;
        std::atomic<std::uint64_t> sNextTriangleIndex{};

        [[nodiscard]] std::uint16_t read_be16(std::span<const std::uint8_t> bytes, std::size_t offset) {
            if (offset + 2U > bytes.size()) {
                throw std::runtime_error("KCL u16 read is outside resource");
            }
            return static_cast<std::uint16_t>((static_cast<std::uint16_t>(bytes[offset]) << 8U) |
                                              static_cast<std::uint16_t>(bytes[offset + 1U]));
        }

        [[nodiscard]] std::uint32_t read_be32(std::span<const std::uint8_t> bytes, std::size_t offset) {
            if (offset + 4U > bytes.size()) {
                throw std::runtime_error("KCL u32 read is outside resource");
            }
            return (static_cast<std::uint32_t>(bytes[offset]) << 24U) |
                   (static_cast<std::uint32_t>(bytes[offset + 1U]) << 16U) |
                   (static_cast<std::uint32_t>(bytes[offset + 2U]) << 8U) |
                   static_cast<std::uint32_t>(bytes[offset + 3U]);
        }

        [[nodiscard]] float read_bef32(std::span<const std::uint8_t> bytes, std::size_t offset) {
            return std::bit_cast<float>(read_be32(bytes, offset));
        }

        [[nodiscard]] TVec3f read_vec3(std::span<const std::uint8_t> bytes, std::size_t offset) {
            return TVec3f(read_bef32(bytes, offset), read_bef32(bytes, offset + 4U), read_bef32(bytes, offset + 8U));
        }

        [[nodiscard]] TVec3f cross(const TVec3f& lhs, const TVec3f& rhs) {
            return TVec3f(lhs.y * rhs.z - lhs.z * rhs.y, lhs.z * rhs.x - lhs.x * rhs.z,
                          lhs.x * rhs.y - lhs.y * rhs.x);
        }

        [[nodiscard]] float dot(const TVec3f& lhs, const TVec3f& rhs) {
            return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
        }

        [[nodiscard]] float length_squared(const TVec3f& value) {
            return dot(value, value);
        }

        [[nodiscard]] bool normalize(TVec3f& value) {
            const auto square_length = length_squared(value);
            if (!(square_length > 1.0e-12F) || !std::isfinite(square_length)) {
                value.zero();
                return false;
            }
            value.scale(1.0F / std::sqrt(square_length));
            return true;
        }

        [[nodiscard]] TVec3f component_extrema_reaction(
            const std::vector<StageCollisionContact>& contacts) {
            auto positive = TVec3f{};
            auto negative = TVec3f{};
            for (const auto& contact : contacts) {
                const auto reaction = contact.normal * contact.penetration;
                positive.x = std::max(positive.x, reaction.x);
                positive.y = std::max(positive.y, reaction.y);
                positive.z = std::max(positive.z, reaction.z);
                negative.x = std::min(negative.x, reaction.x);
                negative.y = std::min(negative.y, reaction.y);
                negative.z = std::min(negative.z, reaction.z);
            }
            return positive + negative;
        }

        [[nodiscard]] bool solve_active_reaction(
            const std::vector<StageCollisionContact>& contacts,
            const std::array<std::size_t, 3U>& active_indices,
            std::size_t active_count, TVec3f* reaction) {
            auto system = std::array<std::array<double, 4U>, 3U>{};
            for (auto row = std::size_t{}; row < active_count; ++row) {
                const auto& row_contact = contacts[active_indices[row]];
                for (auto column = std::size_t{}; column < active_count; ++column) {
                    system[row][column] = static_cast<double>(
                        dot(row_contact.normal,
                            contacts[active_indices[column]].normal));
                }
                system[row][active_count] =
                    static_cast<double>(row_contact.penetration);
            }

            // The active set is at most three planes in three-dimensional
            // space. Partial-pivot Gaussian elimination rejects dependent
            // normals so another smaller independent subset can win.
            for (auto column = std::size_t{}; column < active_count; ++column) {
                auto pivot_row = column;
                for (auto row = column + 1U; row < active_count; ++row) {
                    if (std::abs(system[row][column]) >
                        std::abs(system[pivot_row][column])) {
                        pivot_row = row;
                    }
                }
                if (std::abs(system[pivot_row][column]) <=
                    cReactionPivotTolerance) {
                    return false;
                }
                if (pivot_row != column) {
                    std::swap(system[pivot_row], system[column]);
                }
                const auto inverse_pivot = 1.0 / system[column][column];
                for (auto entry = column; entry <= active_count; ++entry) {
                    system[column][entry] *= inverse_pivot;
                }
                for (auto row = std::size_t{}; row < active_count; ++row) {
                    if (row == column) {
                        continue;
                    }
                    const auto factor = system[row][column];
                    for (auto entry = column; entry <= active_count; ++entry) {
                        system[row][entry] -= factor * system[column][entry];
                    }
                }
            }

            auto candidate = TVec3f{};
            for (auto index = std::size_t{}; index < active_count; ++index) {
                const auto lambda = system[index][active_count];
                if (!std::isfinite(lambda) ||
                    lambda < -static_cast<double>(cReactionConstraintTolerance)) {
                    return false;
                }
                candidate += contacts[active_indices[index]].normal *
                             static_cast<float>(std::max(0.0, lambda));
            }
            if (!std::isfinite(candidate.x) || !std::isfinite(candidate.y) ||
                !std::isfinite(candidate.z)) {
                return false;
            }
            for (const auto& contact : contacts) {
                if (dot(contact.normal, candidate) <
                    contact.penetration - cReactionConstraintTolerance) {
                    return false;
                }
            }
            reaction->set(candidate);
            return true;
        }

        [[nodiscard]] TVec3f minimum_norm_reaction(
            const std::vector<StageCollisionContact>& contacts) {
            auto best = TVec3f{};
            auto best_square_length = std::numeric_limits<float>::infinity();
            auto found = std::ranges::all_of(contacts, [](const auto& contact) {
                return contact.penetration <= cReactionConstraintTolerance;
            });
            if (found) {
                best_square_length = 0.0F;
            }

            const auto consider = [&](const std::array<std::size_t, 3U>& indices,
                                      std::size_t count) {
                auto candidate = TVec3f{};
                if (!solve_active_reaction(contacts, indices, count, &candidate)) {
                    return;
                }
                const auto square_length = length_squared(candidate);
                // Contact order is stable source-prism order. Keeping the
                // first candidate within the tie tolerance makes active-set
                // selection deterministic as well.
                if (!found ||
                    square_length < best_square_length - cReactionTieTolerance) {
                    best = candidate;
                    best_square_length = square_length;
                    found = true;
                }
            };

            for (auto first = std::size_t{}; first < contacts.size(); ++first) {
                consider({first, 0U, 0U}, 1U);
            }
            for (auto first = std::size_t{}; first < contacts.size(); ++first) {
                for (auto second = first + 1U; second < contacts.size(); ++second) {
                    consider({first, second, 0U}, 2U);
                }
            }
            for (auto first = std::size_t{}; first < contacts.size(); ++first) {
                for (auto second = first + 1U; second < contacts.size(); ++second) {
                    for (auto third = second + 1U; third < contacts.size(); ++third) {
                        consider({first, second, third}, 3U);
                    }
                }
            }

            // Opposing or fully degenerate constraints can have no feasible
            // half-space intersection. Preserve Binder's retail component
            // extrema in that exceptional case rather than inventing motion.
            return found ? best : component_extrema_reaction(contacts);
        }

        [[nodiscard]] TVec3f transform_point(const std::array<float, 12U>& matrix, const TVec3f& point) {
            return TVec3f(matrix[0] * point.x + matrix[1] * point.y + matrix[2] * point.z + matrix[3],
                          matrix[4] * point.x + matrix[5] * point.y + matrix[6] * point.z + matrix[7],
                          matrix[8] * point.x + matrix[9] * point.y + matrix[10] * point.z + matrix[11]);
        }

        [[nodiscard]] TVec3f transform_vector(const std::array<float, 12U>& matrix, const TVec3f& vector) {
            return TVec3f(matrix[0] * vector.x + matrix[1] * vector.y + matrix[2] * vector.z,
                          matrix[4] * vector.x + matrix[5] * vector.y + matrix[6] * vector.z,
                          matrix[8] * vector.x + matrix[9] * vector.y + matrix[10] * vector.z);
        }

        template <typename Bounds>
        [[nodiscard]] Bounds empty_bounds() {
            const auto infinity = std::numeric_limits<float>::infinity();
            return Bounds{TVec3f(infinity, infinity, infinity), TVec3f(-infinity, -infinity, -infinity)};
        }

        template <typename Bounds>
        void include(Bounds& bounds, const TVec3f& point) {
            bounds.minimum.x = std::min(bounds.minimum.x, point.x);
            bounds.minimum.y = std::min(bounds.minimum.y, point.y);
            bounds.minimum.z = std::min(bounds.minimum.z, point.z);
            bounds.maximum.x = std::max(bounds.maximum.x, point.x);
            bounds.maximum.y = std::max(bounds.maximum.y, point.y);
            bounds.maximum.z = std::max(bounds.maximum.z, point.z);
        }

        template <typename Bounds>
        void include(Bounds& bounds, const Bounds& other) {
            include(bounds, other.minimum);
            include(bounds, other.maximum);
        }

        template <typename Bounds>
        [[nodiscard]] bool overlaps_sphere(const Bounds& bounds, const TVec3f& center, float radius) {
            auto distance_squared = 0.0F;
            const float values[3]{center.x, center.y, center.z};
            const float minimum[3]{bounds.minimum.x, bounds.minimum.y, bounds.minimum.z};
            const float maximum[3]{bounds.maximum.x, bounds.maximum.y, bounds.maximum.z};
            for (auto axis = 0U; axis < 3U; ++axis) {
                if (values[axis] < minimum[axis]) {
                    const auto distance = minimum[axis] - values[axis];
                    distance_squared += distance * distance;
                } else if (values[axis] > maximum[axis]) {
                    const auto distance = values[axis] - maximum[axis];
                    distance_squared += distance * distance;
                }
            }
            return distance_squared <= radius * radius;
        }

        template <typename Bounds>
        [[nodiscard]] bool intersects_segment(const Bounds& bounds, const TVec3f& start, const TVec3f& offset,
                                              float maximum_fraction) {
            auto near_fraction = 0.0F;
            auto far_fraction = maximum_fraction;
            const float starts[3]{start.x, start.y, start.z};
            const float directions[3]{offset.x, offset.y, offset.z};
            const float minimum[3]{bounds.minimum.x, bounds.minimum.y, bounds.minimum.z};
            const float maximum[3]{bounds.maximum.x, bounds.maximum.y, bounds.maximum.z};
            for (auto axis = 0U; axis < 3U; ++axis) {
                if (std::abs(directions[axis]) < 1.0e-8F) {
                    if (starts[axis] < minimum[axis] || starts[axis] > maximum[axis]) {
                        return false;
                    }
                    continue;
                }
                const auto inverse = 1.0F / directions[axis];
                auto first = (minimum[axis] - starts[axis]) * inverse;
                auto second = (maximum[axis] - starts[axis]) * inverse;
                if (first > second) {
                    std::swap(first, second);
                }
                near_fraction = std::max(near_fraction, first);
                far_fraction = std::min(far_fraction, second);
                if (near_fraction > far_fraction) {
                    return false;
                }
            }
            return true;
        }

        [[nodiscard]] std::optional<float> segment_triangle_fraction(const TVec3f& start, const TVec3f& offset,
                                                                     const TVec3f& a, const TVec3f& b,
                                                                     const TVec3f& c,
                                                                     const std::array<float, 3U>& edge_tolerances) {
            const auto edge1 = b - a;
            const auto edge2 = c - a;
            const auto perpendicular = cross(offset, edge2);
            const auto determinant = dot(edge1, perpendicular);
            if (std::abs(determinant) < 1.0e-8F) {
                return std::nullopt;
            }
            const auto inverse = 1.0F / determinant;
            const auto from_a = start - a;
            const auto u = dot(from_a, perpendicular) * inverse;
            const auto q = cross(from_a, edge1);
            const auto v = dot(offset, q) * inverse;
            // These coefficients were derived from the source KCL prism.
            // Barycentric coordinates survive the placement transform, so
            // the original 0.01 local-unit edge allowance scales with it.
            if (u < -edge_tolerances[0] || v < -edge_tolerances[1] ||
                u + v > 1.0F + edge_tolerances[2]) {
                return std::nullopt;
            }
            const auto fraction = dot(edge2, q) * inverse;
            if (fraction < 0.0F || fraction > 1.0F) {
                return std::nullopt;
            }
            return fraction;
        }

        [[nodiscard]] bool is_strictly_inside_triangle(const TVec3f& point, const TVec3f& a,
                                                       const TVec3f& b, const TVec3f& c) {
            const auto edge0 = b - a;
            const auto edge1 = c - a;
            const auto relative = point - a;
            const auto d00 = dot(edge0, edge0);
            const auto d01 = dot(edge0, edge1);
            const auto d11 = dot(edge1, edge1);
            const auto d20 = dot(relative, edge0);
            const auto d21 = dot(relative, edge1);
            const auto denominator = d00 * d11 - d01 * d01;
            if (!(std::abs(denominator) > 1.0e-12F)) {
                return false;
            }
            const auto inverse = 1.0F / denominator;
            const auto second = (d11 * d20 - d01 * d21) * inverse;
            const auto third = (d00 * d21 - d01 * d20) * inverse;
            const auto first = 1.0F - second - third;
            return first > 0.0F && second > 0.0F && third > 0.0F;
        }

        [[nodiscard]] TVec3f closest_point_on_triangle(const TVec3f& point, const TVec3f& a, const TVec3f& b,
                                                       const TVec3f& c) {
            const auto ab = b - a;
            const auto ac = c - a;
            const auto ap = point - a;
            const auto d1 = dot(ab, ap);
            const auto d2 = dot(ac, ap);
            if (d1 <= 0.0F && d2 <= 0.0F) {
                return a;
            }

            const auto bp = point - b;
            const auto d3 = dot(ab, bp);
            const auto d4 = dot(ac, bp);
            if (d3 >= 0.0F && d4 <= d3) {
                return b;
            }

            const auto vc = d1 * d4 - d3 * d2;
            if (vc <= 0.0F && d1 >= 0.0F && d3 <= 0.0F) {
                return a + ab * (d1 / (d1 - d3));
            }

            const auto cp = point - c;
            const auto d5 = dot(ab, cp);
            const auto d6 = dot(ac, cp);
            if (d6 >= 0.0F && d5 <= d6) {
                return c;
            }

            const auto vb = d5 * d2 - d1 * d6;
            if (vb <= 0.0F && d2 >= 0.0F && d6 <= 0.0F) {
                return a + ac * (d2 / (d2 - d6));
            }

            const auto va = d3 * d6 - d5 * d4;
            if (va <= 0.0F && d4 - d3 >= 0.0F && d5 - d6 >= 0.0F) {
                return b + (c - b) * ((d4 - d3) / ((d4 - d3) + (d5 - d6)));
            }

            const auto inverse = 1.0F / (va + vb + vc);
            return a + ab * (vb * inverse) + ac * (vc * inverse);
        }

        [[nodiscard]] float axis_value(const TVec3f& value, std::uint32_t axis) {
            return axis == 0U ? value.x : (axis == 1U ? value.y : value.z);
        }

    }  // namespace

    StageCollisionRegistrationState::StageCollisionRegistrationState(const bool *inactive_flag) noexcept
        : _inactive_flag(inactive_flag) {
    }

    void StageCollisionRegistrationState::set_enabled(bool enabled) noexcept {
        _enabled = enabled;
    }

    void StageCollisionRegistrationState::release_owner() noexcept {
        _inactive_flag = nullptr;
        _released = true;
    }

    bool StageCollisionRegistrationState::enabled() const noexcept {
        return !_released && _enabled && (_inactive_flag == nullptr || !*_inactive_flag);
    }

    StageCollisionService::~StageCollisionService() {
        deactivate();
    }

    void StageCollisionService::clear() {
        _triangles.clear();
        _triangle_lookup.clear();
        _triangle_indices.clear();
        _nodes.clear();
        _sources.clear();
        _stats = {};
        ++_revision;
        _built = false;
    }

    bool StageCollisionService::add_kcl(std::span<const std::uint8_t> bytes,
                                        const std::array<float, 12U> &matrix, std::string source_name) {
        return register_kcl(bytes, matrix, std::move(source_name), nullptr).accepted;
    }

    StageCollisionRegistrationResult StageCollisionService::register_kcl(
        std::span<const std::uint8_t> bytes, const std::array<float, 12U> &matrix,
        std::string source_name, std::shared_ptr<StageCollisionRegistrationState> registration,
        std::span<const std::uint8_t> attributes, HitSensor* sensor) {
        if (bytes.size() < 0x38U) {
            return {};
        }

        const auto position_offset = static_cast<std::size_t>(read_be32(bytes, 0x00U));
        const auto normal_offset = static_cast<std::size_t>(read_be32(bytes, 0x04U));
        const auto prism_base_offset = static_cast<std::size_t>(read_be32(bytes, 0x08U));
        const auto prism_offset = prism_base_offset + 0x10U;
        const auto octree_offset = static_cast<std::size_t>(read_be32(bytes, 0x0CU));
        const auto thickness = read_bef32(bytes, 0x10U);
        if (position_offset < 0x38U || position_offset > normal_offset || normal_offset > prism_offset ||
            prism_offset > octree_offset || octree_offset > bytes.size() ||
            (normal_offset - position_offset) % 12U != 0U || (prism_offset - normal_offset) % 12U != 0U ||
            (octree_offset - prism_offset) % 16U != 0U || !std::isfinite(thickness)) {
            return {};
        }

        const auto position_count = (normal_offset - position_offset) / 12U;
        const auto normal_count = (prism_offset - normal_offset) / 12U;
        const auto prism_count = (octree_offset - prism_offset) / 16U;
        if (position_count == 0U || normal_count == 0U || prism_count == 0U) {
            return {};
        }

        const auto source_index = static_cast<std::uint32_t>(_sources.size());
        _sources.push_back(Source{
            .name = std::move(source_name),
            .attributes = std::vector<std::uint8_t>(attributes.begin(), attributes.end()),
            .sensor = sensor,
        });
        const auto triangle_count_before = _triangles.size();
        auto local_bounding_radius_squared = 0.0F;
        for (auto prism_index = std::size_t{}; prism_index < prism_count; ++prism_index) {
            const auto offset = prism_offset + prism_index * 16U;
            const auto height = read_bef32(bytes, offset);
            const auto position_index = read_be16(bytes, offset + 4U);
            const auto face_normal_index = read_be16(bytes, offset + 6U);
            const auto edge_normal_0_index = read_be16(bytes, offset + 8U);
            const auto edge_normal_1_index = read_be16(bytes, offset + 10U);
            const auto edge_normal_2_index = read_be16(bytes, offset + 12U);
            const auto attribute = read_be16(bytes, offset + 14U);
            if (!std::isfinite(height) || position_index >= position_count || face_normal_index >= normal_count ||
                edge_normal_0_index >= normal_count || edge_normal_1_index >= normal_count ||
                edge_normal_2_index >= normal_count) {
                ++_stats.rejected_triangle_count;
                continue;
            }

            const auto position = read_vec3(bytes, position_offset + position_index * 12U);
            const auto face_normal = read_vec3(bytes, normal_offset + face_normal_index * 12U);
            const auto edge_normal_0 = read_vec3(bytes, normal_offset + edge_normal_0_index * 12U);
            const auto edge_normal_1 = read_vec3(bytes, normal_offset + edge_normal_1_index * 12U);
            const auto edge_normal_2 = read_vec3(bytes, normal_offset + edge_normal_2_index * 12U);
            const auto to_vertex_1 = cross(face_normal, edge_normal_1);
            const auto to_vertex_2 = cross(edge_normal_0, face_normal);
            const auto divisor_1 = dot(to_vertex_1, edge_normal_2);
            const auto divisor_2 = dot(to_vertex_2, edge_normal_2);
            if (std::abs(divisor_1) < 1.0e-8F || std::abs(divisor_2) < 1.0e-8F) {
                ++_stats.rejected_triangle_count;
                continue;
            }

            const auto local_vertices = std::array<TVec3f, 3U>{
                position,
                position + to_vertex_1 * (height / divisor_1),
                position + to_vertex_2 * (height / divisor_2),
            };
            const auto local_ab = local_vertices[1] - local_vertices[0];
            const auto local_ac = local_vertices[2] - local_vertices[0];
            const auto local_twice_area = std::sqrt(length_squared(cross(local_ab, local_ac)));
            const auto local_ab_length = std::sqrt(length_squared(local_ab));
            const auto local_ac_length = std::sqrt(length_squared(local_ac));
            const auto local_bc_length = std::sqrt(length_squared(local_vertices[2] - local_vertices[1]));
            if (!(local_twice_area > 1.0e-8F) || !(local_ab_length > 1.0e-8F) ||
                !(local_ac_length > 1.0e-8F) || !(local_bc_length > 1.0e-8F)) {
                ++_stats.rejected_triangle_count;
                continue;
            }
            for (const auto &vertex : local_vertices) {
                local_bounding_radius_squared =
                    std::max(local_bounding_radius_squared, length_squared(vertex));
            }

            auto triangle = Triangle{};
            triangle.vertices[0] = transform_point(matrix, local_vertices[0]);
            triangle.vertices[1] = transform_point(matrix, local_vertices[1]);
            triangle.vertices[2] = transform_point(matrix, local_vertices[2]);
            triangle.normal = cross(triangle.vertices[1] - triangle.vertices[0],
                                    triangle.vertices[2] - triangle.vertices[0]);
            const auto transformed_face_normal = transform_vector(matrix, face_normal);
            auto source_normal = transformed_face_normal;
            if (!normalize(triangle.normal) || !normalize(source_normal)) {
                ++_stats.rejected_triangle_count;
                continue;
            }
            if (dot(triangle.normal, source_normal) < 0.0F) {
                triangle.normal.negate();
            }
            // Triangle::fillData transforms the four KCL normal axes with
            // the matrix's linear part and normalizes each independently.
            // Preserve these source axes separately from the geometric plane
            // normal used by the host affine sphere-query implementation.
            triangle.source_normals = {
                source_normal,
                transform_vector(matrix, edge_normal_0),
                transform_vector(matrix, edge_normal_1),
                transform_vector(matrix, edge_normal_2),
            };
            if (!normalize(triangle.source_normals[1]) ||
                !normalize(triangle.source_normals[2]) ||
                !normalize(triangle.source_normals[3])) {
                ++_stats.rejected_triangle_count;
                continue;
            }
            // Local KCL slab planes are separated by `thickness` along the
            // unit face normal. After an affine transform their perpendicular
            // separation is thickness / |M^-T n|, equivalently the projection
            // of M*n onto the transformed unit plane normal.
            const auto normal_scale = std::abs(dot(transformed_face_normal, triangle.normal));
            if (!(normal_scale > 1.0e-8F)) {
                ++_stats.rejected_triangle_count;
                continue;
            }
            triangle.thickness = std::max(0.0F, thickness) * normal_scale;
            triangle.arrow_edge_tolerances = {
                cArrowEdgeTolerance * local_ac_length / local_twice_area,
                cArrowEdgeTolerance * local_ab_length / local_twice_area,
                cArrowEdgeTolerance * local_bc_length / local_twice_area,
            };
            triangle.bounds = empty_bounds<Bounds>();
            include(triangle.bounds, triangle.vertices[0]);
            include(triangle.bounds, triangle.vertices[1]);
            include(triangle.bounds, triangle.vertices[2]);
            // KCL prisms are one-sided volumes extending behind the face by
            // the file thickness. Include that volume in broad-phase bounds
            // so an initially embedded binder is still reported.
            include(triangle.bounds, triangle.vertices[0] - triangle.normal * triangle.thickness);
            include(triangle.bounds, triangle.vertices[1] - triangle.normal * triangle.thickness);
            include(triangle.bounds, triangle.vertices[2] - triangle.normal * triangle.thickness);
            auto linear_square_sum = 0.0F;
            for (const auto index : std::array{0U, 1U, 2U, 4U, 5U, 6U, 8U, 9U, 10U}) {
                linear_square_sum += matrix[index] * matrix[index];
            }
            const auto edge_padding = cArrowEdgeTolerance * std::sqrt(linear_square_sum);
            triangle.bounds.minimum -= TVec3f(edge_padding, edge_padding, edge_padding);
            triangle.bounds.maximum += TVec3f(edge_padding, edge_padding, edge_padding);
            triangle.centroid = (triangle.vertices[0] + triangle.vertices[1] + triangle.vertices[2]) * (1.0F / 3.0F);
            triangle.attribute = attribute;
            const auto triangle_index = sNextTriangleIndex.fetch_add(1U, std::memory_order_relaxed);
            if (triangle_index >= std::numeric_limits<std::uint32_t>::max()) {
                throw std::overflow_error("Stage collision exhausted stable Triangle identities.");
            }
            triangle.triangle_index = static_cast<std::uint32_t>(triangle_index);
            triangle.source_index = source_index;
            triangle.prism_index = static_cast<std::uint32_t>(prism_index);
            triangle.registration = registration;
            _triangle_lookup.emplace(triangle.triangle_index,
                                     static_cast<std::uint32_t>(_triangles.size()));
            _triangles.push_back(triangle);
        }

        if (_triangles.size() == triangle_count_before) {
            _sources.pop_back();
            return {};
        }
        ++_revision;
        ++_stats.mesh_count;
        _stats.triangle_count = _triangles.size();
        _built = false;
        return StageCollisionRegistrationResult{
            .accepted = true,
            .local_bounding_radius = std::sqrt(local_bounding_radius_squared),
        };
    }

    void StageCollisionService::build() {
        _triangle_indices.resize(_triangles.size());
        for (auto index = std::size_t{}; index < _triangle_indices.size(); ++index) {
            _triangle_indices[index] = static_cast<std::uint32_t>(index);
        }
        _nodes.clear();
        if (!_triangles.empty()) {
            _nodes.reserve(_triangles.size() * 2U);
            (void)build_node(0U, static_cast<std::uint32_t>(_triangle_indices.size()));
        }
        _stats.triangle_count = _triangles.size();
        _built = true;
    }

    std::uint32_t StageCollisionService::build_node(std::uint32_t first, std::uint32_t count) {
        const auto node_index = static_cast<std::uint32_t>(_nodes.size());
        _nodes.push_back(BvhNode{});
        auto bounds = empty_bounds<Bounds>();
        auto centroid_bounds = empty_bounds<Bounds>();
        for (auto offset = std::uint32_t{}; offset < count; ++offset) {
            const auto& triangle = _triangles[_triangle_indices[first + offset]];
            include(bounds, triangle.bounds);
            include(centroid_bounds, triangle.centroid);
        }

        _nodes[node_index].bounds = bounds;
        if (count <= cLeafTriangleCount) {
            _nodes[node_index].first = first;
            _nodes[node_index].count = count;
            return node_index;
        }

        const auto extent = centroid_bounds.maximum - centroid_bounds.minimum;
        auto axis = std::uint32_t{};
        if (extent.y > extent.x && extent.y >= extent.z) {
            axis = 1U;
        } else if (extent.z > extent.x && extent.z > extent.y) {
            axis = 2U;
        }
        const auto middle = first + count / 2U;
        std::nth_element(_triangle_indices.begin() + first, _triangle_indices.begin() + middle,
                         _triangle_indices.begin() + first + count, [&](std::uint32_t lhs, std::uint32_t rhs) {
                             return axis_value(_triangles[lhs].centroid, axis) < axis_value(_triangles[rhs].centroid, axis);
                         });
        const auto left = build_node(first, middle - first);
        const auto right = build_node(middle, first + count - middle);
        _nodes[node_index].left = left;
        _nodes[node_index].right = right;
        return node_index;
    }

    bool StageCollisionService::line_cast(const TVec3f& start, const TVec3f& offset, StageCollisionHit* hit,
                                          const StageCollisionTriangleFilter& filter) const {
        if (!_built || _nodes.empty() || length_squared(offset) <= 1.0e-12F) {
            return false;
        }
        // A line ending exactly on the face is a KCHitArrow hit (t == 1).
        auto best_fraction = std::nextafter(1.0F, std::numeric_limits<float>::infinity());
        auto best_triangle = static_cast<const Triangle*>(nullptr);
        auto stack = std::vector<std::uint32_t>{0U};
        while (!stack.empty()) {
            const auto node_index = stack.back();
            stack.pop_back();
            const auto& node = _nodes[node_index];
            if (!intersects_segment(node.bounds, start, offset, best_fraction)) {
                continue;
            }
            if (node.count != 0U) {
                for (auto leaf_index = std::uint32_t{}; leaf_index < node.count; ++leaf_index) {
                    const auto &triangle = _triangles[_triangle_indices[node.first + leaf_index]];
                    if (triangle.registration != nullptr && !triangle.registration->enabled()) {
                        continue;
                    }
                    // KCHitArrow only accepts a ray beginning on the front
                    // side of a KCL prism. Map queries therefore remain
                    // one-sided even though the host triangle routine is not.
                    if (dot(start - triangle.vertices[0], triangle.normal) <= 0.0F) {
                        continue;
                    }
                    const auto fraction = segment_triangle_fraction(start, offset, triangle.vertices[0],
                                                                    triangle.vertices[1], triangle.vertices[2],
                                                                    triangle.arrow_edge_tolerances);
                    if (fraction.has_value() && *fraction < best_fraction &&
                        (!filter || filter(triangle.triangle_index))) {
                        best_fraction = *fraction;
                        best_triangle = &triangle;
                    }
                }
                continue;
            }
            stack.push_back(node.left);
            stack.push_back(node.right);
        }

        if (best_triangle == nullptr) {
            return false;
        }
        if (hit != nullptr) {
            hit->fraction = best_fraction;
            hit->position = start + offset * best_fraction;
            hit->normal = best_triangle->normal;
            hit->attribute = best_triangle->attribute;
            hit->triangle_index = best_triangle->triangle_index;
        }
        return true;
    }

    std::vector<StageCollisionContact> StageCollisionService::sphere_contacts(const TVec3f& center, float radius,
                                                                              std::size_t maximum,
                                                                              const StageCollisionTriangleFilter& filter) const {
        return sphere_contacts_impl(center, radius, maximum, std::nullopt, 0.0F, filter);
    }

    std::vector<StageCollisionContact> StageCollisionService::sphere_contacts_with_thickness(
        const TVec3f& center, float radius, float thickness, std::size_t maximum,
        const StageCollisionTriangleFilter& filter) const {
        if (thickness < 0.0F || !std::isfinite(thickness)) {
            return {};
        }
        return sphere_contacts_impl(center, radius, maximum, thickness, 0.0F, filter);
    }

    std::vector<StageCollisionContact> StageCollisionService::sphere_contacts_impl(
        const TVec3f& center, float radius, std::size_t maximum,
        std::optional<float> thickness_override, float outer_margin,
        const StageCollisionTriangleFilter& filter) const {
        auto contacts = std::vector<StageCollisionContact>{};
        if (!_built || _nodes.empty() || radius < 0.0F || !std::isfinite(radius) ||
            outer_margin < 0.0F || !std::isfinite(outer_margin) || maximum == 0U) {
            return contacts;
        }
        const auto contact_epsilon =
            outer_margin > 0.0F ? cCollisionContactEpsilon : 0.0F;
        const auto query_radius = radius + outer_margin + contact_epsilon;
        const auto broad_radius = query_radius + thickness_override.value_or(0.0F);
        struct IndexedContact {
            std::uint32_t triangle_index = 0U;
            StageCollisionContact contact{};
        };
        auto indexed_contacts = std::vector<IndexedContact>{};
        indexed_contacts.reserve(std::min(maximum, std::size_t{32U}));
        auto stack = std::vector<std::uint32_t>{0U};
        while (!stack.empty()) {
            const auto node_index = stack.back();
            stack.pop_back();
            const auto& node = _nodes[node_index];
            if (!overlaps_sphere(node.bounds, center, broad_radius)) {
                continue;
            }
            if (node.count == 0U) {
                stack.push_back(node.left);
                stack.push_back(node.right);
                continue;
            }
            for (auto leaf_index = std::uint32_t{}; leaf_index < node.count; ++leaf_index) {
                const auto triangle_index = _triangle_indices[node.first + leaf_index];
                const auto &triangle = _triangles[triangle_index];
                if (triangle.registration != nullptr && !triangle.registration->enabled()) {
                    continue;
                }
                const auto plane_distance = dot(center - triangle.vertices[0], triangle.normal);
                if (plane_distance > query_radius) {
                    continue;
                }
                const auto closest = closest_point_on_triangle(center, triangle.vertices[0], triangle.vertices[1],
                                                               triangle.vertices[2]);
                const auto projected = center - triangle.normal * plane_distance;
                const auto lateral = projected - closest;
                const auto lateral_square = length_squared(lateral);
                const auto is_face_interior = lateral_square <= 1.0e-10F;
                if (radius == 0.0F &&
                    (!is_face_interior ||
                     !is_strictly_inside_triangle(projected, triangle.vertices[0], triangle.vertices[1],
                                                  triangle.vertices[2]))) {
                    continue;
                }
                if (!is_face_interior && !(lateral_square < radius * radius)) {
                    continue;
                }
                // KCHitSphere resolves an edge/corner hit along the prism's
                // face axis: sqrt(r^2 - lateral^2) - face distance. This is
                // deliberately not the radial Euclidean overlap of a generic
                // two-sided triangle.
                const auto axial_reach =
                    is_face_interior
                        ? radius
                        : std::sqrt(std::max(0.0F,
                                             radius * radius - lateral_square));
                const auto penetration = axial_reach + outer_margin - plane_distance;
                const auto maximum_penetration = thickness_override.value_or(triangle.thickness);
                if (penetration < -contact_epsilon ||
                    penetration > maximum_penetration + outer_margin) {
                    continue;
                }
                if (filter && !filter(triangle.triangle_index)) {
                    continue;
                }
                indexed_contacts.push_back(IndexedContact{
                    .triangle_index = triangle_index,
                    .contact = StageCollisionContact{
                        .position = closest,
                        .normal = triangle.normal,
                        .reaction_normal = triangle.normal,
                        .penetration = std::max(0.0F, penetration),
                        .attribute = triangle.attribute,
                        .triangle_index = triangle.triangle_index,
                    },
                });
            }
        }
        // KCollision stores prisms in resource-octree encounter order until
        // Binder's plane array is full. The host BVH does not retain those
        // leaf lists, so use deterministic source-prism order rather than
        // leaking BVH traversal or penetration-depth order into capacity.
        std::ranges::stable_sort(indexed_contacts, [](const auto& lhs, const auto& rhs) {
            return lhs.triangle_index < rhs.triangle_index;
        });
        const auto stored_count = std::min(maximum, indexed_contacts.size());
        contacts.reserve(stored_count);
        for (auto index = std::size_t{}; index < stored_count; ++index) {
            contacts.push_back(indexed_contacts[index].contact);
        }
        return contacts;
    }

    StageCollisionMoveResult StageCollisionService::move_sphere(const TVec3f& center, const TVec3f& movement,
                                                                float radius, std::size_t maximum_contacts,
                                                                bool skip_initial_check,
                                                                const StageCollisionTriangleFilter& filter) const {
        auto result = StageCollisionMoveResult{};
        if (!_built || _nodes.empty() || radius < 0.0F || !std::isfinite(radius) || maximum_contacts == 0U) {
            result.displacement = movement;
            return result;
        }

        struct SweepResult {
            TVec3f center{};
            TVec3f movement{};
            std::vector<StageCollisionContact> contacts{};
            bool can_move_more = false;
        };

        const auto sweep = [&](const TVec3f& start, const TVec3f& requested, bool skip_first_check,
                               std::size_t detection_limit) {
            auto sweep_result = SweepResult{.center = start};
            const auto requested_length = std::sqrt(length_squared(requested));
            const auto step_count = std::max(1, static_cast<int>(requested_length * (1.0F / 35.0F)) + 1);
            const auto step = requested * (1.0F / static_cast<float>(step_count));
            for (auto step_index = 0; step_index <= step_count; ++step_index) {
                if (step_index != 0) {
                    sweep_result.center.add(step);
                    sweep_result.movement.add(step);
                } else if (skip_first_check) {
                    continue;
                }

                sweep_result.contacts = sphere_contacts_impl(
                    sweep_result.center, radius, detection_limit, std::nullopt,
                    cCollisionSkin, filter);
                if (!sweep_result.contacts.empty()) {
                    sweep_result.can_move_more = step_index != step_count;
                    return sweep_result;
                }
            }
            return sweep_result;
        };

        auto first = sweep(center, movement, skip_initial_check, maximum_contacts);
        if (first.contacts.empty()) {
            result.displacement = first.movement;
            return result;
        }

        auto resolved_center = first.center;
        auto first_reaction = minimum_norm_reaction(first.contacts);
        resolved_center.add(first_reaction);
        result.fix_reaction.add(first_reaction);
        result.contacts.insert(result.contacts.end(), first.contacts.begin(), first.contacts.end());

        // Binder performs one projected retry for the unconsumed movement.
        // It removes only the component entering the aggregate reaction plane.
        if (first.can_move_more) {
            auto remaining = movement - first.movement;
            auto reaction_normal = first_reaction;
            if (normalize(reaction_normal)) {
                const auto inward = dot(remaining, reaction_normal);
                if (inward < 0.0F) {
                    remaining -= reaction_normal * inward;
                }
            }
            if (dot(movement, remaining) >= 0.0F) {
                const auto remaining_capacity = maximum_contacts - result.contacts.size();
                // Binder still asks KCollision whether the projected retry
                // hits after its plane array is full. That hit stops motion,
                // but cannot add another plane or contribute a reaction.
                auto second = sweep(resolved_center, remaining, true, std::max(remaining_capacity, std::size_t{1U}));
                resolved_center = second.center;
                if (!second.contacts.empty()) {
                    const auto stored_count = std::min(remaining_capacity, second.contacts.size());
                    if (stored_count != 0U) {
                        second.contacts.resize(stored_count);
                        const auto second_reaction = minimum_norm_reaction(second.contacts);
                        resolved_center.add(second_reaction);
                        result.fix_reaction.add(second_reaction);
                        result.contacts.insert(result.contacts.end(), second.contacts.begin(), second.contacts.end());
                    }
                }
            }
        }
        result.displacement = resolved_center - center;
        return result;
    }

    std::optional<StageCollisionSurface> StageCollisionService::surface(std::uint32_t triangle_index) const {
        const auto lookup = _triangle_lookup.find(triangle_index);
        if (lookup == _triangle_lookup.end() || lookup->second >= _triangles.size()) {
            return std::nullopt;
        }
        const auto& triangle = _triangles[lookup->second];
        if (triangle.registration != nullptr && !triangle.registration->enabled()) {
            return std::nullopt;
        }
        if (triangle.source_index >= _sources.size()) {
            return std::nullopt;
        }
        const auto& source = _sources[triangle.source_index];
        return StageCollisionSurface{
            .triangle_index = triangle.triangle_index,
            .source_index = triangle.source_index,
            .prism_index = triangle.prism_index,
            .attribute = triangle.attribute,
            .attributes = source.attributes,
            .source_name = source.name,
            .sensor = source.sensor,
            .vertices = {triangle.vertices[0], triangle.vertices[1], triangle.vertices[2]},
            .normals = triangle.source_normals,
        };
    }

    std::uint64_t StageCollisionService::revision() const noexcept {
        return _revision;
    }

    const StageCollisionStats& StageCollisionService::stats() const {
        return _stats;
    }

    bool StageCollisionService::empty() const {
        return _triangles.empty();
    }

    void StageCollisionService::activate() {
        sActiveService = this;
    }

    void StageCollisionService::deactivate() {
        if (sActiveService == this) {
            sActiveService = nullptr;
        }
    }

    StageCollisionService* StageCollisionService::active() {
        return sActiveService;
    }

    std::array<float, 12U> stage_collision_matrix(const StagePlacementObject& placement) {
        const auto rx = placement.rotation[0] * (cPi / 180.0F);
        const auto ry = placement.rotation[1] * (cPi / 180.0F);
        const auto rz = placement.rotation[2] * (cPi / 180.0F);
        const auto sx = std::sin(rx);
        const auto cx = std::cos(rx);
        const auto sy = std::sin(ry);
        const auto cy = std::cos(ry);
        const auto sz = std::sin(rz);
        const auto cz = std::cos(rz);
        return std::array<float, 12U>{
            cz * cy * placement.scale[0],
            (cz * sy * sx - sz * cx) * placement.scale[1],
            (cz * sy * cx + sz * sx) * placement.scale[2],
            placement.translation[0],
            sz * cy * placement.scale[0],
            (sz * sy * sx + cz * cx) * placement.scale[1],
            (sz * sy * cx - cz * sx) * placement.scale[2],
            placement.translation[1],
            -sy * placement.scale[0],
            cy * sx * placement.scale[1],
            cy * cx * placement.scale[2],
            placement.translation[2],
        };
    }

}  // namespace smgpc::scene
