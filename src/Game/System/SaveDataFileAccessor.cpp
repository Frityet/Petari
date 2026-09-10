#include "Game/System/SaveDataFileAccessor.hpp"
#include "Game/Util/StringUtil.hpp"
#include <cstring>

SaveDataFileAccessor::SaveDataFileAccessor(u8* pData) : mFile(reinterpret_cast< SaveDataFile* >(pData)) {
}

SaveDataFileHeader* SaveDataFileAccessor::getHeader() {
    return &mFile->mHeader;
}

SaveDataFileInfo* SaveDataFileAccessor::getFileInfo(int index) {
    return &mFile->mInfo[index];
}

void SaveDataFileAccessor::makeUserFileInfo(SaveDataUserFileInfo* pInfo, const char* pName) {
    pInfo->mData = nullptr;
    pInfo->mDataSize = 0;
    pInfo->mKind = 1;

    SaveDataFile* pFile = mFile;
    for (u32 i = 0; i < pFile->mHeader.mUserFileInfoNum; i++) {
        SaveDataFileInfo* pFileInfo = &pFile->mInfo[i];
        if (MR::isEqualString(pFileInfo->mName, pName)) {
            if (i == pFile->mHeader.mUserFileInfoNum - 1) {
                pInfo->mDataSize = pFile->mHeader.mFileSize - pFileInfo->mOffset;
            } else {
                pInfo->mDataSize = pFile->mInfo[i + 1].mOffset - pFileInfo->mOffset;
            }
            pInfo->mData = reinterpret_cast< u8* >(mFile) + pFileInfo->mOffset;
            if (strstr(pFileInfo->mName, "mario") != nullptr || strstr(pFileInfo->mName, "luigi") != nullptr) {
                pInfo->mKind = 0;
            }
            if (strstr(pFileInfo->mName, "sysconf") != nullptr) {
                pInfo->mKind = 2;
            }
        }
    }
}
