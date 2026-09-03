#pragma once

#include "JSystem/J3DGraphAnimator/J3DAnimation.hpp"
#include "JSystem/JMath/JMath.hpp"

#include <aurora/ppc_math.hpp>
#include <cmath>

// Shared original interpolation and PPC conversion boundary used by the
// transform and material animation samplers.
namespace {
    // fctiwz truncates, saturates finite overflow, and returns INT_MIN for NaN.
    // Preserve its integer result without relying on native out-of-range casts.
    s32 truncatePpcInteger(f32 value) {
        return aurora::ppc::truncate_s32(value);
    }

    s16 narrowPpcRotation(u32 value) {
        return aurora::ppc::narrow_s16(value);
    }

    // slw tests bit 5 and uses bits 0..4 as the unsigned word shift count;
    // sth then stores only the low halfword, including for negative rotations.
    s16 shiftPpcRotation(s32 value, int shift) {
        return aurora::ppc::narrow_s16(aurora::ppc::shift_left_s32(value, static_cast<u32>(shift)));
    }
}  // namespace

inline f32 J3DHermiteInterpolation(f32 p1, f32 const *p2, f32 const *p3, f32 const *p4, f32 const *p5, f32 const *p6, f32 const *p7) {
    return JMAHermiteInterpolation(p1, *p2, *p3, *p4, *p5, *p6, *p7);
}

inline f32 J3DHermiteInterpolation(f32 pp1, s16 const *pp2, s16 const *pp3, s16 const *pp4,
                                   s16 const *pp5, s16 const *pp6, s16 const *pp7) {
    // Scalar form of the signed-16 PSQ path, preserving its single-precision
    // operations and explicit fused multiply/add order.
    f32 ff2 = *pp2;
    f32 ff0 = *pp5;
    f32 ff7 = *pp3;
    f32 ff5 = ff0 - ff2;
    f32 ff6 = *pp6;
    f32 ff3 = pp1 - ff2;
    ff0 = *pp7;
    f32 ff4 = ff6 - ff7;
    ff3 = ff3 / ff5;
    f32 fout = *pp4;
    ff0 = std::fma(ff0, ff5, ff7);
    ff2 = ff3 * ff3;
    ff4 = -std::fma(ff5, fout, -ff4);
    ff0 = ff0 - ff6;
    ff0 = ff0 - ff4;
    ff0 = ff2 * ff0;
    fout = std::fma(ff5, fout, ff0);
    fout = std::fma(fout, ff3, ff7);
    fout = std::fma(ff4, ff2, fout);
    return fout - ff0;
}

template <typename T>
f32 J3DGetKeyFrameInterpolation(f32 frame, J3DAnmKeyTableBase *pKeyTable, T *pData) {
    if (frame < pData[0]) {
        return pData[1];
    }

    if (pKeyTable->mType == 0) {
        u32 idx = pKeyTable->mMaxFrame - 1;
        if (pData[idx * 3] <= frame) {
            return pData[idx * 3 + 1];
        }

        u32 uVar7 = pKeyTable->mMaxFrame;
        while (uVar7 > 1) {
            u32 uVar2 = uVar7 >> 1;
            u32 tmp = uVar2 * 3;
            if (frame >= pData[tmp]) {
                pData += tmp;
                uVar7 = uVar7 - uVar2;
            } else {
                uVar7 = uVar2;
            }
        }

        f32 interpolated = J3DHermiteInterpolation(frame, &pData[0], &pData[1], &pData[2], &pData[3], &pData[4], &pData[5]);
        return interpolated;
    } else {
        u32 idx = pKeyTable->mMaxFrame - 1;
        if (pData[idx * 4] <= frame) {
            return pData[idx * 4 + 1];
        }

        u32 var_r27 = pKeyTable->mMaxFrame;
        while (var_r27 > 1) {
            u32 var_r25 = var_r27 >> 1;
            u32 var_r23 = var_r25 * 4;
            if (frame >= pData[var_r23]) {
                pData += var_r23;
                var_r27 = var_r27 - var_r25;
            } else {
                var_r27 = var_r25;
            }
        }

        f32 interpolated = J3DHermiteInterpolation(frame, &pData[0], &pData[1], &pData[3], &pData[4], &pData[5], &pData[6]);
        return interpolated;
    }
}

