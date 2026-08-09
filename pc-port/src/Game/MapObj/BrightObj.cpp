#include "Game/MapObj/BrightObj.hpp"

#include "Game/MapObj/Sun.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Screen/LensFlare.hpp"
#include "Game/Util/ActorSwitchUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "render/BrightVisibilityService.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

    constexpr f32 cDirectionEpsilon = 0.001F;
    constexpr f32 cInnerSampleRadius = 0.4F;
    constexpr f32 cOuterSampleRadius = 0.7F;
    constexpr f32 cSampleAngleStep = 0.25F * PI;
    constexpr f32 cSunDistance = 100000.0F;
    constexpr f32 cSunRadius = 3000.0F;
    constexpr f32 cSunModelScale = 100.0F;
    constexpr f32 cRadiansToDegrees = 57.295776F;

    bool calcScreenPosition(TVec2f* result, f32* normalizedZOut, const TVec3f& worldPosition, const TMtx34f& viewMtx,
                            const TProj3f& projectionMtx) {
        TVec3f viewPosition;
        viewMtx.mult(worldPosition, viewPosition);

        const f32 w = -viewPosition.z;
        if (std::abs(w) <= 0.000001F) {
            result->zero();
            return false;
        }

        const f32 inverseW = 1.0F / w;
        const f32 normalizedX =
            (viewPosition.x * projectionMtx.mMtx[0][0] + viewPosition.z * projectionMtx.mMtx[0][2]) * inverseW;
        const f32 normalizedY =
            (viewPosition.y * projectionMtx.mMtx[1][1] + viewPosition.z * projectionMtx.mMtx[1][2]) * inverseW;
        const f32 normalizedZ =
            (viewPosition.z * projectionMtx.mMtx[2][2] + projectionMtx.mMtx[2][3]) * inverseW;

        if (normalizedZOut != nullptr) {
            *normalizedZOut = normalizedZ;
        }

        result->set((normalizedX + 1.0F) * 0.5F * static_cast< f32 >(MR::getScreenWidth()),
                    (1.0F - normalizedY) * 0.5F * static_cast< f32 >(MR::getScreenHeight()));

        return std::abs(normalizedX) <= 1.0F && std::abs(normalizedY) <= 1.0F && normalizedZ <= 0.0F;
    }

    bool calcScreenPosition(TVec2f* result, const TVec3f& worldPosition, const TMtx34f& viewMtx,
                            const TProj3f& projectionMtx) {
        return calcScreenPosition(result, nullptr, worldPosition, viewMtx, projectionMtx);
    }

}  // namespace

BrightInfo::BrightInfo() {
    reset();
}

void BrightInfo::write(const TVec2f& brightnessCenter, const TVec2f& realCenter, f32 bright) {
    mBrightnessCenter[mWriteIndex].set(brightnessCenter);
    mRealCenter[mWriteIndex].set(realCenter);
    mBright[mWriteIndex] = bright;

    const u32 next = (mWriteIndex + 1U) % 3U;
    if (next != mReadIndex) {
        mWriteIndex = next;
    }
}

void BrightInfo::endRead() {
    if (mReadIndex != mWriteIndex) {
        mReadIndex = (mReadIndex + 1U) % 3U;
    }
}

void BrightInfo::reset() {
    mWriteIndex = 0U;
    mReadIndex = 0U;

    for (u32 i = 0U; i < 3U; ++i) {
        mBrightnessCenter[i].zero();
        mRealCenter[i].zero();
        mBright[i] = 0.0F;
    }
}

BrightDrawInfo::BrightDrawInfo() {
    for (u32 i = 0U; i < 2U; ++i) {
        mPosition[i].zero();
        mRadius[i] = 0.0F;
    }
}

void BrightDrawInfo::write(u16 token, const TVec3f& position, f32 radius) {
    const auto index = static_cast< u32 >(token & 1U);
    mPosition[index].set(position);
    mRadius[index] = radius;
}

BrightCamInfo::BrightCamInfo() {
    for (u32 i = 0U; i < 2U; ++i) {
        mViewMtx[i].identity();
        mProjectionMtx[i].identity();
        mCameraDir[i].set(0.0F, 1.0F, 0.0F);
        mCameraPos[i].zero();
    }
}

void BrightCamInfo::write(u16 token, const TPos3f& viewMtx, const TProj3f& projectionMtx, const TVec3f& cameraDir,
                          const TVec3f& cameraPos) {
    const auto index = static_cast< u32 >(token & 1U);
    mViewMtx[index].set(viewMtx);
    mProjectionMtx[index].set(projectionMtx);
    mCameraDir[index].set(cameraDir);
    mCameraPos[index].set(cameraPos);
}

