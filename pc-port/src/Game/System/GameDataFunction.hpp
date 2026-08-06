#pragma once

#include <revolution.h>

namespace GameDataFunction {
    const wchar_t* getUserName();
    u16 getUserFileIndex();
    void onGameEventFlag(const char* pName);
    bool isOnGameEventFlag(const char* pName);
    bool isPassedStoryEvent(const char* pStoryEventName);
    void followStoryEventByName(const char* pStoryEventName);
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
