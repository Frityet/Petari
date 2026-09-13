#include "Game/LiveActor/ViewGroupCtrl.hpp"
#include "Game/AreaObj/AreaObj.hpp"
#include "Game/LiveActor/ClippingActorInfo.hpp"
#include "Game/LiveActor/LodCtrl.hpp"
#include "Game/Util/Array.hpp"
#include "Game/Util/AreaObjUtil.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"

ViewGroupCtrl::ViewGroupCtrl() {
    mViewCubeMgr = nullptr;
    mMaxViewGroupId = 0;
    mViewGroupData = nullptr;
    mViewCtrlCount = 0;
    mLodCtrls = nullptr;
    mLodCtrls = new LodCtrl*[0x100];

    for (u32 i = 0; i < 0x100; i++) {
        mLodCtrls[i] = nullptr;
    }
}

void ViewGroupCtrl::initActorInfo(ClippingActorInfo* pInfo, s32 groupID) {
    pInfo->_12 = groupID;

    if (groupID >= mMaxViewGroupId) {
        mMaxViewGroupId = groupID + 1;
    }
}

void ViewGroupCtrl::initViewGroup(ClippingActorInfoList* pList) {
    for (s32 i = 0; i < pList->_4; i++) {
        s32 id = pList->mClippingActorList[i]->_12;

        if (id < 0) {
            id = mMaxViewGroupId;
        }

        pList->mClippingActorList[i]->_14 = &mViewGroupData[id];
    }
}

void ViewGroupCtrl::endInitViewGroupTable() {
}

void ViewGroupCtrl::entryLodCtrl(LodCtrl* pCtrl, const JMapInfoIter& rIter) {
    s32 groupID = -1;
    if (MR::getJMapInfoViewGroupID(rIter, &groupID)) {
        pCtrl->mViewGroupID = groupID;
        mLodCtrls[mViewCtrlCount] = pCtrl;
        mViewCtrlCount++;
    }
}

void ViewGroupCtrl::update() {
    if (mViewCubeMgr == nullptr) {
        return;
    }

    for (s32 i = 0; i < mMaxViewGroupId; i++) {
        ViewGroupCtrlDataEntry* pEntry = &mViewGroupData[i];
        pEntry->_0 = false;
        pEntry->_1 = false;
        pEntry->_2 = false;
        pEntry->_3 = false;
        pEntry->_4 = false;
    }

    for (s32 i = 0; i < mViewCubeMgr->getNumAreaObj(); i++) {
        AreaObj* pArea = mViewCubeMgr->getAreaObj(i);
        if (pArea->isInVolume(*MR::getPlayerPos())) {
            ViewGroupCtrlDataEntry* pEntry = &mViewGroupData[pArea->mObjArg0];
            pEntry->_0 = true;
            if (pArea->mObjArg1 == 1) {
                pEntry->_1 = true;
            } else if (pArea->mObjArg1 == 2) {
                pEntry->_2 = true;
            } else if (pArea->mObjArg1 == 3) {
                pEntry->_3 = true;
            } else if (pArea->mObjArg1 == 4) {
                pEntry->_0 = false;
                pEntry->_4 = true;
            }
        }
    }
}

void ViewGroupCtrl::startInitViewGroupTable() {
    mViewCubeMgr = MR::getAreaObjManager("ViewGroupCtrlCube");
    if (mViewCubeMgr != nullptr) {
        for (s32 i = 0; i < mViewCubeMgr->getNumAreaObj(); i++) {
            s32 id = mViewCubeMgr->getAreaObj(i)->mObjArg0;
            if (id >= mMaxViewGroupId) {
                mMaxViewGroupId = id + 1;
            }
        }
    }

    mViewGroupData = new ViewGroupCtrlDataEntry[mMaxViewGroupId + 1];
    for (s32 i = 0; i < mMaxViewGroupId + 1; i++) {
        ViewGroupCtrlDataEntry* pEntry = &mViewGroupData[i];
        pEntry->_0 = false;
        pEntry->_1 = false;
        pEntry->_2 = false;
        pEntry->_3 = false;
        pEntry->_4 = false;
    }

    for (s32 i = 0; i < mViewCtrlCount; i++) {
        LodCtrl* pCtrl = mLodCtrls[i];
        s32 id = pCtrl->mViewGroupID;
        if (id < 0) {
            id = mMaxViewGroupId;
        }
        ViewGroupCtrlDataEntry* pEntry = &mViewGroupData[id];
        pCtrl->setViewCtrlPtr(&pEntry->_1, &pEntry->_2, &pEntry->_3, &pEntry->_4);
    }
}
