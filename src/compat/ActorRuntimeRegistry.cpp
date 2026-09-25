#include "Game/Demo/DemoDirector.hpp"
#include "Game/Demo/DemoExecutor.hpp"
#include "Game/NPC/TalkDirector.hpp"
#include <aurora/exception.hpp>
#include "Game/Screen/StarPointerTarget.hpp"
#include "Game/LiveActor/EffectKeeper.hpp"
#include "Game/AudioLib/AudAnmSoundObject.hpp"
#include "compat/ActorRuntimeRegistry.hpp"

#include "Game/LiveActor/ActorLightCtrl.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/ClippingDirector.hpp"
#include "Game/LiveActor/ClippingActorHolder.hpp"
#include "Game/LiveActor/ClippingActorInfo.hpp"
#include "Game/LiveActor/ClippingGroupHolder.hpp"
#include "Game/LiveActor/ViewGroupCtrl.hpp"
#include "Game/Util/JMapIdInfo.hpp"
#include "Game/Util/BaseMatrixFollowTargetHolder.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/HitSensorInfo.hpp"
#include "Game/LiveActor/HitSensorKeeper.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/LiveActorGroupArray.hpp"
#include "Game/LiveActor/AllLiveActorGroup.hpp"
#include "Game/LiveActor/LodCtrl.hpp"
#include "Game/LiveActor/RailRider.hpp"
#include "Game/LiveActor/Spine.hpp"
#include "Game/Map/StageSwitch.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/NameObj/NameObjGroup.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "resource/RarcArchive.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "Game/LiveActor/ActorAnimKeeper.hpp"
#include "Game/LiveActor/ActorPadAndCameraCtrl.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "runtime/RuntimeContext.hpp"
#include "resource/TextEncoding.hpp"
#include "Game/Scene/SceneObjHolder.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace {
    struct NameObjRuntimeState {
        std::shared_ptr<const std::string> host_name{};
        std::uint64_t registration_order = 0U;
        const void* owner = nullptr;
    };

    struct LiveActorRuntimeState {
        std::shared_ptr<ModelManager> model{};
        std::unique_ptr<ActorAnimKeeper> anim_keeper{};
        std::unique_ptr<ActorPadAndCameraCtrl> camera_ctrl{};
        std::shared_ptr<smgpc::compat::JkrAllocationDomain> sound_domain{};
        std::unique_ptr<AudAnmSoundObject> sound_object{};
        std::optional<smgpc::compat::ActorBinderRuntimeConfig> binder{};
        std::unique_ptr<Binder> binder_provider{};
        ClippingActorHolder* clipping_holder = nullptr;
        ClippingGroupHolder* clipping_groups = nullptr;
        std::unique_ptr<Spine> spine{};
        std::unique_ptr<RailRider> rail_rider{};
        std::unique_ptr<StageSwitchCtrl> stage_switch{};
        std::unique_ptr<ActorLightCtrl> light_ctrl{};
        std::unique_ptr<LodCtrl> lod_ctrl{};
    };

    [[nodiscard]] auto& name_obj_states() {
        static auto states = std::unordered_map<const NameObj*, NameObjRuntimeState>{};
        return states;
    }

    [[nodiscard]] auto& next_name_obj_registration_order() {
        static auto order = std::uint64_t{1U};
        return order;
    }

    [[nodiscard]] std::vector<NameObj*> snapshot_name_obj_runtime_objects_from(
        std::uint64_t first_registration_order) {
        smgpc::compat::JkrHostAllocationScope host;
        auto objects = std::vector<NameObj*>{};
        objects.reserve(name_obj_states().size());
        for (const auto& [object, state] : name_obj_states()) {
            if (state.registration_order >= first_registration_order) {
                objects.push_back(const_cast<NameObj*>(object));
            }
        }
        std::ranges::sort(objects, [](const NameObj* left, const NameObj* right) {
            return name_obj_states().at(left).registration_order <
                   name_obj_states().at(right).registration_order;
        });
        return objects;
    }

    [[nodiscard]] auto& actor_states() {
        static auto states = std::unordered_map<const LiveActor*, LiveActorRuntimeState>{};
        return states;
    }

    [[nodiscard]] LiveActorRuntimeState& require_actor_state(const LiveActor* actor) {
        if (actor == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("LiveActor runtime state requires a real actor.");
        }
        const auto found = actor_states().find(actor);
        if (found == actor_states().end()) {
            aurora::throw_host_exception<std::logic_error>("LiveActor has no registered native runtime state.");
        }
        return found->second;
    }


}  // namespace

