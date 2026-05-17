#include "Game/Map/FileSelector.hpp"

#include "Game/LiveActor/Nerve.hpp"
#include "Game/Map/FileSelectCameraController.hpp"
#include "Game/Map/FileSelectEffect.hpp"
#include "Game/Map/FileSelectFunc.hpp"
#include "Game/Map/FileSelectIconID.hpp"
#include "Game/Map/FileSelectItem.hpp"
#include "Game/Map/FileSelectItemDelegator.hpp"
#include "Game/Map/FileSelectSky.hpp"
#include "Game/NPC/MiiFacePartsHolder.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Screen/BackButton.hpp"
#include "Game/Screen/BrosButton.hpp"
#include "Game/Screen/FileSelectButton.hpp"
#include "Game/Screen/FileSelectInfo.hpp"
#include "Game/Screen/InformationMessage.hpp"
#include "Game/Screen/Manual2P.hpp"
#include "Game/Screen/MiiConfirmIcon.hpp"
#include "Game/Screen/MiiSelect.hpp"
#include "Game/Screen/SysInfoWindow.hpp"
#include "Game/Screen/TitleSequenceProduct.hpp"
#include "Game/System/GameSequenceFunction.hpp"
#include "Game/System/UserFile.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/GamePadUtil.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/NerveUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/SequenceUtil.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "Game/Util/StarPointerUtil.hpp"
#include "Game/Util/StringUtil.hpp"

#include <RVLFaceLib.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <ranges>
#include <string_view>

namespace NrvFileSelector {
    NEW_NERVE(FileSelectorNrvWaitBind, FileSelector, WaitBind);
    NEW_NERVE(FileSelectorNrvTitle, FileSelector, Title);
    NEW_NERVE(FileSelectorNrvTitleEnd, FileSelector, TitleEnd);
    NEW_NERVE(FileSelectorNrvRFLError, FileSelector, RFLError);
    NEW_NERVE(FileSelectorNrvRFLWait, FileSelector, RFLWait);
    NEW_NERVE(FileSelectorNrvFileSelectStart, FileSelector, FileSelectStart);
    NEW_NERVE(FileSelectorNrvFileSelect, FileSelector, FileSelect);
    NEW_NERVE(FileSelectorNrvFileConfirmStart, FileSelector, FileConfirmStart);
    NEW_NERVE(FileSelectorNrvFileConfirm, FileSelector, FileConfirm);
    NEW_NERVE(FileSelectorNrvCreateConfirmStart, FileSelector, CreateConfirmStart);
    NEW_NERVE(FileSelectorNrvCreateConfirm, FileSelector, CreateConfirm);
    NEW_NERVE(FileSelectorNrvCreate, FileSelector, Create);
    NEW_NERVE(FileSelectorNrvDemoStartWait, FileSelector, DemoStartWait);
    NEW_NERVE(FileSelectorNrvDemo, FileSelector, Demo);
    NEW_NERVE(FileSelectorNrvCopyWait, FileSelector, CopyWait);
    NEW_NERVE(FileSelectorNrvCopySelect, FileSelector, CopySelect);
    NEW_NERVE(FileSelectorNrvCopyConfirmStart, FileSelector, CopyConfirmStart);
    NEW_NERVE(FileSelectorNrvCopyConfirm, FileSelector, CopyConfirm);
    NEW_NERVE(FileSelectorNrvCopySave, FileSelector, CopySave);
    NEW_NERVE(FileSelectorNrvCopySaveMii, FileSelector, CopySaveMii);
    NEW_NERVE(FileSelectorNrvCopyDemo, FileSelector, CopyDemo);
    NEW_NERVE(FileSelectorNrvCopyRejectStart, FileSelector, CopyRejectStart);
    NEW_NERVE(FileSelectorNrvCopyReject, FileSelector, CopyReject);
    NEW_NERVE(FileSelectorNrvMiiWait, FileSelector, MiiWait);
    NEW_NERVE(FileSelectorNrvMiiSelectStartFirst, FileSelector, MiiSelectStart);
    NEW_NERVE(FileSelectorNrvMiiSelectStart, FileSelector, MiiSelectStart);
    NEW_NERVE(FileSelectorNrvMiiSelect, FileSelector, MiiSelect);
    NEW_NERVE(FileSelectorNrvMiiCancel, FileSelector, MiiCancel);
    NEW_NERVE(FileSelectorNrvMiiConfirmWait, FileSelector, MiiConfirmWait);
    NEW_NERVE(FileSelectorNrvMiiConfirm, FileSelector, MiiConfirm);
    NEW_NERVE(FileSelectorNrvMiiCreateWait, FileSelector, MiiCreateWait);
    NEW_NERVE(FileSelectorNrvMiiCreateDemo, FileSelector, MiiCreateDemo);
    NEW_NERVE(FileSelectorNrvMiiCaution, FileSelector, MiiCaution);
    NEW_NERVE(FileSelectorNrvMiiInfoStart, FileSelector, MiiInfoStart);
    NEW_NERVE(FileSelectorNrvMiiInfo, FileSelector, MiiInfo);
    NEW_NERVE(FileSelectorNrvDeleteConfirmStart, FileSelector, DeleteConfirmStart);
    NEW_NERVE(FileSelectorNrvDeleteConfirm, FileSelector, DeleteConfirm);
    NEW_NERVE(FileSelectorNrvDelete, FileSelector, Delete);
    NEW_NERVE(FileSelectorNrvDeleteDemo, FileSelector, DeleteDemo);
    NEW_NERVE(FileSelectorNrvManualStart, FileSelector, ManualStart);
    NEW_NERVE(FileSelectorNrvManual, FileSelector, Manual);
};  // namespace NrvFileSelector

namespace {
    constexpr std::array< float, 6 > cItemThetaOffsetDegrees{10.0F, -10.0F, 0.0F, 0.0F, 0.0F, 0.0F};
    constexpr std::array< s32, 6 > cItemFileNoOrder{1, 2, 4, 6, 5, 3};
    constexpr auto cItemRingRadius = 5000.0F;
    constexpr auto cItemRingPitchRadians = 0.3490658700466156F;
    constexpr auto cItemAngleStepRadians = 1.0471975803375244F;
    constexpr auto cPi = 3.1415927410125732F;
    constexpr auto cBgmFarState = 5;
    constexpr auto cBgmNearState = 6;
    constexpr auto cBgmStateChangeFrames = 0x3cU;
    constexpr std::array< const char*, 4 > cSelectedME{
        "ME_ASTRO_DOME_SELECT1",
        "ME_ASTRO_DOME_SELECT2",
        "ME_ASTRO_DOME_SELECT3",
        "ME_ASTRO_DOME_SELECT4",
    };
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

    [[nodiscard]] std::size_t item_array_index_by_file_no(s32 file_no) {
        for (auto i = std::size_t{}; i < cItemFileNoOrder.size(); ++i) {
            if (cItemFileNoOrder[i] == file_no) {
                return i;
            }
        }

        return cItemFileNoOrder.size();
    }

    [[nodiscard]] FileSelectIconID file_select_icon_id_from_user_file(const UserFile* pFile) {
        auto icon_id = FileSelectIconID();
        if (pFile == nullptr) {
            return icon_id;
        }

        auto save_icon_id = u32{};
        if (pFile->getIconId(&save_icon_id)) {
            save_icon_id = std::clamp(save_icon_id, 1U, 5U);
            icon_id.setFellowID(static_cast< FileSelectIconID::EFellowID >(save_icon_id - 1U));
            return icon_id;
        }

        auto mii_id = std::array< u8, 16U >{};
        if (pFile->getMiiId(mii_id.data())) {
            auto mii_index = s32{};
            std::memcpy(&mii_index, mii_id.data(), std::min(sizeof(mii_index), mii_id.size()));
            icon_id.setMiiIndex(static_cast< u16 >(std::clamp(mii_index, 0, 0xffff)));
        }
        return icon_id;
    }

    [[nodiscard]] std::array< u16, 11U > icon_name_utf16_u16(const FileSelectIconID& rIconId) {
        auto name = std::array< u16, 11U >{};
        FileSelectFunc::copyMiiName(name.data(), rIconId);
        return name;
    }

    [[nodiscard]] std::array< wchar_t, 11U > icon_name_wide(const FileSelectIconID& rIconId) {
        const auto utf16_name = icon_name_utf16_u16(rIconId);
        auto name = std::array< wchar_t, 11U >{};
        for (auto i = std::size_t{}; i + 1U < name.size() && utf16_name[i] != 0U; ++i) {
            name[i] = static_cast< wchar_t >(utf16_name[i]);
        }
        return name;
    }
}  // namespace

