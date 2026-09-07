#include "Game/System/ResourceHolder.hpp"
#include "Game/Util/ObjUtil.hpp"
#include <cstdio>

namespace MR {
    void* loadResourceFromArc(const char* pArchive, const char* pFile) {
        return createAndAddResourceHolder(pArchive)->mFileInfoTable->getRes(pFile);
    }

    const ResTIMG* loadTexFromArc(const char* pArchive, const char* pFile) {
        return static_cast<const ResTIMG*>(loadResourceFromArc(pArchive, pFile));
    }

    const ResTIMG* loadTexFromArc(const char* pArchive) {
        char arcBuf[256];
        snprintf(arcBuf, sizeof(arcBuf), "%s.arc", pArchive);
        char texBuf[256];
        snprintf(texBuf, sizeof(texBuf), "%s.bti", pArchive);
        return loadTexFromArc(arcBuf, texBuf);
    }
}
