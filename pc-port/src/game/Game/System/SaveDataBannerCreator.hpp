#pragma once

#include "Game/System/NANDManager.hpp"
#include "Game/System/NerveExecutor.hpp"

class SaveDataBannerCreator : public NerveExecutor {
public:
    SaveDataBannerCreator();

    void execute();
    bool isDone() const;
    NANDResultCode getResultCode() const;
    void exeNoOperation();

private:
    /* 0x08 */ NANDRequestInfo *mNANDRequestInfo;
};
