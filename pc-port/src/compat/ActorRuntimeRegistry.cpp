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
#include "compat/BinderCompat.hpp"
#include "compat/GameActorSensorCompat.hpp"
#include "compat/MaterialCtrlCompat.hpp"
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
    };

    struct ActorHitSensorState {
        std::string name{};
        TVec3f offset{};
        std::unique_ptr<HitSensor> sensor{};
    };

    struct ActorAnimationRuntimeState {
        J3DFrameCtrl bck_ctrl{};
        J3DFrameCtrl brk_ctrl{};
        bool bck_available = false;
        bool bck_active = false;
        bool brk_available = false;
        bool brk_active = false;
        std::string current_bck_name{};
        std::string current_brk_name{};
        std::string current_btk_name{};
        std::string current_btp_name{};
    };

    struct LiveActorRuntimeState {
        smgpc::render::J3dMatrix3x4 base_matrix{};
        std::unique_ptr<smgpc::render::live_actor::LiveActorModel> model{};
        ActorAnimationRuntimeState animation{};
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

    std::size_t name_obj_runtime_state_count() {
        return name_obj_states().size();
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

    void destroy_name_obj_runtime_objects_since(
        NameObjRuntimeRegistrationMarker marker) noexcept {
        if (marker.next_registration_order == 0U) {
            return;
        }

        // Selecting the newest identity on each pass avoids allocating while
        // allowing each destructor to mutate the registry safely.
        while (true) {
            auto* newest = static_cast<const NameObj*>(nullptr);
            auto newest_order = std::uint64_t{};
            for (const auto& [object, state] : name_obj_states()) {
                if (state.registration_order >= marker.next_registration_order &&
                    state.registration_order > newest_order) {
                    newest = object;
                    newest_order = state.registration_order;
                }
            }
            if (newest == nullptr) {
                return;
            }
            delete const_cast<NameObj*>(newest);
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
        release_actor_material_ctrl_state(actor);
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

    void initialize_actor_model(LiveActor* actor, const char* model_archive, const char* animation_archive) {
        auto& state = require_actor_state(actor);
        state.model = std::make_unique<smgpc::render::live_actor::LiveActorModel>(
            model_archive != nullptr ? model_archive : "", animation_archive != nullptr ? animation_archive : "");
    }

    smgpc::render::live_actor::LiveActorModel* actor_model(const LiveActor* actor) {
        const auto found = actor_states().find(actor);
        return found != actor_states().end() ? found->second.model.get() : nullptr;
    }

    void require_actor_model(LiveActor* actor) {
        auto& state = require_actor_state(actor);
        if (state.model == nullptr) {
            throw std::logic_error("LiveActor has no native model binding.");
        }
        state.model->requireLoaded();
    }

    std::size_t actor_model_joint_count(const LiveActor* actor) {
        auto& state = require_actor_state(actor);
        if (state.model == nullptr) {
            throw std::logic_error("LiveActor has no native model binding.");
        }
        return state.model->joint_count();
    }

    void release_actor_model_state(const LiveActor* actor) {
        const auto found = actor_states().find(actor);
        if (found != actor_states().end()) {
            found->second.model.reset();
        }
    }

    const smgpc::render::J3dMatrix3x4& actor_base_matrix(const LiveActor* actor) {
        return require_actor_state(actor).base_matrix;
    }

    void set_actor_base_matrix(LiveActor* actor, const smgpc::render::J3dMatrix3x4& matrix) {
        require_actor_state(actor).base_matrix = matrix;
    }

    void set_actor_projmap_effect_matrix(LiveActor* actor, const smgpc::render::J3dMatrix3x4& matrix) {
        if (auto* model = actor_model(actor); model != nullptr) {
            model->setProjmapEffectMatrix(matrix);
        }
    }

    void draw_actor_model(LiveActor* actor, const smgpc::camera::CameraPose& camera_pose,
                          std::uint64_t frame, smgpc::render::live_actor::LiveActorModel::DrawPass pass) {
        auto* model = actor_model(actor);
        if (actor == nullptr || actor->mFlag.mIsDead || actor->mFlag.mIsClipped ||
            actor->mFlag.mIsHiddenModel || model == nullptr) {
            return;
        }
        model->draw(camera_pose, actor_base_matrix(actor), frame, pass);
    }

    void draw_actor_model_3d_for_2d(
        LiveActor* actor,
        const smgpc::render::Model3DFor2DProjection& projection,
        std::uint64_t frame,
        smgpc::render::live_actor::LiveActorModel::DrawPass pass) {
        auto* model = actor_model(actor);
        if (actor == nullptr || actor->mFlag.mIsDead || actor->mFlag.mIsClipped ||
            actor->mFlag.mIsHiddenModel || model == nullptr) {
            return;
        }
        model->drawModel3DFor2D(projection, actor_base_matrix(actor), frame, pass);
    }

    void start_actor_bck(LiveActor* actor, const char* name, const char* file_name) {
        auto& state = require_actor_state(actor);
        auto& animation = state.animation;
        animation.current_bck_name = name != nullptr ? name : "";
        auto frame_max = std::optional<std::int16_t>{};
        if (state.model != nullptr) {
            frame_max = state.model->startBck(animation.current_bck_name, file_name != nullptr ? file_name : "");
        }
        animation.bck_available = frame_max.has_value();
        animation.bck_active = animation.bck_available;
        animation.bck_ctrl.init(frame_max.value_or(0));
        animation.bck_ctrl.setAttribute(
            state.model != nullptr
                ? state.model->bck_attribute().value_or(2U)
                : 2U);
        if (!animation.bck_available || animation.bck_ctrl.mEnd <= 0) {
            animation.bck_ctrl.mRate = 0.0F;
        }
    }

    std::int16_t require_actor_bck(LiveActor* actor, const char* name, const char* file_name) {
        auto& state = require_actor_state(actor);
        if (state.model == nullptr) {
            throw std::logic_error("LiveActor has no native model binding.");
        }
        const auto animation_name = name != nullptr ? std::string_view(name) : std::string_view{};
        const auto resource_name = file_name != nullptr ? std::string_view(file_name) : std::string_view{};
        const auto frame_max = state.model->requireBck(animation_name, resource_name);
        auto& animation = state.animation;
        animation.current_bck_name = std::string(animation_name);
        animation.bck_available = true;
        animation.bck_active = true;
        animation.bck_ctrl.init(frame_max);
        animation.bck_ctrl.setAttribute(
            state.model->bck_attribute().value_or(2U));
        if (frame_max <= 0) {
            animation.bck_ctrl.mRate = 0.0F;
        }
        return frame_max;
    }

    void start_actor_brk(LiveActor* actor, const char* name) {
        auto& state = require_actor_state(actor);
        auto& animation = state.animation;
        animation.current_brk_name = name != nullptr ? name : "";
        auto frame_max = std::optional<std::int16_t>{};
        if (state.model != nullptr) {
            frame_max = state.model->startBrk(animation.current_brk_name);
        }
        animation.brk_available = frame_max.has_value();
        animation.brk_active = animation.brk_available;
        animation.brk_ctrl.init(frame_max.value_or(0));
        animation.brk_ctrl.setAttribute(
            state.model != nullptr
                ? state.model->brk_attribute().value_or(2U)
                : 2U);
        if (!animation.brk_available || animation.brk_ctrl.mEnd <= 0) {
            animation.brk_ctrl.mRate = 0.0F;
        }
    }

    void start_actor_btk(LiveActor* actor, const char* name) {
        auto& state = require_actor_state(actor);
        state.animation.current_btk_name = name != nullptr ? name : "";
        if (state.model != nullptr) {
            (void)state.model->startBtk(state.animation.current_btk_name);
        }
    }

    void start_actor_btp(LiveActor* actor, const char* name) {
        auto& state = require_actor_state(actor);
        state.animation.current_btp_name = name != nullptr ? name : "";
        if (state.model != nullptr) {
            (void)state.model->startBtp(state.animation.current_btp_name);
        }
    }

    bool try_start_actor_bck(LiveActor* actor, const char* name, const char* file_name) {
        auto& state = require_actor_state(actor);
        const auto animation_name = name != nullptr ? std::string_view(name) : std::string_view{};
        const auto resource_name = file_name != nullptr ? std::string_view(file_name) : std::string_view{};
        if (state.model == nullptr || !state.model->hasBck(animation_name, resource_name)) {
            return false;
        }
        start_actor_bck(actor, name, file_name);
        return true;
    }

    bool try_start_actor_brk(LiveActor* actor, const char* name) {
        auto& state = require_actor_state(actor);
        const auto animation_name = name != nullptr ? std::string_view(name) : std::string_view{};
        if (state.model == nullptr || !state.model->hasBrk(animation_name)) {
            return false;
        }
        start_actor_brk(actor, name);
        return true;
    }

    bool try_start_actor_btk(LiveActor* actor, const char* name) {
        auto& state = require_actor_state(actor);
        const auto animation_name = name != nullptr ? std::string_view(name) : std::string_view{};
        if (state.model == nullptr || !state.model->hasBtk(animation_name)) {
            return false;
        }
        start_actor_btk(actor, name);
        return true;
    }

    bool try_start_actor_btp(LiveActor* actor, const char* name) {
        auto& state = require_actor_state(actor);
        const auto animation_name = name != nullptr ? std::string_view(name) : std::string_view{};
        if (state.model == nullptr || !state.model->hasBtp(animation_name)) {
            return false;
        }
        start_actor_btp(actor, name);
        return true;
    }

    void set_actor_brk_frame(LiveActor* actor, float frame) {
        auto& animation = require_actor_state(actor).animation;
        if (!animation.brk_available) {
            throw std::logic_error("BRK animation data is unavailable.");
        }
        animation.brk_active = true;
        animation.brk_ctrl.mFrame = frame;
        if (auto* model = require_actor_state(actor).model.get(); model != nullptr) {
            model->setBrkFrame(frame);
        }
    }

    void set_actor_brk_rate(LiveActor* actor, float rate) {
        auto& animation = require_actor_state(actor).animation;
        if (!animation.brk_available) {
            throw std::logic_error("BRK animation data is unavailable.");
        }
        animation.brk_active = true;
        animation.brk_ctrl.mRate = rate;
    }

    void set_actor_brk_frame_and_stop(LiveActor* actor, float frame) {
        set_actor_brk_frame(actor, frame);
        require_actor_state(actor).animation.brk_ctrl.mRate = 0.0F;
    }

    void set_actor_brk_frame_end_and_stop(LiveActor* actor) {
        auto& animation = require_actor_state(actor).animation;
        set_actor_brk_frame_and_stop(actor, static_cast<float>(animation.brk_ctrl.mEnd));
    }

    void set_actor_bck_frame_and_stop(LiveActor* actor, float frame) {
        auto& state = require_actor_state(actor);
        auto& animation = state.animation;
        if (!animation.bck_available || state.model == nullptr) {
            throw std::logic_error("BCK animation data is unavailable.");
        }
        animation.bck_active = true;
        animation.bck_ctrl.mFrame = frame;
        animation.bck_ctrl.mRate = 0.0F;
        state.model->setBckFrameAndStop(frame);
    }

    J3DFrameCtrl* actor_bck_ctrl(const LiveActor* actor) {
        auto& animation = require_actor_state(actor).animation;
        return animation.bck_available ? &animation.bck_ctrl : nullptr;
    }

    J3DFrameCtrl* actor_brk_ctrl(const LiveActor* actor) {
        auto& animation = require_actor_state(actor).animation;
        return animation.brk_available ? &animation.brk_ctrl : nullptr;
    }

    bool is_actor_brk_one_time_and_stopped(const LiveActor* actor) {
        const auto& animation = require_actor_state(actor).animation;
        if (!animation.brk_available) {
            throw std::logic_error("BRK animation state is unavailable.");
        }
        return !animation.brk_active || animation.brk_ctrl.mRate == 0.0F ||
               animation.brk_ctrl.mFrame >= static_cast<float>(animation.brk_ctrl.mEnd);
    }

    std::string_view actor_current_bck_name(const LiveActor* actor) {
        return require_actor_state(actor).animation.current_bck_name;
    }

    std::string_view actor_current_brk_name(const LiveActor* actor) {
        return require_actor_state(actor).animation.current_brk_name;
    }

    std::string_view actor_current_btk_name(const LiveActor* actor) {
        return require_actor_state(actor).animation.current_btk_name;
    }

    std::string_view actor_current_btp_name(const LiveActor* actor) {
        return require_actor_state(actor).animation.current_btp_name;
    }

    void advance_actor_animation(LiveActor* actor) {
        auto& state = require_actor_state(actor);
        auto& animation = state.animation;
        if (animation.bck_active && animation.bck_ctrl.mRate != 0.0F) {
            animation.bck_ctrl.update();
        }
        if (animation.brk_active && animation.brk_ctrl.mRate != 0.0F) {
            animation.brk_ctrl.update();
        }
        if (animation.brk_active && animation.brk_available &&
            state.model != nullptr) {
            state.model->setBrkFrame(animation.brk_ctrl.mFrame);
        }
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
            reinterpret_cast<MtxPtr>(state.base_matrix.m.data()), &actor->mPosition, &actor->mGravity,
            radius, offset, plane_capacity);
        actor->mBinder = state.binder_provider.get();
        register_binder_owner(actor->mBinder, actor);
        state.binder_contacts = {};
    }

    void register_actor_binder(const LiveActor* actor) {
        if (actor != nullptr) {
            auto& state = require_actor_state(actor);
            state.binder.emplace();
            auto* mutable_actor = const_cast<LiveActor*>(actor);
            state.binder_provider = std::make_unique<Binder>(
                reinterpret_cast<MtxPtr>(state.base_matrix.m.data()), &mutable_actor->mPosition,
                &mutable_actor->mGravity, 0.0F, 0.0F, 0U);
            mutable_actor->mBinder = state.binder_provider.get();
            register_binder_owner(mutable_actor->mBinder, mutable_actor);
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
        require_actor_state(actor).shadow = std::move(shadow);
    }

    ActorShadowControllerRuntimeState& add_actor_shadow_controller(
        LiveActor* actor, std::string_view name, ActorShadowControllerKind kind, float radius) {
        if (actor == nullptr) {
            throw std::invalid_argument("Actor shadow ownership requires a LiveActor.");
        }
        if (name.empty()) {
            throw std::invalid_argument("Actor shadow controllers require a name.");
        }
        if (!std::isfinite(radius) || radius < 0.0F) {
            throw std::invalid_argument("Actor shadow radius must be finite and non-negative.");
        }
        auto* shadow = actor_shadow_runtime_state(actor);
        if (shadow == nullptr) {
            throw std::logic_error("Actor shadow controllers require an initialized controller list.");
        }
        if (shadow->controllers.size() >= shadow->capacity) {
            throw std::length_error("Actor shadow controller list has reached its retail capacity.");
        }
        shadow->controllers.push_back(ActorShadowControllerRuntimeState{
            .name = std::string{name},
            .kind = kind,
            .radius = radius,
            .drop_position = &actor->mPosition,
            .drop_direction = &actor->mGravity,
            .drop_length = 1000.0F,
            .valid = true,
            .visible_sync_host = true,
            .calculation_mode = ActorShadowCalculationMode::Disabled,
            .gravity_mode = ActorShadowGravityMode::HostDirection,
        });
        shadow->valid = true;
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
