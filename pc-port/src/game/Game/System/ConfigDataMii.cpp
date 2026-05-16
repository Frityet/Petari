#include "Game/System/ConfigDataMii.hpp"

#include "Game/Util/MemoryUtil.hpp"

namespace {
constexpr u8 FLAG_HAS_MII = 0x2U;
constexpr u8 ICON_ID_MII = 0U;
constexpr u8 ICON_ID_MARIO = 1U;
constexpr u32 RFL_CREATE_ID_SIZE = 8U;
}  // namespace

ConfigDataMii::ConfigDataMii()
    : mFlag(0U), mIconId(ICON_ID_MARIO), mMiiId(new u8[RFL_CREATE_ID_SIZE]) {
    initializeData();
}

ConfigDataMii::~ConfigDataMii() {
    delete[] mMiiId;
}

u32 ConfigDataMii::makeHeaderHashCode() const {
    return 0x2836E9U;
}

u32 ConfigDataMii::getSignature() const {
    return 0x4D494920U;
}

s32 ConfigDataMii::serialize(u8 *pBuffer, u32 size) const {
    if (pBuffer == nullptr || size < 1U + RFL_CREATE_ID_SIZE + 1U) {
        return 0;
    }

    pBuffer[0] = mFlag;
    MR::copyMemory(pBuffer + 1U, mMiiId, RFL_CREATE_ID_SIZE);
    pBuffer[1U + RFL_CREATE_ID_SIZE] = mIconId;
    return static_cast<s32>(1U + RFL_CREATE_ID_SIZE + 1U);
}

s32 ConfigDataMii::deserialize(const u8 *pBuffer, u32 size) {
    initializeData();
    if (pBuffer == nullptr || size < 1U + RFL_CREATE_ID_SIZE) {
        return 1;
    }

    mFlag = pBuffer[0];
    MR::copyMemory(mMiiId, pBuffer + 1U, RFL_CREATE_ID_SIZE);
    if (size > 1U + RFL_CREATE_ID_SIZE) {
        mIconId = pBuffer[1U + RFL_CREATE_ID_SIZE];
    } else if ((mFlag & 0x1U) != 0U) {
        mIconId = ICON_ID_MII;
    }
    return 0;
}

void ConfigDataMii::initializeData() {
    mFlag = 0U;
    mIconId = ICON_ID_MARIO;
    MR::zeroMemory(mMiiId, RFL_CREATE_ID_SIZE);
}

void ConfigDataMii::setMiiOrIconId(const void *pMiiId, const u32 *pIconId) {
    if (pMiiId != nullptr) {
        MR::copyMemory(mMiiId, pMiiId, RFL_CREATE_ID_SIZE);
        mFlag |= FLAG_HAS_MII;
        mIconId = ICON_ID_MII;
        return;
    }

    MR::zeroMemory(mMiiId, RFL_CREATE_ID_SIZE);
    if (pIconId != nullptr) {
        mIconId = static_cast<u8>(*pIconId);
    }
}

bool ConfigDataMii::getIconId(u32 *pIconId) const {
    if (pIconId != nullptr) {
        *pIconId = mIconId;
    }
    return mIconId != ICON_ID_MII;
}

bool ConfigDataMii::getMiiId(void *pMiiId) const {
    if (pMiiId != nullptr) {
        MR::copyMemory(pMiiId, mMiiId, RFL_CREATE_ID_SIZE);
    }
    return mIconId == ICON_ID_MII;
}
