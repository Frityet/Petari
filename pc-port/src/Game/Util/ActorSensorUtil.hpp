#pragma once

#include <revolution.h>

enum {
    ACTMES_RUSH_BEGIN = 0x91,
    ACTMES_AUTORUSH_BEGIN = 0x92,
    ACTMES_RUSH_CANCEL = 0x93,
    ACTMES_UPDATE_BASEMTX = 0xA1,
};

class HitSensor;

namespace MR {
    bool isMsgAutoRushBegin(u32 msg);
    bool isMsgUpdateBaseMtx(u32 msg);
}  // namespace MR
