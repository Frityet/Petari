#include "Game/Effect/AutoEffectInfo.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/StringUtil.hpp"
#include "Inline.hpp"
#include <cstdlib>
#include <cstring>

namespace {
    u32 str2Color(const char*) NO_INLINE;
    bool isValueOnR(const JMapInfoIter&, const char*) NO_INLINE;
    bool isValueOnS(const JMapInfoIter&, const char*) NO_INLINE;
    bool isValueOnT(const JMapInfoIter&, const char*) NO_INLINE;

    struct DrawOrderData {
        s32 mOrder;
        const char* mName;
    };

    const DrawOrderData sDrawOrderDataTable[] = {
        {0, "3D"},
        {1, "PAUSE_IGNORE"},
        {2, "INDIRECT"},
        {3, "AFTER_INDIRECT"},
        {4, "BLOOM_EFFECT"},
        {5, "AFTER_IMAGE_EFFECT"},
        {6, "2D"},
        {7, "2D_PAUSE_IGNORE"},
        {8, "FOR_2D_MODEL"},
    };

    u32 str2Color(const char* pValue) {
        return strtoul(pValue + 1, nullptr, 16) << 8;
    }

    bool isValueOnR(const JMapInfoIter& rIter, const char* pKey) {
        const char* pValue = "";
        rIter.getValue(pKey, &pValue);
        return strchr(pValue, 'R') != nullptr;
    }

    bool isValueOnS(const JMapInfoIter& rIter, const char* pKey) {
        const char* pValue = "";
        rIter.getValue(pKey, &pValue);
        return strchr(pValue, 'S') != nullptr;
    }

    const char* getStringValue(const JMapInfoIter& rIter, const char* pKey) {
        const char* pValue = "";
        rIter.getValue(pKey, &pValue);
        if (MR::isEqualString(pValue, "")) {
            return nullptr;
        }
        return pValue;
    }

    bool isValueOnT(const JMapInfoIter& rIter, const char* pKey) {
        const char* pValue = "";
        rIter.getValue(pKey, &pValue);
        return strchr(pValue, 'T') != nullptr;
    }

    s32 getDrawOrder(const char* pName) {
        for (u32 i = 0; i < sizeof(sDrawOrderDataTable) / sizeof(*sDrawOrderDataTable); i++) {
            if (MR::isEqualString(pName, sDrawOrderDataTable[i].mName)) {
                return sDrawOrderDataTable[i].mOrder;
            }
        }
        return 0;
    }
}  // namespace

AutoEffectInfo::AutoEffectInfo()
    : mGroupName(nullptr), mAnimName(nullptr), mUniqueName(nullptr), mEffectName(nullptr), mParentName(nullptr), mJointName(nullptr),
      mFlag(0), mStartFrame(0), mEndFrame(-1), mScaleValue(1.0f), mRateValue(1.0f), mLightAffectValue(0.0f), mDrawOrder(0) {
    mPrmColor.mColor = 0;
    mIsValidPrmColor = false;
    mEnvColor.mColor = 0;
    mIsValidEnvColor = false;
}

void AutoEffectInfo::init(const JMapInfoIter& rIter) {
    mGroupName = getStringValue(rIter, "GroupName");
    mUniqueName = getStringValue(rIter, "UniqueName");
    mAnimName = getStringValue(rIter, "AnimName");

    const char* pContinueAnimEnd = "";
    rIter.getValue("ContinueAnimEnd", &pContinueAnimEnd);
    if (MR::isEqualString(pContinueAnimEnd, "on")) {
        mFlag |= 0x40;
    } else {
        mFlag &= ~0x40;
    }

    mJointName = getStringValue(rIter, "JointName");
    mEffectName = getStringValue(rIter, "EffectName");
    mParentName = getStringValue(rIter, "ParentName");
    rIter.getValue("OffsetX", &mOffset.x);
    rIter.getValue("OffsetY", &mOffset.y);
    rIter.getValue("OffsetZ", &mOffset.z);
    rIter.getValue("StartFrame", &mStartFrame);
    rIter.getValue("EndFrame", &mEndFrame);

    if (isValueOnT(rIter, "Affect")) {
        mFlag |= 8;
    } else {
        mFlag &= ~8;
    }
    if (isValueOnR(rIter, "Affect")) {
        mFlag |= 0x10;
    } else {
        mFlag &= ~0x10;
    }
    if (isValueOnS(rIter, "Affect")) {
        mFlag |= 0x20;
    } else {
        mFlag &= ~0x20;
    }
    if (isValueOnT(rIter, "Follow")) {
        mFlag |= 1;
    } else {
        mFlag &= ~1;
    }
    if (isValueOnR(rIter, "Follow")) {
        mFlag |= 2;
    } else {
        mFlag &= ~2;
    }
    if (isValueOnS(rIter, "Follow")) {
        mFlag |= 4;
    } else {
        mFlag &= ~4;
    }

    rIter.getValue("ScaleValue", &mScaleValue);
    rIter.getValue("RateValue", &mRateValue);

    const char* pPrmColor = "";
    rIter.getValue("PrmColor", &pPrmColor);
    mIsValidPrmColor = !MR::isEqualString(pPrmColor, "");
    if (mIsValidPrmColor) {
        mPrmColor.set(Color8(str2Color(pPrmColor)));
    }

    const char* pEnvColor = "";
    rIter.getValue("EnvColor", &pEnvColor);
    mIsValidEnvColor = !MR::isEqualString(pEnvColor, "");
    if (mIsValidEnvColor) {
        mEnvColor.set(Color8(str2Color(pEnvColor)));
    }

    rIter.getValue("LightAffectValue", &mLightAffectValue);
    const char* pDrawOrder = "";
    rIter.getValue("DrawOrder", &pDrawOrder);
    mDrawOrder = getDrawOrder(pDrawOrder);
}

const char* AutoEffectInfo::getName() const {
    return mUniqueName ? mUniqueName : mEffectName;
}
