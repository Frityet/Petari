#include "Game/System/UserFile.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cwchar>

#include "Game/System/ConfigDataHolder.hpp"
#include "Game/System/GameDataHolder.hpp"

namespace {
    constexpr auto cUserNameSize = std::size_t{11U};

    void make_config_name(char* pBuffer, std::size_t size, s32 slotIndex) {
        std::snprintf(pBuffer, size, "config%1d", slotIndex);
    }

    void make_game_name(char* pBuffer, std::size_t size, s32 slotIndex, bool isPlayerMario) {
        std::snprintf(pBuffer, size, "%s%1d", isPlayerMario ? "mario" : "luigi", slotIndex);
    }
}  // namespace

UserFile::UserFile()
    : mGameDataHolder(new GameDataHolder(this)), mConfigDataHolder(new ConfigDataHolder()), mIsPlayerMario(true), mIsGameDataCorrupted(false),
      mIsConfigDataCorrupted(false), mUserName(new wchar_t[cUserNameSize]{}) {
}

UserFile::UserFile(const UserFile& rOther) : UserFile() {
    *this = rOther;
}

UserFile& UserFile::operator=(const UserFile& rOther) {
    if (this == &rOther) {
        return *this;
    }

    mIsPlayerMario = rOther.mIsPlayerMario;
    mIsGameDataCorrupted = rOther.mIsGameDataCorrupted;
    mIsConfigDataCorrupted = rOther.mIsConfigDataCorrupted;
    *mGameDataHolder = *rOther.mGameDataHolder;
    *mConfigDataHolder = *rOther.mConfigDataHolder;
    mGameDataHolder->mUserFile = this;
    std::wmemcpy(mUserName, rOther.mUserName, cUserNameSize);
    return *this;
}

UserFile::~UserFile() {
    delete mGameDataHolder;
    delete mConfigDataHolder;
    delete[] mUserName;
}

bool UserFile::isCreated() const {
    return mConfigDataHolder->isCreated();
}

s32 UserFile::getPowerStarNum() const {
    return mGameDataHolder->calcCurrentPowerStarNum();
}

s32 UserFile::getStarPieceNum() const {
    return mGameDataHolder->getStockedStarPieceNum();
}

s32 UserFile::getPlayerMissNum() const {
    return mGameDataHolder->getPlayerMissNum();
}

bool UserFile::getMiiId(void* pMiiId) const {
    return mConfigDataHolder->getMiiId(pMiiId);
}

bool UserFile::getIconId(u32* pIconId) const {
    return mConfigDataHolder->getIconId(pIconId);
}

bool UserFile::isLastLoadedMario() const {
    return mConfigDataHolder->isLastLoadedMario();
}

bool UserFile::isOnCompleteEndingMarioAndLuigi() const {
    return mConfigDataHolder->isOnCompleteEndingMario() && mConfigDataHolder->isOnCompleteEndingLuigi();
}

OSTime UserFile::getLastModified() const {
    return mConfigDataHolder->getLastModified();
}

void UserFile::setCreated() {
    mConfigDataHolder->setIsCreated(true);
}

void UserFile::setMiiOrIconId(const void* pMiiId, const u32* pIconId) {
    mConfigDataHolder->setMiiOrIconId(pMiiId, pIconId);
}

void UserFile::setLastLoadedMario(bool lastLoadedMario) {
    mConfigDataHolder->setLastLoadedMario(lastLoadedMario);
}

void UserFile::onCompleteEndingCurrentPlayer() {
    if (mIsPlayerMario) {
        mConfigDataHolder->onCompleteEndingMario();
    } else {
        mConfigDataHolder->onCompleteEndingLuigi();
    }
}

void UserFile::updateLastModified() {
    mConfigDataHolder->updateLastModified();
}

void UserFile::setUserName(const wchar_t* pUserName) {
    std::wmemset(mUserName, 0, cUserNameSize);
    if (pUserName != nullptr) {
        std::wmemcpy(mUserName, pUserName, std::min(cUserNameSize - 1U, std::wcslen(pUserName)));
    }
}

const char* UserFile::getGameDataName() const {
    return mGameDataHolder->mName;
}

void UserFile::makeGameDataBinary(u8* pBuffer, u32 size) const {
    mGameDataHolder->makeFileBinary(pBuffer, size);
}

void UserFile::loadFromGameDataBinary(const char* pName, const u8* pBuffer, u32 size) {
    mIsGameDataCorrupted = !mGameDataHolder->loadFromFileBinary(pName, pBuffer, size);
}

