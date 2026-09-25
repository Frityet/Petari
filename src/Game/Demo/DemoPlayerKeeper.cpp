#include "Game/Demo/DemoPlayerKeeper.hpp"
#include "Game/Demo/DemoFunction.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"

DemoPlayerKeeper::DemoPlayerKeeper(const DemoExecutor* pExecutor) : mExecutor(pExecutor), mNumPlayerInfos(0), mPlayerInfos(nullptr) {
    try {
        JMapInfo* map = nullptr;
        mNumPlayerInfos = DemoFunction::createSheetParser(mExecutor, "Player", &map);
        mNativeParser.reset(map);
        mPlayerInfos = new DemoPlayerInfo[mNumPlayerInfos];
        for (int i = 0; i < mNumPlayerInfos; i++) {
            DemoPlayerInfo* playerinfo = &mPlayerInfos[i];
            MR::getCsvDataStrOrNULL(&playerinfo->mPartName, map, "PartName", i);
            MR::getCsvDataStrOrNULL(&playerinfo->mPosName, map, "PosName", i);
            MR::getCsvDataStrOrNULL(&playerinfo->mBckName, map, "BckName", i);
        }
    } catch (...) {
        delete[] mPlayerInfos;
        throw;
    }
}

DemoPlayerInfo::DemoPlayerInfo() : mPosName(nullptr), mPartName(nullptr), mBckName(nullptr) {
}

void DemoPlayerKeeper::update() {
    const DemoPlayerInfo* playerinfo;
    for (int i = 0; i < mNumPlayerInfos; i++) {
        playerinfo = &mPlayerInfos[i];
        if (MR::isDemoPartFirstStep(playerinfo->mPartName)) {
            executePlayer(playerinfo);
        }
    }
}

bool DemoPlayerKeeper::isExistPosName() const {
    for (int i = 0; i < mNumPlayerInfos; i++) {
        if (mPlayerInfos[i].mPosName != nullptr) {
            return true;
        }
    }
    return false;
}

void DemoPlayerKeeper::executePlayer(const DemoPlayerInfo* pPlayerInfo) const {
    if (pPlayerInfo->mPosName != nullptr) {
        MR::setPlayerPosOnGroundAndWait(pPlayerInfo->mPosName);
    }
    if (pPlayerInfo->mBckName != nullptr) {
        MR::startBckPlayer(pPlayerInfo->mBckName, (const char*)nullptr);
    }
}

DemoPlayerKeeper::~DemoPlayerKeeper() {
    delete[] mPlayerInfos;
}
