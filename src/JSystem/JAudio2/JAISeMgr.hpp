#pragma once

#include <revolution/types.h>

// Shared category data from JAISeMgr. Playback remains owned by Aurora's
// JAudio service; no retail sound-manager object is instantiated on the host.
struct JAISeCategoryArrangementItem {
    u8 mMaxActiveSe;
    u8 mMaxInactiveSe;
};

struct JAISeCategoryArrangement {
    JAISeCategoryArrangementItem mItems[16];
};

class JAISeMgr;
