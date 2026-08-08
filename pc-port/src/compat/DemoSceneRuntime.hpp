#pragma once

#include "Game/NameObj/NameObj.hpp"
#include "compat/DemoSheetRuntime.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>

class JMapInfoIter;
class LayoutActor;
class LiveActor;
class Nerve;

namespace MR {
    class FunctorBase;
}

namespace smgpc::resource {
    class RarcArchive;
}

namespace smgpc::runtime {
    class DvdFileSystemService;
    class WipeService;
}  // namespace smgpc::runtime

namespace smgpc::scene {
    struct StageGeneralPos;
    struct StagePlacementObject;
}  // namespace smgpc::scene

namespace smgpc::compat {

    enum class DemoPlayerMode : std::uint8_t {
        Normal,
        MarioPuppetable,
    };

    struct DemoStageSwitches {
        std::int32_t appear = -1;
        std::int32_t dead = -1;
        std::int32_t a = -1;
        std::int32_t b = -1;
        std::int32_t sleep = -1;
    };

    // One source DemoGroup row. Instances remain in placement traversal order:
    // the original holders use the first matching executor rather than a
    // name-keyed map, so duplicate names and links must not overwrite.
    struct DemoSceneDefinition {
        std::int32_t zone_id = -1;
        std::int32_t group_link_id = -1;
        std::string demo_name;
        std::string time_sheet_name;
        DemoStageSwitches switches;
        std::string source_table_path;
        std::int32_t source_row = -1;
        DemoSheetRuntime sheet;
    };

    // DemoSubGroup owns no sheet. Its automatic registration path first joins
    // this zone/link group, then forwards the actor to the first primary
    // executor with the same localized name.
    struct DemoSceneSubGroupDefinition {
        std::int32_t zone_id = -1;
        std::int32_t group_link_id = -1;
        std::string demo_name;
        std::string source_table_path;
        std::int32_t source_row = -1;
    };

    // Scene-owned counterpart of the original DemoDirector/DemoExecutor
    // collection. This class owns definitions, the Time/SubPart clocks, cast
    // membership, registered callbacks, and the installed keeper row dispatch
    // at their original per-executor granularity.
    class DemoSceneRuntime final : public NameObj {
    public:
        DemoSceneRuntime(smgpc::runtime::DvdFileSystemService &dvd,
                         std::span<const smgpc::scene::StagePlacementObject> placements,
                         std::span<const smgpc::scene::StageGeneralPos> general_positions = {},
                         smgpc::runtime::WipeService *wipe_service = nullptr);
        DemoSceneRuntime(const smgpc::resource::RarcArchive &demo_sheet_archive,
                         std::span<const smgpc::scene::StagePlacementObject> placements,
                         std::span<const smgpc::scene::StageGeneralPos> general_positions = {},
                         smgpc::runtime::WipeService *wipe_service = nullptr);
        ~DemoSceneRuntime() override;

        DemoSceneRuntime(const DemoSceneRuntime &) = delete;
        DemoSceneRuntime &operator=(const DemoSceneRuntime &) = delete;

        void movement() override;

        [[nodiscard]] std::span<const DemoSceneDefinition> definitions() const;
        [[nodiscard]] std::span<const DemoSceneSubGroupDefinition> subgroups() const;
        [[nodiscard]] const DemoSceneDefinition *definition(std::size_t index) const;
        [[nodiscard]] std::optional<std::size_t> find_definition(std::int32_t zone_id,
                                                                 std::int32_t group_link_id) const;
        [[nodiscard]] std::optional<std::size_t> find_definition(std::string_view demo_name) const;
        [[nodiscard]] std::optional<std::size_t> find_subgroup(std::int32_t zone_id,
                                                               std::int32_t group_link_id) const;
        [[nodiscard]] std::optional<std::size_t> find_subgroup(std::string_view demo_name) const;

        [[nodiscard]] bool try_register_cast(LiveActor *actor, const JMapInfoIter &iter);
        [[nodiscard]] bool try_register_cast(LiveActor *actor, std::string_view demo_name,
                                             const JMapInfoIter &iter);
        void register_simple_cast(LiveActor *actor);
        void register_simple_cast(LayoutActor *actor);
        void register_simple_cast(NameObj *object);
        void release_actor(const LiveActor *actor);

