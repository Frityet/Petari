#include "Game/Demo/DemoSoundKeeper.hpp"
#include "Game/Demo/DemoFunction.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "Game/Util/StringUtil.hpp"

template void MR::Vector< MR::AssignableArray< DemoSoundInfo > >::push_back(const DemoSoundInfo&) NO_INLINE;

DemoSoundInfo::DemoSoundInfo() : mPartName(nullptr), mBgm(""), mSystemSe(""), mReturnBgm(0), mBgmWipeoutFrame(-1) {
}

DemoSoundKeeper::DemoSoundKeeper(DemoExecutor* pExecutor) : DemoSheetKeeperBase(pExecutor) {
    JMapInfo* map = nullptr;
    DemoExecutor* executor = mExecutor;
    s32 count = DemoFunction::createSheetParser(executor, getTypeString(), &map);
    mInfo.mArray.init(count);
    for (s32 i = 0; i < count; i++) {
        DemoSoundInfo info;
        map->getValue(i, "PartName", &info.mPartName);
        map->getValue(i, "Bgm", &info.mBgm);
        map->getValue(i, "SystemSe", &info.mSystemSe);
        s32 returnBgm = 0;
        map->getValue(i, "ReturnBgm", &returnBgm);
        info.mReturnBgm = static_cast< u32 >(returnBgm) >> 24;
        map->getValue(i, "BgmWipeoutFrame", &info.mBgmWipeoutFrame);
        mInfo.push_back(info);
    }
}

void DemoSoundKeeper::update() {
    DemoSheetKeeperInfoHolder< DemoSoundInfo >::update();
}

void DemoSoundKeeper::executeType(const DemoSoundInfo* pInfo) {
    if (MR::isDemoPartFirstStep(pInfo->mPartName)) {
        if (pInfo->mBgmWipeoutFrame >= 0 && isPermitBgmChange()) {
            MR::stopStageBGM(pInfo->mBgmWipeoutFrame);
        }
        if (!MR::isNullOrEmptyString(pInfo->mBgm) && isPermitBgmChange()) {
            MR::startStageBGM(pInfo->mBgm, false);
        }
        if (!MR::isNullOrEmptyString(pInfo->mSystemSe)) {
            MR::startSystemSE(pInfo->mSystemSe);
        }
    } else if (MR::isDemoPartLastStep(pInfo->mPartName)) {
        if (pInfo->mReturnBgm && isPermitBgmChange()) {
            MR::startLastStageBGM();
        }
    }
}

bool DemoSoundKeeper::isPermitBgmChange() {
    if (MR::isGalaxyRedCometAppearInCurrentStage() || MR::isGalaxyBlackCometAppearInCurrentStage()) {
        return false;
    }
    return true;
}
