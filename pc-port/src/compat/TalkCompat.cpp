#include "Game/NPC/TalkMessageCtrl.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NPC/NPCActor.hpp"
#include "Game/NPC/TalkMessageFunc.hpp"
#include "Game/NPC/TalkNodeCtrl.hpp"
#include "Game/Util/MessageUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/TalkUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "compat/TalkRuntime.hpp"

#include <cstdio>
#include <memory>
#include <stdexcept>

namespace {

    [[nodiscard]] bool time_keep_demo_active() {
        const auto* runtime = smgpc::compat::active_demo_scene_runtime();
        return runtime != nullptr && runtime->is_time_keep_active();
    }

    [[nodiscard]] bool consume_talk_end(TalkMessageCtrl* controller) {
        return controller != nullptr &&
               smgpc::compat::require_talk_runtime("Talk AtEnd query")
                   .consume_end(*controller);
    }

}  // namespace

namespace smgpc::compat {

    TalkMessageCtrl* owned_talk_ctrl(const LiveActor* actor) {
        const auto* runtime = current_talk_runtime();
        return runtime != nullptr ? runtime->owned_controller(actor) : nullptr;
    }

    void release_talk_runtime_state(const LiveActor* actor) {
        if (auto* runtime = current_talk_runtime(); runtime != nullptr) {
            runtime->release_owned_controller(actor);
        }
    }

    bool has_owned_talk_ctrl(const LiveActor* actor) {
        const auto* runtime = current_talk_runtime();
        return runtime != nullptr && runtime->has_owned_controller(actor);
    }

}  // namespace smgpc::compat

namespace MR {

    const wchar_t* getGalaxyNameOnCurrentLanguage(const char* galaxy_name) {
        char message_id[256]{};
        std::snprintf(message_id, sizeof(message_id), "GalaxyName_%s",
                      galaxy_name != nullptr ? galaxy_name : "");
        return getGameMessageDirect(message_id);
    }

    void registerBranchFunc(TalkMessageCtrl* controller, const TalkMessageFuncBase& function) {
        if (controller != nullptr) {
            controller->registerBranchFunc(function);
        }
    }

    void registerEventFunc(TalkMessageCtrl* controller, const TalkMessageFuncBase& function) {
        if (controller != nullptr) {
            controller->registerEventFunc(function);
        }
    }

    void registerAnimeFunc(TalkMessageCtrl* controller, const TalkMessageFuncBase& function) {
        if (controller != nullptr) {
            controller->registerAnimeFunc(function);
        }
    }

    void registerKillFunc(TalkMessageCtrl* controller, const TalkMessageFuncBase& function) {
        if (controller != nullptr) {
            controller->registerKillFunc(function);
        }
    }

    void setMessageArg(TalkMessageCtrl* controller, int value) {
        if (controller != nullptr) {
            controller->setMessageArg(CustomTagArg(value, CustomTagArg::Type_Int));
        }
    }

    void setMessageArg(TalkMessageCtrl* controller, const wchar_t* value) {
        if (controller != nullptr) {
            controller->setMessageArg(CustomTagArg(value, CustomTagArg::Type_Char));
        }
    }

    TalkMessageCtrl* createTalkCtrl(LiveActor* actor, const JMapInfoIter& iter,
                                    const char* name, const TVec3f& offset, MtxPtr matrix) {
        auto controller = std::make_unique<TalkMessageCtrl>(actor, offset, matrix);
        controller->createMessage(iter, name);
        return smgpc::compat::require_talk_runtime("Placement talk-controller ownership")
            .adopt_owned_controller(actor, std::move(controller));
    }

    TalkMessageCtrl* createTalkCtrlDirect(LiveActor* actor, const JMapInfoIter& iter,
                                          const char* name, const TVec3f& offset,
                                          MtxPtr matrix) {
        auto controller = std::make_unique<TalkMessageCtrl>(actor, offset, matrix);
        controller->createMessageDirect(iter, name);
        return smgpc::compat::require_talk_runtime("Direct talk-controller ownership")
            .adopt_owned_controller(actor, std::move(controller));
    }

    TalkMessageCtrl* createTalkCtrlDirectOnRootNodeAutomatic(
        LiveActor* actor, const JMapInfoIter& iter, const char* name,
        const TVec3f& offset, MtxPtr matrix) {
        auto* controller = createTalkCtrlDirect(actor, iter, name, offset, matrix);
        controller->mIsOnRootNodeAuto = true;
        return controller;
    }

    bool tryTalkNearPlayer(TalkMessageCtrl* controller) {
        if (controller == nullptr || time_keep_demo_active() || !controller->requestTalk()) {
            return false;
        }
        controller->startTalk();
        return true;
    }

