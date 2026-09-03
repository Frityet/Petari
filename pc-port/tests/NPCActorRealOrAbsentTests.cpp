#include "Game/Enemy/AnimScaleController.hpp"
#include "Game/NPC/NPCActor.hpp"
#include "Game/NPC/NPCActorItem.hpp"
#include "Game/NPC/TalkMessageCtrl.hpp"
#include "Game/NPC/TalkNodeCtrl.hpp"
#include "Game/Player/GroupChecker.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/NPCUtil.hpp"
#include "Logger.hpp"
#include "RendererService.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/GroupCheckManagerCompat.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace MR {
    bool getNPCItemData(NPCActorItem*, s32);
    bool checkPlayerSwingTrigger();
}  // namespace MR

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void requireUnavailable(const std::function<void()>& operation, std::string_view message) {
        auto unavailable = false;
        try {
            operation();
        } catch (const std::logic_error&) {
            unavailable = true;
        }
        require(unavailable, message);
    }

    void requireInvalid(const std::function<void()>& operation, std::string_view message) {
        auto invalid = false;
        try {
            operation();
        } catch (const std::invalid_argument&) {
            invalid = true;
        }
        require(invalid, message);
    }

    void requireNear(float actual, float expected, std::string_view message) {
        require(std::abs(actual - expected) < 0.00001F, message);
    }

    class DirectFloatBaseProbe final : public NPCActor {
    public:
        DirectFloatBaseProbe() : NPCActor("direct float-base probe") {
        }

        void calcAndSetBaseMtx() override {
            ++virtualCalls;
            NPCActor::calcAndSetBaseMtx();
        }

        int virtualCalls = 0;
    };

    void testFloatOffsetAndBaseMatrix() {
        auto actor = DirectFloatBaseProbe{};
        requireNear(MR::calcFloatOffset(&actor, 8.0F, 150.0F), 7.5F,
                    "an NPC float offset must decay by exactly 0.5 per frame");
        requireNear(MR::calcFloatOffset(&actor, 0.25F, 150.0F), 0.0F,
                    "an NPC float offset must clamp a negative decay to zero");
        requireNear(MR::calcFloatOffset(
                        &actor, std::numeric_limits<float>::quiet_NaN(), 150.0F),
                    0.0F,
                    "a NaN float offset must take the retail comparison's zero branch");
        requireInvalid([] { static_cast<void>(MR::calcFloatOffset(nullptr, 0.0F, 1.0F)); },
                       "a null NPC must fail the float-offset host contract explicitly");

        actor.mPosition.set(10.0F, 20.0F, 30.0F);
        actor._A0.set(0.5F, 0.25F, -0.75F, 2.0F);
        const auto original = actor.mPosition;
        auto rawY = TVec3f{};
        actor._A0.getYDir(rawY);
        constexpr auto offset = 3.0F;
        const auto floated = original + rawY * offset;
        MR::calcAndSetFloatBaseMtx(&actor, offset);
        const auto& matrix = smgpc::compat::actor_base_matrix(&actor);
        require(actor.virtualCalls == 0,
                "float base calculation must directly dispatch NPCActor::calcAndSetBaseMtx");
        requireNear(actor.mPosition.x, original.x,
                    "float base calculation must restore NPC position X");
        requireNear(actor.mPosition.y, original.y,
                    "float base calculation must restore NPC position Y");
        requireNear(actor.mPosition.z, original.z,
                    "float base calculation must restore NPC position Z");
        requireNear(matrix.m[3], floated.x,
                    "float base matrix must capture raw quaternion-Y offset X");
        requireNear(matrix.m[7], floated.y,
                    "float base matrix must capture raw quaternion-Y offset Y");
        requireNear(matrix.m[11], floated.z,
                    "float base matrix must capture raw quaternion-Y offset Z");
        requireInvalid([] { MR::calcAndSetFloatBaseMtx(nullptr, 1.0F); },
                       "a null NPC must fail the float-base host contract explicitly");

        auto logger = smgpc::logging::create_default_logger();
        auto window = smgpc::render::AuroraWindow({
            .width = 640,
            .height = 456,
            .title = "SMG PC NPC float-offset proof",
        });
        auto runtime = smgpc::runtime::RuntimeContext(*logger, window);
        auto holder = SceneObjHolder{};
        auto holderBinding = smgpc::scene::SceneObjHolderBinding(holder);
        require(holder.create(SceneObj_TalkDirector) != nullptr,
                "the talk-near float proof requires the scene-owned TalkRuntime");

        auto player = LiveActor("NPC float-offset player");
        player.mPosition.set(0.0F, 100.0F, 0.0F);
        player.calcAndSetBaseMtx();
        runtime.player_system().attach_actor(player);
        actor.mPosition.zero();
        {
            auto controller = TalkMessageCtrl(&actor, TVec3f{}, nullptr);
            require(controller.mNodeCtrl != nullptr,
                    "the scene-owned TalkRuntime must provide the controller node state");
            controller._18 = 3U;
            actor.mMsgCtrl = &controller;

            requireNear(MR::calcFloatOffset(&actor, 0.0F, 150.0F), 5.5F,
                        "the first talk-near rise must use the retail 5.5 cap");
            requireNear(MR::calcFloatOffset(&actor, 20.0F, 150.0F), 25.0F,
                        "later talk-near rise must cap at decayed+5.5");

            player.mPosition.y = 200.0F;
            runtime.player_system().synchronize_attached_actor();
            requireNear(MR::calcFloatOffset(&actor, 20.0F, 150.0F), 19.5F,
                        "talk-near interpolation must exclude the strict 200-unit boundary");
            player.mPosition.y = -50.0F;
            runtime.player_system().synchronize_attached_actor();
            requireNear(MR::calcFloatOffset(&actor, 20.0F, 150.0F), 19.5F,
                        "talk-near interpolation must require positive player-up separation");

            player.mPosition.y = 100.0F;
            runtime.player_system().synchronize_attached_actor();
            controller.mNodeCtrl->mMessageInfo.mTalkType = 1U;
            requireNear(MR::calcFloatOffset(&actor, 20.0F, 150.0F), 19.5F,
                        "short talk must retain decay without the float rise");
            controller.mNodeCtrl->mMessageInfo.mTalkType = 0U;
            controller._18 = 0U;
            requireNear(MR::calcFloatOffset(&actor, 20.0F, 150.0F), 19.5F,
                        "non-talking message state must retain decay without the float rise");
            actor.mMsgCtrl = nullptr;
        }
        runtime.player_system().detach_actor(&player);
    }
}  // namespace

