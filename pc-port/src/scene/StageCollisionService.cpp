#include "scene/StageCollisionService.hpp"

#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cctype>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace smgpc::scene {
    namespace {
        constexpr auto cPi = 3.14159265358979323846F;
        constexpr auto cLeafTriangleCount = std::uint32_t{8U};
        constexpr auto cCollisionSkin = 1.2F;

        StageCollisionService* sActiveService = nullptr;

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

        [[nodiscard]] std::string lower_copy(std::string_view value) {
            auto result = std::string(value);
            std::ranges::transform(result, result.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
            return result;
        }

        [[nodiscard]] std::string_view basename(std::string_view path) {
            const auto slash = path.find_last_of("/\\");
            return slash == std::string_view::npos ? path : path.substr(slash + 1U);
        }

        [[nodiscard]] std::string_view stem(std::string_view path) {
            const auto name = basename(path);
            const auto dot_pos = name.find_last_of('.');
            return dot_pos == std::string_view::npos ? name : name.substr(0U, dot_pos);
        }

        [[nodiscard]] bool ends_with_lower(std::string_view value, std::string_view suffix) {
            if (value.size() < suffix.size()) {
                return false;
            }
            return lower_copy(value.substr(value.size() - suffix.size())) == suffix;
        }

        [[nodiscard]] const smgpc::resource::RarcEntry* select_kcl_entry(
            const smgpc::resource::RarcArchive& archive, const StagePlacementObject& placement) {
            auto candidates = std::vector<const smgpc::resource::RarcEntry*>{};
            for (const auto& entry : archive.entries()) {
                if (ends_with_lower(entry.path, ".kcl") && lower_copy(basename(entry.path)) != "movelimit.kcl") {
                    candidates.push_back(&entry);
                }
            }
            if (candidates.empty()) {
                return nullptr;
            }

            const auto object_stem = lower_copy(placement.object_name);
            const auto model_stem = lower_copy(placement.model_archive_name);
            const auto exact = std::ranges::find_if(candidates, [&](const auto* entry) {
                const auto entry_stem = lower_copy(stem(entry->path));
                return entry_stem == object_stem || (!model_stem.empty() && entry_stem == model_stem);
            });
            if (exact != candidates.end()) {
                return *exact;
            }
            return candidates.size() == 1U ? candidates.front() : nullptr;
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
                                                                     const TVec3f& c) {
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
            if (u < -1.0e-5F || u > 1.00001F) {
                return std::nullopt;
            }
            const auto q = cross(from_a, edge1);
            const auto v = dot(offset, q) * inverse;
            if (v < -1.0e-5F || u + v > 1.00001F) {
                return std::nullopt;
            }
            const auto fraction = dot(edge2, q) * inverse;
            if (fraction < 0.0F || fraction > 1.0F) {
                return std::nullopt;
            }
            return fraction;
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

    StageCollisionService::~StageCollisionService() {
        deactivate();
    }

    void StageCollisionService::clear() {
        _triangles.clear();
        _triangle_indices.clear();
        _nodes.clear();
        _sources.clear();
        _stats = {};
        _built = false;
    }

    StageCollisionLoadStats StageCollisionService::load(smgpc::runtime::DvdFileSystemService& dvd,
                                                        std::span<const StagePlacementObject> placements) {
        clear();
        _stats.placement_count = placements.size();
        for (const auto& placement : placements) {
            if (placement.object_archive_path.empty()) {
                continue;
            }
            try {
                auto& archive = dvd.archive(placement.object_archive_path);
                ++_stats.archive_count;
                const auto* entry = select_kcl_entry(archive, placement);
                if (entry == nullptr) {
                    continue;
                }
                (void)add_kcl(archive.file_data(*entry), stage_collision_matrix(placement),
                              placement.object_name + ":" + entry->path);
            } catch (const std::exception&) {
                // Missing or malformed optional collision does not make the
                // placement itself unconstructable. add_kcl performs strict
                // validation before accepting any triangles.
            }
        }
        build();
        return _stats;
    }

    bool StageCollisionService::add_kcl(std::span<const std::uint8_t> bytes,
                                        const std::array<float, 12U>& matrix, std::string source_name) {
        if (bytes.size() < 0x38U) {
            return false;
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
            return false;
        }

        const auto position_count = (normal_offset - position_offset) / 12U;
        const auto normal_count = (prism_offset - normal_offset) / 12U;
        const auto prism_count = (octree_offset - prism_offset) / 16U;
        if (position_count == 0U || normal_count == 0U || prism_count == 0U) {
            return false;
        }

        const auto source_index = static_cast<std::uint32_t>(_sources.size());
        _sources.push_back(std::move(source_name));
        const auto triangle_count_before = _triangles.size();
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

            auto triangle = Triangle{};
            triangle.vertices[0] = transform_point(matrix, position);
            triangle.vertices[1] = transform_point(matrix, position + to_vertex_1 * (height / divisor_1));
            triangle.vertices[2] = transform_point(matrix, position + to_vertex_2 * (height / divisor_2));
            triangle.normal = cross(triangle.vertices[1] - triangle.vertices[0],
                                    triangle.vertices[2] - triangle.vertices[0]);
            auto source_normal = transform_vector(matrix, face_normal);
            if (!normalize(triangle.normal) || !normalize(source_normal)) {
                ++_stats.rejected_triangle_count;
                continue;
            }
            if (dot(triangle.normal, source_normal) < 0.0F) {
                triangle.normal.negate();
            }
            triangle.bounds = empty_bounds<Bounds>();
            include(triangle.bounds, triangle.vertices[0]);
            include(triangle.bounds, triangle.vertices[1]);
            include(triangle.bounds, triangle.vertices[2]);
            triangle.centroid = (triangle.vertices[0] + triangle.vertices[1] + triangle.vertices[2]) * (1.0F / 3.0F);
            triangle.thickness = std::max(0.0F, thickness);
            triangle.attribute = attribute;
            triangle.source_index = source_index;
            _triangles.push_back(triangle);
        }

        if (_triangles.size() == triangle_count_before) {
            _sources.pop_back();
            return false;
        }
        ++_stats.mesh_count;
        _stats.triangle_count = _triangles.size();
        _built = false;
        return true;
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

    bool StageCollisionService::line_cast(const TVec3f& start, const TVec3f& offset, StageCollisionHit* hit) const {
        if (!_built || _nodes.empty() || length_squared(offset) <= 1.0e-12F) {
            return false;
        }
        auto best_fraction = 1.0F;
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
                    const auto& triangle = _triangles[_triangle_indices[node.first + leaf_index]];
                    const auto fraction = segment_triangle_fraction(start, offset, triangle.vertices[0],
                                                                    triangle.vertices[1], triangle.vertices[2]);
                    if (fraction.has_value() && *fraction < best_fraction) {
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
        }
        return true;
    }

    std::vector<StageCollisionContact> StageCollisionService::sphere_contacts(const TVec3f& center, float radius,
                                                                              std::size_t maximum) const {
        auto contacts = std::vector<StageCollisionContact>{};
        if (!_built || _nodes.empty() || !(radius > 0.0F) || maximum == 0U) {
            return contacts;
        }
        contacts.reserve(std::min(maximum, std::size_t{32U}));
        auto stack = std::vector<std::uint32_t>{0U};
        while (!stack.empty()) {
            const auto node_index = stack.back();
            stack.pop_back();
            const auto& node = _nodes[node_index];
            if (!overlaps_sphere(node.bounds, center, radius)) {
                continue;
            }
            if (node.count == 0U) {
                stack.push_back(node.left);
                stack.push_back(node.right);
                continue;
            }
            for (auto leaf_index = std::uint32_t{}; leaf_index < node.count; ++leaf_index) {
                const auto& triangle = _triangles[_triangle_indices[node.first + leaf_index]];
                const auto plane_distance = dot(center - triangle.vertices[0], triangle.normal);
                if (plane_distance < -triangle.thickness || plane_distance > radius) {
                    continue;
                }
                const auto closest = closest_point_on_triangle(center, triangle.vertices[0], triangle.vertices[1],
                                                               triangle.vertices[2]);
                auto from_surface = center - closest;
                const auto square_distance = length_squared(from_surface);
                if (!(square_distance < radius * radius)) {
                    continue;
                }
                const auto distance = std::sqrt(std::max(0.0F, square_distance));
                if (!normalize(from_surface) || dot(from_surface, triangle.normal) < 0.0F) {
                    from_surface = triangle.normal;
                }
                contacts.push_back(StageCollisionContact{
                    .position = closest,
                    .normal = triangle.normal,
                    .reaction_normal = from_surface,
                    .penetration = radius - distance,
                    .attribute = triangle.attribute,
                });
            }
        }
        std::ranges::sort(contacts, [](const auto& lhs, const auto& rhs) {
            return lhs.penetration > rhs.penetration;
        });
        if (contacts.size() > maximum) {
            contacts.resize(maximum);
        }
        return contacts;
    }

    StageCollisionMoveResult StageCollisionService::move_sphere(const TVec3f& center, const TVec3f& movement,
                                                                float radius) const {
        auto result = StageCollisionMoveResult{};
        if (!_built || _nodes.empty() || !(radius > 0.0F)) {
            result.displacement = movement;
            return result;
        }

        auto resolved_center = center;
        const auto movement_length = std::sqrt(length_squared(movement));
        const auto step_count = std::max(1, static_cast<int>(movement_length * (1.0F / 35.0F)) + 1);
        const auto step = movement * (1.0F / static_cast<float>(step_count));
        for (auto step_index = 0; step_index < step_count; ++step_index) {
            resolved_center.add(step);
            for (auto pass = 0; pass < 4; ++pass) {
                auto contacts = sphere_contacts(resolved_center, radius, 32U);
                if (contacts.empty()) {
                    break;
                }
                auto moved = false;
                for (const auto& contact : contacts) {
                    if (!(contact.penetration > 1.0e-4F)) {
                        continue;
                    }
                    resolved_center.add(contact.reaction_normal * (contact.penetration + cCollisionSkin));
                    result.contacts.push_back(contact);
                    moved = true;
                }
                if (!moved) {
                    break;
                }
            }
        }
        result.displacement = resolved_center - center;
        return result;
    }

    const StageCollisionLoadStats& StageCollisionService::stats() const {
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
