#include "Game/Screen/LensFlare.hpp"

#include "Game/LiveActor/Nerve.hpp"
#include "Game/MapObj/BrightObj.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/AreaObjUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/TriggerChecker.hpp"
#include "JSystem/J3DGraphAnimator/J3DAnimation.hpp"
#include "render/BrightVisibilityService.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

// The exact source helper is not yet part of pc-port's compiled ObjUtil
// subset. Keep the actor call source-shaped; its generalized provider is a
// separate scheduler integration step.
namespace MR {
    void connectToScene3DModelFor2D(LiveActor* actor);
}

namespace {

    constexpr f32 cFadeStep = 0.05F;
    constexpr f32 cRingScale = 0.135F;
    constexpr f32 cDirectionEpsilon = 0.001F;
    constexpr f32 cRadiansToDegrees = 57.295776F;

    NEW_NERVE(LensFlareModelNrvKill, LensFlareModel, Kill);
    NEW_NERVE(LensFlareModelNrvHide, LensFlareModel, Hide);
    NEW_NERVE(LensFlareModelNrvShow, LensFlareModel, Show);
    NEW_NERVE(LensFlareModelNrvFadeIn, LensFlareModel, FadeIn);
    NEW_NERVE(LensFlareModelNrvFadeOut, LensFlareModel, FadeOut);

    // BrightObjBase unregisters during actor teardown, which may run after the
    // SceneObjHolder binding has already been cleared. This non-owning pointer
    // is therefore cleared before the director releases any of its children.
    LensFlareDirector* sLiveLensFlareDirector = nullptr;

    f32 repeatDegree(f32 degree) {
        degree = std::fmod(degree, 360.0F);
        return degree < 0.0F ? degree + 360.0F : degree;
    }

    f32 calcRotateY(const TVec2f& direction) {
        return repeatDegree(std::atan2(-direction.y, direction.x) * cRadiansToDegrees + 90.0F);
    }

}  // namespace

LensFlareModel::LensFlareModel(const char* name, const char* archiveName)
    : LiveActor(name), mIntensity(0.0F), mFade(0.0F), mFadeStep(0.0F), mAreaTrigger(new TriggerChecker()),
      mBrightTrigger(new TriggerChecker()) {
    initModelManagerWithAnm(archiveName, nullptr, false);
    MR::connectToScene3DModelFor2D(this);
    MR::invalidateClipping(this);
    initNerve(&LensFlareModelNrvKill::sInstance);
    kill();
}

LensFlareModel::~LensFlareModel() {
    delete mAreaTrigger;
    delete mBrightTrigger;
    mAreaTrigger = nullptr;
    mBrightTrigger = nullptr;
}

void LensFlareModel::update(bool inArea, bool brightVisible) {
    mAreaTrigger->update(inArea);

    if (mAreaTrigger->getOnTrigger()) {
        notifyInArea();
    } else if (mAreaTrigger->getOffTrigger()) {
        setNerve(&LensFlareModelNrvFadeOut::sInstance);
    }

    mBrightTrigger->update(brightVisible && inArea);

    if (mBrightTrigger->getOnTrigger()) {
        if (isNerve(&LensFlareModelNrvHide::sInstance)) {
            setNerve(&LensFlareModelNrvShow::sInstance);
        }
    } else if (mBrightTrigger->getOffTrigger() &&
               (isNerve(&LensFlareModelNrvShow::sInstance) || isNerve(&LensFlareModelNrvFadeIn::sInstance))) {
        setNerve(&LensFlareModelNrvHide::sInstance);
    }
}

void LensFlareModel::exeKill() {
    if (MR::isFirstStep(this)) {
        MR::hideModel(this);
        kill();
    }
}

void LensFlareModel::exeHide() {
    if (MR::isFirstStep(this)) {
        MR::hideModel(this);
        mFade = 0.0F;
    }
}

void LensFlareModel::exeShow() {
    if (MR::isFirstStep(this)) {
        MR::showModel(this);
        mFade = 1.0F;
    }
}

void LensFlareModel::exeFadeIn() {
    if (MR::isFirstStep(this)) {
        MR::showModel(this);
    }

    mFade += mFadeStep;
    if (mFade >= 1.0F) {
        mFade = 1.0F;
        setNerve(&LensFlareModelNrvShow::sInstance);
    }
}

void LensFlareModel::exeFadeOut() {
    mFade -= mFadeStep;
    if (mFade <= 0.0F) {
        mFade = 0.0F;
        setNerve(&LensFlareModelNrvKill::sInstance);
    }
}

void LensFlareModel::appear() {
    LiveActor::appear();
    appearAnim();
}

void LensFlareModel::control() {
    controlAnim();
}

void LensFlareModel::appearAnim() {
}

void LensFlareModel::controlAnim() {
}

void LensFlareModel::notifyInArea() {
    if (MR::isDead(this)) {
        appear();
    }

    if (mBrightTrigger->getLevel()) {
        setNerve(&LensFlareModelNrvFadeIn::sInstance);
    } else {
        setNerve(&LensFlareModelNrvHide::sInstance);
    }
}

