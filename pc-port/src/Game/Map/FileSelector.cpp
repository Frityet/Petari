#include "Game/Map/FileSelector.hpp"

#include "Game/LiveActor/Nerve.hpp"
#include "Game/Map/FileSelectCameraController.hpp"
#include "Game/Map/FileSelectItem.hpp"
#include "Game/Map/FileSelectSky.hpp"
#include "Game/NPC/MiiFacePartsHolder.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Screen/MiiSelect.hpp"
#include "Game/Screen/TitleSequenceProduct.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/NerveUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "Game/Util/StarPointerUtil.hpp"
#include "Game/compat/RuntimeContext.hpp"

#include <cmath>

namespace NrvFileSelector {
    NEW_NERVE(FileSelectorNrvWaitBind, FileSelector, WaitBind);
    NEW_NERVE(FileSelectorNrvTitle, FileSelector, Title);
    NEW_NERVE(FileSelectorNrvTitleEnd, FileSelector, TitleEnd);
    NEW_NERVE(FileSelectorNrvRFLError, FileSelector, RFLError);
    NEW_NERVE(FileSelectorNrvRFLWait, FileSelector, RFLWait);
    NEW_NERVE(FileSelectorNrvFileSelect, FileSelector, FileSelect);
};  // namespace NrvFileSelector

namespace {
    constexpr std::array< float, 6 > cItemThetaOffsetDegrees{10.0F, -10.0F, 0.0F, 0.0F, 0.0F, 0.0F};
    constexpr std::array< s32, 6 > cItemFileNoOrder{1, 2, 4, 6, 5, 3};
    constexpr auto cItemRingRadius = 5000.0F;
    constexpr auto cItemRingPitchRadians = 0.3490658700466156F;
    constexpr auto cItemAngleStepRadians = 1.0471975803375244F;
    constexpr auto cPi = 3.1415927410125732F;

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

    [[nodiscard]] smgpc::game::CameraParamVec3 file_select_item_base_position(s32 index) {
        const auto theta_offset = cItemThetaOffsetDegrees[static_cast< std::size_t >(index)] * cPi / 180.0F;
        const auto theta = -static_cast< float >(index + 4) * cItemAngleStepRadians - theta_offset;
        const auto x = cItemRingRadius * std::cos(theta);
        const auto z = cItemRingRadius * std::sin(theta);
        const auto pitch_sin = std::sin(cItemRingPitchRadians);
        const auto pitch_cos = std::cos(cItemRingPitchRadians);

        return smgpc::game::CameraParamVec3{
            .x = x,
            .y = -pitch_sin * z,
            .z = pitch_cos * z,
        };
    }
}  // namespace

FileSelector::FileSelector() : NerveExecutor("ファイルセレクタ") {
    initNerve(&NrvFileSelector::FileSelectorNrvWaitBind::sInstance);
    createCameraController();
    createTitle();
    createSky();
    createMiiSelect();
    createFileItems();
}

FileSelector::~FileSelector() = default;

void FileSelector::update() {
    if (mCameraController != nullptr) {
        mCameraController->update();
    }
    for (s32 i = 0; i < cItemCount; ++i) {
        if (mItems[static_cast< std::size_t >(i)] != nullptr) {
            mItems[static_cast< std::size_t >(i)]->update(mItemBasePositions[static_cast< std::size_t >(i)]);
        }
    }
    updateNerve();
}

void FileSelector::draw(smgpc::render::IRendererEngine& renderer) {
    if (mCameraController != nullptr) {
        const auto camera_pose = mCameraController->getCameraPose();
        if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
            runtime->draw_3d_normal(renderer, camera_pose);
        }
        for (auto& item : mItems) {
            if (item != nullptr) {
                item->draw(renderer, camera_pose);
            }
        }
    }
}

bool FileSelector::receiveOtherMsg(u32 msg, HitSensor*, HitSensor*) {
    if (MR::isMsgAutoRushBegin(msg) && isNerve(&NrvFileSelector::FileSelectorNrvWaitBind::sInstance)) {
        MR::hidePlayer();
        setNerve(&NrvFileSelector::FileSelectorNrvTitle::sInstance);
        return true;
    }

    if (MR::isMsgUpdateBaseMtx(msg)) {
        Mtx base_mtx{};
        make_identity(base_mtx);
        MR::setPlayerBaseMtx(base_mtx);
        return true;
    }

    return false;
}

void FileSelector::createCameraController() {
    mCameraController = std::make_unique< FileSelectCameraController >("ファイル選択カメラ");
}

void FileSelector::createTitle() {
    mTitleSeq = std::make_unique< TitleSequenceProduct >();
    mTitleSeq->kill();
}

void FileSelector::createSky() {
    mSky = std::make_unique< FileSelectSky >("ファイル選択空");
    mSky->initWithoutIter();
    mSky->appear();
}

void FileSelector::createMiiSelect() {
    mMiiSelect = std::make_unique< MiiSelect >("MiiSelect");
    mMiiSelect->initWithoutIter();
}

void FileSelector::createFileItems() {
    for (s32 i = 0; i < cItemCount; ++i) {
        mItems[static_cast< std::size_t >(i)] = std::make_unique< FileSelectItem >(cItemFileNoOrder[static_cast< std::size_t >(i)], true);
    }
    calcBasePos(0.0F);
}

void FileSelector::appearAllItems() {
    mAllItemsAppeared = true;
    for (auto& item : mItems) {
        item->appear();
    }
    if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
        runtime->note_debug_event("FileSelector appeared all file-select items");
    }
}

void FileSelector::initAllItems() {
    mAllItemsInitialized = true;
    for (auto& item : mItems) {
        item->forceChange(true);
    }
    if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
        runtime->note_debug_event("FileSelector initialized all file-select items");
    }
}