FileSelector::FileSelector(const char* pName) : LiveActor(pName) {
}

FileSelector::~FileSelector() = default;

void FileSelector::init(const JMapInfoIter&) {
    MR::connectToScene(this, MR::MovementType_Environment, -1, -1, -1);
    initNerve(&NrvFileSelector::FileSelectorNrvWaitBind::sInstance);
    initUserFileArray();
    initUserFile();
    createCameraController();
    createSky();
    createFileItems();
    createOperationButton();
    createBackButton();
    createBrosButton();
    createInfoMessage();
    createSysInfoWindow();
    createFileInfo();
    createTitle();
    createMiiSelect();
    createMiiConfirmIcon();
    createManual();
    createSelectEffect();
    MR::invalidateClipping(this);
    appear();
}

void FileSelector::appear() {
    LiveActor::appear();
    if (mCameraController != nullptr) {
        mCameraController->appear();
    }
    setNerve(&NrvFileSelector::FileSelectorNrvWaitBind::sInstance);
}

void FileSelector::kill() {
    LiveActor::kill();
}

void FileSelector::control() {
    mPointingItem = nullptr;
    updateBgm();
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
    mCameraController->initWithoutIter();
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

void FileSelector::createOperationButton() {
    mSelectButton = std::make_unique< FileSelectButton >("ファイル選択ボタン");
    mSelectButton->initWithoutIter();
    mSelectButton->setCallbackFunctor(MR::Functor(this, &FileSelector::callbackStart), MR::Functor(this, &FileSelector::callbackCopy),
                                      MR::Functor(this, &FileSelector::callbackMii), MR::Functor(this, &FileSelector::callbackDelete),
                                      MR::Functor(this, &FileSelector::callbackManual));
}

void FileSelector::createBackButton() {
    mBackButton = std::make_unique< BackButton >("戻るボタン", false);
    mBackButton->initWithoutIter();
    MR::connectToScene(mBackButton.get(), MR::MovementType_Layout, MR::CalcAnimType_Layout, -1, MR::DrawType_LayoutDecoration);
}

void FileSelector::createBrosButton() {
    mBrosButton = std::make_unique< BrosButton >("ルイージ切り替えボタン");
    mBrosButton->initWithoutIter();
}

void FileSelector::createInfoMessage() {
    mInfoMessage = std::make_unique< InformationMessage >();
    mInfoMessage->initWithoutIter();
}

void FileSelector::createSysInfoWindow() {
    mSysInfoWindow.reset(MR::createSysInfoWindow());
    MR::connectToSceneLayout(mSysInfoWindow.get());
    mSysInfoWindowMini.reset(MR::createSysInfoWindowMiniExecuteWithChildren());
    MR::connectToSceneLayout(mSysInfoWindowMini.get());
}

void FileSelector::createFileInfo() {
    mSelectInfo = std::make_unique< FileSelectInfo >(11, "ファイル情報");
    mSelectInfo->initWithoutIter();
}

void FileSelector::createMiiConfirmIcon() {
    mMiiConfirmIcon = std::make_unique< MiiConfirmIcon >("Mii確認用アイコン");
    mMiiConfirmIcon->initWithoutIter();
    MR::connectToScene(mMiiConfirmIcon.get(), MR::MovementType_Layout, MR::CalcAnimType_Layout, -1, MR::DrawType_LayoutDecoration);
}

void FileSelector::createManual() {
    mManual2P = std::make_unique< Manual2P >("２Ｐマニュアル");
    mManual2P->initWithoutIter();
}

void FileSelector::createSelectEffect() {
    for (s32 i = 0; i < cItemCount; ++i) {
        auto& effect = mSelectEffects[static_cast< std::size_t >(i)];
        effect = std::make_unique< FileSelectEffect >("選択時エフェクト");
        effect->initWithoutIter();
        const auto& base = mItemBasePositions[static_cast< std::size_t >(i)];
        effect->mPosition.x = base.x;
        effect->mPosition.y = base.y;
        effect->mPosition.z = base.z;
        effect->mScale.x = 0.4F;
        effect->mScale.y = 0.4F;
        effect->mScale.z = 0.4F;
    }
}

void FileSelector::createFileItems() {
    mItemDelegator = std::make_unique< FileSelectItemDelegator< FileSelector > >(this, &FileSelector::notifyItem);

    for (s32 i = 0; i < cItemCount; ++i) {
        const auto file_no = cItemFileNoOrder[static_cast< std::size_t >(i)];
        const auto file_index = static_cast< std::size_t >(file_no - 1);
        const auto* file = file_index < mFiles.size() ? mFiles[file_index].get() : nullptr;
        const auto is_new = file_index >= mFiles.size() || file == nullptr || !file->isCreated() || isUserFileCorrupted(file_no);
        auto icon_id = FileSelectIconID();
        getIconId(&icon_id, file_no);
        mItems[static_cast< std::size_t >(i)] = std::make_unique< FileSelectItem >(file_no, is_new, icon_id);
        mItems[static_cast< std::size_t >(i)]->initWithoutIter();
        mItems[static_cast< std::size_t >(i)]->setSelectDelegator(mItemDelegator.get());
    }
    calcBasePos(0.0F);
}

void FileSelector::initUserFileArray() {
    for (auto& file : mFiles) {
        file = std::make_unique< UserFile >();
    }
}

void FileSelector::initUserFile() {
    for (s32 i = 0; i < cItemCount; ++i) {
        GameSequenceFunction::restoreUserFile(mFiles[static_cast< std::size_t >(i)].get(), i + 1);
    }

    checkAllComplete();
}

void FileSelector::restoreUserFile() {
    for (s32 i = 0; i < cItemCount; ++i) {
        const auto file = mFiles[static_cast< std::size_t >(i)].get();
        GameSequenceFunction::restoreUserFile(file, i + 1, file == nullptr || file->mIsPlayerMario);
    }

    checkAllComplete();
}

void FileSelector::checkAllComplete() {
    for (s32 i = 0; i < cItemCount; ++i) {
        const auto file = mFiles[static_cast< std::size_t >(i)].get();
        mAllComplete[static_cast< std::size_t >(i)] = false;
        if (file == nullptr) {
            continue;
        }

        const auto is_mario = file->mIsPlayerMario;

        if (!is_mario) {
            GameSequenceFunction::restoreUserFile(file, i + 1, true);
        }

        if (file->isPowerStarGetFinalChallengeGalaxy()) {
            GameSequenceFunction::restoreUserFile(file, i + 1, false);
            if (file->isPowerStarGetFinalChallengeGalaxy()) {
                mAllComplete[static_cast< std::size_t >(i)] = true;
            }
        }

        if (is_mario != file->mIsPlayerMario) {
            GameSequenceFunction::restoreUserFile(file, i + 1, is_mario);
        }
    }
}

void FileSelector::callbackStart() {
#ifndef NDEBUG
    mStartCallbackCalled = true;
#endif
    setNerve(&NrvFileSelector::FileSelectorNrvDemoStartWait::sInstance);
}

void FileSelector::callbackCopy() {
#ifndef NDEBUG
    mCopyCallbackCalled = true;
#endif
    setNerve(&NrvFileSelector::FileSelectorNrvCopyWait::sInstance);
}

void FileSelector::callbackMii() {
#ifndef NDEBUG
    mMiiCallbackCalled = true;
#endif
    disappearAllLayout();
    invalidateSelectAll();
    setNerve(&NrvFileSelector::FileSelectorNrvMiiWait::sInstance);
}

void FileSelector::callbackDelete() {
#ifndef NDEBUG
    mDeleteCallbackCalled = true;
#endif
    setNerve(&NrvFileSelector::FileSelectorNrvDeleteConfirmStart::sInstance);
}

void FileSelector::callbackManual() {
#ifndef NDEBUG
    mManualCallbackCalled = true;
#endif
    MR::startSystemSE("SE_SY_FILE_SEL_TIPS_OPEN", -1, -1);
    setNerve(&NrvFileSelector::FileSelectorNrvManualStart::sInstance);
}

void FileSelector::setFileInfo(s32 fileNo) {
    if (mSelectInfo == nullptr || fileNo < 1 || fileNo > cItemCount) {
        return;
    }

    mSelectedFileNo = fileNo;
    auto* file = mFiles[static_cast< std::size_t >(fileNo - 1)].get();
    auto icon_id = FileSelectIconID();
    getIconId(&icon_id, fileNo);
    auto name = icon_name_utf16_u16(icon_id);
    auto calendar = OSCalendarTime{};
    if (file != nullptr) {
        OSTicksToCalendarTime(file->getLastModified(), &calendar);
    }
    auto date_message = std::array< wchar_t, 32U >{};
    auto time_message = std::array< wchar_t, 32U >{};
    MR::makeDateString(date_message.data(), static_cast< s32 >(date_message.size()), calendar.year, calendar.mon + 1, calendar.mday);
    MR::makeTimeString(time_message.data(), static_cast< s32 >(time_message.size()), calendar.hour, calendar.min);
    const auto is_mario = file == nullptr || file->mIsPlayerMario;
    mSelectInfo->setInfo(name.data(), fileNo, file != nullptr ? file->getPowerStarNum() : 0, file != nullptr ? file->getStarPieceNum() : 0, is_mario,
                         file != nullptr && file->isViewNormalEnding(), file != nullptr && file->isViewCompleteEnding(), date_message.data(),
                         time_message.data(), getMissCount(fileNo));
}

bool FileSelector::isUserFileCorrupted(s32 fileNo) const {
    if (fileNo < 1 || fileNo > cItemCount) {
        return false;
    }

    const auto& file = mFiles[static_cast< std::size_t >(fileNo - 1)];
    return file != nullptr && (file->mIsGameDataCorrupted || file->mIsConfigDataCorrupted);
}

bool FileSelector::isUserFileAppearLuigi(s32 fileNo) const {
    if (fileNo < 1 || fileNo > cItemCount) {
        return false;
    }

    const auto& file = mFiles[static_cast< std::size_t >(fileNo - 1)];
    return file != nullptr && (!file->mIsPlayerMario || file->isViewCompleteEnding());
}

bool FileSelector::isUserFileLuigi(s32 fileNo) const {
    if (fileNo < 1 || fileNo > cItemCount) {
        return false;
    }

    const auto& file = mFiles[static_cast< std::size_t >(fileNo - 1)];
    return file != nullptr && !file->mIsPlayerMario;
}

s32 FileSelector::getMissCount(s32 fileNo) const {
    if (fileNo < 1 || fileNo > cItemCount || !mAllComplete[static_cast< std::size_t >(fileNo - 1)]) {
        return -1;
    }

    const auto& file = mFiles[static_cast< std::size_t >(fileNo - 1)];
    return file != nullptr ? file->getPlayerMissNum() : -1;
}

void FileSelector::setUserFileMario(s32 fileNo, bool isMario) {
    if (fileNo < 1 || fileNo > cItemCount) {
        return;
    }

    if (auto& file = mFiles[static_cast< std::size_t >(fileNo - 1)]; file != nullptr) {
        file->mIsPlayerMario = isMario;
        file->setLastLoadedMario(isMario);
    }
}

FileSelectIconID::EFellowID FileSelector::getUserFileFellowID(s32 fileNo) const {
    if (fileNo < 1 || fileNo > cItemCount) {
        return FileSelectIconID::Mario;
    }

    const auto& file = mFiles[static_cast< std::size_t >(fileNo - 1)];
    auto icon_id = u32{};
    if (file == nullptr || !file->getIconId(&icon_id)) {
        return FileSelectIconID::Mario;
    }

    icon_id = std::clamp(icon_id, 1U, 5U);
    return static_cast< FileSelectIconID::EFellowID >(icon_id - 1U);
}

bool FileSelector::isUserFileMiiIdValid(s32 fileNo) const {
    if (fileNo < 1 || fileNo > cItemCount) {
        return false;
    }

    const auto& file = mFiles[static_cast< std::size_t >(fileNo - 1)];
    if (file == nullptr) {
        return false;
    }

    auto mii_id = std::array< u8, 16U >{};
    if (!file->getMiiId(mii_id.data())) {
        return false;
    }

    const auto* create_id = reinterpret_cast< const RFLCreateID* >(mii_id.data());
    auto index = u16{};
    return RFLSearchOfficialData(create_id, &index) == TRUE && RFLIsAvailableOfficialData(index) == TRUE;
}

s32 FileSelector::getUserFileMiiIndex(s32 fileNo) const {
    if (fileNo < 1 || fileNo > cItemCount) {
        return 0;
    }

    const auto& file = mFiles[static_cast< std::size_t >(fileNo - 1)];
    if (file == nullptr) {
        return 0;
    }

    auto mii_id = std::array< u8, 16U >{};
    if (!file->getMiiId(mii_id.data())) {
        return 0;
    }

    const auto* create_id = reinterpret_cast< const RFLCreateID* >(mii_id.data());
    auto index = u16{};
    return RFLSearchOfficialData(create_id, &index) == TRUE ? index : 0;
}

void FileSelector::storeSetMiiIdUserFile(s32 fileNo, const FileSelectIconID& rIconId) {
    if (rIconId.isFellow()) {
        auto icon_id = static_cast< u32 >(rIconId.getFellowID()) + 1U;
        GameSequenceFunction::startSetMiiOrIconIdUserFileSequence(fileNo, nullptr, &icon_id);
        return;
    }

    auto mii_id = std::array< u8, 16U >{};
    auto mii_index = static_cast< s32 >(rIconId.getMiiIndex());
    std::memcpy(mii_id.data(), &mii_index, std::min(sizeof(mii_index), mii_id.size()));
    GameSequenceFunction::startSetMiiOrIconIdUserFileSequence(fileNo, mii_id.data(), nullptr);
}

void FileSelector::getIconId(FileSelectIconID* pIconId, s32 fileNo) const {
    if (pIconId == nullptr) {
        return;
    }

    if (fileNo < 1 || fileNo > cItemCount) {
        pIconId->setFellowID(FileSelectIconID::Mario);
        return;
    }

    const auto& file = mFiles[static_cast< std::size_t >(fileNo - 1)];
    pIconId->set(file_select_icon_id_from_user_file(file.get()));
    if (pIconId->isMii() && !isUserFileMiiIdValid(fileNo)) {
        pIconId->setFellowID(FileSelectIconID::Mario);
    }
}

void FileSelector::playSelectedME() {
    const auto index = static_cast< std::size_t >(MR::getRandom(0, static_cast< s32 >(cSelectedME.size())));
    if (index < cSelectedME.size()) {
        MR::startSystemME(cSelectedME[index]);
    }
}

void FileSelector::updateBgm() {
    auto state = 0;
    auto change_frames = 0U;
    if (mCameraController != nullptr && mCameraController->isToOrAtFarPoint()) {
        state = cBgmFarState;
        change_frames = cBgmStateChangeFrames;
    } else if (mCameraController != nullptr && mCameraController->isToOrAtNearPoint()) {
        state = cBgmNearState;
        change_frames = cBgmStateChangeFrames;
    }

    if (mStageBgmState != state) {
        MR::setStageBGMState(state, change_frames);
        mStageBgmState = state;
    }
}

void FileSelector::notifyItem(FileSelectItem* pItem, s32 action) {
    if (action == 0) {
        onPoint(pItem);
    } else if (action == 1) {
        onSelect(pItem);
    } else if (action == 2 && isNerve(&NrvFileSelector::FileSelectorNrvCopySelect::sInstance)) {
        onPoint(pItem);
    }
}

void FileSelector::onPoint(FileSelectItem* pItem) {
    if (pItem == nullptr || pItem->isSelectInvalid()) {
        return;
    }

    if (mBackButton != nullptr && !mBackButton->isHidden() && mBackButton->isPointing()) {
        return;
    }

    if (mPointingItem == nullptr || mPointingItem->getFileNo() > pItem->getFileNo()) {
        mPointingItem = pItem;
    }
}

void FileSelector::onSelect(FileSelectItem* pItem) {
    if (pItem == nullptr || pItem->isSelectInvalid()) {
        return;
    }

    auto* next_nerve = static_cast< const Nerve* >(nullptr);
    if (isNerve(&NrvFileSelector::FileSelectorNrvCopySelect::sInstance)) {
        next_nerve = &NrvFileSelector::FileSelectorNrvCopyConfirmStart::sInstance;
    } else if (isNerve(&NrvFileSelector::FileSelectorNrvFileSelect::sInstance)) {
        next_nerve = pItem->isNew() ? static_cast< const Nerve* >(&NrvFileSelector::FileSelectorNrvCreateConfirmStart::sInstance) :
                                      static_cast< const Nerve* >(&NrvFileSelector::FileSelectorNrvFileConfirmStart::sInstance);
    }

    if (next_nerve == nullptr) {
        return;
    }

    mSelectedItem = pItem;
    mSelectedFileNo = pItem->getFileNo();
    setNerve(next_nerve);
    MR::startSystemSE("SE_SY_GALAXY_SELECTED", -1, -1);
    playSelectedME();
}

void FileSelector::updateFileInfo() {
    if (mPointingItem != nullptr && mPreviousPointingItem != mPointingItem) {
        if (mPreviousPointingItem != nullptr) {
            mPreviousPointingItem->clearPointing();
            if (mSelectInfo != nullptr) {
                mSelectInfo->disappear();
            }
        }

        if (mPointingItem->isExist()) {
            setFileInfo(mPointingItem->getFileNo());
            if (mSelectInfo != nullptr) {
                mSelectInfo->appear();
                mSelectInfo->forceChange();
            }
        }

        mPreviousPointingItem = mPointingItem;
        mPointingItem->onPointing();
        return;
    }

    if (mPointingItem == nullptr && mPreviousPointingItem != nullptr) {
        mPreviousPointingItem->clearPointing();
        if (mSelectInfo != nullptr) {
            mSelectInfo->disappear();
        }
        mPreviousPointingItem = nullptr;
    }
}

void FileSelector::appearAllItems() {
#ifndef NDEBUG
    mAllItemsAppeared = true;
#endif
    for (auto& item : mItems) {
        item->appear();
    }
}

void FileSelector::appearAllIndex() {
#ifndef NDEBUG
    mAllIndexAppeared = true;
#endif
    for (auto& item : mItems) {
        item->appearIndex();
    }
}

void FileSelector::disappearAllIndex() {
#ifndef NDEBUG
    mAllIndexDisappeared = true;
#endif
    for (auto& item : mItems) {
        item->disappearIndex();
    }
}

void FileSelector::initAllItems() {
#ifndef NDEBUG
    mAllItemsInitialized = true;
#endif
    for (s32 i = 0; i < cItemCount; ++i) {
        auto* item = mItems[static_cast< std::size_t >(i)].get();
        const auto file_no = item != nullptr ? item->getFileNo() : i + 1;
        auto icon_id = FileSelectIconID();
        getIconId(&icon_id, file_no);
        if (item != nullptr) {
            item->forceChange(item->isNew(), icon_id);
        }
    }
}

void FileSelector::invalidateSelectAll() {
#ifndef NDEBUG
    mSelectAllInvalidated = true;
    mSelectAllInvalidatedOnce = true;
#endif
    for (auto& item : mItems) {
        item->invalidateSelect();
    }
}

void FileSelector::validateSelectAll() {
#ifndef NDEBUG
    mSelectAllInvalidated = false;
#endif
    for (auto& item : mItems) {
        item->validateSelect();
    }
}

void FileSelector::validateRotateAllItems() {
#ifndef NDEBUG
    mRotateAllItemsValidated = true;
#endif
    for (auto& item : mItems) {
        item->validateRotate();
    }
}

void FileSelector::disappearAllLayout() {
    if (!MR::isDead(mSelectButton.get())) {
        mSelectButton->disappear();
        if (!MR::isDead(mSelectInfo.get())) {
            mSelectInfo->slideBack();
        }
    }

    if (!MR::isDead(mSelectInfo.get())) {
        mSelectInfo->disappear();
    }

    if (mBackButton != nullptr && !mBackButton->isHidden() && !mBackButton->_24) {
        mBackButton->disappear();
    }

    if (!MR::isDead(mBrosButton.get())) {
        mBrosButton->disappear();
    }
}

bool FileSelector::isHiddenAllLayout() const {
    return MR::isDead(mSelectButton.get()) && MR::isDead(mSelectInfo.get()) && (mBackButton == nullptr || mBackButton->isHidden()) &&
           MR::isDead(mBrosButton.get());
}

void FileSelector::clearPointing() {
#ifndef NDEBUG
    mPointingCleared = true;
#endif
    if (mPreviousPointingItem != nullptr) {
        mPreviousPointingItem->clearPointing();
    }
    if (mPointingItem != nullptr && mPointingItem != mPreviousPointingItem) {
        mPointingItem->clearPointing();
    }
    mPointingItem = nullptr;
    mPreviousPointingItem = nullptr;
}

bool FileSelector::checkSelectedBackButton() {
    if (mBackButton == nullptr || mBackButton->isHidden()) {
        return false;
    }

    if (mBackButton->_24) {
        return true;
    }

    if (MR::testSystemTriggerB()) {
        MR::startSystemSE("SE_SY_GALAXY_DECIDE_CANCEL", -1, -1);
        mBackButton->disappear();
        return true;
    }

    return false;
}

void FileSelector::calcBasePos(float ratio) {
    mBasePosRatio = ratio;
    for (s32 i = 0; i < cItemCount; ++i) {
        mItemBasePositions[static_cast< std::size_t >(i)] = file_select_item_base_position(i);
        if (mItems[static_cast< std::size_t >(i)] != nullptr) {
            mItems[static_cast< std::size_t >(i)]->setBasePosition(mItemBasePositions[static_cast< std::size_t >(i)]);
        }
    }
}

void FileSelector::goToNearPoint() {
    if (mCameraController == nullptr) {
        return;
    }

    calcBasePos(1.0F);
    const auto* item = mSelectedItem != nullptr ? mSelectedItem : getItemByFileNo(mSelectedFileNo);
    const auto base_position = item != nullptr ? item->getBasePosition() : mItemBasePositions.front();
    mCameraController->goToNearPoint(TVec3f{base_position.x, base_position.y, base_position.z});
}

void FileSelector::exeWaitBind() {
}

void FileSelector::exeTitle() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mTitleStarted = true;
#endif
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
#ifndef NDEBUG
        mTitleEnded = true;
#endif
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
#ifndef NDEBUG
    mTitleEnded = true;
#endif
}

