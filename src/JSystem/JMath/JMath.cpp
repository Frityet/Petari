#include "JSystem/JMath/JMath.hpp"
#include "JSystem/JMath/JMATrigonometric.hpp"

// Preserve the original JSystem contraction setting; paired-single translations
// use explicit std::fma only where the console instruction fuses arithmetic.
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

void JMAVECScaleAdd(__REGISTER const Vec* vec1, __REGISTER const Vec* vec2, __REGISTER Vec* dst, __REGISTER f32 scale) {
    __REGISTER f32 v1xy;
    __REGISTER f32 v2xy;
    __REGISTER f32 rxy, v1z, v2z, rz;
#ifdef __MWERKS__  // clang-format off
	asm {
        psq_l v1xy, 0(vec1), 0, 0
        psq_l v2xy, 0(vec2), 0, 0
        ps_madds0 rxy, v1xy, scale, v2xy
        psq_st rxy, 0(dst), 0, 0

        psq_l v1z, 8(vec1), 1, 0
        psq_l v2z, 8(vec2), 1, 0
        ps_madds0 rz, v1z,  scale, v2z
        psq_st rz, 8(dst), 1, 0
	}
#else
    const f32 x = std::fma(vec1->x, scale, vec2->x);
    const f32 y = std::fma(vec1->y, scale, vec2->y);
    const f32 z = std::fma(vec1->z, scale, vec2->z);
    dst->x = ppc_psq_store_f32(x);
    dst->y = ppc_psq_store_f32(y);
    dst->z = ppc_psq_store_f32(z);
#endif  // clang-format on
}

void JMAVECLerp(__REGISTER const Vec* a, __REGISTER const Vec* b, __REGISTER Vec* dst, __REGISTER f32 t) {
    __REGISTER f32 axy, bxy, az, bz;
#ifdef __MWERKS__
    asm {
        psq_l axy, 0(a), 0, 0
        psq_l bxy, 0(b), 0, 0
        lfs az, 8(a)
        lfs bz, 8(b)
        ps_sub bxy, bxy, axy
        fsubs bz, bz, az
        ps_madds0 bxy, bxy, t, axy
        fmadds bz, bz, t, az
        psq_st bxy, 0(dst), 0, 0
        stfs bz, 8(dst)
    }
#else
    // The original loads all components before storing; only the paired XY
    // store flushes denormals. The scalar Z store retains its IEEE value.
    const f32 x = std::fma(b->x - a->x, t, a->x);
    const f32 y = std::fma(b->y - a->y, t, a->y);
    const f32 z = std::fma(b->z - a->z, t, a->z);
    dst->x = ppc_psq_store_f32(x);
    dst->y = ppc_psq_store_f32(y);
    dst->z = z;
#endif
}

void JMAMTXApplyScale(const Mtx src, Mtx dst, f32 x, f32 y, f32 z) {
    Mtx scale;
    PSMTXScale(scale, x, y, z);
    PSMTXConcat(src, scale, dst);
}
