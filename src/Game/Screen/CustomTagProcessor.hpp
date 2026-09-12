#pragma once

#include "Game/Screen/MessageTagSkipTagProcessor.hpp"
#include <nw4r/lyt/textBox.h>
#include <nw4r/ut/TagProcessorBase.h>
#include <revolution/gx/GXStruct.h>

namespace {
    u8 clampU8(s32 val) {
        if (val < 0) {
            return 0;
        }

        u8 ret = 0xFF;
        if (val <= 255) {
            ret = val;
        }

        return ret;
    }
};  // namespace

class CustomTagAlphaCtrl {
public:
    CustomTagAlphaCtrl();

    void init(u32, f32, f32, s32, s32);
    u8 alpha() const;
    void update();
    bool isEnd() const;

    /* 0x00 */ s32 mDelay;
    /* 0x04 */ s32 mEndDelay;
    /* 0x08 */ s32 mFrame;
    /* 0x0C */ s32 mCharIndex;
    /* 0x10 */ s32 mWaitFrames;
    /* 0x14 */ u32 mLength;
    /* 0x18 */ bool mIsActive;
    /* 0x1C */ f32 mCharAlphaStep;
    /* 0x20 */ f32 mFrameAlphaStep;
};

class CustomTagProcessor : public MessageTagSkipTagProcessor {
public:
    CustomTagProcessor(nw4r::lyt::TextBox*);

    virtual nw4r::ut::TagProcessorBase< wchar_t >::Operation Process(u16, nw4r::ut::PrintContext< wchar_t >*);
    virtual nw4r::ut::TagProcessorBase< wchar_t >::Operation CalcRect(nw4r::ut::Rect*, u16, nw4r::ut::PrintContext< wchar_t >*);

    typedef nw4r::ut::TagProcessorBase< wchar_t >::Operation Operation;
    typedef nw4r::ut::PrintContext< wchar_t > Context;
    typedef Operation (CustomTagProcessor::*GroupFunction)(nw4r::ut::Rect*, const MessageEditorMessageTag&, Context*);

    struct Impl {
        struct GroupFunctionInfo {
            u8 mGroup;
            GroupFunction mFunction;
        };
        static const GroupFunctionInfo sGroupFunctionTable[];
        static const GroupFunctionInfo* findGroupFunctionInfo(int);
    };

    void setArgNumber(s32, s32);
    void setArgString(const wchar_t*, s32);
    MessageEditorMessageTag getReplaceTag(const wchar_t*, s32, s32, s32) const;
    bool isIgnoreTag(u16, Context*) const;
    bool writeString(nw4r::ut::Rect*, const wchar_t*, Context*);

    Operation exeDisplayGroup(nw4r::ut::Rect*, const MessageEditorMessageTag&, Context*);
    Operation exeSoundGroup(nw4r::ut::Rect*, const MessageEditorMessageTag&, Context*);
    Operation exePictureGroup(nw4r::ut::Rect*, const MessageEditorMessageTag&, Context*);
    Operation exeFontSizeGroup(nw4r::ut::Rect*, const MessageEditorMessageTag&, Context*);
    Operation exeSystemGroup(nw4r::ut::Rect*, const MessageEditorMessageTag&, Context*);
    Operation exeLocalizeGroup(nw4r::ut::Rect*, const MessageEditorMessageTag&, Context*);
    Operation exeNumberGroup(nw4r::ut::Rect*, const MessageEditorMessageTag&, Context*);
    Operation exeStringGroup(nw4r::ut::Rect*, const MessageEditorMessageTag&, Context*);
    Operation exeSystemGroupRuby(nw4r::ut::Rect*, const MessageEditorMessageTag&, Context*);
    Operation exeDisplayGroupOffset(nw4r::ut::Rect*, const MessageEditorMessageTag&, Context*);
    Operation exeDisplayGroupCenter(nw4r::ut::Rect*, const MessageEditorMessageTag&, Context*);
    Operation exeFontGroup(nw4r::ut::Rect*, const MessageEditorMessageTag&, Context*);
    Operation exePatchimuGroup(nw4r::ut::Rect*, const MessageEditorMessageTag&, Context*);
    Operation exeSystemGroupColor(nw4r::ut::Rect*, int, Context*);
    Operation exeDisplayGroupWait(nw4r::ut::Rect*, u16, Context*);

    void initAlpha(f32, f32, s32, s32);
    void reset(const wchar_t*);

    /* 0x04 */ bool mIsShadow;
    /* 0x05 */ bool mIsText;
    /* 0x06 */ bool mIsInf;
    /* 0x07 */ bool _7;
    /* 0x08 */ CustomTagAlphaCtrl mAlphaCtrl;
    /* 0x2C */ nw4r::lyt::TextBox* mTextBox;
    /* 0x30 */ u8 _30;
    /* 0x31 */ u8 _31;
    /* 0x32 */ u8 _32;
    /* 0x34 */ u16 mLastChar;
    /* 0x36 */ GXColor mColorMin;
    /* 0x3A */ GXColor mColorMax;
    /* 0x40 */ f32 mRubyFontWidth;
    /* 0x44 */ f32 mRubyFontHeight;
    /* 0x48 */ f32 mFontWidth;
    /* 0x4C */ f32 mFontHeight;
};
