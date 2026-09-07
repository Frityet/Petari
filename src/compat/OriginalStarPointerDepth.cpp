#include "JSystem/JUtility/JUTVideo.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Screen/StarPointerController.hpp"

#include "Game/Screen/StarPointerDirector.hpp"

#include "Game/Util/ScreenUtil.hpp"

#include "Game/Util/MathUtil.hpp"

#include "Game/Util/StarPointerUtil.hpp"

#include "compat/StarPointerDepthOwnership.hpp"

#include <dolphin/gx/GXGet.h>


namespace {
static f32 mtx_identity[3][4] = {{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}};
}

StarPointerTransformHolder::StarPointerTransformHolder() : mViewMtx(mtx_identity) {
    // Retail copies 16 floats from its 12-float identity symbol, reading into
    // the adjacent vtable. Copy the defined rows on native hosts; the real
    // camera supplies all 16 projection elements before depth submission.
    for (s32 row = 0; row < 3; ++row) {
        for (s32 column = 0; column < 4; ++column) {
            mProjMtx.mMtx[row][column] = mtx_identity[row][column];
        }
    }
}

void StarPointerTransformHolder::movement() {
    f32 fovyRad = PI_180 * getFovy();
    mFocalLength = ((MR::getScreenHeight() * 0.5f) / MR::tan(fovyRad * 0.5f));
}

void StarPointerPeekZ::drawSyncCallback(u16 token) {
    for (s32 channel = 0; channel < StarPointerFunction::getNumStarPointer(); channel++) {
        if (MR::isInRange(mInfos[channel]->mPos.x, 0.0f, MR::getScreenWidth() - 1) &&
            MR::isInRange(mInfos[channel]->mPos.y, 0.0f, MR::getScreenHeight() - 1)) {
            TVec2f pos;
            MR::convertScreenPosToFrameBufferPos(&pos, mInfos[channel]->mPos);
            GXPeekZ(pos.x, pos.y, &mInfos[channel]->mZDepth);
            mInfos[channel]->mDrawReady = true;
        }
    }
}

// Native draw-sync identity is the retained Aurora snapshot ID. The Wii
// manager token is not submitted to a fake GameSystem/FIFO manager.
StarPointerPeekZ::StarPointerPeekZ() : mToken(0), mInfos(new DpdInfo*[2]) {}

void StarPointerPeekZ::setDrawSyncToken() {
    smgpc::compat::require_star_pointer_depth().capture();
}

namespace StarPointerFunction {

bool forceInsideScreenEdge(TVec2f* pPos) {
        bool forced = false;

        f32 margin = 0.0f;
        f32 width = MR::getScreenWidth() - margin;
        f32 height = MR::getScreenHeight() - margin;

        if (pPos->x < margin) {
            pPos->x = margin;
            forced = true;
        } else if (width < pPos->x) {
            pPos->x = width;
            forced = true;
        }

        if (pPos->y < margin) {
            pPos->y = margin;
            forced = true;
        } else if (height < pPos->y) {
            pPos->y = height;
            forced = true;
        }

        return forced;
    }

s32 getNumStarPointer() {
        return 2;
    }

} // namespace StarPointerFunction

namespace MR {
MtxPtr getStarPointerViewMtx() {
    return smgpc::compat::require_star_pointer_depth().transform().mViewMtx;
}

Mtx44Ptr getStarPointerProjMtx() {
    return smgpc::compat::require_star_pointer_depth().transform().mProjMtx;
}

TVec3f* getStarPointerWorldPosUsingDepth(s32 channel) {
    return &smgpc::compat::require_star_pointer_depth().world_position(channel);
}
} // namespace MR

namespace MR {
    f32 calcPointRadius2D(const TVec3f& rPosition, f32 radius) {
        f32 fovyRad = MR::getFovy() * PI_180;
        f32 tan = MR::tan(fovyRad * 0.5f);
        f32 focalDist = (static_cast< s32 >(JUTVideo::getManager()->getEfbHeight()) * 0.5f) / tan;

        TVec3f viewPos;
        MR::getCameraViewMtx().mult(rPosition, viewPos);

        return radius * focalDist / -viewPos.z;
    }
}
