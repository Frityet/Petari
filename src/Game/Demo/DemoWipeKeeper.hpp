#include "compat/Cp932Literal.hpp"
#pragma once

#include <memory>
class JMapInfo;

#include "Game/Demo/DemoExecutor.hpp"

class DemoWipeInfo {
public:
    /// @brief Creates a new `DemoWipeInfo`.
    DemoWipeInfo();

    /* 0x00 */ const char* mPartName;
    /* 0x04 */ const char* mWipeName;
    /* 0x08 */ s32 mWipeType;
    /* 0x0C */ s32 mWipeFrame;
};

class DemoWipeKeeper : public DemoSheetKeeperBase, public DemoSheetKeeperInfoHolder< DemoWipeInfo > {
public:
    DemoWipeKeeper(DemoExecutor*);
    ~DemoWipeKeeper();

    virtual const char* getName() const {
        return CP932("ワイプ");
    }

    virtual const char* getTypeString() const {
        return "Wipe";
    }

    virtual void start();
    virtual void update();

    virtual void executeType(const DemoWipeInfo*);

private:
    // Original records borrow strings from this native parser until retirement.
    std::unique_ptr<JMapInfo> mNativeParser;
};
