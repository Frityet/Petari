#pragma once

#include <revolution.h>

class NANDRequestInfo;

using NANDReqFunc = void(NANDRequestInfo*);

class NANDRequestInfo {
public:
    NANDRequestInfo();

    void init();
    [[nodiscard]] bool isDone() const;
    const char* setMove(const char* pPath, const char* pDestDir);
    const char* setWriteSeq(const char* pName, const void* pBuf, u32 fsBlock, u8 permission, u8 attr);
    const char* setReadSeq(const char* pName, void* pBuf, u32 fsBlock, u32* pLength);
    const char* setCheck(u32 fsBlock, u32 inode, u32* pAnswer);
    const char* setDelete(const char* pName);

    /* 0x00 */ char mPath[NAND_MAX_PATH];
    /* 0x40 */ u32 _40;
    /* 0x44 */ s32 mType;
    /* 0x48 */ s32 mResult;
    /* 0x4C */ union {
        void* mReadBuf;
        const void* mWriteBuf;
    };
    /* 0x50 */ union {
        const char* mMoveDestDir;
        u32* mReadLength;
        u32* mCheckAnswer;
    };
    /* 0x54 */ NANDReqFunc* _54;
    /* 0x58 */ u8 mPermission;
    /* 0x59 */ u8 mAttribute;
    /* 0x5C */ u32 mFsBlock;
    /* 0x60 */ u32 mInode;
    /* 0x64 */ u8 _64[0x1C];
};

class NANDManager {
public:
    NANDManager();

    bool addRequest(NANDRequestInfo* pRequestInfo);
};

class NANDResultCode {
public:
    explicit NANDResultCode(s32 code) : mCode(code) {}

    [[nodiscard]] s32 getCode() const;
    [[nodiscard]] bool isSuccess() const;
    [[nodiscard]] bool isSaveDataCorrupted() const;
    [[nodiscard]] bool isNANDCorrupted() const;
    [[nodiscard]] bool isMaxBlocks() const;
    [[nodiscard]] bool isMaxFiles() const;
    [[nodiscard]] bool isNoExistFile() const;
    [[nodiscard]] bool isBusyOrAllocFailed() const;
    [[nodiscard]] bool isUnknown() const;

private:
    /* 0x0 */ s32 mCode;
};

namespace MR {
    void addRequestToNANDManager(NANDRequestInfo*);
}
