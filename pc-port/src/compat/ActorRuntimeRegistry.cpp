#include "Game/AudioLib/AudAnmSoundObject.hpp"
#include "compat/ActorRuntimeRegistry.hpp"

#include "Game/LiveActor/ActorLightCtrl.hpp"
#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/LodCtrl.hpp"
#include "Game/LiveActor/RailRider.hpp"
#include "Game/LiveActor/Spine.hpp"
#include "Game/Map/StageSwitch.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "compat/CollisionPartsCompat.hpp"
#include "compat/GameActorSensorCompat.hpp"
#include "compat/ModelManagerOwner.hpp"
#include "compat/ResourceHolderCompat.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "Game/LiveActor/ActorAnimKeeper.hpp"
#include "Game/LiveActor/ActorPadAndCameraCtrl.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "runtime/RuntimeContext.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace {
    struct NameObjRuntimeState {
        std::string name{};
        std::uint64_t registration_order = 0U;
        const void* owner = nullptr;
        const void* postpass_delegate = nullptr;
    };

    struct ActorHitSensorState {
        std::string name{};
        TVec3f offset{};
        std::unique_ptr<HitSensor> sensor{};
    };

    struct LiveActorRuntimeState {
        std::shared_ptr<smgpc::compat::ModelManagerOwner> model_owner{};
        std::unique_ptr<ActorAnimKeeper> anim_keeper{};
        std::unique_ptr<ActorPadAndCameraCtrl> camera_ctrl{};
        std::shared_ptr<smgpc::compat::JkrAllocationDomain> sound_domain{};
        std::unique_ptr<AudAnmSoundObject> sound_object{};
        std::vector<ActorHitSensorState> hit_sensors{};
        std::optional<smgpc::compat::ActorBinderRuntimeConfig> binder{};
        std::unique_ptr<Binder> binder_provider{};
        smgpc::compat::ActorBinderContactState binder_contacts{};
        std::optional<smgpc::compat::ActorClippingRuntimeState> clipping{};
        std::optional<smgpc::compat::ActorShadowRuntimeState> shadow{};
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

    [[nodiscard]] const smgpc::compat::NameObjRuntimeRegistrationCapture*&
    active_name_obj_registration_capture() {
        static const smgpc::compat::NameObjRuntimeRegistrationCapture* capture =
            nullptr;
        return capture;
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
            throw std::invalid_argument("LiveActor runtime state requires a real actor.");
        }
        const auto found = actor_states().find(actor);
        if (found == actor_states().end()) {
            throw std::logic_error("LiveActor has no registered native runtime state.");
        }
        return found->second;
    }

    void destroy_hit_sensors(LiveActorRuntimeState& state) {
        for (auto& entry : state.hit_sensors) {
            if (entry.sensor != nullptr) {
                delete[] entry.sensor->mSensors;
                entry.sensor->mSensors = nullptr;
                entry.sensor->mSensorCount = 0U;
            }
        }
        state.hit_sensors.clear();
    }

    void invalidate_shadow_joint_matrix_bindings(LiveActorRuntimeState& state) noexcept {
        if (!state.shadow.has_value()) {
            return;
        }
        for (auto& controller : state.shadow->controllers) {
            if (controller.position_binding == smgpc::compat::ActorShadowPositionBinding::JointMatrix) {
                controller.drop_position_matrix = nullptr;
            }
        }
    }
}  // namespace

namespace smgpc::compat {
    NameObjRuntimeRegistrationCapture::NameObjRuntimeRegistrationCapture() {
        auto& active = active_name_obj_registration_capture();
        if (active != nullptr) {
            throw std::logic_error(
                "NameObj construction capture cannot overlap or nest.");
        }
        _marker = mark_name_obj_runtime_registrations();
        active = this;
    }

    NameObjRuntimeRegistrationCapture::~NameObjRuntimeRegistrationCapture() {
        auto& active = active_name_obj_registration_capture();
        if (active == this) {
            active = nullptr;
        }
    }

