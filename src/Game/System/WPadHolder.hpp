#pragma once

#include <revolution/hbm.h>
#include <revolution/types.h>
#include <aurora/wpad.hpp>
#include <optional>

class WPad;

class WPadReadDataInfo {
public:
    WPadReadDataInfo();
    ~WPadReadDataInfo();

    KPADStatus* getKPadStatus(u32) const;
    u32 getValidStatusCount() const;

    /* 0x00 */ KPADStatus* mStatusArray;
    /* 0x04 */ u32 mValidStatusCount;
};

class WPadHolder {
public:
    WPadHolder();
    ~WPadHolder();

    void updateReadDataOnly();
    void updateProjectPadData();
    void updateInGame();
    void update();
    void initSensorBarPosition();
    void resetPad();
    WPad* getWPad(s32);
    static void setConnectCallback();

    /* 0x00 */ WPad* mPad[2];
    /* 0x08 */ WPadReadDataInfo* mReadDataInfoArray;
    /* 0x0C */ u32 mMode;

private:
    // Retire asynchronous SDK writes before their original read/info buffers.
    std::optional< aurora::WpadClientScope > mNativeCallbacks;
};

namespace MR {
    WPad* getWPad(s32);
    void resetWPad();
    void setWPadHolderModeHomeButton();
    void setWPadHolderModeGame();
    void getHBMKPadData(HBMKPadData*, s32);
    void setAutoSleepTimeWiiRemote(bool);
};  // namespace MR
