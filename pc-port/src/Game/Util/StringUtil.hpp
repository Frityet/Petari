#pragma once

#include <revolution.h>

namespace MR {
    void makeDateString(wchar_t* pDst, s32 size, s32 year, s32 month, s32 day);
    void makeTimeString(wchar_t* pDst, s32 size, s32 hour, s32 minute);
}  // namespace MR