int main() {
    auto passed = 0;

    auto caps = NPCActorCaps("TestNpc");
    require(!caps.mModel && !caps.mMakeActor && !caps.mLodCtrl && !caps.mShadow,
            "retail NPC caps must begin disabled instead of silently enabling host substitutes");
    caps.setDefault();
    require(caps.mModel && caps.mMakeActor && caps.mLodCtrl && caps.mShadow && caps.mMessage,
            "retail NPC default caps must retain the decompiled defaults");
    ++passed;

    auto actor = NPCActor("npc-reaction-test");
    require(actor.receiveMsgPlayerAttack(ACTMES_PLAYER_TRAMPLE, nullptr, nullptr),
            "the exact NPCActor source must accept the first trample reaction");
    require(!actor.receiveMsgPlayerAttack(ACTMES_PLAYER_TRAMPLE, nullptr, nullptr),
            "the exact NPCActor source must retain its trample cooldown");
    actor.updateReaction();
    require(actor._D8 && !actor._E2, "the exact NPCActor reaction edge must be consumed once");
    actor.updateReaction();
    require(!actor._D8, "the exact NPCActor reaction edge must clear on the next update");
    ++passed;

    auto transform = TPos3f{};
    transform.identity();
    transform.setTrans(12.0F, -4.0F, 30.0F);
    actor.setBaseMtx(transform);
    require(actor.mPosition.x == 12.0F && actor.mPosition.y == -4.0F && actor.mPosition.z == 30.0F,
            "NPCActor must consume a real matrix transform without a host-only replacement body");
    ++passed;

    const auto manager_baseline = smgpc::compat::group_check_manager_runtime_state_count();
    const auto checker_baseline = smgpc::compat::group_checker_runtime_state_count();
    const auto membership_baseline = smgpc::compat::attribute_group_membership_count();
    {
        auto holder = SceneObjHolder{};
        auto binding = smgpc::scene::SceneObjHolderBinding(holder);

        requireUnavailable([&] { MR::addToAttributeGroupSearchTurtle(&actor); },
                           "attribute membership must not fabricate a missing scene-owned manager");
        require(holder.getObj(SceneObj_GroupCheckManager) == nullptr,
                "a failed membership request must leave the required pre-placement SceneObj absent");
        require(MR::createSceneObj(SceneObj_GroupCheckManager) != nullptr,
                "the focused scene fixture must explicitly create its GroupCheckManager");

        MR::addToAttributeGroupSearchTurtle(&actor);
        MR::addToAttributeGroupSearchTurtle(&actor);
        require(MR::isExistInAttributeGroupSearchTurtle(&actor),
                "SearchTurtle membership must be queryable through the scene-owned GroupCheckManager");
        require(!MR::isExistInAttributeGroupReflectSpinningBox(&actor),
                "SearchTurtle membership must not leak into the ReflectSpinningBox group");
        require(smgpc::compat::attribute_group_membership_count() == membership_baseline + 1U,
                "attribute group insertion must be idempotent for the same NameObj name");

        auto same_name = NPCActor("npc-reaction-test");
        require(MR::isExistInAttributeGroupSearchTurtle(&same_name),
                "GroupChecker lookup must use NameObj names like the retail HashSortTable");

        auto second = NPCActor("npc-second-search-target");
        MR::addToAttributeGroupSearchTurtle(&second);
        require(MR::isExistInAttributeGroupSearchTurtle(&second),
                "multiple actors must coexist in the same SearchTurtle group");

        auto reflected = NPCActor("npc-reflected-target");
        MR::addToAttributeGroupReflectSpinningBox(&reflected);
        require(MR::isExistInAttributeGroupReflectSpinningBox(&reflected) &&
                    !MR::isExistInAttributeGroupSearchTurtle(&reflected),
                "the two retail attribute groups must retain independent membership");

        const auto before_transient = smgpc::compat::attribute_group_membership_count();
        {
            auto transient = NPCActor("npc-transient-search-target");
            MR::addToAttributeGroupSearchTurtle(&transient);
            require(smgpc::compat::attribute_group_membership_count() == before_transient + 1U,
                    "a newly inserted NameObj name must contribute one group membership");
        }
        auto same_transient_name = NPCActor("npc-transient-search-target");
        require(smgpc::compat::attribute_group_membership_count() == before_transient + 1U &&
                    MR::isExistInAttributeGroupSearchTurtle(&same_transient_name),
                "name membership must live with the scene-owned checker, not an individual actor identity");
        require(smgpc::compat::group_check_manager_runtime_state_count() == manager_baseline + 1U &&
                    smgpc::compat::group_checker_runtime_state_count() == checker_baseline + 2U,
                "one scene manager must own exactly the two retail group checkers");
    }
    require(smgpc::compat::group_check_manager_runtime_state_count() == manager_baseline &&
                smgpc::compat::group_checker_runtime_state_count() == checker_baseline &&
                smgpc::compat::attribute_group_membership_count() == membership_baseline,
            "scene teardown must release both group checkers and every remaining membership");
    requireInvalid([] { MR::addToAttributeGroupSearchTurtle(nullptr); },
                   "null attribute-group insertion must remain an explicit contract error");
    ++passed;

    auto item = NPCActorItem("TestNpc");
    requireUnavailable([&] { (void)MR::getNPCItemData(&item, 0); },
                       "missing NPC item-table data must be explicitly unavailable");
    requireUnavailable([] { (void)MR::checkPlayerSwingTrigger(); },
                       "missing real MarioActor swing state must be explicitly unavailable");
    {
        auto parameter = AnimScaleParam{};
        auto controller = AnimScaleController(&parameter);
        controller.update();
        require(controller._C.y == 1.0F, "upstream scale controller must remain at rest until triggered");
    }
    ++passed;

    testFloatOffsetAndBaseMatrix();
    ++passed;

    std::cout << "NPCActor real-or-absent tests passed: " << passed << "/6\n";
    return 0;
}
