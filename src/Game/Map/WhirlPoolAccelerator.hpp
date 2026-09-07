#pragma once

#include "Game/LiveActor/LiveActor.hpp"

class WhirlPoolAccelerator : public LiveActor {
public:
    WhirlPoolAccelerator(const char*);
    virtual ~WhirlPoolAccelerator();

    bool calcInfo(const TVec3f&, TVec3f*) const;

private:
    // Preserve the 0x38-byte retail derived tail independently of host pointer width.
    u8 mPad[0xC4 - 0x8C];
};
