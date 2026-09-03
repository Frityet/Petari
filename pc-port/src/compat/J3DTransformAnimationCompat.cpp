#include "JSystem/J3DGraphAnimator/J3DAnimation.hpp"
#include "JSystem/J3DGraphBase/J3DTransform.hpp"
#include "JSystem/JMath/JMath.hpp"

#include <bit>
#include <cmath>
#include <limits>

// The original JSystem unit uses -fp_contract off. In particular, BCA lerp
// performs separate multiply and add operations; Hermite uses explicit FMA.
#if defined(__clang__)
#pragma clang fp contract(off)
#elif defined(__GNUC__)
#pragma GCC optimize("fp-contract=off")
#elif defined(_MSC_VER)
#pragma fp_contract(off)
#endif

namespace {
    // fctiwz truncates, saturates finite overflow, and returns INT_MIN for NaN.
    // Preserve its integer result without relying on native out-of-range casts.
    s32 truncatePpcInteger(f32 value) {
        if (std::isnan(value) || value < -2147483648.0f) {
            return std::numeric_limits<s32>::min();
        }
        if (value >= 2147483648.0f) {
            return std::numeric_limits<s32>::max();
        }
        return static_cast<s32>(value);
    }

    s16 narrowPpcRotation(u32 value) {
        return std::bit_cast<s16>(static_cast<u16>(value));
    }

    // slw tests bit 5 and uses bits 0..4 as the unsigned word shift count;
    // sth then stores only the low halfword, including for negative rotations.
    s16 shiftPpcRotation(s32 value, int shift) {
        const u32 amount = static_cast<u32>(shift);
        const u32 shifted = (amount & 0x20U) != 0 ? 0U : static_cast<u32>(value) << (amount & 0x1FU);
        return narrowPpcRotation(shifted);
    }
}  // namespace

// Original J3DAnimation.cpp transform samplers. Only the PPC integer
// conversion/shift/store operations above require native adaptations.
J3DAnmTransform::J3DAnmTransform(s16 frameMax, f32 *pScaleData, s16 *pRotData, f32 *pTransData) : J3DAnmBase(frameMax) {
    mScaleData = pScaleData;
    mRotData = pRotData;
    mTransData = pTransData;
    field_0x18 = 0;
    field_0x1a = 0;
    field_0x1c = 0;
    field_0x1e = 0;
}

J3DAnmTransformFull::~J3DAnmTransformFull() {
}

