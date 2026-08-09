#include "Game/NameObj/NameObj.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/GameActorSensorCompat.hpp"
#include "scene/NameObjChildOwner.hpp"

#include <array>
#include <cstddef>
#include <exception>
#include <iostream>
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

    std::size_t byte_offset(const void* object, const void* member) {
        return static_cast<std::size_t>(static_cast<const std::byte*>(member) -
                                        static_cast<const std::byte*>(object));
    }

    void test_exact_native_game_layout() {
        static_assert(sizeof(void*) == 8U);
        static_assert(sizeof(NameObj) == 24U);
        static_assert(sizeof(LiveActor) == 208U);

        auto actor = LiveActor("layout-probe");
        require(byte_offset(&actor, &actor.mName) == 8U &&
                    byte_offset(&actor, &actor.mFlag) == 144U &&
                    byte_offset(&actor, &actor.mPosition) == 20U &&
                    byte_offset(&actor, &actor.mModelManager) == 80U &&
                    byte_offset(&actor, &actor.mCameraCtrl) == 200U,
                "native LiveActor must contain only the pointer-width-adjusted retail field sequence");
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
            auto* shadow = smgpc::compat::actor_shadow_runtime_state(&actor);
            shadow->valid = true;

            const auto* binder = smgpc::compat::actor_binder_config(&actor);
            const auto* clipping = smgpc::compat::actor_clipping_runtime_state(&actor);
            require(smgpc::compat::actor_model(&actor) != nullptr && sensor != nullptr &&
                        smgpc::compat::actor_hit_sensor_count(&actor) == 1U &&
                        smgpc::compat::actor_sensor_binding_count(&actor) == 1U && binder != nullptr &&
                        actor.mBinder != nullptr &&
                        binder->radius == 50.0F && binder->offset == 25.0F && binder->plane_capacity == 4U &&
                        clipping != nullptr && clipping->sphere_configured && clipping->sphere_radius == 100.0F &&
                        clipping->far_level == 3 && shadow->valid,
                    "the generalized record must retain model, animation, sensor, binder, clipping, and shadow state");
            require(actor.mModelManager == nullptr && actor.mAnimKeeper == nullptr &&
                        actor.mSensorKeeper == nullptr &&
                        actor.mShadowControllerList == nullptr,
                    "only a real exact provider may occupy a retail provider slot");
        }

        require(smgpc::compat::name_obj_runtime_state_count() == name_baseline &&
                    smgpc::compat::actor_runtime_state_count() == actor_baseline &&
                    !smgpc::compat::has_name_obj_runtime_state(stale_actor) &&
                    !smgpc::compat::has_actor_runtime_state(stale_actor) &&
                    smgpc::compat::actor_model(stale_actor) == nullptr &&
                    smgpc::compat::actor_hit_sensor_count(stale_actor) == 0U &&
                    smgpc::compat::actor_sensor_binding_count(stale_actor) == 0U &&
                    !smgpc::compat::has_actor_binder(stale_actor) &&
                    smgpc::compat::actor_clipping_runtime_state(stale_actor) == nullptr &&
                    smgpc::compat::actor_shadow_runtime_state(stale_actor) == nullptr,
                "LiveActor destruction must remove every external record for the stale identity");
    }

    class CapturedNameObj final : public NameObj {
    public:
        CapturedNameObj(int identity, std::vector<int> &destruction_order)
            : NameObj("construction-capture-probe"), _identity(identity),
              _destruction_order(&destruction_order) {
        }

        ~CapturedNameObj() override {
            _destruction_order->push_back(_identity);
        }

    private:
        int _identity;
        std::vector<int> *_destruction_order;
    };

    void test_construction_child_capture_is_ordered_and_exception_safe() {
        const auto baseline = smgpc::compat::name_obj_runtime_state_count();
        auto destruction_order = std::vector<int>{};
        auto owner = smgpc::scene::NameObjChildOwner{};
        auto construction_rejected = false;

        try {
            owner.capture_construction_children([&] {
                (void)new CapturedNameObj(1, destruction_order);
                (void)new CapturedNameObj(2, destruction_order);
                throw std::runtime_error("intentional construction failure");
            });
        } catch (const std::runtime_error &error) {
            construction_rejected =
                std::string_view(error.what()) ==
                "intentional construction failure";
        }

        require(construction_rejected && owner.size() == 2U &&
                    smgpc::compat::name_obj_runtime_state_count() == baseline + 2U,
                "construction capture did not adopt live raw-new children while preserving the original exception");
        owner.clear();
        require(destruction_order == std::vector<int>{2, 1} &&
                    smgpc::compat::name_obj_runtime_state_count() == baseline,
                "construction capture did not retire children in reverse registration order");
    }

    void test_construction_child_capture_rejects_nested_ownership() {
        const auto baseline = smgpc::compat::name_obj_runtime_state_count();
        auto destruction_order = std::vector<int>{};
        auto outer = smgpc::scene::NameObjChildOwner{};
        auto inner = smgpc::scene::NameObjChildOwner{};
        auto nested_rejected = false;

        outer.capture_construction_children([&] {
            (void)new CapturedNameObj(1, destruction_order);
            try {
                inner.capture_construction_children([&] {
                    (void)new CapturedNameObj(99, destruction_order);
                });
            } catch (const std::logic_error &) {
                nested_rejected = true;
            }
            (void)new CapturedNameObj(2, destruction_order);
        });

        require(nested_rejected && outer.size() == 2U && inner.empty() &&
                    smgpc::compat::name_obj_runtime_state_count() ==
                        baseline + 2U,
                "nested construction capture was not rejected before exposing children to two owners");
        outer.clear();
        require(destruction_order == std::vector<int>{2, 1} &&
                    smgpc::compat::name_obj_runtime_state_count() == baseline,
                "the outer capture did not remain ordered after rejecting a nested capture");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };
}  // namespace

int main() {
    constexpr auto tests = std::array{
        TestCase{"exact native Game layout", test_exact_native_game_layout},
        TestCase{"external NameObj name lifetime", test_name_storage_is_external_and_stable},
        TestCase{"external LiveActor state lifetime", test_actor_state_is_external_and_released},
        TestCase{"ordered exception-safe construction child capture",
                 test_construction_child_capture_is_ordered_and_exception_safe},
        TestCase{"construction child capture rejects nested ownership",
                 test_construction_child_capture_rejects_nested_ownership},
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
