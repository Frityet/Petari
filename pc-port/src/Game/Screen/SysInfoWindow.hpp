#pragma once

#include "Game/Screen/LayoutActor.hpp"

typedef void(MessageChangeFunc)(LayoutActor*, const char*, const char*);

class IconAButton;
class YesNoController;

struct MessageChangeFuncTableEntry {
    MessageChangeFunc* mFuncPtr;
};

class SysInfoWindow : public LayoutActor {
public:
    enum SysInfoWindowType {
        WindowType_Normal,
        WindowType_Mini,
    };

    enum SysInfoExecuteType {
        ExecuteType_Normal,
        ExecuteType_Children,
    };

    enum SysInfoType {
        Type_Key,
        Type_Blocking,
        Type_YesNo,
    };

    enum SysInfoTextPos {
        TextPos_Center,
        TextPos_Bottom,
    };

    enum SysInfoMessageType {
        MessageType_Game,
        MessageType_System,
    };

    SysInfoWindow(SysInfoWindowType windowType, SysInfoExecuteType executeType);
    ~SysInfoWindow() override;

    void init(const JMapInfoIter& rIter) override;
    void movement() override;
    void draw() const override;
    void calcAnim() override;
    void appear() override;
    void kill() override;
    void control() override;

    void appear(const char* pMessageId, SysInfoType type, SysInfoTextPos textPos, SysInfoMessageType messageType);
    void disappear();
    void forceKill();
    [[nodiscard]] bool isWait() const;
    [[nodiscard]] bool isSelectedYes() const;
    [[nodiscard]] bool isDisappear() const;
    [[nodiscard]] const char* getLayoutName() const;
    void exeAppear();
    void exeWait();
    void exeDisappear();
    void setYesNoSelectorSE(const char* pCursorSE, const char* pYesSE, const char* pNoSE);
    void resetYesNoSelectorSE();
    void setTextBoxArgNumber(s32 arg, s32 argIndex);
    void setTextBoxArgString(const wchar_t* pArg, s32 argIndex);

    /* 0x20 */ SysInfoWindowType mWindowType;
    /* 0x24 */ SysInfoType mType;
    /* 0x28 */ YesNoController* mYesNoSelector;
    /* 0x2C */ IconAButton* mIconAButton;
    /* 0x30 */ const char* mTextParentPaneName;
    /* 0x34 */ const char* mWindowParentPaneName;
    /* 0x38 */ bool _38;
};

namespace MR {
    SysInfoWindow* createSysInfoWindow();
    SysInfoWindow* createSysInfoWindowExecuteWithChildren();
    SysInfoWindow* createSysInfoWindowMiniExecuteWithChildren();
}  // namespace MR
