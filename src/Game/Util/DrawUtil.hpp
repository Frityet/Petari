#pragma once

#include "JSystem/J2DGraph/J2DOrthoGraph.hpp"

class J2DOrthoGraphSimple : public J2DOrthoGraph {
public:
    J2DOrthoGraphSimple();

    virtual ~J2DOrthoGraphSimple() {
    }

    virtual void setPort();
};



namespace MR {
    void loadProjectionMtxFor2D();
    void reinitGX();
    void resetTextureCacheSize();
    void loadViewMtxFor2DModel();
    void loadTexProjectionMtx(u32);
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
