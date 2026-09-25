#include "Game/NameObj/NameObj.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "Game/LiveActor/HitSensorKeeper.hpp"
#include "Game/LiveActor/HitSensorInfo.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void test_name_storage_is_external_and_stable() {
        const auto baseline = smgpc::compat::name_obj_runtime_state_count();
        {
            auto source = std::array{'r', 'e', 't', 'a', 'i', 'l', '\0'};
            auto object = NameObj(source.data());
            require(smgpc::compat::name_obj_runtime_state_count() == baseline + 1U &&
                        smgpc::compat::has_name_obj_runtime_state(&object),
                    "constructing NameObj must register exactly one external name record");
            source[0] = 'X';
            require(std::string_view(object.getName()) == "retail",
                    "NameObj must not borrow mutable placement-name storage");

            auto replacement = std::string("replacement");
            object.setName(replacement.c_str());
            replacement.assign(128U, 'x');
            require(std::string_view(object.getName()) == "replacement",
                    "setName must update the external owner and refresh the retail pointer");

            object.requestSuspend();
            require(smgpc::compat::name_obj_is_suspended(&object),
                    "pending retail suspension must be visible to the native scheduler");
            object.syncWithFlags();
            object.requestResume();
            require(!smgpc::compat::name_obj_is_suspended(&object),
                    "pending retail resume must be visible to the native scheduler");
        }
        require(smgpc::compat::name_obj_runtime_state_count() == baseline,
                "NameObj destruction must release its external name record");
    }

    void test_actor_state_is_external_and_released() {
        const auto name_baseline = smgpc::compat::name_obj_runtime_state_count();
        const auto actor_baseline = smgpc::compat::actor_runtime_state_count();
        const LiveActor* stale_actor = nullptr;
        {
            auto actor = LiveActor("actor-registry-probe");
            stale_actor = &actor;
            require(smgpc::compat::name_obj_runtime_state_count() == name_baseline + 1U &&
                        smgpc::compat::actor_runtime_state_count() == actor_baseline + 1U &&
                        smgpc::compat::has_actor_runtime_state(&actor),
                    "LiveActor construction must register one name record and one actor record");

            actor.initModelManagerWithAnm("", "", false);
            actor.initHitSensor(2);
            auto bound_position = TVec3f{1.0F, 2.0F, 3.0F};
            auto* sensor = MR::addHitSensorPosEye(&actor, "eye", 2U, 10.0F, &bound_position, {});
            actor.initBinder(50.0F, 25.0F, 4U);
            smgpc::compat::configure_actor_clipping_sphere(&actor, 100.0F, nullptr);
            smgpc::compat::configure_actor_clipping_far_level(&actor, 3);

            const auto* binder = smgpc::compat::actor_binder_config(&actor);
            const auto* clipping = smgpc::compat::actor_clipping_runtime_state(&actor);
            require(smgpc::compat::actor_model(&actor) != nullptr && sensor != nullptr &&
                        actor.mSensorKeeper->mSensorInfosSize == 1 &&
                        actor.mSensorKeeper->getNthSensorInfo(0)->_18 == &bound_position && binder != nullptr &&
                        actor.mBinder != nullptr &&
                        binder->radius == 50.0F && binder->offset == 25.0F && binder->plane_capacity == 4U &&
                        clipping != nullptr && clipping->sphere_configured && clipping->sphere_radius == 100.0F &&
                        clipping->far_level == 3,
                    "the generalized record must retain model, animation, sensor, binder and clipping state");
            require(actor.mModelManager == nullptr && actor.mAnimKeeper == nullptr &&
                        actor.mShadowControllerList == nullptr,
                    "only a real exact provider may occupy a retail provider slot");
        }

        require(smgpc::compat::name_obj_runtime_state_count() == name_baseline &&
                    smgpc::compat::actor_runtime_state_count() == actor_baseline &&
                    !smgpc::compat::has_name_obj_runtime_state(stale_actor) &&
                    !smgpc::compat::has_actor_runtime_state(stale_actor) &&
                    smgpc::compat::actor_model(stale_actor) == nullptr &&
                    !smgpc::compat::has_actor_binder(stale_actor) &&
                    smgpc::compat::actor_clipping_runtime_state(stale_actor) == nullptr,
                "LiveActor destruction must remove every external record for the stale identity");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };
}  // namespace

int main() {
    constexpr auto tests = std::array{
        TestCase{"external NameObj name lifetime", test_name_storage_is_external_and_stable},
        TestCase{"external LiveActor state lifetime", test_actor_state_is_external_and_released},
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
    return failures == 0 ? 0 : 1;
}
