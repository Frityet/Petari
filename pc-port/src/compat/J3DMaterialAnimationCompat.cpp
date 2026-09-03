#include "JSystem/J3DGraphAnimator/J3DAnimation.hpp"
#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"
#include "JSystem/J3DGraphBase/J3DMaterial.hpp"

// Original J3DAnimation.cpp material families. Keep SDK scalar contraction
// disabled; original Hermite uses its explicit fused operations.
#if defined(__clang__)
#pragma clang fp contract(off)
#elif defined(__GNUC__)
#pragma GCC optimize("fp-contract=off")
#elif defined(_MSC_VER)
#pragma fp_contract(off)
#endif
#include "compat/J3DAnimationInterpolation.hpp"

J3DAnmTextureSRTKey::J3DAnmTextureSRTKey() {
    mDecShift = 0;
    mTrackNum = mScaleNum = mRotNum = mTransNum = 0;
    mAnmTable = NULL;
    mScaleData = mTransData = NULL;
    mRotData = NULL;
    field_0x4a = field_0x44 = field_0x46 = field_0x48 = 0;
    field_0x58 = NULL;
    field_0x4c = field_0x54 = NULL;
    field_0x50 = NULL;
    mTexMtxCalcType = 0;
}

void J3DAnmTextureSRTKey::calcTransform(f32 frame, u16 jointNo, J3DTextureSRTInfo* pTexSRTInfo) const {
    u16 idx = jointNo * 3;
    J3DAnmTransformKeyTable* entryX = &mAnmTable[idx];
    J3DAnmTransformKeyTable* entryY = &mAnmTable[idx + 1];
    J3DAnmTransformKeyTable* entryRot = &mAnmTable[idx + 2];

    switch (entryX->mScaleInfo.mMaxFrame) {
    case 0:
        pTexSRTInfo->mScaleX = 1.0f;
        break;
    case 1:
        pTexSRTInfo->mScaleX = mScaleData[entryX->mScaleInfo.mOffset];
        break;
    default:
        pTexSRTInfo->mScaleX = J3DGetKeyFrameInterpolation(frame, &entryX->mScaleInfo, &mScaleData[entryX->mScaleInfo.mOffset]);
    }

    switch (entryY->mScaleInfo.mMaxFrame) {
    case 0:
        pTexSRTInfo->mScaleY = 1.0f;
        break;
    case 1:
        pTexSRTInfo->mScaleY = mScaleData[entryY->mScaleInfo.mOffset];
        break;
    default:
        pTexSRTInfo->mScaleY = J3DGetKeyFrameInterpolation(frame, &entryY->mScaleInfo, &mScaleData[entryY->mScaleInfo.mOffset]);
    }

    switch (entryRot->mRotationInfo.mMaxFrame) {
    case 0:
        pTexSRTInfo->mRotation = 0;
        break;
    case 1:
        pTexSRTInfo->mRotation = shiftPpcRotation(mRotData[entryRot->mRotationInfo.mOffset], mDecShift);
        break;
    default:
        pTexSRTInfo->mRotation = shiftPpcRotation(truncatePpcInteger(J3DGetKeyFrameInterpolation(
            frame, &entryRot->mRotationInfo, &mRotData[entryRot->mRotationInfo.mOffset])), mDecShift);
    }

    switch (entryX->mTranslateInfo.mMaxFrame) {
    case 0:
        pTexSRTInfo->mTranslationX = 0.0f;
        break;
    case 1:
        pTexSRTInfo->mTranslationX = mTransData[entryX->mTranslateInfo.mOffset];
        break;
    default:
        pTexSRTInfo->mTranslationX = J3DGetKeyFrameInterpolation(frame, &entryX->mTranslateInfo, &mTransData[entryX->mTranslateInfo.mOffset]);
    }

    switch (entryY->mTranslateInfo.mMaxFrame) {
    case 0:
        pTexSRTInfo->mTranslationY = 0.0f;
        break;
    case 1:
        pTexSRTInfo->mTranslationY = mTransData[entryY->mTranslateInfo.mOffset];
        break;
    default:
        pTexSRTInfo->mTranslationY = J3DGetKeyFrameInterpolation(frame, &entryY->mTranslateInfo, &mTransData[entryY->mTranslateInfo.mOffset]);
    }
}

J3DAnmColor::J3DAnmColor() : field_0xc(0), field_0xe(0), field_0x10(0), field_0x12(0), mUpdateMaterialNum(0), mUpdateMaterialID(NULL) {
}

void J3DAnmColor::searchUpdateMaterialID(J3DMaterialTable* pMatTable) {
    for (u16 i = 0; i < mUpdateMaterialNum; i++) {
        int index = pMatTable->getMaterialName()->getIndex(mUpdateMaterialName.getName(i));
        if (index != -1) {
            mUpdateMaterialID[i] = index;
        } else {
            mUpdateMaterialID[i] = 0xffff;
        }
    }
}

void J3DAnmColor::searchUpdateMaterialID(J3DModelData* pModelData) {
    searchUpdateMaterialID(&pModelData->getMaterialTable());
}

