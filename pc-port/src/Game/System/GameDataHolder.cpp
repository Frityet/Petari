#include "Game/System/GameDataHolder.hpp"

#include <cstring>
#include <cstdio>

#include "Game/System/UserFile.hpp"

namespace {
    struct GameDataBinary {
        char magic[4];
        s32 power_star_num;
        s32 star_piece_num;
        s32 player_miss_num;
        bool view_normal_ending;
        bool view_complete_ending;
        bool final_challenge_star;
    };
}  // namespace

GameDataHolder::GameDataHolder(const UserFile* pUserFile) : mUserFile(pUserFile) {
    setName("mario1");
}

bool GameDataHolder::isDataMario() const {
    return mUserFile == nullptr || mUserFile->mIsPlayerMario;
}

s32 GameDataHolder::getPlayerMissNum() const {
    return mPlayerMissNum;
}

void GameDataHolder::incPlayerMissNum() {
    ++mPlayerMissNum;
}

bool GameDataHolder::isOnGameEventFlag(const char* pName) const {
    if (pName == nullptr) {
        return false;
    }

    if (std::strcmp(pName, "ViewNormalEnding") == 0) {
        return mViewNormalEnding;
    }
    if (std::strcmp(pName, "ViewCompleteEnding") == 0) {
        return mViewCompleteEnding;
    }
    if (std::strcmp(pName, "SpecialStarFinalChallenge") == 0) {
        return mFinalChallengeStar;
    }

    return false;
}

void GameDataHolder::tryOnGameEventFlag(const char* pName) {
    if (pName == nullptr) {
        return;
    }

    if (std::strcmp(pName, "ViewNormalEnding") == 0) {
        mViewNormalEnding = true;
    } else if (std::strcmp(pName, "ViewCompleteEnding") == 0) {
        mViewCompleteEnding = true;
    } else if (std::strcmp(pName, "SpecialStarFinalChallenge") == 0) {
        mFinalChallengeStar = true;
    }
}

void GameDataHolder::resetAllData() {
    mPowerStarNum = 0;
    mStarPieceNum = 0;
    mPlayerMissNum = 0;
    mViewNormalEnding = false;
    mViewCompleteEnding = false;
    mFinalChallengeStar = false;
}

s32 GameDataHolder::getStockedStarPieceNum() const {
    return mStarPieceNum;
}

void GameDataHolder::addStockedStarPiece(int num) {
    mStarPieceNum += num;
}

s32 GameDataHolder::calcCurrentPowerStarNum() const {
    return mPowerStarNum;
}

bool GameDataHolder::isCompleteMarioAndLuigi() const {
    return mViewCompleteEnding && mFinalChallengeStar;
}

s32 GameDataHolder::makeFileBinary(u8* pBuffer, u32 size) {
    if (pBuffer == nullptr || size < sizeof(GameDataBinary)) {
        return 0;
    }

    const auto binary = GameDataBinary{
        .magic = {'G', 'A', 'M', '1'},
        .power_star_num = mPowerStarNum,
        .star_piece_num = mStarPieceNum,
        .player_miss_num = mPlayerMissNum,
        .view_normal_ending = mViewNormalEnding,
        .view_complete_ending = mViewCompleteEnding,
        .final_challenge_star = mFinalChallengeStar,
    };
    std::memcpy(pBuffer, &binary, sizeof(binary));
    return static_cast<s32>(sizeof(binary));
}

bool GameDataHolder::loadFromFileBinary(const char* pName, const u8* pBuffer, u32 size) {
    setName(pName);
    if (pBuffer == nullptr || size < sizeof(GameDataBinary)) {
        resetAllData();
        return true;
    }

    auto binary = GameDataBinary{};
    std::memcpy(&binary, pBuffer, sizeof(binary));
    if (std::memcmp(binary.magic, "GAM1", 4U) != 0) {
        resetAllData();
        return false;
    }

    mPowerStarNum = binary.power_star_num;
    mStarPieceNum = binary.star_piece_num;
    mPlayerMissNum = binary.player_miss_num;
    mViewNormalEnding = binary.view_normal_ending;
    mViewCompleteEnding = binary.view_complete_ending;
    mFinalChallengeStar = binary.final_challenge_star;
    return true;
}

void GameDataHolder::setName(const char* pName) {
    std::snprintf(mName, sizeof(mName), "%s", pName != nullptr ? pName : "mario1");
}

void GameDataHolder::setCompatCounts(s32 powerStarNum, s32 starPieceNum, s32 playerMissNum) {
    mPowerStarNum = powerStarNum;
    mStarPieceNum = starPieceNum;
    mPlayerMissNum = playerMissNum;
}

void GameDataHolder::setCompatEndingFlags(bool viewNormalEnding, bool viewCompleteEnding, bool finalChallengeStar) {
    mViewNormalEnding = viewNormalEnding;
    mViewCompleteEnding = viewCompleteEnding;
    mFinalChallengeStar = finalChallengeStar;
}
