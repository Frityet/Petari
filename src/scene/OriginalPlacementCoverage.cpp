#include "scene/OriginalPlacementCoverage.hpp"

#include "Game/Scene/PlacementInfoOrdered.hpp"
#include "Game/Scene/StageDataHolder.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "scene/nameobj/NameObjFactory.hpp"

#include <aurora/allocation.hpp>
#include <aurora/exception.hpp>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace smgpc::scene {
    namespace {
        bool metadata_table(std::string_view name) {
            const auto slash = name.find_last_of("/\\");
            if (slash != std::string_view::npos) name.remove_prefix(slash + 1);
            const auto extension = name.find_last_of('.');
            if (extension != std::string_view::npos) name = name.substr(0, extension);
            auto lower = std::string(name);
            std::ranges::transform(lower, lower.begin(), [](unsigned char c) { return std::tolower(c); });
            // DemoObjInfo rows are actual DemoExecutor/DemoCastSubGroup
            // factory placements in the original process, not helper metadata.
            return lower == "stageobjinfo";
        }

        const char* availability_name(OriginalPlacementAvailability availability) {
            switch (availability) {
            case OriginalPlacementAvailability::Supported: return "supported";
            case OriginalPlacementAvailability::KnownUnlinked: return "known_unlinked";
            case OriginalPlacementAvailability::Unknown: return "unknown";
            case OriginalPlacementAvailability::Metadata: return "metadata";
            }
            aurora::throw_host_exception<std::logic_error>("Invalid original placement availability");
        }

        OriginalPlacementCoverageEntry inspect_row(std::string_view phase, std::string_view object,
                                                   s32 model_no, const JMapInfoIter& iter) {
            if (!iter.isValid())
                aurora::throw_host_exception<std::logic_error>("Original placement coverage requires a valid retained row");
            OriginalPlacementCoverageEntry entry{
                .phase = std::string(phase),
                .table = iter.mInfo->getName() ? iter.mInfo->getName() : "",
                .object = std::string(object),
                .zone = iter.mInfo->getPlacedZoneId(),
                .row = iter.mIndex,
                .model_no = model_no,
            };
            (void)iter.getValue("l_id", &entry.link_id);
            if (metadata_table(entry.table)) {
                entry.availability = OriginalPlacementAvailability::Metadata;
                entry.reason = "original_non_actor_table";
                return entry;
            }
            const auto support = model_no == -1 ? nameobj::describe_name_obj_creator_support(object) :
                                                  nameobj::describe_model_changing_creator_support(object);
            entry.reason = support.reason;
            if (support.kind == nameobj::NameObjCreatorSupportKind::Supported) {
                entry.availability = OriginalPlacementAvailability::Supported;
            } else if (model_no == -1 ? nameobj::original_name_obj_registered(object) :
                       support.kind == nameobj::NameObjCreatorSupportKind::RuntimeClosureUnavailable) {
                entry.availability = OriginalPlacementAvailability::KnownUnlinked;
            }
            return entry;
        }
    }  // namespace

    std::vector<OriginalPlacementCoverageEntry> inspect_original_placement_queues(
        std::span<const OriginalPlacementQueue> queues, const JMapInfoIter& player_start) {
        const aurora::allocation::HostAllocationScope host;
        std::vector<OriginalPlacementCoverageEntry> entries;
        if (player_start.isValid()) {
            const char* name = "";
            (void)MR::getObjectName(&name, player_start);
            entries.push_back(inspect_row("player", name ? name : "", -1, player_start));
        }
        for (const auto& queue : queues) {
            if (!queue.ordered)
                aurora::throw_host_exception<std::logic_error>("Original placement coverage requires initialized original queues");
            const auto count = queue.ordered->getUsedArrayNum();
            for (u32 i = 0; i < count; ++i) {
                const auto* set = queue.ordered->mIdentiferArray[i];
                if (!set)
                    aurora::throw_host_exception<std::logic_error>("Original placement queue contains a null used group");
                u32 links = 0;
                for (auto* link = set->mList.mHead; link; link = link->mNextLink) {
                    if (!link->mValue || ++links > set->mList.mCount)
                        aurora::throw_host_exception<std::logic_error>("Original placement group has invalid linked rows");
                    const auto& iter = static_cast<const PlacementInfoOrdered::Index*>(link->mValue)->mInfoIter;
                    entries.push_back(inspect_row(queue.phase, set->mName ? set->mName : "", set->mModelNo, iter));
                }
                if (links != set->mList.mCount)
                    aurora::throw_host_exception<std::logic_error>("Original placement group row count differs from its links");
            }
        }
        return entries;
    }

    void require_original_placement_closure(std::span<const OriginalPlacementCoverageEntry> entries) {
        const auto first = std::ranges::find_if(entries, [](const auto& entry) {
            return entry.availability == OriginalPlacementAvailability::KnownUnlinked;
        });
        if (first != entries.end())
            aurora::throw_host_exception<std::runtime_error>(
                "Original stage contains an unlinked retail actor: " + first->object +
                " (zone=" + std::to_string(first->zone) + ";table=" + first->table +
                ";row=" + std::to_string(first->row) + ";reason=" + first->reason + ")");
    }

    void report_original_placement_coverage(const StageDataHolder& holder) {
        const aurora::allocation::HostAllocationScope host;
        // This is exactly StageDataHolder::initPlacement's queue order. The
        // original attachments already exclude unplaced/nested-only zones.
        const auto queues = std::array{
            OriginalPlacementQueue{"common_priority", holder._FC},
            OriginalPlacementQueue{"scenario_priority", holder._104},
            OriginalPlacementQueue{"common", holder._100},
            OriginalPlacementQueue{"scenario", holder._108},
            OriginalPlacementQueue{"deferred", holder._10C},
        };
        const auto entries = inspect_original_placement_queues(queues, holder.makeCurrentMarioJMapInfoIter());
        std::array<std::size_t, 4> counts{};
        for (const auto& entry : entries) ++counts[static_cast<std::size_t>(entry.availability)];
        std::fprintf(stderr, "[original-process] Placement coverage %s: %zu supported, %zu known-unlinked, %zu unknown, %zu metadata rows\n",
                     holder._A8, counts[0], counts[1], counts[2], counts[3]);
        std::fflush(stderr);
        if (const char* path = std::getenv("SMGPC_ORIGINAL_PLACEMENT_REPORT_PATH"); path && *path) {
            auto rows = nlohmann::json::array();
            for (const auto& entry : entries) {
                rows.push_back({{"phase", entry.phase}, {"table", entry.table}, {"row", entry.row},
                                {"zone", entry.zone}, {"l_id", entry.link_id}, {"object", entry.object},
                                {"model_no", entry.model_no}, {"availability", availability_name(entry.availability)},
                                {"reason", entry.reason}});
            }
            const auto output = std::filesystem::path(path);
            if (!output.parent_path().empty()) std::filesystem::create_directories(output.parent_path());
            auto stream = std::ofstream(output);
            stream << nlohmann::json{{"stage", holder._A8}, {"scenario", MR::getCurrentScenarioNo()},
                                     {"supported", counts[0]}, {"known_unlinked", counts[1]},
                                     {"unknown", counts[2]}, {"metadata", counts[3]}, {"entries", rows}}.dump(2) << '\n';
            stream.flush();
            if (!stream)
                aurora::throw_host_exception<std::runtime_error>("Cannot write requested original placement report: " + output.string());
        }
        if (const char* strict = std::getenv("SMGPC_STRICT_PLACEMENT"); strict && std::string_view(strict) == "1")
            require_original_placement_closure(entries);
    }
}  // namespace smgpc::scene
