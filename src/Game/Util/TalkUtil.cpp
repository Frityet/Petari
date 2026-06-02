#include "Game/Util/TalkUtil.hpp"
#include "Game/NPC/TalkMessageCtrl.hpp"
#include "Game/NPC/TalkNodeCtrl.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"

extern "C" {
bool startTalk__15TalkMessageCtrlFv(TalkMessageCtrl*);
bool startTalkForce__15TalkMessageCtrlFv(TalkMessageCtrl*);
bool startTalkForcePuppetable__15TalkMessageCtrlFv(TalkMessageCtrl*);
bool startTalkForceWithoutDemo__15TalkMessageCtrlFv(TalkMessageCtrl*);
bool startTalkForceWithoutDemoPuppetable__15TalkMessageCtrlFv(TalkMessageCtrl*);
bool endTalk__15TalkMessageCtrlFv(TalkMessageCtrl*);
void setMessageArg__15TalkMessageCtrlFRC12CustomTagArg(TalkMessageCtrl*, const CustomTagArg&);
}

namespace MR {
    void registerBranchFunc(TalkMessageCtrl* pCtrl, const TalkMessageFuncBase& rFunc) {
        pCtrl->registerBranchFunc(rFunc);
    }

    void registerEventFunc(TalkMessageCtrl* pCtrl, const TalkMessageFuncBase& rFunc) {
        pCtrl->registerEventFunc(rFunc);
    }

    void registerAnimeFunc(TalkMessageCtrl* pCtrl, const TalkMessageFuncBase& rFunc) {
        pCtrl->registerAnimeFunc(rFunc);
    }

    void registerKillFunc(TalkMessageCtrl* pCtrl, const TalkMessageFuncBase& rFunc) {
        pCtrl->registerKillFunc(rFunc);
    }

    void setMessageArg(TalkMessageCtrl* pCtrl, int arg) {
        CustomTagArg tagArg(arg, CustomTagArg::Type_Int);
        setMessageArg__15TalkMessageCtrlFRC12CustomTagArg(pCtrl, tagArg);
    }

    void setMessageArg(TalkMessageCtrl* pCtrl, const wchar_t* pArg) {
        CustomTagArg tagArg(pArg, CustomTagArg::Type_Char);
        setMessageArg__15TalkMessageCtrlFRC12CustomTagArg(pCtrl, tagArg);
    }

    TalkMessageCtrl* createTalkCtrl(LiveActor* pActor, const JMapInfoIter& rIter, const char* pName, const TVec3f& rOffset, MtxPtr pMtx) {
        TalkMessageCtrl* pCtrl = new TalkMessageCtrl(pActor, rOffset, pMtx);
        pCtrl->createMessage(rIter, pName);
        return pCtrl;
    }

    TalkMessageCtrl* createTalkCtrlDirect(LiveActor* pActor, const JMapInfoIter& rIter, const char* pName, const TVec3f& rOffset, MtxPtr pMtx) {
        TalkMessageCtrl* pCtrl = new TalkMessageCtrl(pActor, rOffset, pMtx);
        pCtrl->createMessageDirect(rIter, pName);
        return pCtrl;
    }

    TalkMessageCtrl* createTalkCtrlDirectOnRootNodeAutomatic(LiveActor* pActor, const JMapInfoIter& rIter, const char* pName, const TVec3f& rOffset,
                                                            MtxPtr pMtx) {
        TalkMessageCtrl* pCtrl = new TalkMessageCtrl(pActor, rOffset, pMtx);
        pCtrl->createMessageDirect(rIter, pName);
        pCtrl->mIsOnRootNodeAuto = true;
        return pCtrl;
    }

    bool tryTalkNearPlayer(TalkMessageCtrl* pCtrl) {
        if (MR::isTimeKeepDemoActive()) {
            return false;
        }

        bool result = false;
        if (pCtrl->requestTalk() && startTalk__15TalkMessageCtrlFv(pCtrl)) {
            result = true;
        }

        return result;
    }

    bool tryTalkNearPlayerAtEnd(TalkMessageCtrl* pCtrl) {
        if (MR::isTimeKeepDemoActive()) {
            return false;
        }

        if (endTalk__15TalkMessageCtrlFv(pCtrl)) {
            pCtrl->requestTalk();
            return true;
        }

        if (pCtrl->requestTalk()) {
            startTalk__15TalkMessageCtrlFv(pCtrl);
        }

        return false;
    }

    bool tryTalkForce(TalkMessageCtrl* pCtrl) {
        if (MR::isTimeKeepDemoActive()) {
            return false;
        }

        bool result = false;
        if (pCtrl->requestTalkForce() && startTalkForce__15TalkMessageCtrlFv(pCtrl)) {
            result = true;
        }

        return result;
    }

