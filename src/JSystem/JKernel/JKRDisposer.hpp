#pragma once

#include "JSystem/JSupport/JSUList.hpp"

class JKRHeap;

class JKRDisposer {
public:
    JKRDisposer();
    virtual ~JKRDisposer();

    // A retired guest owner may release host resource leases before its arena
    // is destroyed. The original object and disposer identity stay alive.
    virtual void releaseNativeResourceReferences() noexcept {}
    void retireNativeResourceReferences() noexcept {
        if (!mNativeResourcesRetired) {
            mNativeResourcesRetired = true;
            releaseNativeResourceReferences();
        }
    }
    bool nativeResourcesRetired() const noexcept { return mNativeResourcesRetired; }

    // An intrusive disposer belongs to its own address and heap. Copying or
    // moving payloads must never copy the source object's registration link.
    JKRDisposer(const JKRDisposer&) : JKRDisposer() {}
    JKRDisposer(JKRDisposer&&) : JKRDisposer() {}
    JKRDisposer& operator=(const JKRDisposer&) noexcept { return *this; }
    JKRDisposer& operator=(JKRDisposer&&) noexcept { return *this; }

    JKRHeap* mHeap;                // 0x4
    JSULink< JKRDisposer > mLink;  // 0x8

private:
    bool mNativeResourcesRetired = false;
};
