#include "camera/EventCamera.hpp"

#include "Game/Camera/CameraTargetMtx.hpp"
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

        constexpr auto cPi = 3.14159265358979323846F;

        struct TargetSnapshot {
            CameraParamVec3 position{};
            CameraParamVec3 side{1.0F, 0.0F, 0.0F};
            CameraParamVec3 up{0.0F, 1.0F, 0.0F};
            CameraParamVec3 front{0.0F, 0.0F, 1.0F};
            CameraParamVec3 last_move{};
        };

        [[nodiscard]] CameraParamVec3 add(const CameraParamVec3 &lhs,
                                          const CameraParamVec3 &rhs) {
            return {.x = lhs.x + rhs.x, .y = lhs.y + rhs.y, .z = lhs.z + rhs.z};
        }

        [[nodiscard]] CameraParamVec3 subtract(const CameraParamVec3 &lhs,
                                               const CameraParamVec3 &rhs) {
            return {.x = lhs.x - rhs.x, .y = lhs.y - rhs.y, .z = lhs.z - rhs.z};
        }

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

        [[nodiscard]] CameraParamVec3 camera_vec(
            const std::array<float, 3U> &value) {
            return {.x = value[0], .y = value[1], .z = value[2]};
        }

        [[nodiscard]] CameraParamVec3 transform_vector(
            const smgpc::scene::StageZoneTransform &transform,
            const CameraParamVec3 &value) {
            return camera_vec(
                transform.transform_vector({value.x, value.y, value.z}));
        }

        [[nodiscard]] CameraParamVec3 transform_point(
            const smgpc::scene::StageZoneTransform &transform,
            const CameraParamVec3 &value) {
            return camera_vec(
                transform.transform_point({value.x, value.y, value.z}));
        }

        [[nodiscard]] CameraParamVec3 transform_target_point(
            const TargetSnapshot &target, const CameraParamVec3 &point) {
            return add(target.position,
                       add(add(scale(target.side, point.x),
                               scale(target.up, point.y)),
                           scale(target.front, point.z)));
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

        [[nodiscard]] TargetSnapshot snapshot_target(
            const EventCameraTarget &target) {
            switch (target.kind) {
            case EventCameraTargetKind::Player: {
                if (target.player == nullptr ||
                    !target.player->has_base_matrix()) {
                    throw std::logic_error(
                        "Player-target event camera requires a live player base matrix.");
                }
                return snapshot_from_matrix(target.player->base_matrix(),
                                            camera_vec(std::array<float, 3U>{
                                                target.player->velocity()[0U],
                                                target.player->velocity()[1U],
                                                target.player->velocity()[2U]}));
            }
            case EventCameraTargetKind::LiveActor: {
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
                const auto &base =
                    smgpc::compat::actor_base_matrix(target.actor).m;
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
                if (target.matrix == nullptr ||
                    target.name_obj_identity == nullptr ||
                    target.name_obj_generation == 0U ||
                    smgpc::compat::name_obj_runtime_generation(
                        target.name_obj_identity) !=
                        target.name_obj_generation) {
                    throw std::logic_error(
                        "Matrix-target event camera lost its runtime target.");
                }
                const auto &mtx = target.matrix->mMatrix.mMtx;
                return snapshot_from_matrix(
                    std::span<const float, 12U>(&mtx[0][0], 12U),
                    {target.matrix->mLastMove.x, target.matrix->mLastMove.y,
                     target.matrix->mLastMove.z});
            }
            case EventCameraTargetKind::Retain:
                break;
            }
            throw std::logic_error(
                "Event camera has no retained target to calculate from.");
        }

        [[nodiscard]] CameraParamVec3 desired_local_offset(
            const CameraParamChunk &camera_param,
            const TargetSnapshot &target) {
            return add(scale(target.front, camera_param.extra.l_offset),
                       scale(target.up, camera_param.extra.l_offset_v));
        }

        [[nodiscard]] CameraPose calculate_eye_position_fixed(
            const StaticEventCameraDefinition &definition,
            const TargetSnapshot &target) {
            const auto &camera_param = definition.camera_param;
            const auto eye = transform_point(definition.zone_transform,
                                             camera_param.general.w_point);
            const auto watch = add(
                target.position,
                add(transform_vector(definition.zone_transform,
                                     camera_param.extra.w_offset),
                    desired_local_offset(camera_param, target)));

            auto raw_up = std::optional<CameraParamVec3>{};
            if (camera_param.general.num1 == 0) {
                raw_up = normalized(transform_vector(
                    definition.zone_transform, {0.0F, 1.0F, 0.0F}));
            } else if (camera_param.general.num1 == 2) {
                raw_up = normalized(target.up);
            } else {
                throw std::logic_error(
                    "EYEPOS_FIX event camera requires unsupported rotating-up mode 1.");
            }
            const auto forward = normalized(subtract(watch, eye));
            if (!raw_up.has_value() || !forward.has_value()) {
                throw std::logic_error(
                    "EYEPOS_FIX event camera has a degenerate authored basis.");
            }
            const auto right = normalized(cross(*forward, *raw_up));
            if (!right.has_value()) {
                throw std::logic_error(
                    "EYEPOS_FIX event camera eye and up axes are parallel.");
            }
            const auto corrected_up = normalized(cross(*right, *forward));
            const auto rolled_up = corrected_up.has_value() ? normalized(add(
                                                                  scale(*corrected_up,
                                                                        std::cos(camera_param.extra.roll)),
                                                                  scale(*right,
                                                                        -std::sin(camera_param.extra.roll)))) :
                                                              std::nullopt;
            if (!rolled_up.has_value()) {
                throw std::logic_error(
                    "EYEPOS_FIX event camera roll produced a degenerate up vector.");
            }
            return CameraPose{
                .eye = eye,
                .watch = watch,
                .up = *rolled_up,
                .fovy_degrees = camera_param.is_on_use_fovy() ? camera_param.extra.fovy : 45.0F,
            };
        }

        [[nodiscard]] CameraPose calculate_animation_pose(
            const CameraAnimation &animation, float frame,
            const TargetSnapshot &target) {
            const auto sample = animation.sample(frame);
            const auto eye = transform_target_point(target, sample.eye);
            const auto watch = transform_target_point(target, sample.watch);
            const auto forward = normalized(subtract(watch, eye));
            if (!forward.has_value()) {
                throw std::logic_error(
                    "CANM event camera has coincident eye and watch points.");
            }
            const auto right = normalized(cross(*forward, target.up));
            if (!right.has_value()) {
                throw std::logic_error(
                    "CANM event camera target up axis is parallel to its view.");
            }
            const auto corrected_up = normalized(cross(*right, *forward));
            const auto twist = sample.twist_degrees * cPi / 180.0F;
            const auto rolled_up = corrected_up.has_value() ? normalized(add(
                                                                  scale(*corrected_up, std::cos(twist)),
                                                                  scale(*right, -std::sin(twist)))) :
                                                              std::nullopt;
            if (!rolled_up.has_value() || !(sample.fovy_degrees > 0.0F)) {
                throw std::logic_error(
                    "CANM event camera produced an invalid pose.");
            }
            return CameraPose{.eye = eye,
                              .watch = watch,
                              .up = *rolled_up,
                              .fovy_degrees = sample.fovy_degrees};
        }

        [[nodiscard]] std::string lower_copy(std::string_view value) {
            auto result = std::string(value);
            std::ranges::transform(result, result.begin(), [](char character) {
                return static_cast<char>(std::tolower(
                    static_cast<unsigned char>(character)));
            });
            return result;
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
        const smgpc::runtime::PlayerSystemService &player) noexcept {
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
        _animations.insert_or_assign(
            EventCameraKey{zone_id, std::string(name)}, std::move(animation));
    }

    void EventCameraRuntime::start(std::int32_t zone_id, std::string_view name,
                                   EventCameraTarget target,
                                   std::int32_t interpolation_frames,
                                   float speed) {
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
        const auto has_explicit_target =
            target.kind != EventCameraTargetKind::Retain;
        if (!has_explicit_target) {
            if (!_last_target.has_value()) {
                throw std::logic_error(
                    "No-target event camera has no previous retail target to retain.");
            }
            target = *_last_target;
        }
        auto candidate = ActiveEvent{
            .key = key,
            .target = target,
            .speed = speed,
            .interpolation_frames = interpolation_frames,
            .animation = animation != nullptr,
        };
        candidate.pose = calculate_active_pose(candidate);
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
        if (!_active.has_value()) {
            return;
        }
        if (_active->animation && !paused) {
            _active->animation_frame += _active->speed;
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
        return _active.has_value() ? std::optional{_active->pose} : std::nullopt;
    }

    std::optional<EventCameraKey> EventCameraRuntime::active_key() const {
        return _active.has_value() ? std::optional{_active->key} : std::nullopt;
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
        const auto *animation = find_animation(_active->key);
        return animation == nullptr ||
               _active->animation_frame >= animation->frame_count();
    }

    std::int32_t EventCameraRuntime::animation_frame(
        std::int32_t zone_id, std::string_view name) const {
        return is_active(zone_id, name) && _active->animation ? static_cast<std::int32_t>(_active->animation_frame) : 0;
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
        if (active.animation) {
            const auto *animation = find_animation(active.key);
            if (animation == nullptr) {
                throw std::logic_error(
                    "Active CANM event camera lost its declaration.");
            }
            return calculate_animation_pose(*animation, active.animation_frame,
                                            target);
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
        if (definition->camera_param.camera_type == "CAM_TYPE_EYEPOS_FIX") {
            return calculate_eye_position_fixed(*definition, target);
        }
        if (definition->camera_param.camera_type == "CAM_TYPE_XZ_PARA") {
            if (!active.calculation_initialized) {
                active.calculation_state.local_offset =
                    desired_local_offset(definition->camera_param, target);
                active.calculation_initialized = true;
            }
            const auto calculation = calculate_stage_camera_pose(
                definition->zone_transform, definition->camera_param,
                StageCameraTargetState{.position = target.position,
                                       .up = target.up,
                                       .front = target.front,
                                       .last_move = target.last_move},
                active.calculation_state);
            if (!calculation.has_value()) {
                throw std::logic_error(
                    "XZ_PARA event camera has an invalid authored target basis.");
            }
            active.calculation_state = calculation->state;
            return calculation->pose;
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
