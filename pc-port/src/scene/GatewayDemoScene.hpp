#pragma once

#include "render/J3dModel.hpp"
#include "scene/StageCollisionService.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <cstdint>
#include <memory>
#include <span>

class GravityInfo;
class NameObj;
class PlanetGravity;
class SceneObjHolder;

namespace smgpc::runtime {
    class DvdFileSystemService;
}

namespace smgpc::scene {

    // Evidence returned by the development scene's exact spawn-point query.
    // The surface span remains valid for the lifetime of GatewayDemoScene.
    struct GatewayDemoStartContact {
        TVec3f gravity{};
        StageCollisionHit collision{};
        StageCollisionSurface surface{};
        float separation = 0.0F;
    };

    // A deliberately narrow, real-data scene surface for bringing up Mario.
    // It is not a permissive StageHost replacement: it loads only the retail
    // Gateway scenario-1 StartInfo, the specifically placed mysterious planet,
    // and that child zone's exact point gravity. Production placement/factory
    // policy remains owned by StageHostScene.
    class GatewayDemoScene final {
    public:
        explicit GatewayDemoScene(smgpc::runtime::DvdFileSystemService &dvd);
        ~GatewayDemoScene();

        GatewayDemoScene(const GatewayDemoScene &) = delete;
        GatewayDemoScene &operator=(const GatewayDemoScene &) = delete;
        GatewayDemoScene(GatewayDemoScene &&) = delete;
        GatewayDemoScene &operator=(GatewayDemoScene &&) = delete;

        [[nodiscard]] const StageStartInfo &start_info() const;
        // A forthcoming real MarioActor can consume the retained retail row
        // directly through its normal init boundary.
        [[nodiscard]] JMapInfoIter player_start_iter() const &;
        [[nodiscard]] JMapInfoIter player_start_iter() const && = delete;

        [[nodiscard]] const StagePlacementObject &planet_placement() const;
        [[nodiscard]] const StagePlacementObject &gravity_placement() const;
        [[nodiscard]] std::span<const StagePlacementObject> placements() const;
        [[nodiscard]] std::span<const StageGeneralPos> general_positions() const;
        [[nodiscard]] const smgpc::render::J3dModelGeometry &planet_geometry() const;
        [[nodiscard]] std::span<const std::uint8_t> planet_bdl() const;
        [[nodiscard]] std::span<const std::uint8_t> planet_kcl() const;
        [[nodiscard]] std::span<const std::uint8_t> planet_pa() const;

        [[nodiscard]] SceneObjHolder &scene_obj_holder();
        [[nodiscard]] StageCollisionService &collision();
        [[nodiscard]] const StageCollisionService &collision() const;
        [[nodiscard]] const PlanetGravity &gravity() const;

        [[nodiscard]] bool resolve_gravity(const NameObj &requester, const TVec3f &position,
                                           TVec3f *destination, GravityInfo *info = nullptr) const;
        [[nodiscard]] GatewayDemoStartContact prove_start_contact(const NameObj &requester) const;

    private:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

}  // namespace smgpc::scene
