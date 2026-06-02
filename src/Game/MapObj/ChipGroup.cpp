#include "Game/MapObj/ChipGroup.hpp"
#include "Game/MapObj/ChipBase.hpp"
#include "Game/MapObj/ChipHolder.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/SoundUtil.hpp"

#define CHIP_GROUP_MAX_CHIPS 5
#define CHIP_GROUP_UI_MARGIN 2000.0f
#define CHIP_GROUP_COUNTER_NONE -1

ChipGroup::ChipGroup(const char* pName, s32 chipType) : NameObj(pName) {
    ChipEntry* pEntry = mChips;
    ChipEntry* pEnd = mChips + CHIP_GROUP_MAX_CHIPS;

    while (pEntry < pEnd) {
        pEntry->mChip = nullptr;
        pEntry->mIsGotten = false;
        pEntry++;
    }

    mSwitchCtrl = nullptr;
    mGotCount = 0;
    mTotalCount = 0;
    _4C = CHIP_GROUP_COUNTER_NONE;
    _50 = chipType;
    _54 = CHIP_GROUP_COUNTER_NONE;
    _58 = CHIP_GROUP_COUNTER_NONE;
    _5C = -1.0f;
    _60 = -1.0f;
    _64 = 0;
    _68 = CHIP_GROUP_COUNTER_NONE;
    _6C = false;
    _6D = false;
}

void ChipGroup::updateUIRange() {
    if (_60 < 0.0f) {
        TVec3f min;
        TVec3f max;

        for (s32 i = 0; i < mTotalCount; i++) {
            TVec3f pos(mChips[i].mChip->mPosition);
            TVec3f base(pos);
            TVec3f range(CHIP_GROUP_UI_MARGIN, CHIP_GROUP_UI_MARGIN, CHIP_GROUP_UI_MARGIN);
            TVec3f maxPos(base);
            TVec3f minPos(base);
            TVec3f chipMin;
            TVec3f chipMax;

            maxPos.addInline(range);
            JMathInlineVEC::PSVECSubtract(&minPos, &range, &minPos);
            chipMin.set< f32 >(minPos);
            chipMax.set< f32 >(maxPos);

            if (i == 0) {
                min.set< f32 >(chipMin);
                max.set< f32 >(chipMax);
            } else {
                if (min.x >= chipMin.x) {
                    min.x = chipMin.x;
                }

                if (min.y >= chipMin.y) {
                    min.y = chipMin.y;
                }

                if (min.z >= chipMin.z) {
                    min.z = chipMin.z;
                }

                if (max.x <= chipMax.x) {
                    max.x = chipMax.x;
                }

                if (max.y <= chipMax.y) {
                    max.y = chipMax.y;
                }

                if (max.z <= chipMax.z) {
                    max.z = chipMax.z;
                }
            }
        }

        JMAVECLerp(&max, &min, &_38, 0.5f);

        TVec3f diameter;
        JMathInlineVEC::PSVECSubtract(&max, &min, &diameter);
        _5C = PSVECMag(&diameter) * 0.5f;
    } else {
        _5C = _60;
    }
}

void ChipGroup::movement() {
    if (_64 != 0) {
        return;
    }

    if (_6D) {
        if (PSVECDistance(&_38, MR::getPlayerPos()) < _5C) {
            MR::showChipCounter(_50, _4C);
        } else {
            MR::hideChipCounter(_50, _4C);
        }
    }

    if (_58 >= 0) {
        _58--;

        if (_58 < 0) {
            MR::hideChipCounter(_50, _4C);
            _58 = CHIP_GROUP_COUNTER_NONE;
            _6D = false;
        }
    }
}

void ChipGroup::registerChip(ChipBase* pChip) {
    if (_6C) {
        pChip->makeActorDead();
    }

    if (_64) {
        pChip->deactive();
    }

    mChips[mTotalCount].mChip = pChip;
    mTotalCount++;
    updateUIRange();
}

void ChipGroup::noticeEndCompleteDemo() {
    mSwitchCtrl->onSwitchA();
}

s32 ChipGroup::getGotCount() const {
    return mGotCount;
}

