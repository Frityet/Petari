#include "Game/Util/DirectDraw.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include <JSystem/J3DGraphBase/J3DSys.hpp>

namespace MR {
void calcFogStartEnd(TVec3f vec, f32 f1, f32* pFogStart, f32* pFogEnd) {
        Vec multVec;
        PSMTXMultVec(j3dSys.mViewMtx, vec, &multVec);

        f32 flt = -multVec.z;
        if (f1 == 0.0f) {
            *pFogStart = flt;
            *pFogEnd = 100000.0f;
            return;
        }

        f32 fogStart = 100.0f;
        f32 fogEnd = 100.0f + ((flt - 100.0f) / f1);
        if (fogEnd > 100000.0f) {
            fogEnd = 100000.0f;
            fogStart = flt - (100000.0f - flt) * (f1 / (1.0f - f1));
        }

        *pFogStart = fogStart;
        *pFogEnd = fogEnd;
    }
}
