#pragma once

#include <array>
#include <memory>
#include "compat/ActorRuntimeRegistry.hpp"
namespace smgpc::scene { class StageCollisionService; }

class CollisionDirector;
class CollisionCategorizedKeeper;
class CollisionCode;
class CollisionZone;
class HitInfo;

namespace smgpc::compat {
    // SceneObjHolder owns the original NameObjs. This owner reclaims the
    // original raw arrays and non-NameObj children after those objects retire.
    class CollisionDirectorOwnership final {
    public:
        CollisionDirectorOwnership();
        ~CollisionDirectorOwnership();
        CollisionDirector* construct();
        void prepare_retirement() noexcept;
        bool prepare_rollback(NameObjRuntimeRegistrationMarker marker) noexcept;
        void reclaim() noexcept;
        scene::StageCollisionService& category_service(int category);

    private:
        std::array<std::unique_ptr<scene::StageCollisionService>, 3> _category_services;
        CollisionDirector* _director = nullptr;
        CollisionCategorizedKeeper** _keepers = nullptr;
        CollisionCode* _code = nullptr;
        std::array<HitInfo*, 4> _hit_infos{};
        std::array<std::array<CollisionZone*, 32>, 4> _zones{};
        bool _prepared = false;
    };
}