const char* UserFile::getConfigDataName() const {
    return mConfigDataHolder->mName;
}

void UserFile::makeConfigDataBinary(u8* pBuffer, u32 size) const {
    mConfigDataHolder->makeFileBinary(pBuffer, size);
}

void UserFile::loadFromConfigDataBinary(const char* pName, const u8* pBuffer, u32 size) {
    mIsConfigDataCorrupted = !mConfigDataHolder->loadFromFileBinary(pName, pBuffer, size);
}

void UserFile::resetAllData() {
    mGameDataHolder->resetAllData();
    mConfigDataHolder->resetAllData();
    mIsGameDataCorrupted = false;
    mIsConfigDataCorrupted = false;
}

bool UserFile::isViewNormalEnding() const {
    return mGameDataHolder->isOnGameEventFlag("ViewNormalEnding");
}

bool UserFile::isViewCompleteEnding() const {
    return mGameDataHolder->isOnGameEventFlag("ViewCompleteEnding");
}

bool UserFile::isPowerStarGetFinalChallengeGalaxy() const {
    return mGameDataHolder->isOnGameEventFlag("SpecialStarFinalChallenge");
}

void UserFile::restoreFromSaveDataServiceSlot(const smgpc::runtime::SaveDataService::SlotState& rSlot, s32 slotIndex, bool isPlayerMario) {
    resetAllData();

    auto configName = std::array< char, 16U >{};
    auto gameName = std::array< char, 16U >{};
    make_config_name(configName.data(), configName.size(), slotIndex);
    make_game_name(gameName.data(), gameName.size(), slotIndex, isPlayerMario);
    mConfigDataHolder->setName(configName.data());
    mGameDataHolder->setName(gameName.data());

    mIsPlayerMario = isPlayerMario;
    mIsGameDataCorrupted = rSlot.game_data_corrupted;
    mIsConfigDataCorrupted = rSlot.config_data_corrupted;
    mConfigDataHolder->setIsCreated(rSlot.created);
    mConfigDataHolder->setLastLoadedMario(rSlot.last_loaded_mario);
    mConfigDataHolder->mCompleteEndingMario = rSlot.complete_ending_mario_and_luigi || (rSlot.view_complete_ending && rSlot.last_loaded_mario);
    mConfigDataHolder->mCompleteEndingLuigi = rSlot.complete_ending_mario_and_luigi || (rSlot.view_complete_ending && !rSlot.last_loaded_mario);
    mConfigDataHolder->mLastModified = rSlot.last_modified;
    if (rSlot.has_mii_id) {
        mConfigDataHolder->setMiiIndex(rSlot.rfl_mii_index.value_or(0));
    } else {
        const auto iconId = rSlot.icon_id.value_or(1U);
        mConfigDataHolder->setMiiOrIconId(nullptr, &iconId);
    }
    mGameDataHolder->setSaveDataCounts(rSlot.power_star_num, rSlot.star_piece_num, rSlot.player_miss_num);
    mGameDataHolder->setEndingFlags(rSlot.view_normal_ending, rSlot.view_complete_ending, rSlot.view_complete_ending);
    mGameDataHolder->setEventState(rSlot.game_event_flags, rSlot.game_event_values);
}

smgpc::runtime::SaveDataService::SlotState UserFile::makeSaveDataServiceSlot(s32 slotIndex) const {
    auto iconId = u32{};
    const auto hasIconId = getIconId(&iconId);
    return smgpc::runtime::SaveDataService::SlotState{
        .slot_index = slotIndex,
        .created = isCreated(),
        .game_data_corrupted = mIsGameDataCorrupted,
        .config_data_corrupted = mIsConfigDataCorrupted,
        .last_loaded_mario = isLastLoadedMario(),
        .power_star_num = getPowerStarNum(),
        .star_piece_num = getStarPieceNum(),
        .player_miss_num = getPlayerMissNum(),
        .has_mii_id = !hasIconId,
        .rfl_mii_index = mConfigDataHolder->getMiiIndex(),
        .icon_id = hasIconId ? std::optional< u32 >(iconId) : std::nullopt,
        .view_normal_ending = isViewNormalEnding(),
        .view_complete_ending = isViewCompleteEnding(),
        .complete_ending_mario_and_luigi = isOnCompleteEndingMarioAndLuigi(),
        .game_event_flags = mGameDataHolder->getEventFlags(),
        .game_event_values = mGameDataHolder->getEventValues(),
        .last_modified = getLastModified(),
    };
}
