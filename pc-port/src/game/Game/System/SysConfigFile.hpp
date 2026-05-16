#pragma once

#include "Game/System/BinaryDataChunkHolder.hpp"

class BinaryDataChunkHolder;
class BinaryDataContentHeaderSerializer;

class SysConfigChunk : public BinaryDataChunkBase {
    friend class SysConfigFile;

public:
    SysConfigChunk();
    ~SysConfigChunk() override;

    u32 makeHeaderHashCode() const override;
    u32 getSignature() const override;
    s32 serialize(u8 *, u32) const override;
    s32 deserialize(const u8 *, u32) override;
    void initializeData() override;

    void initHeaderSerializer();

private:
    /* 0x08 */ OSTime mTimeAnnounced;
    /* 0x10 */ OSTime mTimeSent;
    /* 0x18 */ u32 mSentBytes;
    /* 0x1C */ BinaryDataContentHeaderSerializer *mHeaderSerializer;
};

class SysConfigFile {
public:
    SysConfigFile();
    ~SysConfigFile();

    OSTime getTimeAnnounced();
    void updateTimeAnnounced();
    OSTime getTimeSent();
    void setTimeSent(OSTime);
    u32 getSentBytes();
    void setSentBytes(u32);
    void makeDataBinary(u8 *, u32) const;
    void loadFromDataBinary(const u8 *, u32);

private:
    /* 0x00 */ SysConfigChunk *mChunk;
    /* 0x04 */ BinaryDataChunkHolder *mChunkHolder;
};
