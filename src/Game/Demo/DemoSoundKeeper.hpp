#include "resource/TextEncoding.hpp"
#pragma once

#include <memory>
class JMapInfo;

#include "Game/Demo/DemoExecutor.hpp"

class DemoSoundInfo {
public:
    /// @brief Creates a new `DemoSoundInfo`.
    DemoSoundInfo();

    /* 0x00 */ const char* mPartName;
    /* 0x04 */ const char* mBgm;
    /* 0x08 */ const char* mSystemSe;
    /* 0x0C */ u8 mReturnBgm;
    /* 0x10 */ s32 mBgmWipeoutFrame;
};

class DemoSoundKeeper : public DemoSheetKeeperBase, public DemoSheetKeeperInfoHolder< DemoSoundInfo > {
public:
    DemoSoundKeeper(DemoExecutor*);
    ~DemoSoundKeeper();

    virtual const char* getName() const {
        return CP932("サウンド");
    }

    virtual const char* getTypeString() const {
        return "Sound";
    }

    virtual void update();

    virtual void executeType(const DemoSoundInfo*);

    bool isPermitBgmChange();

private:
    // Original records borrow strings from this native parser until retirement.
    std::unique_ptr<JMapInfo> mNativeParser;
};
