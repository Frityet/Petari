#include "SceneExecutionFixture.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/LiveActorGroup.hpp"
#include "Game/NameObj/NameObjGroup.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Scene/SceneNameObjMovementController.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
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
struct FactoryContext { bool fail = true; };
NameObj* factory(int id, void* opaque) {
    if (id != SceneObj_MiiFacePartsHolder) return nullptr;
    auto* child = new NameObj("FactoryMember");
    MR::joinToNameObjGroup(child, "IgnorePauseNameObj");
    if (static_cast<FactoryContext*>(opaque)->fail)
        aurora::throw_host_exception<std::runtime_error>("fixture failure after original group registration");
    return child;
}
void factory_rollback(smgpc::test::SceneExecutionFixture& scene, FactoryContext& context) {
    auto& holder = scene.holder();
    auto* group = dynamic_cast<NameObjGroup*>(MR::createSceneObj(SceneObj_NameObjGroup));
    require(group, "the actual scene owns its original group");
    const auto baseline = smgpc::compat::name_obj_runtime_state_count();
    bool failed = false;
    try { MR::createSceneObj(SceneObj_MiiFacePartsHolder); }
    catch (const std::runtime_error&) { failed = true; }
    require(failed && !holder.getObj(SceneObj_MiiFacePartsHolder) && group->getObjNum() == 0 &&
            !group->getObj(0) && smgpc::compat::name_obj_runtime_state_count() == baseline,
            "actual captured factory rollback removes borrowed membership before the surviving scene continues");
    group->pauseOffAll();
    context.fail = false;
    auto* child = MR::createSceneObj(SceneObj_MiiFacePartsHolder);
    require(child && group->getObjNum() == 1 && group->getObj(0) == child,
            "the same scene successfully creates and owns a later group member");
}
}
int main() {
    try {
        auto heaps = smgpc::compat::JkrHeapRuntime::create(16U << 20);
        smgpc::runtime::SceneScheduler scheduler;
        smgpc::runtime::SceneSchedulerBinding scheduler_binding(scheduler);
        const auto baseline = smgpc::compat::name_obj_runtime_state_count();
        for (int cycle = 0; cycle < 32; ++cycle) {
            {
                FactoryContext context;
                smgpc::test::SceneExecutionFixture scene(
                    scheduler, smgpc::compat::JkrAllocationDomain::create(heaps, 1U << 20), factory, &context);
                membership(); derived_group(); factory_rollback(scene, context);
                scene.complete_initialization();
                scene.objects().complete_initialization();
            }
            require(smgpc::compat::name_obj_runtime_state_count() == baseline,
                    "repeated real group/member/factory ownership returns to the registry baseline");
        }
        std::cout << "Original group membership and captured factory rollback lifetime passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
