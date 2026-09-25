#include "JSystem/JAudio2/JASReport.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include <revolution/os/OSThread.h>
#include <cstdio>
#include <cstdarg>
#include <dolphin/os/OSMutex.h>

static OSMutex sMutex;
static char* sBuffer;
static int sLineMax;
static int sLineCount;
static int sTop;

void JASReport(const char* pFormat, ...) {
    va_list vl;
    if (sBuffer == nullptr) {
        return;
    }

    va_start(vl, pFormat);
    OSMutex* pMutex = &sMutex;
    OSLockMutex(pMutex);
    vsnprintf(sBuffer + (sTop * 64), 64, pFormat, vl);
    va_end(vl);

    sTop++;
    if (sTop >= sLineMax) {
        sTop = 0;
    }

    if (sLineCount < sLineMax) {
        sLineCount++;
    }

    OSUnlockMutex(pMutex);
}
