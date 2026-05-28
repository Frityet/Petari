#include "Game/Map/FileSelectItem.hpp"

#include "Game/LiveActor/Nerve.hpp"
#include "Game/LiveActor/PartsModel.hpp"
#include "Game/Map/FileSelectItemDelegator.hpp"
#include "Game/Map/FileSelectModel.hpp"
#include "Game/Screen/FileSelectNumber.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/NerveUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "Game/Util/StarPointerUtil.hpp"

#include <algorithm>
#include <cmath>
#include <revolution.h>

namespace {
    constexpr auto cDegreesToRadians = 3.14159265358979323846F / 180.0F;
    constexpr auto cFellowModelTableCount = 5;
    const char* sFellowModel[cFellowModelTableCount] = {
        "FileSelectDataMario", "FileSelectDataLuigi", "FileSelectDataYoshi", "FileSelectDataKinopio", "FileSelectDataPeach",
    };
    const char* sPointedME[cFellowModelTableCount] = {
        "ME_ASTRO_DOME_HIT_GALAXY1", "ME_ASTRO_DOME_HIT_GALAXY2", "ME_ASTRO_DOME_HIT_GALAXY3",
        "ME_ASTRO_DOME_HIT_GALAXY4", "ME_ASTRO_DOME_HIT_GALAXY5",
    };
    const char* sPointedNotUsingME[cFellowModelTableCount] = {
        "ME_ASTRO_DOME_HIT_GALAXY_N1", "ME_ASTRO_DOME_HIT_GALAXY_N2", "ME_ASTRO_DOME_HIT_GALAXY_N3",
        "ME_ASTRO_DOME_HIT_GALAXY_N4", "ME_ASTRO_DOME_HIT_GALAXY_N5",
    };
    const TVec3f sDataInfoOffset = {0.0F, 2150.0F, 0.0F};

    NEW_NERVE(FileSelectItemNrvNewWait, FileSelectItem, NewWait);
    NEW_NERVE(FileSelectItemNrvExistWait, FileSelectItem, ExistWait);
    NEW_NERVE(FileSelectItemNrvFormat, FileSelectItem, Format);
    NEW_NERVE(FileSelectItemNrvChangeFellow, FileSelectItem, ChangeFellow);

    [[nodiscard]] const Nerve* new_wait_nerve() {
        return &FileSelectItemNrvNewWait::sInstance;
    }

    [[nodiscard]] const Nerve* exist_wait_nerve() {
        return &FileSelectItemNrvExistWait::sInstance;
    }

    [[nodiscard]] const Nerve* format_nerve() {
        return &FileSelectItemNrvFormat::sInstance;
    }

    [[nodiscard]] const Nerve* change_fellow_nerve() {
        return &FileSelectItemNrvChangeFellow::sInstance;
    }

    [[nodiscard]] float smooth(float current, float target) {
        return (current * 0.95F) + (target * 0.05F);
    }

    [[nodiscard]] TVec3f cross_product(const TVec3f& a, const TVec3f& b) {
        return TVec3f{
            (a.y * b.z) - (a.z * b.y),
            (a.z * b.x) - (a.x * b.z),
            (a.x * b.y) - (a.y * b.x),
        };
    }

    [[nodiscard]] bool check_collision_of_point_and_cylinder(const TVec3f& point, const TVec3f& base, const TVec3f& axis, f32 radius) {
        const auto axis_length = axis.length();
        if (axis_length <= 0.000001F) {
            return point.distance(base) <= radius;
        }

        auto normalized_axis = axis;
        MR::normalize(&normalized_axis);
        const auto offset = point - base;
        const auto projection = normalized_axis.dot(offset);
        if (projection < 0.0F || projection > axis_length) {
            return false;
        }

        normalized_axis.scale(projection);
        return normalized_axis.distance(offset) <= radius;
    }

