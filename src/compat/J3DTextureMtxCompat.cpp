#include "JSystem/J3DGraphBase/J3DTransform.hpp"
#include "JSystem/J3DGraphBase/J3DStruct.hpp"
#include "JSystem/JMath/JMATrigonometric.hpp"
#include <cmath>

#if defined(__clang__)
#pragma clang fp contract(off)
#endif

void J3DGetTextureMtx(const J3DTextureSRTInfo& srt, const Vec& center, Mtx dst) {
    f32 sr = JMASSin(srt.mRotation), cr = JMASCos(srt.mRotation);

    f32 cx = srt.mScaleX * cr;
    f32 sx = srt.mScaleX * sr;
    f32 sy = srt.mScaleY * sr;
    f32 cy = srt.mScaleY * cr;

    dst[0][0] = cx;
    dst[0][1] = -sx;
    dst[0][2] = (-cx * center.x + sx * center.y) + center.x + srt.mTranslationX;

    dst[1][0] = sy;
    dst[1][1] = cy;
    dst[1][2] = (-sy * center.x - cy * center.y) + center.y + srt.mTranslationY;

    dst[0][3] = dst[1][3] = dst[2][0] = dst[2][1] = dst[2][3] = 0.0f;
    dst[2][2] = 1.0f;
}

void J3DGetTextureMtxOld(const J3DTextureSRTInfo& srt, const Vec& center, Mtx dst) {
    f32 sr = JMASSin(srt.mRotation), cr = JMASCos(srt.mRotation);

    f32 cx = srt.mScaleX * cr;
    f32 sx = srt.mScaleX * sr;
    f32 sy = srt.mScaleY * sr;
    f32 cy = srt.mScaleY * cr;

    dst[0][0] = cx;
    dst[0][1] = -sx;
    dst[0][3] = (-cx * center.x + sx * center.y) + center.x + srt.mTranslationX;

    dst[1][0] = sy;
    dst[1][1] = cy;
    dst[1][3] = (-sy * center.x - cy * center.y) + center.y + srt.mTranslationY;

    dst[0][2] = dst[1][2] = dst[2][0] = dst[2][1] = dst[2][3] = 0.0f;
    dst[2][2] = 1.0f;
}

void J3DGetTextureMtxMaya(const J3DTextureSRTInfo& srt, Mtx dst) {
    f32 sr = JMASSin(srt.mRotation), cr = JMASCos(srt.mRotation);
    f32 tx = srt.mTranslationX - 0.5f;
    f32 ty = srt.mTranslationY - 0.5f;

    dst[0][0] = srt.mScaleX * cr;
    dst[0][1] = srt.mScaleY * sr;
    dst[0][2] = tx * cr - sr * (ty + srt.mScaleY) + 0.5f;

    dst[1][0] = -srt.mScaleX * sr;
    dst[1][1] = srt.mScaleY * cr;
    dst[1][2] = -tx * sr - cr * (ty + srt.mScaleY) + 0.5f;

    dst[0][3] = dst[1][3] = dst[2][0] = dst[2][1] = dst[2][3] = 0.0f;
    dst[2][2] = 1.0f;
}

void J3DGetTextureMtxMayaOld(const J3DTextureSRTInfo& srt, Mtx dst) {
    f32 sr = JMASSin(srt.mRotation), cr = JMASCos(srt.mRotation);
    f32 tx = srt.mTranslationX - 0.5f;
    f32 ty = srt.mTranslationY - 0.5f;

    dst[0][0] = srt.mScaleX * cr;
    dst[0][1] = srt.mScaleY * sr;
    dst[0][3] = tx * cr - sr * (ty + srt.mScaleY) + 0.5f;

    dst[1][0] = -srt.mScaleX * sr;
    dst[1][1] = srt.mScaleY * cr;
    dst[1][3] = -tx * sr - cr * (ty + srt.mScaleY) + 0.5f;

    dst[0][2] = dst[1][2] = dst[2][0] = dst[2][1] = dst[2][3] = 0.0f;
    dst[2][2] = 1.0f;
}

void J3DMtxProjConcat(Mtx mtx1, Mtx mtx2, Mtx dst) {
#if defined(__clang__)
#pragma clang fp contract(off)
#endif
    for (u32 row = 0; row < 3; row++) {
        const f32 x = mtx1[row][0];
        const f32 y = mtx1[row][1];
        const f32 z = mtx1[row][2];
        const f32 w = mtx1[row][3];
        for (u32 column = 0; column < 4; column += 2) {
            f32 first = x * mtx2[0][column];
            f32 second = x * mtx2[0][column + 1];
            first = std::fma(y, mtx2[1][column], first);
            second = std::fma(y, mtx2[1][column + 1], second);
            first = std::fma(z, mtx2[2][column], first);
            second = std::fma(z, mtx2[2][column + 1], second);
            first = std::fma(w, mtx2[3][column], first);
            second = std::fma(w, mtx2[3][column + 1], second);
            dst[row][column] = first;
            dst[row][column + 1] = second;
        }
    }
}
