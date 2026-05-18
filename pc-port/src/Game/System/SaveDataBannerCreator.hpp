#pragma once

#include <revolution.h>

#include "Game/System/NerveExecutor.hpp"

class NANDRequestInfo;
class NANDResultCode;
struct NANDBanner;

class SaveDataBannerCreator : public NerveExecutor {
public:
    SaveDataBannerCreator();

    void execute();
    [[nodiscard]] bool isDone() const;
    [[nodiscard]] NANDResultCode getResultCode() const;
    void exeNoOperation();
    void exeCreateOnTemporary();
    void exeMoveToHomeDir();
    void setupBannerInfo();

private:
    /* 0x08 */ NANDRequestInfo* mNANDRequestInfo;
    /* 0x0C */ NANDBanner* mBanner;
    /* 0x10 */ char mHomeDir[NAND_MAX_PATH];
};
