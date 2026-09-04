#include "Game/Effect/ParticleDrawExecutor.hpp"
#include "Game/Effect/EffectSystem.hpp"
#include "Game/NameObj/NameObjAdaptor.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/Color.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include "JSystem/JParticle/JPAEmitterManager.hpp"
#include "JSystem/JUtility/JUTVideo.hpp"

namespace {
    void connectToSceneDrawAdaptor(NameObjAdaptor*, const MR::FunctorBase&, int) NO_INLINE;

    void connectToSceneDrawAdaptor(NameObjAdaptor* pAdaptor, const MR::FunctorBase& rFunctor, int drawType) {
        pAdaptor->connectToDraw(rFunctor);
        MR::connectToScene(pAdaptor, -1, -1, -1, drawType);
        MR::registerPreDrawFunction(MR::Functor_Inline(ParticleDrawExecutor::initDraw), drawType);
    }
}  // namespace

ParticleDrawExecutor::ParticleDrawExecutor(const EffectSystem* pHost, bool createAdaptor)
    : mHost(pHost), _4(nullptr), _8(nullptr), _C(nullptr), _10(nullptr), _14(nullptr), _18(nullptr), _1C(nullptr), _20(true), _21(false) {
    if (createAdaptor) {
        initDrawAdaptor();
    }
}

void ParticleDrawExecutor::initDraw() {
    GXSetFog(GX_FOG_NONE, 0.0f, 0.0f, 0.0f, 0.0f, Color8(0, 0, 0, 0));
}

void ParticleDrawExecutor::draw3D() const {
    drawWithViewMtx3D(MR::getCameraViewMtx());
}

void ParticleDrawExecutor::draw2D() const {
    if (_20) {
        TPos3f viewMtx;
        viewMtx.identity();
        JPADrawInfo info(viewMtx.toMtxPtr());
        f32 width;
        f32 height = static_cast< s32 >(JUTVideo::getManager()->getEfbHeight());
        width = MR::isScreen16Per9() ? MR::getScreenWidth() : 608.0f;
        width *= 0.5f;
        height *= 0.5f;
        Mtx44 projMtx;
        C_MTXOrtho(projMtx, height, -height, -width, width, -1000.0f, 1000.0f);
        GXSetProjection(projMtx, GX_ORTHOGRAPHIC);
        GXSetCullMode(GX_CULL_NONE);
        GXSetZMode(GX_FALSE, GX_NEVER, GX_FALSE);
        C_MTXLightOrtho(info.mPrjMtx, height, -height, -width, width, 0.5f, 0.5f, 0.5f, 0.5f);
        mHost->mEmitterManager->draw(&info, 6);
        GXSetClipMode(GX_CLIP_ENABLE);
        mHost->mEmitterManager->draw(&info, 7);
        GXSetClipMode(GX_CLIP_ENABLE);
    }
}

void ParticleDrawExecutor::drawIndirect() const {
    if (_20) {
        JPADrawInfo info(j3dSys.getViewMtx(), MR::getFovy(), MR::getAspect());
        mHost->mEmitterManager->draw(&info, 2);
        GXSetClipMode(GX_CLIP_ENABLE);
    }
}

void ParticleDrawExecutor::drawAfterIndirect() const {
    drawWithViewMtxAfterIndirect(MR::getCameraViewMtx());
}

void ParticleDrawExecutor::drawFor2DModel() const {
    if (_20) {
        JPADrawInfo info(j3dSys.getViewMtx());
        mHost->mEmitterManager->draw(&info, 8);
        GXSetClipMode(GX_CLIP_ENABLE);
    }
}

void ParticleDrawExecutor::drawForBloomEffect() const {
    drawWithViewMtxForBloomEffect(MR::getCameraViewMtx());
}

void ParticleDrawExecutor::drawAfterImageEffect() const {
    drawWithViewMtxAfterImageEffect(MR::getCameraViewMtx());
}

void ParticleDrawExecutor::drawWithViewMtx3D(const TPos3f& rViewMtx) const {
    if (_20) {
        JPADrawInfo info(rViewMtx);
        mHost->mEmitterManager->draw(&info, 0);
        GXSetClipMode(GX_CLIP_ENABLE);
        mHost->mEmitterManager->draw(&info, 1);
        GXSetClipMode(GX_CLIP_ENABLE);
    }
}

void ParticleDrawExecutor::drawWithViewMtxAfterIndirect(const TPos3f& rViewMtx) const {
    if (_20) {
        JPADrawInfo info(rViewMtx);
        mHost->mEmitterManager->draw(&info, 3);
        GXSetClipMode(GX_CLIP_ENABLE);
    }
}

void ParticleDrawExecutor::drawWithViewMtxForBloomEffect(const TPos3f& rViewMtx) const {
    if (_20) {
        JPADrawInfo info(rViewMtx);
        mHost->mEmitterManager->draw(&info, 4);
        GXSetClipMode(GX_CLIP_ENABLE);
    }
}

void ParticleDrawExecutor::drawWithViewMtxAfterImageEffect(const TPos3f& rViewMtx) const {
    if (_20) {
        JPADrawInfo info(rViewMtx);
        mHost->mEmitterManager->draw(&info, 5);
        GXSetClipMode(GX_CLIP_ENABLE);
    }
}

void ParticleDrawExecutor::initDrawAdaptor() {
    _4 = new NameObjAdaptor("3Dパーティクル");
    connectToSceneDrawAdaptor(_4, MR::Functor(static_cast< const ParticleDrawExecutor* >(this), &ParticleDrawExecutor::draw3D), 71);
    _8 = new NameObjAdaptor("2Dパーティクル");
    connectToSceneDrawAdaptor(_8, MR::Functor(static_cast< const ParticleDrawExecutor* >(this), &ParticleDrawExecutor::draw2D), 74);
    _C = new NameObjAdaptor("インダイレクトパーティクル");
    connectToSceneDrawAdaptor(_C, MR::Functor(static_cast< const ParticleDrawExecutor* >(this), &ParticleDrawExecutor::drawIndirect), 72);
    _10 = new NameObjAdaptor("インダイレクト後パーティクル");
    connectToSceneDrawAdaptor(_10, MR::Functor(static_cast< const ParticleDrawExecutor* >(this), &ParticleDrawExecutor::drawAfterIndirect), 73);
    _14 = new NameObjAdaptor("2Dモデル用パーティクル");
    connectToSceneDrawAdaptor(_14, MR::Functor(static_cast< const ParticleDrawExecutor* >(this), &ParticleDrawExecutor::drawFor2DModel), 75);
    _18 = new NameObjAdaptor("ブルーム用パーティクル");
    connectToSceneDrawAdaptor(_18, MR::Functor(static_cast< const ParticleDrawExecutor* >(this), &ParticleDrawExecutor::drawForBloomEffect), 76);
    _1C = new NameObjAdaptor("イメージエフェクト後パーティクル");
    connectToSceneDrawAdaptor(_1C, MR::Functor(static_cast< const ParticleDrawExecutor* >(this), &ParticleDrawExecutor::drawAfterImageEffect), 77);
}
