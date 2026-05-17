#pragma once

#include <revolution/types.h>

namespace MR {
    s32 getPictureBookChapterCanRead();
    s32 getPictureBookChapterAlreadyRead();
    void setPictureBookChapterAlreadyRead(int chapterAlreadyRead);
}  // namespace MR
