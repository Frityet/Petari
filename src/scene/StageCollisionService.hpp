#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <JSystem/JGeometry/TMatrix.hpp>
#include <JSystem/JGeometry/TVec.hpp>

class HitSensor;
class CollisionParts;
class KCollisionServer;
namespace smgpc::resource {
    class GeneratedKCollisionResource;
}

namespace smgpc::scene {
    // Stable borrowed transforms for the lifetime of one registered KCL source.
    // Static registrations retain the same base and previous matrices.
    struct StageCollisionMatrices {
        TPos3f base;
        TPos3f inverse;
        TPos3f previous;
    };

    struct StageCollisionSurface {
        std::uint32_t triangle_index = 0U;
        std::uint32_t source_index = 0U;
        std::uint32_t prism_index = 0U;
        std::uint16_t attribute = 0U;
        std::span<const std::uint8_t> attributes{};
        std::string_view source_name{};
        HitSensor* sensor = nullptr;
        CollisionParts* parts = nullptr;
        std::optional<std::int32_t> placement_zone_id;
        std::array<TVec3f, 3U> vertices{};
        std::array<TVec3f, 4U> normals{};
    };

    struct StageCollisionStats {
        std::size_t mesh_count = 0U;
        std::size_t triangle_count = 0U;
        std::size_t rejected_triangle_count = 0U;
    };

    // Shared validity for one CollisionParts registration. The inactive flag
    // is the original owning actor's dead/alive state; release_owner() makes
    // retained diagnostic surfaces inert before actor storage is destroyed.
    class StageCollisionRegistrationState final {
    public:
        explicit StageCollisionRegistrationState(const bool *inactive_flag = nullptr,
                                                  CollisionParts* parts = nullptr) noexcept;

        void set_enabled(bool enabled) noexcept;
        void release_owner() noexcept;
        [[nodiscard]] bool enabled() const noexcept;
        [[nodiscard]] CollisionParts* parts() const noexcept;

    private:
        friend class StageCollisionService;
        const bool *_inactive_flag;
        bool _enabled = true;
        bool _released = false;
        CollisionParts* _parts = nullptr;
    };

    struct StageCollisionRegistrationResult {
        bool accepted = false;
        float local_bounding_radius = 0.0F;
    };

    // Host publication and lifetime metadata for original CollisionParts.
    // CollisionDirector, CollisionCategorizedKeeper, CollisionParts and
    // KCollisionServer own all collision queries and their encounter order.
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
        // in source_name. Registration publishes retained diagnostic surfaces.
        // Sensor ownership and placement zone are separate from that resource
        // identity. A geometry-only registration has no authored owner/zone.
        bool add_kcl(std::span<const std::uint8_t> bytes, const std::array<float, 12U> &matrix,
                     std::string source_name = {});
        [[nodiscard]] StageCollisionRegistrationResult register_kcl(
            std::span<const std::uint8_t> bytes, const std::array<float, 12U> &matrix,
            std::string source_name, std::shared_ptr<StageCollisionRegistrationState> registration,
            std::span<const std::uint8_t> attributes = {}, HitSensor* sensor = nullptr,
            std::optional<std::int32_t> placement_zone_id = std::nullopt);
        [[nodiscard]] StageCollisionRegistrationResult register_generated_kcl(
            std::shared_ptr<resource::GeneratedKCollisionResource> resource, KCollisionServer& server,
            const std::array<float, 12U>& matrix, std::string source_name,
            std::shared_ptr<StageCollisionRegistrationState> registration, HitSensor* sensor,
            std::int32_t placement_zone_id);
        // Regenerates cached geometry from the same actual mutable server;
        // registration and prism-to-Triangle identities are stable.
        void update_registered_geometry(const StageCollisionRegistrationState& registration);
        // The caller supplies the collision owner's two committed transforms.
        // Refits all geometry owned by this registration without replacing
        // Triangle identities or borrowed transform objects.
        void update_registered_transform(const StageCollisionRegistrationState& registration,
                                         const std::array<float, 12U>& current,
                                         const std::array<float, 12U>& previous);

        [[nodiscard]] std::optional<StageCollisionSurface> surface(std::uint32_t triangle_index, bool require_enabled = true) const;
        [[nodiscard]] std::optional<StageCollisionSurface> surface(const CollisionParts* parts,
                                                                 std::uint32_t prism_index) const;
        [[nodiscard]] StageCollisionMatrices& matrices_for_triangle(std::uint32_t triangle_index) const;
        // A new lifetime receives a distinct identity even when the allocator
        // reuses an address and the resource revision starts over.
        [[nodiscard]] std::uint64_t generation() const noexcept;
        [[nodiscard]] std::uint64_t revision() const noexcept;

        [[nodiscard]] const StageCollisionStats& stats() const;
        [[nodiscard]] bool empty() const;

        void activate();
        void deactivate();
        [[nodiscard]] static StageCollisionService* active();

        // Native original queries share the generated-resource publication boundary.
        void require_published_geometry() const;

    private:
        struct Triangle {
            TVec3f vertices[3]{};
            std::array<TVec3f, 3U> local_vertices{};
            std::array<TVec3f, 4U> local_normals{};
            std::array<TVec3f, 4U> source_normals{};
            std::uint16_t attribute = 0U;
            std::uint32_t triangle_index = 0U;
            std::uint32_t source_index = 0U;
            std::uint32_t prism_index = 0U;
            bool geometry_enabled = true;
            std::shared_ptr<StageCollisionRegistrationState> registration{};
        };

        struct Source {
            std::string name{};
            std::vector<std::uint8_t> attributes{};
            HitSensor* sensor = nullptr;
            std::optional<std::int32_t> placement_zone_id;
            std::array<float, 12U> matrix{};
            mutable std::unique_ptr<StageCollisionMatrices> matrices{};
            std::shared_ptr<StageCollisionRegistrationState> registration{};
            std::vector<std::uint32_t> prism_triangles{};
            std::shared_ptr<resource::GeneratedKCollisionResource> generated_resource{};
            KCollisionServer* generated_server = nullptr;
            bool generated_geometry_published = true;
        };

        [[nodiscard]] static bool load_native_triangle(Triangle& triangle, const KCollisionServer& server,
                                                       std::uint32_t prism_index,
                                                       const std::array<float, 12U>& matrix);
        [[nodiscard]] static bool transform_triangle_geometry(Triangle& triangle,
                                                              const std::array<float, 12U>& matrix);
        std::vector<Triangle> _triangles{};
        std::unordered_map<std::uint32_t, std::uint32_t> _triangle_lookup{};
        std::vector<Source> _sources{};
        StageCollisionStats _stats{};
        std::size_t _unpublished_generated_sources = 0U;
        const std::uint64_t _generation;
        std::uint64_t _revision = 0U;
    };

}  // namespace smgpc::scene
