#include "JSystem/J3DGraphBase/J3DTransform.hpp"
#include "JSystem/JMath.hpp"

#include <cmath>

#if defined(__clang__)
#pragma clang fp contract(off)
#endif

// Original RMGK01 constants and matrix helpers. The paired-single routines
// use the portable branches restored in root J3DTransform.cpp.
const Vec j3dDefaultScale = {1.0f, 1.0f, 1.0f};
const Mtx j3dDefaultMtx = {{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}};

void J3DCalcBBoardMtx(register Mtx mtx) {
    f32 x = (mtx[0][0] * mtx[0][0]) + (mtx[1][0] * mtx[1][0]) + (mtx[2][0] * mtx[2][0]);
    f32 y = (mtx[0][1] * mtx[0][1]) + (mtx[1][1] * mtx[1][1]) + (mtx[2][1] * mtx[2][1]);
    f32 z = (mtx[0][2] * mtx[0][2]) + (mtx[1][2] * mtx[1][2]) + (mtx[2][2] * mtx[2][2]);

    if (x > 0.0f) {
        x = sqrt(x);
    }
    if (y > 0.0f) {
        y = sqrt(y);
    }
    if (z > 0.0f) {
        z = sqrt(z);
    }

    f32 zero = 0.0f;
    mtx[0][0] = x;
    mtx[0][1] = zero;
    mtx[0][2] = zero;
    mtx[2][0] = zero;
    mtx[2][1] = zero;
    mtx[1][0] = zero;
    mtx[1][1] = y;
    mtx[1][2] = zero;
    mtx[2][2] = z;
}

void J3DCalcYBBoardMtx(Mtx mtx) {
    f32 x = (mtx[0][0] * mtx[0][0]) + (mtx[1][0] * mtx[1][0]) + (mtx[2][0] * mtx[2][0]);
    f32 z = (mtx[0][2] * mtx[0][2]) + (mtx[1][2] * mtx[1][2]) + (mtx[2][2] * mtx[2][2]);

    if (x > 0.0f) {
        x = JMath::fastSqrt(x);
    }
    if (z > 0.0f) {
        z = JMath::fastSqrt(z);
    }

    Vec vec = {0.0f, -mtx[2][1], mtx[1][1]};
    PSVECNormalize(&vec, &vec);

    mtx[0][0] = x;
    mtx[0][2] = 0.0f;
    mtx[1][0] = 0.0f;

    mtx[1][2] = vec.y * z;
    mtx[2][0] = 0.0f;
    mtx[2][2] = vec.z * z;
}

void J3DPSCalcInverseTranspose(Mtx src, Mtx33 dst) {
    f32 a = src[0][0], b = src[0][1], c = src[0][2];
    f32 d = src[1][0], e = src[1][1], f = src[1][2];
    f32 g = src[2][0], h = src[2][1], i = src[2][2];

    f32 x0 = std::fma(e, i, -(h * f));
    f32 x1 = std::fma(f, g, -(i * d));
    f32 y0 = std::fma(h, c, -(b * i));
    f32 y1 = std::fma(i, a, -(c * g));
    f32 z0 = std::fma(b, f, -(e * c));
    f32 z1 = std::fma(c, d, -(f * a));
    f32 x2 = std::fma(d, h, -(e * g));
    f32 y2 = std::fma(b, g, -(a * h));
    f32 z2 = std::fma(a, e, -(b * d));
    f32 determinant = a * x0;
    determinant = std::fma(d, y0, determinant);
    determinant = std::fma(g, z0, determinant);
    if (determinant == 0.0f) {
        return;
    }

    f32 inverse = JMath::fastReciprocal(determinant);
    f32 twice = inverse + inverse;
    f32 square = inverse * inverse;
    inverse = -std::fma(determinant, square, -twice);
    twice = inverse + inverse;
    square = inverse * inverse;
    inverse = -std::fma(determinant, square, -twice);

    dst[0][0] = x0 * inverse;
    dst[0][1] = x1 * inverse;
    dst[1][0] = y0 * inverse;
    dst[1][1] = y1 * inverse;
    dst[2][0] = z0 * inverse;
    dst[2][1] = z1 * inverse;
    dst[0][2] = x2 * inverse;
    dst[1][2] = y2 * inverse;
    dst[2][2] = z2 * inverse;
}

void J3DScaleNrmMtx(Mtx mtx, const Vec& scale) {
    for (int row = 0; row < 3; row++) {
        f32 x = scale.x, y = scale.y, z = scale.z;
        f32 xx = mtx[row][0], xy = mtx[row][1], xz = mtx[row][2];
        mtx[row][0] = xx * x;
        mtx[row][1] = xy * y;
        mtx[row][2] = xz * z;
    }
}

void J3DScaleNrmMtx33(Mtx33 mtx, const Vec& scale) {
    f32 x = scale.x, y = scale.y, z = scale.z;
    f32 xx = mtx[0][0], xy = mtx[0][1], xz = mtx[0][2];
    f32 yx = mtx[1][0], yy = mtx[1][1], yz = mtx[1][2];
    f32 zx = mtx[2][0], zy = mtx[2][1], zz = mtx[2][2];
    mtx[0][0] = xx * x;
    mtx[0][1] = xy * y;
    mtx[0][2] = xz * z;
    mtx[1][0] = yx * x;
    mtx[1][1] = yy * y;
    mtx[1][2] = yz * z;
    mtx[2][0] = zx * x;
    mtx[2][1] = zy * y;
    mtx[2][2] = zz * z;
}

void J3DPSMtxArrayConcat(Mtx mA, Mtx mB, Mtx mAB, u32 count) {
    Mtx* source = reinterpret_cast< Mtx* >(mB);
    Mtx* destination = reinterpret_cast< Mtx* >(mAB);
    do {
        Mtx a, b;
        PSMTXCopy(mA, a);
        PSMTXCopy(*source, b);
        for (int row = 0; row < 3; row++) {
            for (int column = 0; column < 4; column++) {
                f32 value = b[0][column] * a[row][0];
                value = std::fma(b[1][column], a[row][1], value);
                value = std::fma(b[2][column], a[row][2], value);
                if (column >= 2) {
                    value = std::fma(column == 3 ? 1.0f : 0.0f, a[row][3], value);
                }
                (*destination)[row][column] = value;
            }
        }
        source++;
        destination++;
    } while (--count != 0);
}

// Original RMGK01 0x80426304: all nine source elements are loaded before
// any destination store, including when the 3x4 and 3x3 buffers overlap.
void J3DPSMtx33CopyFrom34(MtxPtr src, Mtx3P dst) {
    f32 xx = src[0][0], xy = src[0][1], xz = src[0][2];
    f32 yx = src[1][0], yy = src[1][1], yz = src[1][2];
    f32 zx = src[2][0], zy = src[2][1], zz = src[2][2];
    dst[0][0] = xx;
    dst[0][1] = xy;
    dst[0][2] = xz;
    dst[1][0] = yx;
    dst[1][1] = yy;
    dst[1][2] = yz;
    dst[2][0] = zx;
    dst[2][1] = zy;
    dst[2][2] = zz;
}