    [[nodiscard]] bool is_point_inside_pointer_stroke_triangle(const TVec3f& point, const TVec3f& camera_pos, const TVec3f& current_world,
                                                               const TVec3f& previous_world, f32 radius) {
        const auto edge_camera_to_current = current_world - camera_pos;
        const auto edge_current_to_previous = previous_world - current_world;
        const auto edge_previous_to_camera = camera_pos - previous_world;
        auto normal = cross_product(edge_current_to_previous, edge_camera_to_current);
        MR::normalize(&normal);

        const auto signed_distance = normal.dot(point - camera_pos);
        if (std::fabs(signed_distance) >= radius) {
            return false;
        }

        auto projected = point - (normal * signed_distance);
        auto cross = cross_product(projected - camera_pos, edge_camera_to_current);
        if (cross.dot(normal) < 0.0F) {
            return false;
        }

        cross = cross_product(projected - current_world, edge_current_to_previous);
        if (cross.dot(normal) < 0.0F) {
            return false;
        }

        cross = cross_product(projected - previous_world, edge_previous_to_camera);
        if (cross.dot(normal) >= 0.0F) {
            return true;
        }

        return camera_pos.distance(point) <= radius || current_world.distance(point) <= radius || previous_world.distance(point) <= radius ||
               check_collision_of_point_and_cylinder(point, camera_pos, edge_camera_to_current, radius) ||
               check_collision_of_point_and_cylinder(point, current_world, edge_current_to_previous, radius) ||
               check_collision_of_point_and_cylinder(point, previous_world, edge_previous_to_camera, radius);
    }

    void make_identity(Mtx matrix) {
        matrix[0][0] = 1.0F;
        matrix[0][1] = 0.0F;
        matrix[0][2] = 0.0F;
        matrix[0][3] = 0.0F;
        matrix[1][0] = 0.0F;
        matrix[1][1] = 1.0F;
        matrix[1][2] = 0.0F;
        matrix[1][3] = 0.0F;
        matrix[2][0] = 0.0F;
        matrix[2][1] = 0.0F;
        matrix[2][2] = 1.0F;
        matrix[2][3] = 0.0F;
    }
}  // namespace

namespace FileSelectItemSub {
    NEW_NERVE(ScaleControllerNrvToSmall, ScaleController, ToSmall);
    NEW_NERVE(ScaleControllerNrvSmall, ScaleController, Small);
    NEW_NERVE(ScaleControllerNrvToBig, ScaleController, ToBig);
    NEW_NERVE(ScaleControllerNrvBig, ScaleController, Big);
    NEW_NERVE(BlinkControllerNrvOpen, BlinkController, Open);
    NEW_NERVE(BlinkControllerNrvShut, BlinkController, Shut);
    NEW_NERVE(BlinkControllerNrvSleep, BlinkController, Sleep);
    NEW_NERVE(BlinkControllerNrvBlink, BlinkController, Blink);
}  // namespace FileSelectItemSub

FileSelectItem::FileSelectItem(s32 file_no, bool is_new) : FileSelectItem(file_no, is_new, FileSelectIconID(), "ファイルセレクトアイテム") {
}

FileSelectItem::FileSelectItem(s32 file_no, bool is_new, const FileSelectIconID& rIconId, const char* pName)
    : LiveActor(pName), mFileNo(file_no), mIconID(rIconId), mIsNew(is_new) {
    mScaleCtrl = new FileSelectItemSub::ScaleController();
    mBlinkCtrl = new FileSelectItemSub::BlinkController(this);
    make_identity(mPlanetMatrix);
    make_identity(mFellowMatrix);
}

FileSelectItem::~FileSelectItem() {
    delete mPlanetMapObj;
    for (auto* model : mModels) {
        delete model;
    }
    delete mNumber;
    delete mScaleCtrl;
    delete mBlinkCtrl;
}

void FileSelectItem::init(const JMapInfoIter&) {
    MR::connectToSceneMapObjMovement(this);
    createNew();
    createFellows();
    createNumber();
    MR::initStarPointerTarget(this, 1000.0F, TVec3f(0.0F, 900.0F, 0.0F));
    MR::invalidateClipping(this);
    initNerve(mIsNew ? new_wait_nerve() : exist_wait_nerve());
    makeActorAppeared();
}

void FileSelectItem::movement() {
    if (MR::isDead(this)) {
        return;
    }

    updateNerve();
    if (MR::isDead(this)) {
        return;
    }

    mPosition.set(smooth(mPosition.x, mBasePosition.x), smooth(mPosition.y, mBasePosition.y), smooth(mPosition.z, mBasePosition.z));
    if (mScaleCtrl != nullptr) {
        mScaleCtrl->updateNerve();
    }
    updatePointing();
    updateRotate();
    if (mBlinkCtrl != nullptr) {
        mBlinkCtrl->updateNerve();
    }
    updateModelMatrix();

    if (mNumber != nullptr) {
        auto info_pos = TVec3f(mPosition.x + sDataInfoOffset.x, mPosition.y + sDataInfoOffset.y, mPosition.z + sDataInfoOffset.z);

        auto screen_pos = TVec2f{};
        MR::calcScreenPosition(&screen_pos, info_pos);
        mNumber->setTrans(screen_pos);
    }
}

