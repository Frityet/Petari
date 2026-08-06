#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include <JSystem/JGeometry/TVec.hpp>

namespace smgpc::scene {
    struct StagePlacementObject;

    struct StageGravityLoadStats {
        std::size_t placement_count = 0U;
        std::size_t gravity_count = 0U;
        std::size_t unsupported_count = 0U;
    };

    // Data-driven host counterpart of PlanetGravityManager for static global
    // gravity placements. The supported point and parallel variants cover the
    // same SRT, range, priority, inverse, and vector-combination rules as the
    // original implementations.
    class StageGravityService final {
    public:
        StageGravityService() = default;
        ~StageGravityService();

        StageGravityService(const StageGravityService&) = delete;
        StageGravityService& operator=(const StageGravityService&) = delete;

        StageGravityLoadStats load(std::span<const StagePlacementObject> placements);
        void clear();

        [[nodiscard]] bool query(const TVec3f& position, TVec3f* gravity,
                                 std::uint32_t gravity_type_mask = 1U) const;
        [[nodiscard]] const StageGravityLoadStats& stats() const;
        [[nodiscard]] bool empty() const;

        void activate();
        void deactivate();
        [[nodiscard]] static StageGravityService* active();

    private:
        enum class Kind : std::uint8_t {
            Point,
            ParallelSphere,
            ParallelBox,
            ParallelCylinder,
        };

        struct Field {
            Kind kind = Kind::Point;
            TVec3f position{};
            TVec3f side{1.0F, 0.0F, 0.0F};
            TVec3f up{0.0F, 1.0F, 0.0F};
            TVec3f front{0.0F, 0.0F, 1.0F};
            TVec3f extent{};
            float range = -1.0F;
            float distant = 0.0F;
            float base_distance = 2000.0F;
            float cylinder_radius = 500.0F;
            float cylinder_height = 1000.0F;
            std::int32_t distance_axis = -1;
            std::int32_t priority = 0;
            std::uint32_t gravity_type = 1U;
            bool inverse = false;
        };

        [[nodiscard]] bool calculate(const Field& field, const TVec3f& position, TVec3f& vector) const;

        std::vector<Field> _fields{};
        StageGravityLoadStats _stats{};
    };

}  // namespace smgpc::scene