namespace smgpc::compat {
    void retire_hit_sensor_borrows(const HitSensorKeeper* retiring) noexcept {
        if (!retiring) return;
        // Original message groups borrow the sender until their next movement.
        // Cancel a pending message before its source sensor is destroyed.
        for (const auto& [object, runtime_state] : name_obj_states()) {
            auto* group = dynamic_cast<MsgSharedGroup*>(const_cast<NameObj*>(object));
            if (!group || !group->mSensor) continue;
            for (s32 i = 0; i < retiring->mSensorInfosSize; ++i) {
                if (group->mSensor == retiring->mSensorInfos[i]->mSensor) {
                    group->mMsg = static_cast<u32>(-1);
                    group->mSensor = nullptr;
                    group->mSensorName = nullptr;
                    break;
                }
            }
        }
        // Native teardown can happen from an original attack callback. Remove
        // borrowed contacts before releasing the original sensor storage.
        for (const auto& [actor, state] : actor_states()) {
            auto* keeper = actor->mSensorKeeper;
            if (!keeper || keeper == retiring) continue;
            for (s32 j = 0; j < retiring->mSensorInfosSize; ++j) {
                auto* removed = retiring->mSensorInfos[j]->mSensor;
                if (keeper->mTaking == removed) keeper->mTaking = nullptr;
                if (keeper->mTaken == removed) keeper->mTaken = nullptr;
            }
            for (s32 i = 0; i < keeper->mSensorInfosSize; ++i) {
                auto* sensor = keeper->mSensorInfos[i]->mSensor;
                if (!sensor->mSensorCount) continue;
                for (s32 j = 0; j < retiring->mSensorInfosSize; ++j) {
                    auto* removed = retiring->mSensorInfos[j]->mSensor;
                    auto* end = std::remove(sensor->mSensors, sensor->mSensors + sensor->mSensorCount, removed);
                    std::fill(end, sensor->mSensors + sensor->mSensorCount, nullptr);
                    sensor->mSensorCount = static_cast<u16>(end - sensor->mSensors);
                }
            }
        }
    }

    void register_name_obj_runtime_state(NameObj* object) {
        JkrHostAllocationScope host;
        if (object == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("NameObj runtime state requires a real object.");
        }
        const auto registration_order = next_name_obj_registration_order()++;
        auto [found, inserted] = name_obj_states().try_emplace(
            object, NameObjRuntimeState{
                        .registration_order = registration_order,
                    });
        if (!inserted) {
            aurora::throw_host_exception<std::logic_error>("NameObj runtime state is already registered.");
        }
    }

    void retain_name_obj_host_name(NameObj* object, std::shared_ptr<const std::string> name) {
        JkrHostAllocationScope host;
        if (object == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("NameObj runtime state requires a real object.");
        }
        const auto found = name_obj_states().find(object);
        if (found == name_obj_states().end()) {
            aurora::throw_host_exception<std::logic_error>("NameObj has no registered native runtime state.");
        }
        found->second.host_name = std::move(name);
    }

    void release_name_obj_runtime_state(const NameObj* object) {
        // Groups borrow their members. Native factory rollback and object
        // retirement may leave the group alive after one member is deleted.
        // Scan actual groups so direct original registerObj calls are covered.
        for (const auto& [registered, state] : name_obj_states()) {
            if (registered == object) continue;
            if (auto* followers = dynamic_cast<BaseMatrixFollowTargetHolder*>(const_cast<NameObj*>(registered)))
                followers->releaseNativeReference(object);
            if (auto* talk = dynamic_cast<TalkDirector*>(const_cast<NameObj*>(registered)))
                talk->releaseNativeReference(object);
            if (auto* director = dynamic_cast<DemoDirector*>(const_cast<NameObj*>(registered)))
                director->releaseNativeReference(object);
            if (auto* executor = dynamic_cast<DemoExecutor*>(const_cast<NameObj*>(registered)))
                executor->releaseNativeReference(object);
            if (auto* array = dynamic_cast<LiveActorGroupArray*>(const_cast<NameObj*>(registered))) {
                auto& groups = array->mGroups;
                auto* old_end = groups.end();
                auto* new_end = std::remove_if(groups.begin(), old_end,
                    [object](const MsgSharedGroup* group) { return group == object; });
                std::fill(new_end, old_end, nullptr);
                groups.mCount = static_cast<s32>(new_end - groups.begin());
            }
            auto* group = dynamic_cast<NameObjGroup*>(const_cast<NameObj*>(registered));
            if (!group || group->mObjNum == 0) continue;
            auto* old_end = group->mObjArray + group->mObjNum;
            auto* new_end = std::remove(group->mObjArray, old_end, object);
            std::fill(new_end, old_end, nullptr);
            group->mObjNum = static_cast<s32>(new_end - group->mObjArray);
        }
        name_obj_states().erase(object);
    }

