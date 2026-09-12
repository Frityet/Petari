#include "Game/Screen/CustomTagProcessor.hpp"
#include "Game/Util/MessageUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include "Game/System/Language.hpp"
#include <nw4r/ut/TextWriterBase.h>
#include <nw4r/lyt/material.h>
#include <algorithm>

CustomTagAlphaCtrl::CustomTagAlphaCtrl()
    : mDelay(0), mEndDelay(0), mFrame(0), mCharIndex(0), mWaitFrames(0), mLength(0), mIsActive(false), mCharAlphaStep(0.0f),
      mFrameAlphaStep(0.0f) {
}

void CustomTagAlphaCtrl::init(u32 length, f32 frameAlphaStep, f32 charAlphaStep, s32 delay, s32 endDelay) {
    if (frameAlphaStep == 0.0f) {
        mIsActive = false;
        return;
    }

    mFrame = -delay;
    mIsActive = true;
    mCharIndex = 0;
    mCharAlphaStep = charAlphaStep;
    mFrameAlphaStep = frameAlphaStep;
    mLength = length;
    mWaitFrames = 0;
    mDelay = delay;
    mEndDelay = endDelay;
}

u8 CustomTagAlphaCtrl::alpha() const {
    if (!mIsActive) {
        return 255;
    }

    f32 alpha = mFrameAlphaStep * (mFrame - mWaitFrames) - mCharIndex * mCharAlphaStep;
    return 255.0f * std::max(0.0f, std::min(1.0f, alpha));
}

void CustomTagAlphaCtrl::update() {
    s32 endFrame = mWaitFrames + mEndDelay + static_cast< s32 >((1.0f + mLength * mCharAlphaStep) / mFrameAlphaStep);
    mFrame = std::min(endFrame, mFrame + 1);
}

bool CustomTagAlphaCtrl::isEnd() const {
    if (!mIsActive) {
        return true;
    }

    s32 endFrame = mWaitFrames + mEndDelay + static_cast< s32 >((1.0f + mLength * mCharAlphaStep) / mFrameAlphaStep);
    return mFrame >= endFrame;
}

namespace {
    GXColor setGXColor(GXColorS10 color) {
        GXColor result;
        result.r = clampU8(color.r);
        result.g = clampU8(color.g);
        result.b = clampU8(color.b);
        result.a = clampU8(color.a);
        return result;
    }
};  // namespace

CustomTagProcessor::CustomTagProcessor(nw4r::lyt::TextBox* textBox) : _30(false) {
    mColorMin = setGXColor(textBox->GetMaterial()->GetTevColor(0));
    mColorMax = setGXColor(textBox->GetMaterial()->GetTevColor(1));
    mRubyFontWidth = 0.5f * textBox->mFontSize.width;
    mRubyFontHeight = 0.5f * textBox->mFontSize.height;
    mFontWidth = textBox->mFontSize.width;
    mFontHeight = textBox->mFontSize.height;
    mLastChar = 0;
    mIsShadow = false;
    mIsText = false;
    mIsInf = false;
    _7 = false;
    mTextBox = textBox;
}

void CustomTagProcessor::initAlpha(f32 frameAlphaStep, f32 charAlphaStep, s32 delay, s32 endDelay) {
    mAlphaCtrl.init(MR::countMessageChar(mTextBox->mTextBuf), frameAlphaStep, charAlphaStep, delay, endDelay);
    _30 = false;
    _31 = false;
    _32 = false;
}

void CustomTagProcessor::reset(const wchar_t* message) {
    if (message == mTextBox->mTextBuf) {
        _31 = false;
        _32 = false;
        mAlphaCtrl.mWaitFrames = 0;
        mAlphaCtrl.mCharIndex = 0;
        mLastChar = 0;
    }
}

const CustomTagProcessor::Impl::GroupFunctionInfo CustomTagProcessor::Impl::sGroupFunctionTable[] = {
    {1, &CustomTagProcessor::exeDisplayGroup},
    {2, &CustomTagProcessor::exeSoundGroup},
    {3, &CustomTagProcessor::exePictureGroup},
    {4, &CustomTagProcessor::exeFontSizeGroup},
    {5, &CustomTagProcessor::exeLocalizeGroup},
    {6, &CustomTagProcessor::exeNumberGroup},
    {7, &CustomTagProcessor::exeStringGroup},
    {255, &CustomTagProcessor::exeSystemGroup},
    {10, &CustomTagProcessor::exeFontGroup},
    {11, &CustomTagProcessor::exePatchimuGroup},
    {0, nullptr},
};