void FileSelector::exeRFLWait() {
    if (MR::getSceneObj< MiiFacePartsHolder >(SceneObj_MiiFacePartsHolder)->isInitEnd()) {
        setNerve(&NrvFileSelector::FileSelectorNrvTitleEnd::sInstance);
    }
}

void FileSelector::exeFileSelectStart() {
    if (MR::isFirstStep(this)) {
        mCameraController->goToFarPoint();
        calcBasePos(0.0F);
    }

    if (mCameraController->isAtFarPoint()) {
        setNerve(&NrvFileSelector::FileSelectorNrvFileSelect::sInstance);
    }
}

void FileSelector::exeFileSelect() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mFileSelectStarted = true;
#endif
        validateSelectAll();
        appearAllIndex();
        MR::activeStarPointerGuidance();
    } else {
        updateFileInfo();
    }

    MR::requestFileSelectGuidance();
}

void FileSelector::exeFileConfirmStart() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mFileConfirmStartStarted = true;
#endif
        goToNearPoint();
        invalidateSelectAll();
        disappearAllIndex();
        if (mSelectedItem != nullptr) {
            mSelectedItem->turnToFront(0x28);
        }
    }

    if (mCameraController != nullptr && mCameraController->isAtNearPoint()) {
        setNerve(&NrvFileSelector::FileSelectorNrvFileConfirm::sInstance);
    }
}