void FileSelectItem::appear() {
    LiveActor::appear();
    mIsAppeared = true;
    mIsSelectInvalid = false;

    if (mIsNew) {
        killAllModels();
        if (mPlanetMapObj != nullptr) {
            mPlanetMapObj->makeActorAppeared();
        }
        setNerve(new_wait_nerve());
    } else {
        appearFellowModel();
        setNerve(exist_wait_nerve());
    }
}

void FileSelectItem::makeActorDead() {
    LiveActor::makeActorDead();
    mIsAppeared = false;
    killAllModels();
}

void FileSelectItem::forceChange(bool is_new) {
    deleteCompleteEffect();
    mIsNew = is_new;
    mShouldEmitCompleteEffect = false;
    mShouldEmitCopyEffect = false;
    if (!mIsAppeared) {
        killAllModels();
        setNerve(mIsNew ? new_wait_nerve() : exist_wait_nerve());
        return;
    }

    if (mIsNew) {
        killAllModels();
        if (mPlanetMapObj != nullptr) {
            mPlanetMapObj->makeActorAppeared();
        }
    } else {
        appearFellowModel();
    }
    emitCompleteEffect();
    setNerve(mIsNew ? new_wait_nerve() : exist_wait_nerve());
}

void FileSelectItem::forceChange(bool is_new, const FileSelectIconID& rIconId) {
    mIconID.set(rIconId);
    forceChange(is_new);
}

void FileSelectItem::forceChange(const FileSelectIconID& rIconId, bool isComplete) {
    mIconID.set(rIconId);
    deleteCompleteEffect();
    mIsNew = false;
    mShouldEmitCompleteEffect = isComplete;
    mShouldEmitCopyEffect = false;
    if (!mIsAppeared) {
        killAllModels();
        setNerve(exist_wait_nerve());
        return;
    }

    appearFellowModel();
    emitCompleteEffect();
    setNerve(exist_wait_nerve());
}

void FileSelectItem::change(bool is_new) {
    mIsNew = is_new;
    deleteCompleteEffect();
    mShouldEmitCompleteEffect = false;
    mShouldEmitCopyEffect = false;
    setNerve(mIsNew ? format_nerve() : change_fellow_nerve());
}

void FileSelectItem::change(bool is_new, const FileSelectIconID& rIconId) {
    mIconID.set(rIconId);
    change(is_new);
}

void FileSelectItem::change(const FileSelectIconID& rIconId, bool isComplete) {
    mIconID.set(rIconId);
    change(false);
    mShouldEmitCompleteEffect = isComplete;
}

void FileSelectItem::copyIconID(FileSelectIconID* pIconID) const {
    if (pIconID != nullptr) {
        pIconID->set(mIconID);
    }
}

void FileSelectItem::format() {
    mIsNew = true;
    deleteCompleteEffect();
    mShouldEmitCompleteEffect = false;
    mShouldEmitCopyEffect = false;
    setNerve(format_nerve());
}

void FileSelectItem::exeNewWait() {
}

void FileSelectItem::exeExistWait() {
}

void FileSelectItem::exeFormat() {
    if (getNerveStep() < 40) {
        MR::startSystemLevelSE("SE_SY_LV_FILE_SE_MORPHBLUR", -1, -1);
    }

    if (MR::isStep(this, 40)) {
        MR::startSystemSE("SE_SY_FILE_SEL_MORPH_DUMMY", -1, -1);
        emitVanish();
        killAllModels();
        if (mPlanetMapObj != nullptr) {
            mPlanetMapObj->makeActorAppeared();
        }
        MR::tryRumblePadStrong(this, WPAD_CHAN0);
        MR::shakeCameraNormal();
    }

    if (getNerveStep() >= 40) {
        mRotation.y = 0.0F;
        mRotationVelocityY = 0.0F;
    }

    if (MR::isStep(this, 60)) {
        setNerve(new_wait_nerve());
    }
}