const CustomTagProcessor::Impl::GroupFunctionInfo* CustomTagProcessor::Impl::findGroupFunctionInfo(int group) {
    const GroupFunctionInfo* info = sGroupFunctionTable;
    while (info->mFunction != nullptr) {
        if (info->mGroup == group) {
            return info;
        }
        ++info;
    }
    return nullptr;
}

CustomTagProcessor::Operation CustomTagProcessor::CalcRect(nw4r::ut::Rect* rect, u16 code, Context* context) {
    if (isIgnoreTag(code, context)) {
        return MessageTagSkipTagProcessor::CalcRect(rect, code, context);
    }
    if (!_7 && !(context->flags & 1)) {
        context->writer->MoveCursorX(context->writer->GetCharSpace());
    }
    MessageEditorMessageTag tag(context);
    Operation result = OPERATION_DEFAULT;
    const Impl::GroupFunctionInfo* info = Impl::findGroupFunctionInfo(tag.mMessage[0] & 0xFF);
    if (info != nullptr) {
        result = (this->*info->mFunction)(rect, tag, context);
    }
    context->str += tag.getSkipLength();
    return result;
}

CustomTagProcessor::Operation CustomTagProcessor::Process(u16 code, Context* context) {
    if (isIgnoreTag(code, context)) {
        return MessageTagSkipTagProcessor::Process(code, context);
    }
    if (!_7 && !(context->flags & 1)) {
        context->writer->MoveCursorX(context->writer->GetCharSpace());
    }
    MessageEditorMessageTag tag(context);
    Operation result = OPERATION_DEFAULT;
    const Impl::GroupFunctionInfo* info = Impl::findGroupFunctionInfo(tag.mMessage[0] & 0xFF);
    if (info != nullptr) {
        result = (this->*info->mFunction)(nullptr, tag, context);
    }
    context->str += tag.getSkipLength();
    return result;
}

bool CustomTagProcessor::isIgnoreTag(u16 code, Context* context) const {
    if (code != 0x1A) {
        return true;
    }
    MessageEditorMessageTag tag(context);
    return Impl::findGroupFunctionInfo(tag.mMessage[0] & 0xFF) == nullptr;
}

void CustomTagProcessor::setArgNumber(s32 number, s32 index) {
    for (s32 i = 0; i < 4; ++i) {
        MessageEditorMessageTag tag = getReplaceTag(mTextBox->mTextBuf, 6, index, i);
        if (tag.mMessage == nullptr) {
            break;
        }
        *reinterpret_cast< s32* >(tag.getParamPtr(0)) = number;
    }
}

void CustomTagProcessor::setArgString(const wchar_t* message, s32 index) {
    for (s32 i = 0; i < 4; ++i) {
        MessageEditorMessageTag tag = getReplaceTag(mTextBox->mTextBuf, 7, index, i);
        if (tag.mMessage == nullptr) {
            break;
        }
        *reinterpret_cast< const wchar_t** >(tag.getParamPtr(0)) = message;
    }
}

MessageEditorMessageTag CustomTagProcessor::getReplaceTag(const wchar_t* message, s32 group, s32 index, s32 occurrence) const {
    while (*message != L'\0') {
        if (*message == 0x1A) {
            ++message;
            MessageEditorMessageTag tag(message);
            message += tag.getSkipLength();
            if ((tag.mMessage[0] & 0xFF) == group && tag.getParam32(1) == index) {
                if (occurrence == 0) {
                    return tag;
                }
                --occurrence;
            }
        } else {
            ++message;
        }
    }
    return MessageEditorMessageTag(static_cast< const wchar_t* >(nullptr));
}

bool CustomTagProcessor::writeString(nw4r::ut::Rect* rect, const wchar_t* message, Context* context) {
    nw4r::ut::TextWriterBase< wchar_t >* writer = context->writer;
    if (message == nullptr) {
        return false;
    }
    if (rect == nullptr) {
        writer->MoveCursorY(-writer->GetFontAscent());
        writer->Print(message, MR::getStringLengthWithMessageTag(message));
        writer->MoveCursorY(writer->GetFontAscent());
    } else {
        nw4r::ut::TextWriterBase< wchar_t > copy(*writer);
        f32 x = copy.GetCursorX();
        f32 y = copy.GetCursorY();
        copy.CalcStringRect(rect, message, MR::getStringLengthWithMessageTag(message));
        context->writer->MoveCursorX(rect->GetWidth());
        context->writer->MoveCursorY(rect->GetHeight() - copy.GetFontHeight());
        rect->left += x;
        rect->right += x;
        rect->top += y;
        rect->bottom += y;
    }
    return true;
}

