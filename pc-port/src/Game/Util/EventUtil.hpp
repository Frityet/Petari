#pragma once

#include <revolution/types.h>

class JMapInfoIter;
class LiveActor;

namespace MR {
    bool useStageSwitchWriteA(LiveActor* pActor, const JMapInfoIter& rIter);
    void onSwitchA(LiveActor* pActor);
    void offSwitchA(LiveActor* pActor);
    s32 getPictureBookChapterCanRead();
    s32 getPictureBookChapterAlreadyRead();
    void setPictureBookChapterAlreadyRead(int chapterAlreadyRead);
}  // namespace MR
