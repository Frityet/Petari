#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/NameObj/NameObjExecuteHolder.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "runtime/SceneScheduler.hpp"

#include <array>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    template <typename Base>
    class PhaseProbe final : public Base {
    public:
        explicit PhaseProbe(const char *name) : Base(name) {}

        void movement() override { ++movements; }
        void calcAnim() override { ++animations; }
        void calcViewAndEntry() override { ++views; }
        void draw() const override { ++draws; }

        int movements = 0;
        int animations = 0;
        int views = 0;
        mutable int draws = 0;
    };

    using NameProbe = PhaseProbe<NameObj>;
    using ActorProbe = PhaseProbe<LiveActor>;

    void execute_phases(smgpc::runtime::SceneScheduler &scheduler) {
        scheduler.execute_movement();
        scheduler.execute_calc_anim();
        scheduler.execute_calc_view_and_entry();
        scheduler.execute_draw_type(MR::DrawType_CinemaFrame);
    }

    void connect_probe(smgpc::runtime::SceneScheduler &scheduler,
                       NameProbe &probe, s32 movement) {
        scheduler.connect_name_obj(probe, movement, MR::CalcAnimType_NPC,
                                   -1, MR::DrawType_CinemaFrame);
    }

    void test_category_selectivity_and_resume() {
        auto scheduler = smgpc::runtime::SceneScheduler{};
        auto binding = smgpc::runtime::SceneSchedulerBinding{scheduler};
        auto npc = NameProbe{"npc"};
        auto player = NameProbe{"player"};
        auto decoration = NameProbe{"decoration"};
        connect_probe(scheduler, npc, MR::MovementType_NPC);
        connect_probe(scheduler, player, MR::MovementType_Player);
        connect_probe(scheduler, decoration, MR::MovementType_PlayerDecoration);

        CategoryList::requestMovementOff(MR::MovementType_Player);
        execute_phases(scheduler);
        require(npc.movements == 1 && player.movements == 0 && decoration.movements == 1,
                "category-off must affect only the exact registered movement category");
        require(player.animations == 1 && player.views == 1 && player.draws == 1,
                "movement-off must retain the separate animation, view, and draw lists");

        CategoryList::requestMovementOn(MR::MovementType_Player);
        execute_phases(scheduler);
        require(npc.movements == 2 && player.movements == 1 && decoration.movements == 2,
                "category-on must resume the same objects without rebuilding registration");
    }

    void test_actor_movement_pause_preserves_visual_callbacks() {
        auto scheduler = smgpc::runtime::SceneScheduler{};
        auto binding = smgpc::runtime::SceneSchedulerBinding{scheduler};
        auto actor = ActorProbe{"actor"};
        actor.makeActorAppeared();
        scheduler.register_live_actor_model(actor, MR::MovementType_NPC,
                                            MR::CalcAnimType_NPC,
                                            MR::DrawBufferType_NPC,
                                            MR::DrawType_CinemaFrame);
        CategoryList::requestMovementOff(MR::MovementType_NPC);
        execute_phases(scheduler);
        require(actor.movements == 0 && actor.animations == 1 && actor.views == 1 && actor.draws == 1,
                "a paused LiveActor must still reach its virtual animation, view, and draw callbacks");

        scheduler.disconnect_draw(actor);
        execute_phases(scheduler);
        require(actor.movements == 0 && actor.animations == 2 && actor.draws == 1,
                "temporary draw disconnection must remain independent from movement suspension");
        scheduler.connect_draw(actor);
        actor.makeActorDead();
        execute_phases(scheduler);
        require(actor.animations == 2 && actor.views == 2 && actor.draws == 1,
                "movement-off must not weaken the existing dead-actor exclusion");
        actor.makeActorAppeared();
        CategoryList::requestMovementOn(MR::MovementType_NPC);
        execute_phases(scheduler);
        require(actor.movements == 1 && actor.animations == 3 && actor.views == 3 && actor.draws == 2,
                "an appeared and resumed actor must recover every independent phase");
    }

    void test_category_requests_preserve_retail_signed_byte_identity() {
        auto scheduler = smgpc::runtime::SceneScheduler{};
        auto binding = smgpc::runtime::SceneSchedulerBinding{scheduler};
        auto npc = NameProbe{"narrowed-category"};
        connect_probe(scheduler, npc, MR::MovementType_NPC);
        MR::requestMovementOffWithCategory(0x100 + MR::MovementType_NPC);
        scheduler.execute_movement();
        require(npc.movements == 0,
                "category lookup must narrow the request like retail NameObjExecuteInfo's s8 field");
        MR::requestMovementOnWithCategory(-0x100 + MR::MovementType_NPC);
        scheduler.execute_movement();
        require(npc.movements == 1,
                "signed-byte aliases must use the same resume path");
    }

    void test_requests_apply_to_current_registration_without_category_latch() {
        auto scheduler = smgpc::runtime::SceneScheduler{};
        auto binding = smgpc::runtime::SceneSchedulerBinding{scheduler};
        auto first = NameProbe{"existing"};
        auto later = NameProbe{"later"};
        connect_probe(scheduler, first, MR::MovementType_NPC);
        CategoryList::requestMovementOff(MR::MovementType_NPC);
        connect_probe(scheduler, later, MR::MovementType_NPC);
        scheduler.execute_movement();
        require(first.movements == 0 && later.movements == 1,
                "retail category requests visit current objects and must not latch a future category policy");

        scheduler.disconnect_name_obj(first);
        CategoryList::requestMovementOn(MR::MovementType_NPC);
        require((first.getFlag() & 1U) != 0U,
                "a removed registration must not remain in category traversal");
        connect_probe(scheduler, first, MR::MovementType_NPC);
        scheduler.execute_movement();
        require(first.movements == 0 && later.movements == 2,
                "reconnecting must preserve the actor's existing movement flag");
        CategoryList::requestMovementOn(MR::MovementType_NPC);
        scheduler.execute_movement();
        require(first.movements == 1 && later.movements == 3,
                "a subsequent category-on must include the reconnected actor");
    }

    void test_nested_scene_binding_isolation() {
        auto outer = smgpc::runtime::SceneScheduler{};
        auto inner = smgpc::runtime::SceneScheduler{};
        auto outer_actor = NameProbe{"outer"};
        auto inner_actor = NameProbe{"inner"};
        connect_probe(outer, outer_actor, MR::MovementType_NPC);
        connect_probe(inner, inner_actor, MR::MovementType_NPC);
        auto outer_binding = smgpc::runtime::SceneSchedulerBinding{outer};
        {
            auto inner_binding = smgpc::runtime::SceneSchedulerBinding{inner};
            CategoryList::requestMovementOff(MR::MovementType_NPC);
            outer.execute_movement();
            inner.execute_movement();
            require(outer_actor.movements == 1 && inner_actor.movements == 0,
                    "a bound scene request must not suspend actors in the enclosing scene");
        }
        CategoryList::requestMovementOff(MR::MovementType_NPC);
        outer.execute_movement();
        require(outer_actor.movements == 1,
                "nested scene teardown must restore the exact previous scheduler binding");
    }

    void test_missing_owner_fails_explicitly() {
        for (const auto &operation : std::array<std::function<void()>, 2>{
                 [] { CategoryList::requestMovementOn(MR::MovementType_NPC); },
                 [] { MR::requestMovementOffWithCategory(MR::MovementType_NPC); }}) {
            auto rejected = false;
            try {
                operation();
            } catch (const std::logic_error &) {
                rejected = true;
            }
            require(rejected, "a category request without its scene scheduler must fail explicitly");
        }
    }
}  // namespace

int main() {
    try {
        test_missing_owner_fails_explicitly();
        test_category_selectivity_and_resume();
        test_actor_movement_pause_preserves_visual_callbacks();
        test_category_requests_preserve_retail_signed_byte_identity();
        test_requests_apply_to_current_registration_without_category_latch();
        test_nested_scene_binding_isolation();
        std::cout << "[ok] scene movement category runtime: 6/6\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "[fail] scene movement category runtime: " << error.what() << '\n';
        return 1;
    }
}
