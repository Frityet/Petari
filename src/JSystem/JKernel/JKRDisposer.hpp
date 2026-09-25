#pragma once

#include "JSystem/JSupport/JSUList.hpp"

class JKRHeap;

class JKRDisposer {
public:
    JKRDisposer();
    virtual ~JKRDisposer();

    // An intrusive disposer belongs to its own address and heap. Copying or
    // moving payloads must never copy the source object's registration link.
    JKRDisposer(const JKRDisposer&) : JKRDisposer() {}
    JKRDisposer(JKRDisposer&&) : JKRDisposer() {}
    JKRDisposer& operator=(const JKRDisposer&) noexcept { return *this; }
    JKRDisposer& operator=(JKRDisposer&&) noexcept { return *this; }

    JKRHeap* mHeap;                // 0x4
    JSULink< JKRDisposer > mLink;  // 0x8
};
