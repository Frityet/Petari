#pragma once

#include "Game/System/BinaryDataChunkHolder.hpp"

class ConfigDataMii : public BinaryDataChunkBase {
public:
    ConfigDataMii();
    ~ConfigDataMii() override;

    u32 makeHeaderHashCode() const override;
    u32 getSignature() const override;
    s32 serialize(u8 *, u32) const override;
    s32 deserialize(const u8 *, u32) override;
    void initializeData() override;

    void setMiiOrIconId(const void *, const u32 *);
    bool getIconId(u32 *) const;
    bool getMiiId(void *) const;

private:
    /* 0x04 */ u8 mFlag;
    /* 0x05 */ u8 mIconId;
    /* 0x08 */ u8 *mMiiId;
};