    bool tryTalkForceAtEnd(TalkMessageCtrl* pCtrl) {
        if (MR::isTimeKeepDemoActive()) {
            return false;
        }

        if (endTalk__15TalkMessageCtrlFv(pCtrl)) {
            return true;
        }

        if (pCtrl->requestTalkForce()) {
            startTalkForce__15TalkMessageCtrlFv(pCtrl);
        }

        return false;
    }

    bool tryTalkForceWithoutDemo(TalkMessageCtrl* pCtrl) {
        if (MR::isTimeKeepDemoActive()) {
            return false;
        }

        bool result = false;
        if (pCtrl->requestTalkForce() && startTalkForceWithoutDemo__15TalkMessageCtrlFv(pCtrl)) {
            result = true;
        }

        return result;
    }

    bool tryTalkForceWithoutDemoMarioPuppetable(TalkMessageCtrl* pCtrl) {
        if (MR::isTimeKeepDemoActive()) {
            return false;
        }

        bool result = false;
        if (pCtrl->requestTalkForce() && startTalkForceWithoutDemoPuppetable__15TalkMessageCtrlFv(pCtrl)) {
            result = true;
        }

        return result;
    }

    bool tryTalkForceWithoutDemoAtEnd(TalkMessageCtrl* pCtrl) {
        if (MR::isTimeKeepDemoActive()) {
            return false;
        }

        if (endTalk__15TalkMessageCtrlFv(pCtrl)) {
            return true;
        }

        if (pCtrl->requestTalkForce()) {
            startTalkForceWithoutDemo__15TalkMessageCtrlFv(pCtrl);
        }

        return false;
    }

    bool tryTalkForceWithoutDemoMarioPuppetableAtEnd(TalkMessageCtrl* pCtrl) {
        if (MR::isTimeKeepDemoActive()) {
            return false;
        }

        if (endTalk__15TalkMessageCtrlFv(pCtrl)) {
            return true;
        }

        if (pCtrl->requestTalkForce()) {
            startTalkForceWithoutDemoPuppetable__15TalkMessageCtrlFv(pCtrl);
        }

        return false;
    }

    bool tryTalkTimeKeepDemo(TalkMessageCtrl* pCtrl) {
        bool result = false;
        if (pCtrl->requestTalkForce() && startTalkForce__15TalkMessageCtrlFv(pCtrl)) {
            result = true;
        }

        return result;
    }

    bool tryTalkTimeKeepDemoMarioPuppetable(TalkMessageCtrl* pCtrl) {
        bool result = false;
        if (pCtrl->requestTalkForce() && startTalkForcePuppetable__15TalkMessageCtrlFv(pCtrl)) {
            result = true;
        }

        return result;
    }

    bool tryTalkTimeKeepDemoWithoutPauseMarioPuppetable(TalkMessageCtrl* pCtrl) {
        bool result = false;
        if (pCtrl->requestTalkForce() && startTalkForceWithoutDemoPuppetable__15TalkMessageCtrlFv(pCtrl)) {
            result = true;
        }

        return result;
    }

    bool tryTalkRequest(TalkMessageCtrl* pCtrl) {
        return pCtrl->requestTalk();
    }

    bool tryTalkSelectLeft(TalkMessageCtrl*) {
        bool result = false;
        if (MR::isYesNoSelected() && MR::isYesNoSelectedYes()) {
            result = true;
        }

        return result;
    }

    bool tryTalkSelectRight(TalkMessageCtrl*) {
        bool result = false;
        if (MR::isYesNoSelected() && !MR::isYesNoSelectedYes()) {
            result = true;
        }

        return result;
    }

    const MtxPtr getMessageBalloonFollowMatrix(const TalkMessageCtrl* pCtrl) {
        return pCtrl->mMtx;
    }

    const TVec3f& getMessageBalloonFollowOffset(const TalkMessageCtrl* pCtrl) {
        return pCtrl->_2C;
    }

    void setMessageBalloonFollowOffset(TalkMessageCtrl* pCtrl, const TVec3f& rOffset) {
        pCtrl->_2C = rOffset;
    }

    bool isNearPlayer(const TalkMessageCtrl* pCtrl, f32 distance) {
        return pCtrl->isNearPlayer(distance);
    }

    bool inMessageArea(const TalkMessageCtrl* pCtrl) {
        return pCtrl->inMessageArea();
    }

    bool isTalkNone(const TalkMessageCtrl* pCtrl) {
        return pCtrl->_18 == 0;
    }

    bool isTalkEntry(const TalkMessageCtrl* pCtrl) {
        return pCtrl->_18 == 1;
    }

    bool isTalkTalking(const TalkMessageCtrl* pCtrl) {
        return pCtrl->_18 == 3;
    }

