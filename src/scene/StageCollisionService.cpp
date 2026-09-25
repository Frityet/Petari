#include "Game/Map/CollisionParts.hpp"
#include <aurora/exception.hpp>
#include "scene/StageCollisionService.hpp"

#include "resource/KCollisionResource.hpp"
#include "aurora/allocation.hpp"


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

    }  // namespace

    StageCollisionRegistrationState::StageCollisionRegistrationState(const bool *inactive_flag, CollisionParts* parts) noexcept
        : _inactive_flag(inactive_flag), _parts(parts) {
    }

    void StageCollisionRegistrationState::set_enabled(bool enabled) noexcept {
        if (_released || _enabled == enabled) {
            return;
        }
        _enabled = enabled;
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
    }

    StageCollisionService::~StageCollisionService() {
        deactivate();
    }

    void StageCollisionService::clear() {
        const aurora::allocation::HostAllocationScope host_allocations;
        _triangles.clear();
        _triangle_lookup.clear();
        _sources.clear();
        _unpublished_generated_sources = 0U;
        _stats = {};
        ++_revision;
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
        ++_revision;
        ++_stats.mesh_count;
        _stats.triangle_count = _triangles.size();
        return StageCollisionRegistrationResult{
            .accepted = true,
            .local_bounding_radius = std::sqrt(local_bounding_radius_squared),
        };
    }

    bool StageCollisionService::load_native_triangle(Triangle& triangle, const KCollisionServer& server,
        std::uint32_t prism_index, const std::array<float, 12U>& matrix) {
        const auto* prism = server.getPrismData(prism_index);
        triangle.attribute = prism->mAttribute;
        // Original KCollision skips nonpositive heights. Dynamic writers can
        // collapse a face temporarily; keep its identity for later reactivation
        // without reconstructing undefined vertices or publishing stale geometry.
        triangle.geometry_enabled = prism->mHeight > 0.0F;
        if (!triangle.geometry_enabled) return true;
        for (int i = 0; i < 3; ++i) triangle.local_vertices[i] = server.getPos(prism, i);
        triangle.local_normals = {*server.getFaceNormal(prism), *server.getEdgeNormal1(prism),
                                  *server.getEdgeNormal2(prism), *server.getEdgeNormal3(prism)};
        const auto ab = triangle.local_vertices[1] - triangle.local_vertices[0];
        const auto ac = triangle.local_vertices[2] - triangle.local_vertices[0];
        const auto bc = triangle.local_vertices[2] - triangle.local_vertices[1];
        const auto twice_area = std::sqrt(length_squared(cross(ab, ac)));
        const auto ab_length = std::sqrt(length_squared(ab));
        const auto ac_length = std::sqrt(length_squared(ac));
        const auto bc_length = std::sqrt(length_squared(bc));
        if (!std::isfinite(twice_area) || !(twice_area > 1.0e-8F) ||
            !(ab_length > 1.0e-8F) || !(ac_length > 1.0e-8F) || !(bc_length > 1.0e-8F)) return false;
        return transform_triangle_geometry(triangle, matrix);
    }

    StageCollisionRegistrationResult StageCollisionService::register_generated_kcl(
        std::shared_ptr<resource::GeneratedKCollisionResource> resource, KCollisionServer& server,
        const std::array<float, 12U>& matrix, std::string source_name,
        std::shared_ptr<StageCollisionRegistrationState> registration, HitSensor* sensor,
        std::int32_t placement_zone_id) {
        const aurora::allocation::HostAllocationScope host;
        if (!resource || !registration || !registration->parts() || !sensor || placement_zone_id < 0 ||
            registration->parts()->mServer != &server || registration->parts()->mHitSensor != sensor ||
            server.mFile != resource->native_file()) {
            aurora::throw_host_exception<std::invalid_argument>("Generated collision requires its typed resource, original server and live part registration.");
        }
        resource->validate();
        const auto count = server.getTriangleNum();
        const auto source_index = static_cast<std::uint32_t>(_sources.size());
        std::vector<Triangle> triangles;
        triangles.reserve(count);
        auto radius_squared = 0.0F;
        for (s32 i = 0; i < count; ++i) {
            Triangle triangle;
            if (!load_native_triangle(triangle, server, i, matrix)) {
                aurora::throw_host_exception<std::invalid_argument>("Generated collision contains a degenerate prism.");
            }
            if (triangle.geometry_enabled) {
                for (const auto& vertex : triangle.local_vertices) radius_squared = std::max(radius_squared, vertex.squared());
            }
            const auto identity = sNextTriangleIndex.fetch_add(1U, std::memory_order_relaxed);
            if (identity >= std::numeric_limits<std::uint32_t>::max()) {
                aurora::throw_host_exception<std::overflow_error>("Stage collision exhausted stable Triangle identities.");
            }
            triangle.triangle_index = static_cast<std::uint32_t>(identity);
            triangle.source_index = source_index;
            triangle.prism_index = i;
            triangle.registration = registration;
            triangles.push_back(std::move(triangle));
        }
        Source source;
        source.name = std::move(source_name);
        source.sensor = sensor;
        source.placement_zone_id = placement_zone_id;
        source.matrix = matrix;
        source.registration = registration;
        source.generated_resource = std::move(resource);
        source.generated_server = &server;
        source.prism_triangles.reserve(count);
        for (auto& triangle : triangles) {
            source.prism_triangles.push_back(triangle.triangle_index);
            _triangle_lookup.emplace(triangle.triangle_index, static_cast<std::uint32_t>(_triangles.size()));
            _triangles.push_back(std::move(triangle));
        }
        _sources.push_back(std::move(source));
        ++_revision;
        ++_stats.mesh_count;
        _stats.triangle_count = _triangles.size();
        return {.accepted = true, .local_bounding_radius = std::sqrt(radius_squared)};
    }

    void StageCollisionService::update_registered_geometry(const StageCollisionRegistrationState& registration) {
        const aurora::allocation::HostAllocationScope host;
        if (registration._released) {
            aurora::throw_host_exception<std::logic_error>("Generated collision update requires its live registration.");
        }
        std::vector<std::pair<std::size_t, Triangle>> updates;
        bool found = false;
        for (auto& source : _sources) {
            if (source.registration.get() != &registration) continue;
            if (!source.generated_resource || !source.generated_server) {
                aurora::throw_host_exception<std::logic_error>("Geometry mutation requires a generated collision resource.");
            }
            found = true;
            if (source.generated_geometry_published) {
                source.generated_geometry_published = false;
                ++_unpublished_generated_sources;
            }
        }
        if (!found) aurora::throw_host_exception<std::logic_error>("Generated collision registration is absent from this service.");
        for (const auto& source : _sources) {
            if (source.registration.get() != &registration) continue;
            source.generated_resource->validate();
            if (source.prism_triangles.size() != static_cast<std::size_t>(source.generated_server->getTriangleNum())) {
                aurora::throw_host_exception<std::logic_error>("Generated collision changed its retained prism count.");
            }
            for (const auto identity : source.prism_triangles) {
                const auto index = _triangle_lookup.at(identity);
                auto triangle = _triangles[index];
                if (!load_native_triangle(triangle, *source.generated_server, triangle.prism_index, source.matrix)) {
                    aurora::throw_host_exception<std::invalid_argument>("Generated collision update contains a degenerate prism.");
                }
                updates.emplace_back(index, std::move(triangle));
            }
        }
        for (auto& [index, triangle] : updates) _triangles[index] = std::move(triangle);
        ++_revision;
        for (auto& source : _sources) {
            if (source.registration.get() != &registration) continue;
            source.generated_geometry_published = true;
            --_unpublished_generated_sources;
        }
    }

    void StageCollisionService::require_published_geometry() const {
        if (_unpublished_generated_sources == 0U) return;
        for (const auto& source : _sources) {
            if (!source.generated_geometry_published && source.registration->enabled()) {
                aurora::throw_host_exception<std::logic_error>(
                    "Generated collision queries require a successful geometry publication after mutation.");
            }
        }
    }

    bool StageCollisionService::transform_triangle_geometry(Triangle& triangle,
                                                             const std::array<float, 12U>& matrix) {
        triangle.vertices[0] = transform_point(matrix, triangle.local_vertices[0]);
        triangle.vertices[1] = transform_point(matrix, triangle.local_vertices[1]);
        triangle.vertices[2] = transform_point(matrix, triangle.local_vertices[2]);
        auto normal = cross(triangle.vertices[1] - triangle.vertices[0],
                                triangle.vertices[2] - triangle.vertices[0]);
        const auto transformed_face_normal = transform_vector(matrix, triangle.local_normals[0]);
        auto source_normal = transformed_face_normal;
        if (!normalize(normal) || !normalize(source_normal)) {
            return false;
        }
        if (dot(normal, source_normal) < 0.0F) {
            normal.negate();
        }
        // Triangle::fillData transforms the four KCL normal axes with
        // the matrix's linear part and normalizes each independently.
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
        // Reject a collapsed transform before publishing diagnostic geometry.
        return std::abs(dot(transformed_face_normal, normal)) > 1.0e-8F;
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
        // surface. A rejected transform leaves all previous publication state valid.
        auto transformed = std::vector<std::pair<std::size_t, Triangle>>{};
        for (auto i = std::size_t{}; i < _triangles.size(); ++i) {
            if (_triangles[i].registration.get() != &registration) continue;
            auto triangle = _triangles[i];
            if (!triangle.geometry_enabled) continue;
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
        }
        ++_revision;
    }

    std::optional<StageCollisionSurface> StageCollisionService::surface(std::uint32_t triangle_index, bool require_enabled) const {
        const auto lookup = _triangle_lookup.find(triangle_index);
        if (lookup == _triangle_lookup.end() || lookup->second >= _triangles.size()) {
            return std::nullopt;
        }
        const auto& triangle = _triangles[lookup->second];
        if (!triangle.geometry_enabled) return std::nullopt;
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

}  // namespace smgpc::scene
