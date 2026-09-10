#include "Game/Screen/MessageEditorMessageTag.hpp"
#include "Game/Util/MessageUtil.hpp"

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