    bool has_name_obj_runtime_state(const NameObj* object) {
        return object != nullptr && name_obj_states().contains(object);
    }

    std::uint64_t name_obj_runtime_generation(
        const NameObj* object) noexcept {
        const auto found = name_obj_states().find(object);
        return found != name_obj_states().end()
                   ? found->second.registration_order
                   : 0U;
    }

    std::size_t name_obj_runtime_state_count() {
        return name_obj_states().size();
    }

    void claim_name_obj_runtime_ownership(NameObj* object,
                                          const void* owner) {
        if (object == nullptr || owner == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>(
                "NameObj runtime ownership requires real object and owner identities.");
        }
        const auto found = name_obj_states().find(object);
        if (found == name_obj_states().end()) {
            aurora::throw_host_exception<std::logic_error>(
                "NameObj runtime ownership requires a registered object.");
        }
        if (found->second.owner != nullptr) {
            aurora::throw_host_exception<std::logic_error>(
                "NameObj runtime ownership is already claimed.");
        }
        found->second.owner = owner;
    }

    bool name_obj_runtime_ownership_is_claimed(
        const NameObj* object) noexcept {
        const auto found = name_obj_states().find(object);
        return found != name_obj_states().end() &&
               found->second.owner != nullptr;
    }

    const void* name_obj_runtime_owner(const NameObj* object) noexcept {
        const auto found = name_obj_states().find(object);
        return found != name_obj_states().end() ? found->second.owner
                                                : nullptr;
    }


    std::vector<NameObj*> snapshot_name_obj_runtime_objects() {
        return snapshot_name_obj_runtime_objects_from(0U);
    }

    NameObjRuntimeRegistrationMarker mark_name_obj_runtime_registrations() {
        return NameObjRuntimeRegistrationMarker{
            .next_registration_order = next_name_obj_registration_order(),
        };
    }

    std::vector<NameObj*> snapshot_name_obj_runtime_objects_since(
        NameObjRuntimeRegistrationMarker marker) {
        if (marker.next_registration_order == 0U ||
            marker.next_registration_order > next_name_obj_registration_order()) {
            aurora::throw_host_exception<std::invalid_argument>("NameObj runtime registration marker is invalid.");
        }
        return snapshot_name_obj_runtime_objects_from(marker.next_registration_order);
    }

    NameObj* newest_name_obj_runtime_object_since_if(
        NameObjRuntimeRegistrationMarker marker,
        NameObjRuntimeRegistrationFilter filter,
        const void* context) noexcept {
        if (marker.next_registration_order == 0U ||
            marker.next_registration_order > next_name_obj_registration_order()) {
            return nullptr;
        }

        auto* newest = static_cast<const NameObj*>(nullptr);
        auto newest_order = std::uint64_t{};
        for (const auto& [object, state] : name_obj_states()) {
            if (state.registration_order < marker.next_registration_order ||
                state.registration_order <= newest_order ||
                (filter != nullptr && !filter(object, context))) {
                continue;
            }
            newest = object;
            newest_order = state.registration_order;
        }
        return const_cast<NameObj*>(newest);
    }

    bool name_obj_runtime_object_was_registered_since(
        const NameObj* object,
        NameObjRuntimeRegistrationMarker marker) noexcept {
        if (object == nullptr || marker.next_registration_order == 0U ||
            marker.next_registration_order > next_name_obj_registration_order()) {
            return false;
        }
        const auto found = name_obj_states().find(object);
        return found != name_obj_states().end() &&
               found->second.registration_order >=
                   marker.next_registration_order;
    }

    void destroy_name_obj_runtime_objects_since(
        NameObjRuntimeRegistrationMarker marker) noexcept {
        if (marker.next_registration_order == 0U) {
            return;
        }

        // Selecting the newest identity on each pass avoids allocating while
        // allowing each destructor to mutate the registry safely.
        while (auto* newest = newest_name_obj_runtime_object_since_if(
                   marker, nullptr, nullptr)) {
            delete newest;
        }
    }

    bool name_obj_is_suspended(const NameObj* object) {
        if (object == nullptr) {
            return true;
        }
        // Pending suspend/resume requests take effect only when the original
        // scene movement controller synchronizes the NameObj flags.
        return (object->mFlag & 1U) != 0U;
    }