    bool tryTalkNearPlayerAtEnd(TalkMessageCtrl* controller) {
        if (controller == nullptr || time_keep_demo_active()) {
            return false;
        }
        if (consume_talk_end(controller)) {
            static_cast<void>(controller->requestTalk());
            return true;
        }
        if (controller->requestTalk()) {
            controller->startTalk();
        }
        return false;
    }

    bool tryTalkForce(TalkMessageCtrl* controller) {
        if (controller == nullptr || time_keep_demo_active() || !controller->requestTalkForce()) {
            return false;
        }
        controller->startTalkForce();
        return true;
    }

    bool tryTalkForceAtEnd(TalkMessageCtrl* controller) {
        if (controller == nullptr || time_keep_demo_active()) {
            return false;
        }
        if (consume_talk_end(controller)) {
            return true;
        }
        if (controller->requestTalkForce()) {
            controller->startTalkForce();
        }
        return false;
    }

    bool tryTalkForceWithoutDemo(TalkMessageCtrl* controller) {
        if (controller == nullptr || time_keep_demo_active() || !controller->requestTalkForce()) {
            return false;
        }
        controller->startTalkForceWithoutDemo();
        return true;
    }

    bool tryTalkForceWithoutDemoMarioPuppetable(TalkMessageCtrl* controller) {
        if (controller == nullptr || time_keep_demo_active() || !controller->requestTalkForce()) {
            return false;
        }
        controller->startTalkForceWithoutDemoPuppetable();
        return true;
    }

    bool tryTalkForceWithoutDemoAtEnd(TalkMessageCtrl* controller) {
        if (controller == nullptr || time_keep_demo_active()) {
            return false;
        }
        if (consume_talk_end(controller)) {
            return true;
        }
        if (controller->requestTalkForce()) {
            controller->startTalkForceWithoutDemo();
        }
        return false;
    }

    bool tryTalkForceWithoutDemoMarioPuppetableAtEnd(TalkMessageCtrl* controller) {
        if (controller == nullptr || time_keep_demo_active()) {
            return false;
        }
        if (consume_talk_end(controller)) {
            return true;
        }
        if (controller->requestTalkForce()) {
            controller->startTalkForceWithoutDemoPuppetable();
        }
        return false;
    }

    bool tryTalkTimeKeepDemo(TalkMessageCtrl* controller) {
        if (controller == nullptr || !controller->requestTalkForce()) {
            return false;
        }
        controller->startTalkForce();
        return true;
    }

    bool tryTalkTimeKeepDemoMarioPuppetable(TalkMessageCtrl* controller) {
        if (controller == nullptr || !controller->requestTalkForce()) {
            return false;
        }
        controller->startTalkForcePuppetable();
        return true;
    }

    bool tryTalkTimeKeepDemoWithoutPauseMarioPuppetable(TalkMessageCtrl* controller) {
        if (controller == nullptr || !controller->requestTalkForce()) {
            return false;
        }
        controller->startTalkForceWithoutDemoPuppetable();
        return true;
    }

    bool tryTalkRequest(TalkMessageCtrl* controller) {
        return controller != nullptr && controller->requestTalk();
    }

    bool tryTalkSelectLeft(TalkMessageCtrl*) {
        return isYesNoSelected() && isYesNoSelectedYes();
    }

    bool tryTalkSelectRight(TalkMessageCtrl*) {
        return isYesNoSelected() && !isYesNoSelectedYes();
    }

    const MtxPtr getMessageBalloonFollowMatrix(const TalkMessageCtrl* controller) {
        return controller != nullptr ? controller->mMtx : nullptr;
    }

    const TVec3f& getMessageBalloonFollowOffset(const TalkMessageCtrl* controller) {
        if (controller == nullptr) {
            throw std::logic_error("Talk balloon offset query requires a TalkMessageCtrl.");
        }
        return controller->_2C;
    }

    void setMessageBalloonFollowOffset(TalkMessageCtrl* controller, const TVec3f& offset) {
        if (controller != nullptr) {
            controller->_2C = offset;
        }
    }

    bool isNearPlayer(const TalkMessageCtrl* controller, f32 distance) {
        return controller != nullptr && controller->isNearPlayer(distance);
    }

    bool inMessageArea(const TalkMessageCtrl* controller) {
        return controller != nullptr && controller->inMessageArea();
    }

    bool isTalkNone(const TalkMessageCtrl* controller) {
        return controller != nullptr && controller->_18 == 0U;
    }

    bool isTalkEntry(const TalkMessageCtrl* controller) {
        return controller != nullptr && controller->_18 == 1U;
    }

    bool isTalkTalking(const TalkMessageCtrl* controller) {
        return controller != nullptr && controller->_18 == 3U;
    }

    bool isTalkEnableEnd(const TalkMessageCtrl* controller) {
        return controller != nullptr && controller->_18 == 4U;
    }

