#include "Game/MapObj/BrightObj.hpp"
#include "Game/MapObj/Sun.hpp"
#include "Game/Screen/LensFlare.hpp"
#include "Game/Util/ActorSwitchUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/DirectDraw.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include <JSystem/JGeometry/TUtil.hpp>
#include <JSystem/JMath/JMATrigonometric.hpp>
#include <JSystem/JUtility/JUTVideo.hpp>
#include <cstring>
#include <revolution/gx.h>

#define BRIGHT_SAMPLE_COUNT 8
#define BRIGHT_DRAW_COLOR_OCCLUDED 0xFF000000
#define BRIGHT_DRAW_COLOR_VISIBLE 0x00FF00FF

namespace TDDraw {
    void loadViewMtx(MtxPtr);
};

namespace {
    bool calcScreenPosition(TVec2f* pOut, const TVec3f& rPosition, const TPos3f& rViewMtx, const TProj3f& rProjMtx) {
        TVec3f viewPos;
        rViewMtx.mult(rPosition, viewPos);

        TVec3f projected;
        rProjMtx.mult(viewPos, projected);

        bool isVisible = false;
        if (1.0f >= __fabsf(projected.x) && 1.0f >= __fabsf(projected.y)) {
            isVisible = 0.0f >= projected.z;
        }

        const f32 width = MR::getScreenWidth();
        const f32 height = JUTVideo::getManager()->getRenderMode()->efbHeight;
        TVec3f screenPos;
        screenPos.set((width * 0.5f) + (projected.x * width * 0.5f), (height * 0.5f) + (-projected.y * height * 0.5f), projected.z);
        pOut->set(screenPos.x, screenPos.y);
        return isVisible;
    }
};

BrightInfo::BrightInfo() {
    reset();
}

void BrightInfo::write(const TVec2f& rBrightCenter, const TVec2f& rCenter, f32 bright) {
    mBrightnessCenter[_0].set(rBrightCenter);
    mRealCenter[_0].set(rCenter);
    mBright[_0] = bright;

    u32 nextWrite = _0 + 1;
    if (nextWrite > 2) {
        nextWrite = 0;
    }

    if (nextWrite != _4) {
        _0 = nextWrite;
    }
}

void BrightInfo::endRead() {
    u32 v1 = _4;
    if (v1 != _0) {
        _4 = v1 + 1;

        if (v1 + 1 > 2) {
            _4 = 0;
        }
    }
}

void BrightInfo::reset() {
    _0 = 0;
    _4 = 0;

    for (u32 i = 0; i < 3; i++) {
        mBrightnessCenter[i].zero();
        mRealCenter[i].zero();
        mBright[i] = 0.0f;
    }
}

BrightDrawInfo::BrightDrawInfo() {
    for (u32 i = 0; i < 2; i++) {
        mPosition[i].zero();
        mRadius[i] = 0.0f;
    }
}

void BrightDrawInfo::write(u16 index, const TVec3f& rPosition, f32 radius) {
    mPosition[index].set(rPosition);
    mRadius[index] = radius;
}

BrightCamInfo::BrightCamInfo() {
    for (u32 i = 0; i < 2; i++) {
        mViewMtx[i].identity();

        mProjectionMtx[i].mMtx[0][0] = 1.0f;
        mProjectionMtx[i].mMtx[0][1] = 0.0f;
        mProjectionMtx[i].mMtx[0][2] = 0.0f;
        mProjectionMtx[i].mMtx[0][3] = 0.0f;
        mProjectionMtx[i].mMtx[1][0] = 0.0f;
        mProjectionMtx[i].mMtx[1][1] = 1.0f;
        mProjectionMtx[i].mMtx[1][2] = 0.0f;
        mProjectionMtx[i].mMtx[1][3] = 0.0f;
        mProjectionMtx[i].mMtx[2][0] = 0.0f;
        mProjectionMtx[i].mMtx[2][1] = 0.0f;
        mProjectionMtx[i].mMtx[2][2] = 1.0f;
        mProjectionMtx[i].mMtx[2][3] = 0.0f;
        mProjectionMtx[i].mMtx[3][0] = 0.0f;
        mProjectionMtx[i].mMtx[3][1] = 0.0f;
        mProjectionMtx[i].mMtx[3][2] = 0.0f;
        mProjectionMtx[i].mMtx[3][3] = 1.0f;

        mCameraDir[i].set< f32 >(0.0f, 1.0f, 0.0f);
        mCameraPos[i].zero();
    }
}

