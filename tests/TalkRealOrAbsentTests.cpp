#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NPC/NPCActor.hpp"
#include "Game/NPC/TalkMessageCtrl.hpp"
#include "Game/NPC/TalkNodeCtrl.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Scene/PlacementStateChecker.hpp"
#include "Game/System/GameDataHolder.hpp"
#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/TalkUtil.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "compat/GameDataFunctionCompat.hpp"
#include "compat/StageSessionState.hpp"
#include "compat/TalkRuntime.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/NameObjChildOwner.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/PlacementZoneNameScope.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

    constexpr auto cFlowKey = std::string_view{
        "HeavensDoorMysteriousZone_DemoRabbit000"};
    constexpr auto cRunawayTicoFlowKey = std::string_view{
        "HeavensDoorMysteriousZone_RunawayTico007"};
    constexpr auto cTicoReactionFlowKey = std::string_view{"Common_Tico000"};

    class ElementModeFixtureActor final : public LiveActor {
    public:
        ElementModeFixtureActor() : LiveActor("element-mode fixture") {
        }

        s32 mode = 0;
    };

    class MultiControllerFixtureActor final : public NPCActor {
    public:
        MultiControllerFixtureActor()
            : NPCActor("synthetic Tico talk-controller ownership proof") {
        }

        void initAfterPlacement() override {
            ++postpass_count;
        }

        std::size_t postpass_count = 0U;
    };

    s32 read_fixture_element_mode(const LiveActor& actor) {
        const auto* fixture = dynamic_cast<const ElementModeFixtureActor*>(&actor);
        if (fixture == nullptr) {
            throw std::logic_error(
                "the focused element-mode capability received the wrong actor type");
        }
        return fixture->mode;
    }

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    template <typename Operation>
    void require_logic_error(Operation&& operation, std::string_view message) {
        auto threw = false;
        try {
            std::forward<Operation>(operation)();
        } catch (const std::logic_error&) {
            threw = true;
        }
        require(threw, message);
    }

    [[nodiscard]] std::filesystem::path require_real_disc() {
        if (const auto* configured = std::getenv("SMGPC_REAL_DISC");
            configured != nullptr && configured[0] != '\0') {
            const auto path = std::filesystem::path(configured);
            require(std::filesystem::is_regular_file(path),
                    "SMGPC_REAL_DISC must name the real RMGK01 image");
            return path;
        }

        auto error = std::error_code{};
        auto directory = std::filesystem::current_path(error);
        require(!error, "the Talk proof requires a readable working directory");
        while (true) {
            for (const auto name : {"RMGK01.iso", "RMGK01.wbfs"}) {
                const auto candidate = directory / name;
                if (std::filesystem::is_regular_file(candidate, error) && !error) {
                    return candidate;
                }
                error.clear();
            }
            const auto parent = directory.parent_path();
            if (parent == directory || parent.empty()) {
                break;
            }
            directory = parent;
        }
        throw std::runtime_error(
            "the Talk proof requires real RMGK01.iso (or SMGPC_REAL_DISC)");
    }

    void set_wpad_buttons(smgpc::runtime::RuntimeContext& runtime,
                          std::uint32_t buttons) {
        auto& wpad = runtime.wpad();
        wpad.begin_frame();
        wpad.set_button_mask(WPAD_CHAN0, buttons);
    }

    void move_talk(smgpc::compat::TalkRuntime& talk,
                   smgpc::runtime::RuntimeContext& runtime,
                   std::uint32_t buttons) {
        set_wpad_buttons(runtime, buttons);
        talk.movement();
    }

    void test_missing_scene_binding_is_explicit() {
        require(smgpc::compat::current_talk_runtime() == nullptr,
                "the negative constructor proof must begin without a TalkRuntime binding");
        auto actor = LiveActor("talk missing-binding actor");
        const auto baseline = smgpc::compat::name_obj_runtime_state_count();
        require_logic_error(
            [&] {
                auto controller = TalkMessageCtrl(&actor, TVec3f{}, nullptr);
                static_cast<void>(controller);
            },
            "TalkMessageCtrl construction without SceneObj_TalkDirector must fail explicitly");
        require(smgpc::compat::current_talk_runtime() == nullptr &&
                    smgpc::compat::name_obj_runtime_state_count() == baseline,
                "failed TalkMessageCtrl construction must leave no partial owner or NameObj state");
    }

    void test_missing_stage_session_is_explicit() {
        auto value = u32{};
        require_logic_error(
            [&] {
                static_cast<void>(
                    MR::setupAlreadyDoneFlag("unbound", JMapInfoIter{}, &value));
            },
            "AlreadyDone setup without an active stage session must fail explicitly");
        require_logic_error(
            [] { MR::updateAlreadyDoneFlag(0, 1U); },
            "AlreadyDone update without an active stage session must fail explicitly");
    }

    void test_stage_session_already_done_registry() {
        auto session = smgpc::compat::StageSessionState(
            "Game", "DuplicateGalaxy", 1, JMapIdInfo(0, 0));
        auto value = u32{99U};
        const auto first = session.setup_already_done_flag(
            0x9234U, 5, -1, &value);
        require(first == 0 && value == 0U,
                "a new AlreadyDone key must allocate an off entry");
        session.update_already_done_flag(first, 1U);
        value = 0U;
        const auto duplicate = session.setup_already_done_flag(
            0x1234U, 5, -1, &value);
        require(duplicate == first && value == 1U,
                "AlreadyDone identity must mask the hash high bit and retain its value");
        value = 99U;
        const auto distinct = session.setup_already_done_flag(
            0x1234U, 5, 0, &value);
        require(distinct == 1 && value == 0U,
                "AlreadyDone link ID must remain part of the exact key");

        auto capacity_session = smgpc::compat::StageSessionState(
            "Game", "CapacityGalaxy", 1, JMapIdInfo(0, 0));
        for (auto index = s32{}; index < 64; ++index) {
            value = 99U;
            require(capacity_session.setup_already_done_flag(
                        static_cast<u16>(index + 1), index, index, &value) == index &&
                        value == 0U,
                    "the retail AlreadyDone registry must admit all 64 entries");
        }
        require_logic_error(
            [&] {
                static_cast<void>(capacity_session.setup_already_done_flag(
                    0x7ffeU, 100, 100, &value));
            },
            "the 65th AlreadyDone identity must fail explicitly");
    }

    void test_event_value_utilities() {
        require(!MR::isOnMessageAlreadyRead(3),
                "a fresh MessageAlreadyRead event value must begin clear");
        MR::onMessageAlreadyRead(3);
        require(MR::isOnMessageAlreadyRead(3),
                "MessageAlreadyRead must retain the exact event-value bit");
        require_logic_error(
            [] { static_cast<void>(MR::isOnMessageAlreadyRead(-1)); },
            "negative MessageAlreadyRead bits must fail explicitly");
        require_logic_error(
            [] { MR::onMessageAlreadyRead(16); },
            "MessageAlreadyRead bits outside its u16 storage must fail explicitly");

        MR::offMsgLedPattern();
        require(!MR::isMsgLedPattern(),
                "offMsgLedPattern must clear the retained retail event value");
        MR::onMsgLedPattern();
        require(MR::isMsgLedPattern(),
                "onMsgLedPattern must set the retained retail event value");
    }

    void test_player_and_screen_providers(
        smgpc::runtime::RuntimeContext& runtime) {
        auto host = LiveActor("talk distance host");
        host.mPosition.set(0.0F, 0.0F, 0.0F);
        require_logic_error(
            [&] { static_cast<void>(MR::isNearPlayerAnyTime(&host, 10.0F)); },
            "AnyTime distance must fail without an attached player");
        require_logic_error(
            [] { static_cast<void>(MR::isPlayerElementModeNormal()); },
            "player element mode must fail without an attached player");

        auto generic_player = LiveActor("マリオアクター");
        generic_player.mPosition.set(3.0F, 4.0F, 0.0F);
        runtime.player_system().attach_actor(generic_player);
        require(MR::isNearPlayerAnyTime(&host, 6.0F) &&
                    !MR::isNearPlayerAnyTime(&host, 5.0F),
                "AnyTime distance must use the exact attached-player position and strict radius");
        require_logic_error(
            [] { static_cast<void>(MR::isPlayerElementModeBee()); },
            "a spoofed Mario actor name must not grant an element-mode capability");
        runtime.player_system().detach_actor(&generic_player);

        auto mode_player = ElementModeFixtureActor{};
        runtime.player_system().attach_actor(
            mode_player,
            smgpc::runtime::PlayerActorBridge{
                .read_element_mode = &read_fixture_element_mode,
            });
        require(MR::isPlayerElementModeNormal() &&
                    !MR::isPlayerElementModeBee(),
                "the attached player capability must expose exact normal mode zero");
        mode_player.mode = 4;
        require(MR::isPlayerElementMode(4) &&
                    MR::isPlayerElementModeBee() &&
                    !MR::isPlayerElementModeTeresa(),
                "element-mode helpers must read the owner-supplied retail mode value");
        runtime.player_system().detach_actor(&mode_player);

        require_logic_error(
            [] { static_cast<void>(MR::isYesNoSelected()); },
            "Yes/No selection must fail until a scene selector owns the result");
        require_logic_error(
            [] { static_cast<void>(MR::isYesNoSelectedYes()); },
            "Yes/No answer identity must fail until a scene selector owns the result");
    }

    void test_nested_placement_zone_scope(SceneObjHolder& holder) {
        auto* checker = dynamic_cast<PlacementStateChecker*>(
            holder.create(SceneObj_PlacementStateChecker));
        require(checker != nullptr,
                "the placement-zone proof requires SceneObj_PlacementStateChecker");
        checker->setCurrentPlacementZoneId(41);
        require_logic_error(
            [] { static_cast<void>(MR::getCurrentPlacementZoneName()); },
            "a checker ID alone must not invent a placement-zone name");
        {
            auto outer = smgpc::scene::PlacementZoneNameScope(5, "OuterZone");
            require(MR::getCurrentPlacementZoneId() == 5 &&
                        std::string_view(MR::getCurrentPlacementZoneName()) ==
                            "OuterZone",
                    "the outer placement scope must install its exact ID and copied name");
            {
                auto inner = smgpc::scene::PlacementZoneNameScope(9, "InnerZone");
                require(MR::getCurrentPlacementZoneId() == 9 &&
                            std::string_view(MR::getCurrentPlacementZoneName()) ==
                                "InnerZone",
                        "the nested placement scope must install its own ID and copied name");
            }
            require(MR::getCurrentPlacementZoneId() == 5 &&
                        std::string_view(MR::getCurrentPlacementZoneName()) ==
                            "OuterZone",
                    "nested placement teardown must restore the outer ID and copied name");
        }
        require(MR::getCurrentPlacementZoneId() == 41,
                "outer placement teardown must restore the checker's prior ID");
        require_logic_error(
            [] { static_cast<void>(MR::getCurrentPlacementZoneName()); },
            "outer placement teardown must clear its copied-name surface");
        checker->clearCurrentPlacementZoneId();
    }

    void test_same_actor_multi_controller_ownership(
        smgpc::compat::TalkRuntime& talk) {
        const auto registry_baseline =
            smgpc::compat::name_obj_runtime_state_count();
        const auto marker = smgpc::compat::mark_name_obj_runtime_registrations();
        auto host = std::make_unique<MultiControllerFixtureActor>();
        auto* placement_controller = MR::createTalkCtrlDirect(
            host.get(), JMapInfoIter{}, cRunawayTicoFlowKey.data(),
            TVec3f{}, nullptr);
        host->mMsgCtrl = placement_controller;
        auto* reaction_controller = MR::createTalkCtrlDirect(
            host.get(), JMapInfoIter{}, cTicoReactionFlowKey.data(),
            TVec3f{}, nullptr);

        require(placement_controller != nullptr &&
                    reaction_controller != nullptr &&
                    placement_controller != reaction_controller &&
                    host->mMsgCtrl == placement_controller &&
                    talk.owned_controller(host.get()) == placement_controller &&
                    smgpc::compat::owned_talk_ctrl(host.get()) ==
                        placement_controller &&
                    talk.owned_controller_count(host.get()) == 2U &&
                    talk.flow_key(*placement_controller) ==
                        cRunawayTicoFlowKey &&
                    talk.flow_key(*reaction_controller) ==
                        cTicoReactionFlowKey,
                "one actor must retain distinct placement and Common_Tico000 controller identities");
        require(smgpc::compat::name_obj_runtime_owner(placement_controller) ==
                        &talk &&
                    smgpc::compat::name_obj_runtime_owner(reaction_controller) ==
                        &talk &&
                    smgpc::compat::name_obj_runtime_state_count() ==
                        registry_baseline + 3U,
                "both same-actor controllers must belong to TalkRuntime without replacing the host NameObj");

        // This is the construction topology produced by real Tico::initMessage:
        // the actor is the root while both TalkMessageCtrls have independent
        // TalkRuntime storage. The root postpass must delegate both identities
        // exactly once without taking their storage ownership.
        auto postpass_owner = smgpc::scene::NameObjChildOwner{};
        postpass_owner.adopt_root_registration_suffix(
            marker, *host, &host);
        require(smgpc::compat::name_obj_runtime_postpass_delegate(
                    placement_controller) == &postpass_owner &&
                    smgpc::compat::name_obj_runtime_postpass_delegate(
                        reaction_controller) == &postpass_owner,
                "the placement root must delegate both independently-owned controller postpasses");
        postpass_owner.init_registration_suffix_after_placement();
        require(host->postpass_count == 1U &&
                    !smgpc::compat::name_obj_runtime_postpass_is_delegated(
                    placement_controller) &&
                    !smgpc::compat::name_obj_runtime_postpass_is_delegated(
                        reaction_controller),
                "both same-actor controller postpass delegations must clear after execution");

        // A failed append must retire only the incoming controller. In
        // particular, neither stable controller nor NPCActor::mMsgCtrl may be
        // changed before a new ownership claim succeeds.
        const auto owned_registry_count =
            smgpc::compat::name_obj_runtime_state_count();
        auto rejected_controller = std::make_unique<TalkMessageCtrl>(
            host.get(), TVec3f{}, nullptr);
        const auto foreign_owner = std::uint8_t{};
        smgpc::compat::claim_name_obj_runtime_ownership(
            rejected_controller.get(), &foreign_owner);
        require_logic_error(
            [&] {
                static_cast<void>(talk.adopt_owned_controller(
                    host.get(), std::move(rejected_controller)));
            },
            "a preclaimed same-actor controller insertion must fail transactionally");
        require(rejected_controller == nullptr &&
                    host->mMsgCtrl == placement_controller &&
                    talk.owned_controller(host.get()) == placement_controller &&
                    talk.owned_controller_count(host.get()) == 2U &&
                    talk.flow_key(*placement_controller) ==
                        cRunawayTicoFlowKey &&
                    talk.flow_key(*reaction_controller) ==
                        cTicoReactionFlowKey &&
                    smgpc::compat::name_obj_runtime_state_count() ==
                        owned_registry_count,
                "a rejected third insertion must preserve both stable controllers and retire its temporary NameObj state");

        postpass_owner.clear();
        smgpc::compat::release_talk_runtime_state(host.get());
        require(host->mMsgCtrl == nullptr &&
                    talk.owned_controller(host.get()) == nullptr &&
                    talk.owned_controller_count(host.get()) == 0U &&
                    !smgpc::compat::has_name_obj_runtime_state(
                        placement_controller) &&
                    !smgpc::compat::has_name_obj_runtime_state(
                        reaction_controller) &&
                    smgpc::compat::name_obj_runtime_state_count() ==
                        registry_baseline + 1U,
                "actor teardown must release every controller while retaining only the still-live host");
        host.reset();
        require(smgpc::compat::name_obj_runtime_state_count() ==
                    registry_baseline,
                "the same-actor ownership proof must restore its exact NameObj baseline");
    }

    void require_exact_flow_data(const smgpc::runtime::MessageService& messages) {
        const auto root_message = messages.message_index(cFlowKey);
        require(root_message == std::optional<std::uint32_t>{824U},
                "the real DemoRabbit flow key must resolve to MessageId index 824");
        require(messages.first_flow_node_for_message(*root_message) ==
                    std::optional<std::uint32_t>{267U},
                "the real DemoRabbit flow root must be FLW node 267");

        const auto* node267 = messages.flow_node(267U);
        const auto* node268 = messages.flow_node(268U);
        const auto* node269 = messages.flow_node(269U);
        const auto* node270 = messages.flow_node(270U);
        require(node267 != nullptr && node267->node_type == 1U &&
                    node267->index == 824U && node267->next_index == 268U &&
                    node268 != nullptr && node268->node_type == 1U &&
                    node268->index == 825U && node268->next_index == 269U &&
                    node269 != nullptr && node269->node_type == 1U &&
                    node269->index == 826U && node269->next_index == 270U &&
                    node270 != nullptr && node270->node_type == 1U &&
                    node270->index == 827U && node270->next_index == 0xffffU,
                "RMGK01 must retain the exact 824->825->826->827 DemoRabbit graph");

        const auto* id824 = messages.message_id(824U);
        const auto* id825 = messages.message_id(825U);
        const auto* id826 = messages.message_id(826U);
        const auto* id827 = messages.message_id(827U);
        require(id824 != nullptr && id825 != nullptr && id826 != nullptr &&
                    id827 != nullptr &&
                    messages.message_info(*id824)->talk_type == 4U &&
                    messages.message_info(*id825)->talk_type == 2U &&
                    messages.message_info(*id826)->talk_type == 1U &&
                    messages.message_info(*id827)->talk_type == 2U &&
                    messages.message_info(*id825)->camera_type == 2U &&
                    messages.message_info(*id826)->camera_type == 2U &&
                    messages.message_info(*id827)->camera_type == 2U,
                "the four real messages must retain FlowTalk/event/short/event metadata");
        require(messages.flow_node(std::numeric_limits<std::uint32_t>::max()) == nullptr &&
                    !messages.branch_flow_node(
                         std::numeric_limits<std::uint32_t>::max())
                         .has_value(),
                "out-of-range retained graph queries must expose explicit absence");
    }

    [[nodiscard]] const smgpc::scene::StagePlacementObject&
    require_demo_rabbit_cast(
        const std::vector<smgpc::scene::StagePlacementObject>& placements) {
        for (const auto& placement : placements) {
            if (placement.object_name == "DemoRabbit" && placement.cast_id == 0 &&
                placement.demo_group_id >= 0) {
                return placement;
            }
        }
        throw std::runtime_error(
            "HeavensDoorGalaxy scenario 1 is missing its exact cast-0 DemoRabbit placement");
    }

    void test_real_gateway_talk_runtime(smgpc::runtime::RuntimeContext& runtime) {
        runtime.set_current_stage_name("HeavensDoorGalaxy");
        require_exact_flow_data(runtime.messages());
        require(runtime.messages().message_index(cRunawayTicoFlowKey).has_value() &&
                    runtime.messages().message_index(cTicoReactionFlowKey).has_value(),
                "RMGK01 must contain RunawayTico007 and Common_Tico000 talk flows");
        test_player_and_screen_providers(runtime);

        auto runtime_teardown_host = NPCActor(
            "TalkRuntime multi-controller teardown proof");
        const auto scene_registry_baseline =
            smgpc::compat::name_obj_runtime_state_count();
        auto holder = SceneObjHolder{};
        {
            auto holder_binding = smgpc::scene::SceneObjHolderBinding(holder);
            test_nested_placement_zone_scope(holder);
            const auto registrations_before_talk =
                runtime.scheduler().registration_marker();
            auto* talk = dynamic_cast<smgpc::compat::TalkRuntime*>(
                holder.create(SceneObj_TalkDirector));
            require(talk != nullptr &&
                        smgpc::compat::current_talk_runtime() == talk &&
                        runtime.scheduler().registration_marker() ==
                            registrations_before_talk + 1U,
                    "SceneObj_TalkDirector must create and schedule exactly one TalkRuntime");

            const auto registrations_after_talk =
                runtime.scheduler().registration_marker();
            require(MR::createSceneObj(SceneObj_TalkDirector) == talk &&
                        runtime.scheduler().registration_marker() ==
                            registrations_after_talk,
                    "re-requesting SceneObj_TalkDirector must be idempotent");

            test_same_actor_multi_controller_ownership(*talk);

            auto actor = LiveActor("DemoRabbit talk proof");
            auto* controller = MR::createTalkCtrlDirect(
                &actor, JMapInfoIter{}, cFlowKey.data(), TVec3f{}, nullptr);
            require(controller != nullptr &&
                        smgpc::compat::owned_talk_ctrl(&actor) == controller &&
                        smgpc::compat::has_owned_talk_ctrl(&actor) &&
                        smgpc::compat::name_obj_runtime_owner(controller) ==
                            talk &&
                        MR::createSceneObj(SceneObj_TalkDirector) == talk &&
                        runtime.scheduler().registration_marker() ==
                            registrations_after_talk,
                    "TalkMessageCtrl's retail createSceneObj call must reuse the pre-created owner and retain one TalkRuntime storage owner");
            require(talk->flow_key(*controller) == cFlowKey &&
                        talk->current_node_index(*controller) ==
                            std::optional<std::uint32_t>{268U} &&
                        controller->getMessageID() == 825U,
                    "FlowTalk node267 must be skipped and leave DemoRabbit on node268/message825");

            // Carry A into the forced open. It must not dismiss until release
            // and a later fresh A edge.
            set_wpad_buttons(runtime, WPAD_BUTTON_A);
            require(MR::tryTalkForceWithoutDemoMarioPuppetable(controller),
                    "forced no-demo talk must open the exact message825 presentation");
            require(talk->active_presentation().has_value() &&
                        talk->active_presentation()->message_index == 825U &&
                        talk->active_presentation()->node_index ==
                            std::optional<std::uint32_t>{268U},
                    "the active presentation must capture node268/message825");
            MR::forwardNode(controller);
            require(talk->current_node_index(*controller) ==
                            std::optional<std::uint32_t>{269U} &&
                        controller->getMessageID() == 826U &&
                        talk->active_presentation().has_value() &&
                        talk->active_presentation()->message_index == 825U,
                    "advancing DemoRabbit to message826 must not replace displayed message825");
            move_talk(*talk, runtime, WPAD_BUTTON_A);
            require(talk->active_presentation().has_value() && !MR::isTalkEnd(controller),
                    "a carried A hold must not complete event talk");
            move_talk(*talk, runtime, 0U);
            require(talk->active_presentation().has_value() &&
                        talk->active_presentation()->info.talk_type == 2U &&
                        controller->mNodeCtrl->mMessageInfo.isShortTalk() &&
                        talk->current_node_index(*controller) ==
                            std::optional<std::uint32_t>{269U},
                    "release must retain frozen event825 while the actor-side controller remains on short826");
            move_talk(*talk, runtime, WPAD_BUTTON_A);
            require(!talk->active_presentation().has_value() && MR::isTalkEnd(controller),
                    "a fresh post-release A edge must complete event talk");
            controller->endTalk();
            require(MR::isTalkEnd(controller),
                    "TalkMessageCtrl::endTalk must remain a non-recursive ABI-shaped end query");
            require(MR::tryTalkForceWithoutDemoMarioPuppetableAtEnd(controller) &&
                        !MR::isTalkEnd(controller),
                    "AtEnd must consume the completed-talk edge exactly once");

            // Message826 is short talk. Continued actor requests retain it;
            // one missed actor frame is tolerated because TalkDirector moves
            // before NPCs, then the following director frame closes it.
            set_wpad_buttons(runtime, 0U);
            require(controller->requestTalkForce(),
                    "short message826 must accept a forced request");
            controller->startTalkForceWithoutDemo();
            require(talk->active_presentation().has_value() &&
                        talk->active_presentation()->message_index == 826U,
                    "the free chase hint must present exact short message826");
            move_talk(*talk, runtime, 0U);
            require(talk->active_presentation().has_value() &&
                        controller->requestTalkForce(),
                    "a continued actor request must retain short message826");
            move_talk(*talk, runtime, 0U);
            require(talk->active_presentation().has_value(),
                    "TalkDirector-before-NPC order must retain the last short request for this frame");
            move_talk(*talk, runtime, 0U);
            require(!talk->active_presentation().has_value() &&
                        talk->consume_end(*controller),
                    "short message826 must close after its request lifetime expires");

            // Talk1 forwards before opening, selecting the final event line.
            MR::forwardNode(controller);
            require(talk->current_node_index(*controller) ==
                            std::optional<std::uint32_t>{270U} &&
                        controller->getMessageID() == 827U,
                    "DemoRabbit Talk1 pre-forward must select node270/message827");
            set_wpad_buttons(runtime, 0U);
            require(controller->requestTalkForce(),
                    "message827 must accept a forced request");
            controller->startTalkForceWithoutDemoPuppetable();
            require(talk->active_presentation().has_value() &&
                        talk->active_presentation()->message_index == 827U,
                    "Talk1 must present exact message827");
            move_talk(*talk, runtime, WPAD_BUTTON_A);
            require(!talk->active_presentation().has_value() &&
                        talk->consume_end(*controller),
                    "message827 must complete on a fresh A edge");

            // The real TicoGuide executor owns the same DemoRabbit cast. Talk
            // pause/resume must mutate that exact retained sheet.
            const auto placements = smgpc::scene::resolve_stage_placement_objects(
                runtime.dvd(), "HeavensDoorGalaxy", 1);
            const auto& rabbit_placement = require_demo_rabbit_cast(placements);
            {
                auto demo = smgpc::compat::DemoSceneRuntime(runtime.dvd(), placements);
                require(MR::tryRegisterDemoCast(
                            &actor,
                            JMapInfoIter(&rabbit_placement.jmap_info,
                                         rabbit_placement.jmap_entry_index)),
                        "the proof actor must register to the exact DemoRabbit executor");
                const auto start = demo.start_demo(
                    &actor, "チコガイドデモ", std::nullopt,
                    smgpc::compat::DemoPlayerMode::Normal);
                require(start == std::optional<smgpc::compat::DemoSheetStartResult>{
                                     smgpc::compat::DemoSheetStartResult::Started},
                        "the real TicoGuide timekeeper must start");
                const auto guide_index = demo.find_definition("チコガイドデモ");
                require(guide_index.has_value(),
                        "the real DemoSheet archive must contain TicoGuideDemo");
                const auto* guide = demo.definition(*guide_index);
                require(guide != nullptr && !guide->sheet.is_paused(),
                        "the exact TicoGuide sheet must begin unpaused");

                MR::resetNode(controller);
                require(talk->current_node_index(*controller) ==
                                std::optional<std::uint32_t>{268U} &&
                            controller->getMessageID() == 825U,
                        "resetNode must restore the post-FlowTalk root message825");
                set_wpad_buttons(runtime, 0U);
                require(MR::tryTalkTimeKeepDemoMarioPuppetable(controller),
                        "DemoRabbit Talk0 must open inside the active timekeeper");
                require(guide->sheet.is_paused() &&
                            talk->active_presentation().has_value() &&
                            talk->active_presentation()->time_keep_paused,
                        "opening Talk0 must pause the real TicoGuide sheet");
                MR::forwardNode(controller);
                move_talk(*talk, runtime, WPAD_BUTTON_A);
                require(!guide->sheet.is_paused() &&
                            !talk->active_presentation().has_value() &&
                            talk->consume_end(*controller),
                        "fresh-A Talk0 completion must resume the exact TicoGuide sheet");
                require(demo.stop_active_demo(
                            &actor,
                            std::optional<std::string_view>{"チコガイドデモ"}),
                        "the focused proof must stop its exact timekeeper cleanly");
            }

            // Missing keys and malformed/out-of-range retained graph links
            // fail explicitly rather than becoming message zero or an end.
            const auto baseline_controllers =
                smgpc::compat::name_obj_runtime_state_count();
            require_logic_error(
                [&] {
                    auto missing = TalkMessageCtrl(&actor, TVec3f{}, nullptr);
                    missing.createMessageDirect(JMapInfoIter{},
                                                "Missing_Talk_Flow_Key");
                },
                "a missing direct flow key must fail explicitly");
            require(smgpc::compat::name_obj_runtime_state_count() ==
                        baseline_controllers,
                    "a failed flow-key load must unregister its temporary controller");

            MR::resetNode(controller);
            auto* node = controller->mNodeCtrl->mCurrentNode;
            require(node != nullptr && node->mNodeType == 1U,
                    "the malformed-graph proof requires the real node268");
            const auto saved_next = node->mNextIdx;
            node->mNextIdx = 0xfffeU;
            require_logic_error(
                [&] { MR::forwardNode(controller); },
                "an out-of-range FLW next-node index must fail explicitly");
            node->mNextIdx = saved_next;
            const auto saved_type = node->mNodeType;
            node->mNodeType = 0xfeU;
            require_logic_error(
                [&] { MR::forwardNode(controller); },
                "an unknown FLW node kind must fail explicitly");
            node->mNodeType = saved_type;

            // Releasing the actor-owned controller while a presentation is
            // active must clear both the owner record and director identity.
            set_wpad_buttons(runtime, 0U);
            require(controller->requestTalkForce(),
                    "the teardown proof must start from a live controller");
            controller->startTalkForceWithoutDemo();
            require(talk->active_presentation().has_value(),
                    "the teardown proof must own an active presentation");
            smgpc::compat::release_talk_runtime_state(&actor);
            require(!smgpc::compat::has_owned_talk_ctrl(&actor) &&
                        smgpc::compat::owned_talk_ctrl(&actor) == nullptr &&
                        !talk->active_presentation().has_value(),
                    "actor teardown must clear its owned controller and active presentation");

            // Exercise the non-direct retail construction path against the
            // exact cast-0 placement. The copied placement-zone scope builds
            // the same key as NPCActor and the session-owned AlreadyDone
            // registry survives controller replacement.
            const auto rabbit_iter = JMapInfoIter(
                &rabbit_placement.jmap_info,
                rabbit_placement.jmap_entry_index);
            auto placement_actor = LiveActor("DemoRabbit placement talk proof");
            auto already_done_index = u32{};
            {
                auto zone = smgpc::scene::PlacementZoneNameScope(
                    rabbit_placement.zone_id, rabbit_placement.zone_name);
                auto* placement_controller = MR::createTalkCtrl(
                    &placement_actor, rabbit_iter, "DemoRabbit", TVec3f{}, nullptr);
                require(placement_controller != nullptr &&
                            talk->flow_key(*placement_controller) == cFlowKey &&
                            placement_controller->mZoneID == rabbit_placement.zone_id &&
                            placement_controller->_3C == 0U,
                        "the exact placement path must derive the real flow key and a fresh AlreadyDone entry");
                already_done_index = placement_controller->mAlreadyDoneFlags;
                placement_controller->readMessage();
                smgpc::compat::release_talk_runtime_state(&placement_actor);
            }

            auto replacement_actor = LiveActor("DemoRabbit replacement talk proof");
            {
                auto zone = smgpc::scene::PlacementZoneNameScope(
                    rabbit_placement.zone_id, rabbit_placement.zone_name);
                auto* replacement_controller = MR::createTalkCtrl(
                    &replacement_actor, rabbit_iter, "DemoRabbit", TVec3f{}, nullptr);
                require(replacement_controller != nullptr &&
                            replacement_controller->mAlreadyDoneFlags == already_done_index &&
                            replacement_controller->_3C == 1U,
                        "the duplicate exact placement key must recover the same completed AlreadyDone value");
                smgpc::compat::release_talk_runtime_state(&replacement_actor);
            }

            auto* runtime_teardown_placement = MR::createTalkCtrlDirect(
                &runtime_teardown_host, JMapInfoIter{},
                cRunawayTicoFlowKey.data(), TVec3f{}, nullptr);
            runtime_teardown_host.mMsgCtrl = runtime_teardown_placement;
            auto* runtime_teardown_reaction = MR::createTalkCtrlDirect(
                &runtime_teardown_host, JMapInfoIter{},
                cTicoReactionFlowKey.data(), TVec3f{}, nullptr);
            require(runtime_teardown_placement !=
                            runtime_teardown_reaction &&
                        talk->owned_controller(&runtime_teardown_host) ==
                            runtime_teardown_placement &&
                        talk->owned_controller_count(
                            &runtime_teardown_host) == 2U,
                    "the runtime-teardown proof must leave two live same-actor controllers owned by TalkRuntime");
        }
        require(smgpc::compat::current_talk_runtime() == nullptr &&
                    runtime_teardown_host.mMsgCtrl == nullptr &&
                    smgpc::compat::name_obj_runtime_state_count() ==
                        scene_registry_baseline,
                "SceneObjHolder teardown must release both same-actor controllers, clear the NPC pointer, and remove the TalkRuntime binding");
    }

}  // namespace

