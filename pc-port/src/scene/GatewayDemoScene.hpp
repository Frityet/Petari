#pragma once

#include "scene/StageCollisionService.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <cstdint>
#include <memory>
#include <span>

class GravityInfo;
class NameObj;
class PlanetGravity;
class PlanetMap;
class ProjectionMapSky;
class SceneObjHolder;

namespace smgpc::runtime {
    class DvdFileSystemService;
}

namespace smgpc::compat {
    class DemoSceneRuntime;
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

    struct GatewayDemoVisual {
        const StagePlacementObject *placement = nullptr;
        NameObj *actor = nullptr;
    };

    // A deliberately narrow, real-data scene surface for bringing up Mario.
    // It is not a permissive StageHost replacement: the Gateway scenario-1
    // route selects authored visual rows and resources through the shared
    // production factory, while exposing the mysterious planet and its child
    // zone gravity as the exact player-start surface. Production placement and
    // factory policy remain owned by StageHostScene.
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
        [[nodiscard]] const StagePlacementObject &sky_placement() const;
        // Runtime-free data probes still retain the authored sky placement;
        // the playable route constructs and exposes its exact actor.
        [[nodiscard]] ProjectionMapSky *sky();
        [[nodiscard]] const ProjectionMapSky *sky() const;
        [[nodiscard]] PlanetMap *planet();
        [[nodiscard]] const PlanetMap *planet() const;
        [[nodiscard]] std::span<const GatewayDemoVisual> visuals() const;
        [[nodiscard]] std::span<const StagePlacementObject> placements() const;
        [[nodiscard]] std::span<const StageGeneralPos> general_positions() const;
        [[nodiscard]] smgpc::compat::DemoSceneRuntime &demo_runtime();
        [[nodiscard]] const smgpc::compat::DemoSceneRuntime &demo_runtime() const;
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