BrightObjBase::BrightObjBase()
    : mBrightInfo(), mBrightnessCenter(0.0F, 0.0F), mRealCenter(0.0F, 0.0F), mBright(0.0F), mNoVisible(true),
      mDrawInfo(), mVisibilitySourceId(smgpc::render::allocate_bright_visibility_source_id()) {
    MR::addBrightObj(this);
}

BrightObjBase::~BrightObjBase() {
    smgpc::render::forget_bright_visibility_source(mVisibilitySourceId);
    MR::removeBrightObj(this);
}

f32 BrightObjBase::getBright() const {
    return mBrightInfo.mBright[mBrightInfo.mReadIndex];
}

const TVec2f* BrightObjBase::getBrightCenter() const {
    return &mBrightInfo.mBrightnessCenter[mBrightInfo.mReadIndex];
}

const TVec2f* BrightObjBase::getCenter() const {
    return &mBrightInfo.mRealCenter[mBrightInfo.mReadIndex];
}

void BrightObjBase::endRead() {
    mBrightInfo.endRead();
}

void BrightObjBase::getNowCenter(TVec2f* center) const {
    center->set(mRealCenter);
}

void BrightObjBase::checkVisibilityOfSphere(u16 token, const BrightCamInfo& camera) {
    const auto index = static_cast< u32 >(token & 1U);
    const TVec3f sphereCenter = mDrawInfo.mPosition[index];
    const f32 sphereRadius = mDrawInfo.mRadius[index];
    const TMtx34f& viewMtx = camera.mViewMtx[index];
    const TProj3f& projectionMtx = camera.mProjectionMtx[index];

    smgpc::render::BrightVisibilityResult completed;
    if (smgpc::render::take_bright_visibility_result(mVisibilitySourceId, completed)) {
        CheckArg result{
            .mSampleCount = completed.sample_count,
            .mVisibleCount = completed.visible_count,
            .mVisiblePositionSum = completed.visible_position_sum,
            .mRealCenter = completed.real_center,
        };
        setResult(result);
    }

    TVec3f viewDirection = camera.mCameraPos[index] - sphereCenter;
    if (MR::isNearZero(viewDirection, cDirectionEpsilon)) {
        mNoVisible = true;
        mBright = 0.0F;
        return;
    }
    viewDirection.normalize();

    const TVec3f right = camera.mCameraDir[index].cross(viewDirection);
    const TVec3f up = viewDirection.cross(right);

    TPos3f sampleMtx;
    sampleMtx.identity();
    sampleMtx.setXYZDir(right, up, viewDirection);
    sampleMtx.setTrans(sphereCenter);

    smgpc::render::BrightVisibilityBatch batch{
        .source = mVisibilitySourceId,
        .draw_token = static_cast< u16 >(index),
        .sphere = {
            .center = sphereCenter,
            .radius = sphereRadius,
        },
        .center_ndc_z = std::numeric_limits< f32 >::quiet_NaN(),
    };
    calcScreenPosition(&batch.real_center, &batch.center_ndc_z, sphereCenter, viewMtx, projectionMtx);

    auto appendProbe = [&](const TVec3f& position, u32 sampleIndex) {
        auto& probe = batch.probes[sampleIndex];
        probe.on_screen = calcScreenPosition(&probe.screen_position, position, viewMtx, projectionMtx);
        if (probe.on_screen) {
            MR::convertScreenPosToFrameBufferPos(&probe.framebuffer_position, probe.screen_position);
        }
    };

    u32 sampleIndex = 0U;
    appendProbe(sphereCenter, sampleIndex++);

    for (u32 i = 0U; i < 8U; ++i) {
        const f32 angle = static_cast< f32 >(i) * cSampleAngleStep;
        TVec3f sample(MR::cos(angle), MR::sin(angle), 0.0F);
        sample.scale(cInnerSampleRadius * sphereRadius);
        sampleMtx.mult(sample, sample);
        appendProbe(sample, sampleIndex++);
    }

    for (u32 i = 0U; i < 8U; ++i) {
        const f32 angle = (static_cast< f32 >(i) + 0.5F) * cSampleAngleStep;
        TVec3f sample(MR::cos(angle), MR::sin(angle), 0.0F);
        sample.scale(cOuterSampleRadius * sphereRadius);
        sampleMtx.mult(sample, sample);
        appendProbe(sample, sampleIndex++);
    }

    smgpc::render::submit_bright_visibility_batch(batch);
}

