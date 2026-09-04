#include "JSystem/JKernel/JKRArchive.hpp"

void* JKRArchive::getIdxResource(u32 fileIndex) {
    SDIFileEntry* file = findIdxResource(fileIndex);

    if (file != nullptr) {
        return fetchResource(file, 0);
    }

    return nullptr;
}

void* JKRMemArchive::fetchResource(SDIFileEntry* pFile, u32* pSize) {
    if (pFile->mFileData == nullptr) {
        pFile->mFileData = mFileDataStart + pFile->mDataOffset;
    }

    if (pSize != nullptr) {
        *pSize = pFile->mDataSize;
    }

    return pFile->mFileData;
}
