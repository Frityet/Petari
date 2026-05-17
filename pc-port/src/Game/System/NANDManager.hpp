#pragma once

#include <revolution.h>

class NANDRequestInfo;

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
