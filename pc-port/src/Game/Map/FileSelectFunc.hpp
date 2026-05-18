#pragma once

#include <revolution.h>

class FileSelectIconID;

namespace FileSelectFunc {
    u32 getMiiNameBufferSize();
    void copyMiiName(u16* pName, const FileSelectIconID& rIcon);
}  // namespace FileSelectFunc
