#include "Game/Util/MapUtil.hpp"

#include "scene/StageCollisionService.hpp"

#include <stdexcept>

namespace {
    smgpc::scene::StageCollisionService& require_stage_collision() {
        auto* collision = smgpc::scene::StageCollisionService::active();
        if (collision == nullptr) {
            throw std::logic_error("Map-collision queries require a scene-owned CollisionDirector equivalent.");
        }
        return *collision;
    }
}  // namespace

namespace MR {
    bool getFirstPolyOnLineToMap(TVec3f* pPosition, Triangle*, const TVec3f& rStart, const TVec3f& rOffset) {
        auto hit = smgpc::scene::StageCollisionHit{};
        if (!require_stage_collision().line_cast(rStart, rOffset, &hit)) {
            return false;
        }
        if (pPosition != nullptr) {
            pPosition->set(hit.position);
        }
        return true;
    }

    bool getFirstPolyNormalOnLineToMap(TVec3f* pNormal, const TVec3f& rStart, const TVec3f& rOffset,
                                       TVec3f* pPosition, const HitSensor*) {
        auto hit = smgpc::scene::StageCollisionHit{};
        if (!require_stage_collision().line_cast(rStart, rOffset, &hit)) {
            return false;
        }
        if (pNormal != nullptr) {
            pNormal->set(hit.normal);
        }
        if (pPosition != nullptr) {
            pPosition->set(hit.position);
        }
        return true;
    }

    bool isExistMapCollision(const TVec3f& rStart, const TVec3f& rOffset) {
        return require_stage_collision().line_cast(rStart, rOffset);
    }
}  // namespace MR
