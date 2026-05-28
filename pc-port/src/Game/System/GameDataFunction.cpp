#include "Game/System/GameDataFunction.hpp"

#include <cstring>

#include "Game/System/GameDataHolder.hpp"
#include "Game/System/SaveDataHandleSequence.hpp"
#include "Game/System/SysConfigFile.hpp"
#include "Game/System/UserFile.hpp"
#include "runtime/RuntimeContext.hpp"

namespace GameDataFunction {

    const wchar_t* getUserName() {
        auto* file = smgpc::game::save_data_handle_sequence().getCurrentUserFile();
        return file != nullptr ? file->mUserName : L"";
    }

    u16 getUserFileIndex() {
        auto* file = smgpc::game::save_data_handle_sequence().getCurrentUserFile();
        if (file == nullptr) {
            return 1U;
        }

        const char* pGameDataName = file->getGameDataName();
        const auto length = std::strlen(pGameDataName);
        if (length == 0U) {
            return 1U;
        }

        const auto ch = pGameDataName[length - 1U];
        return ch <= '0' || ch > '9' ? 1U : static_cast< u16 >(ch - '0');
    }

    s32 getPictureBookChapterCanRead() {
        auto* file = smgpc::game::save_data_handle_sequence().getCurrentUserFile();
        return file != nullptr ? file->mGameDataHolder->getPictureBookChapterCanRead() : 0;
    }

    u16 getPictureBookChapterAlreadyRead() {
        auto* file = smgpc::game::save_data_handle_sequence().getCurrentUserFile();
        return file != nullptr ? file->mGameDataHolder->getPictureBookChapterAlreadyRead() : 0U;
    }

    void setPictureBookChapterAlreadyRead(int chapterAlreadyRead) {
        if (auto* file = smgpc::game::save_data_handle_sequence().getCurrentUserFile()) {
            file->mGameDataHolder->setPictureBookChapterAlreadyRead(chapterAlreadyRead);
        }
    }

    OSTime getSysConfigFileTimeAnnounced() {
        if (auto* runtime = smgpc::compat::RuntimeContext::try_instance()) {
            return runtime->save_data().sys_config_time_announced();
        }

        auto* sysConfig = smgpc::game::save_data_handle_sequence().getSysConfigFile();
        return sysConfig != nullptr ? sysConfig->getTimeAnnounced() : 0;
    }

    void updateSysConfigFileTimeAnnounced() {
        if (auto* runtime = smgpc::compat::RuntimeContext::try_instance()) {
            runtime->save_data().update_sys_config_time_announced();
        }
        if (auto* sysConfig = smgpc::game::save_data_handle_sequence().getSysConfigFile()) {
            sysConfig->updateTimeAnnounced();
        }
    }

    OSTime getSysConfigFileTimeSent() {
        if (auto* runtime = smgpc::compat::RuntimeContext::try_instance()) {
            return runtime->save_data().sys_config_time_sent();
        }

        auto* sysConfig = smgpc::game::save_data_handle_sequence().getSysConfigFile();
        return sysConfig != nullptr ? sysConfig->getTimeSent() : 0;
    }

    void setSysConfigFileTimeSent(OSTime time) {
        if (auto* runtime = smgpc::compat::RuntimeContext::try_instance()) {
            runtime->save_data().set_sys_config_time_sent(time);
        }
        if (auto* sysConfig = smgpc::game::save_data_handle_sequence().getSysConfigFile()) {
            sysConfig->setTimeSent(time);
        }
    }

    u32 getSysConfigFileSentBytes() {
        if (auto* runtime = smgpc::compat::RuntimeContext::try_instance()) {
            return runtime->save_data().sys_config_sent_bytes();
        }

        auto* sysConfig = smgpc::game::save_data_handle_sequence().getSysConfigFile();
        return sysConfig != nullptr ? sysConfig->getSentBytes() : 0U;
    }

    void setSysConfigFileSentBytes(u32 bytes) {
        if (auto* runtime = smgpc::compat::RuntimeContext::try_instance()) {
            runtime->save_data().set_sys_config_sent_bytes(bytes);
        }
        if (auto* sysConfig = smgpc::game::save_data_handle_sequence().getSysConfigFile()) {
            sysConfig->setSentBytes(bytes);
        }
    }

}  // namespace GameDataFunction
