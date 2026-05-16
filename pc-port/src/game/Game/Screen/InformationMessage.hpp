#pragma once

#include "Game/Screen/LayoutActor.hpp"

class IconAButton;
class JMapInfoIter;

class InformationMessage : public LayoutActor {
public:
    InformationMessage();
    ~InformationMessage() override;

    void init(const JMapInfoIter &);
    void initWithoutIter();
    void appear() override;

    void appearWithButtonLayout();
    void disappear();
    void setMessage(const char *pMessageId);
    void setMessage(const wchar_t *pMessage);
    void setReplaceString(const wchar_t *pString, s32 index);
    void exeAppear();
    void exeWait();
    void exeDisappear();

    void setCenter(bool isCenter) {
        mIsCenter = isCenter;
    }

    [[nodiscard]] const IconAButton *getAButtonIcon() const {
        return mAButtonIcon;
    }

private:
    /* 0x20 */ IconAButton *mAButtonIcon;
    /* 0x28 */ bool mIsCenter;
};
