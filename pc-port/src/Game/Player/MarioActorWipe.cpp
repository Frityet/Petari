#include "Game/Player/MarioActor.hpp"

#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/DirectDraw.hpp"
#include "Game/Util/DrawUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include <JSystem/JUtility/JUTVideo.hpp>

void MarioActor::drawPreWipe() const {
    if (!_1C3) {
        if (!_A61) {
            return;
        }

        const f32 width = MR::getScreenWidth();
        const f32 height = static_cast< s32 >(JUTVideo::getManager()->getEfbHeight());
        TDDraw::setup(0, 1, 2);

        const TVec3f topLeft(0.0f, 0.0f, 0.0f);
        const TVec3f bottomRight(width, height, 0.0f);
        TDDraw::drawFillBox(topLeft, bottomRight, _A6C <= 0x80 ? _A6C + (_A6C >> 1) + (_A6C >> 2) : 0xE0);
    }

    MR::loadViewMtx();
    MR::loadProjectionMtx();
    MR::loadActorLight(this);
    drawMarioModel();
    MR::drawInitFor2DModel();
}

namespace NrvMarioActor {
    INIT_NERVE(MarioActorNrvWait);
    INIT_NERVE(MarioActorNrvGameOver);
    INIT_NERVE(MarioActorNrvGameOverAbyss);
    INIT_NERVE(MarioActorNrvGameOverAbyss2);
    INIT_NERVE(MarioActorNrvGameOverFire);
    INIT_NERVE(MarioActorNrvGameOverBlackHole);
    INIT_NERVE(MarioActorNrvGameOverNonStop);
    INIT_NERVE(MarioActorNrvGameOverSink);
    INIT_NERVE(MarioActorNrvTimeWait);
    INIT_NERVE(MarioActorNrvNoRush);
};  // namespace NrvMarioActor