void FileSelector::exeFileConfirm() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mFileConfirmStarted = true;
#endif
        if (mSelectButton != nullptr) {
            mSelectButton->appear();
        }
        if (mBackButton != nullptr) {
            mBackButton->appear();
        }
        setFileInfo(mSelectedFileNo);
        if (mSelectInfo != nullptr) {
            mSelectInfo->appear();
            mSelectInfo->forceChange();
            mSelectInfo->slide();
        }
        if (mBrosButton != nullptr && isUserFileAppearLuigi(mSelectedFileNo)) {
            mBrosButton->appear(!isUserFileLuigi(mSelectedFileNo));
        }
    }

    if (mBrosButton != nullptr && mBrosButton->isSelected()) {
        const auto file_no = mSelectedFileNo >= 1 && mSelectedFileNo <= cItemCount ? mSelectedFileNo : 1;
        setUserFileMario(file_no, mBrosButton->isSelectedMario());
        restoreUserFile();
        setFileInfo(file_no);
        if (mSelectInfo != nullptr) {
            mSelectInfo->change();
        }
        mBrosButton->resume();
    }

    if (checkSelectedBackButton()) {
        disappearAllLayout();
        clearPointing();
        setNerve(&NrvFileSelector::FileSelectorNrvFileSelectStart::sInstance);
    }
}