void BrightCamInfo::write(u16 index, const TPos3f& rViewMtx, const TProj3f& rProjMtx, const TVec3f& rCameraDir, const TVec3f& rCameraPos) {
    mViewMtx[index].setInline(rViewMtx);
    mProjectionMtx[index].setInline(rProjMtx);
    mCameraDir[index].set(rCameraDir);
    mCameraPos[index].set(rCameraPos);
}

BrightObjBase::BrightObjBase()
    : mBrightInfo(), mBrightCenter(0.0f, 0.0f), mNowCenter(0.0f, 0.0f), mBright(0.0f), mIsNotVisible(true), mDrawInfo() {
    MR::addBrightObj(this);
}

BrightObjBase::~BrightObjBase() {}

void BrightObjBase::checkVisibilityOfSphere(u16 index, const BrightCamInfo& rCamInfo) {
    GXPokeAlphaRead(GX_READ_NONE);

    TVec3f center(mDrawInfo.mPosition[index]);
    TVec3f cameraDir(rCamInfo.mCameraDir[index]);
    TVec3f cameraToCenter = rCamInfo.mCameraPos[index] - center;

    if (MR::isNearZero(cameraToCenter, 0.001f)) {
        mIsNotVisible = true;
        mBright = 0.0f;
        return;
    }

    MR::normalize(&cameraToCenter);

    TVec3f xDir;
    PSVECCrossProduct(&cameraDir, &cameraToCenter, &xDir);
    PSVECCrossProduct(&cameraToCenter, &xDir, &cameraDir);

    TPos3f sphereMtx;
    sphereMtx.mMtx[0][0] = xDir.x;
    sphereMtx.mMtx[1][0] = xDir.y;
    sphereMtx.mMtx[2][0] = xDir.z;
    sphereMtx.mMtx[0][1] = cameraDir.x;
    sphereMtx.mMtx[1][1] = cameraDir.y;
    sphereMtx.mMtx[2][1] = cameraDir.z;
    sphereMtx.mMtx[0][2] = cameraToCenter.x;
    sphereMtx.mMtx[1][2] = cameraToCenter.y;
    sphereMtx.mMtx[2][2] = cameraToCenter.z;
    sphereMtx.mMtx[0][3] = center.x;
    sphereMtx.mMtx[1][3] = center.y;
    sphereMtx.mMtx[2][3] = center.z;

    CheckArg arg;
    arg.mCheckCount = 0;
    arg.mVisibleCount = 0;
    arg.mBrightCenterSum.x = 0.0f;
    arg.mBrightCenterSum.y = 0.0f;
    arg.mCenter.x = 0.0f;
    arg.mCenter.y = 0.0f;

    calcScreenPosition(&arg.mCenter, center, rCamInfo.mViewMtx[index], rCamInfo.mProjectionMtx[index]);
    checkVisible(&arg, center, rCamInfo.mViewMtx[index], rCamInfo.mProjectionMtx[index]);

    for (u32 i = 0; i < BRIGHT_SAMPLE_COUNT; i++) {
        const f32 angle = i * 3.1415927f * 0.25f;
        TVec3f probe(MR::cos(angle), MR::sin(angle), 0.0f);
        probe.scale(mDrawInfo.mRadius[index] * 0.4f);
        sphereMtx.mult(probe, probe);
        checkVisible(&arg, probe, rCamInfo.mViewMtx[index], rCamInfo.mProjectionMtx[index]);
    }

    for (u32 i = 0; i < BRIGHT_SAMPLE_COUNT; i++) {
        const f32 angle = (0.5f + i) * 3.1415927f * 0.25f;
        TVec3f probe(MR::cos(angle), MR::sin(angle), 0.0f);
        probe.scale(mDrawInfo.mRadius[index] * 0.7f);
        sphereMtx.mult(probe, probe);
        checkVisible(&arg, probe, rCamInfo.mViewMtx[index], rCamInfo.mProjectionMtx[index]);
    }

    setResult(arg);
}

