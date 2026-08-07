#include "Game/NPC/TalkMessageCtrl.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NPC/NPCActor.hpp"
#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/MessageUtil.hpp"
#include "Game/Util/TalkUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "runtime/RuntimeContext.hpp"

#include <cstdio>
#include <memory>
#include <string>
#include <unordered_map>

namespace {
    struct TalkRuntimeState {
        s32 message_id = -1;
        s32 node_index = 0;
    };

    std::unordered_map<const TalkMessageCtrl *, TalkRuntimeState> sTalkStates;
    std::unordered_map<const LiveActor *, std::unique_ptr<TalkMessageCtrl>> sOwnedTalkCtrls;

    void trace_talk(const TalkMessageCtrl *pCtrl, const char *event) {
#ifndef NDEBUG
        if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr && pCtrl != nullptr) {
            const auto *host = pCtrl->mHostActor;
            runtime->emit_semantic_trace_event("talk", event,
                                               "host=" + std::string(host != nullptr && host->getName() != nullptr ? host->getName() : "") +
                                                   ";message_id=" + std::to_string(sTalkStates[pCtrl].message_id) +
                                                   ";node=" + std::to_string(sTalkStates[pCtrl].node_index));
        }
#else
        static_cast<void>(pCtrl);
        static_cast<void>(event);
#endif
    }
}  // namespace

namespace smgpc::compat {
    TalkMessageCtrl *owned_talk_ctrl(const LiveActor *actor) {
        const auto found = sOwnedTalkCtrls.find(actor);
        return found != sOwnedTalkCtrls.end() ? found->second.get() : nullptr;
    }

    void release_talk_runtime_state(const LiveActor *actor) {
        const auto found = sOwnedTalkCtrls.find(actor);
        if (found == sOwnedTalkCtrls.end()) {
            return;
        }
        if (auto *npc = dynamic_cast<NPCActor *>(const_cast<LiveActor *>(actor)); npc != nullptr && npc->mMsgCtrl == found->second.get()) {
            npc->mMsgCtrl = nullptr;
        }
        sOwnedTalkCtrls.erase(found);
    }

    bool has_owned_talk_ctrl(const LiveActor *actor) {
        return sOwnedTalkCtrls.contains(actor);
    }
}  // namespace smgpc::compat

TalkMessageCtrl::TalkMessageCtrl(LiveActor *pHost, const TVec3f &rOffset, MtxPtr pMtx)
    : NameObj("TalkMessageCtrl"), mHostActor(pHost), mNodeCtrl(nullptr), mZoneID(-1), _18(0), _1C(), mMtx(pMtx), _2C(rOffset),
      mTalkDistance(500.0F), _3C(0), mAlreadyDoneFlags(0), mIsOnRootNodeAuto(false), mIsOnReadNodeAuto(false), mIsStartOnlyFront(false),
      mCameraInfo(nullptr), mBranchFunc(nullptr), mEventFunc(nullptr), mAnimeFunc(nullptr), mKillFunc(nullptr),
      mTagArg(0, CustomTagArg::Type_Uninitialized) {
    sTalkStates.emplace(this, TalkRuntimeState{});
}

TalkMessageCtrl::~TalkMessageCtrl() {
    sTalkStates.erase(this);
}

void TalkMessageCtrl::createMessage(const JMapInfoIter &rIter, const char *) {
    auto message_id = s32{-1};
    (void)MR::getJMapInfoMessageID(rIter, &message_id);
    sTalkStates[this].message_id = message_id;
}

void TalkMessageCtrl::createMessageDirect(const JMapInfoIter &rIter, const char *pName) {
    createMessage(rIter, pName);
}

u32 TalkMessageCtrl::getMessageID() const {
    const auto found = sTalkStates.find(this);
    return found != sTalkStates.end() && found->second.message_id >= 0 ? static_cast<u32>(found->second.message_id) : 0U;
}

bool TalkMessageCtrl::requestTalk() {
    return isNearPlayer(mTalkDistance);
}

bool TalkMessageCtrl::requestTalkForce() {
    return mHostActor != nullptr;
}

void TalkMessageCtrl::startTalk() {
    _18 = 3;
    trace_talk(this, "started");
}

