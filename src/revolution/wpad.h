#pragma once

#include <revolution.h>

struct WPADInfo {
    BOOL dpd, speaker, attach, lowBat, nearempty;
    u8 battery, led, protocol, firmware;
};
