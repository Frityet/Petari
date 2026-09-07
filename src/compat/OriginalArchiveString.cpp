#include "Game/Util/StringUtil.hpp"
#include <cstdio>
#include <cstring>
#include <stdint.h>

namespace MR {
char* removeExtensionString(char* pDst, u32 size, const char* pPath) {
        snprintf(pDst, size, "%s", pPath);

        char* pExtSeparator = strrchr(pDst, '.');
        char* pDirSeparator = strrchr(pDst, '/');

        if (reinterpret_cast< uintptr_t >(pExtSeparator) < reinterpret_cast< uintptr_t >(pDirSeparator) ||
            reinterpret_cast< uintptr_t >(pDirSeparator) + 1 == reinterpret_cast< uintptr_t >(pExtSeparator)) {
            return pDirSeparator;
        }

        *pExtSeparator = '\0';

        return pDirSeparator;
    }

void copyString(char* pDst, const char* pSrc, u32 num) {
        strncpy(pDst, pSrc, num);
    }

const char* getBasename(const char* pPath) {
        const char* pBasename = strrchr(pPath, '/');

        if (pBasename == nullptr) {
            return pPath;
        }

        return pBasename + 1;
    }
}
