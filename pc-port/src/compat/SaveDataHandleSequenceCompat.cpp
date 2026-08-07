#include "compat/SaveDataHandleSequenceCompat.hpp"

#include <stdexcept>
#include <string>
#include <string_view>

#include "Game/System/SaveDataHandleSequence.hpp"
#include "Game/System/UserFile.hpp"

namespace {
[[noreturn]] void unavailable(std::string_view operation) {
    throw std::logic_error("SaveDataHandleSequence operation is unavailable without retail GameData/NAND backing: " +
                           std::string(operation));
}
}  // namespace

SaveDataHandleSequence::SaveDataHandleSequence()
    : NerveExecutor("セーブ/ロード"), mSysConfigFile(nullptr), mCurrentUserFile(nullptr), mBackupUserFile(nullptr),
      mSaveDataHandler(nullptr), mNANDErrorSequence(nullptr), mSysInfoWindowConfirm(nullptr), mSysInfoWindowSave(nullptr),
      _24(0), mIsConfirmRemind(false), mIsSaveAndQuitMsg(false), _2A(false), _2B(false), _2C(false),
      mWorkUserFile(nullptr), mNerveForError(nullptr), mTempBuffer(nullptr), mOnSaveSuccessFunc(nullptr),
      mJustBeforeSaveFunc(nullptr), mSaveIcon(nullptr) {
}

void SaveDataHandleSequence::initAfterResourceLoaded() {
    unavailable("initialization");
}

void SaveDataHandleSequence::registerFunctorOnSaveSuccess(const MR::FunctorBase&) {
    unavailable("save-success callback registration");
}

void SaveDataHandleSequence::registerFunctorJustBeforeSave(const MR::FunctorBase&) {
    unavailable("pre-save callback registration");
}

void SaveDataHandleSequence::update() {
    unavailable("update");
}

void SaveDataHandleSequence::draw() const {
    unavailable("draw");
}

void SaveDataHandleSequence::startPreLoad() {
    unavailable("preload");
}

void SaveDataHandleSequence::startCreateUserFile(int) {
    unavailable("user-file creation");
}

void SaveDataHandleSequence::startDeleteUserFile(int) {
    unavailable("user-file deletion");
}

void SaveDataHandleSequence::startSave(bool, bool) {
    unavailable("save");
}

void SaveDataHandleSequence::startSaveBackup(bool, bool) {
    unavailable("backup save");
}

void SaveDataHandleSequence::startSaveAll() {
    unavailable("save-all");
}

void SaveDataHandleSequence::startSaveTotalMailSize() {
    unavailable("system-config save");
}

void SaveDataHandleSequence::startLoad(int, bool) {
    unavailable("load");
}

void SaveDataHandleSequence::storeMiiOrIconId(int, const void*, const u32*) {
    unavailable("Mii/icon persistence");
}

void SaveDataHandleSequence::storeCopyUserFile(int, int) {
    unavailable("user-file copy");
}

bool SaveDataHandleSequence::tryNANDErrorSequence(s32) {
    unavailable("NAND error sequence");
}

bool SaveDataHandleSequence::isActive() const {
    unavailable("active-state query");
}

bool SaveDataHandleSequence::isPermitToReset() const {
    unavailable("reset-permission query");
}

void SaveDataHandleSequence::prepareReset() {
    unavailable("reset preparation");
}

bool SaveDataHandleSequence::isPreparedReset() const {
    unavailable("reset-preparation query");
}

void SaveDataHandleSequence::restoreFromReset() {
    unavailable("reset restoration");
}

bool SaveDataHandleSequence::isInitializedGameDataHolder() const {
    unavailable("game-data initialization query");
}

void SaveDataHandleSequence::restoreUserFile(UserFile*, int) {
    unavailable("user-file restore");
}

void SaveDataHandleSequence::restoreUserFile(UserFile*, int, bool) {
    unavailable("player user-file restore");
}

