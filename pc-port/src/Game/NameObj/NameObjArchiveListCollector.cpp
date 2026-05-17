#include "Game/NameObj/NameObjArchiveListCollector.hpp"

#include <algorithm>
#include <cstring>

NameObjArchiveListCollector::NameObjArchiveListCollector() {
    mCount = 0;
}

void NameObjArchiveListCollector::addArchive(const char* pArchive) {
    char* dst = mArchiveNames[mCount];
    std::fill_n(dst, 0x40, '\0');

    if (pArchive != nullptr) {
        std::strncpy(dst, pArchive, 0x3F);
    }

    mCount++;
}

const char* NameObjArchiveListCollector::getArchive(s32 idx) const {
    return mArchiveNames[idx];
}