void FileSelectItem::exeChangeFellow() {
    if (getNerveStep() < 40) {
        MR::startSystemLevelSE("SE_SY_FILE_SEL_MORPHBLR", -1, -1);
    }

    if (MR::isStep(this, 40)) {
        MR::startSystemSE("SE_SY_FILE_SEL_MORPH_MARIO", -1, -1);
        appearFellowModel();
        emitCompleteEffect();
        if (mShouldEmitCopyEffect) {
            emitCopy();
        } else {
            emitOpen();
        }
        MR::tryRumblePadStrong(this, WPAD_CHAN0);
        MR::shakeCameraNormal();
    }

    if (getNerveStep() >= 40) {
        mRotation.y = 0.0F;
        mRotationVelocityY = 0.0F;
    }

    if (MR::isStep(this, 150)) {
        setNerve(exist_wait_nerve());
    }
}

void FileSelectItem::invalidateSelect() {
    if (mIsPointing) {
        offPointing();
    }

    mIsSelectInvalid = true;
}

void FileSelectItem::validateSelect() {
    mIsSelectInvalid = false;
}

void FileSelectItem::appearIndex() {
    if (mNumber != nullptr) {
        mNumber->appear();
    }
}

void FileSelectItem::disappearIndex() {
    if (mNumber != nullptr) {
        mNumber->disappear();
    }
}

void FileSelectItem::validateRotate() {
    mIsRotateInvalid = false;
}

void FileSelectItem::onPointing() {
    if (mIsSelectInvalid) {
        return;
    }

    if (isNerve(new_wait_nerve())) {
        playPointedNotUsingME();
    } else {
        playPointedME();
    }

    if (mNumber != nullptr) {
        mNumber->onSelectIn();
    }
    mIsPointing = true;
    mWasPointed = true;
    if (mScaleCtrl != nullptr) {
        mScaleCtrl->setNerve(&FileSelectItemSub::ScaleControllerNrvToBig::sInstance);
    }
    MR::tryRumblePadWeak(this, WPAD_CHAN0);
}

void FileSelectItem::offPointing() {
    if (mIsSelectInvalid) {
        return;
    }

    if (mNumber != nullptr) {
        mNumber->onSelectOut();
    }
    mIsPointing = false;
    if (mScaleCtrl != nullptr) {
        mScaleCtrl->setNerve(&FileSelectItemSub::ScaleControllerNrvToSmall::sInstance);
    }
}

void FileSelectItem::clearPointing() {
    offPointing();
    mPointingCleared = true;
}

void FileSelectItem::turnToFront(s32 frameCount) {
    mTurnedToFront = true;
    mTurnToFrontFrameCount = frameCount;
    mTurnToFrontDuration = frameCount;
    mTurnToFrontStep = 0;
}

void FileSelectItem::setBasePosition(const smgpc::camera::CameraParamVec3& base_position) {
    mBasePosition = base_position;
}

void FileSelectItem::setSelectDelegator(FileSelectItemDelegatorBase* pDelegator) {
    mDelegator = pDelegator;
}

s32 FileSelectItem::getFileNo() const {
    return mFileNo;
}

bool FileSelectItem::isExist() const {
    return isNerve(exist_wait_nerve());
}

bool FileSelectItem::isAppeared() const {
    return mIsAppeared;
}

bool FileSelectItem::isNew() const {
    return isNerve(new_wait_nerve());
}

bool FileSelectItem::isSelectInvalid() const {
    return mIsSelectInvalid;
}

bool FileSelectItem::isRotateInvalid() const {
    return mIsRotateInvalid;
}

bool FileSelectItem::isPointing() const {
    return mIsPointing;
}

#ifndef NDEBUG
bool FileSelectItem::wasPointed() const {
    return mWasPointed;
}

bool FileSelectItem::wasPointingCleared() const {
    return mPointingCleared;
}

bool FileSelectItem::didTurnToFront() const {
    return mTurnedToFront;
}

s32 FileSelectItem::getTurnToFrontFrameCount() const {
    return mTurnToFrontFrameCount;
}

const TVec3f& FileSelectItem::getPosition() const {
    return mPosition;
}
#endif

const smgpc::camera::CameraParamVec3& FileSelectItem::getBasePosition() const {
    return mBasePosition;
}

void FileSelectItem::createNew() {
    updateModelMatrix();
    mPlanetMapObj = MR::createPartsModelMapObj(this, "ニューフェイス", "FileSelectDataPlanet", mPlanetMatrix);
    mPlanetMapObj->mScale.x = 30.0F;
    mPlanetMapObj->mScale.y = 30.0F;
    mPlanetMapObj->mScale.z = 30.0F;
    mPlanetMapObj->makeActorDead();
}

