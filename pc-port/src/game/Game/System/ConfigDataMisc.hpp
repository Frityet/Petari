#pragma once

#include "Game/System/BinaryDataChunkHolder.hpp"

class ConfigDataMisc : public BinaryDataChunkBase {
public:
    ConfigDataMisc();

    u32 makeHeaderHashCode() const override;
    u32 getSignature() const override;
    s32 serialize(u8 *, u32) const override;
    s32 deserialize(const u8 *, u32) override;
    void initializeData() override;

    bool isLastLoadedMario() const;
    void setLastLoadedMario(bool);
    void onCompleteEndingMario();
    void onCompleteEndingLuigi();
    bool isOnCompleteEndingMario();
    bool isOnCompleteEndingLuigi();
    OSTime getLastModified() const;
    void updateLastModified();

private:
    /* 0x04 */ u8 mFlag;
    /* 0x08 */ OSTime mLastModified;
};
