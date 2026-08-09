#include "scene/AuthoredPlacementInstantiator.hpp"

#include "Game/NameObj/NameObj.hpp"
#include "Game/NameObj/NameObjFactory.hpp"
#include "Game/Util/FileUtil.hpp"
#include "scene/AreaObjRuntime.hpp"
#include "scene/NameObjLifecycleService.hpp"
#include "scene/nameobj/PlanetMapCatalog.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <exception>
#include <iterator>
#include <numeric>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace smgpc::scene {
    namespace {

        [[nodiscard]] bool equal_case_insensitive(
            std::string_view left, std::string_view right) noexcept {
            return left.size() == right.size() &&
                   std::ranges::equal(
                       left, right, [](char left_character, char right_character) {
                           return std::tolower(static_cast<unsigned char>(
                                      left_character)) ==
                                  std::tolower(static_cast<unsigned char>(
                                      right_character));
                       });
        }

        [[nodiscard]] std::string_view table_basename(
            const StagePlacementObject &placement) noexcept {
            if (!placement.table_name.empty()) {
                return placement.table_name;
            }

            auto basename = std::string_view(placement.table_path);
            const auto slash = basename.find_last_of("/\\");
            if (slash != std::string_view::npos) {
                basename.remove_prefix(slash + 1U);
            }
            const auto extension = basename.find_last_of('.');
            if (extension != std::string_view::npos) {
                basename = basename.substr(0U, extension);
            }
            return basename;
        }

        [[nodiscard]] bool is_high_priority_table(
            const StagePlacementObject &placement) noexcept {
            constexpr auto names = std::array<std::string_view, 4U>{
                "AreaObjInfo", "PlanetObjInfo", "DemoObjInfo",
                "CameraCubeInfo"};
            const auto table_name = table_basename(placement);
            return std::ranges::any_of(names, [&](std::string_view name) {
                return equal_case_insensitive(table_name, name);
            });
        }

        [[nodiscard]] bool default_deferred_archive_decision(
            const StagePlacementObject &placement) {
            const auto *catalog =
                smgpc::scene::nameobj::PlanetMapCatalog::active();
            return catalog != nullptr &&
                   catalog->requires_scenario_selected_archive_load(
                       authored_placement_identifier(placement));
        }

        [[nodiscard]] AuthoredPlacementRetailPass retail_pass(
            const StagePlacementObject &placement,
            bool load_archive_after_scenario_selected) noexcept {
            const auto common = placement.load_batch ==
                                StagePlacementLoadBatch::CommonBootstrap;
            if (is_high_priority_table(placement)) {
                return common
                           ? AuthoredPlacementRetailPass::CommonHighPriority
                           : AuthoredPlacementRetailPass::ScenarioHighPriority;
            }
            if (!common) {
                return AuthoredPlacementRetailPass::ScenarioNormal;
            }
            return placement.shape_model_no == -1 &&
                           load_archive_after_scenario_selected
                       ? AuthoredPlacementRetailPass::DeferredCommonNormal
                       : AuthoredPlacementRetailPass::CommonNormal;
        }

        [[nodiscard]] bool is_deferred_archive_candidate(
            const StagePlacementObject &placement) noexcept {
            return placement.load_batch ==
                       StagePlacementLoadBatch::CommonBootstrap &&
                   !is_high_priority_table(placement) &&
                   placement.shape_model_no == -1;
        }

        [[nodiscard]] AuthoredPlacementGroupLoadOrder
        default_group_load_order(const StagePlacementObject &placement) {
            const auto identifier = authored_placement_identifier(placement);
            if (NameObjFactory::isPlayerArchiveLoaderObj(
                    identifier.data())) {
                return AuthoredPlacementGroupLoadOrder::PlayerArchiveLoader;
            }

            if (placement.shape_model_no != -1) {
                auto model_name = std::array<char, 128U>{};
                const auto written = std::snprintf(
                    model_name.data(), model_name.size(), "%s%02d",
                    placement.creator_identifier.c_str(),
                    placement.shape_model_no);
                if (written < 0 ||
                    static_cast<std::size_t>(written) >= model_name.size()) {
                    throw std::runtime_error(
                        "Model-changing authored placement identifier is too long.");
                }
                auto archive_path = std::array<char, 128U>{};
                if (!MR::makeObjectArchiveFileNameFromPrefix(
                        archive_path.data(), archive_path.size(),
                        model_name.data(), true)) {
                    return AuthoredPlacementGroupLoadOrder::ArchiveLoadRequired;
                }
                return MR::isLoadedFile(archive_path.data())
                           ? AuthoredPlacementGroupLoadOrder::ArchivesReady
                           : AuthoredPlacementGroupLoadOrder::ArchiveLoadRequired;
            }

            const auto iter = JMapInfoIter(
                &placement.jmap_info, placement.jmap_entry_index);
            if (!iter.isValid()) {
                throw std::logic_error(
                    "Authored placement ordering requires a retained JMap row.");
            }
            return NameObjFactory::isReadResourceFromDVD(
                       identifier.data(), iter)
                       ? AuthoredPlacementGroupLoadOrder::ArchiveLoadRequired
                       : AuthoredPlacementGroupLoadOrder::ArchivesReady;
        }

        [[nodiscard]] bool default_creator_available(
            const StagePlacementObject &placement) {
            // PC has not linked MR::getModelChangableObjCreator. Never let a
            // ShapeModelNo row borrow the ordinary NameObjFactory creator.
            if (placement.shape_model_no != -1) {
                return false;
            }
            return NameObjFactory::getCreator(
                       placement.creator_identifier.c_str()) != nullptr;
        }

        struct RetailPlacementGroup final {
            std::string_view identifier;
            s32 shape_model_no = -1;
            AuthoredPlacementGroupLoadOrder load_order =
                AuthoredPlacementGroupLoadOrder::ArchivesReady;
            std::vector<std::size_t> source_indices{};
        };

        struct RetailPlacementPlanEntry final {
            std::size_t source_index = 0U;
            AuthoredPlacementRetailPass pass =
                AuthoredPlacementRetailPass::CommonHighPriority;
            std::size_t group_index = 0U;
            AuthoredPlacementGroupLoadOrder group_load_order =
                AuthoredPlacementGroupLoadOrder::ArchivesReady;
        };

        constexpr auto retail_pass_count = std::size_t{5U};
        using RetailPlacementGroupsByPass =
            std::array<std::vector<RetailPlacementGroup>,
                       retail_pass_count>;

        [[nodiscard]] bool retail_group_precedes(
            const RetailPlacementGroup &candidate,
            const RetailPlacementGroup &prior) noexcept {
            if (candidate.load_order != prior.load_order) {
                return candidate.load_order < prior.load_order;
            }
            return candidate.source_indices.size() >
                   prior.source_indices.size();
        }

        void shell_sort_retail_groups(
            std::vector<RetailPlacementGroup> &groups) {
            // PlacementInfoOrdered::sort uses this exact Knuth-like gap
            // sequence. Equal load/count keys do not shift, which matters for
            // the sequence left by earlier non-unit gaps.
            auto gap = std::size_t{13U};
            while (gap < groups.size()) {
                gap = (gap * 3U) + 1U;
            }
            gap /= 9U;

            while (gap > 0U) {
                for (auto index = gap; index < groups.size(); ++index) {
                    auto candidate = std::move(groups[index]);
                    auto destination = index;
                    while (destination >= gap &&
                           retail_group_precedes(
                               candidate, groups[destination - gap])) {
                        groups[destination] =
                            std::move(groups[destination - gap]);
                        destination -= gap;
                    }
                    groups[destination] = std::move(candidate);
                }
                gap /= 3U;
            }
        }

        [[nodiscard]] RetailPlacementGroupsByPass
        collect_retail_placement_groups(
            std::span<const StagePlacementObject> placements,
            const AuthoredPlacementInstantiationOptions &options) {
            auto pass_groups =
                RetailPlacementGroupsByPass{};

            // The retained source vector is evidence storage, not attachment
            // policy. StageDataHolder attaches complete root arrays followed
            // by child arrays, and the resolver retains that ordinal.
            auto attachment_order =
                std::vector<std::size_t>(placements.size());
            std::iota(
                attachment_order.begin(), attachment_order.end(),
                std::size_t{});
            std::ranges::stable_sort(
                attachment_order,
                [&](std::size_t left_index, std::size_t right_index) {
                    const auto &left = placements[left_index];
                    const auto &right = placements[right_index];
                    if (left.load_batch != right.load_batch) {
                        return left.load_batch < right.load_batch;
                    }
                    if (left.placement_attachment_order !=
                        right.placement_attachment_order) {
                        return left.placement_attachment_order <
                               right.placement_attachment_order;
                    }
                    if (left.jmap_entry_index != right.jmap_entry_index) {
                        return left.jmap_entry_index <
                               right.jmap_entry_index;
                    }
                    return left_index < right_index;
                });

            for (const auto source_index : attachment_order) {
                const auto &placement = placements[source_index];
                auto deferred = false;
                if (is_deferred_archive_candidate(placement)) {
                    deferred = options.deferred_archive_resolver
                                   ? options.deferred_archive_resolver(
                                         placement)
                                   : default_deferred_archive_decision(
                                         placement);
                }
                const auto pass = retail_pass(placement, deferred);
                auto &groups =
                    pass_groups[static_cast<std::size_t>(pass)];
                auto group = std::ranges::find_if(
                    groups, [&](const RetailPlacementGroup &candidate) {
                        return candidate.identifier ==
                                   authored_placement_identifier(placement) &&
                               candidate.shape_model_no ==
                                   placement.shape_model_no;
                    });
                if (group == groups.end()) {
                    groups.push_back(RetailPlacementGroup{
                        .identifier =
                            authored_placement_identifier(placement),
                        .shape_model_no = placement.shape_model_no,
                    });
                    group = std::prev(groups.end());
                }
                group->source_indices.push_back(source_index);
            }

            return pass_groups;
        }

        void rank_and_sort_retail_pass(
            std::vector<RetailPlacementGroup> &groups,
            std::span<const StagePlacementObject> placements,
            const AuthoredPlacementInstantiationOptions &options) {
            for (auto &group : groups) {
                const auto &first =
                    placements[group.source_indices.front()];
                group.load_order = options.group_load_order_resolver
                                       ? options.group_load_order_resolver(first)
                                       : default_group_load_order(first);
            }
            shell_sort_retail_groups(groups);
        }

        [[nodiscard]] std::vector<RetailPlacementPlanEntry>
        flatten_retail_placement_plan(
            const RetailPlacementGroupsByPass &pass_groups,
            std::size_t placement_count) {
            auto plan = std::vector<RetailPlacementPlanEntry>{};
            plan.reserve(placement_count);
            for (auto pass_index = std::size_t{};
                 pass_index < pass_groups.size(); ++pass_index) {
                const auto &groups = pass_groups[pass_index];
                for (auto group_index = std::size_t{};
                     group_index < groups.size(); ++group_index) {
                    const auto &group = groups[group_index];
                    for (const auto source_index : group.source_indices) {
                        plan.push_back(RetailPlacementPlanEntry{
                            .source_index = source_index,
                            .pass = static_cast<
                                AuthoredPlacementRetailPass>(pass_index),
                            .group_index = group_index,
                            .group_load_order = group.load_order,
                        });
                    }
                }
            }
            return plan;
        }

        [[nodiscard]] std::string support_reason_or(
            const StagePlacementObject &placement,
            std::string_view fallback) {
            return placement.support_reason.empty() ? std::string(fallback) : placement.support_reason;
        }

        [[nodiscard]] std::string strict_preflight_error(
            const StageAuthoredData &data,
            const AuthoredPlacementInstantiationReport &report) {
            auto out = std::ostringstream{};
            out << "Authored placement preflight rejected "
                << data.stage_name() << " scenario " << data.scenario_no()
                << ": " << report.blocked_count << " blocked active rows";
            const auto first = std::ranges::find_if(
                report.entries, [](const auto &entry) {
                    return entry.support.kind ==
                           AuthoredPlacementSupportKind::Blocked;
                });
            if (first != report.entries.end() &&
                first->placement != nullptr) {
                out << "; first="
                    << authored_placement_identifier(*first->placement)
                    << "; raw_name=" << first->placement->object_name
                    << " in " << first->placement->table_path
                    << " row " << first->placement->jmap_entry_index
                    << " (" << first->support.reason << ')';
            }
            return out.str();
        }

        class NameObjPlacementLifecycleAdapter final
            : public AuthoredPlacementLifecycle {
        public:
            explicit NameObjPlacementLifecycleAdapter(
                NameObjLifecycleService &lifecycle)
                : _lifecycle(lifecycle) {
            }

            std::vector<smgpc::scene::nameobj::NameObjArchiveRequest>
            preload_archives(
                std::string_view object_name,
                const NameObjPlacementContext &placement) override {
                return _lifecycle.preload_archives(object_name, &placement);
            }

            std::vector<smgpc::scene::nameobj::NameObjArchiveRequest>
            preload_model_changing_archive(
                std::string_view object_name, s32 shape_model_no,
                const NameObjPlacementContext &placement) override {
                return _lifecycle.preload_model_changing_archive(
                    object_name, shape_model_no, &placement);
            }

            std::unique_ptr<NameObj> construct_and_init(
                std::string_view object_name, const char *actor_name,
                const NameObjPlacementContext &placement) override {
                return _lifecycle.construct_and_init(
                    object_name, actor_name, &placement);
            }

            std::unique_ptr<NameObj> construct_model_changing_and_init(
                std::string_view, s32, const char *,
                const NameObjPlacementContext &) override {
                throw std::runtime_error(
                    "Model-changing NameObj creator is unavailable on PC.");
            }

            void init_after_placement(NameObj &object) override {
                _lifecycle.init_after_placement(object);
            }

            void destroy(NameObj &object) override {
                _lifecycle.destroy(object);
            }

        private:
            NameObjLifecycleService &_lifecycle;
        };

    }  // namespace

    std::string_view authored_placement_identifier(
        const StagePlacementObject &placement) noexcept {
        return placement.creator_identifier;
    }

    AuthoredPlacementRetailPass authored_placement_retail_pass(
        const StagePlacementObject &placement) {
        return retail_pass(
            placement,
            is_deferred_archive_candidate(placement) &&
                default_deferred_archive_decision(placement));
    }

    AuthoredPlacementSupport classify_authored_placement(
        const StagePlacementObject &placement) {
        if (placement.intentionally_ignored) {
            return AuthoredPlacementSupport{
                .kind = AuthoredPlacementSupportKind::Ignored,
                .reason = support_reason_or(
                    placement, "intentionally_ignored"),
            };
        }

        if (placement.shape_model_no != -1) {
            return AuthoredPlacementSupport{
                .kind = AuthoredPlacementSupportKind::Blocked,
                .reason = "model_changing_creator_unavailable",
            };
        }

        if (placement_has_complete_area_obj_runtime(
                authored_placement_identifier(placement),
                placement.table_path,
                placement.factory_supported)) {
            return AuthoredPlacementSupport{
                .kind = AuthoredPlacementSupportKind::Ready,
                .reason = support_reason_or(placement, "original_factory"),
            };
        }

        if (placement.factory_supported &&
            is_area_obj_placement_table(placement.table_path)) {
            return AuthoredPlacementSupport{
                .kind = AuthoredPlacementSupportKind::Blocked,
                .reason =
                    "area_obj_creator_manager_closure_unavailable",
            };
        }

        return AuthoredPlacementSupport{
            .kind = AuthoredPlacementSupportKind::Blocked,
            .reason = support_reason_or(
                placement, "retail_creator_not_linked"),
        };
    }

    AuthoredPlacementInstantiator::AuthoredPlacementInstantiator(
        const StageAuthoredData &data,
        AuthoredPlacementLifecycle &lifecycle,
        AuthoredPlacementInstantiationOptions options)
        : _data(data), _lifecycle(&lifecycle),
          _options(std::move(options)) {
        build_report();
    }

    AuthoredPlacementInstantiator::AuthoredPlacementInstantiator(
        const StageAuthoredData &data, NameObjLifecycleService &lifecycle,
        AuthoredPlacementInstantiationOptions options)
        : _data(data),
          _owned_lifecycle(
              std::make_unique<NameObjPlacementLifecycleAdapter>(lifecycle)),
          _lifecycle(_owned_lifecycle.get()),
          _options(std::move(options)) {
        build_report();
    }

    AuthoredPlacementInstantiator::~AuthoredPlacementInstantiator() {
        clear_impl(false);
    }

    void AuthoredPlacementInstantiator::build_report() {
        _report = AuthoredPlacementInstantiationReport{
            .mode = _options.mode,
        };

        const auto placements = _data.placements();
        _report.entries.reserve(placements.size());
        for (auto source_index = std::size_t{};
             source_index < placements.size(); ++source_index) {
            const auto &placement = placements[source_index];
            auto support = _options.support_resolver ? _options.support_resolver(placement) : classify_authored_placement(placement);
            if (support.reason.empty()) {
                support.reason = "unspecified_support_result";
            }

            auto outcome = AuthoredPlacementOutcome::Pending;
            switch (support.kind) {
            case AuthoredPlacementSupportKind::Ready:
                ++_report.ready_count;
                break;
            case AuthoredPlacementSupportKind::Ignored:
                ++_report.ignored_count;
                outcome = AuthoredPlacementOutcome::Ignored;
                break;
            case AuthoredPlacementSupportKind::Blocked:
                ++_report.blocked_count;
                outcome = AuthoredPlacementOutcome::Blocked;
                break;
            }

            _report.entries.push_back(AuthoredPlacementReportEntry{
                .source_index = source_index,
                // Final deferred classification and group order are resolved
                // only during preload(), after Strict support preflight
                // has completed without consulting placement-order policy.
                .retail_pass = retail_pass(placement, false),
                .placement = &placement,
                .support = std::move(support),
                .outcome = outcome,
            });
        }
    }

    void AuthoredPlacementInstantiator::preflight_or_throw() {
        _report.preflight_accepted =
            _report.blocked_count == 0U ||
            _options.mode ==
                AuthoredPlacementMode::SupportedSubsetForDevelopment;
        if (_report.preflight_accepted) {
            return;
        }

        _report.state = AuthoredPlacementRuntimeState::Failed;
        throw std::runtime_error(strict_preflight_error(_data, _report));
    }

    const AuthoredPlacementInstantiationReport &
    AuthoredPlacementInstantiator::preflight() {
        if (_report.state != AuthoredPlacementRuntimeState::Prepared) {
            throw std::logic_error(
                "Authored placement preflight requires a prepared runtime.");
        }
        preflight_or_throw();
        return _report;
    }

    const AuthoredPlacementInstantiationReport &
    AuthoredPlacementInstantiator::preload() {
        if (_report.state != AuthoredPlacementRuntimeState::Prepared) {
            throw std::logic_error(
                "Authored placements can only be preloaded once.");
        }
        preflight_or_throw();

        AuthoredPlacementReportEntry *current_entry = nullptr;
        try {
            const auto placements = _data.placements();
            auto pass_groups =
                collect_retail_placement_groups(placements, _options);

            const auto preload_entry = [&](std::size_t source_index,
                                           s32 shape_model_no) {
                auto &entry = _report.entries[source_index];
                current_entry = &entry;
                const auto context = _data.placement_context(source_index);
                auto archives = shape_model_no == -1
                                    ? _lifecycle->preload_archives(
                                          authored_placement_identifier(
                                              *entry.placement),
                                          context)
                                    : _lifecycle
                                          ->preload_model_changing_archive(
                                              authored_placement_identifier(
                                                  *entry.placement),
                                              shape_model_no, context);
                if (!std::ranges::all_of(
                        archives, [](const auto &request) {
                            return request.loaded;
                        })) {
                    throw std::runtime_error(
                        "Authored placement archive preload returned an "
                        "unloaded request.");
                }
                entry.archives = std::move(archives);
                current_entry = nullptr;
            };

            const auto preload_group = [&](const RetailPlacementGroup &group) {
                const auto &first =
                    placements[group.source_indices.front()];
                const auto creator_available =
                    _options.creator_availability_resolver
                        ? _options.creator_availability_resolver(first)
                        : default_creator_available(first);
                if (!creator_available) {
                    return;
                }

                if (group.shape_model_no != -1) {
                    // Model-changing SameIdSets generate Identifier%02d and
                    // mount that archive once for the complete set.
                    preload_entry(
                        group.source_indices.front(), group.shape_model_no);
                    const auto shared_archives =
                        _report.entries[group.source_indices.front()].archives;
                    for (auto index = std::size_t{1U};
                         index < group.source_indices.size(); ++index) {
                        _report.entries[group.source_indices[index]].archives =
                            shared_archives;
                    }
                    return;
                }

                // Ordinary PlacementInfoOrdered sets request every row because
                // per-row archive-list callbacks can inspect JMap arguments.
                // Once the SameIdSet has an available creator, table-local PC
                // support differences do not filter those retail requests.
                for (const auto source_index : group.source_indices) {
                    preload_entry(source_index, -1);
                }
            };

            const auto rank_and_sort_pass =
                [&](AuthoredPlacementRetailPass pass) {
                    rank_and_sort_retail_pass(
                        pass_groups[static_cast<std::size_t>(pass)],
                        placements, _options);
                };
            const auto preload_pass = [&](AuthoredPlacementRetailPass pass) {
                for (const auto &group :
                     pass_groups[static_cast<std::size_t>(pass)]) {
                    preload_group(group);
                }
            };

            // Retail ranks both common holders from the same initial archive
            // state before either holder requests files.
            rank_and_sort_pass(
                AuthoredPlacementRetailPass::CommonHighPriority);
            rank_and_sort_pass(
                AuthoredPlacementRetailPass::CommonNormal);
            preload_pass(AuthoredPlacementRetailPass::CommonHighPriority);
            preload_pass(AuthoredPlacementRetailPass::CommonNormal);

            // All three post-scenario holders then snapshot the state left by
            // the complete common preload, before any of the three begins its
            // own requests.
            rank_and_sort_pass(
                AuthoredPlacementRetailPass::ScenarioHighPriority);
            rank_and_sort_pass(
                AuthoredPlacementRetailPass::ScenarioNormal);
            rank_and_sort_pass(
                AuthoredPlacementRetailPass::DeferredCommonNormal);
            preload_pass(AuthoredPlacementRetailPass::ScenarioHighPriority);
            preload_pass(AuthoredPlacementRetailPass::ScenarioNormal);
            preload_pass(AuthoredPlacementRetailPass::DeferredCommonNormal);

            const auto plan = flatten_retail_placement_plan(
                pass_groups, placements.size());
            auto source_entries = std::move(_report.entries);
            _report.entries.clear();
            _report.entries.reserve(plan.size());
            for (const auto &planned : plan) {
                auto entry =
                    std::move(source_entries[planned.source_index]);
                entry.retail_pass = planned.pass;
                entry.retail_group_index = planned.group_index;
                entry.retail_group_load_order =
                    planned.group_load_order;
                _report.entries.push_back(std::move(entry));
            }
            current_entry = nullptr;

            _report.state = AuthoredPlacementRuntimeState::Preloaded;
            return _report;
        } catch (const std::exception &error) {
            if (current_entry != nullptr) {
                current_entry->outcome = AuthoredPlacementOutcome::Failed;
                current_entry->failure_detail = error.what();
            }
            _report.state = AuthoredPlacementRuntimeState::Failed;
            clear_impl(false);
            throw;
        } catch (...) {
            if (current_entry != nullptr) {
                current_entry->outcome = AuthoredPlacementOutcome::Failed;
                current_entry->failure_detail = "unknown_lifecycle_failure";
            }
            _report.state = AuthoredPlacementRuntimeState::Failed;
            clear_impl(false);
            throw;
        }
    }

    const AuthoredPlacementInstantiationReport &
    AuthoredPlacementInstantiator::instantiate() {
        if (_report.state != AuthoredPlacementRuntimeState::Preloaded) {
            throw std::logic_error(
                "Authored placement construction requires one completed "
                "preload pass.");
        }

        _owned_instances.reserve(_report.ready_count);
        _instance_views.reserve(_report.ready_count);
        AuthoredPlacementReportEntry *current_entry = nullptr;
        try {
            auto group_begin = std::size_t{};
            while (group_begin < _report.entries.size()) {
                const auto pass =
                    _report.entries[group_begin].retail_pass;
                const auto group_index =
                    _report.entries[group_begin].retail_group_index;
                auto group_end = group_begin + 1U;
                while (group_end < _report.entries.size() &&
                       _report.entries[group_end].retail_pass == pass &&
                       _report.entries[group_end].retail_group_index ==
                           group_index) {
                    ++group_end;
                }

                auto &first_entry = _report.entries[group_begin];
                current_entry = &first_entry;
                const auto creator_available =
                    _options.creator_availability_resolver
                        ? _options.creator_availability_resolver(
                              *first_entry.placement)
                        : default_creator_available(*first_entry.placement);
                if (!creator_available) {
                    for (auto entry_index = group_begin;
                         entry_index < group_end; ++entry_index) {
                        auto &entry = _report.entries[entry_index];
                        if (entry.support.kind !=
                            AuthoredPlacementSupportKind::Ready) {
                            continue;
                        }
                        entry.support.kind =
                            AuthoredPlacementSupportKind::Blocked;
                        entry.support.reason =
                            "creator_unavailable_at_init_placement";
                        entry.outcome = AuthoredPlacementOutcome::Blocked;
                        --_report.ready_count;
                        ++_report.blocked_count;
                    }
                    current_entry = nullptr;
                    group_begin = group_end;
                    continue;
                }

                auto actor_name = std::optional<std::string>{};
                if (_options.actor_name_resolver) {
                    actor_name =
                        _options.actor_name_resolver(*first_entry.placement);
                }
                // PlacementInfoOrdered resolves the Japanese object name once
                // per SameIdSet. Each row retains report evidence, while every
                // constructor receives the first entry's one stable pointer.
                for (auto entry_index = group_begin;
                     entry_index < group_end; ++entry_index) {
                    _report.entries[entry_index].actor_name = actor_name;
                }
                const auto *group_actor_name =
                    first_entry.actor_name.has_value()
                        ? first_entry.actor_name->c_str()
                        : nullptr;
                current_entry = nullptr;

                for (auto entry_index = group_begin;
                     entry_index < group_end; ++entry_index) {
                    auto &entry = _report.entries[entry_index];
                    if (entry.support.kind !=
                        AuthoredPlacementSupportKind::Ready) {
                        continue;
                    }

                    current_entry = &entry;
                    const auto context =
                        _data.placement_context(entry.source_index);

                    auto actor = entry.placement->shape_model_no == -1
                                     ? _lifecycle->construct_and_init(
                                           authored_placement_identifier(
                                               *entry.placement),
                                           group_actor_name, context)
                                     : _lifecycle
                                           ->construct_model_changing_and_init(
                                               authored_placement_identifier(
                                                   *entry.placement),
                                               entry.placement->shape_model_no,
                                               group_actor_name, context);
                    if (actor == nullptr) {
                        throw std::runtime_error(
                            "Authored placement lifecycle returned a null actor.");
                    }

                    auto *actor_ptr = actor.get();
                    _owned_instances.push_back(OwnedInstance{
                        .report_index = entry_index,
                        .actor = std::move(actor),
                    });
                    _instance_views.push_back(AuthoredPlacementInstance{
                        .placement = entry.placement,
                        .actor = actor_ptr,
                    });
                    entry.actor = actor_ptr;
                    entry.outcome = AuthoredPlacementOutcome::Created;
                    ++_report.created_count;
                }
                group_begin = group_end;
            }
            _report.state = AuthoredPlacementRuntimeState::Instantiated;
            return _report;
        } catch (const std::exception &error) {
            if (current_entry != nullptr) {
                current_entry->outcome = AuthoredPlacementOutcome::Failed;
                current_entry->failure_detail = error.what();
            }
            _report.state = AuthoredPlacementRuntimeState::Failed;
            clear_impl(false);
            throw;
        } catch (...) {
            if (current_entry != nullptr) {
                current_entry->outcome = AuthoredPlacementOutcome::Failed;
                current_entry->failure_detail = "unknown_lifecycle_failure";
            }
            _report.state = AuthoredPlacementRuntimeState::Failed;
            clear_impl(false);
            throw;
        }
    }

    const AuthoredPlacementInstantiationReport &
    AuthoredPlacementInstantiator::init_after_placement() {
        if (_report.state !=
            AuthoredPlacementRuntimeState::Instantiated) {
            throw std::logic_error(
                "Authored initAfterPlacement requires a completed placement "
                "construction pass.");
        }

        for (auto &instance : _owned_instances) {
            auto &entry = _report.entries[instance.report_index];
            try {
                _lifecycle->init_after_placement(*instance.actor);
                entry.outcome =
                    AuthoredPlacementOutcome::InitializedAfterPlacement;
                ++_report.initialized_after_placement_count;
            } catch (const std::exception &error) {
                entry.outcome = AuthoredPlacementOutcome::Failed;
                entry.failure_detail = error.what();
                _report.state = AuthoredPlacementRuntimeState::Failed;
                throw;
            } catch (...) {
                entry.outcome = AuthoredPlacementOutcome::Failed;
                entry.failure_detail = "unknown_init_after_placement_failure";
                _report.state = AuthoredPlacementRuntimeState::Failed;
                throw;
            }
        }

        _report.state =
            AuthoredPlacementRuntimeState::InitializedAfterPlacement;
        return _report;
    }

    void AuthoredPlacementInstantiator::clear_impl(bool propagate_errors) {
        auto first_error = std::exception_ptr{};
        // Placement actors can retain non-owning pointers to actors created
        // before them (children, group peers, demo casts, follow targets).
        // Retire the dependency graph in the opposite direction from its
        // construction order.
        for (auto instance_iter = _owned_instances.rbegin();
             instance_iter != _owned_instances.rend(); ++instance_iter) {
            auto &instance = *instance_iter;
            auto &entry = _report.entries[instance.report_index];
            if (instance.actor != nullptr) {
                try {
                    _lifecycle->destroy(*instance.actor);
                } catch (...) {
                    if (first_error == nullptr) {
                        first_error = std::current_exception();
                    }
                }
                // Actual actor destructors can unregister or consult peers.
                // Delete each object at its reverse-order retirement point,
                // even when the lifecycle destroy hook failed.
                instance.actor.reset();
                ++_report.destroyed_count;
            }
            entry.actor = nullptr;
            if (entry.outcome != AuthoredPlacementOutcome::Failed) {
                entry.outcome = AuthoredPlacementOutcome::Destroyed;
            }
        }
        _instance_views.clear();
        _owned_instances.clear();

        if (propagate_errors && first_error != nullptr) {
            std::rethrow_exception(first_error);
        }
    }

    void AuthoredPlacementInstantiator::clear() {
        clear_impl(true);
        _report.state = AuthoredPlacementRuntimeState::Cleared;
    }

    const AuthoredPlacementInstantiationReport &
    AuthoredPlacementInstantiator::report() const noexcept {
        return _report;
    }

    std::span<const AuthoredPlacementInstance>
    AuthoredPlacementInstantiator::instances() const noexcept {
        return _instance_views;
    }

}  // namespace smgpc::scene
