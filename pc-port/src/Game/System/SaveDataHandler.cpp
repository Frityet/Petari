#include "Game/System/SaveDataHandler.hpp"

#include <algorithm>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include "Game/LiveActor/Nerve.hpp"
#include "Game/System/NANDManager.hpp"
#include "Game/System/SysConfigFile.hpp"
#include "Game/System/UserFile.hpp"
#include "Game/compat/RuntimeContext.hpp"

namespace {
    NEW_NERVE(SaveDataHandlerNrvWait, SaveDataHandler, Wait);
    NEW_NERVE(SaveDataHandlerNrvProcessing, SaveDataHandler, Processing);

    smgpc::game::SaveDataService& backing_save_data() {
        return smgpc::game::RuntimeContext::instance().save_data();
    }

    [[nodiscard]] s32 slot_index_from_name(std::string_view name) {
        if (name.empty()) {
            return 1;
        }

        const auto ch = name.back();
        return ch <= '0' || ch > '9' ? 1 : static_cast<s32>(ch - '0');
    }

    [[nodiscard]] bool is_config_name(std::string_view name) {
        return name.starts_with("config");
    }

    [[nodiscard]] bool is_game_name(std::string_view name) {
        return name.starts_with("mario") || name.starts_with("luigi");
    }

    [[nodiscard]] bool is_mario_name(std::string_view name) {
        return name.starts_with("mario");
    }
}  // namespace

SaveDataHandler::SaveDataHandler(const SysConfigFile*, const UserFile*)
    : NerveExecutor("セーブデータハンドラ"), mNANDRequestInfo(nullptr), _C(0), _10(0), _14(nullptr), _18(nullptr), mBannerCreator(nullptr) {
    initNerve(&SaveDataHandlerNrvWait::sInstance);
}

void SaveDataHandler::update() {
    updateNerve();
}

void SaveDataHandler::requestCheckEnableToCreate() {
    mDone = true;
    setResult(NAND_RESULT_OK);
}

void SaveDataHandler::requestLoadSaveData() {
    backing_save_data().load_host_files();
    mDone = true;
    setResult(NAND_RESULT_OK);
}

bool SaveDataHandler::requestVerifyAfterLoadGameDataFile() {
    setResult(NAND_RESULT_OK);
    return true;
}

void SaveDataHandler::initializeUserFileMemory(int index, const UserFile* pUserFile) {
    if (pUserFile == nullptr) {
        setResult(NAND_RESULT_INVALID);
        return;
    }

    backing_save_data().store_user_file(index, *pUserFile);
    setResult(NAND_RESULT_OK);
}

void SaveDataHandler::copyUserFileMemory(int indexDst, int indexSrc) {
    backing_save_data().copy_slot_state(indexDst, indexSrc);
    setResult(NAND_RESULT_OK);
}

void SaveDataHandler::restoreGameDataFile(const char* pName, void* pBuffer, u32 size) {
    if (pBuffer == nullptr || size == 0U) {
        setResult(NAND_RESULT_INVALID);
        return;
    }

    std::memset(pBuffer, 0, size);
    const auto name = pName != nullptr ? std::string_view(pName) : std::string_view{};
    if (const auto bytes = backing_save_data().read_file(name)) {
        std::memcpy(pBuffer, bytes->data(), std::min<std::size_t>(size, bytes->size()));
        setResult(NAND_RESULT_OK);
        return;
    }

    if (is_config_name(name) || is_game_name(name)) {
        auto file = UserFile();
        const auto slot_index = slot_index_from_name(name);
        backing_save_data().restore_user_file(file, slot_index, is_mario_name(name));
        if (is_config_name(name)) {
            file.makeConfigDataBinary(static_cast<u8*>(pBuffer), size);
        } else {
            file.makeGameDataBinary(static_cast<u8*>(pBuffer), size);
        }
        setResult(NAND_RESULT_OK);
        return;
    }

    if (name == "sysconf") {
        auto sys_config = SysConfigFile();
        sys_config.setTimeAnnounced(backing_save_data().sys_config_time_announced());
        sys_config.setTimeSent(backing_save_data().sys_config_time_sent());
        sys_config.setSentBytes(backing_save_data().sys_config_sent_bytes());
        sys_config.makeDataBinary(static_cast<u8*>(pBuffer), size);
        setResult(NAND_RESULT_OK);
        return;
    }

    setResult(NAND_RESULT_NOEXISTS);
}