    bool isTalkEnableEnd(const TalkMessageCtrl* pCtrl) {
        return pCtrl->_18 == 4;
    }

    void clearTalkState(TalkMessageCtrl* pCtrl) {
        TalkFunction::onTalkStateNone(pCtrl);
    }

    void resetNode(TalkMessageCtrl* pCtrl) {
        TalkFunction::onTalkStateNone(pCtrl);
        pCtrl->mNodeCtrl->resetFlowNode();
    }

    void readMessage(TalkMessageCtrl* pCtrl) {
        pCtrl->readMessage();
    }

    void forwardNode(TalkMessageCtrl* pCtrl) {
        TalkFunction::onTalkStateNone(pCtrl);
        pCtrl->mNodeCtrl->forwardFlowNode();
        pCtrl->mNodeCtrl->recordTempFlowNode();
    }

    void resetAndForwardNode(TalkMessageCtrl* pCtrl, s32 count) {
        TalkFunction::onTalkStateNone(pCtrl);
        pCtrl->mNodeCtrl->resetFlowNode();

        for (s32 i = 0; i < count; i++) {
            TalkFunction::onTalkStateNone(pCtrl);
            pCtrl->mNodeCtrl->forwardFlowNode();
            pCtrl->mNodeCtrl->recordTempFlowNode();
        }
    }

    void forwardNodeNextBranchLeft(TalkMessageCtrl* pCtrl) {
        TalkFunction::onTalkStateNone(pCtrl);
        pCtrl->mNodeCtrl->forwardFlowNode();
        pCtrl->mNodeCtrl->forwardCurrentBranchNode(true);
        pCtrl->mNodeCtrl->recordTempFlowNode();
    }

    void forwardNodeNextBranchRight(TalkMessageCtrl* pCtrl) {
        TalkFunction::onTalkStateNone(pCtrl);
        pCtrl->mNodeCtrl->forwardFlowNode();
        pCtrl->mNodeCtrl->forwardCurrentBranchNode(false);
        pCtrl->mNodeCtrl->recordTempFlowNode();
    }

    void forwardNodeCurrentBranchLeft(TalkMessageCtrl* pCtrl) {
        TalkFunction::onTalkStateNone(pCtrl);
        pCtrl->mNodeCtrl->forwardCurrentBranchNode(true);
        pCtrl->mNodeCtrl->recordTempFlowNode();
    }

    void forwardNodeCurrentBranchRight(TalkMessageCtrl* pCtrl) {
        TalkFunction::onTalkStateNone(pCtrl);
        pCtrl->mNodeCtrl->forwardCurrentBranchNode(false);
        pCtrl->mNodeCtrl->recordTempFlowNode();
    }

    void tryForwardNode(TalkMessageCtrl* pCtrl) {
        if (pCtrl->mNodeCtrl->isExistNextNode()) {
            TalkFunction::onTalkStateNone(pCtrl);
            TalkFunction::onTalkStateNone(pCtrl);
            pCtrl->mNodeCtrl->forwardFlowNode();
            pCtrl->mNodeCtrl->recordTempFlowNode();
        }
    }

    bool isExistNextNode(const TalkMessageCtrl* pCtrl) {
        return pCtrl->mNodeCtrl->isExistNextNode();
    }

    bool isShortTalk(const TalkMessageCtrl* pCtrl) {
        return TalkFunction::isShortTalk(pCtrl);
    }

    void setDistanceToTalk(TalkMessageCtrl* pCtrl, f32 distance) {
        pCtrl->mTalkDistance = distance;
    }

    void onRootNodeAutomatic(TalkMessageCtrl* pCtrl) {
        pCtrl->mIsOnRootNodeAuto = true;
    }

    void offRootNodeAutomatic(TalkMessageCtrl* pCtrl) {
        pCtrl->mIsOnRootNodeAuto = false;
    }

    void onReadNodeAutomatic(TalkMessageCtrl* pCtrl) {
        pCtrl->mIsOnReadNodeAuto = true;
    }

    void offReadNodeAutomatic(TalkMessageCtrl* pCtrl) {
        pCtrl->mIsOnReadNodeAuto = false;
    }

    void onStartOnlyFront(TalkMessageCtrl* pCtrl) {
        pCtrl->mIsStartOnlyFront = true;
    }

    bool isTalkStart(const TalkMessageCtrl* pCtrl) {
        return TalkFunction::isTalkSystemStart(pCtrl);
    }

    bool isTalkEnd(const TalkMessageCtrl* pCtrl) {
        return TalkFunction::isTalkSystemEnd(pCtrl);
    }
};  // namespace MR

extern "C" void setMessageArg__15TalkMessageCtrlFRC12CustomTagArg(TalkMessageCtrl* pCtrl, const CustomTagArg& rArg) {
    pCtrl->mTagArg = rArg;
}