void FileSelector::exeCreateConfirmStart() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mCreateConfirmStartStarted = true;
#endif
        goToNearPoint();
        invalidateSelectAll();
        disappearAllIndex();
        if (mSelectedItem != nullptr) {
            mSelectedItem->turnToFront(0x28);
        }
    }

    if (mCameraController != nullptr && mCameraController->isAtNearPoint()) {
        setNerve(&NrvFileSelector::FileSelectorNrvCreateConfirm::sInstance);
    }
}

void FileSelector::exeCreateConfirm() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mCreateConfirmStarted = true;
#endif
        mSysInfoWindow->appear("System_FileSelect001", SysInfoWindow::Type_YesNo, SysInfoWindow::TextPos_Center, SysInfoWindow::MessageType_System);
        mSysInfoWindow->setYesNoSelectorSE("SE_SY_BUTTON_CURSOR_ON", "SE_SY_FILE_SEL_NEW_FILE", "SE_SY_TALK_SELECT_NO");
    }

    if (MR::isDead(mSysInfoWindow.get())) {
        mSysInfoWindow->resetYesNoSelectorSE();
        if (mSysInfoWindow->isSelectedYes()) {
            setNerve(&NrvFileSelector::FileSelectorNrvCreate::sInstance);
        } else {
            clearPointing();
            setNerve(&NrvFileSelector::FileSelectorNrvFileSelectStart::sInstance);
        }
    }
}

void FileSelector::exeCreate() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mCreateStarted = true;
#endif
        const auto file_no = mSelectedItem != nullptr ? mSelectedItem->getFileNo() : mSelectedFileNo;
        GameSequenceFunction::startCreateUserFileSequence(file_no);
    }

    if (!GameSequenceFunction::isActiveSaveDataHandleSequence()) {
        if (GameSequenceFunction::isSuccessSaveDataHandleSequence()) {
            restoreUserFile();
            setNerve(&NrvFileSelector::FileSelectorNrvMiiSelectStartFirst::sInstance);
        } else {
            clearPointing();
            setNerve(&NrvFileSelector::FileSelectorNrvFileSelectStart::sInstance);
        }
    }
}

void FileSelector::exeDemoStartWait() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mDemoStartWaitStarted = true;
#endif
        MR::startSystemSE("SE_SY_FILE_SELECTED", -1, -1);
        MR::stopStageBGM(0x5a);
        disappearAllLayout();
        MR::closeWipeFade(0x3c);
    }

    if (MR::isWipeBlank()) {
        setNerve(&NrvFileSelector::FileSelectorNrvDemo::sInstance);
    }
}

void FileSelector::exeDemo() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mDemoStarted = true;
#endif
        const auto file_no = mSelectedFileNo >= 1 && mSelectedFileNo <= cItemCount ? mSelectedFileNo : 1;
        const auto* file = mFiles[static_cast< std::size_t >(file_no - 1)].get();
        const auto is_luigi = file != nullptr && !file->mIsPlayerMario;
        GameSequenceFunction::startGameDataLoadSequence(file_no, !is_luigi);
        MR::stopStageBGM(0x5a);
    }

    if (!GameSequenceFunction::isActiveSaveDataHandleSequence()) {
        const auto file_no = mSelectedFileNo >= 1 && mSelectedFileNo <= cItemCount ? mSelectedFileNo : 1;
        const auto* file = mFiles[static_cast< std::size_t >(file_no - 1)].get();
        const auto is_luigi = file != nullptr && !file->mIsPlayerMario;
        auto name = std::array< wchar_t, 11U >{};
        const auto fallback = is_luigi ? std::wstring_view{L"Luigi"} : std::wstring_view{L"Mario"};
        for (auto i = std::size_t{}; i < fallback.size() && i + 1U < name.size(); ++i) {
            name[i] = fallback[i];
        }
        GameSequenceFunction::reserveUserName(name.data());
        MR::requestChangeStageInGameAfterLoadingGameData();
    }
}

void FileSelector::exeCopyWait() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mCopyWaitStarted = true;
#endif
        mCopySourceItem = mSelectedItem;
        mCopySourceFileNo = mSelectedItem != nullptr ? mSelectedItem->getFileNo() : mSelectedFileNo;
        MR::startSystemSE("SE_SY_FILE_SEL_UPPER_DECIDE", -1, -1);
        disappearAllLayout();
        clearPointing();
        if (mCameraController != nullptr) {
            mCameraController->goToFarPoint();
        }
        calcBasePos(0.0F);
    }

    if (mCameraController != nullptr && mCameraController->isAtFarPoint()) {
        setNerve(&NrvFileSelector::FileSelectorNrvCopySelect::sInstance);
    }
}

void FileSelector::exeCopySelect() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mCopySelectStarted = true;
#endif
        validateSelectAll();
        if (mCopySourceItem != nullptr) {
            mCopySourceItem->invalidateSelect();
        }
        if (mBackButton != nullptr) {
            mBackButton->appear();
        }
        appearAllIndex();
        MR::activeStarPointerGuidance();
        if (mCopySourceItem != nullptr) {
            const auto index = item_array_index_by_file_no(mCopySourceItem->getFileNo());
            if (index < mSelectEffects.size() && mSelectEffects[index] != nullptr) {
                mSelectEffects[index]->appear();
            }
        }
    }

    MR::requestFileSelectCopyGuidance();
    updateFileInfo();

    if (mBackButton != nullptr && mBackButton->isPointing()) {
        clearPointing();
        if (mSelectInfo != nullptr) {
            mSelectInfo->disappear();
        }
    }

    if (checkSelectedBackButton()) {
        invalidateSelectAll();
        if (mCopySourceItem != nullptr) {
            const auto index = item_array_index_by_file_no(mCopySourceItem->getFileNo());
            if (index < mSelectEffects.size() && mSelectEffects[index] != nullptr) {
                mSelectEffects[index]->disappear();
            }
            mSelectedItem = mCopySourceItem;
            mSelectedFileNo = mCopySourceFileNo;
        }
        if (mInfoMessage != nullptr) {
            mInfoMessage->disappear();
        }
        MR::deactiveStarPointerGuidance();
        setFileInfo(mSelectedFileNo);
        if (mSelectInfo != nullptr) {
            mSelectInfo->appear();
            mSelectInfo->forceChange();
        }
        setNerve(&NrvFileSelector::FileSelectorNrvFileConfirmStart::sInstance);
    }
}

void FileSelector::exeCopyConfirmStart() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mCopyConfirmStartStarted = true;
#endif
        disappearAllLayout();
        invalidateSelectAll();
        if (mCopySourceItem != nullptr) {
            const auto index = item_array_index_by_file_no(mCopySourceItem->getFileNo());
            if (index < mSelectEffects.size() && mSelectEffects[index] != nullptr) {
                mSelectEffects[index]->disappear();
            }
        }
    }

    if (isHiddenAllLayout()) {
        setNerve(&NrvFileSelector::FileSelectorNrvCopyConfirm::sInstance);
    }
}

void FileSelector::exeCopyConfirm() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mCopyConfirmStarted = true;
#endif
        clearPointing();
        if (mSysInfoWindow != nullptr && mSelectedItem != nullptr) {
            mSysInfoWindow->appear(mSelectedItem->isNew() ? "System_FileSelect016" : "System_FileSelect014", SysInfoWindow::Type_YesNo,
                                   SysInfoWindow::TextPos_Center, SysInfoWindow::MessageType_System);
            mSysInfoWindow->setTextBoxArgNumber(mCopySourceFileNo, 0);
            mSysInfoWindow->setTextBoxArgNumber(mSelectedItem->getFileNo(), 1);
        }
    }

    if (mSysInfoWindow != nullptr && MR::isDead(mSysInfoWindow.get())) {
        if (mSysInfoWindow->isSelectedYes()) {
            setNerve(&NrvFileSelector::FileSelectorNrvCopySave::sInstance);
        } else {
            mSelectedItem = mCopySourceItem;
            mSelectedFileNo = mCopySourceFileNo;
            setNerve(&NrvFileSelector::FileSelectorNrvCopySelect::sInstance);
        }
    }
}

