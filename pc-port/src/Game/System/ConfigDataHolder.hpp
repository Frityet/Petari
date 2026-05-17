#pragma once

#include <array>
#include <optional>

#include <revolution.h>

class ConfigDataHolder {
public:
    ConfigDataHolder();

    void setIsCreated(bool isCreated);
    [[nodiscard]] bool isCreated() const;
    void setLastLoadedMario(bool lastLoadedMario);
    [[nodiscard]] bool isLastLoadedMario() const;
    void onCompleteEndingMario();
    void onCompleteEndingLuigi();
    [[nodiscard]] bool isOnCompleteEndingMario();
    [[nodiscard]] bool isOnCompleteEndingLuigi();
    void updateLastModified();
    [[nodiscard]] OSTime getLastModified() const;
    void setMiiOrIconId(const void* pMiiId, const u32* pIconId);
    [[nodiscard]] bool getMiiId(void* pMiiId) const;
    [[nodiscard]] bool getIconId(u32* pIconId) const;
    void resetAllData();
    s32 makeFileBinary(u8* pBuffer, u32 size);
    bool loadFromFileBinary(const char* pName, const u8* pBuffer, u32 size);
    void setCompatMiiIndex(std::optional<s32> miiIndex);
    [[nodiscard]] std::optional<s32> getCompatMiiIndex() const;

private:
    void setName(const char* pName);

    friend class UserFile;

    bool mIsCreated = false;
    bool mLastLoadedMario = true;
    bool mCompleteEndingMario = false;
    bool mCompleteEndingLuigi = false;
    bool mUsesMii = false;
    u32 mIconId = 1U;
    std::optional<s32> mMiiIndex{};
    std::array<u8, 16U> mMiiId{};
    OSTime mLastModified = 0;
    char mName[16]{};
};