#include "Game/Screen/CustomTagProcessor.hpp"
#include "nw4r/math/constant.h"
#include "nw4r/ut/TextWriterBase.h"
#include "nw4r/ut/inlines.h"

namespace nw4r {
    namespace ut {
        template <>
        f32 TextWriterBase< wchar_t >::PrintImpl(StreamType str, int length) {
            f32 xOrigin = GetCursorX();
            f32 yOrigin = GetCursorY();
            const bool bUseLimit = mWidthLimit < math::F_MAX;
            const f32 orgY = yOrigin;
            StreamType prevStreamPos = str;
            StreamType lineStart = str;
            bool bCharSpace = false;
            f32 textWidth = AdjustCursor(&xOrigin, &yOrigin, str, length);
            const f32 yOffset = orgY - GetCursorY();
            PrintContext< wchar_t > context = {this, str, xOrigin, yOrigin, 0};
            CharStrmReader reader = GetFont()->GetCharStrmReader();

            reader.Set(str);
            CustomTagProcessor* tagProcessor = nullptr;
            const u8 orgAlpha = GetAlpha();

            if (GetTagProcessor() != &mDefaultTagProcessor) {
                tagProcessor = static_cast< CustomTagProcessor* >(GetTagProcessor());
                tagProcessor->reset(str);
            }

            for (CharCode code = reader.Next(); reinterpret_cast< StreamType >(reader.GetCurrentPos()) - str <= length;) {
                if (code < ' ') {
                    context.str = reinterpret_cast< StreamType >(reader.GetCurrentPos());
                    context.flags = bCharSpace ? 0 : CONTEXT_NO_CHAR_SPACE;

                    if (bUseLimit && code != '\n' && prevStreamPos != lineStart) {
                        PrintContext< wchar_t > context2 = context;
                        TextWriterBase< wchar_t > myCopy = *this;
                        Rect rect;

                        context2.writer = &myCopy;
                        mTagProcessor->CalcRect(&rect, code, &context2);

                        if (rect.GetWidth() > 0.0f && myCopy.GetCursorX() - context.xOrigin > mWidthLimit) {
                            reader.Set(prevStreamPos);
                            code = '\n';
                            continue;
                        }
                    }

                    const TagProcessor::Operation operation = mTagProcessor->Process(code, &context);

                    if (operation == TagProcessor::OPERATION_NEXT_LINE) {
                        if (IsDrawFlagSet(HORIZONTAL_ALIGN_MASK, HORIZONTAL_ALIGN_CENTER)) {
                            const f32 width = CalcLineWidth(context.str, length - (context.str - str));
                            SetCursorX(context.xOrigin + (textWidth - width) * 0.5f);
                        } else if (IsDrawFlagSet(HORIZONTAL_ALIGN_MASK, HORIZONTAL_ALIGN_RIGHT)) {
                            const f32 width = CalcLineWidth(context.str, length - (context.str - str));
                            SetCursorX(context.xOrigin + (textWidth - width));
                        } else {
                            textWidth = Max(textWidth, GetCursorX() - context.xOrigin);
                            SetCursorX(context.xOrigin);
                        }

                        if (bUseLimit) {
                            lineStart = reinterpret_cast< StreamType >(reader.GetCurrentPos());
                        }
                        bCharSpace = false;
                    } else if (operation == TagProcessor::OPERATION_NO_CHAR_SPACE) {
                        bCharSpace = false;
                    } else if (operation == TagProcessor::OPERATION_CHAR_SPACE) {
                        bCharSpace = true;
                    } else if (operation == TagProcessor::OPERATION_END_DRAW) {
                        break;
                    }

                    reader.Set(context.str);
                } else {
                    if (tagProcessor != nullptr) {
                        SetAlpha(tagProcessor->mAlphaCtrl.alpha());
                        ++tagProcessor->mAlphaCtrl.mCharIndex;
                    }

                    const f32 y = GetCursorY();

                    if (bUseLimit && prevStreamPos != lineStart) {
                        const f32 x = GetCursorX();
                        const f32 charSpace = bCharSpace ? GetCharSpace() : 0.0f;
                        const f32 charWidth = IsWidthFixed() ? GetFixedWidth() : GetFont()->GetCharWidth(code) * GetScaleH();

                        if (charWidth + (charSpace + (x - xOrigin)) > mWidthLimit) {
                            reader.Set(prevStreamPos);
                            code = '\n';
                            continue;
                        }
                    }

                    if (bCharSpace) {
                        MoveCursorX(GetCharSpace());
                    }

                    bCharSpace = true;
                    MoveCursorY(-GetFont()->GetBaselinePos() * GetScaleV());
                    CharWriter::Print(code);

                    if (tagProcessor != nullptr) {
                        tagProcessor->mLastChar = code;
                    }

                    SetCursorY(y);
                }

                if (bUseLimit) {
                    prevStreamPos = reinterpret_cast< StreamType >(reader.GetCurrentPos());
                }

                code = reader.Next();
            }

            textWidth = Max(textWidth, GetCursorX() - context.xOrigin);

            if (IsDrawFlagSet(VERTICAL_ORIGIN_MASK, VERTICAL_ORIGIN_MIDDLE) ||
                IsDrawFlagSet(VERTICAL_ORIGIN_MASK, VERTICAL_ORIGIN_BOTTOM)) {
                SetCursorY(orgY);
            } else {
                MoveCursorY(yOffset);
            }

            if (tagProcessor != nullptr) {
                SetAlpha(orgAlpha);
            }

            return textWidth;
        }
    };  // namespace ut
};  // namespace nw4r

