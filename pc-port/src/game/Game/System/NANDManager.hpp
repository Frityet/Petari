#pragma once

#include "compat/Types.hpp"

#include <cstdio>

constexpr s32 NAND_RESULT_OK = 0;
constexpr s32 NAND_RESULT_ACCESS = -1;
constexpr s32 NAND_RESULT_ALLOC_FAILED = -2;
constexpr s32 NAND_RESULT_BUSY = -3;
constexpr s32 NAND_RESULT_CORRUPT = -4;
constexpr s32 NAND_RESULT_ECC_CRIT = -5;
constexpr s32 NAND_RESULT_EXISTS = -6;
constexpr s32 NAND_RESULT_INVALID = -8;
constexpr s32 NAND_RESULT_MAXBLOCKS = -9;
constexpr s32 NAND_RESULT_MAXFD = -10;
constexpr s32 NAND_RESULT_MAXFILES = -11;
constexpr s32 NAND_RESULT_NOEXISTS = -12;
constexpr s32 NAND_RESULT_NOTEMPTY = -13;
constexpr s32 NAND_RESULT_OPENFD = -14;
constexpr s32 NAND_RESULT_AUTHENTICATION = -15;
constexpr s32 NAND_RESULT_UNKNOWN = -64;
constexpr int NAND_MAX_PATH = 64;

class NANDRequestInfo {
public:
    NANDRequestInfo();

    void init();
    bool isDone() const;
    const char *setMove(const char *, const char *);
    const char *setWriteSeq(const char *, const void *, u32, u8, u8);
    const char *setReadSeq(const char *, void *, u32, u32 *);
    const char *setCheck(u32, u32, u32 *);
    const char *setDelete(const char *);

    /* 0x00 */ char mPath[NAND_MAX_PATH];
    /* 0x40 */ u32 _40;
    /* 0x44 */ s32 mType;
    /* 0x48 */ s32 mResult;
    /* 0x4C */ union {
        void *mReadBuf;
        const void *mWriteBuf;
    };
    /* 0x50 */ union {
        const char *mMoveDestDir;
        u32 *mReadLength;
        u32 *mCheckAnswer;
    };
    /* 0x54 */ void (*_54)(NANDRequestInfo *);
    /* 0x58 */ u8 mPermission;
    /* 0x59 */ u8 mAttribute;
    /* 0x5C */ u32 mFsBlock;
    /* 0x60 */ u32 mInode;
};

class NANDManager {
public:
    NANDManager();
    bool addRequest(NANDRequestInfo *);
};

class NANDResultCode {
public:
    explicit NANDResultCode(s32 code)
        : mCode(code) {
    }

    s32 getCode() const;
    bool isSuccess() const;
    bool isSaveDataCorrupted() const;
    bool isNANDCorrupted() const;
    bool isMaxBlocks() const;
    bool isMaxFiles() const;
    bool isNoExistFile() const;
    bool isBusyOrAllocFailed() const;
    bool isUnknown() const;

private:
    /* 0x0 */ s32 mCode;
};

namespace MR {
void addRequestToNANDManager(NANDRequestInfo *);
}  // namespace MR
