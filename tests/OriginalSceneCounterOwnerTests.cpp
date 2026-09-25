#include "OriginalSaveDataSnapshot.hpp"
#include "OriginalStageResourceProcessFixture.hpp"
#include "Game/System/AlreadyDoneFlagInGalaxy.hpp"
#include "Game/System/GameDataTemporaryInGalaxy.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/Scene/ScenePlayingResult.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SequenceUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "Game/Util/HashUtil.hpp"
#include "Game/Util/JMapIdInfo.hpp"
#include <limits>
#include <stdexcept>
namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void verify_counters() {
    auto& sequence = smgpc::test::original_save_sequence();
    auto& current = *sequence.mCurrentUserFile;
    const smgpc::test::OriginalUserFileSnapshot restore_current(current);
    auto& profile = *current.mGameDataHolder;
    auto& data = *SingletonHolder<GameSystem>::get()->mSequenceDirector->mGameDataTemporaryInGalaxy;
    const auto star_pieces = data.mStarPieceNum;
    const auto last_one_up = data.mLast1upStarPieceNum;
    struct RestoreTemporary {
        GameDataTemporaryInGalaxy& data;
        int pieces, last;
        ~RestoreTemporary() { data.mStarPieceNum = pieces; data.mLast1upStarPieceNum = last; }
    } restore_temporary{data, star_pieces, last_one_up};
    data.resetStageResultStarPieceParam();
    GameDataFunction::addStarPiece(998);
    GameDataFunction::addStarPiece(9);
    require(GameDataFunction::getStarPieceNum() == 999 && data.mStarPieceNum == 999,
            "public stage counts use the original sequence's saturating temporary-data owner");
    GameDataFunction::addStarPiece(-1200);
    require(GameDataFunction::getStarPieceNum() == 0, "original Star Bit decrement clamps at zero");
    GameDataFunction::setLast1upStarPieceNum(150);
    require(data.mLast1upStarPieceNum == 150 && GameDataFunction::getLast1upStarPieceNum() == 150,
            "one-up threshold routes through the same original temporary owner");
    require(MR::getPlayerRestartIdInfo() == data.mPlayerRestartIdInfo,
            "restart accessor returns the actual temporary owner's ID object");
    require(data.mAlreadyDoneFlag && data.mAlreadyDoneFlag->mDoneInfos.size() == 64,
            "actual process initializes the original 64-entry done-flag owner");
    profile.addPlayerLeft(17 - profile.getPlayerLeft());
    require(MR::getPlayerLeft() == 17, "HUD life query reads the actual selected profile");
    MR::incPlayerLeft();
    require(MR::getPlayerLeft() == 18 && GameDataFunction::getPlayerLeft() == 18,
            "life increment and query use the same original profile owner");
    profile.setGameEventValue("MissPointForLetter", 19);
    profile.setGameEventValue("MissNum", 9998);
    MR::decPlayerLeft();
    require(MR::getPlayerLeft() == 17 && profile.getGameEventValue("MissPointForLetter") == 20 &&
                profile.getPlayerMissNum() == 9999,
            "one death must update lives, letter miss points and miss count in the same selected profile");
    MR::decPlayerLeft();
    require(MR::getPlayerLeft() == 16 && profile.getGameEventValue("MissPointForLetter") == 20 &&
                profile.getPlayerMissNum() == 9999,
            "death counters retain original upper saturation");
    GameDataFunction::addMissPoint(-99);
    require(profile.getGameEventValue("MissPointForLetter") == 0, "letter miss points clamp below zero");
    profile.setGameEventValue("MissPointForLetter", 1);
    GameDataFunction::addMissPoint(std::numeric_limits<s32>::max());
    require(profile.getGameEventValue("MissPointForLetter") == 0,
            "letter miss point addition preserves PPC unsigned wrap followed by signed clamp");
    profile.addPlayerLeft(-profile.getPlayerLeft());
    MR::decPlayerLeft();
    require(MR::getPlayerLeft() == 0 && profile.getGameEventValue("MissPointForLetter") == 1,
            "zero remaining lives still records the real death counters");
    require(MR::getPowerStarNum() == profile.calcCurrentPowerStarNum(), "HUD Power Star query uses actual profile state");
    ScenePlayingResult result;
    require(result.getCoinNum() == 0 && result.mPurpleCoinNum == 0, "original scene result initializes both coin counters");
    result.mCoinNum = -3; require(result.getCoinNum() == 0, "original coin query clamps its lower bound");
    result.mCoinNum = 1234; require(result.getCoinNum() == 999, "original coin query clamps its upper bound");
}
}
int main() { return smgpc::test::run_stage_resource_process("scene-counter-owner", verify_counters); }