    NameObjRuntimeRegistrationMarker
    NameObjRuntimeRegistrationCapture::marker() const noexcept {
        return _marker;
    }

    const char* register_name_obj_runtime_state(NameObj* object, const char* name) {
        JkrHostAllocationScope host;
        if (object == nullptr) {
            throw std::invalid_argument("NameObj runtime state requires a real object.");
        }
        const auto registration_order = next_name_obj_registration_order()++;
        auto [found, inserted] = name_obj_states().try_emplace(
            object, NameObjRuntimeState{
                        .name = name != nullptr ? name : "",
                        .registration_order = registration_order,
                    });
        if (!inserted) {
            throw std::logic_error("NameObj runtime state is already registered.");
        }
        return found->second.name.c_str();
    }

    const char* update_name_obj_runtime_name(NameObj* object, const char* name) {
        JkrHostAllocationScope host;
        if (object == nullptr) {
            throw std::invalid_argument("NameObj runtime state requires a real object.");
        }
        const auto found = name_obj_states().find(object);
        if (found == name_obj_states().end()) {
            throw std::logic_error("NameObj has no registered native runtime state.");
        }
        found->second.name = name != nullptr ? name : "";
        return found->second.name.c_str();
    }

    void release_name_obj_runtime_state(const NameObj* object) {
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
            throw std::invalid_argument(
                "NameObj runtime ownership requires real object and owner identities.");
        }
        const auto found = name_obj_states().find(object);
        if (found == name_obj_states().end()) {
            throw std::logic_error(
                "NameObj runtime ownership requires a registered object.");
        }
        if (found->second.owner != nullptr) {
            throw std::logic_error(
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

    void delegate_name_obj_runtime_postpass(NameObj* object,
                                            const void* delegate) {
        if (object == nullptr || delegate == nullptr) {
            throw std::invalid_argument(
                "NameObj postpass delegation requires real object and delegate identities.");
        }
        const auto found = name_obj_states().find(object);
        if (found == name_obj_states().end()) {
            throw std::logic_error(
                "NameObj postpass delegation requires a registered object.");
        }
        if (found->second.postpass_delegate != nullptr &&
            found->second.postpass_delegate != delegate) {
            throw std::logic_error(
                "NameObj postpass is already delegated to another boundary.");
        }
        found->second.postpass_delegate = delegate;
    }

    void release_name_obj_runtime_postpass_delegation(
        const NameObj* object, const void* delegate) noexcept {
        const auto found = name_obj_states().find(object);
        if (found != name_obj_states().end() &&
            found->second.postpass_delegate == delegate) {
            found->second.postpass_delegate = nullptr;
        }
    }

    bool name_obj_runtime_postpass_is_delegated(
        const NameObj* object) noexcept {
        const auto found = name_obj_states().find(object);
        return found != name_obj_states().end() &&
               found->second.postpass_delegate != nullptr;
    }

    const void* name_obj_runtime_postpass_delegate(
        const NameObj* object) noexcept {
        const auto found = name_obj_states().find(object);
        return found != name_obj_states().end()
                   ? found->second.postpass_delegate
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
            throw std::invalid_argument("NameObj runtime registration marker is invalid.");
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
        constexpr auto movement_off = u16{1U};
        constexpr auto suspend_requested = u16{2U};
        constexpr auto resume_requested = u16{4U};
        return (object->mFlag & (movement_off | suspend_requested)) != 0U &&
               (object->mFlag & resume_requested) == 0U;
    }

    void register_actor_runtime_state(LiveActor* actor) {
        if (actor == nullptr) {
            throw std::invalid_argument("LiveActor runtime state requires a real actor.");
        }
        if (!actor_states().try_emplace(actor).second) {
            throw std::logic_error("LiveActor runtime state is already registered.");
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

        release_actor_collision_parts(actor);
        release_actor_sensor_bindings(actor);
        release_talk_runtime_state(actor);
        release_demo_runtime_state(actor);

        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->scene_lights().unregister_player_light_ctrl(actor->mActorLightCtrl);
            runtime->star_pointer().unregister_target(*actor);
            runtime->unregister_effect_keeper(actor->getName(), actor);
            runtime->unregister_live_actor_model(*const_cast<LiveActor*>(actor));
        }

        destroy_hit_sensors(found->second);
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
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
            runtime->scene_lights().unregister_player_light_ctrl(state.light_ctrl.get());
        }
        state.light_ctrl = std::make_unique<ActorLightCtrl>(actor);
        actor->mActorLightCtrl = state.light_ctrl.get();
    }

    void adopt_actor_lod_ctrl(LiveActor* actor, LodCtrl* lod_ctrl) {
        if (lod_ctrl == nullptr) {
            throw std::invalid_argument("LiveActor LOD ownership requires a real LodCtrl.");
        }
        auto& state = require_actor_state(actor);
        if (state.lod_ctrl != nullptr && state.lod_ctrl.get() != lod_ctrl) {
            throw std::logic_error("LiveActor already owns a different LodCtrl.");
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
        if (state.model_owner) {
            throw std::logic_error("Actor model replacement requires scene draw retirement first");
        }
        auto* service = ResourceHolderService::active();
        if (!service) {
            throw std::logic_error("Actor ModelManager requires the active scene resource service");
        }
        JkrHostAllocationScope host;
        auto owner = std::make_shared<ModelManagerOwner>(*service, service->allocation_domain(),
                                                        model_archive, animation_archive, create_display_list);
        actor->mModelManager = &owner->manager();
        state.model_owner = std::move(owner);
    }

    std::shared_ptr<JkrAllocationDomain> actor_scene_allocation_domain(const LiveActor* actor) {
        const auto& state = require_actor_state(actor);
        auto* service = ResourceHolderService::active();
        if (!service) throw std::logic_error("Actor sound construction requires the active scene resource cohort");
        if (state.model_owner && state.model_owner->allocation_domain() != service->allocation_domain())
            throw std::logic_error("Actor model and sound must retain the same scene resource cohort");
        return service->allocation_domain();
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

    std::shared_ptr<ModelManagerOwner> retain_actor_model_owner(const LiveActor* actor) {
        return require_actor_state(actor).model_owner;
    }

    std::optional<std::span<const std::uint8_t>>
    actor_model_resource_data_if_present(const LiveActor* actor, std::string_view resource_name) {
        if (!actor || !actor->mModelManager || resource_name.empty()) return std::nullopt;
        auto* service = ResourceHolderService::active();
        if (!service) return std::nullopt;
        const auto& archive = service->backing(*MR::getModelResourceHolder(actor)).archive();
        if (!archive.contains_resource(resource_name)) return std::nullopt;
        return archive.resource_data(resource_name);
    }

    void initialize_actor_hit_sensors(LiveActor* actor, int sensor_count) {
        auto& state = require_actor_state(actor);
        release_actor_sensor_bindings(actor);
        destroy_hit_sensors(state);
        if (sensor_count > 0) {
            state.hit_sensors.reserve(static_cast<std::size_t>(sensor_count));
        }
    }

    HitSensor* add_actor_hit_sensor(LiveActor* actor, const char* name, std::uint32_t type,
                                    std::uint16_t group_size, float radius, const TVec3f& offset) {
        auto& state = require_actor_state(actor);
        auto entry = ActorHitSensorState{};
        entry.name = name != nullptr ? name : "";
        entry.offset = offset;
        entry.sensor = std::make_unique<HitSensor>(type, group_size, radius, actor);
        entry.sensor->mPosition = actor->mPosition + offset;
        entry.sensor->validateBySystem();
        if (actor->mFlag.mIsDead) {
            entry.sensor->invalidate();
        } else {
            entry.sensor->validate();
        }
        state.hit_sensors.push_back(std::move(entry));
        return state.hit_sensors.back().sensor.get();
    }

    HitSensor* actor_hit_sensor(const LiveActor* actor, const char* name) {
        if (actor == nullptr || name == nullptr) {
            return nullptr;
        }
        auto& sensors = require_actor_state(actor).hit_sensors;
        for (auto& entry : sensors) {
            if (entry.name == name) {
                return entry.sensor.get();
            }
        }
        return nullptr;
    }

    const char* actor_hit_sensor_name(const LiveActor* actor, const HitSensor* sensor) {
        if (actor == nullptr || sensor == nullptr) {
            return "";
        }
        for (const auto& entry : require_actor_state(actor).hit_sensors) {
            if (entry.sensor.get() == sensor) {
                return entry.name.c_str();
            }
        }
        return "";
    }

    void collect_actor_hit_sensors(const LiveActor* actor, std::vector<HitSensor*>& sensors) {
        if (actor == nullptr) {
            return;
        }
        for (const auto& entry : require_actor_state(actor).hit_sensors) {
            if (entry.sensor != nullptr) {
                sensors.push_back(entry.sensor.get());
            }
        }
    }

    void validate_actor_hit_sensors(LiveActor* actor) {
        if (actor == nullptr) {
            return;
        }
        for (const auto& entry : require_actor_state(actor).hit_sensors) {
            if (entry.sensor != nullptr) {
                entry.sensor->validate();
            }
        }
    }

    void invalidate_actor_hit_sensors(LiveActor* actor) {
        if (actor == nullptr) {
            return;
        }
        for (const auto& entry : require_actor_state(actor).hit_sensors) {
            if (entry.sensor != nullptr) {
                entry.sensor->invalidate();
            }
        }
    }

    void update_actor_hit_sensors(LiveActor* actor) {
        if (actor == nullptr) {
            return;
        }
        for (const auto& entry : require_actor_state(actor).hit_sensors) {
            if (entry.sensor != nullptr) {
                entry.sensor->mPosition = actor->mPosition + entry.offset;
            }
        }
        update_actor_sensor_bindings(actor);
    }

    std::size_t actor_hit_sensor_count(const LiveActor* actor) {
        const auto found = actor_states().find(actor);
        return found != actor_states().end() ? found->second.hit_sensors.size() : 0U;
    }

    void configure_actor_binder(LiveActor* actor, float radius, float offset, std::uint32_t plane_capacity) {
        auto& state = require_actor_state(actor);
        state.binder = ActorBinderRuntimeConfig{radius, offset, plane_capacity};
        state.binder_provider = std::make_unique<Binder>(
            actor->getBaseMtx(), &actor->mPosition, &actor->mGravity,
            radius, offset, plane_capacity);
        actor->mBinder = state.binder_provider.get();
        state.binder_contacts = {};
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
            state.binder_contacts = {};
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

    void clear_actor_binder_contacts(LiveActor* actor) {
        if (actor == nullptr) {
            return;
        }
        auto& state = require_actor_state(actor);
        if (state.binder.has_value()) {
            state.binder_contacts = {};
        }
    }

    void record_actor_binder_contacts(LiveActor* actor, const ActorBinderContactState& contacts) {
        if (actor == nullptr) {
            return;
        }
        auto& state = require_actor_state(actor);
        if (state.binder.has_value()) {
            state.binder_contacts = contacts;
        }
    }

    const ActorBinderContactState* actor_binder_contacts(const LiveActor* actor) {
        if (actor == nullptr) {
            return nullptr;
        }
        const auto found = actor_states().find(actor);
        return found != actor_states().end() && found->second.binder.has_value()
                   ? &found->second.binder_contacts
                   : nullptr;
    }

    void release_actor_binder_state(const LiveActor* actor) {
        if (actor == nullptr) {
            return;
        }
        auto& state = require_actor_state(actor);
        const_cast<LiveActor*>(actor)->mBinder = nullptr;
        state.binder_provider.reset();
        state.binder.reset();
        state.binder_contacts = {};
    }

    void configure_actor_clipping_sphere(LiveActor* actor, float radius, const TVec3f* center) {
        if (actor == nullptr) {
            throw std::invalid_argument("Actor clipping operation requires a LiveActor.");
        }
        if (!std::isfinite(radius) || radius < 0.0F) {
            throw std::invalid_argument("Actor clipping radius must be finite and non-negative.");
        }
        auto& stored_clipping = require_actor_state(actor).clipping;
        if (!stored_clipping.has_value()) {
            stored_clipping.emplace();
        }
        auto& clipping = *stored_clipping;
        clipping.sphere_configured = true;
        clipping.sphere_radius = radius;
        clipping.sphere_center = center;
    }

    void configure_actor_clipping_far_level(LiveActor* actor, int level) {
        if (actor == nullptr) {
            throw std::invalid_argument("Actor clipping operation requires a LiveActor.");
        }
        if (level < 0 || level > 7) {
            throw std::invalid_argument("Actor clipping far level must be in the original 0..7 range.");
        }
        auto& clipping = require_actor_state(actor).clipping;
        if (!clipping.has_value()) {
            clipping.emplace();
        }
        clipping->far_level = level;
    }

    const ActorClippingRuntimeState* actor_clipping_runtime_state(const LiveActor* actor) {
        if (actor == nullptr) {
            return nullptr;
        }
        const auto found = actor_states().find(actor);
        if (found == actor_states().end()) {
            return nullptr;
        }
        const auto& clipping = found->second.clipping;
        return clipping.has_value() ? &*clipping : nullptr;
    }

    void release_actor_clipping_state(const LiveActor* actor) {
        if (actor != nullptr) {
            require_actor_state(actor).clipping.reset();
        }
    }

    void initialize_actor_shadow_controller_list(LiveActor* actor, std::uint32_t capacity) {
        if (actor == nullptr) {
            throw std::invalid_argument("Actor shadow ownership requires a LiveActor.");
        }
        auto shadow = ActorShadowRuntimeState{
            .valid = false,
            .calculation_enabled = false,
            .private_gravity = false,
            .capacity = capacity,
            .controllers = {},
        };
        shadow.controllers.reserve(capacity);
        replace_actor_shadow_runtime_state(actor, std::move(shadow));
    }

    ActorShadowControllerRuntimeState make_actor_shadow_controller_runtime_state(
        LiveActor* actor, std::string_view name, ActorShadowControllerKind kind, float radius) {
        if (actor == nullptr) {
            throw std::invalid_argument("Actor shadow ownership requires a LiveActor.");
        }
        if (!std::isfinite(radius) || radius < 0.0F) {
            throw std::invalid_argument("Actor shadow radius must be finite and non-negative.");
        }
        return ActorShadowControllerRuntimeState{
            .name = std::string{name},
            .name_raw = std::string{name},
            .group_name = {},
            .kind = kind,
            .position_binding = ActorShadowPositionBinding::ActorTranslation,
            .joint_name = {},
            .joint_name_raw = {},
            .model_name = {},
            .line_start_name = {},
            .line_end_name = {},
            .line_start_name_raw = {},
            .line_end_name_raw = {},
            .radius = radius,
            .size = TVec3f{100.0F, 100.0F, 100.0F},
            .drop_offset = {},
            .fixed_drop_position = {},
            .fixed_drop_direction = TVec3f{0.0F, -1.0F, 0.0F},
            .drop_position = &actor->mPosition,
            .drop_position_matrix = nullptr,
            .drop_direction = &actor->mGravity,
            .drop_length = 1000.0F,
            .drop_start_offset = 50.0F,
            .volume_start_offset = 100.0F,
            .volume_end_offset = 100.0F,
            .line_start_radius = 100.0F,
            .line_end_radius = 100.0F,
            .line_start_controller_index = {},
            .line_end_controller_index = {},
            .volume_cut_drop_length = false,
            .follow_host_scale = false,
            .valid = true,
            .visible_sync_host = true,
            .calculation_mode = ActorShadowCalculationMode::Continuous,
            .gravity_mode = ActorShadowGravityMode::HostDirection,
        };
    }

    void replace_actor_shadow_runtime_state(LiveActor* actor, ActorShadowRuntimeState state) {
        if (actor == nullptr) {
            throw std::invalid_argument("Actor shadow ownership requires a LiveActor.");
        }
        if (state.controllers.size() > state.capacity) {
            throw std::length_error("Actor shadow controller list exceeds its retail capacity.");
        }
        const auto index_is_valid = [&state](const std::optional<std::size_t>& index) {
            return !index.has_value() || *index < state.controllers.size();
        };
        for (const auto& controller : state.controllers) {
            if (!index_is_valid(controller.line_start_controller_index) ||
                !index_is_valid(controller.line_end_controller_index)) {
                throw std::out_of_range("Actor shadow line endpoint index is outside the controller list.");
            }
        }
        state.controllers.reserve(state.capacity);
        require_actor_state(actor).shadow = std::move(state);
    }

    ActorShadowControllerRuntimeState& add_actor_shadow_controller(
        LiveActor* actor, std::string_view name, ActorShadowControllerKind kind, float radius) {
        if (actor == nullptr) {
            throw std::invalid_argument("Actor shadow ownership requires a LiveActor.");
        }
        auto* shadow = actor_shadow_runtime_state(actor);
        if (shadow == nullptr) {
            throw std::logic_error("Actor shadow controllers require an initialized controller list.");
        }
        if (shadow->controllers.size() >= shadow->capacity) {
            throw std::length_error("Actor shadow controller list has reached its retail capacity.");
        }
        shadow->controllers.push_back(make_actor_shadow_controller_runtime_state(actor, name, kind, radius));
        shadow->valid = true;
        shadow->calculation_enabled = true;
        return shadow->controllers.back();
    }

    ActorShadowControllerRuntimeState* actor_shadow_controller_runtime_state(LiveActor* actor, const char* name) {
        auto* shadow = actor_shadow_runtime_state(actor);
        if (shadow == nullptr || shadow->controllers.empty()) {
            return nullptr;
        }
        if (shadow->controllers.size() == 1U) {
            return &shadow->controllers.front();
        }
        if (name == nullptr) {
            return nullptr;
        }
        const auto found = std::ranges::find_if(shadow->controllers, [name](const auto& controller) {
            return controller.name == name;
        });
        return found != shadow->controllers.end() ? &*found : nullptr;
    }

    const ActorShadowControllerRuntimeState* actor_shadow_controller_runtime_state(
        const LiveActor* actor, const char* name) {
        const auto* shadow = actor_shadow_runtime_state(actor);
        if (shadow == nullptr || shadow->controllers.empty()) {
            return nullptr;
        }
        if (shadow->controllers.size() == 1U) {
            return &shadow->controllers.front();
        }
        if (name == nullptr) {
            return nullptr;
        }
        const auto found = std::ranges::find_if(shadow->controllers, [name](const auto& controller) {
            return controller.name == name;
        });
        return found != shadow->controllers.end() ? &*found : nullptr;
    }

    ActorShadowRuntimeState* actor_shadow_runtime_state(LiveActor* actor) {
        if (actor == nullptr) {
            return nullptr;
        }
        auto& shadow = require_actor_state(actor).shadow;
        if (!shadow.has_value()) {
            shadow.emplace();
        }
        return &*shadow;
    }

    const ActorShadowRuntimeState* actor_shadow_runtime_state(const LiveActor* actor) {
        if (actor == nullptr) {
            return nullptr;
        }
        const auto found = actor_states().find(actor);
        if (found == actor_states().end()) {
            return nullptr;
        }
        const auto& shadow = found->second.shadow;
        return shadow.has_value() ? &*shadow : nullptr;
    }

    std::size_t actor_shadow_runtime_state_count() {
        auto count = std::size_t{};
        for (const auto& [actor, state] : actor_states()) {
            (void)actor;
            count += state.shadow.has_value() ? 1U : 0U;
        }
        return count;
    }
}  // namespace smgpc::compat
