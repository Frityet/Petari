#include "Game/Map/CollisionParts.hpp"
#include <aurora/exception.hpp>
#include "scene/StageCollisionService.hpp"

#include "scene/StagePlacementResolver.hpp"
#include "resource/KCollisionResource.hpp"
#include "aurora/allocation.hpp"

#include "Game/LiveActor/Binder.hpp"
#include "Game/Util/MathUtil.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstddef>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>

namespace smgpc::scene {
    // CollisionZone erases by swapping with its last part. Preserve that
    // mutation at the registration boundary so capacity-limited queries see
    // the same ordering even when a part toggles between two queries.
    struct StageCollisionAreaOrder {
        struct Zone {
            std::size_t registered_count = 0U;
            std::vector<std::uint32_t> parts{};
        };
        std::map<std::int32_t, Zone> zones{};
    };

    struct StageCollisionAreaMembership {
        std::weak_ptr<StageCollisionAreaOrder> owner{};
        std::int32_t zone = 0;
        std::uint32_t source = 0U;
        bool joined = false;

        void set_enabled(bool enabled) noexcept {
            if (joined == enabled) {
                return;
            }
            const auto order = owner.lock();
            if (order == nullptr) {
                return;
            }
            const auto found = order->zones.find(zone);
            if (found == order->zones.end()) {
                return;
            }
            auto& parts = found->second.parts;
            if (enabled) {
                // Registration reserves one slot for every possible member,
                // including disabled parts, so callbacks never allocate.
                parts.push_back(source);
            } else {
                const auto part = std::find(parts.begin(), parts.end(), source);
                if (part != parts.end()) {
                    *part = parts.back();
                    parts.pop_back();
                }
            }
            joined = enabled;
        }
    };

    namespace {
        constexpr auto cPi = 3.14159265358979323846F;
        constexpr auto cLeafTriangleCount = std::uint32_t{8U};
        constexpr auto cArrowEdgeTolerance = 0.01F;

        StageCollisionService* sActiveService = nullptr;
        std::atomic<std::uint64_t> sNextTriangleIndex{};
        std::atomic<std::uint64_t> sNextServiceGeneration{1U};

        [[nodiscard]] std::uint64_t next_service_generation() {
            auto generation = sNextServiceGeneration.load(std::memory_order_relaxed);
            while (generation != 0U) {
                const auto next = generation == std::numeric_limits<std::uint64_t>::max() ? 0U : generation + 1U;
                if (sNextServiceGeneration.compare_exchange_weak(
                        generation, next, std::memory_order_relaxed)) {
                    return generation;
                }
            }
            aurora::throw_host_exception<std::overflow_error>("Stage collision service generations are exhausted.");
        }

        [[nodiscard]] std::uint16_t read_be16(std::span<const std::uint8_t> bytes, std::size_t offset) {
            if (offset + 2U > bytes.size()) {
                aurora::throw_host_exception<std::runtime_error>("KCL u16 read is outside resource");
            }
            return static_cast<std::uint16_t>((static_cast<std::uint16_t>(bytes[offset]) << 8U) |
                                              static_cast<std::uint16_t>(bytes[offset + 1U]));
        }