    void register_actor_runtime_state(LiveActor* actor) {
        if (actor == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("LiveActor runtime state requires a real actor.");
        }
        {
            JkrHostAllocationScope host;
            if (!actor_states().try_emplace(actor).second) {
                aurora::throw_host_exception<std::logic_error>("LiveActor runtime state is already registered.");
            }
        }
        if (MR::getSceneObjHolder() != nullptr) {
            MR::getAllLiveActorGroup()->registerActor(actor);
            auto* director = MR::getClippingDirector();
            director->registerActor(actor);
            auto& state = require_actor_state(actor);
            state.clipping_holder = director->mActorHolder;
            state.clipping_groups = director->mGroupHolder;
        }
    }

    bool has_actor_runtime_state(const LiveActor* actor) {
        return actor != nullptr && actor_states().contains(actor);
    }

    std::size_t actor_runtime_state_count() {
        return actor_states().size();
    }

    void release_actor_runtime_state(const LiveActor* actor) {
        const auto found = actor_states().find(actor);
        if (found == actor_states().end()) {
            return;
        }

        auto& state = found->second;
        if (auto* holder = state.clipping_holder) {
            ClippingActorInfoList* lists[] = {holder->_10, holder->_14, holder->_18, holder->_1C};
            ClippingActorInfo* info = nullptr;
            for (auto* list : lists) {
                if (list->findOrNone(actor) != nullptr) {
                    info = list->remove(const_cast<LiveActor*>(actor));
                    --holder->_C;
                    break;
                }
            }
            if (info != nullptr) {
                if (auto* groups = state.clipping_groups) {
                    for (s32 i = 0; i < groups->mNumGroups; ++i) {
                        auto* group = groups->mInfoGroups[i];
                        for (s32 j = 0; j < group->_10; ++j) {
                            if (group->_14[j] == info) {
                                group->_14[j] = group->_14[--group->_10];
                                break;
                            }
                        }
                    }
                }
                delete info;
            }
            state.clipping_holder = nullptr;
            state.clipping_groups = nullptr;
        }
        delete actor->mEffectKeeper;
        const_cast<LiveActor*>(actor)->mEffectKeeper = nullptr;
        const_cast<LiveActor*>(actor)->releaseNativeCollisionParts();

        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->star_pointer().unregister_target(*actor);
            runtime->unregister_live_actor_model(*const_cast<LiveActor*>(actor));
        }

        delete actor->mStarPointerTarget;
        const_cast<LiveActor*>(actor)->mStarPointerTarget = nullptr;