LensFlareRing::LensFlareRing() : LensFlareModel("レンズフレアリング", "LensFlare"), mDistanceFrame(0.0F) {
    mScale.set(cRingScale, cRingScale, cRingScale);
    mFadeStep = cFadeStep;
}

void LensFlareRing::appearAnim() {
    MR::startBckWithInterpole(this, "LensFlare", 0);
}

void LensFlareRing::controlAnim() {
    MR::startBrk(this, "LensFlare");
    MR::setBrkRate(this, 0.0F);
    MR::setBrkFrame(this, (1.0F - mIntensity * mFade) * static_cast< f32 >(MR::getBrkCtrl(this)->getEnd()));

    if (MR::isBckStopped(this)) {
        MR::startBckWithInterpole(this, "LensFlare", 0);
    }

    MR::setBckFrameAndStop(this, mDistanceFrame * static_cast< f32 >(MR::getBckCtrl(this)->getEnd()));
}

LensFlareGlow::LensFlareGlow() : LensFlareModel("グレア（円形）", "GlareGlow") {
    mFadeStep = cFadeStep;
}

void LensFlareGlow::appearAnim() {
    MR::startBtk(this, "GlareGlow");
}

void LensFlareGlow::controlAnim() {
    MR::startBrk(this, "GlareGlow");
    MR::setBrkRate(this, 0.0F);
    MR::setBrkFrame(this, (1.0F - mIntensity * mFade) * static_cast< f32 >(MR::getBrkCtrl(this)->getEnd()));
}

LensFlareLine::LensFlareLine() : LensFlareModel("グレア（ライン）", "GlareLine") {
    mFadeStep = cFadeStep;
}

void LensFlareLine::appearAnim() {
}

void LensFlareLine::controlAnim() {
    MR::startBrk(this, "GlareLine");
    MR::setBrkRate(this, 0.0F);
    MR::setBrkFrame(this, (1.0F - mIntensity * mFade) * static_cast< f32 >(MR::getBrkCtrl(this)->getEnd()));
}

LensFlareDirector::LensFlareDirector()
    : NameObj("レンズフレア管理"), mDrawSyncCallbackHost(nullptr), mRing(nullptr), mGlow(nullptr), mLine(nullptr),
      mBrightObjArray(), mBrightnessCenter(0.0F, 0.0F), mBright(0.0F), mRealCenter(0.0F, 0.0F),
      mNowCenter(0.0F, 0.0F), mDrawSyncTokenBase(0U), mDrawSyncTokenIndex(0U), mBrightCamInfo(nullptr) {
    sLiveLensFlareDirector = this;
}

LensFlareDirector::~LensFlareDirector() {
    if (sLiveLensFlareDirector == this) {
        sLiveLensFlareDirector = nullptr;
    }

    mBrightObjArray.clear();
    delete mBrightCamInfo;
    mRing = nullptr;
    mGlow = nullptr;
    mLine = nullptr;
    mBrightCamInfo = nullptr;
}

void LensFlareDirector::init(const JMapInfoIter&) {
    mRing = new LensFlareRing();
    mGlow = new LensFlareGlow();
    mLine = new LensFlareLine();
    mBrightCamInfo = new BrightCamInfo();
    MR::connectToSceneMapObjMovement(this);
}

void LensFlareDirector::movement() {
    const s32 area = checkArea();
    controlFlare(area, checkBrightObj(area != 0));

    const auto* projection = MR::getCameraProjectionMtx();
    if (projection == nullptr || mBrightCamInfo == nullptr) {
        return;
    }

    const TPos3f view(MR::getCameraViewMtx());
    mBrightCamInfo->write(mDrawSyncTokenIndex, view, *projection, MR::getCamYdir(), MR::getCamPos());
}

void LensFlareDirector::drawSyncCallback(u16 token) {
    if (mBrightCamInfo == nullptr) {
        return;
    }

    const u16 bufferIndex = static_cast< u16 >(token - mDrawSyncTokenBase);
    smgpc::render::begin_bright_visibility_capture(bufferIndex);
    for (auto* brightObj : mBrightObjArray) {
        if (brightObj != nullptr) {
            brightObj->calcBrightInfo(bufferIndex, *mBrightCamInfo);
        }
    }
    smgpc::render::end_bright_visibility_capture();
}

void LensFlareDirector::pauseOff() {
    MR::requestMovementOn(this);
    MR::requestMovementOn(mRing);
    MR::requestMovementOn(mGlow);
    MR::requestMovementOn(mLine);
}

void LensFlareDirector::setDrawSyncToken() {
    // The scheduler reaches this at the retail BrightSun pass boundary. The
    // host visibility service tags that exact depth pass; results are consumed
    // only after the matching asynchronous snapshot completes.
    drawSyncCallback(static_cast< u16 >(mDrawSyncTokenBase + mDrawSyncTokenIndex));
    mDrawSyncTokenIndex ^= 1U;
}

