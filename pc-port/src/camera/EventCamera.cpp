#include "camera/EventCamera.hpp"

#include "Game/Camera/CameraTargetMtx.hpp"
#include "Game/Camera/CameraPoseParam.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/TextEncoding.hpp"
#include "runtime/RuntimeServices.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <utility>

namespace smgpc::camera {
    namespace {

        struct TargetSnapshot {
            CameraParamVec3 position{};
            CameraParamVec3 side{1.0F, 0.0F, 0.0F};
            CameraParamVec3 up{0.0F, 1.0F, 0.0F};
            CameraParamVec3 front{0.0F, 0.0F, 1.0F};
            CameraParamVec3 last_move{};
            std::optional<CameraParamVec3> ground_position;
            std::optional<CameraParamVec3> gravity;
            bool jumping = false;
            bool fast_rise = false;
            bool fast_drop = false;
        };

        [[nodiscard]] CameraParamVec3 scale(const CameraParamVec3 &value,
                                            float factor) {
            return {.x = value.x * factor, .y = value.y * factor, .z = value.z * factor};
        }

        [[nodiscard]] float dot(const CameraParamVec3 &lhs,
                                const CameraParamVec3 &rhs) {
            return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
        }

        [[nodiscard]] CameraParamVec3 cross(const CameraParamVec3 &lhs,
                                            const CameraParamVec3 &rhs) {
            return {.x = lhs.y * rhs.z - lhs.z * rhs.y,
                    .y = lhs.z * rhs.x - lhs.x * rhs.z,
                    .z = lhs.x * rhs.y - lhs.y * rhs.x};
        }

        [[nodiscard]] std::optional<CameraParamVec3> normalized(
            const CameraParamVec3 &value) {
            const auto magnitude = std::sqrt(dot(value, value));
            if (!(magnitude > 0.000001F)) {
                return std::nullopt;
            }
            return scale(value, 1.0F / magnitude);
        }

        [[nodiscard]] TargetSnapshot snapshot_from_matrix(
            std::span<const float, 12U> matrix,
            CameraParamVec3 last_move = {}) {
            auto result = TargetSnapshot{
                .position = {matrix[3U], matrix[7U], matrix[11U]},
                .side = {matrix[0U], matrix[4U], matrix[8U]},
                .up = {matrix[1U], matrix[5U], matrix[9U]},
                .front = {matrix[2U], matrix[6U], matrix[10U]},
                .last_move = last_move,
            };
            const auto side = normalized(result.side);
            const auto up = normalized(result.up);
            const auto front = normalized(result.front);
            if (!side.has_value() || !up.has_value() || !front.has_value()) {
                throw std::logic_error(
                    "Event-camera target has a degenerate base matrix.");
            }
            result.side = *side;
            result.up = *up;
            result.front = *front;
            return result;
        }

        void validate_target_reference(const EventCameraTarget &target) {
            switch (target.kind) {
            case EventCameraTargetKind::Player:
                if (target.player == nullptr || target.player->camera_target() == nullptr) {
                    throw std::logic_error(
                        "Player-target event camera requires a live player camera-state provider.");
                }
                return;
            case EventCameraTargetKind::LiveActor:
                if (target.actor == nullptr || target.name_obj_identity == nullptr ||
                    target.name_obj_generation == 0U ||
                    smgpc::compat::name_obj_runtime_generation(
                        target.name_obj_identity) !=
                        target.name_obj_generation ||
                    !smgpc::compat::has_actor_runtime_state(target.actor)) {
                    throw std::logic_error(
                        "LiveActor-target event camera lost its runtime actor.");
                }
                if (target.actor->mFlag.mIsDead) {
                    throw std::logic_error(
                        "LiveActor-target event camera lost its runtime actor.");
                }
                return;
            case EventCameraTargetKind::Matrix:
                if (target.matrix == nullptr ||
                    target.name_obj_identity == nullptr ||
                    target.name_obj_generation == 0U ||
                    smgpc::compat::name_obj_runtime_generation(
                        target.name_obj_identity) !=
                        target.name_obj_generation) {
                    throw std::logic_error(
                        "Matrix-target event camera lost its runtime target.");
                }
                return;
            case EventCameraTargetKind::Retain:
                break;
            }
            throw std::logic_error(
                "Event camera has no retained target to calculate from.");
        }

