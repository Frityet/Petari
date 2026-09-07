#include "Game/System/GameDataFunction.hpp"

#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>

#include "Game/System/GameDataHolder.hpp"
#include "Game/System/SaveDataHandleSequence.hpp"
#include "Game/System/SysConfigFile.hpp"
#include "Game/System/UserFile.hpp"
#include "compat/GameDataFunctionCompat.hpp"

namespace {
thread_local GameDataHolder* sCurrentGameDataOverride = nullptr;
thread_local GameDataHolder* sSceneStartGameDataOverride = nullptr;

[[noreturn]] void unavailable(std::string_view operation) {
    throw std::logic_error("GameDataFunction operation is unavailable: " + std::string(operation));
}

UserFile& require_current_user_file() {
    auto* file = smgpc::game::save_data_handle_sequence().getCurrentUserFile();
    if (file == nullptr || file->mGameDataHolder == nullptr) {
        unavailable("current user file");
    }
    return *file;
}

UserFile& require_backup_user_file() {
    auto* file = smgpc::game::save_data_handle_sequence().getBackupUserFile();
    if (file == nullptr || file->mGameDataHolder == nullptr) {
        unavailable("scene-start user file");
    }
    return *file;
}

SysConfigFile& require_sys_config_file() {
    auto* file = smgpc::game::save_data_handle_sequence().getSysConfigFile();
    if (file == nullptr) {
        unavailable("system configuration file");
    }
    return *file;
}
}  // namespace

namespace smgpc::compat {

ScopedGameDataHolderOverride::ScopedGameDataHolderOverride(
    GameDataHolder& current, GameDataHolder* scene_start)
    : _previous_current(sCurrentGameDataOverride),
      _previous_scene_start(sSceneStartGameDataOverride) {
    sCurrentGameDataOverride = &current;
    sSceneStartGameDataOverride = scene_start != nullptr ? scene_start : &current;
}

ScopedGameDataHolderOverride::~ScopedGameDataHolderOverride() {
    sCurrentGameDataOverride = _previous_current;
    sSceneStartGameDataOverride = _previous_scene_start;
}

}  // namespace smgpc::compat

namespace GameDataFunction {

const wchar_t* getUserName() {
    auto& file = require_current_user_file();
    if (file.mUserName == nullptr) {
        unavailable("current user name");
    }
    return file.mUserName;
}

u16 getUserFileIndex() {
    const auto* name = require_current_user_file().getGameDataName();
    if (name == nullptr || *name == '\0') {
        unavailable("current game-data name");
    }
    const auto length = std::strlen(name);
    const auto last = name[length - 1U];
    if (last <= '0' || last > '9') {
        return 1U;
    }
    return static_cast<u16>(last - '0');
}

void onGameEventFlag(const char* name) {
    getCurrentGameDataHolder()->tryOnGameEventFlag(name);
}

bool isOnGameEventFlag(const char* name) {
    return getCurrentGameDataHolder()->isOnGameEventFlag(name);
}

u32 getGameEventValue(const char* name) {
    return static_cast<u32>(getCurrentGameDataHolder()->getGameEventValue(name));
}

void setGameEventValue(const char* name, u16 value) {
    getCurrentGameDataHolder()->setGameEventValue(name, value);
}

bool isOnGameEventValueForBit(const char* name, int bit) {
    return getCurrentGameDataHolder()->isOnGameEventValueForBit(name, bit);
}

void setGameEventValueForBit(const char* name, int bit, bool is_on) {
    getCurrentGameDataHolder()->setGameEventValueForBit(name, bit, is_on);
}

bool isPassedStoryEvent(const char* name) {
    return getCurrentGameDataHolder()->isPassedStoryEvent(name);
}

void followStoryEventByName(const char* name) {
    auto* holder = getCurrentGameDataHolder();
    if (!holder->isPassedStoryEvent(name)) {
        holder->followStoryEventByName(name);
    }
}

s32 getPictureBookChapterCanRead() {
    return getCurrentGameDataHolder()->getPictureBookChapterCanRead();
}

u32 getPictureBookChapterAlreadyRead() {
    return static_cast<u32>(getCurrentGameDataHolder()->getPictureBookChapterAlreadyRead());
}

void setPictureBookChapterAlreadyRead(int chapter) {
    getCurrentGameDataHolder()->setPictureBookChapterAlreadyRead(chapter);
}

void resetAllGameData() {
    getCurrentGameDataHolder()->resetAllData();
}

GameDataHolder* getCurrentGameDataHolder() {
    if (sCurrentGameDataOverride != nullptr) {
        return sCurrentGameDataOverride;
    }
    return require_current_user_file().mGameDataHolder;
}

GameDataHolder* getSceneStartGameDataHolder() {
    if (sSceneStartGameDataOverride != nullptr) {
        return sSceneStartGameDataOverride;
    }
    return require_backup_user_file().mGameDataHolder;
}

OSTime getSysConfigFileTimeAnnounced() {
    return require_sys_config_file().getTimeAnnounced();
}

void updateSysConfigFileTimeAnnounced() {
    require_sys_config_file().updateTimeAnnounced();
}

OSTime getSysConfigFileTimeSent() {
    return require_sys_config_file().getTimeSent();
}

void setSysConfigFileTimeSent(OSTime time) {
    require_sys_config_file().setTimeSent(time);
}

u32 getSysConfigFileSentBytes() {
    return require_sys_config_file().getSentBytes();
}

void setSysConfigFileSentBytes(u32 bytes) {
    require_sys_config_file().setSentBytes(bytes);
}

}  // namespace GameDataFunction