s32 LensFlareDirector::checkArea() {
    const auto* playerPosition = MR::getPlayerPos();
    if (playerPosition == nullptr) {
        return 0;
    }

    auto* area = MR::getAreaObj("LensFlareArea", *playerPosition);
    if (area == nullptr) {
        return 0;
    }

    const s32 flags = MR::getAreaObjArg(area, 0);
    return flags == -1 ? 0xFFFF : flags;
}

bool LensFlareDirector::checkBrightObj(bool check) {
    if (!check || mBrightObjArray.size() == 0) {
        return false;
    }

    BrightObjBase* selected = nullptr;
    for (auto* brightObj : mBrightObjArray) {
        if (brightObj != nullptr && brightObj->getBright() > 0.0F) {
            selected = brightObj;
            break;
        }
    }

    if (selected != nullptr) {
        mBrightnessCenter.set(*selected->getBrightCenter());
        mBright = selected->getBright();
        mRealCenter.set(*selected->getCenter());
        selected->getNowCenter(&mNowCenter);
    }

    for (auto* brightObj : mBrightObjArray) {
        if (brightObj != nullptr) {
            brightObj->endRead();
        }
    }

    return selected != nullptr;
}

void LensFlareDirector::controlFlare(s32 areaFlags, bool hasBrightObj) {
    mRing->update((areaFlags & 0x2) != 0, hasBrightObj);
    mGlow->update((areaFlags & 0x4) != 0, hasBrightObj);
    mLine->update((areaFlags & 0x8) != 0, hasBrightObj);

    if (!hasBrightObj || areaFlags == 0) {
        return;
    }

    TVec2f flarePosition = mBrightnessCenter - mRealCenter;
    flarePosition.add(mNowCenter);
    const TVec3f actorPosition(flarePosition.x, -flarePosition.y, 0.0F);
    mRing->mPosition.set(actorPosition);
    mGlow->mPosition.set(actorPosition);
    mLine->mPosition.set(actorPosition);

    const TVec2f screenHalf(static_cast< f32 >(MR::getScreenWidth()) * 0.5F,
                            static_cast< f32 >(MR::getScreenHeight()) * 0.5F);
    const f32 halfDiagonal = screenHalf.length();
    const TVec2f fromCenter = screenHalf - flarePosition;
    const f32 distanceRatio = halfDiagonal > 0.0F ? fromCenter.length() / halfDiagonal : 0.0F;

    TVec3f ringRotation(0.0F, 0.0F, 0.0F);
    if (fromCenter.length() >= cDirectionEpsilon) {
        TVec2f direction = fromCenter;
        direction.scale(1.0F / direction.length());
        ringRotation.z = calcRotateY(direction);
    }
    mRing->mRotation.set(ringRotation);
    mRing->mDistanceFrame = std::min(distanceRatio, 1.0F);

    const f32 intensity = MR::clamp((1.0F - distanceRatio) * mBright, 0.0F, 1.0F);
    mRing->mIntensity = intensity;
    mGlow->mIntensity = intensity;
    mLine->mIntensity = intensity;
}

void LensFlareDirector::addBrightObj(BrightObjBase* brightObj) {
    if (brightObj == nullptr) {
        return;
    }

    if (std::find(mBrightObjArray.begin(), mBrightObjArray.end(), brightObj) != mBrightObjArray.end()) {
        return;
    }
    if (mBrightObjArray.size() >= mBrightObjArray.capacity()) {
        throw std::logic_error("LensFlareDirector supports at most sixteen bright objects.");
    }

    mBrightObjArray.push_back(brightObj);
}

void LensFlareDirector::removeBrightObj(BrightObjBase* brightObj) noexcept {
    if (brightObj == nullptr) {
        return;
    }

    const auto it = std::find(mBrightObjArray.begin(), mBrightObjArray.end(), brightObj);
    if (it != mBrightObjArray.end()) {
        mBrightObjArray.erase(it);
    }
}

namespace MR {

    void addBrightObj(BrightObjBase* brightObj) {
        if (brightObj == nullptr) {
            return;
        }

        if (!MR::isExistSceneObj(SceneObj_LensFlareDirector)) {
            MR::createSceneObj(SceneObj_LensFlareDirector);
        }

        auto* director = MR::getSceneObj< LensFlareDirector >(SceneObj_LensFlareDirector);
        if (director == nullptr) {
            throw std::logic_error("SceneObj_LensFlareDirector could not be created.");
        }
        director->addBrightObj(brightObj);
    }

    void removeBrightObj(BrightObjBase* brightObj) noexcept {
        if (sLiveLensFlareDirector != nullptr) {
            sLiveLensFlareDirector->removeBrightObj(brightObj);
        }
    }

    void setLensFlareDrawSyncToken() {
        if (sLiveLensFlareDirector != nullptr) {
            sLiveLensFlareDirector->setDrawSyncToken();
        }
    }

    u16 getLensFlareDrawSyncTokenIndex() {
        return sLiveLensFlareDirector != nullptr ? sLiveLensFlareDirector->mDrawSyncTokenIndex : 0U;
    }

}  // namespace MR