void BrightObjBase::checkVisible(CheckArg* pArg, const TVec3f& rPosition, const TPos3f& rViewMtx, const TProj3f& rProjMtx) {
    TVec2f screenPos;

    if (calcScreenPosition(&screenPos, rPosition, rViewMtx, rProjMtx)) {
        TVec2f frameBufferPos;
        MR::convertScreenPosToFrameBufferPos(&frameBufferPos, screenPos);

        u32 argb;
        GXPeekARGB((u16)(s32)frameBufferPos.x, (u16)(s32)frameBufferPos.y, &argb);

        if ((argb & 0xFF000000) >= 0xF0000000) {
            pArg->mVisibleCount++;
            pArg->mBrightCenterSum.x += screenPos.x;
            pArg->mBrightCenterSum.y += screenPos.y;
        }
    }

    pArg->mCheckCount++;
}

void BrightObjBase::setResult(const CheckArg& rArg) {
    f32 bright = static_cast<f32>(rArg.mVisibleCount) / static_cast<f32>(rArg.mCheckCount);

    if (rArg.mVisibleCount != 0) {
        TVec2f brightCenter = rArg.mBrightCenterSum * (1.0f / static_cast<f32>(rArg.mVisibleCount));
        mBrightCenter.set(brightCenter);
        mBright = bright;
        mNowCenter.set(rArg.mCenter);
        mIsNotVisible = false;
    }
    else {
        mBright = bright;
        mIsNotVisible = true;
    }

    mBrightInfo.write(mBrightCenter, rArg.mCenter, mBright);
}

void BrightObjBase::drawSphere(const TVec3f& rPosition, f32 radius) const {
    MtxPtr cameraViewMtx = MR::getCameraViewMtx();
    TPos3f viewMtx;
    memcpy(viewMtx.mMtx, cameraViewMtx, sizeof(Mtx));

    TPos3f drawMtx;
    drawMtx.identity33();
    drawMtx.mMtx[0][3] = rPosition.x;
    drawMtx.mMtx[1][3] = rPosition.y;
    drawMtx.mMtx[2][3] = rPosition.z;
    drawMtx.concat(viewMtx, drawMtx);

    TDDraw::setup(0, 0, 1);
    TDDraw::loadViewMtx(drawMtx.toMtxPtr());
    GXSetColorUpdate(GX_FALSE);
    GXSetAlphaUpdate(GX_TRUE);
    GXSetDstAlpha(GX_FALSE, 0);
    GXSetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);

    TVec3f zero(0, 0, 0);
    TDDraw::drawSphere(zero, radius * 1.1f, BRIGHT_DRAW_COLOR_OCCLUDED, 0x10);

    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);

    TVec3f zero2(0, 0, 0);
    TDDraw::drawSphere(zero2, radius, BRIGHT_DRAW_COLOR_VISIBLE, 0x10);

    GXSetColorUpdate(GX_TRUE);
    GXSetAlphaUpdate(GX_FALSE);
}

void BrightObjBase::endRead() {
    mBrightInfo.endRead();
}

const TVec2f* BrightObjBase::getCenter() const {
    return &mBrightInfo.mRealCenter[mBrightInfo._4];
}

const TVec2f* BrightObjBase::getBrightCenter() const {
    return &mBrightInfo.mBrightnessCenter[mBrightInfo._4];
}

f32 BrightObjBase::getBright() const {
    return mBrightInfo.mBright[mBrightInfo._4];
}

void BrightObjBase::getNowCenter(TVec2f* pCenter) const {
    pCenter->set(mNowCenter);
}

BrightObj::BrightObj(const char* pName) : LiveActor(pName), BrightObjBase(), mRadius(100.0f) {}

BrightObj::~BrightObj() {}

void BrightObj::init(const JMapInfoIter& rIter) {
    MR::initDefaultPos(this, rIter);
    MR::getJMapInfoArg0NoInit(rIter, &mRadius);
    MR::invalidateClipping(this);
    MR::connectToScene(this, 0x21, -1, -1, 0x39);
    makeActorAppeared();
}

void BrightObj::control() {
    f32 radius = mRadius;
    mDrawInfo.write(MR::getLensFlareDrawSyncTokenIndex(), mPosition, radius);
}

void BrightObj::draw() const {
    if (!MR::isDead(this) && !MR::isHiddenModel(this) && !MR::isClipped(this)) {
        drawSphere(mPosition, mRadius);
    }
}

void BrightObj::calcBrightInfo(u16 index, const BrightCamInfo& rCamInfo) {
    checkVisibilityOfSphere(index, rCamInfo);
}

