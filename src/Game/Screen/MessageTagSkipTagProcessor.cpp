#include "Game/Screen/MessageTagSkipTagProcessor.hpp"
#if defined(TARGET_PC)
#include <aurora/exception.hpp>
#include <stdexcept>
#endif

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
    #if defined(TARGET_PC)
    return ((static_cast<u32>(*mMessage) >> 8) - 2U) >> 1;
#else
    return (reinterpret_cast< const u8* >(mMessage)[0] - 2U) >> 1;
#endif
}

u32 MessageEditorMessageTag::getParam32(int index) const {
    #if defined(TARGET_PC)
    // Parameters are original big-endian pairs of retained UTF-16 code units.
    return (static_cast<u32>(mMessage[2 + index * 2]) << 16) | static_cast<u32>(mMessage[3 + index * 2]);
#else
    return *reinterpret_cast< const u32* >(reinterpret_cast< const u8* >(mMessage) + index * 4 + 4);
#endif
}

MessageEditorMessageTag::MessageEditorMessageTag(const nw4r::ut::PrintContext< wchar_t >* context) : mMessage(context->str) {
}

u32 MessageEditorMessageTag::getTagLength() const {
#if defined(TARGET_PC)
    return (static_cast<u32>(mMessage[0]) >> 8) - 2U;
#else
    return reinterpret_cast< const u8* >(mMessage)[0] - 2U;
#endif
}

u32 MessageEditorMessageTag::getParamLength() const {
#if defined(TARGET_PC)
    return (static_cast<u32>(mMessage[0]) >> 8) - 6U;
#else
    return reinterpret_cast< const u8* >(mMessage)[0] - 6U;
#endif
}

u8 MessageEditorMessageTag::getParam8(int index) const {
#if defined(TARGET_PC)
    return static_cast<u32>(mMessage[2 + index / 2]) >> ((index & 1) ? 0 : 8);
#else
    return reinterpret_cast< const u8* >(mMessage)[index + 4];
#endif
}

u16 MessageEditorMessageTag::getParam16(int index) const {
    return mMessage[index + 2];
}

wchar_t* MessageEditorMessageTag::getParamPtr(int offset) const {
#if defined(TARGET_PC)
    if (offset & 1) {
        aurora::throw_host_exception<std::logic_error>("Odd byte tag payloads require indexed byte access on native wchar_t");
    }
    return const_cast<wchar_t*>(mMessage) + offset / 2 + 2;
#else
    return reinterpret_cast< wchar_t* >(reinterpret_cast< u8* >(const_cast< wchar_t* >(mMessage)) + offset + 4);
#endif
}