J3DAnmColorFull::J3DAnmColorFull() {
    mColorR = NULL;
    mColorG = NULL;
    mColorB = NULL;
    mColorA = NULL;
    mAnmTable = NULL;
}

void J3DAnmColorFull::getColor(u16 index, GXColor* pColor) const {
    J3DAnmColorFullTable* entry = &mAnmTable[index];

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

J3DAnmColorKey::J3DAnmColorKey() {
    mColorR = NULL;
    mColorG = NULL;
    mColorB = NULL;
    mColorA = NULL;
    mAnmTable = NULL;
}

void J3DAnmColorKey::getColor(u16 index, GXColor* pColor) const {
    J3DAnmColorKeyTable* entry = &mAnmTable[index];

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
        if (col < 0.0f) {
            pColor->r = 0;
        } else if (col > 255.0f) {
            pColor->r = 255;
        } else {
            OSf32tou8(&col, &pColor->r);
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
        if (col < 0.0f) {
            pColor->g = 0;
        } else if (col > 255.0f) {
            pColor->g = 255;
        } else {
            OSf32tou8(&col, &pColor->g);
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
        if (col < 0.0f) {
            pColor->b = 0;
        } else if (col > 255.0f) {
            pColor->b = 255;
        } else {
            OSf32tou8(&col, &pColor->b);
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
        if (col < 0.0f) {
            pColor->a = 0;
        } else if (col > 255.0f) {
            pColor->a = 255;
        } else {
            OSf32tou8(&col, &pColor->a);
        }
    }
}

J3DAnmTevRegKey::J3DAnmTevRegKey() {
    mCRegUpdateMaterialNum = mKRegUpdateMaterialNum = 0;

    mCRegDataCountR = mCRegDataCountG = mCRegDataCountB = mCRegDataCountA = 0;

    mKRegDataCountR = mKRegDataCountG = mKRegDataCountB = mKRegDataCountA = 0;

    mCRegUpdateMaterialID = mKRegUpdateMaterialID = NULL;

    mAnmCRegDataR = mAnmCRegDataG = mAnmCRegDataB = mAnmCRegDataA = NULL;

    mAnmKRegDataR = mAnmKRegDataG = mAnmKRegDataB = mAnmKRegDataA = NULL;
}

J3DAnmTexPattern::J3DAnmTexPattern() : mTextureIndex(NULL), mAnmTable(NULL), field_0x14(0), mUpdateMaterialNum(0), mUpdateMaterialID(NULL) {
}

void J3DAnmTexPattern::getTexNo(u16 index, u16* pTexNo) const {
    u32 maxFrame = mAnmTable[index].mMaxFrame;

    if (mFrame < 0.0f) {
        *pTexNo = mTextureIndex[mAnmTable[index].mOffset];
    } else if (mFrame >= (u16)maxFrame) {
        *pTexNo = mTextureIndex[mAnmTable[index].mOffset + ((u16)maxFrame - 1)];
    } else {
        *pTexNo = mTextureIndex[mAnmTable[index].mOffset + truncatePpcInteger(mFrame)];
    }
}

void J3DAnmTexPattern::searchUpdateMaterialID(J3DMaterialTable* pMatTable) {
    for (u16 i = 0; i < mUpdateMaterialNum; i++) {
        s32 index = pMatTable->getMaterialName()->getIndex(mUpdateMaterialName.getName(i));
        if (index != -1) {
            mUpdateMaterialID[i] = index;
        } else {
            mUpdateMaterialID[i] = -1;
        }
    }
}

void J3DAnmTexPattern::searchUpdateMaterialID(J3DModelData* pModelData) {
    searchUpdateMaterialID(&pModelData->getMaterialTable());
}

void J3DAnmTextureSRTKey::searchUpdateMaterialID(J3DMaterialTable* pMatTable) {
    for (u16 i = 0; i < getUpdateMaterialNum(); i++) {
        s32 index = pMatTable->getMaterialName()->getIndex(mUpdateMaterialName.getName(i));
        if (index != -1) {
            mUpdateMaterialID[i] = index;
        } else {
            mUpdateMaterialID[i] = -1;
        }
    }

    for (u16 i = 0; i < getPostUpdateMaterialNum(); i++) {
        s32 index = pMatTable->getMaterialName()->getIndex(mPostUpdateMaterialName.getName(i));
        if (index != -1) {
            mPostUpdateMaterialID[i] = index;
        } else {
            mPostUpdateMaterialID[i] = -1;
        }
    }
}

void J3DAnmTextureSRTKey::searchUpdateMaterialID(J3DModelData* pModelData) {
    searchUpdateMaterialID(&pModelData->getMaterialTable());
}

void J3DAnmTevRegKey::getTevColorReg(u16 index, GXColorS10* pColor) const {
    J3DAnmCRegKeyTable* entry = &mAnmCRegKeyTable[index];

    f32 col;
    switch (entry->mRTable.mMaxFrame) {
    case 0:
        pColor->r = 0;
        break;
    case 1:
        pColor->r = mAnmCRegDataR[entry->mRTable.mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(mFrame, &entry->mRTable, &mAnmCRegDataR[entry->mRTable.mOffset]);
        if (col < -0x400) {
            pColor->r = -0x400;
        } else if (col > 0x3FF) {
            pColor->r = 0x3FF;
        } else {
            OSf32tos16(&col, &pColor->r);
        }
    }

    switch (entry->mGTable.mMaxFrame) {
    case 0:
        pColor->g = 0;
        break;
    case 1:
        pColor->g = mAnmCRegDataG[entry->mGTable.mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(mFrame, &entry->mGTable, &mAnmCRegDataG[entry->mGTable.mOffset]);
        if (col < -0x400) {
            pColor->g = -0x400;
        } else if (col > 0x3FF) {
            pColor->g = 0x3FF;
        } else {
            OSf32tos16(&col, &pColor->g);
        }
    }

    switch (entry->mBTable.mMaxFrame) {
    case 0:
        pColor->b = 0;
        break;
    case 1:
        pColor->b = mAnmCRegDataB[entry->mBTable.mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(mFrame, &entry->mBTable, &mAnmCRegDataB[entry->mBTable.mOffset]);
        if (col < -0x400) {
            pColor->b = -0x400;
        } else if (col > 0x3FF) {
            pColor->b = 0x3FF;
        } else {
            OSf32tos16(&col, &pColor->b);
        }
    }

    switch (entry->mATable.mMaxFrame) {
    case 0:
        pColor->a = 0;
        break;
    case 1:
        pColor->a = mAnmCRegDataA[entry->mATable.mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(mFrame, &entry->mATable, &mAnmCRegDataA[entry->mATable.mOffset]);
        if (col < -0x400) {
            pColor->a = -0x400;
        } else if (col > 0x3FF) {
            pColor->a = 0x3FF;
        } else {
            OSf32tos16(&col, &pColor->a);
        }
    }
}

void J3DAnmTevRegKey::getTevKonstReg(u16 index, GXColor* pColor) const {
    J3DAnmKRegKeyTable* entry = &mAnmKRegKeyTable[index];

    f32 col;
    switch (entry->mRTable.mMaxFrame) {
    case 0:
        pColor->r = 0;
        break;
    case 1:
        pColor->r = mAnmKRegDataR[entry->mRTable.mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(mFrame, &entry->mRTable, &mAnmKRegDataR[entry->mRTable.mOffset]);
        if (col < 0) {
            pColor->r = 0;
        } else if (col > 0xFF) {
            pColor->r = 0xFF;
        } else {
            OSf32tou8(&col, &pColor->r);
        }
    }

    switch (entry->mGTable.mMaxFrame) {
    case 0:
        pColor->g = 0;
        break;
    case 1:
        pColor->g = mAnmKRegDataG[entry->mGTable.mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(mFrame, &entry->mGTable, &mAnmKRegDataG[entry->mGTable.mOffset]);
        if (col < 0) {
            pColor->g = 0;
        } else if (col > 0xFF) {
            pColor->g = 0xFF;
        } else {
            OSf32tou8(&col, &pColor->g);
        }
    }

    switch (entry->mBTable.mMaxFrame) {
    case 0:
        pColor->b = 0;
        break;
    case 1:
        pColor->b = mAnmKRegDataB[entry->mBTable.mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(mFrame, &entry->mBTable, &mAnmKRegDataB[entry->mBTable.mOffset]);
        if (col < 0) {
            pColor->b = 0;
        } else if (col > 0xFF) {
            pColor->b = 0xFF;
        } else {
            OSf32tou8(&col, &pColor->b);
        }
    }

    switch (entry->mATable.mMaxFrame) {
    case 0:
        pColor->a = 0;
        break;
    case 1:
        pColor->a = mAnmKRegDataA[entry->mATable.mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(mFrame, &entry->mATable, &mAnmKRegDataA[entry->mATable.mOffset]);
        if (col < 0) {
            pColor->a = 0;
        } else if (col > 0xFF) {
            pColor->a = 0xFF;
        } else {
            OSf32tou8(&col, &pColor->a);
        }
    }
}

void J3DAnmTevRegKey::searchUpdateMaterialID(J3DMaterialTable* pMatTable) {
    for (u16 i = 0; i < mCRegUpdateMaterialNum; i++) {
        s32 index = pMatTable->getMaterialName()->getIndex(mCRegUpdateMaterialName.getName(i));
        if (index != -1) {
            mCRegUpdateMaterialID[i] = index;
        } else {
            mCRegUpdateMaterialID[i] = -1;
        }
    }

    for (u16 i = 0; i < mKRegUpdateMaterialNum; i++) {
        s32 index = pMatTable->getMaterialName()->getIndex(mKRegUpdateMaterialName.getName(i));
        if (index != -1) {
            mKRegUpdateMaterialID[i] = index;
        } else {
            mKRegUpdateMaterialID[i] = -1;
        }
    }
}

void J3DAnmTevRegKey::searchUpdateMaterialID(J3DModelData* pModelData) {
    searchUpdateMaterialID(&pModelData->getMaterialTable());
}
