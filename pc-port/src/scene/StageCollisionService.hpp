#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <JSystem/JGeometry/TVec.hpp>

class HitSensor;

namespace smgpc::scene {
    struct StagePlacementObject;

    struct StageCollisionHit {
        TVec3f position{};
        TVec3f normal{};
        float fraction = 0.0F;
        std::uint16_t attribute = 0U;
        std::uint32_t triangle_index = 0U;
    };

    struct StageCollisionContact {
        TVec3f position{};
        TVec3f normal{};
        TVec3f reaction_normal{};
        float penetration = 0.0F;
        std::uint16_t attribute = 0U;
        std::uint32_t triangle_index = 0U;
    };

    struct StageCollisionSurface {
        std::uint32_t triangle_index = 0U;
        std::uint32_t source_index = 0U;
        std::uint32_t prism_index = 0U;
        std::uint16_t attribute = 0U;
        std::span<const std::uint8_t> attributes{};
        std::string_view source_name{};
        HitSensor* sensor = nullptr;
        std::optional<std::int32_t> placement_zone_id;
        std::array<TVec3f, 3U> vertices{};
        std::array<TVec3f, 4U> normals{};
    };

    // An empty predicate accepts every triangle. Query filters run before
    // hit selection and capacity limits, so rejected prisms cannot obstruct
    // a sweep or hide another accepted contact.
    using StageCollisionTriangleFilter = std::function<bool(std::uint32_t)>;

    struct StageCollisionMoveResult {
        TVec3f displacement{};
        TVec3f fix_reaction{};
        std::vector<StageCollisionContact> contacts{};
    };

    struct StageCollisionStats {
        std::size_t mesh_count = 0U;
        std::size_t triangle_count = 0U;
        std::size_t rejected_triangle_count = 0U;
    };

    // Shared validity for one CollisionParts registration. The inactive flag
    // is the original owning actor's dead/alive state; release_owner() makes
    // cached BVH triangles inert before that actor storage is destroyed.
    class StageCollisionRegistrationState final {
    public:
        explicit StageCollisionRegistrationState(const bool *inactive_flag = nullptr) noexcept;

        void set_enabled(bool enabled) noexcept;
        void release_owner() noexcept;
        [[nodiscard]] bool enabled() const noexcept;

    private:
        const bool *_inactive_flag;
        bool _enabled = true;
        bool _released = false;
    };

    struct StageCollisionRegistrationResult {
        bool accepted = false;
        float local_bounding_radius = 0.0F;
    };

    // Host-side map collision assembled from the original KCL resources. The
    // original game owns equivalent data through CollisionDirector and
    // CollisionParts; keeping this implementation outside Game lets source-
    // close actors continue to call their normal MR map-query boundary.
    class StageCollisionService final {
    public:
        StageCollisionService();
        ~StageCollisionService();

        StageCollisionService(const StageCollisionService&) = delete;
        StageCollisionService& operator=(const StageCollisionService&) = delete;

        void clear();
        // Explicitly registers one decompressed KCL using the exact resource
        // bytes and row-major 3x4 matrix requested by its CollisionParts
        // owner. Stage placement and archive contents are never inspected or
        // guessed here; callers must preserve the request's resource identity
        // in source_name and call build() after completing registrations.
        // Sensor ownership and placement zone are separate from that resource
        // identity. A geometry-only registration has no authored owner/zone.
        bool add_kcl(std::span<const std::uint8_t> bytes, const std::array<float, 12U> &matrix,
                     std::string source_name = {});
        [[nodiscard]] StageCollisionRegistrationResult register_kcl(
            std::span<const std::uint8_t> bytes, const std::array<float, 12U> &matrix,
            std::string source_name, std::shared_ptr<StageCollisionRegistrationState> registration,
            std::span<const std::uint8_t> attributes = {}, HitSensor* sensor = nullptr,
            std::optional<std::int32_t> placement_zone_id = std::nullopt);
        void build();

        [[nodiscard]] bool line_cast(const TVec3f& start, const TVec3f& offset,
                                     StageCollisionHit* hit = nullptr,
                                     const StageCollisionTriangleFilter& filter = {}) const;
        [[nodiscard]] std::vector<StageCollisionContact> sphere_contacts(const TVec3f& center, float radius,
                                                                         std::size_t maximum = 32U,
                                                                         const StageCollisionTriangleFilter& filter = {}) const;
        [[nodiscard]] std::vector<StageCollisionContact> sphere_contacts_with_thickness(
            const TVec3f& center, float radius, float thickness, std::size_t maximum = 32U,
            const StageCollisionTriangleFilter& filter = {}) const;
        [[nodiscard]] StageCollisionMoveResult move_sphere(const TVec3f& center, const TVec3f& movement,
                                                           float radius, std::size_t maximum_contacts = 32U,
                                                           bool skip_initial_check = false,
                                                           const StageCollisionTriangleFilter& filter = {}) const;
        [[nodiscard]] std::optional<StageCollisionSurface> surface(std::uint32_t triangle_index) const;
        // A new lifetime receives a distinct identity even when the allocator
        // reuses an address and the resource revision starts over.
        [[nodiscard]] std::uint64_t generation() const noexcept;
        [[nodiscard]] std::uint64_t revision() const noexcept;

        [[nodiscard]] const StageCollisionStats& stats() const;
        [[nodiscard]] bool empty() const;

        void activate();
        void deactivate();
        [[nodiscard]] static StageCollisionService* active();

    private:
        struct Bounds {
            TVec3f minimum{};
            TVec3f maximum{};
        };

        struct Triangle {
            TVec3f vertices[3]{};
            TVec3f normal{};
            std::array<TVec3f, 4U> source_normals{};
            Bounds bounds{};
            TVec3f centroid{};
            float thickness = 0.0F;
            std::array<float, 3U> arrow_edge_tolerances{};
            std::uint16_t attribute = 0U;
            std::uint32_t triangle_index = 0U;
            std::uint32_t source_index = 0U;
            std::uint32_t prism_index = 0U;
            std::shared_ptr<StageCollisionRegistrationState> registration{};
        };

        struct Source {
            std::string name{};
            std::vector<std::uint8_t> attributes{};
            HitSensor* sensor = nullptr;
            std::optional<std::int32_t> placement_zone_id;
        };

        struct BvhNode {
            Bounds bounds{};
            std::uint32_t first = 0U;
            std::uint32_t count = 0U;
            std::uint32_t left = 0U;
            std::uint32_t right = 0U;
        };

        [[nodiscard]] std::uint32_t build_node(std::uint32_t first, std::uint32_t count);
        [[nodiscard]] std::vector<StageCollisionContact> sphere_contacts_impl(
            const TVec3f& center, float radius, std::size_t maximum,
            std::optional<float> thickness_override, float outer_margin,
            const StageCollisionTriangleFilter& filter) const;

        std::vector<Triangle> _triangles{};
        std::unordered_map<std::uint32_t, std::uint32_t> _triangle_lookup{};
        std::vector<std::uint32_t> _triangle_indices{};
        std::vector<BvhNode> _nodes{};
        std::vector<Source> _sources{};
        StageCollisionStats _stats{};
        const std::uint64_t _generation;
        std::uint64_t _revision = 0U;
        bool _built = false;
    };

    [[nodiscard]] std::array<float, 12U> stage_collision_matrix(const StagePlacementObject& placement);

}  // namespace smgpc::scene
