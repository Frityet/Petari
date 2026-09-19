#include "scene/OriginalPlacementCoverage.hpp"

#include "Game/Scene/PlacementInfoOrdered.hpp"
#include "Game/Scene/StageDataHolder.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "scene/nameobj/NameObjFactory.hpp"
#include "JSystem/JKernel/JKRArchive.hpp"
#include "JSystem/JKernel/JKRFileFinder.hpp"

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
#include <map>
#include <memory>
#include <stdexcept>

namespace smgpc::scene {
    namespace {
        using ResourcePaths = std::multimap<const void*, std::string>;

        void index_archive_paths(JKRArchive& archive, const std::string& directory, ResourcePaths& paths) {
            const auto finder = std::unique_ptr<JKRArcFinder>(archive.getFirstFile(directory.c_str()));
            if (!finder) return;
            for (; finder->mHasMoreFiles; finder->findNextFile()) {
                const std::string_view name = finder->mName ? finder->mName : "";
                if (name.empty() || name == "." || name == "..") continue;
                const auto path = directory + (directory == "/" ? "" : "/") + std::string(name);
                if (finder->mFileIsFolder) {
                    index_archive_paths(archive, path, paths);
                } else {
                    const void* source = archive.getIdxResource(finder->mDirIndex);
                    // Empty or aliased resources can share an archive offset.
                    // Only an ambiguous retained table is an error below.
                    if (source) paths.emplace(source, path);
                }
            }
        }

        bool find_holder_path(const StageDataHolder& current, const StageDataHolder* wanted, std::vector<s32>& path) {
            if (&current == wanted) return true;
            for (s32 i = 0; i < current.mStageDataHolderCount; ++i) {
                path.push_back(i);
                if (find_holder_path(*current.mStageDataArray[i], wanted, path)) return true;
                path.pop_back();
            }
            return false;
        }

        struct Provenance {
            const StageDataHolder* root;
            std::map<JKRArchive*, ResourcePaths> archives;

            void fill(OriginalPlacementCoverageEntry& entry, const JMapInfoIter& iter) {
                if (!root) return;
                const auto* owner = root->findPlacedStageDataHolder(iter);
                if (!owner || !owner->mArchive || !find_holder_path(*root, owner, entry.holder_path))
                    aurora::throw_host_exception<std::logic_error>("Original placement row has no attached stage/archive owner");
                entry.zone = owner->mZoneID;
                entry.zone_name = owner->_A8 ? owner->_A8 : "";
                for (std::size_t row = 0; row < 3; ++row)
                    for (std::size_t column = 0; column < 4; ++column)
                        entry.zone_placement_matrix[row * 4 + column] = owner->mPlacementMtx[row][column];
                auto [archive, inserted] = archives.try_emplace(owner->mArchive);
                if (inserted) index_archive_paths(*owner->mArchive, "/", archive->second);
                const auto resources = archive->second.equal_range(iter.mInfo->getData());
                for (auto resource = resources.first; resource != resources.second; ++resource) {
                    const auto basename = std::string_view(resource->second).substr(resource->second.find_last_of('/') + 1);
                    if (basename != entry.table) continue;
                    if (!entry.table_path.empty())
                        aurora::throw_host_exception<std::logic_error>("Original placement table has ambiguous archive provenance");
                    entry.table_path = resource->second;
                }
                if (entry.table_path.empty())
                    aurora::throw_host_exception<std::logic_error>("Original placement table is absent from its owner's archive");
                // All original layered JMap categories share /jmp/category/layer/file.
                const auto category_end = entry.table_path.find('/', 5);
                const auto layer_end = category_end == std::string::npos ? category_end : entry.table_path.find('/', category_end + 1);
                if (entry.table_path.starts_with("/jmp/") && layer_end != std::string::npos) {
                    entry.layer = entry.table_path.substr(category_end + 1, layer_end - category_end - 1);
                    std::ranges::transform(entry.layer, entry.layer.begin(), [](unsigned char c) { return std::tolower(c); });
                }
            }
        };

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
                                                   s32 model_no, const JMapInfoIter& iter, Provenance& provenance) {
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
            provenance.fill(entry, iter);
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
        std::span<const OriginalPlacementQueue> queues, const JMapInfoIter& player_start, const StageDataHolder* original_root) {
        const aurora::allocation::HostAllocationScope host;
        Provenance provenance{original_root, {}};
        std::vector<OriginalPlacementCoverageEntry> entries;
        if (player_start.isValid()) {
            const char* name = "";
            (void)MR::getObjectName(&name, player_start);
            entries.push_back(inspect_row("player", name ? name : "", -1, player_start, provenance));
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
                    entries.push_back(inspect_row(queue.phase, set->mName ? set->mName : "", set->mModelNo, iter, provenance));
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
        const auto entries = inspect_original_placement_queues(queues, holder.makeCurrentMarioJMapInfoIter(), &holder);
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
                                {"reason", entry.reason}, {"zone_name", entry.zone_name}, {"table_path", entry.table_path},
                                {"layer", entry.layer}, {"holder_path", entry.holder_path},
                                {"zone_placement_matrix", entry.zone_placement_matrix}});
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
