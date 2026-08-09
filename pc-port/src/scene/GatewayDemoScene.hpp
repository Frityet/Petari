#pragma once

#include "scene/StageCollisionService.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <cstdint>
#include <memory>
#include <span>

class GravityInfo;
class LiveActor;
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
    class StageSessionState;
}

namespace smgpc::scene {

    struct AuthoredPlacementInstantiationReport;

    enum class GatewayDemoSceneState : std::uint8_t {
        Preloaded,
        Finalizing,
        Active,
        Retired,
    };

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
    // route reports the complete authored set and constructs every ready row
    // through the shared placement lifecycle, while exposing the mysterious
    // planet and its child-zone gravity as exact player-start evidence.
    // Construction stops after the exact placement archive preload. The caller
    // owns and initializes Mario at that boundary, then receives a move-only
    // lease which keeps all placement actors active. Declaring the lease after
    // the external player makes reverse local destruction retire placements
    // and collision first.
    // Production placement policy remains Strict in StageHostScene.
    class GatewayDemoScene final {
        class Impl;

    public:
        class PlacementLease final {
        public:
            PlacementLease() = default;
            ~PlacementLease();

            PlacementLease(const PlacementLease &) = delete;
            PlacementLease &operator=(const PlacementLease &) = delete;
            PlacementLease(PlacementLease &&other) noexcept;
            PlacementLease &operator=(PlacementLease &&other) noexcept;

            [[nodiscard]] explicit operator bool() const noexcept;
            void reset() noexcept;

        private:
            friend class GatewayDemoScene;
            explicit PlacementLease(std::weak_ptr<Impl> impl) noexcept;

            std::weak_ptr<Impl> _impl{};
            bool _armed = false;
        };

        explicit GatewayDemoScene(smgpc::runtime::DvdFileSystemService &dvd);
        ~GatewayDemoScene();

        GatewayDemoScene(const GatewayDemoScene &) = delete;
        GatewayDemoScene &operator=(const GatewayDemoScene &) = delete;
        GatewayDemoScene(GatewayDemoScene &&) = delete;
        GatewayDemoScene &operator=(GatewayDemoScene &&) = delete;

        // The supplied player must already be attached to the active runtime
        // and must have completed its ordinary init(JMapInfoIter) scene
        // registration. This call owns its initAfterPlacement boundary but
        // never owns or destroys the player itself.
        [[nodiscard]] PlacementLease finalize_placements(LiveActor &player);
        [[nodiscard]] GatewayDemoSceneState state() const noexcept;

        [[nodiscard]] const StageStartInfo &start_info() const;
        // The external player consumes the retained retail row directly
        // through its normal init boundary before finalization.
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
        [[nodiscard]] const AuthoredPlacementInstantiationReport &
        authored_placement_report() const;
        [[nodiscard]] const smgpc::compat::StageSessionState &
        stage_session() const;
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
        std::shared_ptr<Impl> _impl;
    };

}  // namespace smgpc::scene