void FileSelectItem::createFellows() {
    for (auto i = s32{0}; i < cFellowModelCount; ++i) {
        mModels[static_cast< std::size_t >(i)] = new FileSelectModel(sFellowModel[i], mFellowMatrix, "キャラフェイス");
    }
}

void FileSelectItem::createNumber() {
    mNumber = new FileSelectNumber("ファイル番号");
    mNumber->initWithoutIter();
    mNumber->setNumber(mFileNo);
}

void FileSelectItem::killAllModels() {
    if (mPlanetMapObj != nullptr) {
        mPlanetMapObj->makeActorDead();
    }

    for (auto* model : mModels) {
        if (model != nullptr) {
            model->makeActorDead();
        }
    }
}

void FileSelectItem::appearFellowModel() {
    killAllModels();

    const auto index = static_cast< std::size_t >(getFellowModelIndex());
    if (index < mModels.size() && mModels[index] != nullptr) {
        mModels[index]->makeActorAppeared();
    }
}

void FileSelectItem::emitOpen() {
    if (!MR::isDead(mPlanetMapObj)) {
        MR::emitEffect(mPlanetMapObj, "Open");
    }

    for (auto* model : mModels) {
        if (!MR::isDead(model)) {
            model->emitOpen();
        }
    }
}

void FileSelectItem::emitVanish() {
    if (!MR::isDead(mPlanetMapObj)) {
        MR::emitEffect(mPlanetMapObj, "Vanish");
    }

    for (auto* model : mModels) {
        if (!MR::isDead(model)) {
            model->emitVanish();
        }
    }
}

void FileSelectItem::emitCopy() {
    if (!MR::isDead(mPlanetMapObj)) {
        MR::emitEffect(mPlanetMapObj, "Copy");
    }

    for (auto* model : mModels) {
        if (!MR::isDead(model)) {
            model->emitCopy();
        }
    }
}

void FileSelectItem::emitCompleteEffect() {
    if (!mShouldEmitCompleteEffect) {
        return;
    }

    for (auto* model : mModels) {
        if (!MR::isDead(model)) {
            model->emitCompleteEffect();
        }
    }
}

void FileSelectItem::deleteCompleteEffect() {
    for (auto* model : mModels) {
        if (model != nullptr) {
            model->deleteCompleteEffect();
        }
    }
}

void FileSelectItem::playPointedME() {
    MR::startSystemME(sPointedME[static_cast< std::size_t >(MR::getRandom(0, cFellowModelTableCount))]);
}

void FileSelectItem::playPointedNotUsingME() {
    MR::startSystemME(sPointedNotUsingME[static_cast< std::size_t >(MR::getRandom(0, cFellowModelTableCount))]);
}

s32 FileSelectItem::getFellowModelIndex() const {
    if (!mIconID.isFellow()) {
        return 0;
    }

    const auto index = static_cast< s32 >(mIconID.getFellowID());
    return std::clamp(index, 0, cFellowModelCount - 1);
}

void FileSelectItem::updatePointing() {
    if (mIsSelectInvalid) {
        if (MR::isStarPointerPointing1PWithoutCheckZ(this, nullptr, false, false) && mDelegator != nullptr) {
            mDelegator->notify(this, 2);
        }
    } else if (MR::isStarPointerPointingFileSelect(this) && mDelegator != nullptr) {
        mDelegator->notify(this, 0);
    }

    if (!mIsSelectInvalid && mIsPointing && MR::testDPDMenuPadDecideTrigger() && mDelegator != nullptr) {
        mDelegator->notify(this, 1);
        if (mScaleCtrl != nullptr) {
            mScaleCtrl->setNerve(&FileSelectItemSub::ScaleControllerNrvToSmall::sInstance);
        }
    }
}

