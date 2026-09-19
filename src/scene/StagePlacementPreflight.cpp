#include "scene/StagePlacementPreflight.hpp"
#include "scene/AuthoredPlacementInstantiator.hpp"

#include <aurora/exception.hpp>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace smgpc::scene {
    namespace {
        [[nodiscard]] bool placement_has_complete_runtime(const StagePlacementObject &placement) {
            return classify_authored_placement(placement).kind ==
                   AuthoredPlacementSupportKind::Ready;
        }

        [[nodiscard]] std::string placement_runtime_support_reason(
            const StagePlacementObject &placement) {
            return classify_authored_placement(placement).reason;
        }

#ifndef NDEBUG
        [[nodiscard]] std::filesystem::path debug_stage_placement_report_path() {
            const auto *path = std::getenv("SMGPC_STAGE_PLACEMENT_REPORT_PATH");
            if (path == nullptr || path[0] == '\0') {
                return {};
            }

            return std::filesystem::path(path);
        }

        [[nodiscard]] bool placement_runtime_is_complete(const StagePlacementObject &placement) {
            return placement_has_complete_runtime(placement);
        }

        [[nodiscard]] std::string_view placement_preflight_status_name(const StagePlacementObject &placement) {
            if (placement_has_complete_runtime(placement)) {
                return "complete";
            }
            if (placement.intentionally_ignored) {
                return "ignored";
            }
            return "blocked";
        }

        struct PlacementRailSummary {
            bool attached = false;
            s32 path_info_index = -1;
            s32 point_count = 0;
            bool has_first_point = false;
            std::array<f32, 3U> first_point{};
        };

        [[nodiscard]] PlacementRailSummary placement_rail_summary(const StagePlacementObject &placement) {
            const JMapInfo *point_info = nullptr;
            auto path_info_index = s32{-1};
            const auto attached = placement.jmap_info.getRailInfo(placement.jmap_entry_index, nullptr, &point_info, &path_info_index);
            auto first_point = std::array<f32, 3U>{};
            const auto has_first_point = point_info != nullptr && point_info->getValue(0, "pnt0_x", &first_point[0]) &&
                                         point_info->getValue(0, "pnt0_y", &first_point[1]) &&
                                         point_info->getValue(0, "pnt0_z", &first_point[2]);
            return PlacementRailSummary{
                .attached = attached,
                .path_info_index = path_info_index,
                .point_count = point_info != nullptr ? point_info->getNumEntries() : 0,
                .has_first_point = has_first_point,
                .first_point = first_point,
            };
        }

        void write_stage_placement_report(std::string_view stage_name, s32 scenario_no,
                                          std::span<const StagePlacementObject> placements,
                                          const std::vector<const StagePlacementObject *> &blocked_placements) {
            const auto report_path = debug_stage_placement_report_path();
            if (report_path.empty()) {
                return;
            }

            std::error_code error;
            if (!report_path.parent_path().empty()) {
                std::filesystem::create_directories(report_path.parent_path(), error);
            }

            auto out = std::ofstream(report_path);
            if (!out) {
                return;
            }

            const auto complete_count = std::ranges::count_if(placements, placement_runtime_is_complete);
            const auto ignored_count = std::ranges::count_if(placements, [](const auto &placement) { return placement.intentionally_ignored; });
            out << "# Stage Placement Report\n";
            out << "phase: preflight\n";
            out << "stage: " << stage_name << "\n";
            out << "scenario: " << scenario_no << "\n";
            out << "total_objects: " << placements.size() << "\n";
            out << "complete_objects: " << complete_count << "\n";
            out << "blocked_objects: " << blocked_placements.size() << "\n";
            out << "intentionally_ignored_objects: " << ignored_count << "\n\n";
            out << "## Objects\n";
            for (const auto &placement : placements) {
                const auto rail = placement_rail_summary(placement);
                out << "- status: " << placement_preflight_status_name(placement) << "\n";
                out << "  object: "
                    << authored_placement_identifier(placement) << "\n";
                out << "  authored_name: " << placement.object_name << "\n";
                out << "  zone: " << placement.zone_name << "\n";
                out << "  zone_id: " << placement.zone_id << "\n";
                out << "  table: " << placement.table_path << "\n";
                out << "  row: " << placement.jmap_entry_index << "\n";
                out << "  child_count: " << placement.child_object_count << "\n";
                out << "  common_path_id: " << placement.common_path_id << "\n";
                out << "  rail_info_attached: " << (rail.attached ? "true" : "false") << "\n";
                out << "  rail_path_row: " << rail.path_info_index << "\n";
                out << "  rail_point_count: " << rail.point_count << "\n";
                if (rail.has_first_point) {
                    out << "  rail_first_point: [" << rail.first_point[0] << ", " << rail.first_point[1] << ", " << rail.first_point[2] << "]\n";
                }
                out << "  support_reason: " << placement_runtime_support_reason(placement) << "\n";
                out << "  archive:";
                if (!placement.object_archive_path.empty()) {
                    out << " " << placement.object_archive_path;
                }
                out << "\n";
            }
        }
#endif

        [[nodiscard]] std::string unsupported_placement_error(std::string_view stage_name,
                                                              const std::vector<const StagePlacementObject *> &blocked_placements) {
            auto out = std::ostringstream();
            out << "Unsupported placement objects for " << stage_name << ": " << blocked_placements.size() << " blocked";
            if (!blocked_placements.empty()) {
                out << "; first="
                    << authored_placement_identifier(
                           *blocked_placements.front())
                    << "; raw_name="
                    << blocked_placements.front()->object_name << " in "
                    << blocked_placements.front()->table_path;
            }
            return out.str();
        }
    }  // namespace

    void preflight_stage_placements_or_throw(
        std::string_view stage_name, s32 scenario_no,
        std::span<const StagePlacementObject> placements) {
        auto blocked_placements = std::vector<const StagePlacementObject *>{};
        for (const auto &placement : placements) {
            const auto support = classify_authored_placement(placement);
            if (support.kind != AuthoredPlacementSupportKind::Blocked) {
                continue;
            }
            blocked_placements.push_back(&placement);
        }
#ifndef NDEBUG
        write_stage_placement_report(stage_name, scenario_no, placements, blocked_placements);
#else
        (void)scenario_no;
#endif
        if (!blocked_placements.empty()) {
            aurora::throw_host_exception<std::runtime_error>(unsupported_placement_error(stage_name, blocked_placements));
        }
    }

}  // namespace smgpc::scene
