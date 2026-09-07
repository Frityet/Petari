#include "Game/Util/StringUtil.hpp"

#include <cstdio>
#include <cstring>

// Original StringUtil scanning entry points. Keep the substring gate and the
// original scanf conversion formats shared by all resource text consumers.
namespace MR {
    void scan32(const char* pSrc, const char* pSubStr, s32* pDst) {
        if (strstr(pSrc, pSubStr) == nullptr) {
            return;
        }

        sscanf(pSrc, "\t%d", pDst);
    }

    void scan16(const char* pSrc, const char* pSubStr, u16* pDst) {
        int temp;

        if (strstr(pSrc, pSubStr) == nullptr) {
            return;
        }

        sscanf(pSrc, "\t%d", &temp);

        *pDst = temp;
    }

    void scan8(const char* pSrc, const char* pSubStr, u8* pDst) {
        int temp;

        if (strstr(pSrc, pSubStr) == nullptr) {
            return;
        }

        sscanf(pSrc, "\t%d", &temp);

        *pDst = temp;
    }

    void scanf32(const char* pSrc, const char* pSubStr, f32* pDst) {
        if (strstr(pSrc, pSubStr) == nullptr) {
            return;
        }

        sscanf(pSrc, "\t%ff", pDst);
    }

    void scanu8x4(const char* pSrc, const char* pSubStr, u8* pDst) {
        int temp[4];

        if (strstr(pSrc, pSubStr) == nullptr) {
            return;
        }

        sscanf(pSrc, "\t{%d,%d,%d,%d}", &temp[0], &temp[1], &temp[2], &temp[3]);

        for (int i = 0; i < ARRAY_SIZE(temp); i++) {
            pDst[i] = temp[i];
        }
    }

    void scans16x4(const char* pSrc, const char* pSubStr, s16* pDst) {
        int temp[4];

        if (strstr(pSrc, pSubStr) == nullptr) {
            return;
        }

        sscanf(pSrc, "\t{%d,%d,%d,%d}", &temp[0], &temp[1], &temp[2], &temp[3]);

        for (int i = 0; i < ARRAY_SIZE(temp); i++) {
            pDst[i] = temp[i];
        }
    }

    void scanf32x4(const char* pSrc, const char* pSubStr, f32* pDst) {
        if (strstr(pSrc, pSubStr) == nullptr) {
            return;
        }

        sscanf(pSrc, "\t{%ff,%ff,%ff,%ff}", &pDst[0], &pDst[1], &pDst[2], &pDst[3]);
    }
}