void FileSelector::exeCopySave() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mCopySaveStarted = true;
#endif
        const auto dst_file_no = mSelectedItem != nullptr ? mSelectedItem->getFileNo() : mSelectedFileNo;
        const auto src_file_no = mCopySourceItem != nullptr ? mCopySourceItem->getFileNo() : mCopySourceFileNo;
        const auto* src_file = src_file_no >= 1 && src_file_no <= cItemCount ? mFiles[static_cast< std::size_t >(src_file_no - 1)].get() : nullptr;
        setUserFileMario(dst_file_no, src_file == nullptr || src_file->mIsPlayerMario);

        if (mSelectedItem != nullptr && mSelectedItem->isNew()) {
            GameSequenceFunction::startCopyUserFileSequence(dst_file_no, src_file_no);
        } else {
            GameSequenceFunction::storeCopyUserFileSequence(dst_file_no, src_file_no);
            setNerve(&NrvFileSelector::FileSelectorNrvCopySaveMii::sInstance);
            return;
        }
    }

    if (!GameSequenceFunction::isActiveSaveDataHandleSequence()) {
        setNerve(&NrvFileSelector::FileSelectorNrvCopyDemo::sInstance);
    }
}

void FileSelector::exeCopySaveMii() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mCopySaveMiiStarted = true;
#endif
        GameSequenceFunction::startSaveAllUserFileSequence();
    }

    if (!GameSequenceFunction::isActiveSaveDataHandleSequence()) {
        setNerve(&NrvFileSelector::FileSelectorNrvCopyDemo::sInstance);
    }
}

void FileSelector::exeCopyDemo() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mCopyDemoStarted = true;
#endif
        MR::startSystemSE("SE_SY_FILE_SEL_COPY", -1, -1);
        restoreUserFile();
        if (mSelectedItem != nullptr) {
            const auto file_no = std::clamp(mSelectedItem->getFileNo(), 1, cItemCount);
            auto icon_id = FileSelectIconID();
            getIconId(&icon_id, file_no);
            mSelectedItem->change(false, icon_id);
        }
        if (mBackButton != nullptr) {
            mBackButton->disappear();
        }
        if (mInfoMessage != nullptr) {
            mInfoMessage->disappear();
        }
        invalidateSelectAll();
    }

    if (mSelectedItem == nullptr || mSelectedItem->isExist()) {
        clearPointing();
        setNerve(&NrvFileSelector::FileSelectorNrvFileSelect::sInstance);
    }
}

void FileSelector::exeCopyRejectStart() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mCopyRejectStartStarted = true;
#endif
        disappearAllLayout();
    }

    if (isHiddenAllLayout()) {
        setNerve(&NrvFileSelector::FileSelectorNrvCopyReject::sInstance);
    }
}

void FileSelector::exeCopyReject() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mCopyRejectStarted = true;
#endif
        if (mSysInfoWindowMini != nullptr) {
            mSysInfoWindowMini->appear("System_FileSelect003", SysInfoWindow::Type_Key, SysInfoWindow::TextPos_Center,
                                       SysInfoWindow::MessageType_System);
        }
        MR::startSystemSE("SE_SY_FILE_SEL_NG", -1, -1);
    }

    if (mSysInfoWindowMini != nullptr && MR::isDead(mSysInfoWindowMini.get())) {
        if (mSelectButton != nullptr) {
            mSelectButton->shiftSelect();
        }
        setNerve(&NrvFileSelector::FileSelectorNrvFileConfirm::sInstance);
    }
}

void FileSelector::exeMiiWait() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mMiiWaitStarted = true;
#endif
        MR::startSystemSE("SE_SY_FILE_SEL_UPPER_DECIDE", -1, -1);
    }

    if ((mBackButton == nullptr || mBackButton->isHidden()) && MR::isDead(mSelectInfo.get())) {
        setNerve(&NrvFileSelector::FileSelectorNrvMiiSelectStart::sInstance);
    }
}

void FileSelector::exeMiiSelectStart() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mMiiSelectStartStarted = true;
#endif
        if (mSelectedItem != nullptr && mMiiSelect != nullptr) {
            auto icon_id = FileSelectIconID();
            mSelectedItem->copyIconID(&icon_id);
            mMiiSelect->prohibitIcon(icon_id);
        } else if (mMiiSelect != nullptr) {
            mMiiSelect->admitIcon();
        }

        if (mMiiSelect != nullptr) {
            if (isUserFileAppearLuigi(mSelectedFileNo)) {
                mMiiSelect->validateAllSpecialMii();
            } else {
                mMiiSelect->invalidateSpecialMii(FileSelectIconID::Luigi);
            }
            mMiiSelect->appear();
        }
    }

    if (mMiiSelect == nullptr || !mMiiSelect->isAppearing()) {
        setNerve(&NrvFileSelector::FileSelectorNrvMiiSelect::sInstance);
    }
}

void FileSelector::exeMiiSelect() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mMiiSelectStarted = true;
#endif
        if (mBackButton != nullptr && mBackButton->isHidden()) {
            mBackButton->appear();
        }
    }

    if (mMiiSelect != nullptr && mMiiSelect->isDummySelected()) {
        MR::startSystemSE("SE_SY_FILE_SEL_MII_SELECTED", -1, -1);
        mMiiSelect->disappear();
        if (mBackButton != nullptr && !mBackButton->isHidden()) {
            mBackButton->disappear();
        }
        setNerve(&NrvFileSelector::FileSelectorNrvMiiInfoStart::sInstance);
        return;
    }

    if (mMiiSelect != nullptr && mMiiSelect->isSelected()) {
        MR::startSystemSE("SE_SY_FILE_SEL_MII_SELECTED", -1, -1);
        mMiiSelect->disappear();
        if (mBackButton != nullptr && !mBackButton->isHidden()) {
            mBackButton->disappear();
        }
        setNerve(&NrvFileSelector::FileSelectorNrvMiiConfirmWait::sInstance);
        return;
    }

    if (checkSelectedBackButton()) {
        if (mMiiSelect != nullptr) {
            mMiiSelect->disappear();
        }
        if (mBackButton != nullptr) {
            mBackButton->disappear();
        }
        setNerve(&NrvFileSelector::FileSelectorNrvMiiCancel::sInstance);
    }
}

void FileSelector::exeMiiCancel() {
    if ((mMiiSelect == nullptr || MR::isDead(mMiiSelect.get())) && (mBackButton == nullptr || mBackButton->isHidden())) {
        setNerve(&NrvFileSelector::FileSelectorNrvFileConfirm::sInstance);
    }
}

void FileSelector::exeMiiConfirmWait() {
    if ((mMiiSelect == nullptr || MR::isDead(mMiiSelect.get())) && (mBackButton == nullptr || mBackButton->isHidden())) {
        setNerve(&NrvFileSelector::FileSelectorNrvMiiConfirm::sInstance);
    }
}

void FileSelector::exeMiiConfirm() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mMiiConfirmStarted = true;
#endif
        auto selected_id = FileSelectIconID();
        if (mMiiSelect != nullptr) {
            mMiiSelect->getSelectedID(&selected_id);
        }
        const auto selected_name = icon_name_wide(selected_id);
        if (mSysInfoWindow != nullptr) {
            mSysInfoWindow->appear(mIsFirstMiiSelection ? "System_FileSelect013" : "System_FileSelect005", SysInfoWindow::Type_YesNo,
                                   SysInfoWindow::TextPos_Bottom, SysInfoWindow::MessageType_System);
            mSysInfoWindow->setYesNoSelectorSE("SE_SY_BUTTON_CURSOR_ON", "SE_SY_FILE_SEL_MII_CHANGE", "SE_SY_TALK_SELECT_NO");
        }
        if (mMiiConfirmIcon != nullptr) {
            mMiiConfirmIcon->appear(mMiiSelect != nullptr ? mMiiSelect->getSelectedMiiTexMap() : nullptr, selected_name.data());
        }
    }

    if (mMiiConfirmIcon != nullptr && !mMiiConfirmIcon->isDisappear() && mSysInfoWindow != nullptr && mSysInfoWindow->isDisappear()) {
        mMiiConfirmIcon->disappear();
    }

    if (mSysInfoWindow != nullptr && MR::isDead(mSysInfoWindow.get())) {
        mSysInfoWindow->resetYesNoSelectorSE();
        if (mSysInfoWindow->isSelectedYes()) {
            setNerve(&NrvFileSelector::FileSelectorNrvMiiCreateWait::sInstance);
        } else if (mIsFirstMiiSelection) {
            setNerve(&NrvFileSelector::FileSelectorNrvMiiSelectStartFirst::sInstance);
        } else {
            setNerve(&NrvFileSelector::FileSelectorNrvMiiSelectStart::sInstance);
        }
    }
}