        [[nodiscard]] TargetSnapshot snapshot_target(
            const EventCameraTarget &target) {
            validate_target_reference(target);
            switch (target.kind) {
            case EventCameraTargetKind::Player: {
                const auto state = target.player->camera_target_state();
                if (!state.has_value()) {
                    throw std::logic_error(
                        "Player-target event camera requires its target movement before calculation.");
                }
                const auto side = state->side.value_or(cross(state->up, state->front));
                return {
                    .position = state->position,
                    .side = side,
                    .up = state->up,
                    .front = state->front,
                    .last_move = state->last_move,
                    .ground_position = state->ground_position,
                    .gravity = state->gravity,
                    .jumping = state->jumping,
                    .fast_rise = state->fast_rise,
                    .fast_drop = state->fast_drop,
                };
            }
            case EventCameraTargetKind::LiveActor: {
                const auto &base = smgpc::compat::actor_base_matrix(target.actor).m;
                auto result = snapshot_from_matrix(
                    std::span<const float, 12U>(base),
                    {target.actor->mVelocity.x, target.actor->mVelocity.y,
                     target.actor->mVelocity.z});
                result.position = {target.actor->mPosition.x,
                                   target.actor->mPosition.y,
                                   target.actor->mPosition.z};
                return result;
            }
            case EventCameraTargetKind::Matrix: {
                const auto &mtx = target.matrix->mMatrix.mMtx;
                return snapshot_from_matrix(
                    std::span<const float, 12U>(&mtx[0][0], 12U),
                    {target.matrix->mLastMove.x, target.matrix->mLastMove.y,
                     target.matrix->mLastMove.z});
            }
            case EventCameraTargetKind::Retain:
                break;
            }
            throw std::logic_error("Event camera has no retained target to calculate from.");
        }

        void validate_target_snapshot(const TargetSnapshot &target) {
            const auto finite = [](const CameraParamVec3 &value) {
                return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
            };
            if (!finite(target.position) || !finite(target.up) || !finite(target.front) ||
                !finite(target.side) || !finite(target.last_move) ||
                dot(target.up, target.up) <= 1.0e-12F ||
                dot(target.front, target.front) <= 1.0e-12F || dot(target.side, target.side) <= 1.0e-12F ||
                (target.ground_position.has_value() && !finite(*target.ground_position)) ||
                (target.gravity.has_value() && !finite(*target.gravity))) {
                throw std::logic_error("Event camera target requires finite vectors and a non-degenerate basis.");
            }
        }

        [[nodiscard]] std::string lower_copy(std::string_view value) {
            auto result = std::string(value);
            std::ranges::transform(result, result.begin(), [](char character) {
                return static_cast<char>(std::tolower(
                    static_cast<unsigned char>(character)));
            });
            return result;
        }

        [[nodiscard]] std::int32_t start_interpolation_frames(
            const CameraParamChunk &param, std::int32_t requested_frames) {
            // CameraManEvent::getInterpolateFrame. The authored override
            // takes precedence, followed by the request and retail default.
            auto frames = param.event_enable_erp_frame != 0 ? param.extra.cam_int : -1;
            if (frames < 0 && requested_frames >= 0) {
                frames = requested_frames;
            }
            return frames < 0 ? 60 : frames;
        }

    }  // namespace

