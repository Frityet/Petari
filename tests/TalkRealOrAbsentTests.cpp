#include "OriginalStageResourceProcessFixture.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "NativeHeapFixture.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NPC/TalkBalloon.hpp"
#include "Game/NPC/TalkDirector.hpp"
#include "Game/NPC/TalkMessageCtrl.hpp"
#include "Game/NPC/TalkNodeCtrl.hpp"
#include "Game/NPC/TalkTextFormer.hpp"
#include "Game/Screen/LayoutManager.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/MessageHolder.hpp"
#include "Game/System/ResourceHolderManager.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "Game/Util/TalkUtil.hpp"
#include <nw4r/lyt/textBox.h>
#include <aurora/dvd.h>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>

namespace {
constexpr auto cFlow = "HeavensDoorMysteriousZone_DemoRabbit000";
void require(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string(message));
}
void verify_original_talk() {
    const auto allocation = MR::getSceneObjHolder()->nativeAllocationHeap();
    require(allocation != nullptr, "original Talk fixture requires the initialized GameScene heap");
    const JKRHeap::CurrentHeapScope game(*(allocation));
    const aurora::allocation::ClientAllocationScope gameRouting({true, true});
    auto* talk = MR::getSceneObj<TalkDirector>(SceneObj_TalkDirector);
    require(talk && talk->mPeekZ && talk->mBalloonHolder && talk->mStateHolder,
            "original process constructs the real TalkDirector and child owners");
    require(!MR::isSystemTalking() && !MR::isNormalTalking() && !MR::getTalkingActor(),
            "the initialized original Wait nerve has no active conversation");
    const auto baseline = talk->mMsgControls.mCount;
    struct RestoreInvalid {
        TalkDirector& talk; bool invalid;
        ~RestoreInvalid() { talk._4E = invalid; }
    } restore{*talk, talk->_4E};
    auto actor = std::make_unique<LiveActor>("Original talk flow participant");
    auto first = std::unique_ptr<TalkMessageCtrl>(MR::createTalkCtrlDirect(actor.get(), JMapInfoIter{}, cFlow, TVec3f{}, nullptr));
    auto second = std::unique_ptr<TalkMessageCtrl>(MR::createTalkCtrlDirect(actor.get(), JMapInfoIter{}, cFlow, TVec3f{}, nullptr));
    require(first.get() != second.get() && talk->mMsgControls.mCount == baseline + 2 &&
                talk->mMsgControls.begin()[baseline] == first.get() && talk->mMsgControls.begin()[baseline + 1] == second.get(),
            "multiple controllers for one actor preserve original registration order and independent identities");
    auto* messages = MessageSystem::getSceneMessageData();
    require(first->mNodeCtrl && std::string_view(first->mNodeCtrl->_0) == cFlow &&
                first->mNodeCtrl->mCurrentNode == messages->getNode(268) && first->getMessageID() == 825,
            "the original reader skips Gateway FlowTalk node 267 and selects node 268/message 825");

    auto* initial_node = first->mNodeCtrl->mCurrentNode;
    auto* next_node = first->mNodeCtrl->getNextNode();
    require(next_node, "the real Gateway flow must contain its authored next node");
    MR::forwardNode(first.get());
    require(first->mNodeCtrl->mCurrentNode == next_node && second->mNodeCtrl->mCurrentNode == initial_node,
            "original flow progression changes exactly one controller");
    MR::resetNode(first.get());
    require(first->mNodeCtrl->mCurrentNode == initial_node, "original reset restores the recorded Gateway root node");

    MR::invalidateTalkDirector();
    require(!first->requestTalkForce() && !first->startTalkForceWithoutDemoPuppetable() && MR::isTalkNone(first.get()),
            "the original invalidation gate rejects forced requests without publishing a partial talk");
    require(!MR::isSystemTalking() && !MR::getTalkingActor(), "rejected requests retain the original idle query results");

    auto* balloon = talk->mBalloonHolder->mBalloonShortArray[0];
    struct CloseBalloonBeforeControllers {
        TalkBalloon& balloon;
        ~CloseBalloonBeforeControllers() { balloon.kill(); }
    } close_balloon{*balloon};
    balloon->open(first.get());
    auto* box = nw4r::ut::DynamicCast<nw4r::lyt::TextBox*>(balloon->getLayoutManager()->getPane("TxtText"));
    require(box && box->mTextLen && balloon->mMessageCtrl == first.get() &&
                balloon->mTextFormer->mMsg == TalkFunction::getMessage(first.get()) && !MR::isDead(balloon),
            "the original balloon and text former populate the actual TextBox from the selected BMG message");
    unsigned steps = 0;
    while (!balloon->mTextFormer->isTextAppearedAll() && steps < 4096) {
        balloon->mTextFormer->updateTalking();
        ++steps;
    }
    require(steps > 0 && steps < 4096, "original tag reveal advances over multiple steps and finishes");
    balloon->kill();
    require(!balloon->mMessageCtrl && MR::isDead(balloon), "original balloon kill releases its controller reference");
    first.reset();
    require(talk->mMsgControls.mCount == baseline + 1 && talk->mMsgControls.begin()[baseline] == second.get(),
            "native deletion removes the exact completed controller from the original director vector");

    balloon->open(second.get());
    // Original kill ends the balloon borrow before deleting a participant in a live scene.
    balloon->kill();
    second.reset();
    require(talk->mMsgControls.mCount == baseline && !balloon->mMessageCtrl,
            "original balloon kill permits retiring the remaining controller from the live director");
    actor.reset();
}
}
int main() { return smgpc::test::run_stage_resource_process("talk-owner", verify_original_talk); }