void FileSelectItem::updateRotate() {
    if (mIsRotateInvalid) {
        return;
    }

    if (mTurnToFrontDuration > 0) {
        mRotationVelocityY = 0.0F;

        const auto step = ++mTurnToFrontStep;
        const auto denominator = static_cast< f32 >(mTurnToFrontDuration);
        auto progress = denominator > 0.0F ? static_cast< f32 >(step) / denominator : 1.0F;
        progress = std::clamp(progress * progress, 0.0F, 1.0F);

        if (mTurnToFrontStep >= mTurnToFrontDuration) {
            mTurnToFrontStep = 0;
            mTurnToFrontDuration = 0;
        }

        const auto wrapped = MR::repeat(mRotation.y, -180.0F, 360.0F);
        mRotation.y = wrapped - (wrapped * progress);
        if (mBlinkCtrl != nullptr) {
            mBlinkCtrl->open();
            mBlinkCtrl->setNerve(&FileSelectItemSub::BlinkControllerNrvOpen::sInstance);
        }
        return;
    } else if (MR::isStarPointerInScreen(WPAD_CHAN0)) {
        const auto target = mPosition + TVec3f(0.0F, 900.0F, 0.0F);
        const auto* screen_position = MR::getStarPointerScreenPosition(WPAD_CHAN0);
        const auto current_screen = screen_position != nullptr ? *screen_position : TVec2f{};

        if (mNeedsPointerScreenReset) {
            mPreviousPointerScreen = current_screen;
            mNeedsPointerScreenReset = false;
        }

        const auto camera_pos = MR::getCamPos();
        const auto camera_to_target = target - camera_pos;
        const auto unprojection_distance = 900.0F + camera_to_target.length();

        if (std::sqrt(current_screen.squareDist(mPreviousPointerScreen)) < 2.0F) {
            auto current_world = TVec3f{};
            MR::calcWorldPositionFromScreen(&current_world, current_screen, unprojection_distance);
            auto camera_to_pointer = current_world - camera_pos;
            MR::normalize(&camera_to_pointer);
            if (cross_product(camera_to_target, camera_to_pointer).length() < 900.0F) {
                mPointerStrokeHit = true;
            }
        } else {
            auto current_world = TVec3f{};
            auto previous_world = TVec3f{};
            MR::calcWorldPositionFromScreen(&current_world, current_screen, unprojection_distance);
            MR::calcWorldPositionFromScreen(&previous_world, mPreviousPointerScreen, unprojection_distance);
            if (is_point_inside_pointer_stroke_triangle(target, camera_pos, current_world, previous_world, 900.0F)) {
                mPointerStrokeHit = true;
            }
        }

        if (mPointerStrokeHit) {
            auto velocity = mRotationVelocityY + ((0.03F * (current_screen.x - mPreviousPointerScreen.x)) / mScale.x);
            velocity = std::clamp(velocity, -25.0F, 25.0F);
            mRotationVelocityY = velocity;
        }

        mPreviousPointerScreen = current_screen;
    } else {
        mNeedsPointerScreenReset = true;
    }

    mRotationVelocityY *= 0.98F;
    if (mRotationVelocityY >= 0.0F && mRotationVelocityY < 0.5F) {
        mRotationVelocityY = 0.5F;
    } else if (mRotationVelocityY < 0.0F && mRotationVelocityY > -0.5F) {
        mRotationVelocityY = -0.5F;
    }
    mRotation.y = MR::repeat(mRotation.y + mRotationVelocityY, 0.0F, 360.0F);
    mPointerStrokeHit = false;
}

void FileSelectItem::updateModelMatrix() {
    constexpr auto cNewItemLocalYOffset = 900.0F;
    const auto scale = mScaleCtrl != nullptr ? mScaleCtrl->_8 : 1.0F;
    const auto yaw = mRotation.y * cDegreesToRadians;
    const auto cos_y = std::cos(yaw) * scale;
    const auto sin_y = std::sin(yaw) * scale;

    make_identity(mPlanetMatrix);
    mPlanetMatrix[0][0] = cos_y;
    mPlanetMatrix[0][2] = sin_y;
    mPlanetMatrix[1][1] = scale;
    mPlanetMatrix[2][0] = -sin_y;
    mPlanetMatrix[2][2] = cos_y;
    mPlanetMatrix[0][3] = mPosition.x;
    mPlanetMatrix[1][3] = mPosition.y + cNewItemLocalYOffset;
    mPlanetMatrix[2][3] = mPosition.z;

    make_identity(mFellowMatrix);
    mFellowMatrix[0][0] = cos_y;
    mFellowMatrix[0][2] = sin_y;
    mFellowMatrix[1][1] = scale;
    mFellowMatrix[2][0] = -sin_y;
    mFellowMatrix[2][2] = cos_y;
    mFellowMatrix[0][3] = mPosition.x;
    mFellowMatrix[1][3] = mPosition.y;
    mFellowMatrix[2][3] = mPosition.z;
}

