#include "Game/LiveActor/AllLiveActorGroup.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "SceneExecutionFixture.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"

#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
    void require(bool condition, const char* message) {
        if (!condition) {
            smgpc::compat::JkrHostAllocationScope host;
            throw std::runtime_error(message);
        }
    }

    struct Receipt {
        LiveActor* actor;
        u32 message;
        HitSensor* sender;
        HitSensor* receiver;
    };

    class Receiver final : public LiveActor {
    public:
        Receiver(const char* name, std::vector<Receipt>& receipts) : LiveActor(name), _receipts(receipts) {
            // The fixture controls only liveness; no scheduler/model is
            // substituted for the original global actor-group membership.
            mFlag.mIsDead = false;
        }
        bool receiveMessage(u32 message, HitSensor* sender, HitSensor* receiver) override {
            _receipts.push_back({this, message, sender, receiver});
            if (on_message) on_message();
            return accepts;
        }
        std::function<void()> on_message;
        bool accepts = true;
    private:
        std::vector<Receipt>& _receipts;
    };

    void verify_broadcast() {
        auto heaps = smgpc::compat::JkrHeapRuntime::create(16U << 20);
        auto scheduler = smgpc::runtime::SceneScheduler{};
        auto scheduler_binding = smgpc::runtime::SceneSchedulerBinding(scheduler);
        const auto baseline = smgpc::compat::name_obj_runtime_state_count();
        for (int generation = 0; generation < 2; ++generation) {
            std::weak_ptr<smgpc::compat::JkrAllocationDomain> retired;
            auto receipts = std::vector<Receipt>{};
            receipts.reserve(64);
            {
                auto domain = smgpc::compat::JkrAllocationDomain::create(heaps, 1U << 20);
                retired = domain;
                auto scene = smgpc::test::SceneExecutionFixture(scheduler, domain);
                auto game = smgpc::compat::JkrAllocationScope(domain);
                auto* group = MR::getAllLiveActorGroup();
                require(group != nullptr && group->getObjNum() == 0,
                        "each scene starts with an actual empty AllLiveActorGroup");
                const auto scene_registration_count = scheduler.snapshot().size();
                Receiver ordinary("ordinary", receipts);
                Receiver suspended("suspended", receipts);
                Receiver clipped("clipped", receipts);
                Receiver dead("dead", receipts);
                Receiver excluded("excluded", receipts);
                NameObj non_actor("not an actor");
                suspended.requestSuspend();
                suspended.syncWithFlags();
                clipped.mFlag.mIsClipped = true;
                dead.mFlag.mIsDead = true;
                ordinary.accepts = false;
                require(group->getObjNum() == 5 && group->getActor(0) == &ordinary &&
                            group->getActor(4) == &excluded && scheduler.snapshot().size() == scene_registration_count,
                        "construction registers every actor in order without an execution registration");
                require(smgpc::compat::name_obj_is_suspended(&suspended),
                        "the suspended recipient must have the actual synchronized NameObj flag");

                MR::sendMsgToAllLiveActor(ACTMES_START_DEMO, &excluded);
                require(receipts.size() == 3 && receipts[0].actor == &ordinary &&
                            receipts[1].actor == &suspended && receipts[2].actor == &clipped,
                        "original broadcasts include suspended/clipped actors and exclude only dead actors and the sender");
                require(receipts[0].sender == nullptr && receipts[0].receiver == nullptr,
                        "an absent message sensor stays absent without suppressing original dispatch");
                receipts.clear();
                require(MR::createSceneObj(SceneObj_MessageSensorHolder) != nullptr,
                        "the scene creates its actual MessageSensorHolder explicitly");
                auto* sensor = MR::getMessageSensor();
                require(sensor != nullptr && group->getObjNum() == 6,
                        "the original message-sensor actor also joins the actual group");

                std::unique_ptr<Receiver> appended;
                ordinary.on_message = [&] {
                    dead.mFlag.mIsDead = false;
                    clipped.mFlag.mIsDead = true;
                    if (!appended) appended = std::make_unique<Receiver>("appended", receipts);
                };
                MR::sendMsgToAllLiveActor(ACTMES_END_DEMO, &excluded);
                require(receipts.size() == 4 && receipts[0].actor == &ordinary &&
                            receipts[1].actor == &suspended && receipts[2].actor == &dead &&
                            receipts[3].actor == appended.get(),
                        "broadcasts reread current liveness and group count, including actors appended by a receiver");
                for (const auto& receipt : receipts)
                    require(receipt.message == ACTMES_END_DEMO && receipt.sender == sensor && receipt.receiver == sensor,
                            "every recipient receives the requested message and original shared sensor twice");
                require(group->getObjNum() == 7, "append keeps a single original membership");
                ordinary.on_message = nullptr;
                appended.reset();
                require(group->getObjNum() == 6,
                        "native actor retirement removes its borrowed pointer from the still-live original group");

                auto victim = std::make_unique<Receiver>("retired before dispatch", receipts);
                receipts.clear();
                ordinary.on_message = [&] { victim.reset(); };
                MR::sendMsgToAllLiveActor(ACTMES_END_DEMO, &excluded);
                require(!victim && receipts.size() == 3 && group->getObjNum() == 6,
                        "removing a future recipient leaves the live group traversable without a stale pointer");
                ordinary.on_message = nullptr;
            }
            require(retired.expired() && smgpc::compat::name_obj_runtime_state_count() == baseline &&
                        scheduler.snapshot().empty(),
                    "scene retirement releases its group, actors, registrations and Game heap");
        }
    }
}

int main() try {
    verify_broadcast();
    std::cout << "Original actor broadcasts passed in two scene generations\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "[fail] original actor broadcast: " << error.what() << '\n';
    return 1;
}
