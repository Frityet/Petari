#include "Game/Screen/MessageTagSkipTagProcessor.hpp"

MessageTagSkipTagProcessor::MessageTagSkipTagProcessor() : nw4r::ut::TagProcessorBase< wchar_t >() {
}

nw4r::ut::TagProcessorBase< wchar_t >::Operation MessageTagSkipTagProcessor::CalcRect(nw4r::ut::Rect* pRect, u16 code, ContextType* pPrintContext) {
    if (code != 0x1A) {
        return nw4r::ut::TagProcessorBase< wchar_t >::CalcRect(pRect, code, pPrintContext);
    } else {
        return skipTag(pRect, pPrintContext, false);
    }
}

nw4r::ut::TagProcessorBase< wchar_t >::Operation MessageTagSkipTagProcessor::Process(u16 code, ContextType* pPrintContext) {
    if (code != 0x1A) {
        return nw4r::ut::TagProcessorBase< wchar_t >::Process(code, pPrintContext);
    } else {
        return skipTag(nullptr, pPrintContext, false);
    }
}

nw4r::ut::TagProcessorBase< wchar_t >::Operation MessageTagSkipTagProcessor::skipTag(nw4r::ut::Rect*, ContextType* pPrintContext, bool) {
    MessageEditorMessageTag tag(pPrintContext->str);
    pPrintContext->str += tag.getSkipLength();
    return OPERATION_DEFAULT;
}

MessageEditorMessageTag::MessageEditorMessageTag(const wchar_t* pMessage) : mMessage(pMessage) {
}

u32 MessageEditorMessageTag::getSkipLength() const {
    return (reinterpret_cast< const u8* >(mMessage)[0] - 2U) >> 1;
}

u32 MessageEditorMessageTag::getParam32(int index) const {
    return *reinterpret_cast< const u32* >(reinterpret_cast< const u8* >(mMessage) + index * 4 + 4);
}

MessageEditorMessageTag::MessageEditorMessageTag(const nw4r::ut::PrintContext< wchar_t >* context) : mMessage(context->str) {
}

u32 MessageEditorMessageTag::getTagLength() const {
    return reinterpret_cast< const u8* >(mMessage)[0] - 2U;
}

u32 MessageEditorMessageTag::getParamLength() const {
    return reinterpret_cast< const u8* >(mMessage)[0] - 6U;
}

u8 MessageEditorMessageTag::getParam8(int index) const {
    return reinterpret_cast< const u8* >(mMessage)[index + 4];
}

u16 MessageEditorMessageTag::getParam16(int index) const {
    return mMessage[index + 2];
}

wchar_t* MessageEditorMessageTag::getParamPtr(int offset) const {
    return reinterpret_cast< wchar_t* >(reinterpret_cast< u8* >(const_cast< wchar_t* >(mMessage)) + offset + 4);
}