    EventCameraCatalog EventCameraCatalog::from_stage_tables(
        smgpc::runtime::DvdFileSystemService &dvd,
        std::span<const smgpc::scene::StagePlacementTable> tables) {
        auto result = EventCameraCatalog{};
        for (const auto &table : tables) {
            if (table.category != "camera" ||
                lower_copy(table.table_name) != "cameraparam.bcam") {
                continue;
            }
            const auto &archive = dvd.archive_for_path(table.archive_path);
            const auto chunks = load_camera_param_chunks(
                smgpc::resource::BcsvTable::from_bytes(
                    archive.resource_data(table.table_path)));
            for (const auto &chunk : chunks) {
                auto normalized_chunk = chunk;
                normalized_chunk.id =
                    smgpc::resource::decode_cp932(chunk.id);
                if (!normalized_chunk.id.starts_with("e:") ||
                    normalized_chunk.id.size() == 2U) {
                    continue;
                }
                auto key = EventCameraKey{.zone_id = table.zone_id,
                                          .name = normalized_chunk.id.substr(2U)};
                const auto [found, inserted] = result._definitions.try_emplace(
                    key,
                    StaticEventCameraDefinition{
                        .camera_param = normalized_chunk,
                        .zone_transform = table.zone_transform,
                        .archive_path = table.archive_path,
                        .holder_instance_id = table.holder_instance_id});
                if (!inserted &&
                    (found->second.camera_param.id != normalized_chunk.id ||
                     found->second.archive_path != table.archive_path ||
                     found->second.holder_instance_id !=
                         table.holder_instance_id ||
                     found->second.zone_transform.matrix !=
                         table.zone_transform.matrix)) {
                    throw std::runtime_error(
                        "Event-camera catalog contains conflicting holder occurrences for zone-qualified identity " +
                        std::to_string(key.zone_id) + ":" + key.name + ".");
                }
            }
        }
        return result;
    }

    const StaticEventCameraDefinition *EventCameraCatalog::find(
        std::int32_t zone_id, std::string_view name) const {
        const auto found =
            _definitions.find(EventCameraKey{zone_id, std::string(name)});
        return found == _definitions.end() ? nullptr : &found->second;
    }

    std::size_t EventCameraCatalog::size() const noexcept {
        return _definitions.size();
    }

    EventCameraTarget EventCameraTarget::retain() noexcept {
        return {};
    }

    EventCameraTarget EventCameraTarget::target_player(
        smgpc::runtime::PlayerSystemService &player) noexcept {
        return {.kind = EventCameraTargetKind::Player, .player = &player};
    }

    EventCameraTarget EventCameraTarget::target_actor(
        const LiveActor &actor) noexcept {
        const auto *identity = static_cast<const NameObj *>(&actor);
        return {.kind = EventCameraTargetKind::LiveActor,
                .actor = &actor,
                .name_obj_identity = identity,
                .name_obj_generation =
                    smgpc::compat::name_obj_runtime_generation(identity)};
    }

    EventCameraTarget EventCameraTarget::target_matrix(
        const CameraTargetMtx &matrix) noexcept {
        const auto *identity = static_cast<const NameObj *>(&matrix);
        return {.kind = EventCameraTargetKind::Matrix,
                .matrix = &matrix,
                .name_obj_identity = identity,
                .name_obj_generation =
                    smgpc::compat::name_obj_runtime_generation(identity)};
    }

    void EventCameraRuntime::attach_catalog(const EventCameraCatalog &catalog) {
        if (_catalog != nullptr && _catalog != &catalog) {
            throw std::logic_error(
                "Only one stage event-camera catalog may be active.");
        }
        _catalog = &catalog;
    }

    void EventCameraRuntime::detach_catalog(
        const EventCameraCatalog &catalog) noexcept {
        if (_catalog != &catalog) {
            return;
        }
        _catalog = nullptr;
        _active.reset();
        _last_target.reset();
        _animations.clear();
        _actor_camera_infos.clear();
        _declared_static.clear();
    }

    void EventCameraRuntime::declare_static(std::int32_t zone_id,
                                            std::string_view name) {
        if (name.empty()) {
            throw std::invalid_argument(
                "Event-camera declaration requires a non-empty name.");
        }
        _declared_static.emplace(zone_id, std::string(name));
    }

    void EventCameraRuntime::declare_animation(std::int32_t zone_id,
                                               std::string_view name,
                                               CameraAnimation animation) {
        if (name.empty()) {
            throw std::invalid_argument(
                "Animation event-camera declaration requires a non-empty name.");
        }
        if (animation.native_data().bytes().empty()) {
            throw std::invalid_argument("Animation event-camera declaration requires a decoded resource.");
        }
        _animations.insert_or_assign(
            EventCameraKey{zone_id, std::string(name)}, std::move(animation));
    }

