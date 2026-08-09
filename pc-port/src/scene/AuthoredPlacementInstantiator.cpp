#include "scene/AuthoredPlacementInstantiator.hpp"

#include "Game/NameObj/NameObj.hpp"
#include "scene/AreaObjRuntime.hpp"
#include "scene/NameObjLifecycleService.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <exception>
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
                out << "; first=" << first->placement->object_name
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

            std::unique_ptr<NameObj> construct_and_init(
                std::string_view object_name, const char *actor_name,
                const NameObjPlacementContext &placement) override {
                return _lifecycle.construct_and_init(
                    object_name, actor_name, &placement);
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

    int authored_placement_retail_phase(
        const StagePlacementObject &placement) noexcept {
        const auto common =
            equal_case_insensitive(placement.layer_name, "common");
        if (is_high_priority_table(placement)) {
            return common ? 0 : 1;
        }
        return common ? 2 : 3;
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

        if (placement_has_complete_area_obj_runtime(
                placement.object_name, placement.table_path,
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
        auto order = std::vector<std::size_t>(placements.size());
        std::iota(order.begin(), order.end(), std::size_t{});
        std::ranges::stable_sort(order, [&](std::size_t left,
                                            std::size_t right) {
            return authored_placement_retail_phase(placements[left]) <
                   authored_placement_retail_phase(placements[right]);
        });

        _report.entries.reserve(order.size());
        for (const auto source_index : order) {
            const auto &placement = placements[source_index];
            auto support = _options.support_resolver ? _options.support_resolver(placement) : classify_authored_placement(placement);
            if (support.reason.empty()) {
                support.reason = "unspecified_support_result";
            }

            auto actor_name = std::optional<std::string>{};
            if (support.kind == AuthoredPlacementSupportKind::Ready &&
                _options.actor_name_resolver) {
                actor_name = _options.actor_name_resolver(placement);
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
                .retail_phase =
                    authored_placement_retail_phase(placement),
                .placement = &placement,
                .support = std::move(support),
                .actor_name = std::move(actor_name),
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
    AuthoredPlacementInstantiator::instantiate() {
        if (_report.state != AuthoredPlacementRuntimeState::Prepared) {
            throw std::logic_error(
                "Authored placements can only be instantiated once.");
        }
        preflight_or_throw();

        _owned_instances.reserve(_report.ready_count);
        _instance_views.reserve(_report.ready_count);
        AuthoredPlacementReportEntry *current_entry = nullptr;
        try {
            for (auto entry_index = std::size_t{};
                 entry_index < _report.entries.size(); ++entry_index) {
                auto &entry = _report.entries[entry_index];
                if (entry.support.kind !=
                    AuthoredPlacementSupportKind::Ready) {
                    continue;
                }

                current_entry = &entry;
                const auto context =
                    _data.placement_context(entry.source_index);
                entry.archives = _lifecycle->preload_archives(
                    entry.placement->object_name, context);
                if (!std::ranges::all_of(
                        entry.archives, [](const auto &request) {
                            return request.loaded;
                        })) {
                    throw std::runtime_error(
                        "Authored placement archive preload returned an "
                        "unloaded request.");
                }

                const auto *actor_name = entry.actor_name.has_value() ? entry.actor_name->c_str() : nullptr;
                auto actor = _lifecycle->construct_and_init(
                    entry.placement->object_name, actor_name, context);
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
