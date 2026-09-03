// Complete original ObjUtil CSV helpers over the bounded native JMap resource boundary.
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/System/ResourceHolder.hpp"
#include <cstdio>
#include <cstdarg>

namespace {
    bool isExistFileInArcLocal(const ResourceHolder* pHolder, const char* pArchive, va_list pFormat) NO_INLINE {
        char buf[0x100];
        vsnprintf(buf, sizeof(buf), pArchive, pFormat);

        return pHolder->mFileInfoTable->isExistRes(buf) != false;
    }

    JMapInfo* tryCreateCsvParserLocal(const ResourceHolder* pHolder, const char* pArchive, va_list pFormat) NO_INLINE {
        char buf[0x100];
        vsnprintf(buf, sizeof(buf), pArchive, pFormat);

        if (!pHolder->mFileInfoTable->isExistRes(buf)) {
            return nullptr;
        }

        JMapInfo* inf = new JMapInfo();
        inf->attach(pHolder->mFileInfoTable->getRes(buf));

        return inf;
    }
};  // namespace

namespace MR {
    bool isExistFileInArc(const ResourceHolder* pHolder, const char* pFormat, ...) {
        va_list list;
        va_start(list, pFormat);

        return ::isExistFileInArcLocal(pHolder, pFormat, list);
    }

    JMapInfo* createCsvParser(const ResourceHolder* pHolder, const char* pFormat, ...) {
        va_list list;
        va_start(list, pFormat);

        return ::tryCreateCsvParserLocal(pHolder, pFormat, list);
    }

    JMapInfo* tryCreateCsvParser(const LiveActor* pActor, const char* pFormat, ...) {
        va_list list;
        va_start(list, pFormat);

        return ::tryCreateCsvParserLocal(getResourceHolder(pActor), pFormat, list);
    }

    JMapInfo* tryCreateCsvParser(const ResourceHolder* pHolder, const char* pFormat, ...) {
        // exact same code as createCsvParser(const ResourceHolder*, const char*, ...)
        va_list list;
        va_start(list, pFormat);

        return ::tryCreateCsvParserLocal(pHolder, pFormat, list);
    }

    s32 getCsvDataElementNum(const JMapInfo* pMapInfo) {
        if (pMapInfo->mData != nullptr) {
            return pMapInfo->mData->mNumEntries;
        }

        return 0;
    }

    void getCsvDataStr(const char** pOut, const JMapInfo* pMapInfo, const char* pKey, s32 idx) {
        pMapInfo->getValue(idx, pKey, pOut);
    }

    void getCsvDataStrOrNULL(const char** pOut, const JMapInfo* pMapInfo, const char* pKey, s32 idx) {
        getCsvDataStr(pOut, pMapInfo, pKey, idx);

        if (*pOut[0] == 0) {
            *pOut = nullptr;
        }
    }

    void getCsvDataS32(s32* pOut, const JMapInfo* pMapInfo, const char* pKey, s32 idx) {
        pMapInfo->getValue< s32 >(idx, pKey, pOut);
    }

    void getCsvDataU8(u8* pOut, const JMapInfo* pMapInfo, const char* pKey, s32 idx) {
        s32 val = 0;
        pMapInfo->getValue< s32 >(idx, pKey, &val);
        *pOut = val;
    }

    void getCsvDataF32(f32* pOut, const JMapInfo* pMapInfo, const char* pKey, s32 idx) {
        pMapInfo->getValue< f32 >(idx, pKey, pOut);
    }

    void getCsvDataBool(bool* pOut, const JMapInfo* pMapInfo, const char* pKey, s32 idx) {
        pMapInfo->getValue< bool >(idx, pKey, pOut);
    }

    void getCsvDataVec(Vec* pOut, const JMapInfo* pMapInfo, const char* pKey, s32 idx) {
        char buf[0x100];
        snprintf(buf, sizeof(buf), "%sX", pKey);
        getCsvDataF32(&pOut->x, pMapInfo, buf, idx);
        snprintf(buf, sizeof(buf), "%sY", pKey);
        getCsvDataF32(&pOut->y, pMapInfo, buf, idx);
        snprintf(buf, sizeof(buf), "%sZ", pKey);
        getCsvDataF32(&pOut->z, pMapInfo, buf, idx);
    }
}  // namespace MR
