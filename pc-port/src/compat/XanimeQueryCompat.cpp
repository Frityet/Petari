#include "Game/Animation/XanimePlayer.hpp"
#include "Game/Animation/XanimeResource.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "Game/System/ResourceInfo.hpp"
#include "Game/Util/HashUtil.hpp"
#include "Game/Util/StringUtil.hpp"

#include <cstring>

// Original Xanime animation-name query and its resource lookup closure.
// This does not create or fabricate Xanime state; callers need a constructed
// player and original resource tables. See notes/original-camera-target-player-20260903.
namespace {

const unsigned char __lower_mapC[0x100] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    ' ',  '!',  '"',  '#',  '$',  '%',  '&',  '\'', '(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
    '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
    '@',  'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
    'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',  '[',  '\\', ']',  '^',  '_',
    '`',  'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
    'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',  '{',  '|',  '}',  '~',  0x7F,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

}  // namespace

bool XanimePlayer::isRun(const char* pName) const {
    if (mCurrentAnimation == nullptr) {
        return false;
    }

    XanimeGroupInfo* group = getSimpleGroup();
    if (mCurrentAnimation == group) {
        return mResourceTable->findResMotion(pName) == getSimpleGroup()->_20[0];
    }

    return mCurrentAnimation == mResourceTable->getGroupInfo(pName);
}

XanimeGroupInfo* XanimePlayer::getSimpleGroup() const {
    if (mSimpleGroup != nullptr) {
        return mSimpleGroup;
    }

    return &mResourceTable->_1C;
}

const XanimeGroupInfo* XanimeResourceTable::getGroupInfo(const char* pArg) const {
    switch (_0) {
    case 0:
        return nullptr;

    case 1: {
        s32 groupIndex = getGroupIndex(pArg);
        if (groupIndex == -1) {
            s32 simpleIndex = getSimpleIndex(pArg);
            if (simpleIndex == -1) {
                return nullptr;
            }

            return &mSimpleGroupInfos[simpleIndex];
        }

        return &mGroupInfos[groupIndex];
    }

    case 2:
        return getGroupInfo(pArg, mDirectories);
    }

    return nullptr;
}

const XanimeGroupInfo* XanimeResourceTable::getGroupInfo(const char* pPath, XanimeDirectory* pDir) const {
    int i = 0;
    while (true) {
        i++;

        if (pPath[i] == '\0') {
            break;
        }

        if (pPath[i] == '/') {
            char pathOneDirectoryDown[32];
            MR::extractString(pathOneDirectoryDown, &pPath[i], i, 32);

            u32 index = getIndex(pDir, pathOneDirectoryDown);

            switch (pDir[index].mDirectoryType) {
            case XanimeDirectory::Recursive:
                return getGroupInfo(&pPath[i + 1], pDir[index].mSubDirectories);

            case XanimeDirectory::Leaf:
                u32 hash = MR::getHashCode(&pPath[i + 1]);
                XanimeGroupInfo* infos = pDir[index].mSubInformations;

                for (int j = 0; j < pDir[index].mSize; j++) {
                    if (infos[j].mHash == hash) {
                        return &infos[j];
                    }
                }

                return nullptr;
            }
        }
    }

    i = 0;
    u32 hash = MR::getHashCode(pPath);
    const XanimeGroupInfo* result = nullptr;

    while (true) {
        XanimeDirectory* entry = &pDir[i];
        if (entry->mName[0] == '\0') {
            break;
        }

        switch (entry->mDirectoryType) {
        case XanimeDirectory::Recursive:
            result = getGroupInfo(pPath, entry->mSubDirectories);
            break;
        case XanimeDirectory::Leaf:
            XanimeGroupInfo* subInfo = entry->mSubInformations;
            for (int j = 0; j < entry->mSize; j++) {
                if (subInfo[j].mHash == hash) {
                    return &entry->mSubInformations[j];
                }
            }
            break;
        }

        if (result != nullptr) {
            return result;
        }
        i++;
    }

    return nullptr;
}

u32 XanimeResourceTable::getIndex(XanimeDirectory* pDir, const char* pTarget) const {
    int i = 0;
    u32 hash = MR::getHashCode(pTarget);

    while (true) {
        if (pDir[i].mName[0] == '\0') {
            return -1;
        }

        if (hash == pDir[i].mHash) {
            return i;
        }

        i++;
    }
}

u32 XanimeResourceTable::getGroupIndex(const char* pTarget) const {
    u32 hash = MR::getHashCode(pTarget);

    switch (_0) {
    case 0:
        return -1;
    case 1:
        u32 position;
        if (mSortTable->search(hash, &position)) {
            return position;
        }
    case 2:
        break;
    }

    return -1;
}

u32 XanimeResourceTable::getSimpleIndex(const char* pTarget) const {
    u32 hash = MR::getHashCode(pTarget);

    for (int i = 0; i < mAmountOfSimpleGroupInfos; i++) {
        if (hash == mSimpleGroupInfos[i].mHash) {
            return i;
        }
    }

    return -1;
}

void* XanimeResourceTable::findResMotion(const char* pTarget) const {
    const char* newName = swapBckName(pTarget, mSwapTable);
    if (!mResourceHolder->mMotionResTable->isExistRes(newName)) {
        return nullptr;
    }
    return mResourceHolder->mMotionResTable->getRes(newName);
}

const char* XanimeResourceTable::swapBckName(const char* pOriginal, XanimeSwapTable* pTable) const {
    if (pTable == nullptr) {
        return pOriginal;
    }

    int i = 0;

    while (true) {
        if (pTable[i].mOriginal[0] == '\0') {
            return pOriginal;
        }

        if (MR::strcasecmp(pTable[i].mOriginal, pOriginal) == 0) {
            return pTable[i].mSwapped;
        }
        i++;
    }
}

void* ResTable::getRes(const char* pName) const {
    return findRes(pName);
}

bool ResTable::isExistRes(const char* pRes) const {
    return getResIndex(pRes) != -1;
}

void* ResTable::findRes(const char* pName) const {
    int idx = getResIndex(pName);

    if (idx != -1) {
        return mFileInfoTable[idx].mResource;
    }

    return 0;
}

int ResTable::getResIndex(const char* pName) const {
    u32 hash = MR::getHashCodeLower(pName);

    for (int i = 0; i < mCount; i++) {
        u32 curHash = mFileInfoTable[i].mHashCode;

        if (curHash == hash) {
            return i;
        }
    }

    return -1;
}

bool HashSortTable::search(u32 a1, u32* a2) {
    u8 upperByte = a1 >> 24;

    if (a2 != nullptr) {
        *a2 = 0;
    }

    u32 count = _10[upperByte];

    if (count == 0) {
        return false;
    }

    u32* thing = &mHashCodes[_C[upperByte]];

    for (int i = 0; i < count; i++) {
        if (*thing == a1) {
            if (a2 != nullptr) {
                *a2 = _8[_C[upperByte] + i];
            }

            return true;
        }

        if (*thing > a1) {
            return false;
        }

        thing++;
    }

    return false;
}

namespace MR {

    void extractString(char* pDst, const char* pSrc, u32 num, u32) {
        strncpy(pDst, pSrc, num);

        pDst[num] = '\0';
    }

    // Original MSL C-locale lower-case table. Preserve resource-name bytes on
    // hosts with signed char, without depending on the host process locale.

    u32 getHashCodeLower(const char* pStr) {
        u32 hash;

        for (hash = 0; *pStr != '\0'; pStr++) {
            hash = __lower_mapC[static_cast<u8>(*pStr)] + hash * 31;
        }

        return hash;
    }

}  // namespace MR
