#include "JSystem/JMath/JMath.hpp"
#include "JSystem/JMath/JMATrigonometric.hpp"

#if !defined(__MWERKS__)
#include <cmath>
#endif

// Match the original JSystem compilation setting; explicit paired operations
// below still use std::fma where the console instruction fuses them.
#if defined(__clang__)
#pragma clang fp contract(off)
#elif defined(__GNUC__)
#pragma GCC optimize("fp-contract=off")
#elif defined(_MSC_VER)
#pragma fp_contract(off)
#endif

void JMAEulerToQuat(s16 x, s16 y, s16 z, Quaternion* quat) {
    f32 cosX = JMASCos(x / 2);
    f32 cosY = JMASCos(y / 2);
    f32 cosZ = JMASCos(z / 2);
    f32 sinX = JMASSin(x / 2);
    f32 sinY = JMASSin(y / 2);
    f32 sinZ = JMASSin(z / 2);

    f32 cyz = cosY * cosZ;
    f32 syz = sinY * sinZ;
    quat->w = cosX * (cyz) + sinX * (syz);
    quat->x = sinX * (cyz)-cosX * (syz);
    quat->y = cosZ * (cosX * sinY) + sinZ * (sinX * cosY);
    quat->z = sinZ * (cosX * cosY) - cosZ * (sinX * sinY);
}

void JMAQuatLerp(__REGISTER const Quaternion* p, __REGISTER const Quaternion* q, f32 t, Quaternion* dst) {
    __REGISTER f32 pxy, pzw, qxy, qzw;
    __REGISTER f32 dp;

#ifdef __MWERKS__  // clang-format off
    // compute dot product
    asm {
        psq_l       pxy, 0(p), 0, 0
        psq_l       qxy, 0(q), 0, 0
        ps_mul      dp, pxy, qxy

        psq_l       pzw, 8(p), 0, 0
        psq_l       qzw, 8(q), 0, 0
        ps_madd     dp, pzw, qzw, dp

        ps_sum0     dp, dp, dp, dp
    }
#else
    f32 xy0 = p->x * q->x;
    f32 xy1 = p->y * q->y;
    f32 zw0 = std::fma(p->z, q->z, xy0);
    f32 zw1 = std::fma(p->w, q->w, xy1);
    dp = zw0 + zw1;
#endif  // clang-format on
    f32 local_78 = dp;
    if (local_78 < 0.0) {
        int unused;
        dst->x = -t * (p->x + q->x) + p->x;
        dst->y = -t * (p->y + q->y) + p->y;
        dst->z = -t * (p->z + q->z) + p->z;
        dst->w = -t * (p->w + q->w) + p->w;
    } else {
        dst->x = -t * (p->x - q->x) + p->x;
        dst->y = -t * (p->y - q->y) + p->y;
        dst->z = -t * (p->z - q->z) + p->z;
        dst->w = -t * (p->w - q->w) + p->w;
    }
}