void SaveDataHandler::storeUserFile(const UserFile* pUserFile) {
    if (pUserFile == nullptr) {
        setResult(NAND_RESULT_INVALID);
        return;
    }

    backing_save_data().store_user_file(slot_index_from_name(pUserFile->getConfigDataName()), *pUserFile);
    setResult(NAND_RESULT_OK);
}

void SaveDataHandler::storeSysConfigFile(const SysConfigFile* pSysConfigFile) {
    if (pSysConfigFile == nullptr) {
        setResult(NAND_RESULT_INVALID);
        return;
    }

    backing_save_data().set_sys_config_time_announced(pSysConfigFile->getTimeAnnounced());
    backing_save_data().set_sys_config_time_sent(pSysConfigFile->getTimeSent());
    backing_save_data().set_sys_config_sent_bytes(pSysConfigFile->getSentBytes());
    setResult(NAND_RESULT_OK);
}

void SaveDataHandler::requestSaveSaveData() {
    backing_save_data().flush_host_files();
    mDone = true;
    setResult(NAND_RESULT_OK);
}

void SaveDataHandler::requestRemoveSaveData() {
    for (auto slot_index = s32{1}; slot_index <= 6; ++slot_index) {
        auto slot_state = smgpc::game::SaveDataService::SlotState{};
        slot_state.slot_index = slot_index;
        backing_save_data().set_slot_state(slot_index, slot_state);
        backing_save_data().erase("config" + std::to_string(slot_index));
        backing_save_data().erase("mario" + std::to_string(slot_index));
        backing_save_data().erase("luigi" + std::to_string(slot_index));
    }
    setResult(NAND_RESULT_OK);
}

u32 SaveDataHandler::getEnoughtTempBufferSize() {
    return 512U;
}

bool SaveDataHandler::isDone() const {
    return mDone;
}

NANDResultCode SaveDataHandler::getLastResultCode() const {
    return NANDResultCode(mLastResult);
}

void SaveDataHandler::exeWait() {
}

void SaveDataHandler::exeProcessing() {
}

void SaveDataHandler::exeSaveProcessingGameData() {
}

void SaveDataHandler::exeSaveProcessingBanner() {
}

void SaveDataHandler::exeRemoveProcessingBanner() {
}

void SaveDataHandler::exeRemoveProcessingGameData() {
}

void SaveDataHandler::resetSaveData(u8* pBuffer) {
    if (pBuffer != nullptr) {
        std::memset(pBuffer, 0, getEnoughtTempBufferSize());
    }
}

void SaveDataHandler::initializeAllFileInSaveData(u8*, const SysConfigFile* pSysConfigFile, const UserFile* pUserFile) {
    auto handler = SaveDataHandler(pSysConfigFile, pUserFile);
    if (pSysConfigFile != nullptr) {
        handler.storeSysConfigFile(pSysConfigFile);
    }
    if (pUserFile != nullptr) {
        for (auto slot_index = s32{1}; slot_index <= 6; ++slot_index) {
            handler.initializeUserFileMemory(slot_index, pUserFile);
        }
    }
}

bool SaveDataHandler::isCorrectFileHeader(const u8*) {
    return true;
}

void SaveDataHandler::copySaveDataEachFile(u8* pDst, const u8* pSrc) {
    if (pDst != nullptr && pSrc != nullptr) {
        std::memcpy(pDst, pSrc, getEnoughtTempBufferSize());
    }
}

void SaveDataHandler::createCommunicationBuffer() {
}

bool SaveDataHandler::tryRemoveFile(const char* pName, bool* pIsDone) {
    if (pName != nullptr) {
        backing_save_data().erase(pName);
    }
    if (pIsDone != nullptr) {
        *pIsDone = true;
    }
    setResult(NAND_RESULT_OK);
    return true;
}

bool SaveDataHandler::trySave(bool* pIsDone, bool) {
    requestSaveSaveData();
    if (pIsDone != nullptr) {
        *pIsDone = true;
    }
    return true;
}

void SaveDataHandler::setResult(s32 result) {
    mLastResult = result;
}
