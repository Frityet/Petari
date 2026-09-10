#include "Game/Demo/DemoWipeKeeper.hpp"
#include "Game/Demo/DemoFunction.hpp"
#include "Game/Screen/SceneWipeHolder.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/ObjUtil.hpp"

template void MR::Vector< MR::AssignableArray< DemoWipeInfo > >::push_back(const DemoWipeInfo&) NO_INLINE;

DemoWipeInfo::DemoWipeInfo() : mPartName(nullptr), mWipeName("フェードワイプ"), mWipeType(0), mWipeFrame(-1) {
}

DemoWipeKeeper::DemoWipeKeeper(DemoExecutor* pExecutor) : DemoSheetKeeperBase(pExecutor) {
    JMapInfo* map = nullptr;
    DemoExecutor* executor = mExecutor;
    s32 count = DemoFunction::createSheetParser(executor, getTypeString(), &map);
    mInfo.mArray.init(count);
    for (s32 i = 0; i < count; i++) {
        DemoWipeInfo info;
        MR::getCsvDataStrOrNULL(&info.mPartName, map, "PartName", i);
        MR::getCsvDataStrOrNULL(&info.mWipeName, map, "WipeName", i);
        MR::getCsvDataS32(&info.mWipeType, map, "WipeType", i);
        MR::getCsvDataS32(&info.mWipeFrame, map, "WipeFrame", i);
        mInfo.push_back(info);
    }
}

void DemoWipeKeeper::start() {
}

void DemoWipeKeeper::update() {
    DemoSheetKeeperInfoHolder< DemoWipeInfo >::update();
}

void DemoWipeKeeper::executeType(const DemoWipeInfo* pInfo) {
    if (MR::isDemoPartFirstStep(pInfo->mPartName)) {
        if (pInfo->mWipeType == 0) {
            SceneWipeHolderFunction::openWipe(pInfo->mWipeName, pInfo->mWipeFrame);
        } else if (pInfo->mWipeType == 1) {
            SceneWipeHolderFunction::closeWipe(pInfo->mWipeName, pInfo->mWipeFrame);
        } else if (pInfo->mWipeType == 2) {
            SceneWipeHolderFunction::forceOpenWipe(pInfo->mWipeName);
        } else if (pInfo->mWipeType == 3) {
            SceneWipeHolderFunction::forceCloseWipe(pInfo->mWipeName);
        }
    }
}