namespace {
    const GXColor sTextColors[] = {
        {0xFF, 0xFF, 0xFF, 0xFF},
        {0xDC, 0x82, 0x82, 0xFF},
        {0x50, 0xAA, 0x50, 0xFF},
        {0x50, 0x8C, 0xD2, 0xFF},
        {0xEB, 0xC8, 0x00, 0xFF},
        {0xB4, 0x6E, 0xC8, 0xFF},
        {0xFF, 0xBE, 0xBE, 0xFF},
        {0x6E, 0xF3, 0x46, 0xFF},
        {0x78, 0xFF, 0xFF, 0xFF},
        {0xFF, 0xFF, 0x50, 0xFF},
        {0xFB, 0xBC, 0xFA, 0xFF},
        {0xBE, 0xBE, 0xC8, 0xFF},
    };
    const GXColor sAlternateTextColors[] = {
        {0xFF, 0xFF, 0xFF, 0xFF},
        {0xE6, 0xA0, 0x00, 0xFF},
        {0x50, 0xAA, 0x50, 0xFF},
        {0x50, 0x8C, 0xD2, 0xFF},
        {0xEB, 0xC8, 0x00, 0xFF},
        {0xB4, 0x6E, 0xC8, 0xFF},
        {0xFF, 0xD2, 0x50, 0xFF},
        {0x6E, 0xF3, 0x46, 0xFF},
        {0x78, 0xFF, 0xFF, 0xFF},
        {0xFF, 0xFF, 0x50, 0xFF},
        {0xFB, 0xBC, 0xFA, 0xFF},
        {0xBE, 0xBE, 0xC8, 0xFF},
    };

    const GXColor* getTextColor(s32 index) {
        if (MR::getLanguage() == 0x49) {
            return &sAlternateTextColors[index];
        }
        return &sTextColors[index];
    }

    void setTextColor(nw4r::ut::TextWriterBase< wchar_t >* writer, s32 index) {
        GXColor color = *getTextColor(index);
        color.a = writer->GetTextColor().a;
        writer->SetTextColor(color);
    }
};  // namespace

CustomTagProcessor::Operation CustomTagProcessor::exePictureGroup(nw4r::ut::Rect* rect, const MessageEditorMessageTag& tag, Context* context) {
    nw4r::ut::TextWriterBase< wchar_t > writer(*context->writer);
    writer.SetFont(*MR::getPictureFontNW4R());
    wchar_t picture = static_cast< u16 >(tag.mMessage[1] + L'0');
    f32 width = writer.GetCharSpace() + writer.CalcStringWidth(&picture, 1);
    context->writer->MoveCursorX(width);
    if (rect != nullptr) {
        rect->right = rect->left + width;
        rect->SetHeight(writer.GetFontHeight() + writer.GetLineSpace());
    } else {
        if (!mIsShadow) {
            writer.ResetColorMapping();
            writer.SetupGX();
            if (_32) {
                setTextColor(&writer, 0);
            }
        }
        writer.MoveCursorY(-2.0f - writer.GetFontAscent());
        writer.Print(&picture, 1);
        if (!mIsShadow) {
            context->writer->SetupGX();
        }
    }
    return OPERATION_DEFAULT;
}

