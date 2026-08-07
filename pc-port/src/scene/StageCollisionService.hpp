#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include <JSystem/JGeometry/TVec.hpp>

namespace smgpc::scene {
    struct StagePlacementObject;

    struct StageCollisionHit {
        TVec3f position{};
        TVec3f normal{};
        float fraction = 0.0F;
        std::uint16_t attribute = 0U;
    };

    struct StageCollisionContact {
        TVec3f position{};
        TVec3f normal{};
        TVec3f reaction_normal{};
        float penetration = 0.0F;
        std::uint16_t attribute = 0U;
    };

    struct StageCollisionMoveResult {
        TVec3f displacement{};
        std::vector<StageCollisionContact> contacts{};
    };

    struct StageCollisionStats {
        std::size_t mesh_count = 0U;
        std::size_t triangle_count = 0U;
        std::size_t rejected_triangle_count = 0U;
    };

    // Host-side map collision assembled from the original KCL resources. The
    // original game owns equivalent data through CollisionDirector and
    // CollisionParts; keeping this implementation outside Game lets source-
    // close actors continue to call their normal MR map-query boundary.
    class StageCollisionService final {
    public:
        StageCollisionService() = default;
        ~StageCollisionService();

        StageCollisionService(const StageCollisionService&) = delete;
        StageCollisionService& operator=(const StageCollisionService&) = delete;

        void clear();
        // Explicitly registers one decompressed KCL using the exact resource
        // bytes and row-major 3x4 matrix requested by its CollisionParts
        // owner. Stage placement and archive contents are never inspected or
        // guessed here; callers must preserve the request's resource identity
        // in source_name and call build() after completing registrations.
        bool add_kcl(std::span<const std::uint8_t> bytes, const std::array<float, 12U>& matrix,
                     std::string source_name = {});
        void build();

        [[nodiscard]] bool line_cast(const TVec3f& start, const TVec3f& offset,
                                     StageCollisionHit* hit = nullptr) const;
        [[nodiscard]] std::vector<StageCollisionContact> sphere_contacts(const TVec3f& center, float radius,
                                                                         std::size_t maximum = 32U) const;
        [[nodiscard]] StageCollisionMoveResult move_sphere(const TVec3f& center, const TVec3f& movement,
                                                           float radius, std::size_t maximum_contacts = 32U) const;

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
            Bounds bounds{};
            TVec3f centroid{};
            float thickness = 0.0F;
            std::array<float, 3U> arrow_edge_tolerances{};
            std::uint16_t attribute = 0U;
            std::uint32_t source_index = 0U;
        };

        struct BvhNode {
            Bounds bounds{};
            std::uint32_t first = 0U;
            std::uint32_t count = 0U;
            std::uint32_t left = 0U;
            std::uint32_t right = 0U;
        };

        [[nodiscard]] std::uint32_t build_node(std::uint32_t first, std::uint32_t count);

        std::vector<Triangle> _triangles{};
        std::vector<std::uint32_t> _triangle_indices{};
        std::vector<BvhNode> _nodes{};
        std::vector<std::string> _sources{};
        StageCollisionStats _stats{};
        bool _built = false;
    };

    [[nodiscard]] std::array<float, 12U> stage_collision_matrix(const StagePlacementObject& placement);

}  // namespace smgpc::scene