        [[nodiscard]] std::optional<DemoSheetStartResult> start_demo(
            NameObj *starter, std::string_view demo_name,
            std::optional<std::string_view> part_name,
            DemoPlayerMode player_mode);
        [[nodiscard]] std::optional<DemoSheetStartResult> start_demo_registered(
            LiveActor *starter, std::optional<std::string_view> part_name,
            DemoPlayerMode player_mode);
        [[nodiscard]] bool stop_active_demo(
            const NameObj *starter,
            std::optional<std::string_view> demo_name);
        void release_puppetable_control(bool force_enable);
        void pause_time_keep(const LiveActor *actor);
        void resume_time_keep(const LiveActor *actor);

        [[nodiscard]] bool try_register_action_functor(const LiveActor *actor,
                                                       const MR::FunctorBase &functor,
                                                       std::optional<std::string_view> part_name);
        [[nodiscard]] bool try_register_action_functor(const LiveActor *actor,
                                                       std::string_view demo_name,
                                                       const MR::FunctorBase &functor,
                                                       std::optional<std::string_view> part_name);
        [[nodiscard]] bool try_register_action_nerve(const LiveActor *actor, const Nerve *nerve,
                                                     std::optional<std::string_view> part_name);

        [[nodiscard]] bool has_cast(const LiveActor *actor) const;
        [[nodiscard]] bool has_cast(const LiveActor *actor, std::string_view demo_name) const;
        [[nodiscard]] bool is_active() const;
        [[nodiscard]] bool is_active(std::string_view demo_name) const;
        [[nodiscard]] bool is_time_keep_active() const;
        [[nodiscard]] bool is_active_registered(const LiveActor *actor) const;
        [[nodiscard]] bool registered_demo_has_player_rows(
            const LiveActor *actor) const;
        [[nodiscard]] bool part_exists(const LiveActor *actor,
                                       std::string_view part_name) const;
        [[nodiscard]] bool is_part_active(std::string_view part_name) const;
        [[nodiscard]] bool is_demo_last_step() const;
        [[nodiscard]] std::optional<std::int32_t> part_step(
            std::string_view part_name) const;
        [[nodiscard]] std::optional<std::int32_t> part_total_step(
            std::string_view part_name) const;
        [[nodiscard]] std::optional<std::string_view> current_main_part_name(
            std::string_view demo_name) const;
        [[nodiscard]] std::string_view active_demo_name() const;
        [[nodiscard]] std::size_t membership_count(const LiveActor *actor) const;
        [[nodiscard]] std::size_t subgroup_membership_count(const LiveActor *actor) const;
        [[nodiscard]] std::size_t simple_cast_registration_count(
            const NameObj *object) const;
        [[nodiscard]] std::size_t action_count(const LiveActor *actor) const;
        [[nodiscard]] std::size_t action_count(const LiveActor *actor,
                                               std::string_view demo_name) const;
        [[nodiscard]] std::size_t functor_count(const LiveActor *actor,
                                                std::string_view demo_name) const;
        [[nodiscard]] std::size_t nerve_count(const LiveActor *actor,
                                              std::string_view demo_name) const;
        [[nodiscard]] std::optional<std::int32_t> cast_id(const LiveActor *actor,
                                                          std::size_t definition_index) const;
        [[nodiscard]] std::string_view cast_name(const LiveActor *actor,
                                                 std::size_t definition_index) const;

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };

    [[nodiscard]] DemoSceneRuntime *active_demo_scene_runtime();
    [[nodiscard]] DemoSceneRuntime &require_active_demo_scene_runtime(
        std::string_view operation);
    void release_actor_from_all_demo_scenes(const LiveActor *actor);
    [[nodiscard]] bool has_any_demo_scene_cast(const LiveActor *actor);
    [[nodiscard]] std::size_t demo_scene_membership_count(const LiveActor *actor);
    [[nodiscard]] std::size_t demo_scene_action_count(const LiveActor *actor);

    // Source-faithful DemoWipeKeeper row operation. Keeping this as a small
    // service-level primitive lets the scene dispatcher and focused tests use
    // the same arbitrary-name/raw-frame behavior.
    void dispatch_demo_wipe_row(const DemoWipeRow &row,
                                smgpc::runtime::WipeService &wipe);

}  // namespace smgpc::compat