void FileSelector::exeMiiCreateWait() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mMiiCreateWaitStarted = true;
#endif
        auto selected_id = FileSelectIconID();
        if (mMiiSelect != nullptr) {
            mMiiSelect->getSelectedID(&selected_id);
        }
        storeSetMiiIdUserFile(mSelectedFileNo, selected_id);
        mIsFirstMiiSelection = false;
    }

    if (!GameSequenceFunction::isActiveSaveDataHandleSequence()) {
        setNerve(&NrvFileSelector::FileSelectorNrvMiiCreateDemo::sInstance);
    }
}

void FileSelector::exeMiiCreateDemo() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mMiiCreateDemoStarted = true;
#endif
        restoreUserFile();
        auto icon_id = FileSelectIconID();
        getIconId(&icon_id, mSelectedFileNo);
        if (mSelectedItem != nullptr) {
            mSelectedItem->change(false, icon_id);
        }
        setFileInfo(mSelectedFileNo);
        MR::startSystemSE("SE_SY_FILE_SEL_MII_CHANGE", -1, -1);
    }

    if (mSelectedItem == nullptr || mSelectedItem->isExist()) {
        auto icon_id = FileSelectIconID();
        getIconId(&icon_id, mSelectedFileNo);
        setNerve(icon_id.isMii() ? static_cast< const Nerve* >(&NrvFileSelector::FileSelectorNrvMiiCaution::sInstance) :
                                   static_cast< const Nerve* >(&NrvFileSelector::FileSelectorNrvFileConfirm::sInstance));
    }
}

void FileSelector::exeMiiCaution() {
    if (MR::isFirstStep(this) && mSysInfoWindowMini != nullptr) {
        mSysInfoWindowMini->appear("System_FileSelect006", SysInfoWindow::Type_Key, SysInfoWindow::TextPos_Center,
                                   SysInfoWindow::MessageType_System);
    }

    if (mSysInfoWindowMini != nullptr && MR::isDead(mSysInfoWindowMini.get())) {
        setNerve(&NrvFileSelector::FileSelectorNrvFileConfirm::sInstance);
    }
}

void FileSelector::exeMiiInfoStart() {
    if ((mMiiSelect == nullptr || MR::isDead(mMiiSelect.get())) && (mBackButton == nullptr || mBackButton->isHidden())) {
        setNerve(&NrvFileSelector::FileSelectorNrvMiiInfo::sInstance);
    }
}

void FileSelector::exeMiiInfo() {
    if (MR::isFirstStep(this) && mSysInfoWindowMini != nullptr) {
        mSysInfoWindowMini->appear("System_FileSelect015", SysInfoWindow::Type_Key, SysInfoWindow::TextPos_Center,
                                   SysInfoWindow::MessageType_System);
    }

    if (mSysInfoWindowMini != nullptr && MR::isDead(mSysInfoWindowMini.get())) {
        setNerve(mIsFirstMiiSelection ? static_cast< const Nerve* >(&NrvFileSelector::FileSelectorNrvMiiSelectStartFirst::sInstance) :
                                        static_cast< const Nerve* >(&NrvFileSelector::FileSelectorNrvMiiSelectStart::sInstance));
    }
}

void FileSelector::exeDeleteConfirmStart() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mDeleteConfirmStartStarted = true;
#endif
        MR::startSystemSE("SE_SY_FILE_SEL_UPPER_DECIDE", -1, -1);
        disappearAllLayout();
    }

    if (isHiddenAllLayout()) {
        setNerve(&NrvFileSelector::FileSelectorNrvDeleteConfirm::sInstance);
    }
}

void FileSelector::exeDeleteConfirm() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mDeleteConfirmStarted = true;
#endif
        mSysInfoWindow->setYesNoSelectorSE("SE_SY_BUTTON_CURSOR_ON", "SE_SY_FILE_SEL_DELETE", "SE_SY_TALK_SELECT_NO");
        mSysInfoWindow->appear("System_FileSelect007", SysInfoWindow::Type_YesNo, SysInfoWindow::TextPos_Center, SysInfoWindow::MessageType_System);
    }

    if (MR::isDead(mSysInfoWindow.get())) {
        mSysInfoWindow->resetYesNoSelectorSE();
        if (mSysInfoWindow->isSelectedYes()) {
            setNerve(&NrvFileSelector::FileSelectorNrvDelete::sInstance);
        } else {
            if (mSelectButton != nullptr) {
                mSelectButton->shiftSelect();
            }
            setNerve(&NrvFileSelector::FileSelectorNrvFileConfirm::sInstance);
        }
    }
}

void FileSelector::exeDelete() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mDeleteStarted = true;
#endif
        const auto file_no = mSelectedItem != nullptr ? mSelectedItem->getFileNo() : mSelectedFileNo;
        setUserFileMario(file_no, true);
        GameSequenceFunction::startDeleteUserFileSequence(file_no);
    }

    if (!GameSequenceFunction::isActiveSaveDataHandleSequence()) {
        if (GameSequenceFunction::isSuccessSaveDataHandleSequence()) {
            setNerve(&NrvFileSelector::FileSelectorNrvDeleteDemo::sInstance);
        } else {
            clearPointing();
            setNerve(&NrvFileSelector::FileSelectorNrvFileSelectStart::sInstance);
        }
    }
}

void FileSelector::exeDeleteDemo() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mDeleteDemoStarted = true;
#endif
        if (mSelectedItem != nullptr) {
            mSelectedItem->format();
        }
    }

    if (mSelectedItem == nullptr || mSelectedItem->isNew()) {
        initUserFile();
        clearPointing();
        setNerve(&NrvFileSelector::FileSelectorNrvFileSelectStart::sInstance);
    }
}

void FileSelector::exeManualStart() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mManualStartStarted = true;
#endif
        disappearAllLayout();
    }

    if (isHiddenAllLayout()) {
        setNerve(&NrvFileSelector::FileSelectorNrvManual::sInstance);
    }
}

void FileSelector::exeManual() {
    if (MR::isFirstStep(this)) {
#ifndef NDEBUG
        mManualStarted = true;
#endif
        if (mManual2P != nullptr) {
            mManual2P->appear();
        }
    }

    if (mManual2P != nullptr && mManual2P->isClosed()) {
        setNerve(&NrvFileSelector::FileSelectorNrvFileConfirm::sInstance);
    }
}

#ifndef NDEBUG
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
    return mSelectAllInvalidatedOnce;
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