int main() {
    test_missing_scene_binding_is_explicit();
    test_missing_stage_session_is_explicit();

    const auto disc_path = require_real_disc();
    aurora_dvd_close();
    const auto disc_string = disc_path.string();
    require(aurora_dvd_open(disc_string.c_str()),
            "the selected RMGK01 image must open through Aurora DVD");
    struct DiscCloseGuard final {
        ~DiscCloseGuard() { aurora_dvd_close(); }
    } disc_close_guard;
    DVDInit();

    auto logger = smgpc::logging::create_default_logger();
    auto window = smgpc::render::AuroraWindow({
        .width = 640,
        .height = 456,
        .title = "SMG PC exact Talk/FLW proof",
    });
    auto resource_runtime = smgpc::resource::GameResourceRuntime{};
    auto runtime = smgpc::runtime::RuntimeContext(*logger, window, resource_runtime);
    {
        auto event_game_data = GameDataHolder(nullptr);
        auto event_game_data_binding =
            smgpc::compat::ScopedGameDataHolderOverride(event_game_data);
        test_event_value_utilities();
    }
    auto game_data = GameDataHolder(nullptr);
    auto game_data_binding =
        smgpc::compat::ScopedGameDataHolderOverride(game_data);
    auto stage_session = smgpc::compat::StageSessionState(
        "Game", "HeavensDoorGalaxy", 1, JMapIdInfo(0, 0));
    auto stage_session_binding =
        smgpc::compat::StageSessionBinding(stage_session);
    test_stage_session_already_done_registry();
    test_real_gateway_talk_runtime(runtime);

    std::cout << "Talk real-RMGK01 runtime and utility-provider tests passed\n";
    return 0;
}
