#include "Game/Util/EventUtil.hpp"

#include "Game/System/GameDataFunction.hpp"

namespace MR {
    bool useStageSwitchWriteA(LiveActor*, const JMapInfoIter&) {
        return false;
    }

    void onSwitchA(LiveActor*) {
    }

    void offSwitchA(LiveActor*) {
    }

    s32 getPictureBookChapterCanRead() {
        return GameDataFunction::getPictureBookChapterCanRead();
    }

    s32 getPictureBookChapterAlreadyRead() {
        return GameDataFunction::getPictureBookChapterAlreadyRead();
    }

    void setPictureBookChapterAlreadyRead(int chapterAlreadyRead) {
        GameDataFunction::setPictureBookChapterAlreadyRead(chapterAlreadyRead);
    }
}  // namespace MR