        actor_states().erase(found);
    }

    void replace_actor_spine(LiveActor* actor, const Nerve* nerve) {
        auto& state = require_actor_state(actor);
        state.spine = std::make_unique<Spine>(actor, nerve);
        actor->mSpine = state.spine.get();
    }

    void update_actor_nerve(LiveActor* actor) {
        if (actor != nullptr && actor->mSpine != nullptr) {
            actor->mSpine->update();
        }
    }

    void replace_actor_rail_rider(LiveActor* actor, const JMapInfoIter& iter) {
        auto& state = require_actor_state(actor);
        state.rail_rider = std::make_unique<RailRider>(iter);
        actor->mRailRider = state.rail_rider.get();
    }

    void adopt_actor_stage_switch(LiveActor* actor, StageSwitchCtrl* controller) {
        auto& state = require_actor_state(actor);
        state.stage_switch.reset(controller);
        actor->mStageSwitchCtrl = state.stage_switch.get();
    }

    void replace_actor_light_ctrl(LiveActor* actor) {
        auto& state = require_actor_state(actor);
        state.light_ctrl = std::make_unique<ActorLightCtrl>(actor);
        actor->mActorLightCtrl = state.light_ctrl.get();
    }

    void adopt_actor_lod_ctrl(LiveActor* actor, LodCtrl* lod_ctrl) {
        if (lod_ctrl == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("LiveActor LOD ownership requires a real LodCtrl.");
        }
        auto& state = require_actor_state(actor);
        if (state.lod_ctrl != nullptr && state.lod_ctrl.get() != lod_ctrl) {
            aurora::throw_host_exception<std::logic_error>("LiveActor already owns a different LodCtrl.");
        }
        state.lod_ctrl.reset(lod_ctrl);
    }

    std::size_t actor_lod_ctrl_runtime_state_count() {
        return std::ranges::count_if(actor_states(), [](const auto& entry) {
            return entry.second.lod_ctrl != nullptr;
        });
    }

    void initialize_actor_model(LiveActor* actor, const char* model_archive,
                                const char* animation_archive, bool create_display_list) {
        auto& state = require_actor_state(actor);
        if (state.model) {
            aurora::throw_host_exception<std::logic_error>("Actor model replacement requires scene draw retirement first");
        }
        JkrHostAllocationScope host;
        auto* heap = JKRHeap::getCurrentHeap();
        if (!heap)
            aurora::throw_host_exception<std::logic_error>("Actor ModelManager requires the original caller's Game heap");
        auto owner = ModelManager::createNative(JkrAllocationDomain::retain_heap(*heap),
                                                        model_archive, animation_archive, create_display_list);
        actor->mModelManager = owner.get();
        state.model = std::move(owner);
    }

    std::shared_ptr<JkrAllocationDomain> actor_scene_allocation_domain(const LiveActor* actor) {
        const auto& state = require_actor_state(actor);
        if (state.model) return state.model->nativeAllocationDomain();
        auto* heap = JKRHeap::getCurrentHeap();
        if (!heap)
            aurora::throw_host_exception<std::logic_error>("Actor sound construction requires the original caller's Game heap");
        return JkrAllocationDomain::retain_heap(*heap);
    }

    void adopt_actor_sound_object(LiveActor* actor, std::shared_ptr<JkrAllocationDomain> domain) {
        auto& state = require_actor_state(actor);
        // Retain the old cohort until its captured old object has been destroyed.
        auto previous_domain = std::move(state.sound_domain);
        state.sound_domain = std::move(domain);
        state.sound_object.reset(actor->mSoundObject);
    }

    void adopt_actor_animation_helpers(LiveActor* actor) {
        auto& state = require_actor_state(actor);
        state.anim_keeper.reset(actor->mAnimKeeper);
        state.camera_ctrl.reset(actor->mCameraCtrl);
    }

    std::shared_ptr<ModelManager> retain_actor_model(const LiveActor* actor) {
        return require_actor_state(actor).model;
    }

    std::optional<std::span<const std::uint8_t>>
    actor_model_resource_data_if_present(const LiveActor* actor, std::string_view resource_name) {
        if (!actor || !actor->mModelManager || resource_name.empty()) return std::nullopt;
        const auto& archive = MR::getModelResourceHolder(actor)->nativeResourceSource();
        if (!archive.contains_resource(resource_name)) return std::nullopt;
        return archive.resource_data(resource_name);
    }

    void configure_actor_binder(LiveActor* actor, float radius, float offset, std::uint32_t plane_capacity) {
        auto& state = require_actor_state(actor);
        state.binder = ActorBinderRuntimeConfig{radius, offset, plane_capacity};
        state.binder_provider = std::make_unique<Binder>(
            actor->getBaseMtx(), &actor->mPosition, &actor->mGravity,
            radius, offset, plane_capacity);
        actor->mBinder = state.binder_provider.get();
    }

    void register_actor_binder(const LiveActor* actor) {
        if (actor != nullptr) {
            auto& state = require_actor_state(actor);
            state.binder.emplace();
            auto* mutable_actor = const_cast<LiveActor*>(actor);
            state.binder_provider = std::make_unique<Binder>(
                actor->getBaseMtx(), &mutable_actor->mPosition,
                &mutable_actor->mGravity, 0.0F, 0.0F, 0U);
            mutable_actor->mBinder = state.binder_provider.get();
        }
    }

    bool has_actor_binder(const LiveActor* actor) {
        const auto found = actor_states().find(actor);
        return found != actor_states().end() && found->second.binder.has_value();
    }

    const ActorBinderRuntimeConfig* actor_binder_config(const LiveActor* actor) {
        if (actor == nullptr) {
            return nullptr;
        }
        const auto found = actor_states().find(actor);
        if (found == actor_states().end()) {
            return nullptr;
        }
        const auto& binder = found->second.binder;
        return binder.has_value() ? &*binder : nullptr;
    }

    void release_actor_binder_state(const LiveActor* actor) {
        if (actor == nullptr) {
            return;
        }
        auto& state = require_actor_state(actor);
        const_cast<LiveActor*>(actor)->mBinder = nullptr;
        state.binder_provider.reset();
        state.binder.reset();
    }

    void retire_clipping_actor_holder(ClippingActorHolder& holder) noexcept {
        for (auto& [actor, state] : actor_states()) {
            if (state.clipping_holder == &holder) {
                state.clipping_holder = nullptr;
                state.clipping_groups = nullptr;
            }
        }
    }

    void retire_clipping_group_holder(ClippingGroupHolder& holder) noexcept {
        for (auto& [actor, state] : actor_states()) {
            if (state.clipping_groups == &holder) {
                state.clipping_groups = nullptr;
            }
        }
    }

}  // namespace smgpc::compat