void TalkMessageCtrl::startTalkForce() {
    startTalk();
}

void TalkMessageCtrl::startTalkForcePuppetable() {
    startTalk();
}

void TalkMessageCtrl::startTalkForceWithoutDemo() {
    startTalk();
}

void TalkMessageCtrl::startTalkForceWithoutDemoPuppetable() {
    startTalk();
}

void TalkMessageCtrl::endTalk() {
    _18 = 4;
    trace_talk(this, "ended");
}

bool TalkMessageCtrl::isNearPlayer(const TalkMessageCtrl *) {
    return isNearPlayer(mTalkDistance);
}

bool TalkMessageCtrl::isNearPlayer(f32 distance) const {
    return mHostActor != nullptr && MR::isNearPlayer(mHostActor, distance);
}

bool TalkMessageCtrl::inMessageArea() const {
    return true;
}

namespace MR {
    const wchar_t *getGalaxyNameOnCurrentLanguage(const char *pGalaxyName) {
        char message_id[256]{};
        std::snprintf(message_id, sizeof(message_id), "GalaxyName_%s", pGalaxyName != nullptr ? pGalaxyName : "");
        return getGameMessageDirect(message_id);
    }

    TalkMessageCtrl *createTalkCtrl(LiveActor *pActor, const JMapInfoIter &rIter, const char *pName, const TVec3f &rOffset, MtxPtr pMtx) {
        auto owned_ctrl = std::make_unique<TalkMessageCtrl>(pActor, rOffset, pMtx);
        auto *ctrl = owned_ctrl.get();
        ctrl->createMessage(rIter, pName);
        sOwnedTalkCtrls.insert_or_assign(pActor, std::move(owned_ctrl));
        return ctrl;
    }

    TalkMessageCtrl *createTalkCtrlDirect(LiveActor *pActor, const JMapInfoIter &rIter, const char *pName, const TVec3f &rOffset,
                                          MtxPtr pMtx) {
        auto owned_ctrl = std::make_unique<TalkMessageCtrl>(pActor, rOffset, pMtx);
        auto *ctrl = owned_ctrl.get();
        ctrl->createMessageDirect(rIter, pName);
        sOwnedTalkCtrls.insert_or_assign(pActor, std::move(owned_ctrl));
        return ctrl;
    }

    bool tryTalkNearPlayer(TalkMessageCtrl *pCtrl) {
        if (pCtrl == nullptr || !pCtrl->requestTalk()) {
            return false;
        }
        pCtrl->startTalk();
        return true;
    }

    bool tryTalkTimeKeepDemoMarioPuppetable(TalkMessageCtrl *pCtrl) {
        if (pCtrl == nullptr || !pCtrl->requestTalkForce()) {
            return false;
        }
        pCtrl->startTalkForcePuppetable();
        return true;
    }

    bool tryTalkTimeKeepDemoWithoutPauseMarioPuppetable(TalkMessageCtrl *pCtrl) {
        if (pCtrl == nullptr || !pCtrl->requestTalkForce()) {
            return false;
        }
        pCtrl->startTalkForceWithoutDemoPuppetable();
        return true;
    }

    bool isNearPlayer(const TalkMessageCtrl *pCtrl, f32 distance) {
        return pCtrl != nullptr && pCtrl->isNearPlayer(distance);
    }

    void forwardNode(TalkMessageCtrl *pCtrl) {
        if (pCtrl == nullptr) {
            return;
        }
        ++sTalkStates[pCtrl].node_index;
        pCtrl->_18 = 0;
        trace_talk(pCtrl, "node_forwarded");
    }

    void setDistanceToTalk(TalkMessageCtrl *pCtrl, f32 distance) {
        if (pCtrl != nullptr) {
            pCtrl->mTalkDistance = distance;
        }
    }

    void onRootNodeAutomatic(TalkMessageCtrl *pCtrl) {
        if (pCtrl != nullptr) {
            pCtrl->mIsOnRootNodeAuto = true;
        }
    }

    void offRootNodeAutomatic(TalkMessageCtrl *pCtrl) {
        if (pCtrl != nullptr) {
            pCtrl->mIsOnRootNodeAuto = false;
        }
    }
}  // namespace MR
