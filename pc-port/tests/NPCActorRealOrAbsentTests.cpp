#include "Game/Enemy/AnimScaleController.hpp"
#include "Game/NPC/NPCActor.hpp"
#include "Game/NPC/NPCActorItem.hpp"
#include "Game/Player/GroupChecker.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "compat/GroupCheckManagerCompat.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

#include <functional>
#include <iostream>
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
    requireUnavailable([] {
        auto parameter = AnimScaleParam{};
        auto controller = AnimScaleController(&parameter);
        (void)controller;
    }, "missing J3D joint-controller support must reject AnimScale construction");
    ++passed;

    std::cout << "NPCActor real-or-absent tests passed: " << passed << "/5\n";
    return 0;
}
