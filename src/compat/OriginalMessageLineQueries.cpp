#include "Game/Screen/MessageEditorMessageTag.hpp"
#include "Game/Util/MessageUtil.hpp"

MessageEditorMessageTag::MessageEditorMessageTag(const wchar_t* pMessage) : mMessage(pMessage) {}

u32 MessageEditorMessageTag::getSkipLength() const {
    // The first retail byte is the upper byte of the retained UTF-16 code unit.
    return ((static_cast<u32>(*mMessage) >> 8) - 2U) >> 1;
}

bool MessageEditorMessageTag::isGroupTagId(int group, int tag) const {
    return (static_cast<u32>(*mMessage) & 0xFFU) == group && mMessage[1] == tag;
}

namespace MR {
    s32 countMessageLine(const wchar_t* pMessage) {
        s32 count = 1;
        while (*pMessage != 0) {
            if (*pMessage == 0x1A) {
                pMessage++;
                MessageEditorMessageTag tag(pMessage);
                pMessage += tag.getSkipLength();
                if (tag.isGroupTagId(1, 1)) {
                    break;
                }
            } else {
                if (*pMessage == L'\n') {
                    count++;
                }
                pMessage++;
            }
        }
        return count;
    }
}
