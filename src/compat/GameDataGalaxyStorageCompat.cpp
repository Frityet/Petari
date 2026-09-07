#include "Game/System/GameDataGalaxyStorage.hpp"
#include "Game/System/GameDataConst.hpp"
#include "Game/System/GameEventFlagTable.hpp"
#include "Game/System/GalaxyStatusAccessor.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/StringUtil.hpp"

#include <cstring>

// Original in-memory storage methods from GameDataGalaxyStorage.cpp. The
// native holder owns their lifetime; retail GALA serialization is not enabled.
bool GameDataSomeScenarioAccessor::hasPowerStar() const {
    return mSomeGalaxyStorage->hasPowerStar(mScenarioNum - 1);
}

bool GameDataSomeScenarioAccessor::isAlreadyVisited() const {
    return mSomeGalaxyStorage->isAlreadyVisited(mScenarioNum - 1);
}

s32 GameDataSomeScenarioAccessor::getMaxCoinNum() const {
    return MR::clamp(mSomeGalaxyStorage->getMaxCoinNum(mScenarioNum - 1), 0, 999L);
}

void GameDataSomeScenarioAccessor::setPowerStarFlag(bool val) {
    setBitFlagAccordingToBool(&mSomeGalaxyStorage->mPowerStarOwnedFlags, val);
}

void GameDataSomeScenarioAccessor::setFlagAlreadyVisited(bool val) {
    setBitFlagAccordingToBool(&mSomeGalaxyStorage->mAlreadyVisitedFlags, val);
}

void GameDataSomeScenarioAccessor::updateMaxCoinNum(int coinNum) {
    if (getMaxCoinNum() < coinNum) {
        s32 scenarioNo = mScenarioNum - 1;
        u16 maxCoins = MR::clamp(coinNum, 0, 999L);
        mSomeGalaxyStorage->setMaxCoinNum(scenarioNo, maxCoins);
    }
}

void GameDataSomeScenarioAccessor::setBitFlagAccordingToBool(u8* pFlags, bool val) {
    if (val) {
        *pFlags |= (1 << (mScenarioNum - 1));
    } else {
        *pFlags &= ~(1 << (mScenarioNum - 1));
    }
}

GameDataSomeGalaxyStorage::GameDataSomeGalaxyStorage(const GalaxyStatusAccessor& rAccessor) {
    mGalaxyName = rAccessor.getName();
    mPowerStarNum = rAccessor.getPowerStarNum();
    resetAllData();
}

void GameDataSomeGalaxyStorage::resetAllData() {
    mPowerStarOwnedFlags = 0b00000000;
    mAlreadyVisitedFlags = 0b00000000;
    _A = 0;
    _B = 0;

    for (s32 idx = 0; idx < 8; idx++) {
        mMaxCoinNum[idx] = 0;
    }
}

s32 GameDataSomeGalaxyStorage::getPowerStarNumOwned() const {
    s32 numOwned = 0;
    for (s32 scenarioNo = 1; scenarioNo <= mPowerStarNum; scenarioNo++) {
        if (hasPowerStar(scenarioNo - 1)) {
            numOwned++;
        }
    }
    return numOwned;
}

bool GameDataConst::isGrandStar(const char* pGalaxy, s32 starId) {
    return isPowerStarSpecial(pGalaxy, starId, "SpecialStarGrand");
}

bool GameDataConst::isPowerStarSpecial(const char* pGalaxy, s32 starId, const char* pSpecial) {
    for (GameEventFlagIter iter = GameEventFlagTable::getBeginIter(); !iter.isEnd(); iter.goNext()) {
        GameEventFlagAccessor accessor(iter.getFlag());

        if (accessor.isTypeSpecialStar() && accessor.getStarId() == starId) {
            if (MR::isEqualString(accessor.getGalaxyName(), pGalaxy) && strstr(accessor.getName(), pSpecial) != nullptr) {
                return true;
            }
        }
    }
    return false;
}