void BrightObjBase::setResult(const CheckArg& result) {
    const f32 sampleCount = static_cast< f32 >(result.mSampleCount);
    mBright = sampleCount > 0.0F ? static_cast< f32 >(result.mVisibleCount) / sampleCount : 0.0F;

    if (result.mVisibleCount != 0U) {
        mBrightnessCenter.set(result.mVisiblePositionSum);
        mBrightnessCenter.scale(1.0F / static_cast< f32 >(result.mVisibleCount));
        mRealCenter.set(result.mRealCenter);
        mNoVisible = false;
    } else {
        mNoVisible = true;
    }

    mBrightInfo.write(mBrightnessCenter, result.mRealCenter, mBright);
}

void BrightObjBase::drawSphere(const TVec3f& position, f32 radius) const {
    smgpc::render::submit_bright_visibility_sphere(mVisibilitySourceId,
                                                   {.center = position, .radius = radius});
}

BrightObj::BrightObj(const char* name) : LiveActor(name), BrightObjBase(), mRadius(100.0F) {
}

BrightObj::~BrightObj() = default;

void BrightObj::init(const JMapInfoIter& iter) {
    MR::initDefaultPos(this, iter);
    MR::getJMapInfoArg0NoInit(iter, &mRadius);
    MR::invalidateClipping(this);
    MR::connectToScene(this, MR::MovementType_Environment, -1, -1, MR::DrawType_BrightSun);
    makeActorAppeared();
}

void BrightObj::control() {
    mDrawInfo.write(MR::getLensFlareDrawSyncTokenIndex(), mPosition, mRadius);
}

void BrightObj::draw() const {
    if (!MR::isDead(this) && !MR::isHiddenModel(this) && !MR::isClipped(this)) {
        drawSphere(mPosition, mRadius);
    }
}

void BrightObj::calcBrightInfo(u16 token, const BrightCamInfo& camera) {
    checkVisibilityOfSphere(token, camera);
}

void BrightObj::getNowCenter(TVec2f* center) const {
    const auto* projection = MR::getCameraProjectionMtx();
    if (projection == nullptr) {
        center->zero();
        return;
    }

    const TPos3f view(MR::getCameraViewMtx());
    calcScreenPosition(center, mPosition, view, *projection);
}

BrightSun::BrightSun(const char* name) : LiveActor(name), BrightObjBase(), mSun(nullptr) {
}

BrightSun::~BrightSun() = default;

void BrightSun::init(const JMapInfoIter& iter) {
    MR::initDefaultPos(this, iter);
    MR::invalidateClipping(this);
    MR::connectToScene(this, MR::MovementType_Environment, -1, -1, MR::DrawType_BrightSun);

    mSun = new Sun("太陽");
    mSun->initWithoutIter();

    MR::tryRegisterDemoCast(this, iter);
    if (MR::useStageSwitchReadAppear(this, iter)) {
        MR::syncStageSwitchAppear(this);
        makeActorDead();
    } else {
        makeActorAppeared();
    }
}

void BrightSun::control() {
    TPos3f offsetMtx;
    offsetMtx.identity();
    offsetMtx.setTrans(0.0F, 0.0F, cSunDistance);

    TPos3f cameraMtx;
    MR::makeMtxTR(cameraMtx.toMtxPtr(), MR::getCamPos(), mRotation);
    cameraMtx.concat(offsetMtx);
    cameraMtx.getTrans(mPosition);

    controlSunModel();
    mDrawInfo.write(MR::getLensFlareDrawSyncTokenIndex(), mPosition, cSunRadius);
}

void BrightSun::draw() const {
    if (!MR::isDead(this) && !MR::isHiddenModel(this) && !MR::isClipped(this)) {
        drawSphere(mPosition, cSunRadius);
    }
}

void BrightSun::calcBrightInfo(u16 token, const BrightCamInfo& camera) {
    if (!MR::isDead(this)) {
        checkVisibilityOfSphere(token, camera);
    }
}

void BrightSun::getNowCenter(TVec2f* center) const {
    const auto* projection = MR::getCameraProjectionMtx();
    if (projection == nullptr) {
        center->zero();
        return;
    }

    const TPos3f view(MR::getCameraViewMtx());
    calcScreenPosition(center, mPosition, view, *projection);
}

void BrightSun::controlSunModel() {
    if (mSun == nullptr) {
        return;
    }

    mSun->mPosition.set(mPosition);
    mSun->mScale.set(cSunModelScale, cSunModelScale, cSunModelScale);

    TVec3f direction = MR::getCamPos() - mPosition;
    if (MR::normalizeOrZero(&direction)) {
        mSun->mRotation.zero();
        return;
    }

    const TVec3f front(0.0F, 0.0F, 1.0F);
    if (front.dot(direction) < -0.999F) {
        mSun->mRotation.set(0.0F, 180.0F, 0.0F);
        return;
    }

    TQuat4f rotation;
    rotation.setRotate(front, direction, 1.0F);
    rotation.getEuler(mSun->mRotation);
    mSun->mRotation.scale(cRadiansToDegrees);
}
