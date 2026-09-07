#pragma once

#include <revolution/types.h>

class MessageEditorMessageTag {
public:
    MessageEditorMessageTag(const wchar_t*);

    u32 getTagLength() const;
    u32 getSkipLength() const;
    u32 getParamLength() const;
    u8 getParam8(int) const;
    u16 getParam16(int) const;
    u32 getParam32(int) const;
    wchar_t* getParamPtr(int) const;
    bool isGroupTagId(int, int) const;

    /* 0x00 */ const wchar_t* mMessage;
};
