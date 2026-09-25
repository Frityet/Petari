#include "NativeHeapFixture.hpp"
#include "SceneExecutionFixture.hpp"
#include "OriginalSceneControllerFixture.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/LiveActorGroup.hpp"
#include "Game/NameObj/NameObjGroup.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Scene/SceneNameObjMovementController.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/NameObj/NameObj.hpp"
#include <aurora/exception.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
void membership() {
    NameObjGroup first("FirstGroup", 8), second("SecondGroup", 8);
    NameObj left("Left"), right("Right");
    auto retired = std::make_unique<NameObj>("Retired");
    first.registerObj(&left); first.registerObj(retired.get());
    first.registerObj(&right); first.registerObj(retired.get()); first.registerObj(&left);
    second.registerObj(retired.get()); second.registerObj(&right);
    MR::joinToNameObjGroup(retired.get(), "SecondGroup");
    retired.reset();
    require(first.getObjNum() == 3 && first.getObj(0) == &left && first.getObj(1) == &right &&
            first.getObj(2) == &left && !first.getObj(3) && !first.getObj(4),
            "direct original registrations compact all retiring duplicates without changing survivor order and clear vacated slots");
    require(second.getObjNum() == 1 && second.getObj(0) == &right && !second.getObj(1) && !second.getObj(2),
            "both direct and MR registrations retire from every actual group");
    left.requestSuspend(); left.syncWithFlags(); right.requestSuspend(); right.syncWithFlags();
    first.pauseOffAll();
    require((left.getFlag() & 1) && (right.getFlag() & 1),
            "original pauseOffAll requests resume without bypassing the scene's deferred flag phase");
    MR::getSceneNameObjMovementController()->movement();
    require(!(left.getFlag() & 1) && !(right.getFlag() & 1), "original pauseOffAll only visits retained live members");
    auto transient_group = std::make_unique<NameObjGroup>("EarlierGroup", 2);
    transient_group->registerObj(&left);
    transient_group.reset();
}
void derived_group() {
    LiveActorGroup group("ActualDerivedGroup", 4);
    auto actor = std::make_unique<LiveActor>("ActualActor");
    group.registerActor(actor.get()); group.registerActor(actor.get());
    actor.reset();
    require(group.getObjNum() == 0 && !group.getObj(0) && !group.getObj(1),
            "original LiveActorGroup direct registrations share actual NameObjGroup retirement");
}

}
int main() {
    try {
        auto heaps = smgpc::test::create_native_root_heap(16U << 20);
        smgpc::runtime::SceneScheduler scheduler;
        smgpc::runtime::SceneSchedulerBinding scheduler_binding(scheduler);
        smgpc::test::OriginalSceneControllerFixture original(heaps);
        const auto baseline = NameObj::snapshotNativeObjects().size();
        for (int cycle = 0; cycle < 32; ++cycle) {
            {
                smgpc::test::SceneExecutionFixture scene(
                    scheduler, smgpc::test::create_native_solid_heap(heaps, 1U << 20),
                    &original.scene);
                membership(); derived_group();
                scene.complete_initialization();
            }
            require(NameObj::snapshotNativeObjects().size() == baseline,
                    "repeated real group/member ownership returns to the registry baseline");
        }
        std::cout << "Original group membership lifetime passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
