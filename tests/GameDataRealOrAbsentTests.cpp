#include "OriginalStageResourceProcessFixture.hpp"
#include "OriginalSaveDataSnapshot.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/System/GameDataPlayerStatus.hpp"
#include "Game/System/SysConfigFile.hpp"
#include "resource/TextEncoding.hpp"
#include <stdexcept>
#include <string_view>

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

void verify_original_save_data() {
    auto& sequence = smgpc::test::original_save_sequence();
    auto& file = *sequence.getCurrentUserFile();
    auto& backup_file = *sequence.getBackupUserFile();
    smgpc::test::OriginalUserFileSnapshot preserve_current(file), preserve_backup(backup_file);
    auto& current = *file.mGameDataHolder;
    auto& backup = *backup_file.mGameDataHolder;
    require(GameDataFunction::getCurrentGameDataHolder() == &current &&
                GameDataFunction::getSceneStartGameDataHolder() == &backup && &current != &backup &&
                current.mUserFile == &file && backup.mUserFile == &backup_file,
            "GameDataFunction must route through distinct actual current and backup UserFiles");
    require(GameDataFunction::getUserName() == file.mUserName && GameDataFunction::getUserFileIndex() == 1,
            "global user name and slot must come from the original selected save sequence");
    require(GameDataFunction::getSysConfigFileTimeAnnounced() == sequence.getSysConfigFile()->getTimeAnnounced() &&
                GameDataFunction::getSysConfigFileTimeSent() == sequence.getSysConfigFile()->getTimeSent() &&
                GameDataFunction::getSysConfigFileSentBytes() == sequence.getSysConfigFile()->getSentBytes(),
            "system configuration queries must use the process save sequence's real SysConfigFile");

    current.resetAllData();
    sequence.backupCurrentUserFile();
    const auto flag = smgpc::resource::encode_cp932("ハチマリオ初変身");
    const auto story = smgpc::resource::encode_cp932("チコガイドデモ終了");
    require(current.mPlayerStatus->mStoryProgress == 0 && backup.mPlayerStatus->mStoryProgress == 0,
            "reset and original backup must preserve original zero story progress");
    GameDataFunction::onGameEventFlag(flag.c_str());
    GameDataFunction::setPictureBookChapterAlreadyRead(3);
    GameDataFunction::followStoryEventByName(story.c_str());
    require(GameDataFunction::isOnJustGameEventFlag(flag.c_str()) && !backup.isOnGameEventFlag(flag.c_str()) &&
                current.getPictureBookChapterAlreadyRead() == 3 && backup.getPictureBookChapterAlreadyRead() == 0 &&
                current.mPlayerStatus->mStoryProgress == 10 && backup.mPlayerStatus->mStoryProgress == 0,
            "current mutations must leave the original backup independent until the real backup operation");
    sequence.backupCurrentUserFile();
    require(!GameDataFunction::isOnJustGameEventFlag(flag.c_str()) && backup.isOnGameEventFlag(flag.c_str()) &&
                backup.getPictureBookChapterAlreadyRead() == 3 && backup.mPlayerStatus->mStoryProgress == 10,
            "backupCurrentUserFile must serialize game data and change just-event queries");
    current.resetAllData();
    require(!current.isOnGameEventFlag(flag.c_str()) && backup.isOnGameEventFlag(flag.c_str()) &&
                current.mPlayerStatus->mStoryProgress == 0 && backup.mPlayerStatus->mStoryProgress == 10,
            "resetting current data must not reset the independent scene-start file");
}
}

int main() {
    return smgpc::test::run_stage_resource_process("game-data-owner", verify_original_save_data);
}