void SaveDataHandleSequence::backupCurrentUserFile() {
    unavailable("user-file backup");
}

void SaveDataHandleSequence::exeNoOperation() {
    unavailable("no-operation state");
}

void SaveDataHandleSequence::exeCheckEnableToCreate() {
    unavailable("create-permission state");
}

void SaveDataHandleSequence::exeSaveConfirm() {
    unavailable("save-confirm state");
}

void SaveDataHandleSequence::exeSave() {
    unavailable("save state");
}

void SaveDataHandleSequence::exeSaveWindowDisappear() {
    unavailable("save-window state");
}

void SaveDataHandleSequence::exeSaveDoneKeyWait() {
    unavailable("save-complete state");
}

void SaveDataHandleSequence::exeSaveAllWithoutKeyWait() {
    unavailable("save-all state");
}

void SaveDataHandleSequence::exeSaveAllWithoutKeyWaitDisappear() {
    unavailable("save-all window state");
}

void SaveDataHandleSequence::exeSaveAllWithoutWindow() {
    unavailable("windowless save state");
}

void SaveDataHandleSequence::exePreLoad() {
    unavailable("preload state");
}

void SaveDataHandleSequence::exePreLoadDone() {
    unavailable("preload-complete state");
}

void SaveDataHandleSequence::exeNoSaveConfirmRemind() {
    unavailable("no-save reminder state");
}

void SaveDataHandleSequence::exeErrorHandling() {
    unavailable("error state");
}

GameDataHolder* SaveDataHandleSequence::getHolder() {
    unavailable("game-data holder access");
}

SysConfigFile* SaveDataHandleSequence::getSysConfigFile() {
    unavailable("system-config access");
}

UserFile* SaveDataHandleSequence::getCurrentUserFile() {
    unavailable("current user-file access");
}

UserFile* SaveDataHandleSequence::getBackupUserFile() {
    unavailable("backup user-file access");
}

void SaveDataHandleSequence::restoreUserFileConfigData(UserFile*, int) {
    unavailable("config restore");
}

void SaveDataHandleSequence::restoreUserFileGameData(UserFile*, int, bool) {
    unavailable("game-data restore");
}

void SaveDataHandleSequence::restoreSysConfigFile(SysConfigFile*) {
    unavailable("system-config restore");
}

bool SaveDataHandleSequence::trySave() {
    unavailable("save processing");
}

bool SaveDataHandleSequence::trySaveWindowDisappear(bool*, const Nerve*) {
    unavailable("save-window completion");
}

bool SaveDataHandleSequence::trySaveWithoutWindow(bool*, const Nerve*) {
    unavailable("windowless save processing");
}

bool SaveDataHandleSequence::tryConfirm(const char*, bool*) {
    unavailable("save confirmation");
}

bool SaveDataHandleSequence::tryProcessDoneKeyWait(const char*) {
    unavailable("save-complete input");
}

bool SaveDataHandleSequence::tryNoSave() {
    unavailable("no-save processing");
}

bool SaveDataHandleSequence::isEnablePointer() const {
    unavailable("pointer-enabled query");
}

bool SaveDataHandleSequence::executeSaveFinish(bool*, const Nerve*) {
    unavailable("save completion");
}

void SaveDataHandleSequence::syncNoSaveFlagsFromErrorSequence() {
    unavailable("NAND error flag synchronization");
}

namespace smgpc::compat {
[[noreturn]] void ensure_save_data_core_initialized(SaveDataHandleSequence&) {
    unavailable("partial core initialization");
}

bool try_initialize_save_data_ui(SaveDataHandleSequence&, smgpc::runtime::RuntimeContext&) {
    return false;
}

bool try_initialize_save_data_ui(SaveDataHandleSequence&) {
    return false;
}
}  // namespace smgpc::compat

namespace smgpc::game {
[[noreturn]] SaveDataHandleSequence& save_data_handle_sequence() {
    unavailable("global sequence backing");
}
}  // namespace smgpc::game