CustomTagProcessor::Operation CustomTagProcessor::exeFontGroup(nw4r::ut::Rect* rect, const MessageEditorMessageTag& tag, Context* context) {
    nw4r::ut::TextWriterBase< wchar_t > writer(*context->writer);
    writer.SetFont(*MR::getNumberFontNW4R());
    const wchar_t* message = tag.getParamPtr(0);
    s32 length = static_cast< s32 >(tag.getParamLength()) / 2;
    f32 width = writer.GetCharSpace() + writer.CalcStringWidth(message, length);
    context->writer->MoveCursorX(width);
    if (rect != nullptr) {
        rect->right = rect->left + width;
        rect->SetHeight(writer.GetFontHeight() + writer.GetLineSpace());
    } else {
        f32 ascent = -writer.GetFontAscent();
        if (!mIsShadow) {
            writer.ResetColorMapping();
            writer.SetupGX();
            setTextColor(&writer, 0);
        }
        writer.MoveCursorY(ascent);
        writer.Print(message, length);
        if (!mIsShadow) {
            context->writer->SetupGX();
        }
    }
    return OPERATION_NO_CHAR_SPACE;
}

CustomTagProcessor::Operation CustomTagProcessor::exeSystemGroupColor(nw4r::ut::Rect* rect, int index, Context* context) {
    if (mIsShadow) {
        return OPERATION_NO_CHAR_SPACE;
    }
    if (mIsInf && index >= 1 && index < 6) {
        index += 5;
    }
    if (rect == nullptr) {
        _32 = index;
        if (index == 0) {
            context->writer->SetColorMapping(mColorMin, mColorMax);
        } else {
            GXColor color = *getTextColor(index);
            color.a = mColorMin.a;
            context->writer->SetColorMapping(color, *getTextColor(index));
        }
        context->writer->SetupGX();
        setTextColor(context->writer, index);
    }
    return OPERATION_NO_CHAR_SPACE;
}

CustomTagProcessor::Operation CustomTagProcessor::exePatchimuGroup(nw4r::ut::Rect* rect, const MessageEditorMessageTag& tag, Context* context) {
    wchar_t message[16] = {};
    wchar_t* output = message;
    bool hasFinal = true;
    if (mLastChar >= L'0' && mLastChar < L'9') {
        hasFinal = mLastChar == L'3' || mLastChar == L'6';
    } else if (mLastChar >= 0xAC00) {
        s32 final = (mLastChar - 0xAC00) % 28;
        hasFinal = final != 0;
        if (tag.mMessage[1] == 4 && final == 8) {
            hasFinal = false;
        }
    }
    switch (tag.mMessage[1]) {
    case 0: *output++ = hasFinal ? 0xC740 : 0xB294; break;
    case 1: *output++ = hasFinal ? 0xC744 : 0xB97C; break;
    case 2: *output++ = hasFinal ? 0xC774 : 0xAC00; break;
    case 3: *output++ = hasFinal ? 0xACFC : 0xC640; break;
    case 4:
        if (!hasFinal) { return OPERATION_NO_CHAR_SPACE; }
        *output++ = 0xC73C;
        break;
    case 5: *output++ = hasFinal ? 0xBEC6 : 0xBEDF; break;
    case 6:
        if (!hasFinal) { return OPERATION_NO_CHAR_SPACE; }
        *output++ = 0xC774;
        break;
    }
    *output = L'\0';
    writeString(rect, message, context);
    return OPERATION_DEFAULT;
}