    void clearTalkState(TalkMessageCtrl* controller) {
        TalkFunction::onTalkStateNone(controller);
    }

    void resetNode(TalkMessageCtrl* controller) {
        if (controller == nullptr || controller->mNodeCtrl == nullptr) {
            return;
        }
        TalkFunction::onTalkStateNone(controller);
        controller->mNodeCtrl->resetFlowNode();
    }

    void readMessage(TalkMessageCtrl* controller) {
        if (controller != nullptr) {
            controller->readMessage();
        }
    }

    void forwardNode(TalkMessageCtrl* controller) {
        if (controller == nullptr || controller->mNodeCtrl == nullptr) {
            return;
        }
        TalkFunction::onTalkStateNone(controller);
        controller->mNodeCtrl->forwardFlowNode();
        controller->mNodeCtrl->recordTempFlowNode();
    }

    void resetAndForwardNode(TalkMessageCtrl* controller, s32 count) {
        if (controller == nullptr || controller->mNodeCtrl == nullptr) {
            return;
        }
        TalkFunction::onTalkStateNone(controller);
        controller->mNodeCtrl->resetFlowNode();
        for (auto index = s32{}; index < count; ++index) {
            TalkFunction::onTalkStateNone(controller);
            controller->mNodeCtrl->forwardFlowNode();
            controller->mNodeCtrl->recordTempFlowNode();
        }
    }

    void forwardNodeNextBranchLeft(TalkMessageCtrl* controller) {
        if (controller == nullptr || controller->mNodeCtrl == nullptr) return;
        TalkFunction::onTalkStateNone(controller);
        controller->mNodeCtrl->forwardFlowNode();
        controller->mNodeCtrl->forwardCurrentBranchNode(true);
        controller->mNodeCtrl->recordTempFlowNode();
    }

    void forwardNodeNextBranchRight(TalkMessageCtrl* controller) {
        if (controller == nullptr || controller->mNodeCtrl == nullptr) return;
        TalkFunction::onTalkStateNone(controller);
        controller->mNodeCtrl->forwardFlowNode();
        controller->mNodeCtrl->forwardCurrentBranchNode(false);
        controller->mNodeCtrl->recordTempFlowNode();
    }

    void forwardNodeCurrentBranchLeft(TalkMessageCtrl* controller) {
        if (controller == nullptr || controller->mNodeCtrl == nullptr) return;
        TalkFunction::onTalkStateNone(controller);
        controller->mNodeCtrl->forwardCurrentBranchNode(true);
        controller->mNodeCtrl->recordTempFlowNode();
    }

    void forwardNodeCurrentBranchRight(TalkMessageCtrl* controller) {
        if (controller == nullptr || controller->mNodeCtrl == nullptr) return;
        TalkFunction::onTalkStateNone(controller);
        controller->mNodeCtrl->forwardCurrentBranchNode(false);
        controller->mNodeCtrl->recordTempFlowNode();
    }

    void tryForwardNode(TalkMessageCtrl* controller) {
        if (controller != nullptr && controller->mNodeCtrl != nullptr &&
            controller->mNodeCtrl->isExistNextNode()) {
            forwardNode(controller);
        }
    }

    bool isExistNextNode(const TalkMessageCtrl* controller) {
        return controller != nullptr && controller->mNodeCtrl != nullptr &&
               controller->mNodeCtrl->isExistNextNode();
    }

    bool isShortTalk(const TalkMessageCtrl* controller) {
        return TalkFunction::isShortTalk(controller);
    }

    void setDistanceToTalk(TalkMessageCtrl* controller, f32 distance) {
        if (controller != nullptr) controller->mTalkDistance = distance;
    }
    void onRootNodeAutomatic(TalkMessageCtrl* controller) {
        if (controller != nullptr) controller->mIsOnRootNodeAuto = true;
    }
    void offRootNodeAutomatic(TalkMessageCtrl* controller) {
        if (controller != nullptr) controller->mIsOnRootNodeAuto = false;
    }
    void onReadNodeAutomatic(TalkMessageCtrl* controller) {
        if (controller != nullptr) controller->mIsOnReadNodeAuto = true;
    }
    void offReadNodeAutomatic(TalkMessageCtrl* controller) {
        if (controller != nullptr) controller->mIsOnReadNodeAuto = false;
    }
    void onStartOnlyFront(TalkMessageCtrl* controller) {
        if (controller != nullptr) controller->mIsStartOnlyFront = true;
    }
    bool isTalkStart(const TalkMessageCtrl* controller) {
        return TalkFunction::isTalkSystemStart(controller);
    }
    bool isTalkEnd(const TalkMessageCtrl* controller) {
        return TalkFunction::isTalkSystemEnd(controller);
    }

}  // namespace MR