namespace FileSelectItemSub {
    ScaleController::ScaleController() : NerveExecutor("ファイルセレクタアイコンサイズ管理") {
        initNerve(&ScaleControllerNrvSmall::sInstance);
    }

    void ScaleController::exeToSmall() {
        if (getNerveStep() <= 30) {
            const auto ratio = static_cast< f32 >(getNerveStep()) / 30.0F;
            _8 += ratio * (1.0F - _8);
        }

        if (getNerveStep() >= 30) {
            setNerve(&ScaleControllerNrvSmall::sInstance);
        }
    }

    void ScaleController::exeToBig() {
        if (getNerveStep() <= 30) {
            const auto ratio = static_cast< f32 >(getNerveStep()) / 30.0F;
            _8 += ratio * (1.2F - _8);
        }

        if (getNerveStep() >= 30) {
            setNerve(&ScaleControllerNrvBig::sInstance);
        }
    }

    void ScaleController::exeSmall() {
        _8 = 1.0F;
    }

    void ScaleController::exeBig() {
        _8 = 1.2F;
    }

    BlinkController::BlinkController(FileSelectItem* pItem) : NerveExecutor("ファイルセレクタアイコン瞬き管理"), mItem(pItem) {
        initNerve(&BlinkControllerNrvOpen::sInstance);
    }

    void BlinkController::exeOpen() {
        if (MR::isFirstStep(this)) {
            _C = MR::getRandom(180, 300);
            _10 = 0;
        }

        const auto velocity = std::fabs(mItem != nullptr ? mItem->mRotationVelocityY : 0.0F);
        if (velocity > 10.0F) {
            _10 += 2;
        } else if (velocity > 6.0F) {
            ++_10;
        } else {
            _10 = 0;
        }

        if (_10 > 180) {
            setNerve(&BlinkControllerNrvSleep::sInstance);
        } else if (getNerveStep() >= _C) {
            setNerve(&BlinkControllerNrvShut::sInstance);
        }
    }

    void BlinkController::exeShut() {
        if (MR::isFirstStep(this)) {
            shut();
        }

        if (getNerveStep() >= 10) {
            open();
            setNerve(&BlinkControllerNrvOpen::sInstance);
        }
    }

    void BlinkController::exeSleep() {
        if (MR::isFirstStep(this)) {
            sleep();
            _10 = 0;
        }

        ++_10;
        if (std::fabs(mItem != nullptr ? mItem->mRotationVelocityY : 0.0F) > 2.0F) {
            _10 = 0;
        }

        if (_10 > 60) {
            setNerve(&BlinkControllerNrvBlink::sInstance);
        }
    }

    void BlinkController::exeBlink() {
        if (MR::isFirstStep(this) && mItem != nullptr && !mItem->mIsNew) {
            const auto index = static_cast< std::size_t >(mItem->getFellowModelIndex());
            if (index < mItem->mModels.size() && mItem->mModels[index] != nullptr) {
                mItem->mModels[index]->blink();
            }
        }

        if (mItem != nullptr) {
            const auto index = static_cast< std::size_t >(mItem->getFellowModelIndex());
            if (index < mItem->mModels.size() && mItem->mModels[index] != nullptr && mItem->mModels[index]->isOpen()) {
                setNerve(&BlinkControllerNrvOpen::sInstance);
            }
        } else {
            setNerve(&BlinkControllerNrvOpen::sInstance);
        }
    }

    void BlinkController::shut() {
        if (mItem != nullptr && !mItem->mIsNew) {
            const auto index = static_cast< std::size_t >(mItem->getFellowModelIndex());
            if (index < mItem->mModels.size() && mItem->mModels[index] != nullptr) {
                mItem->mModels[index]->blinkOnce();
            }
        }
    }

    void BlinkController::open() {
        if (mItem != nullptr && !mItem->mIsNew) {
            const auto index = static_cast< std::size_t >(mItem->getFellowModelIndex());
            if (index < mItem->mModels.size() && mItem->mModels[index] != nullptr) {
                mItem->mModels[index]->open();
            }
        }
    }

    void BlinkController::sleep() {
        if (mItem != nullptr && !mItem->mIsNew) {
            const auto index = static_cast< std::size_t >(mItem->getFellowModelIndex());
            if (index < mItem->mModels.size() && mItem->mModels[index] != nullptr) {
                mItem->mModels[index]->close();
            }
        }
    }
}  // namespace FileSelectItemSub