CustomTagProcessor::Operation CustomTagProcessor::exeSystemGroupRuby(nw4r::ut::Rect* rect, const MessageEditorMessageTag& tag, Context* context) {
    if (MR::getLanguage() != 0x10) {
        return OPERATION_NO_CHAR_SPACE;
    }
    s32 baseLength = tag.getParam8(0);
    const wchar_t* rubySource = tag.getParamPtr(1);
    s32 rubyLength = (tag.getParamLength() - 2) / 2;
    wchar_t base[32];
    wchar_t ruby[32];
    MR::copyMemory(ruby, rubySource, rubyLength * sizeof(wchar_t));
    ruby[rubyLength] = L'\0';
    MR::copyMemory(base, context->str + tag.getTagLength() / 2, baseLength * sizeof(wchar_t));
    base[baseLength] = L'\0';
    nw4r::ut::TextWriterBase< wchar_t >* original = context->writer;
    original->GetCharSpace();
    nw4r::ut::TextWriterBase< wchar_t > writer(*original);
    writer.SetDrawFlag(0x300);
    writer.SetLineSpace(0.0f);
    writer.SetCharSpace(2.0f);
    writer.SetFontSize(mRubyFontWidth, mRubyFontHeight);
    writer.SetTagProcessor(this);
    f32 baseWidth = original->CalcStringWidth(base, baseLength);
    f32 rubyWidth = writer.CalcStringWidth(ruby, rubyLength);
    if (rect == nullptr) {
        f32 difference = baseWidth - rubyWidth;
        if (difference > 0.0f) {
            f32 space = difference / (rubyLength + 1);
            writer.MoveCursorX(space);
            writer.SetCharSpace(2.0f + space);
        } else {
            writer.MoveCursorX(difference * 0.5f);
        }
        writer.MoveCursorY(3.0f - original->GetFontAscent());
        CustomTagAlphaCtrl alpha = mAlphaCtrl;
        mAlphaCtrl.mCharAlphaStep = baseLength * mAlphaCtrl.mCharAlphaStep / rubyLength;
        mAlphaCtrl.mCharIndex = rubyLength * mAlphaCtrl.mCharIndex / baseLength;
        writer.Print(ruby, rubyLength);
        mAlphaCtrl = alpha;
    }
    return OPERATION_NO_CHAR_SPACE;
}

#include "Game/Screen/CustomTagProcessor.hpp"
#include "Game/Screen/MessageEditorMessageTag.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include "nw4r/ut/TextWriterBase.h"
#include <cstddef>

extern "C" int swprintf(wchar_t*, size_t, const wchar_t*, ...);

namespace ReplaceTagProcessor {
    u32 exeLocalizeGroup(wchar_t*, const MessageEditorMessageTag&);
};  // namespace ReplaceTagProcessor

CustomTagProcessor::Operation CustomTagProcessor::exeDisplayGroup(nw4r::ut::Rect* rect, const MessageEditorMessageTag& tag, Context* context) {
    switch (tag.mMessage[1]) {
    case 0:
        return exeDisplayGroupWait(rect, tag.getParam16(0), context);
    case 2:
        return exeDisplayGroupOffset(rect, tag, context);
    case 3:
        return exeDisplayGroupCenter(rect, tag, context);
    case 1:
        return OPERATION_END_DRAW;
    default:
        return OPERATION_NO_CHAR_SPACE;
    }
}

CustomTagProcessor::Operation CustomTagProcessor::exeSoundGroup(nw4r::ut::Rect* rect, const MessageEditorMessageTag& tag, Context*) {
    if (rect != nullptr || mIsText || mAlphaCtrl.alpha() == 0) {
        return OPERATION_NO_CHAR_SPACE;
    }

    u8 mask = 1 << _31;
    if ((_30 & mask) != mask) {
        char soundName[256];
        s32 length = static_cast< s32 >(tag.getParamLength()) / 2;
        MR::convertUTF16ToASCII(soundName, tag.getParamPtr(0), length + 1);
        MR::startSystemSE(soundName, -1, -1);
        _30 |= static_cast< u8 >(1 << _31);
    }

    ++_31;
    return OPERATION_NO_CHAR_SPACE;
}

CustomTagProcessor::Operation CustomTagProcessor::exeFontSizeGroup(nw4r::ut::Rect*, const MessageEditorMessageTag& tag, Context* context) {
    nw4r::ut::TextWriterBase< wchar_t >* writer = context->writer;

    switch (tag.mMessage[1]) {
    case 0:
        writer->SetFontSize(0.75f * mFontWidth, 0.75f * mFontHeight);
        break;
    case 1:
        writer->SetFontSize(mFontWidth, mFontHeight);
        break;
    case 2:
        writer->SetFontSize(1.5f * mFontWidth, 1.5f * mFontHeight);
        break;
    }

    if (tag.mMessage[1] == 2 && mTextBox->GetTextPositionV() != 0) {
        writer->MoveCursorY(0.175f * mFontHeight);
    }

    return OPERATION_NO_CHAR_SPACE;
}

