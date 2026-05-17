#include "Game/Util/EventUtil.hpp"

#include "Game/System/GameDataFunction.hpp"

namespace MR {
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
