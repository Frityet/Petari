#pragma once

#include <revolution.h>

class SysConfigFile {
public:
    SysConfigFile();

    [[nodiscard]] OSTime getTimeAnnounced() const;
    void setTimeAnnounced(OSTime time);
    void updateTimeAnnounced();
    [[nodiscard]] OSTime getTimeSent() const;
    void setTimeSent(OSTime time);
    [[nodiscard]] u32 getSentBytes() const;
    void setSentBytes(u32 bytes);
    void makeDataBinary(u8* pBuffer, u32 size) const;
    void loadFromDataBinary(const u8* pBuffer, u32 size);

private:
    OSTime mTimeAnnounced = 0;
    OSTime mTimeSent = 0;
    u32 mSentBytes = 0U;
};