void J3DAnmTransformFull::getTransform(u16 jointNo, J3DTransformInfo *pTransform) const {
    u16 idx = jointNo * 3;
    J3DAnmTransformFullTable *entryX = &mAnmTable[idx];
    J3DAnmTransformFullTable *entryY = &mAnmTable[idx + 1];
    J3DAnmTransformFullTable *entryZ = &mAnmTable[idx + 2];

    if (mFrame < 0.0f) {
        pTransform->mScale.x = mScaleData[entryX->mScaleOffset];
        pTransform->mScale.y = mScaleData[entryY->mScaleOffset];
        pTransform->mScale.z = mScaleData[entryZ->mScaleOffset];

        pTransform->mRotation.x = mRotData[entryX->mRotationOffset];
        pTransform->mRotation.y = mRotData[entryY->mRotationOffset];
        pTransform->mRotation.z = mRotData[entryZ->mRotationOffset];

        pTransform->mTranslate.x = mTransData[entryX->mTranslateOffset];
        pTransform->mTranslate.y = mTransData[entryY->mTranslateOffset];
        pTransform->mTranslate.z = mTransData[entryZ->mTranslateOffset];
    } else {
        u32 frame_max;
        u32 frame = static_cast<u32>(truncatePpcInteger(mFrame + 0.5f));

        frame_max = entryX->mScaleMaxFrame;
        if (frame >= frame_max) {
            pTransform->mScale.x = mScaleData[entryX->mScaleOffset + (frame_max - 1)];
        } else {
            pTransform->mScale.x = mScaleData[entryX->mScaleOffset + frame];
        }

        frame_max = entryX->mRotationMaxFrame;
        if (frame >= frame_max) {
            pTransform->mRotation.x = mRotData[entryX->mRotationOffset + (frame_max - 1)];
        } else {
            pTransform->mRotation.x = mRotData[entryX->mRotationOffset + frame];
        }

        frame_max = entryX->mTranslateMaxFrame;
        if (frame >= frame_max) {
            pTransform->mTranslate.x = mTransData[entryX->mTranslateOffset + (frame_max - 1)];
        } else {
            pTransform->mTranslate.x = mTransData[entryX->mTranslateOffset + frame];
        }

        frame_max = entryY->mScaleMaxFrame;
        if (frame >= frame_max) {
            pTransform->mScale.y = mScaleData[entryY->mScaleOffset + (frame_max - 1)];
        } else {
            pTransform->mScale.y = mScaleData[entryY->mScaleOffset + frame];
        }

        frame_max = entryY->mRotationMaxFrame;
        if (frame >= frame_max) {
            pTransform->mRotation.y = mRotData[entryY->mRotationOffset + (frame_max - 1)];
        } else {
            pTransform->mRotation.y = mRotData[entryY->mRotationOffset + frame];
        }

        frame_max = entryY->mTranslateMaxFrame;
        if (frame >= frame_max) {
            pTransform->mTranslate.y = mTransData[entryY->mTranslateOffset + (frame_max - 1)];
        } else {
            pTransform->mTranslate.y = mTransData[entryY->mTranslateOffset + frame];
        }

        frame_max = entryZ->mScaleMaxFrame;
        if (frame >= frame_max) {
            pTransform->mScale.z = mScaleData[entryZ->mScaleOffset + (frame_max - 1)];
        } else {
            pTransform->mScale.z = mScaleData[entryZ->mScaleOffset + frame];
        }

        frame_max = entryZ->mRotationMaxFrame;
        if (frame >= frame_max) {
            pTransform->mRotation.z = mRotData[entryZ->mRotationOffset + (frame_max - 1)];
        } else {
            pTransform->mRotation.z = mRotData[entryZ->mRotationOffset + frame];
        }

        frame_max = entryZ->mTranslateMaxFrame;
        if (frame >= frame_max) {
            pTransform->mTranslate.z = mTransData[entryZ->mTranslateOffset + (frame_max - 1)];
        } else {
            pTransform->mTranslate.z = mTransData[entryZ->mTranslateOffset + frame];
        }
    }
}

