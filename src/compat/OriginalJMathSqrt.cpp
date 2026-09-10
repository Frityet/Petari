#include "Game/Util/MathUtil.hpp"
#include <dolphin/ppc_math.h>

// JMASqrt's Gekko estimate and single-precision refinement instruction order.
f32 JMASqrt(f32 x) {
#if defined(__clang__)
#pragma clang fp contract(off)
#pragma clang fp reassociate(off)
#endif
    if (x > 0.0f) {
        const double estimate = ::frsqrte(static_cast<double>(x));
        const f32 scaled = static_cast<f32>(estimate * static_cast<double>(x));
        const f32 product = static_cast<f32>(static_cast<double>(scaled) * estimate);
        f32 correction = -(product - 3.0f);
        correction *= scaled;
        correction *= 0.5f;
        return correction;
    }

    return x;
}