void BrightObj::getNowCenter(TVec2f* pCenter) const {
    calcScreenPosition(pCenter, mPosition, *reinterpret_cast< const TPos3f* >(MR::getCameraViewMtx()), *MR::getCameraProjectionMtx());
}

BrightSun::BrightSun(const char* pName) : LiveActor(pName), BrightObjBase(), mSun(nullptr) {}

BrightSun::~BrightSun() {}

void BrightSun::control() {
    const TVec3f cameraPos = MR::getCamPos();
    TPos3f offsetMtx;
    offsetMtx.identity33();
    offsetMtx.mMtx[0][3] = 0.0f;
    offsetMtx.mMtx[1][3] = 0.0f;
    offsetMtx.mMtx[2][3] = 100000.0f;

    TPos3f cameraMtx;
    MR::makeMtxTR(cameraMtx.toMtxPtr(), cameraPos.x, cameraPos.y, cameraPos.z, mRotation.x, mRotation.y, mRotation.z);
    cameraMtx.concat(offsetMtx);
    mPosition.set< f32 >(cameraMtx.mMtx[0][3], cameraMtx.mMtx[1][3], cameraMtx.mMtx[2][3]);

    controlSunModel();
    mDrawInfo.write(MR::getLensFlareDrawSyncTokenIndex(), mPosition, 3000.0f);
}

void BrightSun::draw() const {
    if (!MR::isDead(this) && !MR::isHiddenModel(this) && !MR::isClipped(this)) {
        drawSphere(mPosition, 3000.0f);
    }
}

void BrightSun::calcBrightInfo(u16 index, const BrightCamInfo& rCamInfo) {
    if (!MR::isDead(this)) {
        checkVisibilityOfSphere(index, rCamInfo);
    }
}

void BrightSun::getNowCenter(TVec2f* pCenter) const {
    calcScreenPosition(pCenter, mPosition, *reinterpret_cast< const TPos3f* >(MR::getCameraViewMtx()), *MR::getCameraProjectionMtx());
}

void BrightSun::controlSunModel() {
    mSun->mPosition.set(mPosition);
    mSun->mScale.set(100.0f, 100.0f, 100.0f);

    const TVec3f cameraPos = MR::getCamPos();
    TVec3f cameraDir = cameraPos - mPosition;
    MR::normalize(&cameraDir);

    TVec3f zDir(0.0f, 0.0f, 1.0f);
    TRot3f rotationMtx;
    rotationMtx.identity();

    TQuat4f quat;
    quat.setRotate(zDir, cameraDir);
    rotationMtx.setQuat(quat);

    TVec3f rotation;
    if (zDir.dot(cameraDir) < -0.999f) {
        rotation.set(0.0f, 180.0f, 0.0f);
    }
    else {
        if (rotationMtx.mMtx[2][0] - 1.0f >= -0.0000038146973f) {
            rotation.x = JMath::sAtanTable.atan2_(-rotationMtx.mMtx[0][1], rotationMtx.mMtx[1][1]);
            rotation.y = -1.5707964f;
            rotation.z = 0.0f;
        }
        else if (rotationMtx.mMtx[2][0] + 1.0f <= 0.0000038146973f) {
            rotation.x = JMath::sAtanTable.atan2_(rotationMtx.mMtx[0][1], rotationMtx.mMtx[1][1]);
            rotation.y = 1.5707964f;
            rotation.z = 0.0f;
        }
        else {
            rotation.x = JMath::sAtanTable.atan2_(rotationMtx.mMtx[2][1], rotationMtx.mMtx[2][2]);
            rotation.z = JMath::sAtanTable.atan2_(rotationMtx.mMtx[1][0], rotationMtx.mMtx[0][0]);
            rotation.y = JGeometry::TUtil<f32>::asin(-rotationMtx.mMtx[2][0]);
        }

        rotation.scale(57.295776f);
    }

    mSun->mRotation.set(rotation);
}

void BrightSun::init(const JMapInfoIter& rIter) {
    MR::initDefaultPos(this, rIter);
    MR::invalidateClipping(this);
    MR::connectToScene(this, 0x21, -1, -1, 0x39);

    mSun = new Sun("太陽");
    mSun->initWithoutIter();

    MR::tryRegisterDemoCast(this, rIter);

    if (MR::useStageSwitchReadAppear(this, rIter)) {
        MR::syncStageSwitchAppear(this);
        makeActorDead();
    }
    else {
        makeActorAppeared();
    }
}
