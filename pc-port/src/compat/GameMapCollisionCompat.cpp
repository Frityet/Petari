#include "Game/Util/MapUtil.hpp"

namespace MR {
    bool getFirstPolyOnLineToMap(TVec3f*, Triangle*, const TVec3f&, const TVec3f&) {
        // This is the generalized host map-collision boundary. The stage host
        // does not expose collision triangles yet, so a query reports no hit
        // and deliberately leaves caller-provided outputs unchanged.
        return false;
    }

    bool getFirstPolyNormalOnLineToMap(TVec3f* pNormal, const TVec3f&, const TVec3f&, TVec3f* pPosition, const HitSensor*) {
        if (pNormal != nullptr) {
            pNormal->zero();
        }
        if (pPosition != nullptr) {
            pPosition->zero();
        }
        return false;
    }

    bool isExistMapCollision(const TVec3f&, const TVec3f&) {
        return false;
    }
}  // namespace MR
