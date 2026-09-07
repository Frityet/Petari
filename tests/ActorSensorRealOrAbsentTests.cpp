#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/HitSensorInfo.hpp"
#include "Game/LiveActor/HitSensorKeeper.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "Game/Player/MarioMessenger.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "runtime/SceneScheduler.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/MessageSensorHolder.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "compat/GameActorSensorCompat.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

#include <array>
#include <exception>
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

    void require_vec(const TVec3f& actual, const TVec3f& expected, std::string_view message) {
        require(actual.epsilonEquals(expected, 0.00001F),
                std::string(message) + ": actual=(" + std::to_string(actual.x) + "," +
                    std::to_string(actual.y) + "," + std::to_string(actual.z) + ")");
    }

    class RecordingActor final : public LiveActor {
    public:
        explicit RecordingActor(const char* name) : LiveActor(name) {
        }

        bool receiveMessage(u32 message, HitSensor* sender, HitSensor* receiver) override {
            last_message = message;
            last_sender = sender;
            last_receiver = receiver;
            ++message_count;
            return true;
        }

        u32 last_message = 0U;
        HitSensor* last_sender = nullptr;
        HitSensor* last_receiver = nullptr;
        int message_count = 0;
    };

    void test_actor_relative_registration_is_real() {
        auto actor = LiveActor("actor-relative-sensor");
        actor.mPosition.set(10.0F, 20.0F, 30.0F);
        actor.initHitSensor(2);

        auto* ordinary = MR::addHitSensorEnemy(&actor, "Catch", 8U, 50.0F, TVec3f{1.0F, 2.0F, 3.0F});
        require(ordinary != nullptr, "a valid actor-relative sensor must be created");
        require(ordinary->mType == ATYPE_ENEMY,
                "the sensor name Catch must not trigger a fabricated host-only type heuristic");
        require(ordinary->mGroupSize == 8U && ordinary->mRadius == 50.0F,
                "real group size and radius must be retained");
        require_vec(ordinary->mPosition, TVec3f{11.0F, 22.0F, 33.0F},
                    "actor-relative registration must use the actor position and requested offset");

        actor.mPosition.set(-5.0F, 6.0F, 7.0F);
        MR::updateHitSensorsAll(&actor);
        require_vec(ordinary->mPosition, TVec3f{-4.0F, 8.0F, 10.0F},
                    "actor-relative sensor position must update with its actor");
    }

    void test_matrix_binding_tracks_the_supplied_matrix() {
        auto actor = LiveActor("matrix-sensor");
        actor.initHitSensor(1);
        Mtx matrix = {
            {0.0F, -1.0F, 0.0F, 10.0F},
            {1.0F, 0.0F, 0.0F, 20.0F},
            {0.0F, 0.0F, 1.0F, 30.0F},
        };

        auto* sensor = MR::addHitSensorMtxEnemy(&actor, "matrix", 4U, 25.0F, matrix, TVec3f{2.0F, 3.0F, 4.0F});
        require(sensor != nullptr && sensor->mType == ATYPE_ENEMY,
                "a valid matrix-bound enemy sensor must be created with its retail type");
        require(smgpc::compat::actor_sensor_binding_count(&actor) == 1U,
                "the compatibility layer must own the one non-actor binding");
        require_vec(sensor->mPosition, TVec3f{7.0F, 22.0F, 34.0F},
                    "matrix-bound offset must be transformed by the supplied matrix");

        actor.mPosition.set(900.0F, 900.0F, 900.0F);
        matrix[0][3] = 40.0F;
        matrix[1][3] = 50.0F;
        matrix[2][3] = 60.0F;
        MR::updateHitSensorsAll(&actor);
        require_vec(sensor->mPosition, TVec3f{37.0F, 52.0F, 64.0F},
                    "matrix-bound sensor must track matrix changes instead of falling back to the actor");

        actor.initHitSensor(1);
        require(smgpc::compat::actor_sensor_binding_count(&actor) == 0U,
                "reinitializing actor sensors must release compatibility binding state");
    }

    void test_position_binding_tracks_the_supplied_position() {
        auto actor = LiveActor("position-sensor");
        actor.initHitSensor(1);
        auto position = TVec3f{3.0F, 4.0F, 5.0F};
        auto* sensor = MR::addHitSensorPosEye(&actor, "eye", 2U, 10.0F, &position, TVec3f{1.0F, 2.0F, 3.0F});
        require(sensor != nullptr && sensor->mType == ATYPE_EYE,
                "a valid external-position sensor must retain its requested retail type");
        require_vec(sensor->mPosition, TVec3f{4.0F, 6.0F, 8.0F},
                    "position-bound sensor must use the requested world position");

        actor.mPosition.set(100.0F, 100.0F, 100.0F);
        position.set(-2.0F, -4.0F, -6.0F);
        MR::updateHitSensorsAll(&actor);
        require_vec(sensor->mPosition, TVec3f{-1.0F, -2.0F, -3.0F},
                    "position-bound sensor must not silently switch to actor-relative placement");
    }

    void test_missing_matrix_and_joint_remain_absent() {
        auto actor = LiveActor("absent-joint-sensor");
        actor.mPosition.set(10.0F, 20.0F, 30.0F);
        actor.calcAndSetBaseMtx();
        actor.initHitSensor(4);

        require(MR::addHitSensorMtxEnemy(&actor, "missing-matrix", 1U, 5.0F, nullptr, {}) == nullptr,
                "a missing matrix must not create an origin- or actor-bound sensor");
        require(MR::addHitSensorAtJointEnemy(&actor, "missing-joint", "HandR", 1U, 5.0F, {}) == nullptr,
                "an unavailable named J3D joint must not fall back to the actor base matrix");
        require(MR::addHitSensorAtJointEnemy(&actor, "empty-joint", "", 1U, 5.0F, {}) == nullptr,
                "an empty joint name must remain unavailable");
        require(MR::addHitSensorAtJointEnemy(nullptr, "missing-actor", "HandR", 1U, 5.0F, {}) == nullptr,
                "a missing actor must not manufacture a joint sensor");
        require(actor.getSensor("missing-matrix") == nullptr && actor.getSensor("missing-joint") == nullptr &&
                    actor.getSensor("empty-joint") == nullptr,
                "rejected bindings must not leave ordinary fallback sensors behind");
        require(smgpc::compat::actor_sensor_binding_count(&actor) == 0U,
                "rejected bindings must not leave compatibility state behind");
    }

    void test_message_sensor_is_owned_by_the_active_scene() {
        require(MR::getMessageSensor() == nullptr,
                "no active scene must mean no process-global fabricated message sensor");

        {
            auto holder = SceneObjHolder{};
            const auto binding = smgpc::scene::SceneObjHolderBinding(holder);
            require(MR::getMessageSensor() == nullptr,
                    "binding a scene holder must not implicitly fabricate its message sensor");

            auto* object = MR::createSceneObj(SceneObj_MessageSensorHolder);
            auto* message_holder = dynamic_cast<MessageSensorHolder*>(object);
            require(message_holder != nullptr,
                    "explicit SceneObj creation must instantiate the real retail MessageSensorHolder");
            auto* sensor = MR::getMessageSensor();
            require(sensor != nullptr && sensor == message_holder->getSensor("body"),
                    "the message API must return the scene-owned holder's actual body sensor");
            require(sensor->mHost == message_holder && sensor->mType == ATYPE_MESSAGE_SENSOR,
                    "the message sensor must retain its real host and retail type");

            auto receiver = RecordingActor("message-receiver");
            require(MR::sendSimpleMsgToActor(ACTMES_START_DEMO, &receiver),
                    "simple actor messaging must dispatch through the real scene sensor");
            require(receiver.last_sender == sensor && receiver.last_receiver == sensor,
                    "both message endpoints must be the active scene-owned message sensor");
        }

        require(MR::getMessageSensor() == nullptr,
                "destroying the scene must remove its message sensor instead of leaking a static replacement");
    }

    void test_arbitrary_message_dispatch_is_real_or_absent() {
        auto sender_actor = LiveActor("sender");
        sender_actor.initHitSensor(1);
        auto* sender = MR::addHitSensor(&sender_actor, "body", ATYPE_PLAYER, 1U, 1.0F, {});

        auto receiver_actor = RecordingActor("receiver");
        receiver_actor.initHitSensor(1);
        auto* receiver = MR::addHitSensor(&receiver_actor, "body", ATYPE_NPC, 1U, 1.0F, {});

        require(MR::sendArbitraryMsg(ACTMES_PUSH_FORCE, receiver, sender),
                "a concrete receiver must receive the requested message");
        require(receiver_actor.message_count == 1 && receiver_actor.last_message == ACTMES_PUSH_FORCE &&
                    receiver_actor.last_sender == sender && receiver_actor.last_receiver == receiver,
                "message dispatch must preserve the concrete sender and receiver");
        require(!MR::sendArbitraryMsg(ACTMES_PUSH_FORCE, nullptr, sender),
                "an absent receiver must return false instead of reporting a fabricated delivery");
    }

    class KeeperActor final : public LiveActor {
    public:
        KeeperActor() : LiveActor("original keeper fixture") {}
        MtxPtr getBaseMtx() const override { return has_matrix ? const_cast<MtxPtr>(matrix) : nullptr; }
        void updateHitSensor(HitSensor* sensor) override {
            require(mSensorKeeper && mSensorKeeper->getSensorInfo("callback")->mSensor == sensor,
                    "callback registration must publish the original info before calling its host");
            ++callbacks;
            sensor->mPosition.set(7, 8, 9);
        }
        void attackSensor(HitSensor* sender, HitSensor* receiver) override {
            ++attacks;
            last_sender = sender;
            last_receiver = receiver;
        }
        void control() override { attacks_at_control = attacks; }
        bool receiveMessage(u32 message, HitSensor* sender, HitSensor* receiver) override {
            require(received < messages.size(), "messenger sent beyond its actual 32-entry capacity");
            messages[received++] = message;
            last_sender = sender;
            last_receiver = receiver;
            return true;
        }
        bool has_matrix = true;
        Mtx matrix = {{0, -2, 0, 100}, {3, 0, 0, 200}, {0, 0, 4, 300}};
        unsigned callbacks = 0, attacks = 0, attacks_at_control = 0, received = 0;
        std::array<u32, 32> messages{};
        HitSensor* last_sender = nullptr;
        HitSensor* last_receiver = nullptr;
    };

    void test_original_keeper_callbacks_offsets_and_validity() {
        KeeperActor actor;
        actor.mPosition.set(10, 20, 30);
        actor.initHitSensor(3);
        auto* ordinary = MR::addHitSensorEnemy(&actor, "ordinary", 4, 8, TVec3f(1, 2, 3));
        require(actor.mSensorKeeper && actor.mSensorKeeper->mSensorInfosSize == 1 &&
                    actor.mSensorKeeper->mSensorCount == 3,
                "LiveActor must own the actual original keeper and capacities");
        require(actor.getSensor("any-name") == ordinary,
                "the original single-sensor shortcut must remain observable");
        require_vec(ordinary->mPosition, TVec3f(6, 23, 42),
                    "actor-relative offset must use base matrix rotation and scale without its translation");
        TVec3f external(40, 50, 60);
        auto* positioned = MR::addHitSensorPosEye(&actor, "position", 2, 3, &external, TVec3f(1, 2, 3));
        require_vec(positioned->mPosition, TVec3f(36, 53, 72),
                    "external-position offset must also use the actor's base matrix");
        auto* callback = MR::addHitSensorCallback(&actor, "callback", ATYPE_PLAYER, 4, 5);
        require(actor.callbacks == 1 && callback->mValidByHost && !callback->mValidBySystem &&
                    callback->mSensorCount == 0 && callback->mSensorGroup == nullptr,
                "original callback registration and HitSensor initial state must be deterministic");
        require_vec(callback->mPosition, TVec3f(7, 8, 9), "callback result must not be overwritten by an offset fallback");
        ordinary->invalidate();
        actor.makeActorAppeared();
        require(!ordinary->mValidByHost && ordinary->mValidBySystem && callback->isValid(),
                "appearance must change only system validity and preserve explicit host invalidation");
        const auto before = actor.callbacks;
        actor.movement();
        require(actor.callbacks == before + 1, "ordinary movement must update each callback once after control");
        actor.startClipped();
        require(!ordinary->mValidBySystem && !callback->mValidBySystem, "clipping must invalidate through the system");
        actor.endClipped();
        require(!ordinary->mValidByHost && ordinary->mValidBySystem && callback->isValid(),
                "unclipping must preserve explicit host invalidation");
        actor.makeActorDead();
        require(!ordinary->mValidBySystem && !callback->mValidBySystem && callback->mValidByHost,
                "death must preserve each host's validity decision");
        actor.has_matrix = false;
        MR::setSensorOffset(&actor, "ordinary", TVec3f(3, 2, 1));
        MR::updateHitSensorsAll(&actor);
        require_vec(ordinary->mPosition, TVec3f(13, 22, 31), "null original base matrix must use its explicit direct-offset branch");
        require_vec(positioned->mPosition, TVec3f(41, 52, 63), "position binding must retain the original null-base branch");
    }

    void test_original_contact_delivery_and_retirement() {
        KeeperActor actor;
        actor.initHitSensor(1);
        auto* sensor = MR::addHitSensorEnemy(&actor, "body", 4, 8, {});
        auto receiver = std::make_unique<KeeperActor>();
        receiver->initHitSensor(1);
        auto* other = MR::addHitSensorEnemy(receiver.get(), "body", 4, 8, {});
        actor.makeActorAppeared();
        receiver->makeActorAppeared();
        sensor->addHitSensor(other);
        actor.movement();
        require(actor.attacks == 1 && actor.attacks_at_control == 1 && actor.last_sender == sensor && actor.last_receiver == other,
                "the original keeper must deliver contacts before control with unchanged endpoints");
        receiver->makeActorDead();
        sensor->addHitSensor(other);
        actor.movement();
        require(actor.attacks == 1, "the original info must skip receivers that have died");
        receiver.reset();
        require(sensor->mSensorCount == 0, "native owner retirement must remove borrowed contacts before deleting sensor storage");
        actor.movement();
        require(actor.attacks == 1, "retired contacts must never be dispatched");
    }

    void test_keeper_and_original_messenger_game_heap_lifetime() {
        using namespace smgpc::compat;
        auto heaps = JkrHeapRuntime::create(2U << 20);
        const auto baseline = heaps->root_heap().getFreeSize();
        {
            auto domain = JkrAllocationDomain::create(heaps, 64U << 10);
            smgpc::runtime::SceneScheduler scheduler;
            smgpc::runtime::SceneSchedulerBinding scheduler_binding(scheduler);
            smgpc::runtime::SceneSchedulerAllocationBinding scene_domain(scheduler, domain);
            JkrAllocationScope game(domain);
            KeeperActor sender, receiver;
            for (auto* actor : {&sender, &receiver}) {
                actor->initHitSensor(1);
                auto* sensor = MR::addHitSensorEnemy(actor, "body", 4, 8, {});
                auto* keeper = actor->mSensorKeeper;
                auto* info = keeper->getNthSensorInfo(0);
                require(JKRHeap::findFromRoot(keeper) == &domain->heap() &&
                            JKRHeap::findFromRoot(keeper->mSensorInfos) == &domain->heap() &&
                            JKRHeap::findFromRoot(info) == &domain->heap() &&
                            JKRHeap::findFromRoot(sensor) == &domain->heap() &&
                            JKRHeap::findFromRoot(sensor->mSensors) == &domain->heap(),
                        "all original keeper allocations must remain in the caller's Game arena");
            }
            MarioMessenger messenger(sender.getSensor("body"));
            for (u32 i = 0; i < 35; ++i) messenger.addRequest(receiver.getSensor("body"), 1000 + i);
            scheduler.execute_movement();
            require(receiver.received == 32, "the actual messenger must cap requests at 32 and execute from its registered category");
            for (u32 i = 0; i < 32; ++i) require(receiver.messages[i] == 1000 + i, "messenger must retain FIFO message order");
            require(receiver.last_sender == sender.getSensor("body") && receiver.last_receiver == receiver.getSensor("body"),
                    "messenger must preserve actual sensor endpoints");
            scheduler.execute_movement();
            require(receiver.received == 32, "messenger movement must clear its queue after delivery");
        }
        require(heaps->root_heap().getFreeSize() == baseline,
                "original keeper and messenger storage must retire with their Game arena");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };

}  // namespace