        [[nodiscard]] std::uint32_t read_be32(std::span<const std::uint8_t> bytes, std::size_t offset) {
            if (offset + 4U > bytes.size()) {
                aurora::throw_host_exception<std::runtime_error>("KCL u32 read is outside resource");
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

    StageCollisionRegistrationState::StageCollisionRegistrationState(const bool *inactive_flag, CollisionParts* parts) noexcept
        : _inactive_flag(inactive_flag), _parts(parts) {
    }

    void StageCollisionRegistrationState::set_enabled(bool enabled) noexcept {
        if (_released || _enabled == enabled) {
            return;
        }
        _enabled = enabled;
        for (const auto& weak : _area_memberships) {
            if (const auto membership = weak.lock()) {
                membership->set_enabled(enabled);
            }
        }
    }

    void StageCollisionRegistrationState::release_owner() noexcept {
        set_enabled(false);
        _inactive_flag = nullptr;
        _released = true;
        _parts = nullptr;
    }

    bool StageCollisionRegistrationState::enabled() const noexcept {
        return !_released && _enabled && (_inactive_flag == nullptr || !*_inactive_flag);
    }

    CollisionParts* StageCollisionRegistrationState::parts() const noexcept { return _parts; }

    StageCollisionService::StageCollisionService() : _generation(next_service_generation()) {
        const aurora::allocation::HostAllocationScope host_allocations;
        _area_order = std::make_shared<StageCollisionAreaOrder>();
    }

    StageCollisionService::~StageCollisionService() {
        deactivate();
    }

    void StageCollisionService::clear() {
        const aurora::allocation::HostAllocationScope host_allocations;
        _triangles.clear();
        _triangle_lookup.clear();
        _triangle_indices.clear();
        _nodes.clear();
        _sources.clear();
        _area_order->zones.clear();
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
        std::span<const std::uint8_t> attributes, HitSensor* sensor,
        std::optional<std::int32_t> placement_zone_id) {
        const aurora::allocation::HostAllocationScope host_allocations;
        if (sensor != nullptr && registration == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("Collision sensor ownership requires a retained registration lifetime.");
        }
        if (placement_zone_id.has_value() && *placement_zone_id < 0) {
            aurora::throw_host_exception<std::invalid_argument>("Collision placement provenance requires a non-negative zone ID.");
        }
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
            .name = std::string(source_name),
            .attributes = std::vector<std::uint8_t>(attributes.begin(), attributes.end()),
            .sensor = sensor,
            .placement_zone_id = placement_zone_id,
            .kcl_bytes = std::vector<std::uint8_t>(bytes.begin(), bytes.end()),
            .matrix = matrix,
            .registration = registration,
            .prism_triangles = std::vector<std::uint32_t>(prism_count, std::numeric_limits<std::uint32_t>::max()),
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
            triangle.local_vertices = local_vertices;
            triangle.local_normals = {face_normal, edge_normal_0, edge_normal_1, edge_normal_2};
            triangle.local_thickness = std::max(0.0F, thickness);
            triangle.arrow_edge_tolerances = {
                cArrowEdgeTolerance * local_ac_length / local_twice_area,
                cArrowEdgeTolerance * local_ab_length / local_twice_area,
                cArrowEdgeTolerance * local_bc_length / local_twice_area,
            };
            if (!transform_triangle_geometry(triangle, matrix)) {
                ++_stats.rejected_triangle_count;
                continue;
            }
            triangle.attribute = attribute;
            const auto triangle_index = sNextTriangleIndex.fetch_add(1U, std::memory_order_relaxed);
            if (triangle_index >= std::numeric_limits<std::uint32_t>::max()) {
                aurora::throw_host_exception<std::overflow_error>("Stage collision exhausted stable Triangle identities.");
            }
            triangle.triangle_index = static_cast<std::uint32_t>(triangle_index);
            triangle.source_index = source_index;
            triangle.prism_index = static_cast<std::uint32_t>(prism_index);
            triangle.registration = registration;
            _triangle_lookup.emplace(triangle.triangle_index,
                                     static_cast<std::uint32_t>(_triangles.size()));
            _triangles.push_back(triangle);
            _sources.back().prism_triangles[prism_index] = triangle.triangle_index;
        }

        if (_triangles.size() == triangle_count_before) {
            _sources.pop_back();
            return {};
        }
        auto& source = _sources.back();
        auto& zone = _area_order->zones[source.placement_zone_id.value_or(0)];
        zone.parts.reserve(++zone.registered_count);
        auto membership = std::make_shared<StageCollisionAreaMembership>();
        membership->owner = _area_order;
        membership->zone = source.placement_zone_id.value_or(0);
        membership->source = source_index;
        if (registration != nullptr) {
            auto& observers = registration->_area_memberships;
            std::erase_if(observers, [](const auto& weak) { return weak.expired(); });
            observers.push_back(membership);
        }
        membership->set_enabled(registration == nullptr || (!registration->_released && registration->_enabled));
        source.area_membership = std::move(membership);
        ++_revision;
        ++_stats.mesh_count;
        _stats.triangle_count = _triangles.size();
        _built = false;
        return StageCollisionRegistrationResult{
            .accepted = true,
            .local_bounding_radius = std::sqrt(local_bounding_radius_squared),
        };
    }

    bool StageCollisionService::transform_triangle_geometry(Triangle& triangle,
                                                             const std::array<float, 12U>& matrix) {
        triangle.vertices[0] = transform_point(matrix, triangle.local_vertices[0]);
        triangle.vertices[1] = transform_point(matrix, triangle.local_vertices[1]);
        triangle.vertices[2] = transform_point(matrix, triangle.local_vertices[2]);
        triangle.normal = cross(triangle.vertices[1] - triangle.vertices[0],
                                triangle.vertices[2] - triangle.vertices[0]);
        const auto transformed_face_normal = transform_vector(matrix, triangle.local_normals[0]);
        auto source_normal = transformed_face_normal;
        if (!normalize(triangle.normal) || !normalize(source_normal)) {
            return false;
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
            transform_vector(matrix, triangle.local_normals[1]),
            transform_vector(matrix, triangle.local_normals[2]),
            transform_vector(matrix, triangle.local_normals[3]),
        };
        if (!normalize(triangle.source_normals[1]) ||
            !normalize(triangle.source_normals[2]) ||
            !normalize(triangle.source_normals[3])) {
            return false;
        }
        // Local KCL slab planes are separated by `thickness` along the
        // unit face normal. After an affine transform their perpendicular
        // separation is thickness / |M^-T n|, equivalently the projection
        // of M*n onto the transformed unit plane normal.
        const auto normal_scale = std::abs(dot(transformed_face_normal, triangle.normal));
        if (!(normal_scale > 1.0e-8F)) {
            return false;
        }
        triangle.thickness = triangle.local_thickness * normal_scale;
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
        return true;
    }

    void StageCollisionService::build() {
        const aurora::allocation::HostAllocationScope host_allocations;
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

    void StageCollisionService::update_registered_transform(
        const StageCollisionRegistrationState& registration, const std::array<float, 12U>& current,
        const std::array<float, 12U>& previous) {
        const aurora::allocation::HostAllocationScope host_allocations;
        if (registration._released) {
            aurora::throw_host_exception<std::logic_error>("Collision transform updates require a live registration owner.");
        }
        StageCollisionMatrices matrices;
        std::copy(current.begin(), current.end(), &matrices.base.mMtx[0][0]);
        std::copy(previous.begin(), previous.end(), &matrices.previous.mMtx[0][0]);
        Mtx previous_inverse;
        if (!std::ranges::all_of(current, [](float x) { return std::isfinite(x); }) ||
            !std::ranges::all_of(previous, [](float x) { return std::isfinite(x); }) ||
            PSMTXInverse(matrices.base, matrices.inverse) == 0U ||
            PSMTXInverse(matrices.previous, previous_inverse) == 0U) {
            aurora::throw_host_exception<std::invalid_argument>("Collision transforms require finite invertible current and previous matrices.");
        }

        auto sources = std::vector<std::size_t>{};
        for (auto i = std::size_t{}; i < _sources.size(); ++i) {
            if (_sources[i].registration.get() == &registration) sources.push_back(i);
        }
        if (sources.empty()) {
            aurora::throw_host_exception<std::logic_error>("Collision transform registration does not belong to this live service.");
        }
        // Prepare every replacement before publishing any changed matrix or
        // surface. A rejected transform leaves all previous query state valid.
        auto transformed = std::vector<std::pair<std::size_t, Triangle>>{};
        for (auto i = std::size_t{}; i < _triangles.size(); ++i) {
            if (_triangles[i].registration.get() != &registration) continue;
            auto triangle = _triangles[i];
            if (!transform_triangle_geometry(triangle, current)) {
                aurora::throw_host_exception<std::invalid_argument>("Collision transform produces a degenerate registered prism.");
            }
            transformed.emplace_back(i, std::move(triangle));
        }
        auto new_matrices = std::vector<std::pair<std::size_t, std::unique_ptr<StageCollisionMatrices>>>{};
        for (const auto index : sources) {
            if (_sources[index].matrices == nullptr) {
                new_matrices.emplace_back(index, std::make_unique<StageCollisionMatrices>(matrices));
            }
        }
        for (auto& [index, matrix] : new_matrices) _sources[index].matrices = std::move(matrix);
        for (auto& [index, triangle] : transformed) _triangles[index] = std::move(triangle);
        for (const auto index : sources) {
            auto& source = _sources[index];
            source.matrix = current;
            *source.matrices = matrices;
            if (source.area_server != nullptr) {
                const auto scale = (std::sqrt(current[0] * current[0] + current[4] * current[4] + current[8] * current[8]) +
                                    std::sqrt(current[1] * current[1] + current[5] * current[5] + current[9] * current[9]) +
                                    std::sqrt(current[2] * current[2] + current[6] * current[6] + current[10] * current[10])) / 3.0F;
                source.area_bounding_radius = scale * source.area_server->server().mMaxVertexDistance;
            }
        }
        ++_revision;
        if (_built) build();
    }

    void StageCollisionService::prepare_kcl_source(const Source& source) const {
            if (source.area_server == nullptr) {
                auto owner = std::make_unique<resource::OwnedKCollisionServer>(
                    resource::KCollisionResource(source.kcl_bytes, source.attributes));
                auto& server = owner->server();
                // Original calcFarthestVertexDistance marks parallel prisms
                // inactive before building the part's bounding sphere.
                auto farthest_squared = 0.0F;
                for (auto i = 0; i < server.getTriangleNum(); ++i) {
                    auto* prism = server.getPrismData(static_cast<u32>(i));
                    if (server.isNearParallelNormal(prism)) {
                        prism->mHeight = -std::abs(prism->mHeight);
                    } else {
                        for (auto vertex = 0; vertex < 3; ++vertex) {
                            farthest_squared = std::max(farthest_squared, server.getPos(prism, vertex).squared());
                        }
                    }
                }
                server.mMaxVertexDistance = std::sqrt(farthest_squared);
                const auto& matrix = source.matrix;
                const auto scale = (std::sqrt(matrix[0] * matrix[0] + matrix[4] * matrix[4] + matrix[8] * matrix[8]) +
                                    std::sqrt(matrix[1] * matrix[1] + matrix[5] * matrix[5] + matrix[9] * matrix[9]) +
                                    std::sqrt(matrix[2] * matrix[2] + matrix[6] * matrix[6] + matrix[10] * matrix[10])) / 3.0F;
                source.area_bounding_radius = scale * server.mMaxVertexDistance;
                source.area_server = std::move(owner);
            }
            if (source.registration && source.registration->parts()) {
                source.area_bounding_radius = source.registration->parts()->_D8;
            }
    }

    std::vector<std::uint32_t> StageCollisionService::area_polygons(
        std::span<const TVec3f> points, std::size_t maximum) const {
        const aurora::allocation::HostAllocationScope host_allocations;
        auto result = std::vector<std::uint32_t>{};
        if (maximum == 0U || points.empty()) {
            return result;
        }
        // Retail CollisionParts has 32 point and 512 prism stack slots.
        // Reject calls outside that contract instead of overrunning them.
        if (points.size() > 32U || maximum > 512U) {
            aurora::throw_host_exception<std::invalid_argument>("Area polygon queries exceed the original point/prism capacity.");
        }
        for (const auto& point : points) {
            if (!std::isfinite(point.x) || !std::isfinite(point.y) || !std::isfinite(point.z)) {
                aurora::throw_host_exception<std::invalid_argument>("Area polygon queries require finite points.");
            }
        }

        TVec3f world_min, world_max;
        MR::createBoundingBox(points.data(), static_cast<u32>(points.size()), &world_min, &world_max);
        const auto overlaps = [](const TVec3f& minimum, const TVec3f& maximum,
                                 const TVec3f& center, float radius) {
            // Original isSphereOverlappingWithBox tests the expanded axes,
            // including corner overlap; it does not measure sphere distance.
            return !(center.x < minimum.x - radius || maximum.x + radius < center.x ||
                     center.y < minimum.y - radius || maximum.y + radius < center.y ||
                     center.z < minimum.z - radius || maximum.z + radius < center.z);
        };
        const auto center_of = [](const Source& source) {
            return TVec3f(source.matrix[3], source.matrix[7], source.matrix[11]);
        };
        const auto zone_of = [](const Source& source) {
            // Unauthored geometry belongs to the global query bucket; this
            // never manufactures placement provenance for its surfaces.
            return source.placement_zone_id.value_or(0);
        };

        auto sources = std::vector<const Source*>{};
        sources.reserve(_sources.size());
        for (const auto& [zone, membership] : _area_order->zones) {
          for (const auto source_index : membership.parts) {
            const auto& source = _sources[source_index];
            if (source.registration != nullptr && !source.registration->enabled()) {
                continue;
            }
            prepare_kcl_source(source);
            sources.push_back(&source);
          }
        }
        result.reserve(std::min(maximum, _triangles.size()));

        for (auto first = std::size_t{}; first < sources.size();) {
            auto last = first + 1U;
            const auto zone = zone_of(*sources[first]);
            while (last < sources.size() && zone_of(*sources[last]) == zone) {
                ++last;
            }
            if (zone != 0) {
                // CollisionZone seeds its bounds from the first part, takes
                // their midpoint, then encloses every part sphere.
                const auto first_center = center_of(*sources[first]);
                const auto first_radius = sources[first]->area_bounding_radius;
                auto minimum = first_center - TVec3f(first_radius, first_radius, first_radius);
                auto maximum = first_center + TVec3f(first_radius, first_radius, first_radius);
                for (auto i = first; i < last; ++i) {
                    const auto center = center_of(*sources[i]);
                    const auto radius = sources[i]->area_bounding_radius;
                    minimum.x = std::min(minimum.x, center.x - radius);
                    minimum.y = std::min(minimum.y, center.y - radius);
                    minimum.z = std::min(minimum.z, center.z - radius);
                    maximum.x = std::max(maximum.x, center.x + radius);
                    maximum.y = std::max(maximum.y, center.y + radius);
                    maximum.z = std::max(maximum.z, center.z + radius);
                }
                const auto center = (minimum + maximum) * 0.5F;
                auto radius = 0.0F;
                for (auto i = first; i < last; ++i) {
                    radius = std::max(radius, std::sqrt((center_of(*sources[i]) - center).squared()) +
                                               sources[i]->area_bounding_radius);
                }
                if (!overlaps(world_min, world_max, center, radius)) {
                    first = last;
                    continue;
                }
            }
            for (auto i = first; i < last; ++i) {
                const auto& source = *sources[i];
                if (!overlaps(world_min, world_max, center_of(source), source.area_bounding_radius)) {
                    continue;
                }
                Mtx matrix, inverse;
                std::copy(source.matrix.begin(), source.matrix.end(), &matrix[0][0]);
                if (PSMTXInverse(matrix, inverse) == 0U) {
                    aurora::throw_host_exception<std::logic_error>("Area polygon queries require an invertible collision part matrix.");
                }
                auto local_points = std::array<TVec3f, 32U>{};
                for (auto point = std::size_t{}; point < points.size(); ++point) {
                    PSMTXMultVec(inverse, reinterpret_cast<const Vec*>(&points[point]),
                                reinterpret_cast<Vec*>(&local_points[point]));
                }
                TVec3f local_min, local_max;
                MR::createBoundingBox(local_points.data(), static_cast<u32>(points.size()), &local_min, &local_max);
                auto prisms = std::array<KC_PrismData*, 512U>{};
                auto& server = source.area_server->server();
                const auto found = server.checkArea3D(reinterpret_cast<Fxyz*>(&local_min),
                                                      reinterpret_cast<Fxyz*>(&local_max), prisms.data(),
                                                      static_cast<u32>(maximum - result.size()));
                for (auto prism = 0U; prism < found; ++prism) {
                    const auto local_index = server.toIndex(prisms[prism]);
                    const auto identity = source.prism_triangles.at(static_cast<std::size_t>(local_index));
                    if (identity == std::numeric_limits<std::uint32_t>::max()) {
                        aurora::throw_host_exception<std::logic_error>("KCL area query selected a prism without a registered native surface.");
                    }
                    result.push_back(identity);
                }
                if (result.size() == maximum) {
                    return result;
                }
            }
            first = last;
        }
        return result;
    }

    std::vector<StageCollisionHit> StageCollisionService::line_hits(
        const TVec3f& start, const TVec3f& offset, std::size_t maximum,
        const StageCollisionTriangleFilter& filter) const {
        const aurora::allocation::HostAllocationScope host_allocations;
        auto result = std::vector<StageCollisionHit>{};
        if (maximum > 32U) {
            aurora::throw_host_exception<std::invalid_argument>("All-hit line queries exceed the original 32-hit capacity.");
        }
        for (const auto* vector : {&start, &offset}) {
            if (!std::isfinite(vector->x) || !std::isfinite(vector->y) || !std::isfinite(vector->z)) {
                aurora::throw_host_exception<std::invalid_argument>("All-hit line queries require finite vectors.");
            }
        }
        if (maximum == 0U || (offset.x == 0.0F && offset.y == 0.0F && offset.z == 0.0F)) {
            return result;
        }
        const TVec3f end = start + offset;
        const TVec3f minimum(std::min(start.x, end.x), std::min(start.y, end.y), std::min(start.z, end.z));
        const TVec3f maximum_point(std::max(start.x, end.x), std::max(start.y, end.y), std::max(start.z, end.z));
        const auto overlaps = [&](const TVec3f& center, float radius) {
            if (center.x < minimum.x - radius || maximum_point.x + radius < center.x ||
                center.y < minimum.y - radius || maximum_point.y + radius < center.y ||
                center.z < minimum.z - radius || maximum_point.z + radius < center.z) {
                return false;
            }
            return MR::checkHitSegmentSphere(center, start, end, radius, nullptr);
        };
        const auto center_of = [](const Source& source) {
            return TVec3f(source.matrix[3], source.matrix[7], source.matrix[11]);
        };
        result.reserve(maximum);
        for (const auto& [zone_id, zone] : _area_order->zones) {
            auto sources = std::vector<const Source*>{};
            sources.reserve(zone.parts.size());
            for (const auto source_index : zone.parts) {
                const auto& source = _sources[source_index];
                if (source.registration != nullptr && !source.registration->enabled()) {
                    continue;
                }
                prepare_kcl_source(source);
                sources.push_back(&source);
            }
            if (sources.empty()) {
                continue;
            }
            if (zone_id != 0) {
                const auto first_center = center_of(*sources.front());
                const auto first_radius = sources.front()->area_bounding_radius;
                auto zone_min = first_center - TVec3f(first_radius, first_radius, first_radius);
                auto zone_max = first_center + TVec3f(first_radius, first_radius, first_radius);
                for (const auto* source : sources) {
                    const auto center = center_of(*source);
                    const auto radius = source->area_bounding_radius;
                    zone_min.x = std::min(zone_min.x, center.x - radius);
                    zone_min.y = std::min(zone_min.y, center.y - radius);
                    zone_min.z = std::min(zone_min.z, center.z - radius);
                    zone_max.x = std::max(zone_max.x, center.x + radius);
                    zone_max.y = std::max(zone_max.y, center.y + radius);
                    zone_max.z = std::max(zone_max.z, center.z + radius);
                }
                const auto center = (zone_min + zone_max) * 0.5F;
                auto radius = 0.0F;
                for (const auto* source : sources) {
                    radius = std::max(radius, std::sqrt((center_of(*source) - center).squared()) + source->area_bounding_radius);
                }
                if (!overlaps(center, radius)) {
                    continue;
                }
            }
            for (const auto* source : sources) {
                if (!overlaps(center_of(*source), source->area_bounding_radius)) {
                    continue;
                }
                Mtx matrix, inverse;
                std::copy(source->matrix.begin(), source->matrix.end(), &matrix[0][0]);
                if (PSMTXInverse(matrix, inverse) == 0U) {
                    aurora::throw_host_exception<std::logic_error>("All-hit line queries require an invertible collision part matrix.");
                }
                TVec3f local_start, local_end;
                PSMTXMultVec(inverse, &start, &local_start);
                PSMTXMultVec(inverse, &end, &local_end);
                const TVec3f local_offset = local_end - local_start;
                auto fractions = std::array<float, 32U>{};
                auto flags = std::array<u8, 32U>{};
                auto prisms = std::array<KC_PrismData*, 32U>{};
                auto count = u32{};
                auto& server = source->area_server->server();
                server.checkArrow(local_start, local_offset, fractions.data(), flags.data(), &count,
                                  prisms.data(), static_cast<u32>(maximum - result.size()));
                for (auto i = 0U; i < count; ++i) {
                    const auto prism_index = server.toIndex(prisms[i]);
                    const auto identity = source->prism_triangles.at(static_cast<std::size_t>(prism_index));
                    const auto surface_info = surface(identity);
                    if (!surface_info.has_value()) {
                        aurora::throw_host_exception<std::logic_error>("KCL line query selected a prism without a registered native surface.");
                    }
                    if (filter) {
                        // CollisionParts applies its original predicate after
                        // the per-part KCL capacity limit. Game callbacks must
                        // regain their selected original allocation domain.
                        const aurora::allocation::ClientAllocationScope client_allocations;
                        if (!filter(identity)) {
                            continue;
                        }
                    }
                    TVec3f position = local_offset;
                    position.scale(fractions[i]);
                    position += local_start;
                    PSMTXMultVec(matrix, &position, &position);
                    result.push_back(StageCollisionHit{position, surface_info->normals[0], fractions[i],
                                                       surface_info->attribute, identity});
                }
                if (result.size() == maximum) {
                    return result;
                }
            }
        }
        return result;
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
        const aurora::allocation::HostAllocationScope host_allocations;
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
        return sphere_contacts_impl(center, radius, maximum, std::nullopt, filter);
    }

    std::vector<StageCollisionContact> StageCollisionService::sphere_contacts_with_thickness(
        const TVec3f& center, float radius, float thickness, std::size_t maximum,
        const StageCollisionTriangleFilter& filter) const {
        if (thickness < 0.0F || !std::isfinite(thickness)) {
            return {};
        }
        return sphere_contacts_impl(center, radius, maximum, thickness, filter);
    }

    std::vector<StageCollisionContact> StageCollisionService::sphere_contacts_impl(
        const TVec3f& center, float radius, std::size_t maximum,
        std::optional<float> thickness_override,
        const StageCollisionTriangleFilter& filter) const {
        auto contacts = std::vector<StageCollisionContact>{};
        if (!_built || _nodes.empty() || radius < 0.0F || !std::isfinite(radius) ||
            maximum == 0U) {
            return contacts;
        }
        const auto query_radius = radius;
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
                const auto penetration = axial_reach - plane_distance;
                const auto maximum_penetration = thickness_override.value_or(triangle.thickness);
                if (penetration < 0.0F ||
                    penetration > maximum_penetration) {
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
                        .moving_reaction = TVec3f{},
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
        if (maximum_contacts > std::numeric_limits<u32>::max()) {
            aurora::throw_host_exception<std::invalid_argument>("A Binder plane capacity must fit its original u32 count.");
        }

        // Geometry probes use a real Binder as well. This service owns only
        // prism queries; response, margin, stepping and retries come from Game.
        struct ServiceScope {
            StageCollisionService* previous = sActiveService;
            explicit ServiceScope(const StageCollisionService* service) {
                sActiveService = const_cast<StageCollisionService*>(service);
            }
            ~ServiceScope() {
                sActiveService = previous;
            }
        } scope(this);
        struct QueryFilter final : TriangleFilterBase {
            const StageCollisionTriangleFilter& filter;
            const StageCollisionService& service;
            QueryFilter(const StageCollisionTriangleFilter& value, const StageCollisionService& owner) : filter(value), service(owner) {}
            bool isInvalidTriangle(const ::Triangle* triangle) const override {
                const auto source = triangle->mParts ? service.surface(triangle->mParts, triangle->mIdx) : service.surface(triangle->mIdx);
                return !source || !filter(source->triangle_index);
            }
        } query_filter(filter, *this);

        // Gravity affects only ground/wall/roof classification, which these
        // geometry probes do not consume. Game actors retain their own gravity.
        const auto gravity = TVec3f{0.0F, -1.0F, 0.0F};
        auto binder = Binder(nullptr, &center, &gravity, radius, 0.0F, static_cast<u32>(maximum_contacts));
        binder._1EC._3 = skip_initial_check;
        if (filter) {
            binder.setTriangleFilter(&query_filter);
        }
        result.displacement = binder.bind(movement);
        result.fix_reaction = binder.mFixReactionVector;
        result.contacts.reserve(binder.mPlaneNum);
        for (auto index = u32{}; index < binder.mPlaneNum; ++index) {
            const auto& info = *binder.getPlane(static_cast<int>(index));
            const auto& triangle = info.mParentTriangle;
            const auto source = triangle.mParts ? surface(triangle.mParts, triangle.mIdx) : surface(triangle.mIdx);
            if (!source.has_value()) {
                aurora::throw_host_exception<std::logic_error>("A Binder contact must retain its live source prism.");
            }
            result.contacts.push_back(StageCollisionContact{
                .position = info.mHitPos,
                .normal = *info.mParentTriangle.getNormal(0),
                .moving_reaction = info._7C,
                .penetration = info._60,
                .attribute = source->attribute,
                .triangle_index = source->triangle_index,
            });
        }
        return result;
    }

    std::optional<StageCollisionSurface> StageCollisionService::surface(std::uint32_t triangle_index, bool require_enabled) const {
        const auto lookup = _triangle_lookup.find(triangle_index);
        if (lookup == _triangle_lookup.end() || lookup->second >= _triangles.size()) {
            return std::nullopt;
        }
        const auto& triangle = _triangles[lookup->second];
        if (require_enabled && triangle.registration != nullptr && !triangle.registration->enabled()) {
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
            .parts = source.registration ? source.registration->parts() : nullptr,
            .placement_zone_id = source.placement_zone_id,
            .vertices = {triangle.vertices[0], triangle.vertices[1], triangle.vertices[2]},
            .normals = triangle.source_normals,
        };
    }

    std::optional<StageCollisionSurface> StageCollisionService::surface(const CollisionParts* parts,
                                                                       std::uint32_t prism_index) const {
        if (parts == nullptr) return std::nullopt;
        for (const auto& source : _sources) {
            if (source.registration && source.registration->parts() == parts &&
                prism_index < source.prism_triangles.size()) {
                return surface(source.prism_triangles[prism_index], false);
            }
        }
        return std::nullopt;
    }

    StageCollisionMatrices& StageCollisionService::matrices_for_triangle(std::uint32_t triangle_index) const {
        const aurora::allocation::HostAllocationScope host_allocations;
        const auto hit = surface(triangle_index);
        if (!hit) {
            aurora::throw_host_exception<std::logic_error>("Collision transforms require a live source triangle.");
        }
        const auto& source = _sources[hit->source_index];
        if (!source.matrices) {
            auto matrices = std::make_unique<StageCollisionMatrices>();
            for (std::size_t row = 0; row < 3; ++row) {
                for (std::size_t column = 0; column < 4; ++column) {
                    const auto value = source.matrix[row * 4 + column];
                    if (!std::isfinite(value)) {
                        aurora::throw_host_exception<std::logic_error>("Collision transforms require a finite source matrix.");
                    }
                    matrices->base.mMtx[row][column] = value;
                }
            }
            // Original CollisionParts::resetAllMtxPrivate preserves the full
            // affine matrix and uses the SDK inverse, including authored scale.
            matrices->previous.set(matrices->base);
            if (PSMTXInverse(matrices->base.toMtxPtr(), matrices->inverse.toMtxPtr()) == 0) {
                aurora::throw_host_exception<std::logic_error>("Collision transforms require an invertible source matrix.");
            }
            source.matrices = std::move(matrices);
        }
        return *source.matrices;
    }

    std::uint64_t StageCollisionService::generation() const noexcept {
        return _generation;
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
