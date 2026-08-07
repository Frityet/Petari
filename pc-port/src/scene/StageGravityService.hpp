#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include <JSystem/JGeometry/TVec.hpp>

namespace smgpc::scene {
    struct StagePlacementObject;

}  // namespace smgpc::scene

class GravityInfo;
class PlanetGravity;

namespace smgpc::scene {

    struct StageGravityLoadStats {
        std::size_t placement_count = 0U;
        std::size_t gravity_count = 0U;
        std::size_t unsupported_count = 0U;
    };

    // Scene-owned host counterpart of PlanetGravityManager. Registered
    // PlanetGravity objects retain the original priority, type, host, power,
    // activation, and vector-combination behavior.
    class StageGravityService final {
    public:
        StageGravityService();
        ~StageGravityService();

        StageGravityService(const StageGravityService&) = delete;
        StageGravityService& operator=(const StageGravityService&) = delete;

        StageGravityLoadStats load(std::span<const StagePlacementObject> placements);
        void clear();

        void register_gravity(PlanetGravity* gravity);

        [[nodiscard]] bool query(const TVec3f& position, TVec3f* gravity,
                                 std::uint32_t gravity_type_mask = 1U,
                                 GravityInfo* info = nullptr,
                                 std::uint32_t host = 0U) const;
        [[nodiscard]] const StageGravityLoadStats& stats() const;
        [[nodiscard]] bool empty() const;

        void activate();
        void deactivate();
        [[nodiscard]] static StageGravityService* active();

    private:
        std::vector<std::unique_ptr<PlanetGravity>> _placement_gravities{};
        std::vector<PlanetGravity*> _gravities{};
        StageGravityLoadStats _stats{};
    };

}  // namespace smgpc::scene