    void EventCameraRuntime::start(std::int32_t zone_id, std::string_view name,
                                   EventCameraTarget target,
                                   std::int32_t interpolation_frames,
                                   float speed, const CameraPoseParam *game_seed) {
        if (name.empty() || !std::isfinite(speed) || !(speed > 0.0F)) {
            throw std::invalid_argument(
                "Event-camera start requires a name and positive finite speed.");
        }
        const auto key = EventCameraKey{zone_id, std::string(name)};
        const auto *animation = find_animation(key);
        const auto *definition =
            _catalog != nullptr ? _catalog->find(zone_id, name) : nullptr;
        if (animation == nullptr &&
            (!_declared_static.contains(key) || definition == nullptr)) {
            throw std::logic_error(
                "Zone-qualified event camera is not present in the active stage catalog.");
        }
        if (animation == nullptr &&
            definition->camera_param.camera_type != "CAM_TYPE_XZ_PARA" &&
            definition->camera_param.camera_type != "CAM_TYPE_EYEPOS_FIX") {
            throw std::logic_error("Unsupported event-camera type " +
                                   definition->camera_param.camera_type);
        }
        const auto has_explicit_target =
            target.kind != EventCameraTargetKind::Retain;
        if (!has_explicit_target) {
            if (!_last_target.has_value()) {
                throw std::logic_error(
                    "No-target event camera has no previous retail target to retain.");
            }
            target = *_last_target;
        }
        // CameraManEvent::start requests the chunk and sets its target. The
        // director advances that target before CameraManEvent::calc next frame.
        validate_target_reference(target);
        if (_active.has_value() && _active->key == key &&
            ((animation != nullptr && _active->animation) ||
             (animation == nullptr && !_active->animation && _active->controller))) {
            // CameraManEvent::checkReset preserves the controller when its
            // chunk and type are unchanged. Validate before changing either
            // current or retained target; pose advances only on movement.
            _active->target = target;
            _active->speed = speed;
            if (_active->animation_controller) {
                _active->animation_controller->set_speed(speed);
            }
            _active->interpolation_frames = interpolation_frames;
            if (has_explicit_target) {
                _last_target = target;
            }
            return;
        }
        // CameraDirector::startEvent copies the original game manager pose
        // only on entry; event-to-event requests retain the event manager pose.
        const CameraPoseParam *seed = game_seed;
        if (_active.has_value()) {
            if (_active->animation_controller) {
                seed = &_active->animation_controller->pose_param();
            } else if (_active->controller) {
                seed = &_active->controller->pose_param();
            } else {
                // A pending request has not updated the manager. Preserve
                // its full raw pose through further requests in this phase.
                seed = _active->manager_seed.get();
            }
        }
        std::shared_ptr<CameraPoseParam> manager_seed;
        if (seed != nullptr) {
            manager_seed = std::make_shared<CameraPoseParam>();
            manager_seed->copyFrom(*seed);
        }
        auto candidate = ActiveEvent{
            .key = key,
            .target = target,
            .manager_seed = std::move(manager_seed),
            .pose = active_pose(),
            .speed = speed,
            .interpolation_frames = interpolation_frames,
            .animation = animation != nullptr,
        };
        if (has_explicit_target) {
            _last_target = target;
        }
        _active = std::move(candidate);
    }

    void EventCameraRuntime::end(std::int32_t zone_id, std::string_view name,
                                 bool force,
                                 std::int32_t interpolation_frames) {
        (void)force;
        (void)interpolation_frames;
        if (_active.has_value() && _active->key.zone_id == zone_id &&
            _active->key.name == name) {
            _active.reset();
        }
    }

    void EventCameraRuntime::begin_frame(bool paused) {
        if (!_active.has_value() || paused) {
            return;
        }
        _active->pose = calculate_active_pose(*_active);
    }

    ActorCameraInfo *EventCameraRuntime::create_actor_camera_info(
        std::int32_t camera_set_id, std::int32_t zone_id) {
        if (_catalog == nullptr) {
            throw std::logic_error(
                "ActorCameraInfo allocation requires an active stage event-camera owner.");
        }
        auto info =
            std::make_unique<ActorCameraInfo>(camera_set_id, zone_id);
        auto *result = info.get();
        _actor_camera_infos.push_back(std::move(info));
        return result;
    }

    std::optional<CameraPose> EventCameraRuntime::active_pose() const {
        return _active.has_value() ? _active->pose : std::nullopt;
    }

    std::optional<EventCameraKey> EventCameraRuntime::active_key() const {
        return _active.has_value() ? std::optional{_active->key} : std::nullopt;
    }

