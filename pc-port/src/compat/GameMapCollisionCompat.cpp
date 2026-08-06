#include "Game/Util/MapUtil.hpp"

#include "scene/StageCollisionService.hpp"

namespace MR {
    bool getFirstPolyOnLineToMap(TVec3f* pPosition, Triangle*, const TVec3f& rStart, const TVec3f& rOffset) {
        auto* collision = smgpc::scene::StageCollisionService::active();
        auto hit = smgpc::scene::StageCollisionHit{};
        if (collision == nullptr || !collision->line_cast(rStart, rOffset, &hit)) {
            return false;
        }
        if (pPosition != nullptr) {
            pPosition->set(hit.position);
        }
        return true;
    }

    bool getFirstPolyNormalOnLineToMap(TVec3f* pNormal, const TVec3f& rStart, const TVec3f& rOffset,
                                       TVec3f* pPosition, const HitSensor*) {
        auto* collision = smgpc::scene::StageCollisionService::active();
        auto hit = smgpc::scene::StageCollisionHit{};
        if (collision == nullptr || !collision->line_cast(rStart, rOffset, &hit)) {
            if (pNormal != nullptr) {
                pNormal->zero();
            }
            if (pPosition != nullptr) {
                pPosition->zero();
            }
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
        auto* collision = smgpc::scene::StageCollisionService::active();
        return collision != nullptr && collision->line_cast(rStart, rOffset);
    }
}  // namespace MR