bool ChipGroup::isComplete() const {
    s32 count = mTotalCount;
    for (s32 i = 0; i < count; i++) {
        if (!mChips[i].mIsGotten) {
            return false;
        }
    }

    return true;
}

BlueChipGroup::BlueChipGroup(const char* pName) : ChipGroup(pName, 0) {}

ChipGroup::~ChipGroup() {}

YellowChipGroup::YellowChipGroup(const char* pName) : ChipGroup(pName, 1) {}

void ChipGroup::init(const JMapInfoIter& rIter) {
    MR::getJMapInfoTrans(rIter, &_38);
    MR::getJMapInfoArg0WithInit(rIter, &_4C);
    MR::getJMapInfoArg1WithInit(rIter, &_54);
    _60 = -1.0f;
    MR::getJMapInfoArg2NoInit(rIter, &_60);

    s32 useAlreadyDoneFlag = 0;
    MR::getJMapInfoArg3NoInit(rIter, &useAlreadyDoneFlag);

    if (useAlreadyDoneFlag == 1) {
        _64 = 0;
        _68 = MR::setupAlreadyDoneFlag("チップ集め済み", rIter, &_64);
    } else {
        _64 = 0;
        _68 = CHIP_GROUP_COUNTER_NONE;
    }

    MR::createChipHolder(_50);
    MR::registerChipGroup(_50, this);

    mSwitchCtrl = MR::createStageSwitchCtrl(this, rIter);

    if (mSwitchCtrl->isValidSwitchAppear()) {
        MR::FunctorV0M< ChipGroup*, void (ChipGroup::*)() > functor(this, &ChipGroup::receiveAppearRequest);
        MR::listenNameObjStageSwitchOnAppear(this, mSwitchCtrl, functor);
        _6C = true;
    } else {
        _6C = false;
    }

    MR::connectToSceneMapObjMovement(this);
}

void ChipGroup::noticeGet(ChipBase* pChip) {
    if (_64 != 0) {
        return;
    }

    MR::showChipCounter(_50, _4C);

    if (!_6C || _54 == CHIP_GROUP_COUNTER_NONE) {
        _6D = true;
    }

    for (s32 i = 0; i < mTotalCount; i++) {
        bool isRequestedChip = false;

        if (pChip != nullptr && mChips[i].mChip == pChip) {
            isRequestedChip = true;
        }

        if (isRequestedChip) {
            mChips[i].mIsGotten = true;
            mGotCount++;

            if (isComplete()) {
                MR::requestStartChipCompleteDemo(_50, _4C);
                MR::requestMovementOn(pChip);

                if (_68 >= 0) {
                    MR::updateAlreadyDoneFlag(_68, 1);
                }

                if (_50 == 0) {
                    MR::startSystemSE("SE_OJ_BLUECHIP_COMPLETE", mGotCount, -1);
                } else if (_50 == 1) {
                    MR::startSystemSE("SE_OJ_YELLOWCHIP_COMPLETE", mGotCount, -1);
                }

                _58 = CHIP_GROUP_COUNTER_NONE;
                _6D = false;
            }

            return;
        }
    }
}

void ChipGroup::receiveAppearRequest() {
    if (_64 != 0) {
        return;
    }

    for (s32 i = 0; i < mTotalCount; i++) {
        if (_54 >= 0) {
            mChips[i].mChip->appearFlashing(_54);
        } else {
            mChips[i].mChip->appearWait();
        }
    }

    MR::showChipCounter(_50, _4C);

    if (_54 >= 0) {
        _58 = _54;
        _6D = false;
    } else {
        _58 = CHIP_GROUP_COUNTER_NONE;
        _6D = true;
    }

    if (_50 == 0) {
        MR::startSystemSE("SE_OJ_BLUECHIP_APPEAR", -1, -1);
    } else if (_50 == 1) {
        MR::startSystemSE("SE_OJ_YELLOWCHIP_APPEAR", -1, -1);
    }
}

BlueChipGroup::~BlueChipGroup() {}

YellowChipGroup::~YellowChipGroup() {}