CustomTagProcessor::Operation CustomTagProcessor::exeSystemGroup(nw4r::ut::Rect* rect, const MessageEditorMessageTag& tag, Context* context) {
    switch (tag.mMessage[1]) {
    case 0:
        return exeSystemGroupColor(rect, tag.getParam8(0), context);
    case 2:
        return exeSystemGroupRuby(rect, tag, context);
    default:
        return OPERATION_NO_CHAR_SPACE;
    }
}

CustomTagProcessor::Operation CustomTagProcessor::exeLocalizeGroup(nw4r::ut::Rect* rect, const MessageEditorMessageTag& tag, Context* context) {
    wchar_t buffer[32];
    ReplaceTagProcessor::exeLocalizeGroup(buffer, tag);
    writeString(rect, buffer, context);
    return OPERATION_DEFAULT;
}

CustomTagProcessor::Operation CustomTagProcessor::exeNumberGroup(nw4r::ut::Rect* rect, const MessageEditorMessageTag& tag, Context* context) {
    wchar_t buffer[16];
    s32 number = *reinterpret_cast< const s32* >(tag.getParamPtr(0));

    switch (tag.mMessage[1]) {
    case 5:
        swprintf(buffer, 256, L"%02d", number);
    case 6:
        swprintf(buffer, 256, L"%03d", number);
    case 7:
        swprintf(buffer, 256, L"%04d", number);
    case 8:
        swprintf(buffer, 256, L"%05d", number);
    case 9:
        swprintf(buffer, 256, L"%06d", number);
    default:
        swprintf(buffer, 256, L"%d", number);
    }

    writeString(rect, buffer, context);
    return OPERATION_DEFAULT;
}

CustomTagProcessor::Operation CustomTagProcessor::exeStringGroup(nw4r::ut::Rect* rect, const MessageEditorMessageTag& tag, Context* context) {
    if (*reinterpret_cast< const u8* >(tag.getParamPtr(0)) == 0) {
        return OPERATION_DEFAULT;
    }

    writeString(rect, *reinterpret_cast< const wchar_t* const* >(tag.getParamPtr(0)), context);
    return OPERATION_DEFAULT;
}

#include "Game/Screen/CustomTagProcessor.hpp"
#include "Game/Screen/MessageEditorMessageTag.hpp"
#include "Game/Util/StringUtil.hpp"
#include "nw4r/ut/TextWriterBase.h"

CustomTagProcessor::Operation CustomTagProcessor::exeDisplayGroupWait(nw4r::ut::Rect* rect, u16 waitFrames, Context*) {
    if (rect == nullptr) {
        mAlphaCtrl.mWaitFrames += waitFrames;
    }

    return OPERATION_NO_CHAR_SPACE;
}

CustomTagProcessor::Operation CustomTagProcessor::exeDisplayGroupOffset(nw4r::ut::Rect*, const MessageEditorMessageTag& tag, Context* context) {
    nw4r::ut::TextWriterBase< wchar_t >* writer = context->writer;
    const wchar_t* string = context->str + tag.getSkipLength();
    nw4r::ut::Rect rect;

    if (mTextBox->GetTextPositionV() == 0) {
        writer->CalcStringRect(&rect, string, MR::getStringLengthWithMessageTag(string));
        writer->MoveCursorY((mTextBox->mSize.height - rect.GetHeight()) * 0.5f);
    }

    return OPERATION_NO_CHAR_SPACE;
}

CustomTagProcessor::Operation CustomTagProcessor::exeDisplayGroupCenter(nw4r::ut::Rect*, const MessageEditorMessageTag& tag, Context* context) {
    nw4r::ut::TextWriterBase< wchar_t > writer = *context->writer;
    const wchar_t* string = context->str + tag.getSkipLength();
    nw4r::ut::Rect rect;
    writer.CalcStringRect(&rect, string, MR::getStringLengthWithMessageTag(string));
    f32 offset = (mTextBox->mSize.width - rect.GetWidth()) * 0.5f;
    context->writer->MoveCursorX(offset);
    context->xOrigin += offset;
    return OPERATION_NO_CHAR_SPACE;
}
