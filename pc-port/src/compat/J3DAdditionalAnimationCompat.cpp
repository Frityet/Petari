#include "JSystem/J3DGraphAnimator/J3DAnimation.hpp"

// Original cluster, visibility and vertex-color sampler families. Keep the
// SDK's unfused scalar operations and explicit Hermite fusion order.
#if defined(__clang__)
#pragma clang fp contract(off)
#elif defined(__GNUC__)
#pragma GCC optimize("fp-contract=off")
#elif defined(_MSC_VER)
#pragma fp_contract(off)
#endif
#include "compat/J3DAnimationInterpolation.hpp"

void J3DAnmVisibilityFull::getVisibility(u16 index, u8* pVisibility) const {
    int maxFrame = mAnmTable[index]._0;
    int frame = truncatePpcInteger(0.5f + mFrame);
    if (frame < 0) {
        *pVisibility = mVisibility[mAnmTable[index]._2];
    } else if (frame >= maxFrame) {
        *pVisibility = mVisibility[mAnmTable[index]._2 + maxFrame - 1];
    } else {
        *pVisibility = mVisibility[mAnmTable[index]._2 + frame];
    }
}

f32 J3DAnmClusterFull::getWeight(u16 index) const {
    int maxFrame = mAnmTable[index].mMaxFrame;
    int frame = truncatePpcInteger(mFrame + 0.5f);

    if (mFrame < 0.0f) {
        return mWeight[mAnmTable[index].mOffset];
    } else if (frame >= (u16)maxFrame) {
        return mWeight[mAnmTable[index].mOffset + ((u16)maxFrame - 1)];
    } else {
        return mWeight[mAnmTable[index].mOffset + frame];
    }
}

f32 J3DAnmClusterKey::getWeight(u16 index) const {
    int maxFrame = (u16)mAnmTable[index].mWeightTable.mMaxFrame;
    switch (maxFrame) {
    case 0:
        return 1.0f;
    case 1:
        return mWeight[mAnmTable[index].mWeightTable.mOffset];
    default:
        return J3DGetKeyFrameInterpolation(mFrame, &mAnmTable[index].mWeightTable, &mWeight[mAnmTable[index].mWeightTable.mOffset]);
    }
}

J3DAnmVtxColor::J3DAnmVtxColor() {
    for (int i = 0; i < 2; i++) {
        mAnmTableNum[i] = 0;
    }
    for (int i = 0; i < 2; i++) {
        mAnmVtxColorIndexData[i] = NULL;
    }
}

J3DAnmVtxColorFull::J3DAnmVtxColorFull() {
    for (int i = 0; i < 2; i++) {
        mpTable[i] = NULL;
    }
}

void J3DAnmVtxColorFull::getColor(u8 tableNo, u16 index, GXColor* pColor) const {
    J3DAnmColorFullTable* entry = &mpTable[tableNo][index];

    if (mFrame < 0.0f) {
        pColor->r = mColorR[entry->mROffset];
        pColor->g = mColorG[entry->mGOffset];
        pColor->b = mColorB[entry->mBOffset];
        pColor->a = mColorA[entry->mAOffset];
    } else {
        int frame = truncatePpcInteger(mFrame + 0.5f);
        u16 maxFrame;

        maxFrame = entry->mRMaxFrame;
        if (frame >= maxFrame) {
            pColor->r = mColorR[entry->mROffset + (maxFrame - 1)];
        } else {
            pColor->r = mColorR[entry->mROffset + frame];
        }

        maxFrame = entry->mGMaxFrame;
        if (frame >= maxFrame) {
            pColor->g = mColorG[entry->mGOffset + (maxFrame - 1)];
        } else {
            pColor->g = mColorG[entry->mGOffset + frame];
        }

        maxFrame = entry->mBMaxFrame;
        if (frame >= maxFrame) {
            pColor->b = mColorB[entry->mBOffset + (maxFrame - 1)];
        } else {
            pColor->b = mColorB[entry->mBOffset + frame];
        }

        maxFrame = entry->mAMaxFrame;
        if (frame >= maxFrame) {
            pColor->a = mColorA[entry->mAOffset + (maxFrame - 1)];
        } else {
            pColor->a = mColorA[entry->mAOffset + frame];
        }
    }
}

J3DAnmVtxColorKey::J3DAnmVtxColorKey() {
    for (int i = 0; i < 2; i++) {
        mpTable[i] = 0;
    }
}

void J3DAnmVtxColorKey::getColor(u8 tableNo, u16 index, GXColor* pColor) const {
    J3DAnmColorKeyTable* entry = &mpTable[tableNo][index];

    f32 col;
    switch (entry->mRInfo.mMaxFrame) {
    case 0:
        pColor->r = 0;
        break;
    case 1:
        pColor->r = mColorR[entry->mRInfo.mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(mFrame, &entry->mRInfo, &mColorR[entry->mRInfo.mOffset]);
        if (col <= 0.0f) {
            pColor->r = 0;
        } else if (col <= 255.0f) {
            OSf32tou8(&col, &pColor->r);
        } else {
            pColor->r = 255;
        }
    }

    switch (entry->mGInfo.mMaxFrame) {
    case 0:
        pColor->g = 0;
        break;
    case 1:
        pColor->g = mColorG[entry->mGInfo.mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(mFrame, &entry->mGInfo, &mColorG[entry->mGInfo.mOffset]);
        if (col <= 0.0f) {
            pColor->g = 0;
        } else if (col <= 255.0f) {
            OSf32tou8(&col, &pColor->g);
        } else {
            pColor->g = 255;
        }
    }

    switch (entry->mBInfo.mMaxFrame) {
    case 0:
        pColor->b = 0;
        break;
    case 1:
        pColor->b = mColorB[entry->mBInfo.mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(mFrame, &entry->mBInfo, &mColorB[entry->mBInfo.mOffset]);
        if (col <= 0.0f) {
            pColor->b = 0;
        } else if (col <= 255.0f) {
            OSf32tou8(&col, &pColor->b);
        } else {
            pColor->b = 255;
        }
    }

    switch (entry->mAInfo.mMaxFrame) {
    case 0:
        pColor->a = 0;
        break;
    case 1:
        pColor->a = mColorA[entry->mAInfo.mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(mFrame, &entry->mAInfo, &mColorA[entry->mAInfo.mOffset]);
        if (col <= 0.0f) {
            pColor->a = 0;
        } else if (col <= 255.0f) {
            OSf32tou8(&col, &pColor->a);
        } else {
            pColor->a = 255;
        }
    }
}

