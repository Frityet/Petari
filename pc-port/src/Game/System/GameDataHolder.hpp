#pragma once

#include <map>
#include <string>

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
    [[nodiscard]] u16 getGameEventValue(const char* pName) const;
    void setGameEventValue(const char* pName, u16 value);
    [[nodiscard]] s32 getPictureBookChapterCanRead() const;
    [[nodiscard]] u16 getPictureBookChapterAlreadyRead() const;
    void setPictureBookChapterAlreadyRead(int chapterAlreadyRead);
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
    void setCompatEventState(const std::map<std::string, bool>& rFlags, const std::map<std::string, u16>& rValues);
    [[nodiscard]] const std::map<std::string, bool>& getCompatEventFlags() const;
    [[nodiscard]] const std::map<std::string, u16>& getCompatEventValues() const;

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
    std::map<std::string, bool> mEventFlags;
    std::map<std::string, u16> mEventValues;
};