int main() {
    constexpr auto tests = std::array{
        TestCase{"original keeper callbacks, offsets and validity", test_original_keeper_callbacks_offsets_and_validity},
        TestCase{"original contact delivery and retirement", test_original_contact_delivery_and_retirement},
        TestCase{"original keeper and messenger arena lifetime", test_keeper_and_original_messenger_game_heap_lifetime},
        TestCase{"actor-relative registration", test_actor_relative_registration_is_real},
        TestCase{"matrix binding", test_matrix_binding_tracks_the_supplied_matrix},
        TestCase{"position binding", test_position_binding_tracks_the_supplied_position},
        TestCase{"missing matrix and joint", test_missing_matrix_and_joint_remain_absent},
        TestCase{"scene-owned message sensor", test_message_sensor_is_owned_by_the_active_scene},
        TestCase{"arbitrary message dispatch", test_arbitrary_message_dispatch_is_real_or_absent},
    };

    auto failures = 0;
    for (const auto& test : tests) {
        try {
            test.run();
            std::cout << "[ok] " << test.name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[fail] " << test.name << ": " << error.what() << '\n';
        }
    }

    if (failures != 0) {
        std::cerr << failures << " ActorSensor real-or-absent test(s) failed\n";
        return 1;
    }

    std::cout << tests.size() << " ActorSensor real-or-absent tests passed\n";
    return 0;
}
