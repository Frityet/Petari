#include "Game/Util/DrawUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include "JSystem/J3DGraphBase/J3DShape.hpp"
#include <dolphin/gx.h>
static bool sIsReinitTextureCache;
static GXTexCacheSize sReinitTextureCacheSize;

namespace {
    const f32 cNearZ = -10000.0f;
    const f32 cFarZ = 10000.0f;
    inline s32 getScreenHeightInline() {
        return MR::getScreenHeight();
    }
}
namespace MR {
    void setupDrawForNW4RLayout(f32 a1, bool) {
        f32 v1 = MR::getScreenHeight() * 0.5f * a1;
        f32 v2 = 608.0f * 0.5f * a1;

        Mtx44 projMtx;
        C_MTXOrtho(projMtx, v1, -v1, -v2, v2, -1000.0f, 1000.0f);
        GXSetProjection(projMtx, GX_ORTHOGRAPHIC);
        GXSetCullMode(GX_CULL_NONE);
        GXSetZMode(GX_FALSE, GX_NEVER, GX_FALSE);
    }

    void reinitGX() {
        j3dSys.reinitGX();
        GXSetAlphaUpdate(GX_FALSE);
        J3DShape::resetVcdVatCache();

        if (sIsReinitTextureCache) {
            j3dSys.setTexCacheRegion(sReinitTextureCacheSize);
        }
    }

    void resetTextureCacheSize() {
        if (sIsReinitTextureCache) {
            j3dSys.setTexCacheRegion(sReinitTextureCacheSize);
        }
    }

    void loadViewMtxFor2DModel() {
        Mtx viewMtx;
        PSMTXIdentity(viewMtx);
        PSMTXCopy(viewMtx, j3dSys.mViewMtx);
    }

    void drawInitFor2DModel() {
        MR::reinitGX();
        GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);

        Mtx44 projMtx;
        C_MTXOrtho(projMtx, 0.0f, -MR::getScreenHeight(), 0.0f, MR::getScreenWidth(), ::cNearZ, ::cFarZ);
        GXSetProjection(projMtx, GX_ORTHOGRAPHIC);

        Mtx viewMtx;
        PSMTXIdentity(viewMtx);
        PSMTXCopy(viewMtx, j3dSys.mViewMtx);
    }

    void drawInit() {
        j3dSys.offFlag(2);
        j3dSys.drawInit();
        j3dSys.setTexCacheRegion(GX_TEXCACHE_128K);
    }
    void setDefaultViewportAndScissor() {
        s32 width = MR::getFrameBufferWidth();
        s32 height = getScreenHeightInline();
        GXSetViewport(0.0f, 0.0f, width, height, 0.0f, 1.0f);
        GXSetScissor(0, 0, width, height);
    }
}