bool FileSelector::isItemNew(s32 index) const {
    if (index < 0 || index >= cItemCount || mItems[static_cast< std::size_t >(index)] == nullptr) {
        return false;
    }

    return mItems[static_cast< std::size_t >(index)]->isNew();
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

bool FileSelector::isOperationButtonCreated() const {
    return mSelectButton != nullptr;
}

bool FileSelector::isFileInfoCreated() const {
    return mSelectInfo != nullptr;
}

bool FileSelector::isBackButtonCreated() const {
    return mBackButton != nullptr;
}

bool FileSelector::isBrosButtonCreated() const {
    return mBrosButton != nullptr;
}

bool FileSelector::isInfoMessageCreated() const {
    return mInfoMessage != nullptr;
}

bool FileSelector::isSysInfoWindowCreated() const {
    return mSysInfoWindow != nullptr && mSysInfoWindowMini != nullptr;
}

bool FileSelector::isMiiConfirmIconCreated() const {
    return mMiiConfirmIcon != nullptr;
}

bool FileSelector::isManualCreated() const {
    return mManual2P != nullptr;
}

bool FileSelector::isSelectEffectCreated() const {
    return std::ranges::all_of(mSelectEffects, [](const auto& effect) { return effect != nullptr; });
}

s32 FileSelector::getSelectEffectCount() const {
    return static_cast< s32 >(std::ranges::count_if(mSelectEffects, [](const auto& effect) { return effect != nullptr; }));
}

bool FileSelector::isOperationButtonAlive() const {
    return mSelectButton != nullptr && !MR::isDead(mSelectButton.get());
}

bool FileSelector::isFileInfoAlive() const {
    return mSelectInfo != nullptr && !MR::isDead(mSelectInfo.get());
}

bool FileSelector::wasStartCallbackCalled() const {
    return mStartCallbackCalled;
}

bool FileSelector::wasCopyCallbackCalled() const {
    return mCopyCallbackCalled;
}

bool FileSelector::wasMiiCallbackCalled() const {
    return mMiiCallbackCalled;
}

bool FileSelector::wasDeleteCallbackCalled() const {
    return mDeleteCallbackCalled;
}

bool FileSelector::wasManualCallbackCalled() const {
    return mManualCallbackCalled;
}

bool FileSelector::didClearPointing() const {
    return mPointingCleared;
}

bool FileSelector::didStartDemoStartWait() const {
    return mDemoStartWaitStarted;
}

bool FileSelector::didStartCopyWait() const {
    return mCopyWaitStarted;
}

bool FileSelector::didStartCopySelect() const {
    return mCopySelectStarted;
}

bool FileSelector::didStartCopyConfirmStart() const {
    return mCopyConfirmStartStarted;
}

bool FileSelector::didStartCopyConfirm() const {
    return mCopyConfirmStarted;
}

bool FileSelector::didStartCopySave() const {
    return mCopySaveStarted;
}

bool FileSelector::didStartCopySaveMii() const {
    return mCopySaveMiiStarted;
}

bool FileSelector::didStartCopyDemo() const {
    return mCopyDemoStarted;
}

bool FileSelector::didStartCopyRejectStart() const {
    return mCopyRejectStartStarted;
}

bool FileSelector::didStartCopyReject() const {
    return mCopyRejectStarted;
}

bool FileSelector::didStartMiiWait() const {
    return mMiiWaitStarted;
}

bool FileSelector::didStartMiiSelectStart() const {
    return mMiiSelectStartStarted;
}

bool FileSelector::didStartMiiSelect() const {
    return mMiiSelectStarted;
}

bool FileSelector::didStartMiiConfirm() const {
    return mMiiConfirmStarted;
}

bool FileSelector::didStartMiiCreateWait() const {
    return mMiiCreateWaitStarted;
}

bool FileSelector::didStartMiiCreateDemo() const {
    return mMiiCreateDemoStarted;
}

bool FileSelector::didStartDeleteConfirmStart() const {
    return mDeleteConfirmStartStarted;
}

bool FileSelector::didStartManualStart() const {
    return mManualStartStarted;
}

bool FileSelector::didStartManual() const {
    return mManualStarted;
}

bool FileSelector::didStartDemo() const {
    return mDemoStarted;
}

bool FileSelector::didAppearAllIndex() const {
    return mAllIndexAppeared;
}

bool FileSelector::didDisappearAllIndex() const {
    return mAllIndexDisappeared;
}

bool FileSelector::didStartFileConfirmStart() const {
    return mFileConfirmStartStarted;
}

bool FileSelector::didStartFileConfirm() const {
    return mFileConfirmStarted;
}

bool FileSelector::didStartCreateConfirmStart() const {
    return mCreateConfirmStartStarted;
}

bool FileSelector::didStartCreateConfirm() const {
    return mCreateConfirmStarted;
}

bool FileSelector::didStartCreate() const {
    return mCreateStarted;
}

bool FileSelector::didStartDeleteConfirm() const {
    return mDeleteConfirmStarted;
}

bool FileSelector::didStartDelete() const {
    return mDeleteStarted;
}

bool FileSelector::didStartDeleteDemo() const {
    return mDeleteDemoStarted;
}

bool FileSelector::isDemoStartWait() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvDemoStartWait::sInstance);
}

bool FileSelector::isDemo() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvDemo::sInstance);
}

bool FileSelector::isFileSelectStart() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvFileSelectStart::sInstance);
}

bool FileSelector::isFileSelect() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvFileSelect::sInstance);
}

bool FileSelector::isFileConfirmStart() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvFileConfirmStart::sInstance);
}

bool FileSelector::isFileConfirm() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvFileConfirm::sInstance);
}

bool FileSelector::isCreateConfirmStart() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvCreateConfirmStart::sInstance);
}

bool FileSelector::isCreateConfirm() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvCreateConfirm::sInstance);
}

bool FileSelector::isCreate() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvCreate::sInstance);
}

bool FileSelector::isCopyWait() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvCopyWait::sInstance);
}

bool FileSelector::isCopySelect() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvCopySelect::sInstance);
}

bool FileSelector::isCopyConfirmStart() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvCopyConfirmStart::sInstance);
}

bool FileSelector::isCopyConfirm() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvCopyConfirm::sInstance);
}

bool FileSelector::isCopySave() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvCopySave::sInstance);
}

bool FileSelector::isCopySaveMii() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvCopySaveMii::sInstance);
}

bool FileSelector::isCopyDemo() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvCopyDemo::sInstance);
}

bool FileSelector::isCopyRejectStart() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvCopyRejectStart::sInstance);
}

bool FileSelector::isCopyReject() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvCopyReject::sInstance);
}

bool FileSelector::isMiiWait() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvMiiWait::sInstance);
}

bool FileSelector::isMiiSelectStartFirst() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvMiiSelectStartFirst::sInstance);
}

bool FileSelector::isMiiSelectStart() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvMiiSelectStart::sInstance);
}

bool FileSelector::isMiiSelect() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvMiiSelect::sInstance);
}

bool FileSelector::isMiiCancel() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvMiiCancel::sInstance);
}

bool FileSelector::isMiiConfirmWait() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvMiiConfirmWait::sInstance);
}

bool FileSelector::isMiiConfirm() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvMiiConfirm::sInstance);
}

bool FileSelector::isMiiCreateWait() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvMiiCreateWait::sInstance);
}

bool FileSelector::isMiiCreateDemo() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvMiiCreateDemo::sInstance);
}

bool FileSelector::isMiiCaution() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvMiiCaution::sInstance);
}

bool FileSelector::isMiiInfoStart() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvMiiInfoStart::sInstance);
}

bool FileSelector::isMiiInfo() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvMiiInfo::sInstance);
}

bool FileSelector::isDeleteConfirmStart() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvDeleteConfirmStart::sInstance);
}

bool FileSelector::isDeleteConfirm() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvDeleteConfirm::sInstance);
}

bool FileSelector::isDelete() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvDelete::sInstance);
}

bool FileSelector::isDeleteDemo() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvDeleteDemo::sInstance);
}

bool FileSelector::isManualStart() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvManualStart::sInstance);
}

bool FileSelector::isManual() const {
    return isNerve(&NrvFileSelector::FileSelectorNrvManual::sInstance);
}

s32 FileSelector::getSelectedFileNo() const {
    return mSelectedFileNo;
}

s32 FileSelector::getPointedFileNo() const {
    return mPointingItem != nullptr ? mPointingItem->getFileNo() : 0;
}

s32 FileSelector::getPreviousPointingFileNo() const {
    return mPreviousPointingItem != nullptr ? mPreviousPointingItem->getFileNo() : 0;
}

s32 FileSelector::getCurrentFileInfoFileNo() const {
    return mSelectInfo != nullptr ? mSelectInfo->getFileNumber() : 0;
}

s32 FileSelector::getCurrentFileInfoMissCount() const {
    return mSelectInfo != nullptr ? mSelectInfo->getMissNum() : -1;
}

bool FileSelector::isCurrentFileInfoSelectedMario() const {
    return mSelectInfo != nullptr && mSelectInfo->isSelectedMario();
}

const wchar_t* FileSelector::getCurrentFileInfoDateMessage() const {
    return mSelectInfo != nullptr ? mSelectInfo->getDateMessage() : L"";
}

const wchar_t* FileSelector::getCurrentFileInfoTimeMessage() const {
    return mSelectInfo != nullptr ? mSelectInfo->getTimeMessage() : L"";
}

bool FileSelector::isCameraAtNearPoint() const {
    return mCameraController != nullptr && mCameraController->isAtNearPoint();
}
#endif

FileSelectItem* FileSelector::getItemByFileNo(s32 fileNo) const {
    for (const auto& item : mItems) {
        if (item != nullptr && item->getFileNo() == fileNo) {
            return item.get();
        }
    }

    return nullptr;
}

#ifndef NDEBUG
bool FileSelector::wasItemPointed(s32 fileNo) const {
    const auto* item = getItemByFileNo(fileNo);
    return item != nullptr && item->wasPointed();
}

bool FileSelector::didItemTurnToFront(s32 fileNo) const {
    const auto* item = getItemByFileNo(fileNo);
    return item != nullptr && item->didTurnToFront();
}

s32 FileSelector::getItemTurnToFrontFrameCount(s32 fileNo) const {
    const auto* item = getItemByFileNo(fileNo);
    return item != nullptr ? item->getTurnToFrontFrameCount() : 0;
}

const smgpc::game::CameraParamVec3& FileSelector::getItemBasePosition(s32 index) const {
    return mItemBasePositions.at(static_cast< std::size_t >(index));
}

const TVec3f& FileSelector::getItemPosition(s32 index) const {
    return mItems.at(static_cast< std::size_t >(index))->getPosition();
}
#endif
