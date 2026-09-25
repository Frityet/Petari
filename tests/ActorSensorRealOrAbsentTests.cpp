#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/HitSensorInfo.hpp"
#include "Game/LiveActor/HitSensorKeeper.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "Game/Player/MarioMessenger.hpp"
#include "Game/NPC/NPCActor.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "runtime/SceneScheduler.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/MessageSensorHolder.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/LiveActor/ClippingDirector.hpp"
#include "Game/LiveActor/SensorHitChecker.hpp"
#include "SceneExecutionFixture.hpp"
#include "OriginalSceneControllerFixture.hpp"

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

    void test_npc_sensor_capability_owns_body_registration() {
        NPCActor actor("NPC optional sensor fixture");
        NPCActorCaps caps("NPC optional sensor fixture");
        caps.mSwitchDead = false;
        actor.initialize(JMapInfoIter(), caps);
        require(actor.mSensorKeeper == nullptr,
                "disabled NPC sensor capability must skip both keeper and Body sensor creation");

        caps.mSensor = true;
        caps.mSensorMax = 2;
        actor.initialize(JMapInfoIter(), caps);
        require(actor.mSensorKeeper != nullptr && actor.mSensorKeeper->mSensorCount == 2 &&
                    actor.mSensorKeeper->mSensorInfosSize == 1 && actor.getSensor("Body")->isType(ATYPE_NPC),
                "enabled NPC capability must initialize the requested keeper and original Body sensor together");
    }

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
        require(actor.mSensorKeeper->getSensorInfo("matrix")->_1C == matrix,
                "the original HitSensorInfo must retain the supplied matrix pointer");
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
        require(actor.mSensorKeeper->mSensorInfosSize == 0,
                "reinitializing the keeper must release its old sensor infos");
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

    void test_optional_matrix_uses_original_actor_relative_path() {
        auto actor = LiveActor("optional matrix sensor");
        actor.mPosition.set(10.0F, 20.0F, 30.0F);
        actor.initHitSensor(1);
        auto* sensor = MR::addHitSensorMtxEnemy(&actor, "body", 1U, 5.0F, nullptr, TVec3f(1, 2, 3));
        require(sensor != nullptr && actor.mSensorKeeper->getSensorInfo("body")->_1C == nullptr,
                "the original optional matrix must reach HitSensorInfo without a substitute binding");
        require_vec(sensor->mPosition, TVec3f(11, 22, 33),
                    "HitSensorInfo's original null-matrix path uses the actor position and offset");
    }

    void test_message_sensor_is_owned_by_the_active_scene() {
        require(!MR::isExistSceneObj(SceneObj_MessageSensorHolder),
                "the fixture must explicitly create the original message sensor owner");
        auto* object = MR::createSceneObj(SceneObj_MessageSensorHolder);
        auto* message_holder = dynamic_cast<MessageSensorHolder*>(object);
        require(message_holder != nullptr,
                "explicit SceneObj creation must instantiate the original MessageSensorHolder");
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
                    callback->mSensorCount == 0 &&
                    callback->mSensorGroup == MR::getSceneObj<SensorHitChecker>(SceneObj_SensorHitChecker)->mPlayerGroup,
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
        actor.mSensorKeeper->mTaking = other;
        actor.mSensorKeeper->mTaken = other;
        receiver.reset();
        require(sensor->mSensorCount == 0 && sensor->mSensors[0] == nullptr &&
                    actor.mSensorKeeper->mTaking == nullptr && actor.mSensorKeeper->mTaken == nullptr,
                "native retirement must clear borrowed contacts and take links before releasing sensor storage");
        actor.movement();
        require(actor.attacks == 1, "retired contacts must never be dispatched");
    }

    void test_keeper_and_original_messenger_game_heap_lifetime() {
        auto& scheduler = *smgpc::runtime::try_active_scene_scheduler();
        auto domain = scheduler.allocation_domain();
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

    void test_original_sensor_distance_and_direction() {
        LiveActor first("first distance owner"), second("second distance owner");
        first.initHitSensor(1);
        second.initHitSensor(1);
        auto* a = MR::addHitSensorEnemy(&first, "body", 0, 100, TVec3f(0, 0, 0));
        auto* b = MR::addHitSensorEnemy(&second, "body", 0, 200, TVec3f(0, 0, 0));
        a->mPosition.set(10, -5, 2);
        b->mPosition.set(13, -1, 2);
        TVec3f direction(9, 9, 9);
        require(MR::calcDistance(a, b, &direction) == 5, "sensor distance must use centers without subtracting radii");
        require_vec(direction, TVec3f(0.6F, 0.8F, 0), "direction must point from the first sensor toward the second");
        require(MR::calcDistance(b, a, nullptr) == 5, "the original direction output is optional");
        require(MR::calcDistance(a, b, &b->mPosition) == 5, "distance must support an output alias of a sensor position");
        require_vec(b->mPosition, direction, "the original difference snapshot must precede output writes");
        b->mPosition = a->mPosition;
        require(MR::calcDistance(a, b, &direction) == 0, "coincident sensor centers must return zero distance");
        require_vec(direction, TVec3f(0, 0, 0), "coincident centers must clear the direction");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };

}  // namespace

int main() {
    constexpr auto tests = std::array{
        TestCase{"NPC optional sensor capability", test_npc_sensor_capability_owns_body_registration},
        TestCase{"original sensor center distance", test_original_sensor_distance_and_direction},
        TestCase{"original keeper callbacks, offsets and validity", test_original_keeper_callbacks_offsets_and_validity},
        TestCase{"original contact delivery and retirement", test_original_contact_delivery_and_retirement},
        TestCase{"original keeper and messenger arena lifetime", test_keeper_and_original_messenger_game_heap_lifetime},
        TestCase{"actor-relative registration", test_actor_relative_registration_is_real},
        TestCase{"matrix binding", test_matrix_binding_tracks_the_supplied_matrix},
        TestCase{"position binding", test_position_binding_tracks_the_supplied_position},
        TestCase{"original optional matrix semantics", test_optional_matrix_uses_original_actor_relative_path},
        TestCase{"scene-owned message sensor", test_message_sensor_is_owned_by_the_active_scene},
        TestCase{"arbitrary message dispatch", test_arbitrary_message_dispatch_is_real_or_absent},
    };

    auto failures = 0;
    for (const auto& test : tests) {
        try {
            auto heaps = smgpc::compat::JkrHeapRuntime::create(16U << 20);
            const auto root_free = heaps->root_heap().getFreeSize();
            const auto names = smgpc::compat::name_obj_runtime_state_count();
            const auto actors = smgpc::compat::actor_runtime_state_count();
            {
                smgpc::test::OriginalSceneControllerFixture original(heaps);
                auto domain = smgpc::compat::JkrAllocationDomain::create(heaps, 4U << 20);
                smgpc::runtime::SceneScheduler scheduler;
                smgpc::runtime::SceneSchedulerBinding active(scheduler);
                smgpc::test::SceneExecutionFixture execution(scheduler, domain,
                                                          &original.scene, original.controller().mObjHolder);
                smgpc::compat::JkrAllocationScope game(domain);
                auto* clipping = static_cast<ClippingDirector*>(MR::createSceneObj(SceneObj_ClippingDirector));
                scheduler.disconnect_name_obj(*clipping);
                execution.complete_initialization();
                test.run();
            }
            require(heaps->root_heap().getFreeSize() == root_free &&
                        smgpc::compat::name_obj_runtime_state_count() == names &&
                        smgpc::compat::actor_runtime_state_count() == actors,
                    "each original scene must retire all registered sensor owners and its Game arenas");
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
