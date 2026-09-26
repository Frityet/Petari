#include "Game/Screen/MessageTagSkipTagProcessor.hpp"
#include <aurora/exception.hpp>

MessageEditorMessageTag::MessageEditorMessageTag(const nw4r::ut::PrintContext< wchar_t >* pContext) : mMessage(pContext->str) {
}

MessageEditorMessageTag::MessageEditorMessageTag(const wchar_t* pMessage) : mMessage(pMessage) {
}

u32 MessageEditorMessageTag::getTagLength() const {
    return (static_cast<u32>(mMessage[0]) >> 8) - 2;
}

u32 MessageEditorMessageTag::getSkipLength() const {
    return getTagLength() / 2;
}

s32 MessageEditorMessageTag::getParamLength() const {
    return static_cast<s32>(static_cast<u32>(mMessage[0]) >> 8) - 6;
}

u8 MessageEditorMessageTag::getParam8(int index) const {
    return static_cast<u32>(mMessage[2 + index / 2]) >> ((index & 1) ? 0 : 8);
}

u16 MessageEditorMessageTag::getParam16(int index) const {
    return mMessage[2 + index];
}

u32 MessageEditorMessageTag::getParam32(int index) const {
    return (static_cast<u32>(mMessage[2 + index * 2]) << 16) | static_cast<u32>(mMessage[3 + index * 2]);
}

wchar_t* MessageEditorMessageTag::getParamPtr(int index) const {
    if (index & 1) {
        aurora::throw_host_exception<std::logic_error>("Odd byte tag payloads require indexed byte access on native wchar_t");
    }
    return const_cast<wchar_t*>(mMessage) + index / 2 + 2;
}

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

nw4r::ut::TagProcessorBase< wchar_t >::Operation MessageTagSkipTagProcessor::skipTag(nw4r::ut::Rect* pRect, ContextType* pPrintContext, bool param3) {
    MessageEditorMessageTag tag(pPrintContext);
    pPrintContext->str += tag.getSkipLength();
    return OPERATION_DEFAULT;
}
