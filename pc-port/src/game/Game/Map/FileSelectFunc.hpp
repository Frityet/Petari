#pragma once

#include "compat/Types.hpp"

class FileSelectIconID;

namespace FileSelectFunc {

    u32 getMiiNameBufferSize();
    void copyMiiName(u16* pName, const FileSelectIconID& rIcon);

}  // namespace FileSelectFunc
