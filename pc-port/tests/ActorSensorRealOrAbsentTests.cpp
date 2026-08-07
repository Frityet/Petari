#include "Game/LiveActor/HitSensor.hpp"
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
        actor.updateHitSensors();
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
        actor.updateHitSensors();
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
        actor.updateHitSensors();
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

    struct TestCase {
        std::string_view name;
        void (*run)();
    };

}  // namespace

int main() {
    constexpr auto tests = std::array{
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
