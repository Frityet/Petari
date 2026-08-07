#include "Game/Enemy/AnimScaleController.hpp"
#include "Game/NPC/NPCActor.hpp"
#include "Game/NPC/NPCActorItem.hpp"
#include "Game/Util/ActorSensorUtil.hpp"

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace MR {
    bool getNPCItemData(NPCActorItem*, s32);
    bool checkPlayerSwingTrigger();
    void addToAttributeGroupSearchTurtle(const LiveActor*);
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

    auto item = NPCActorItem("TestNpc");
    requireUnavailable([&] { (void)MR::getNPCItemData(&item, 0); },
                       "missing NPC item-table data must be explicitly unavailable");
    requireUnavailable([] { (void)MR::checkPlayerSwingTrigger(); },
                       "missing real MarioActor swing state must be explicitly unavailable");
    requireUnavailable([&] { MR::addToAttributeGroupSearchTurtle(&actor); },
                       "missing GroupCheckManager ownership must be explicitly unavailable");
    requireUnavailable([] {
        auto parameter = AnimScaleParam{};
        auto controller = AnimScaleController(&parameter);
        (void)controller;
    }, "missing J3D joint-controller support must reject AnimScale construction");
    ++passed;

    std::cout << "NPCActor real-or-absent tests passed: " << passed << "/4\n";
    return 0;
}
