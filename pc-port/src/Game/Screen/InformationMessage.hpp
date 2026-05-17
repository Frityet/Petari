#pragma once

#include "Game/Screen/LayoutActor.hpp"

class IconAButton;

class InformationMessage : public LayoutActor {
public:
    InformationMessage();
    ~InformationMessage() override;

    void init(const JMapInfoIter& rIter) override;
    void appear() override;

    void appearWithButtonLayout();
    void disappear();
    void setMessage(const char* pMessageId);
    void setMessage(const wchar_t* pMessage);
    void setReplaceString(const wchar_t* pString, s32 param2);
    void exeAppear();
    void exeWait();
    void exeDisappear();

    void setCenter(bool isCenter) { mIsCenter = isCenter; }

private:
    /* 0x20 */ IconAButton* mAButtonIcon;
    /* 0x24 */ bool mIsCenter;
};
