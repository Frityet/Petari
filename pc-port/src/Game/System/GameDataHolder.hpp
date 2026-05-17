#pragma once

#include <revolution.h>

class UserFile;

class GameDataHolder {
public:
    explicit GameDataHolder(const UserFile* pUserFile);

    [[nodiscard]] bool isDataMario() const;
    [[nodiscard]] s32 getPlayerMissNum() const;
    void incPlayerMissNum();
    [[nodiscard]] bool isOnGameEventFlag(const char* pName) const;
    void tryOnGameEventFlag(const char* pName);
    void resetAllData();
    [[nodiscard]] s32 getStockedStarPieceNum() const;
    void addStockedStarPiece(int num);
    [[nodiscard]] s32 calcCurrentPowerStarNum() const;
    [[nodiscard]] bool isCompleteMarioAndLuigi() const;
    s32 makeFileBinary(u8* pBuffer, u32 size);
    bool loadFromFileBinary(const char* pName, const u8* pBuffer, u32 size);

private:
    void setName(const char* pName);
    void setCompatCounts(s32 powerStarNum, s32 starPieceNum, s32 playerMissNum);
    void setCompatEndingFlags(bool viewNormalEnding, bool viewCompleteEnding, bool finalChallengeStar);

    friend class UserFile;

public:
    char mName[16]{};

private:
    const UserFile* mUserFile = nullptr;
    s32 mPowerStarNum = 0;
    s32 mStarPieceNum = 0;
    s32 mPlayerMissNum = 0;
    bool mViewNormalEnding = false;
    bool mViewCompleteEnding = false;
    bool mFinalChallengeStar = false;
};