    smgpc::runtime::PlayerSystemService *EventCameraRuntime::active_player_target() const {
        return _active.has_value() && _active->target.kind == EventCameraTargetKind::Player
                   ? _active->target.player : nullptr;
    }

    bool EventCameraRuntime::is_active(std::int32_t zone_id,
                                       std::string_view name) const {
        return _active.has_value() && _active->key.zone_id == zone_id &&
               _active->key.name == name;
    }

    bool EventCameraRuntime::is_declared(std::int32_t zone_id,
                                         std::string_view name) const {
        const auto key = EventCameraKey{zone_id, std::string(name)};
        return _declared_static.contains(key) || _animations.contains(key);
    }

    bool EventCameraRuntime::is_animation_end(std::int32_t zone_id,
                                              std::string_view name) const {
        if (!is_active(zone_id, name) || !_active->animation) {
            return true;
        }
        return _active->animation_controller != nullptr && _active->animation_controller->is_end();
    }

    std::int32_t EventCameraRuntime::animation_frame(
        std::int32_t zone_id, std::string_view name) const {
        return is_active(zone_id, name) && _active->animation_controller
                   ? static_cast<std::int32_t>(_active->animation_controller->current_frame()) : 0;
    }

    std::int32_t EventCameraRuntime::event_frames(
        std::int32_t zone_id, std::string_view name) const {
        const auto key = EventCameraKey{zone_id, std::string(name)};
        if (const auto *animation = find_animation(key); animation != nullptr) {
            return static_cast<std::int32_t>(animation->frame_count());
        }
        if (_catalog != nullptr) {
            if (const auto *definition = _catalog->find(zone_id, name);
                definition != nullptr) {
                return static_cast<std::int32_t>(
                    definition->camera_param.event_frame);
            }
        }
        return 0;
    }

    std::size_t EventCameraRuntime::actor_camera_info_count() const noexcept {
        return _actor_camera_infos.size();
    }

    CameraPose EventCameraRuntime::calculate_active_pose(ActiveEvent &active) {
        const auto target = snapshot_target(active.target);
        validate_target_snapshot(target);
        const auto published_target = StageCameraTargetState{
            .position = target.position, .up = target.up, .front = target.front,
            .last_move = target.last_move, .ground_position = target.ground_position,
            .gravity = target.gravity, .jumping = target.jumping,
            .fast_rise = target.fast_rise, .fast_drop = target.fast_drop, .side = target.side};
        if (active.animation) {
            const auto *animation = find_animation(active.key);
            if (animation == nullptr) {
                throw std::logic_error(
                    "Active CANM event camera lost its declaration.");
            }
            if (!active.animation_controller) {
                active.animation_controller = std::make_unique<OriginalAnimationCamera>(
                    *animation, published_target, active.speed, active.manager_seed.get());
            }
            return active.animation_controller->calc(published_target);
        }
        if (_catalog == nullptr) {
            throw std::logic_error(
                "Active static event camera lost its stage catalog.");
        }
        const auto *definition =
            _catalog->find(active.key.zone_id, active.key.name);
        if (definition == nullptr) {
            throw std::logic_error(
                "Active static event camera lost its zone-qualified chunk.");
        }
        if (definition->camera_param.camera_type == "CAM_TYPE_XZ_PARA" ||
            definition->camera_param.camera_type == "CAM_TYPE_EYEPOS_FIX") {
            if (!active.controller) {
                active.controller = std::make_unique<OriginalGameCamera>(
                    definition->zone_transform, definition->camera_param,
                    published_target, 45.0F,
                    StageCameraCalculationState{}, smgpc::compat::OriginalCameraMode::Event,
                    active.manager_seed.get(),
                    start_interpolation_frames(definition->camera_param, active.interpolation_frames) == 0);
            }
            return active.controller->calc(published_target).pose;
        }
        throw std::logic_error("Unsupported event-camera type " +
                               definition->camera_param.camera_type);
    }

    const CameraAnimation *EventCameraRuntime::find_animation(
        const EventCameraKey &key) const {
        const auto found = _animations.find(key);
        return found == _animations.end() ? nullptr : &found->second;
    }

}  // namespace smgpc::camera