void J3DAnmTransformFullWithLerp::getTransform(u16 jointNo, J3DTransformInfo *pTransform) const {
    u16 idx = jointNo * 3;
    J3DAnmTransformFullTable *entryX = &mAnmTable[idx];
    J3DAnmTransformFullTable *entryY = &mAnmTable[idx + 1];
    J3DAnmTransformFullTable *entryZ = &mAnmTable[idx + 2];

    if (mFrame < 0.0f) {
        pTransform->mScale.x = mScaleData[entryX->mScaleOffset];
        pTransform->mScale.y = mScaleData[entryY->mScaleOffset];
        pTransform->mScale.z = mScaleData[entryZ->mScaleOffset];

        pTransform->mRotation.x = mRotData[entryX->mRotationOffset];
        pTransform->mRotation.y = mRotData[entryY->mRotationOffset];
        pTransform->mRotation.z = mRotData[entryZ->mRotationOffset];

        pTransform->mTranslate.x = mTransData[entryX->mTranslateOffset];
        pTransform->mTranslate.y = mTransData[entryY->mTranslateOffset];
        pTransform->mTranslate.z = mTransData[entryZ->mTranslateOffset];
    } else {
        u32 frame_max;
        int frame = truncatePpcInteger(mFrame);

        if (frame == mFrame) {
            frame_max = entryX->mScaleMaxFrame;
            if (frame >= frame_max) {
                pTransform->mScale.x = mScaleData[entryX->mScaleOffset + (frame_max - 1)];
            } else {
                pTransform->mScale.x = mScaleData[entryX->mScaleOffset + frame];
            }

            frame_max = entryX->mRotationMaxFrame;
            if (frame >= frame_max) {
                pTransform->mRotation.x = mRotData[entryX->mRotationOffset + (frame_max - 1)];
            } else {
                pTransform->mRotation.x = mRotData[entryX->mRotationOffset + frame];
            }

            frame_max = entryX->mTranslateMaxFrame;
            if (frame >= frame_max) {
                pTransform->mTranslate.x = mTransData[entryX->mTranslateOffset + (frame_max - 1)];
            } else {
                pTransform->mTranslate.x = mTransData[entryX->mTranslateOffset + frame];
            }

            frame_max = entryY->mScaleMaxFrame;
            if (frame >= frame_max) {
                pTransform->mScale.y = mScaleData[entryY->mScaleOffset + (frame_max - 1)];
            } else {
                pTransform->mScale.y = mScaleData[entryY->mScaleOffset + frame];
            }

            frame_max = entryY->mRotationMaxFrame;
            if (frame >= frame_max) {
                pTransform->mRotation.y = mRotData[entryY->mRotationOffset + (frame_max - 1)];
            } else {
                pTransform->mRotation.y = mRotData[entryY->mRotationOffset + frame];
            }

            frame_max = entryY->mTranslateMaxFrame;
            if (frame >= frame_max) {
                pTransform->mTranslate.y = mTransData[entryY->mTranslateOffset + (frame_max - 1)];
            } else {
                pTransform->mTranslate.y = mTransData[entryY->mTranslateOffset + frame];
            }

            frame_max = entryZ->mScaleMaxFrame;
            if (frame >= frame_max) {
                pTransform->mScale.z = mScaleData[entryZ->mScaleOffset + (frame_max - 1)];
            } else {
                pTransform->mScale.z = mScaleData[entryZ->mScaleOffset + frame];
            }

            frame_max = entryZ->mRotationMaxFrame;
            if (frame >= frame_max) {
                pTransform->mRotation.z = mRotData[entryZ->mRotationOffset + (frame_max - 1)];
            } else {
                pTransform->mRotation.z = mRotData[entryZ->mRotationOffset + frame];
            }

            frame_max = entryZ->mTranslateMaxFrame;
            if (frame >= frame_max) {
                pTransform->mTranslate.z = mTransData[entryZ->mTranslateOffset + (frame_max - 1)];
            } else {
                pTransform->mTranslate.z = mTransData[entryZ->mTranslateOffset + frame];
            }
        } else {
            f32 rate = mFrame - frame;
            f32 var_f30 = 1.0f - rate;

            u32 next_frame = static_cast<u32>(frame) + 1U;

            frame_max = entryX->mScaleMaxFrame;
            if (next_frame >= frame_max) {
                pTransform->mScale.x = mScaleData[entryX->mScaleOffset + (frame_max - 1)];
            } else {
                pTransform->mScale.x = mScaleData[entryX->mScaleOffset + frame] +
                                       rate * (mScaleData[entryX->mScaleOffset + next_frame] - mScaleData[entryX->mScaleOffset + frame]);
            }

            frame_max = entryX->mRotationMaxFrame;
            if (next_frame >= frame_max) {
                pTransform->mRotation.x = mRotData[entryX->mRotationOffset + (frame_max - 1)];
            } else {
                u32 rot1 = (u16)mRotData[entryX->mRotationOffset + frame];
                u32 rot2 = (u16)mRotData[entryX->mRotationOffset + next_frame];
                int delta = static_cast<int>(rot2) - static_cast<int>(rot1);
                if (delta > 0x8000) {
                    rot1 += 0x10000;
                    delta -= 0x10000;
                } else if (-delta > 0x8000) {
                    delta += 0x10000;
                }
                pTransform->mRotation.x = narrowPpcRotation(static_cast<u32>((f32)rot1 + rate * (f32)delta));
            }

            frame_max = entryX->mTranslateMaxFrame;
            if (next_frame >= frame_max) {
                pTransform->mTranslate.x = mTransData[entryX->mTranslateOffset + (frame_max - 1)];
            } else {
                pTransform->mTranslate.x = mTransData[entryX->mTranslateOffset + frame] +
                                           rate * (mTransData[entryX->mTranslateOffset + next_frame] - mTransData[entryX->mTranslateOffset + frame]);
            }

            frame_max = entryY->mScaleMaxFrame;
            if (next_frame >= frame_max) {
                pTransform->mScale.y = mScaleData[entryY->mScaleOffset + (frame_max - 1)];
            } else {
                pTransform->mScale.y = mScaleData[entryY->mScaleOffset + frame] +
                                       rate * (mScaleData[entryY->mScaleOffset + next_frame] - mScaleData[entryY->mScaleOffset + frame]);
            }

            frame_max = entryY->mRotationMaxFrame;
            if (next_frame >= frame_max) {
                pTransform->mRotation.y = mRotData[entryY->mRotationOffset + (frame_max - 1)];
            } else {
                u32 rot1 = (u16)mRotData[entryY->mRotationOffset + frame];
                u32 rot2 = (u16)mRotData[entryY->mRotationOffset + next_frame];
                int delta = static_cast<int>(rot2) - static_cast<int>(rot1);
                if (delta > 0x8000) {
                    rot1 += 0x10000;
                    delta -= 0x10000;
                } else if (-delta > 0x8000) {
                    delta += 0x10000;
                }
                pTransform->mRotation.y = narrowPpcRotation(static_cast<u32>((f32)rot1 + rate * (f32)delta));
            }

            frame_max = entryY->mTranslateMaxFrame;
            if (next_frame >= frame_max) {
                pTransform->mTranslate.y = mTransData[entryY->mTranslateOffset + (frame_max - 1)];
            } else {
                pTransform->mTranslate.y = mTransData[entryY->mTranslateOffset + frame] +
                                           rate * (mTransData[entryY->mTranslateOffset + next_frame] - mTransData[entryY->mTranslateOffset + frame]);
            }

            frame_max = entryZ->mScaleMaxFrame;
            if (next_frame >= frame_max) {
                pTransform->mScale.z = mScaleData[entryZ->mScaleOffset + (frame_max - 1)];
            } else {
                pTransform->mScale.z = mScaleData[entryZ->mScaleOffset + frame] +
                                       rate * (mScaleData[entryZ->mScaleOffset + next_frame] - mScaleData[entryZ->mScaleOffset + frame]);
            }

            frame_max = entryZ->mRotationMaxFrame;
            if (next_frame >= frame_max) {
                pTransform->mRotation.z = mRotData[entryZ->mRotationOffset + (frame_max - 1)];
            } else {
                u32 rot1 = (u16)mRotData[entryZ->mRotationOffset + frame];
                u32 rot2 = (u16)mRotData[entryZ->mRotationOffset + next_frame];
                int delta = static_cast<int>(rot2) - static_cast<int>(rot1);
                if (delta > 0x8000) {
                    rot1 += 0x10000;
                    delta -= 0x10000;
                } else if (-delta > 0x8000) {
                    delta += 0x10000;
                }
                pTransform->mRotation.z = narrowPpcRotation(static_cast<u32>((f32)rot1 + rate * (f32)delta));
            }

            frame_max = entryZ->mTranslateMaxFrame;
            if (next_frame >= frame_max) {
                pTransform->mTranslate.z = mTransData[entryZ->mTranslateOffset + (frame_max - 1)];
            } else {
                pTransform->mTranslate.z = mTransData[entryZ->mTranslateOffset + frame] +
                                           rate * (mTransData[entryZ->mTranslateOffset + next_frame] - mTransData[entryZ->mTranslateOffset + frame]);
            }
        }
    }
}

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

