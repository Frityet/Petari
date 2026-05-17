#pragma once

#include <revolution.h>

namespace GameDataFunction {
    const wchar_t* getUserName();
    u16 getUserFileIndex();
    s32 getPictureBookChapterCanRead();
    u16 getPictureBookChapterAlreadyRead();
    void setPictureBookChapterAlreadyRead(int chapterAlreadyRead);
    OSTime getSysConfigFileTimeAnnounced();
    void updateSysConfigFileTimeAnnounced();
    OSTime getSysConfigFileTimeSent();
    void setSysConfigFileTimeSent(OSTime time);
    u32 getSysConfigFileSentBytes();
    void setSysConfigFileSentBytes(u32 bytes);
}
