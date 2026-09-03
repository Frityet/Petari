#pragma once

namespace MR {
    void reinitGX();
    void resetTextureCacheSize();
    void loadViewMtxFor2DModel();
    void clearAlphaBuffer(u8);
    void clearAlphaBuffer(u8, const TVec2f&, const TVec2f&);
    void drawInit();
    void setDefaultViewportAndScissor();
    void fillSilhouetteColor();
    void drawInitFor2DModel();
    void activateGameSceneDraw3D();
    void deactivateGameSceneDraw3D();
}  // namespace MR

#include "JSystem/JGeometry/TVec.hpp"
class JUTTexture;
namespace MR {
 const JUTTexture* getMarioShadowTex();
 const JUTTexture* getMarioShadowTexForLoad();
 const TVec3f& getMarioShadowVec();
 void setMarioShadowTex(const JUTTexture*);
 void setMarioShadowVec(const TVec3f&);
}
