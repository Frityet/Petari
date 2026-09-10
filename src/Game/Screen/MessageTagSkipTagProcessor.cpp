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
