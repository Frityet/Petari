#include "Game/Util/DirectDraw.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include <JSystem/J3DGraphBase/J3DSys.hpp>

namespace TDDraw {

void mixFogColor(TVec3f a1, f32 a2, u32 a3) {
        f32 nearZ = MR::getNearZ();
        f32 farZ = MR::getFarZ();
        GXColor color;
        setGXColor(a3, &color);
        f32 v11;
        f32 v10;
        MR::calcFogStartEnd(a1, a2, &v11, &v10);
        GXSetFog(GX_FOG_PERSP_LIN, v11, v10, nearZ, farZ, color);
    }

void setGXColor(u32 a1, GXColor* pColor) {
        pColor->r = (a1 >> 24) & 0xFF;
        pColor->g = (a1 >> 16) & 0xFF;
        pColor->b = (a1 >> 8) & 0xFF;
        pColor->a = a1 & 0xFF;
    }

}

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
