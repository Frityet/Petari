#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "Game/Map/HitInfo.hpp"

namespace MR {
    void calcVelocityMovingPoint(const Triangle* pTriangle, const TVec3f& rPos, TVec3f* pVelocity) {
        if (isSameMtx(pTriangle->getBaseMtx()->toMtxPtr(), pTriangle->getPrevBaseMtx()->toMtxPtr())) {
            pVelocity->zero();
            return;
        }

        TVec3f localPos;
        PSMTXMultVec(pTriangle->getBaseInvMtx()->toMtxPtr(), &rPos, &localPos);
        TVec3f prevPos;
        PSMTXMultVec(pTriangle->getPrevBaseMtx()->toMtxPtr(), &localPos, &prevPos);
        *pVelocity = rPos - prevPos;
    }

}