void FileSelector::invalidateSelectAll() {
    mSelectAllInvalidated = true;
    for (auto& item : mItems) {
        item->invalidateSelect();
    }
    if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
        runtime->note_debug_event("FileSelector invalidated all file-select item selection");
    }
}

void FileSelector::validateSelectAll() {
    mSelectAllInvalidated = false;
    for (auto& item : mItems) {
        item->validateSelect();
    }
}

void FileSelector::validateRotateAllItems() {
    mRotateAllItemsValidated = true;
    for (auto& item : mItems) {
        item->validateRotate();
    }
    if (auto* runtime = smgpc::game::RuntimeContext::try_instance()) {
        runtime->note_debug_event("FileSelector validated file-select item rotation");
    }
}

void FileSelector::calcBasePos(float ratio) {
    mBasePosRatio = ratio;
    for (s32 i = 0; i < cItemCount; ++i) {
        mItemBasePositions[static_cast< std::size_t >(i)] = file_select_item_base_position(i);
    }
}

void FileSelector::exeWaitBind() {
}

void FileSelector::exeTitle() {
    if (MR::isFirstStep(this)) {
        mTitleStarted = true;
        mTitleSeq->appear();
        MR::deactivateDefaultGameLayout();
        MR::startStarPointerModeTitle(this);
        MR::resetCameraMan();
    }

    mTitleSeq->updateNerve();

    if (!mTitleSeq->isActive()) {
        if (MR::getSceneObj< MiiFacePartsHolder >(SceneObj_MiiFacePartsHolder)->isInitEnd()) {
            if (MR::getSceneObj< MiiFacePartsHolder >(SceneObj_MiiFacePartsHolder)->isError()) {
                setNerve(&NrvFileSelector::FileSelectorNrvRFLError::sInstance);
            } else {
                setNerve(&NrvFileSelector::FileSelectorNrvTitleEnd::sInstance);
            }
        } else {
            setNerve(&NrvFileSelector::FileSelectorNrvRFLWait::sInstance);
        }
    }
}

void FileSelector::exeTitleEnd() {
    if (MR::isFirstStep(this)) {
        mTitleEnded = true;
        mCameraController->goToFarPoint();
        calcBasePos(0.0F);
        appearAllItems();
        initAllItems();
        mMiiSelect->collectValidMiiIndex();
        invalidateSelectAll();
        MR::startStarPointerModeFileSelect(this);
        MR::startStageBGM("MBGM_FILE_SELECT", false);
    }

    if (mCameraController->isAtFarPoint()) {
        validateRotateAllItems();
        setNerve(&NrvFileSelector::FileSelectorNrvFileSelect::sInstance);
    }
}

void FileSelector::exeRFLError() {
    mTitleEnded = true;
}

void FileSelector::exeRFLWait() {
    if (MR::getSceneObj< MiiFacePartsHolder >(SceneObj_MiiFacePartsHolder)->isInitEnd()) {
        setNerve(&NrvFileSelector::FileSelectorNrvTitleEnd::sInstance);
    }
}

void FileSelector::exeFileSelect() {
    if (MR::isFirstStep(this)) {
        mFileSelectStarted = true;
    }
}

std::uint64_t FileSelector::getSkyStep() const {
    return mSky != nullptr ? static_cast< std::uint64_t >(mSky->getNerveStep()) : 0U;
}

bool FileSelector::isTitleActive() const {
    return mTitleSeq != nullptr && mTitleSeq->isActive();
}

bool FileSelector::isTitleStarted() const {
    return mTitleStarted;
}

bool FileSelector::isTitleEnded() const {
    return mTitleEnded;
}

bool FileSelector::isCameraAtFarPoint() const {
    return mCameraController != nullptr && mCameraController->isAtFarPoint();
}

bool FileSelector::isFileSelectStarted() const {
    return mFileSelectStarted;
}

bool FileSelector::didAppearAllItems() const {
    return mAllItemsAppeared;
}

bool FileSelector::didInitAllItems() const {
    return mAllItemsInitialized;
}

bool FileSelector::didInvalidateSelectAll() const {
    return mSelectAllInvalidated;
}

bool FileSelector::didValidateRotateAllItems() const {
    return mRotateAllItemsValidated;
}

float FileSelector::getBasePosRatio() const {
    return mBasePosRatio;
}

s32 FileSelector::getMiiValidIndexCollectionCount() const {
    return mMiiSelect != nullptr ? mMiiSelect->getCollectValidMiiIndexCount() : 0;
}

s32 FileSelector::getItemCount() const {
    return cItemCount;
}

s32 FileSelector::getAppearedItemCount() const {
    auto count = 0;
    for (const auto& item : mItems) {
        if (item->isAppeared()) {
            ++count;
        }
    }

    return count;
}

s32 FileSelector::getSelectInvalidItemCount() const {
    auto count = 0;
    for (const auto& item : mItems) {
        if (item->isSelectInvalid()) {
            ++count;
        }
    }

    return count;
}

s32 FileSelector::getRotateInvalidItemCount() const {
    auto count = 0;
    for (const auto& item : mItems) {
        if (item->isRotateInvalid()) {
            ++count;
        }
    }

    return count;
}

s32 FileSelector::getItemFileNo(s32 index) const {
    return mItems.at(static_cast< std::size_t >(index))->getFileNo();
}

const smgpc::game::CameraParamVec3& FileSelector::getItemBasePosition(s32 index) const {
    return mItemBasePositions.at(static_cast< std::size_t >(index));
}

const smgpc::game::CameraParamVec3& FileSelector::getItemPosition(s32 index) const {
    return mItems.at(static_cast< std::size_t >(index))->getPosition();
}
