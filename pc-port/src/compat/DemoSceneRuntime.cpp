#include "compat/DemoSceneRuntime.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "resource/RarcArchive.hpp"
#include "resource/TextEncoding.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace smgpc::compat {
    namespace {

        constexpr auto cDemoSheetArchivePath = std::string_view{"/ObjectData/DemoSheet.arc"};
        constexpr auto cPrimaryDemoObjectName = std::string_view{"DemoGroup"};
        constexpr auto cSubDemoObjectName = std::string_view{"DemoSubGroup"};

        struct DefinitionSeed {
            std::int32_t zone_id = -1;
            std::int32_t group_link_id = -1;
            std::string demo_name;
            std::string time_sheet_name;
            DemoStageSwitches switches;
            std::string source_table_path;
            std::int32_t source_row = -1;
        };

        struct SubGroupSeed {
            std::int32_t zone_id = -1;
            std::int32_t group_link_id = -1;
            std::string demo_name;
            std::string source_table_path;
            std::int32_t source_row = -1;
        };

        struct CollectedSeeds {
            std::vector<DefinitionSeed> definitions;
            std::vector<SubGroupSeed> subgroups;
        };

        [[nodiscard]] std::string lower_ascii(std::string_view value) {
            auto lower = std::string(value);
            std::ranges::transform(lower, lower.begin(), [](unsigned char character) {
                return static_cast<char>(std::tolower(character));
            });
            return lower;
        }

        [[nodiscard]] bool is_demo_obj_info_path(std::string_view path) {
            auto lower = lower_ascii(path);
            std::ranges::replace(lower, '\\', '/');
            while (!lower.empty() && lower.back() == '/') {
                lower.pop_back();
            }
            return lower == "demoobjinfo" || lower.ends_with("/demoobjinfo");
        }

        void read_switch(const JMapInfoIter &iter, const char *field, std::int32_t &value) {
            value = -1;
            (void)iter.getValue(field, &value);
        }

        [[nodiscard]] CollectedSeeds collect_definition_seeds(
            std::span<const smgpc::scene::StagePlacementObject> placements) {
            auto collected = CollectedSeeds{};
            for (const auto &placement : placements) {
                if ((placement.object_name != cPrimaryDemoObjectName &&
                     placement.object_name != cSubDemoObjectName) ||
                    !is_demo_obj_info_path(placement.table_path)) {
                    continue;
                }

                const auto iter = JMapInfoIter(&placement.jmap_info, placement.jmap_entry_index);
                if (!iter.isValid()) {
                    throw std::runtime_error("Invalid DemoGroup placement iterator at " +
                                             placement.table_path + " row " +
                                             std::to_string(placement.jmap_entry_index));
                }

                const char *raw_demo_name = nullptr;
                const char *raw_sheet_name = nullptr;
                (void)iter.getValue("DemoName", &raw_demo_name);
                (void)iter.getValue("TimeSheetName", &raw_sheet_name);

                auto group_link_id = placement.l_id;
                (void)iter.getValue("l_id", &group_link_id);

                const auto demo_name = smgpc::resource::decode_cp932(
                    raw_demo_name != nullptr ? std::string_view(raw_demo_name) : std::string_view{});
                if (placement.object_name == cSubDemoObjectName) {
                    collected.subgroups.push_back(SubGroupSeed{
                        .zone_id = placement.zone_id,
                        .group_link_id = group_link_id,
                        .demo_name = demo_name,
                        .source_table_path = placement.table_path,
                        .source_row = placement.jmap_entry_index,
                    });
                    continue;
                }

                auto seed = DefinitionSeed{
                    .zone_id = placement.zone_id,
                    .group_link_id = group_link_id,
                    .demo_name = demo_name,
                    .time_sheet_name = raw_sheet_name != nullptr ? raw_sheet_name : "",
                    .source_table_path = placement.table_path,
                    .source_row = placement.jmap_entry_index,
                };
                read_switch(iter, "SW_APPEAR", seed.switches.appear);
                read_switch(iter, "SW_DEAD", seed.switches.dead);
                read_switch(iter, "SW_A", seed.switches.a);
                read_switch(iter, "SW_B", seed.switches.b);
                read_switch(iter, "SW_SLEEP", seed.switches.sleep);
                collected.definitions.push_back(std::move(seed));
            }
            return collected;
        }

        [[nodiscard]] std::string actor_name(const LiveActor *actor) {
            return actor != nullptr && actor->getName() != nullptr ? actor->getName() : "";
        }

        auto &installed_runtimes() {
            static auto runtimes = std::vector<DemoSceneRuntime *>{};
            return runtimes;
        }

    }  // namespace

    struct DemoSceneRuntime::Impl {
        struct Action {
            const Nerve *nerve = nullptr;
            std::unique_ptr<MR::FunctorBase> functor;
        };

        struct Cast {
            LiveActor *actor = nullptr;
            std::string name;
            std::int32_t cast_id = -1;
            // DemoActionKeeper registers an actor into matching Action rows at
            // each registerDemoActor call. Re-registering the same actor with
            // another CastId therefore retains the union of earlier matches.
            std::vector<std::uint8_t> targeted_rows;
            // Indexed exactly like DemoSceneDefinition::sheet.action_rows().
            // This preserves distinct callbacks for duplicate part names.
            std::vector<Action> actions;
        };

        std::vector<DemoSceneDefinition> definitions;
        std::vector<std::vector<Cast>> casts;
        std::vector<DemoSceneSubGroupDefinition> subgroups;
        std::vector<std::vector<LiveActor *>> subgroup_casts;

        [[nodiscard]] Cast *find_cast(std::size_t definition_index, const LiveActor *actor) {
            if (definition_index >= casts.size()) {
                return nullptr;
            }
            const auto found = std::ranges::find_if(casts[definition_index], [actor](const auto &cast) {
                return cast.actor == actor;
            });
            return found != casts[definition_index].end() ? &*found : nullptr;
        }

        [[nodiscard]] const Cast *find_cast(std::size_t definition_index,
                                            const LiveActor *actor) const {
            if (definition_index >= casts.size()) {
                return nullptr;
            }
            const auto found = std::ranges::find_if(casts[definition_index], [actor](const auto &cast) {
                return cast.actor == actor;
            });
            return found != casts[definition_index].end() ? &*found : nullptr;
        }

        [[nodiscard]] std::optional<std::pair<std::size_t, Cast *>> first_cast(
            const LiveActor *actor) {
            for (auto index = std::size_t{}; index < casts.size(); ++index) {
                if (auto *cast = find_cast(index, actor); cast != nullptr) {
                    return std::pair{index, cast};
                }
            }
            return std::nullopt;
        }

        void retain_action_targets(std::size_t definition_index, Cast &cast,
                                   LiveActor *actor, std::int32_t cast_id) {
            const auto name = actor_name(actor);
            const auto rows = definitions[definition_index].sheet.action_rows();
            for (auto index = std::size_t{}; index < rows.size(); ++index) {
                const auto &row = rows[index];
                if (row.cast_name == name && (row.cast_id < 0 || row.cast_id == cast_id)) {
                    cast.targeted_rows[index] = 1U;
                }
            }
            cast.name = name;
            cast.cast_id = cast_id;
        }

        [[nodiscard]] Cast *add_cast(std::size_t definition_index, LiveActor *actor,
                                     std::int32_t cast_id) {
            auto *cast = find_cast(definition_index, actor);
            if (cast == nullptr) {
                const auto row_count = definitions[definition_index].sheet.action_rows().size();
                casts[definition_index].push_back(Cast{
                    .actor = actor,
                    .targeted_rows = std::vector<std::uint8_t>(row_count),
                    .actions = std::vector<Action>(row_count),
                });
                cast = &casts[definition_index].back();
            }
            retain_action_targets(definition_index, *cast, actor, cast_id);
            return cast;
        }

        void add_subgroup_cast(std::size_t subgroup_index, LiveActor *actor) {
            auto &actors = subgroup_casts[subgroup_index];
            if (std::ranges::find(actors, actor) == actors.end()) {
                actors.push_back(actor);
            }
        }

        void load(std::span<const DefinitionSeed> seeds,
                  const smgpc::resource::RarcArchive &archive) {
            definitions.reserve(seeds.size());
            casts.reserve(seeds.size());
            for (const auto &seed : seeds) {
                try {
                    definitions.push_back(DemoSceneDefinition{
                        .zone_id = seed.zone_id,
                        .group_link_id = seed.group_link_id,
                        .demo_name = seed.demo_name,
                        .time_sheet_name = seed.time_sheet_name,
                        .switches = seed.switches,
                        .source_table_path = seed.source_table_path,
                        .source_row = seed.source_row,
                        .sheet = DemoSheetRuntime::load(archive, seed.time_sheet_name),
                    });
                    casts.emplace_back();
                } catch (const std::exception &error) {
                    throw std::runtime_error("Cannot load DemoGroup at " + seed.source_table_path +
                                             " row " + std::to_string(seed.source_row) +
                                             " (TimeSheetName='" + seed.time_sheet_name + "'): " +
                                             error.what());
                }
            }
        }

        void load_subgroups(std::span<const SubGroupSeed> seeds) {
            subgroups.reserve(seeds.size());
            subgroup_casts.reserve(seeds.size());
            for (const auto &seed : seeds) {
                subgroups.push_back(DemoSceneSubGroupDefinition{
                    .zone_id = seed.zone_id,
                    .group_link_id = seed.group_link_id,
                    .demo_name = seed.demo_name,
                    .source_table_path = seed.source_table_path,
                    .source_row = seed.source_row,
                });
                subgroup_casts.emplace_back();
            }
        }

        [[nodiscard]] bool action_targets_cast(std::size_t definition_index,
                                               std::size_t action_index,
                                               const Cast &cast) const {
            static_cast<void>(definition_index);
            return action_index < cast.targeted_rows.size() &&
                   cast.targeted_rows[action_index] != 0U;
        }

        [[nodiscard]] bool has_action_capability(std::size_t definition_index,
                                                 const Cast &cast,
                                                 std::int32_t action_type) const {
            const auto rows = definitions[definition_index].sheet.action_rows();
            for (auto index = std::size_t{}; index < rows.size(); ++index) {
                if (rows[index].action_type == action_type &&
                    action_targets_cast(definition_index, index, cast)) {
                    return true;
                }
            }
            return false;
        }

        template <typename Assign>
        void assign_actions(std::size_t definition_index, Cast &cast,
                            std::optional<std::string_view> part_name, Assign &&assign) {
            const auto rows = definitions[definition_index].sheet.action_rows();
            for (auto index = std::size_t{}; index < rows.size(); ++index) {
                if (!action_targets_cast(definition_index, index, cast) ||
                    (part_name.has_value() && rows[index].part_name != *part_name)) {
                    continue;
                }
                assign(cast.actions[index]);
            }
        }
    };

    namespace {

        void install_runtime(DemoSceneRuntime &runtime) {
            MR::connectToScene(&runtime, MR::MovementType_DemoDirector, -1, -1, -1);
            try {
                installed_runtimes().push_back(&runtime);
            } catch (...) {
                MR::disconnectToScene(&runtime);
                throw;
            }
        }

        template <typename Cast>
        void trace_cast_event(const char *event, const DemoSceneDefinition &definition,
                              const Cast &cast,
                              std::string_view action_name = {}) {
#ifndef NDEBUG
            if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr) {
                runtime->emit_semantic_trace_event(
                    "demo", event,
                    "actor=" + actor_name(cast.actor) + ";demo=" + definition.demo_name +
                        ";zone=" + std::to_string(definition.zone_id) +
                        ";group=" + std::to_string(definition.group_link_id) +
                        ";cast=" + std::to_string(cast.cast_id) +
                        (!action_name.empty() ? ";action=" + std::string(action_name) : std::string{}));
            }
#else
            static_cast<void>(event);
            static_cast<void>(definition);
            static_cast<void>(cast);
            static_cast<void>(action_name);
#endif
        }

    }  // namespace

    DemoSceneRuntime::DemoSceneRuntime(
        smgpc::runtime::DvdFileSystemService &dvd,
        std::span<const smgpc::scene::StagePlacementObject> placements)
        : NameObj("DemoDirector"), _impl(std::make_unique<Impl>()) {
        const auto seeds = collect_definition_seeds(placements);
        if (!seeds.definitions.empty()) {
            try {
                _impl->load(seeds.definitions, dvd.archive(cDemoSheetArchivePath));
            } catch (const std::exception &error) {
                throw std::runtime_error("Cannot initialize scene DemoSheet definitions: " +
                                         std::string(error.what()));
            }
        }
        _impl->load_subgroups(seeds.subgroups);
        install_runtime(*this);
    }

    DemoSceneRuntime::DemoSceneRuntime(
        const smgpc::resource::RarcArchive &demo_sheet_archive,
        std::span<const smgpc::scene::StagePlacementObject> placements)
        : NameObj("DemoDirector"), _impl(std::make_unique<Impl>()) {
        const auto seeds = collect_definition_seeds(placements);
        _impl->load(seeds.definitions, demo_sheet_archive);
        _impl->load_subgroups(seeds.subgroups);
        install_runtime(*this);
    }

    DemoSceneRuntime::~DemoSceneRuntime() {
        auto &runtimes = installed_runtimes();
        runtimes.erase(std::remove(runtimes.begin(), runtimes.end(), this), runtimes.end());
        MR::disconnectToScene(this);
    }

    void DemoSceneRuntime::movement() {
        // DemoSheetRuntime advancement and keeper dispatch are the next slice.
    }

    std::span<const DemoSceneDefinition> DemoSceneRuntime::definitions() const {
        return _impl->definitions;
    }

    std::span<const DemoSceneSubGroupDefinition> DemoSceneRuntime::subgroups() const {
        return _impl->subgroups;
    }

    const DemoSceneDefinition *DemoSceneRuntime::definition(std::size_t index) const {
        return index < _impl->definitions.size() ? &_impl->definitions[index] : nullptr;
    }

    std::optional<std::size_t> DemoSceneRuntime::find_definition(
        std::int32_t zone_id, std::int32_t group_link_id) const {
        for (auto index = std::size_t{}; index < _impl->definitions.size(); ++index) {
            const auto &definition = _impl->definitions[index];
            if (definition.zone_id == zone_id && definition.group_link_id == group_link_id) {
                return index;
            }
        }
        return std::nullopt;
    }

    std::optional<std::size_t> DemoSceneRuntime::find_subgroup(
        std::int32_t zone_id, std::int32_t group_link_id) const {
        for (auto index = std::size_t{}; index < _impl->subgroups.size(); ++index) {
            const auto &subgroup = _impl->subgroups[index];
            if (subgroup.zone_id == zone_id && subgroup.group_link_id == group_link_id) {
                return index;
            }
        }
        return std::nullopt;
    }

    std::optional<std::size_t> DemoSceneRuntime::find_subgroup(
        std::string_view demo_name) const {
        for (auto index = std::size_t{}; index < _impl->subgroups.size(); ++index) {
            if (_impl->subgroups[index].demo_name == demo_name) {
                return index;
            }
        }
        return std::nullopt;
    }

    std::optional<std::size_t> DemoSceneRuntime::find_definition(
        std::string_view demo_name) const {
        for (auto index = std::size_t{}; index < _impl->definitions.size(); ++index) {
            if (_impl->definitions[index].demo_name == demo_name) {
                return index;
            }
        }
        return std::nullopt;
    }

    bool DemoSceneRuntime::try_register_cast(LiveActor *actor, const JMapInfoIter &iter) {
        if (actor == nullptr || !iter.isValid()) {
            return false;
        }

        auto group_id = std::int32_t{-1};
        if (!MR::getJMapInfoDemoGroupID(iter, &group_id) || group_id < 0) {
            return false;
        }
        const auto found_definition = find_definition(MR::getPlacedZoneId(iter), group_id);
        if (found_definition.has_value()) {
            auto *cast = _impl->add_cast(*found_definition, actor, MR::getDemoCastID(iter));
            trace_cast_event("cast_registered", _impl->definitions[*found_definition], *cast);
            return true;
        }

        const auto found_subgroup = find_subgroup(MR::getPlacedZoneId(iter), group_id);
        if (!found_subgroup.has_value()) {
            return false;
        }
        _impl->add_subgroup_cast(*found_subgroup, actor);
        const auto forwarded_definition = find_definition(_impl->subgroups[*found_subgroup].demo_name);
        if (!forwarded_definition.has_value()) {
            return false;
        }
        auto *cast = _impl->add_cast(*forwarded_definition, actor, MR::getDemoCastID(iter));
        trace_cast_event("subgroup_cast_forwarded", _impl->definitions[*forwarded_definition], *cast);
        return true;
    }

    bool DemoSceneRuntime::try_register_cast(LiveActor *actor, std::string_view demo_name,
                                             const JMapInfoIter &iter) {
        // The original explicit-name overload matches the holder by name and
        // still registers with an invalid iterator (whose optional CastId is
        // consequently the -1 sentinel).
        if (actor == nullptr) {
            return false;
        }
        const auto found_definition = find_definition(demo_name);
        if (found_definition.has_value()) {
            auto *cast = _impl->add_cast(*found_definition, actor, MR::getDemoCastID(iter));
            trace_cast_event("cast_registered", _impl->definitions[*found_definition], *cast);
            return true;
        }
        const auto found_subgroup = find_subgroup(demo_name);
        if (!found_subgroup.has_value()) {
            return false;
        }
        // The original named overload resolves the subgroup holder's base
        // implementation, which joins only the subgroup and does not forward.
        _impl->add_subgroup_cast(*found_subgroup, actor);
        return true;
    }

    void DemoSceneRuntime::release_actor(const LiveActor *actor) {
        for (auto &casts : _impl->casts) {
            casts.erase(std::remove_if(casts.begin(), casts.end(), [actor](const auto &cast) {
                            return cast.actor == actor;
                        }),
                        casts.end());
        }
        for (auto &casts : _impl->subgroup_casts) {
            casts.erase(std::remove(casts.begin(), casts.end(), actor), casts.end());
        }
    }

    bool DemoSceneRuntime::try_register_action_functor(
        const LiveActor *actor, const MR::FunctorBase &functor,
        std::optional<std::string_view> part_name) {
        const auto found = _impl->first_cast(actor);
        if (!found.has_value() ||
            !_impl->has_action_capability(found->first, *found->second, 2)) {
            return false;
        }
        _impl->assign_actions(found->first, *found->second, part_name,
                              [&functor](auto &action) {
                                  action.functor.reset(functor.clone(nullptr));
                              });
        trace_cast_event("functor_registered", _impl->definitions[found->first],
                         *found->second, part_name.value_or(std::string_view{}));
        return true;
    }

    bool DemoSceneRuntime::try_register_action_functor(
        const LiveActor *actor, std::string_view demo_name, const MR::FunctorBase &functor,
        std::optional<std::string_view> part_name) {
        const auto found_definition = find_definition(demo_name);
        if (!found_definition.has_value()) {
            return false;
        }
        auto *cast = _impl->find_cast(*found_definition, actor);
        if (cast == nullptr || !_impl->has_action_capability(*found_definition, *cast, 2)) {
            return false;
        }
        _impl->assign_actions(*found_definition, *cast, part_name,
                              [&functor](auto &action) {
                                  action.functor.reset(functor.clone(nullptr));
                              });
        trace_cast_event("functor_registered", _impl->definitions[*found_definition], *cast,
                         part_name.value_or(std::string_view{}));
        return true;
    }

    bool DemoSceneRuntime::try_register_action_nerve(
        const LiveActor *actor, const Nerve *nerve,
        std::optional<std::string_view> part_name) {
        const auto found = _impl->first_cast(actor);
        if (!found.has_value() ||
            !_impl->has_action_capability(found->first, *found->second, 3)) {
            return false;
        }
        _impl->assign_actions(found->first, *found->second, part_name,
                              [nerve](auto &action) { action.nerve = nerve; });
        trace_cast_event("nerve_registered", _impl->definitions[found->first],
                         *found->second, part_name.value_or(std::string_view{}));
        return true;
    }

    bool DemoSceneRuntime::has_cast(const LiveActor *actor) const {
        return membership_count(actor) != 0U;
    }

    bool DemoSceneRuntime::has_cast(const LiveActor *actor, std::string_view demo_name) const {
        const auto found_definition = find_definition(demo_name);
        return found_definition.has_value() &&
               _impl->find_cast(*found_definition, actor) != nullptr;
    }

    std::size_t DemoSceneRuntime::membership_count(const LiveActor *actor) const {
        auto count = std::size_t{};
        for (auto index = std::size_t{}; index < _impl->casts.size(); ++index) {
            count += _impl->find_cast(index, actor) != nullptr ? 1U : 0U;
        }
        return count;
    }

    std::size_t DemoSceneRuntime::subgroup_membership_count(const LiveActor *actor) const {
        auto count = std::size_t{};
        for (const auto &casts : _impl->subgroup_casts) {
            count += std::ranges::find(casts, actor) != casts.end() ? 1U : 0U;
        }
        return count;
    }

    std::size_t DemoSceneRuntime::action_count(const LiveActor *actor) const {
        auto count = std::size_t{};
        for (auto index = std::size_t{}; index < _impl->casts.size(); ++index) {
            if (const auto *cast = _impl->find_cast(index, actor); cast != nullptr) {
                count += std::ranges::count_if(cast->actions, [](const auto &action) {
                    return action.nerve != nullptr || action.functor != nullptr;
                });
            }
        }
        return count;
    }

    std::size_t DemoSceneRuntime::action_count(const LiveActor *actor,
                                               std::string_view demo_name) const {
        const auto found_definition = find_definition(demo_name);
        if (!found_definition.has_value()) {
            return 0U;
        }
        const auto *cast = _impl->find_cast(*found_definition, actor);
        return cast != nullptr ? static_cast<std::size_t>(std::ranges::count_if(
                                     cast->actions, [](const auto &action) {
                                         return action.nerve != nullptr || action.functor != nullptr;
                                     })) :
                                 0U;
    }

    std::size_t DemoSceneRuntime::functor_count(const LiveActor *actor,
                                                std::string_view demo_name) const {
        const auto found_definition = find_definition(demo_name);
        if (!found_definition.has_value()) {
            return 0U;
        }
        const auto *cast = _impl->find_cast(*found_definition, actor);
        return cast != nullptr ? static_cast<std::size_t>(std::ranges::count_if(
                                     cast->actions, [](const auto &action) {
                                         return action.functor != nullptr;
                                     })) :
                                 0U;
    }

    std::size_t DemoSceneRuntime::nerve_count(const LiveActor *actor,
                                              std::string_view demo_name) const {
        const auto found_definition = find_definition(demo_name);
        if (!found_definition.has_value()) {
            return 0U;
        }
        const auto *cast = _impl->find_cast(*found_definition, actor);
        return cast != nullptr ? static_cast<std::size_t>(std::ranges::count_if(
                                     cast->actions, [](const auto &action) {
                                         return action.nerve != nullptr;
                                     })) :
                                 0U;
    }

    std::optional<std::int32_t> DemoSceneRuntime::cast_id(
        const LiveActor *actor, std::size_t definition_index) const {
        const auto *cast = _impl->find_cast(definition_index, actor);
        return cast != nullptr ? std::optional<std::int32_t>(cast->cast_id) : std::nullopt;
    }

    std::string_view DemoSceneRuntime::cast_name(const LiveActor *actor,
                                                 std::size_t definition_index) const {
        const auto *cast = _impl->find_cast(definition_index, actor);
        return cast != nullptr ? std::string_view(cast->name) : std::string_view{};
    }

    DemoSceneRuntime *active_demo_scene_runtime() {
        const auto &runtimes = installed_runtimes();
        return !runtimes.empty() ? runtimes.back() : nullptr;
    }

    void release_actor_from_all_demo_scenes(const LiveActor *actor) {
        for (auto *runtime : installed_runtimes()) {
            runtime->release_actor(actor);
        }
    }

    bool has_any_demo_scene_cast(const LiveActor *actor) {
        return std::ranges::any_of(installed_runtimes(), [actor](const auto *runtime) {
            return runtime->has_cast(actor);
        });
    }

    std::size_t demo_scene_membership_count(const LiveActor *actor) {
        auto count = std::size_t{};
        for (const auto *runtime : installed_runtimes()) {
            count += runtime->membership_count(actor);
        }
        return count;
    }

    std::size_t demo_scene_action_count(const LiveActor *actor) {
        auto count = std::size_t{};
        for (const auto *runtime : installed_runtimes()) {
            count += runtime->action_count(actor);
        }
        return count;
    }

}  // namespace smgpc::compat
