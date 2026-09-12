#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NPC/TalkBalloon.hpp"
#include "Game/NPC/TalkDirector.hpp"
#include "Game/NPC/TalkMessageCtrl.hpp"
#include "Game/NPC/TalkNodeCtrl.hpp"
#include "Game/NPC/TalkTextFormer.hpp"
#include "Game/Scene/PlacementStateChecker.hpp"
#include "Game/Screen/LayoutManager.hpp"
#include "Game/System/GameDataHolder.hpp"
#include "Game/System/GameSystem.hpp"
#include "Game/System/MessageHolder.hpp"
#include "Game/System/ResourceHolderManager.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "Game/Util/TalkUtil.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "SceneExecutionFixture.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/GameDataFunctionCompat.hpp"
#include "compat/StageSessionState.hpp"
#include "runtime/RuntimeContext.hpp"
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

void require_no_talk_owner() {
    require(!MR::isExistSceneObj(SceneObj_TalkDirector) &&
                !MR::isSystemTalking() && !MR::isNormalTalking() && MR::getTalkingActor() == nullptr,
            "original DemoUtil explicitly reports no talk when the scene has no TalkDirector");
}

void require_process_owners() {
    // Positive cases require the same owners as original process initialization.
    // A RuntimeContext by itself does not stand in for GameSystem or its fonts.
    auto* system = SingletonHolder<GameSystem>::get();
    require(system && system->mFontHolder,
            "original Talk integration requires completed GameSystem font initialization");
    require(SingletonHolder<ResourceHolderManager>::get(),
            "original Talk integration requires GameSystemObjHolder's ResourceHolderManager");
}

void test_original_talk(smgpc::runtime::RuntimeContext& runtime) {
    require_process_owners();
    runtime.set_current_stage_name("HeavensDoorGalaxy");
    auto game_data = GameDataHolder(nullptr);
    const smgpc::compat::ScopedGameDataHolderOverride game_data_binding(game_data);
    auto stage = smgpc::compat::StageSessionState("Game", "HeavensDoorGalaxy", 1, JMapIdInfo(0, 0));
    const smgpc::compat::StageSessionBinding stage_binding(stage);
    const smgpc::runtime::SceneSchedulerBinding scheduler_binding(runtime.scheduler());
    const auto registry_baseline = smgpc::compat::name_obj_runtime_state_count();
    const auto allocation = smgpc::compat::JkrAllocationDomain::create(runtime.host_heaps(), 8U << 20);
    {
        smgpc::test::SceneExecutionFixture execution(runtime.scheduler(), allocation);
        const smgpc::compat::JkrAllocationScope game(allocation);
        execution.holder().create(SceneObj_NameObjGroup);
        execution.holder().create(SceneObj_PlacementStateChecker);
        auto* talk = dynamic_cast<TalkDirector*>(execution.holder().create(SceneObj_TalkDirector));
        require(talk && talk->mPeekZ && talk->mBalloonHolder && talk->mStateHolder && talk->mMsgControls.mCount == 0,
                "the scene factory must fully initialize the original TalkDirector and its actual child owners");
        require(execution.holder().create(SceneObj_TalkDirector) == talk,
                "repeated original scene creation returns the same TalkDirector");
        require(!MR::isSystemTalking() && !MR::isNormalTalking() && !MR::getTalkingActor(),
                "the original Wait nerve has no active conversation");

        auto actor = std::make_unique<LiveActor>("Original talk flow participant");
        auto first = std::unique_ptr<TalkMessageCtrl>(MR::createTalkCtrlDirect(actor.get(), JMapInfoIter{}, cFlow, TVec3f{}, nullptr));
        auto second = std::unique_ptr<TalkMessageCtrl>(MR::createTalkCtrlDirect(actor.get(), JMapInfoIter{}, cFlow, TVec3f{}, nullptr));
        require(first.get() != second.get() && talk->mMsgControls.mCount == 2 &&
                    talk->mMsgControls.begin()[0] == first.get() && talk->mMsgControls.begin()[1] == second.get(),
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
        balloon->open(first.get());
        auto* box = nw4r::ut::DynamicCast<nw4r::lyt::TextBox*>(balloon->getLayoutManager()->getPane("Text00"));
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
        require(talk->mMsgControls.mCount == 1 && talk->mMsgControls.begin()[0] == second.get(),
                "native deletion removes the exact completed controller from the original director vector");

        balloon->open(second.get());
        execution.objects().prepare_retirement();
        second.reset();
        require(talk->mMsgControls.mCount == 0 && !balloon->mMessageCtrl,
                "scene retirement removes a still-borrowed controller after movement has stopped");
        actor.reset();
    }
    require_no_talk_owner();
    require(smgpc::compat::name_obj_runtime_state_count() == registry_baseline,
            "the original TalkDirector, balloons and helper NameObjs retire with the actual scene graph");
}
} // namespace

int main() {
    require_no_talk_owner();
    const auto* disc = std::getenv("SMGPC_REAL_DISC");
    require(disc && std::filesystem::is_regular_file(disc), "SMGPC_REAL_DISC must name the real game image");
    require(aurora_dvd_open(disc), "the real disc must open through Aurora DVD");
    struct Disc { ~Disc() { aurora_dvd_close(); } } close_disc;
    DVDInit();
    auto logger = smgpc::logging::create_default_logger();
    smgpc::render::AuroraWindow window({.width = 640, .height = 456, .title = "Original Talk owner proof"});
    smgpc::resource::GameResourceRuntime resources;
    smgpc::runtime::RuntimeContext runtime(*logger, window, resources);
    test_original_talk(runtime);
    std::cout << "PASS original Talk owner, real flow, balloon interaction and scene retirement\n";
}
