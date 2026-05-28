#pragma once

#include <revolution.h>

#include "runtime/RuntimeServices.hpp"

class ConfigDataHolder;
class GameDataHolder;

class UserFile {
public:
    UserFile();
    UserFile(const UserFile& rOther);
    UserFile& operator=(const UserFile& rOther);
    ~UserFile();

    [[nodiscard]] bool isCreated() const;
    [[nodiscard]] s32 getPowerStarNum() const;
    [[nodiscard]] s32 getStarPieceNum() const;
    [[nodiscard]] s32 getPlayerMissNum() const;
    [[nodiscard]] bool getMiiId(void* pMiiId) const;
    [[nodiscard]] bool getIconId(u32* pIconId) const;
    [[nodiscard]] bool isLastLoadedMario() const;
    [[nodiscard]] bool isOnCompleteEndingMarioAndLuigi() const;
    [[nodiscard]] OSTime getLastModified() const;
    void setCreated();
    void setMiiOrIconId(const void* pMiiId, const u32* pIconId);
    void setLastLoadedMario(bool lastLoadedMario);
    void onCompleteEndingCurrentPlayer();
    void updateLastModified();
    void setUserName(const wchar_t* pUserName);
    [[nodiscard]] const char* getGameDataName() const;
    void makeGameDataBinary(u8* pBuffer, u32 size) const;
    void loadFromGameDataBinary(const char* pName, const u8* pBuffer, u32 size);
    [[nodiscard]] const char* getConfigDataName() const;
    void makeConfigDataBinary(u8* pBuffer, u32 size) const;
    void loadFromConfigDataBinary(const char* pName, const u8* pBuffer, u32 size);
    void resetAllData();
    [[nodiscard]] bool isViewNormalEnding() const;
    [[nodiscard]] bool isViewCompleteEnding() const;
    [[nodiscard]] bool isPowerStarGetFinalChallengeGalaxy() const;
    void restoreFromSaveDataServiceSlot(const smgpc::compat::SaveDataService::SlotState& rSlot, s32 slotIndex, bool isPlayerMario);
    [[nodiscard]] smgpc::compat::SaveDataService::SlotState makeSaveDataServiceSlot(s32 slotIndex) const;

    /* 0x00 */ GameDataHolder* mGameDataHolder;
    /* 0x04 */ ConfigDataHolder* mConfigDataHolder;
    /* 0x08 */ bool mIsPlayerMario;
    /* 0x09 */ bool mIsGameDataCorrupted;
    /* 0x0A */ bool mIsConfigDataCorrupted;
    /* 0x0C */ wchar_t* mUserName;
};
