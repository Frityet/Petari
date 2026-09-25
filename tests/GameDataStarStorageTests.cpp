#include "OriginalSaveDataSnapshot.hpp"
#include "OriginalStageResourceProcessFixture.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/System/GameDataConst.hpp"
#include "Game/System/GameDataGalaxyStorage.hpp"
#include "Game/System/GameEventFlag.hpp"
#include "Game/System/GameEventFlagTable.hpp"
#include "Game/System/GalaxyStatusAccessor.hpp"
#include "Game/System/ScenarioDataParser.hpp"
#include "Game/Util/SceneUtil.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
void require(bool pass, const char* message) {
    if (!pass) throw std::runtime_error(message);
}
void verify_star_storage() {
    auto& sequence = smgpc::test::original_save_sequence();
    auto& current = *sequence.mCurrentUserFile;
    auto& backup = *sequence.mBackupUserFile;
    const smgpc::test::OriginalUserFileSnapshot restore_current(current), restore_backup(backup);
    auto& holder = *current.mGameDataHolder;
    auto& previous = *backup.mGameDataHolder;
    holder.resetAllData();
    sequence.backupCurrentUserFile();
    require(GameDataFunction::getCurrentGameDataHolder() == &holder &&
                GameDataFunction::getSceneStartGameDataHolder() == &previous && &holder != &previous,
            "public queries use the original save sequence's distinct current and backup UserFiles");
    std::string first_name;
    s32 first_count = 0, expected_total = 0, hidden_stars = 0, galaxy_count = 0;
    for (auto iterator = MR::makeBeginScenarioDataIter(); !iterator.isEnd(); iterator.goNext()) {
        const auto accessor = iterator.makeAccessor();
        const auto count = accessor.getPowerStarNum();
        if (count == 0) {
            require(accessor.getPowerStarNumOwned() == 0 && !accessor.isCompleted(),
                    "galaxies without authored stars remain uncompleted with no storage record");
            continue;
        }
        if (first_name.empty()) {
            first_name = accessor.getName();
            first_count = count;
        }
        ++galaxy_count;
        for (s32 star = 1; star <= count; ++star) {
            require(!accessor.hasPowerStar(star), "each unset authored ownership bit is initially clear");
            GameDataFunction::setGameFlagPowerStarSuccess(accessor.getName(), star, true);
            require(accessor.hasPowerStar(star) && accessor.getPowerStarNumOwned() == star,
                    "original galaxy accessor counts actual individual ownership writes");
            GameDataFunction::setGameFlagPowerStarSuccess(accessor.getName(), star, true);
            require(accessor.getPowerStarNumOwned() == star, "repeated awards do not duplicate an ownership bit");
            hidden_stars += accessor.isHiddenStar(star);
        }
        expected_total += count;
        require(accessor.isCompleted(), "all authored star bits complete the original galaxy accessor");

    }
    require(galaxy_count > 1 && hidden_stars > 0, "actual catalog exercises multiple galaxies and hidden stars");
    require(GameDataFunction::calcCurrentPowerStarNum() == expected_total && MR::getPowerStarNum() == expected_total,
            "original SceneUtil counter sums the selected file's real galaxy bits, including hidden stars");

    s32 grand_stars = 0;
    for (s32 i = 0; i < GameEventFlagTable::getTableSize(); ++i) {
        const auto* flag = GameEventFlagTable::getFlag(i);
        constexpr std::string_view prefix = "SpecialStarGrand";
        if (flag->mType != GameEventFlag::Type_SpecialStar || !std::string_view(flag->mName).starts_with(prefix)) continue;
        const auto id = std::stoi(std::string(flag->mName + prefix.size()));
        require(GameDataConst::isGrandStar(flag->mGalaxyName, flag->mStarID) &&
                    MR::makeGalaxyStatusAccessor(flag->mGalaxyName).isExistGrandStar(),
                "original Grand Star classification reads the actual event table");
        require(GameDataFunction::hasGrandStar(id), "original Grand Star predicates read actual per-galaxy bits");
        GameDataFunction::setGameFlagPowerStarSuccess(flag->mGalaxyName, flag->mStarID, false);
        require(!GameDataFunction::hasGrandStar(id) && GameDataFunction::calcCurrentPowerStarNum() == expected_total - 1 &&
                    MR::getPowerStarNum() == expected_total - 1,
                "clearing a Grand Star changes both its derived flag and total exactly once");
        GameDataFunction::setGameFlagPowerStarSuccess(flag->mGalaxyName, flag->mStarID, true);
        ++grand_stars;
    }
    require(grand_stars > 0, "actual event table exercises Grand Star ownership");
    auto scenario = holder.makeGalaxyScenarioAccessor(first_name.c_str(), 1);
    scenario.updateMaxCoinNum(1234);
    scenario.updateMaxCoinNum(12);
    scenario.setFlagAlreadyVisited(true);
    require(scenario.getMaxCoinNum() == 999 && scenario.isAlreadyVisited(),
            "original scenario records clamp coins, preserve maxima, and retain visits");
    holder.addPlayerLeft(34 - holder.getPlayerLeft());
    holder.addStockedStarPiece(321 - holder.getStockedStarPieceNum());
    sequence.backupCurrentUserFile();
    require(previous.calcCurrentPowerStarNum() == expected_total && holder.getPlayerLeft() == 34 &&
                previous.getPlayerLeft() == 4 && previous.getStockedStarPieceNum() == 321,
            "original backup serialization preserves stars and bits while PLAY loading resets lives to four");
    const auto backup_scenario = previous.makeGalaxyScenarioAccessor(first_name.c_str(), 1);
    require(backup_scenario.mSomeGalaxyStorage != scenario.mSomeGalaxyStorage &&
                backup_scenario.getMaxCoinNum() == 999 && backup_scenario.isAlreadyVisited(),
            "current and backup own distinct deep copies of scenario data");
    std::array<u8, 4096> binary{};
    const auto binary_size = holder.makeFileBinary(binary.data(), binary.size());
    require(binary_size > 0 && binary_size <= binary.size(), "original chunks fit their file buffer");
    const std::string file_name(current.getGameDataName());
    holder.resetAllData();
    require(GameDataFunction::calcCurrentPowerStarNum() == 0 && MR::getPowerStarNum() == 0 &&
                previous.calcCurrentPowerStarNum() == expected_total && !scenario.hasPowerStar() &&
                !scenario.isAlreadyVisited() && scenario.getMaxCoinNum() == 0,
            "reset clears current records in place without mutating the original backup");
    current.loadFromGameDataBinary(file_name.c_str(), binary.data(), binary_size);
    require(!current.mIsGameDataCorrupted && MR::getPowerStarNum() == expected_total &&
                holder.getPlayerLeft() == 4 && holder.getStockedStarPieceNum() == 321 &&
                scenario.isAlreadyVisited() && scenario.getMaxCoinNum() == 999,
            "original UserFile deserialization restores all star, coin, visit and PLAY chunks");
    GameDataFunction::setGameFlagPowerStarSuccess(first_name.c_str(), 1, false);
    require(GameDataFunction::getPowerStarNumOwned(first_name.c_str()) == first_count - 1 &&
                MR::getPowerStarNum() == expected_total - 1 && previous.calcCurrentPowerStarNum() == expected_total,
            "loaded individual bits stay mutable while the scene-start snapshot remains independent");
    std::cout << "PASS original galaxy records=" << galaxy_count << " stars=" << expected_total
              << " hidden=" << hidden_stars << " grand=" << grand_stars << '\n';
}
}
int main() { return smgpc::test::run_stage_resource_process("star-storage-owner", verify_star_storage); }
