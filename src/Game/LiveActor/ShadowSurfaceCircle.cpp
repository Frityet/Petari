#include "resource/TextEncoding.hpp"
#include "Game/LiveActor/ShadowSurfaceCircle.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/ShadowController.hpp"
#include "Game/Util/DirectDraw.hpp"

ShadowSurfaceCircle::~ShadowSurfaceCircle() {
}

ShadowSurfaceCircle::ShadowSurfaceCircle() : ShadowSurfaceDrawer(CP932("影描画[水面円]")), mRadius(100.0f) {
}

void ShadowSurfaceCircle::setRadius(f32 radius) {
    mRadius = radius;
}

void ShadowSurfaceCircle::draw() const {
    ShadowController* controller = getController();
    if (!controller->isProjected() || !controller->isDraw()) {
        return;
    }

    f32 radius = mRadius;
    if (controller->isFollowHostScale()) {
        radius *= controller->getHost()->mScale.x;
    }

    TVec3f position;
    TVec3f normal;
    controller->getProjectionPos(&position);
    controller->getProjectionNormal(&normal);
    TDDraw::resetViewMtx();
    TDDraw::drawFillCircle(position + normal * 1.0f, -normal, radius, 0x00000080, 20);
}
