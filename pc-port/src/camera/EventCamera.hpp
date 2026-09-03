#pragma once

#include "Game/LiveActor/ActorCameraInfo.hpp"
#include "camera/CameraAnimation.hpp"
#include "camera/CameraPose.hpp"
#include "camera/OriginalGameCamera.hpp"
#include "camera/StageStartCamera.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <compare>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <vector>

class CameraTargetMtx;
class LiveActor;
class NameObj;

namespace smgpc::runtime {
    class DvdFileSystemService;
    class PlayerSystemService;
}  // namespace smgpc::runtime

namespace smgpc::camera {

    struct EventCameraKey {
        std::int32_t zone_id = 0;
        std::string name{};

        auto operator<=>(const EventCameraKey &) const = default;
    };

    struct StaticEventCameraDefinition {
        CameraParamChunk camera_param{};
        smgpc::scene::StageZoneTransform zone_transform{};
        std::string archive_path{};
        std::size_t holder_instance_id = 0U;
    };

    class EventCameraCatalog final {
    public:
        [[nodiscard]] static EventCameraCatalog from_stage_tables(
            smgpc::runtime::DvdFileSystemService &dvd,
            std::span<const smgpc::scene::StagePlacementTable> tables);

        [[nodiscard]] const StaticEventCameraDefinition *find(
            std::int32_t zone_id, std::string_view name) const;
        [[nodiscard]] std::size_t size() const noexcept;

    private:
        std::map<EventCameraKey, StaticEventCameraDefinition> _definitions{};
    };

    enum class EventCameraTargetKind {
        Retain,
        Player,
        LiveActor,
        Matrix,
    };

    struct EventCameraTarget {
        EventCameraTargetKind kind = EventCameraTargetKind::Retain;
        smgpc::runtime::PlayerSystemService *player = nullptr;
        const LiveActor *actor = nullptr;
        const CameraTargetMtx *matrix = nullptr;
        const NameObj *name_obj_identity = nullptr;
        std::uint64_t name_obj_generation = 0U;

        [[nodiscard]] static EventCameraTarget retain() noexcept;
        [[nodiscard]] static EventCameraTarget target_player(
            smgpc::runtime::PlayerSystemService &player) noexcept;
        [[nodiscard]] static EventCameraTarget target_actor(
            const LiveActor &actor) noexcept;
        [[nodiscard]] static EventCameraTarget target_matrix(
            const CameraTargetMtx &matrix) noexcept;
    };

    class EventCameraRuntime final {
    public:
        void attach_catalog(const EventCameraCatalog &catalog);
        void detach_catalog(const EventCameraCatalog &catalog) noexcept;

        void declare_static(std::int32_t zone_id, std::string_view name);
        void declare_animation(std::int32_t zone_id, std::string_view name,
                               CameraAnimation animation);
        void start(std::int32_t zone_id, std::string_view name,
                   EventCameraTarget target, std::int32_t interpolation_frames,
                   float speed = 1.0F);
        void end(std::int32_t zone_id, std::string_view name, bool force,
                 std::int32_t interpolation_frames);
        void begin_frame(bool paused);
        [[nodiscard]] ActorCameraInfo *create_actor_camera_info(
            std::int32_t camera_set_id, std::int32_t zone_id);

        [[nodiscard]] std::optional<CameraPose> active_pose() const;
        [[nodiscard]] std::optional<EventCameraKey> active_key() const;
        [[nodiscard]] smgpc::runtime::PlayerSystemService *active_player_target() const;
        [[nodiscard]] bool is_active(std::int32_t zone_id,
                                     std::string_view name) const;
        [[nodiscard]] bool is_declared(std::int32_t zone_id,
                                       std::string_view name) const;
        [[nodiscard]] bool is_animation_end(std::int32_t zone_id,
                                            std::string_view name) const;
        [[nodiscard]] std::int32_t animation_frame(
            std::int32_t zone_id, std::string_view name) const;
        [[nodiscard]] std::int32_t event_frames(std::int32_t zone_id,
                                                std::string_view name) const;
        [[nodiscard]] std::size_t actor_camera_info_count() const noexcept;

    private:
        struct ActiveEvent {
            EventCameraKey key{};
            EventCameraTarget target{};
            std::unique_ptr<OriginalGameCamera> controller;
            std::optional<CameraPose> pose;
            float animation_frame = 0.0F;
            float speed = 1.0F;
            std::int32_t interpolation_frames = 0;
            bool animation = false;
        };

        [[nodiscard]] CameraPose calculate_active_pose(ActiveEvent &active);
        [[nodiscard]] const CameraAnimation *find_animation(
            const EventCameraKey &key) const;

        const EventCameraCatalog *_catalog = nullptr;
        std::set<EventCameraKey> _declared_static{};
        std::map<EventCameraKey, CameraAnimation> _animations{};
        std::vector<std::unique_ptr<ActorCameraInfo>> _actor_camera_infos{};
        std::optional<ActiveEvent> _active{};
        std::optional<EventCameraTarget> _last_target{};
    };

}  // namespace smgpc::camera