void J3DAnmTransformKey::calcTransform(f32 frame, u16 jointNo, J3DTransformInfo *pTransform) const {
    u16 idx = jointNo * 3;
    J3DAnmTransformKeyTable *entryX = &mAnmTable[idx];
    J3DAnmTransformKeyTable *entryY = &mAnmTable[idx + 1];
    J3DAnmTransformKeyTable *entryZ = &mAnmTable[idx + 2];

    switch (entryX->mScaleInfo.mMaxFrame) {
    case 0:
        pTransform->mScale.x = 1.0f;
        break;
    case 1:
        pTransform->mScale.x = mScaleData[entryX->mScaleInfo.mOffset];
        break;
    default:
        pTransform->mScale.x = J3DGetKeyFrameInterpolation(frame, &entryX->mScaleInfo, &mScaleData[entryX->mScaleInfo.mOffset]);
    }

    switch (entryY->mScaleInfo.mMaxFrame) {
    case 0:
        pTransform->mScale.y = 1.0f;
        break;
    case 1:
        pTransform->mScale.y = mScaleData[entryY->mScaleInfo.mOffset];
        break;
    default:
        pTransform->mScale.y = J3DGetKeyFrameInterpolation(frame, &entryY->mScaleInfo, &mScaleData[entryY->mScaleInfo.mOffset]);
    }

    switch (entryZ->mScaleInfo.mMaxFrame) {
    case 0:
        pTransform->mScale.z = 1.0f;
        break;
    case 1:
        pTransform->mScale.z = mScaleData[entryZ->mScaleInfo.mOffset];
        break;
    default:
        pTransform->mScale.z = J3DGetKeyFrameInterpolation(frame, &entryZ->mScaleInfo, &mScaleData[entryZ->mScaleInfo.mOffset]);
    }

    switch (entryX->mRotationInfo.mMaxFrame) {
    case 0:
        pTransform->mRotation.x = 0;
        break;
    case 1:
        pTransform->mRotation.x = shiftPpcRotation(mRotData[entryX->mRotationInfo.mOffset], mDecShift);
        break;
    default:
        pTransform->mRotation.x = shiftPpcRotation(truncatePpcInteger(J3DGetKeyFrameInterpolation(frame, &entryX->mRotationInfo,
                                                                                                  &mRotData[entryX->mRotationInfo.mOffset])),
                                                   mDecShift);
    }

    switch (entryY->mRotationInfo.mMaxFrame) {
    case 0:
        pTransform->mRotation.y = 0;
        break;
    case 1:
        pTransform->mRotation.y = shiftPpcRotation(mRotData[entryY->mRotationInfo.mOffset], mDecShift);
        break;
    default:
        pTransform->mRotation.y = shiftPpcRotation(truncatePpcInteger(J3DGetKeyFrameInterpolation(frame, &entryY->mRotationInfo,
                                                                                                  &mRotData[entryY->mRotationInfo.mOffset])),
                                                   mDecShift);
    }

    switch (entryZ->mRotationInfo.mMaxFrame) {
    case 0:
        pTransform->mRotation.z = 0;
        break;
    case 1:
        pTransform->mRotation.z = shiftPpcRotation(mRotData[entryZ->mRotationInfo.mOffset], mDecShift);
        break;
    default:
        pTransform->mRotation.z = shiftPpcRotation(truncatePpcInteger(J3DGetKeyFrameInterpolation(frame, &entryZ->mRotationInfo,
                                                                                                  &mRotData[entryZ->mRotationInfo.mOffset])),
                                                   mDecShift);
    }

    switch (entryX->mTranslateInfo.mMaxFrame) {
    case 0:
        pTransform->mTranslate.x = 0.0f;
        break;
    case 1:
        pTransform->mTranslate.x = mTransData[entryX->mTranslateInfo.mOffset];
        break;
    default:
        pTransform->mTranslate.x = J3DGetKeyFrameInterpolation(frame, &entryX->mTranslateInfo, &mTransData[entryX->mTranslateInfo.mOffset]);
    }

    switch (entryY->mTranslateInfo.mMaxFrame) {
    case 0:
        pTransform->mTranslate.y = 0.0f;
        break;
    case 1:
        pTransform->mTranslate.y = mTransData[entryY->mTranslateInfo.mOffset];
        break;
    default:
        pTransform->mTranslate.y = J3DGetKeyFrameInterpolation(frame, &entryY->mTranslateInfo, &mTransData[entryY->mTranslateInfo.mOffset]);
    }

    switch (entryZ->mTranslateInfo.mMaxFrame) {
    case 0:
        pTransform->mTranslate.z = 0.0f;
        break;
    case 1:
        pTransform->mTranslate.z = mTransData[entryZ->mTranslateInfo.mOffset];
        break;
    default:
        pTransform->mTranslate.z = J3DGetKeyFrameInterpolation(frame, &entryZ->mTranslateInfo, &mTransData[entryZ->mTranslateInfo.mOffset]);
    }
}
