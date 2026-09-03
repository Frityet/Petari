#include "runtime/ArchiveMountService.hpp"
#include "runtime/RuntimeServices.hpp"
#include "resource/GameResourceRuntime.hpp"
#include "resource/JMapResource.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/RarcArchive.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "Game/System/ScenarioDataParser.hpp"
#include "Game/System/StationedArchiveLoader.hpp"
#include "Game/System/GalaxyNameSortTable.hpp"
#include "Game/System/GameEventFlagTable.hpp"
#include "Game/Util/FileUtil.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include <aurora/aurora.h>
#include <aurora/dvd.h>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <cstring>
#include <stdexcept>

namespace aurora { extern AuroraConfig g_config; }
namespace {
    void require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }
    std::vector<std::string> scenario_paths(smgpc::runtime::DvdFileSystemService& dvd) {
        std::vector<std::string> result;
        for (const auto& entry : dvd.directory_entries("/StageData")) {
            if (!entry.is_directory) continue;
            char path[256];
            MR::makeScenarioArchiveFileName(path, sizeof(path), entry.name.c_str());
            if (dvd.exists(path)) result.emplace_back(path);
        }
        return result;
    }
}

int main() {
    try {
        const char* disc = std::getenv("SMGPC_REAL_DISC");
        require(disc && aurora_dvd_open(disc), "fixture requires the supplied actual disc");
        struct DiscGuard { ~DiscGuard() { aurora_dvd_close(); } } disc_guard;
        DVDInit();
        aurora::g_config.mem1Size = 24U * 1024U * 1024U;
        smgpc::resource::GameResourceRuntime process;
        smgpc::runtime::DvdFileSystemService dvd({});
        const auto paths = scenario_paths(dvd);
        require(!paths.empty() && paths.size() <= 64, "authored catalog must fit original parser storage");
        const auto name_count = smgpc::compat::name_obj_runtime_state_count();

        require(GalaxyNameSortTable::getGalaxySortIndex("HeavensDoorGalaxy") == 6,
                "original GalaxyID row order includes the preceding empty spacer");
        require(GalaxyNameSortTable::getGalaxySortIndex("EggStarGalaxy") == 8,
                "original second spacer is retained");
        require(GalaxyNameSortTable::getGalaxySortIndex("") == 5,
                "original findElement returns the first empty spacer row");
        require(GalaxyNameSortTable::getGalaxySortIndex("NoSuchAuthoredGalaxy") == -1,
                "unknown galaxy retains original sort result");
        std::cout << "PASS exact embedded table and original sort query\n";

        for (unsigned cycle = 0; cycle < 2; ++cycle) {
            auto domain = smgpc::compat::JkrAllocationDomain::create(process.host_heaps(), 8U * 1024U * 1024U);
            std::weak_ptr<JMapInfo::DataCompat> last_map;
            {
                smgpc::runtime::ArchiveMountService mounts(dvd);
                require(MR::receiveArchive(paths.front().c_str()) == nullptr && mounts.size() == 0,
                        "receive does not silently mount an archive");
                StationedArchiveLoader::loadScenarioData(&domain->heap());
                require(mounts.size() == paths.size(), "original preloader mounts every discovered scenario archive");
                auto first = mounts.retain(paths.front());
                require(first && &first->archive() == MR::mountArchive(paths.front().c_str(), nullptr),
                        "repeat mount retains actual archive identity and first heap");
                JKRArchive* archive = nullptr;
                JKRHeap* heap = nullptr;
                MR::getMountedArchiveAndHeap(paths.front().c_str(), &archive, &heap);
                require(archive == &first->archive() && heap == &domain->heap(), "original requested heap provenance survives remount");

                std::unique_ptr<ScenarioDataParser> parser;
                {
                    smgpc::compat::JkrAllocationScope original(domain);
                    parser = std::make_unique<ScenarioDataParser>("Actual scenario catalog fixture");
                    parser->initWithoutIter();
                }
                require(parser->mScenarioData.size() == static_cast<s32>(paths.size()), "actual parser enumerates complete authored catalog");
                auto previous_index = std::numeric_limits<s32>::min();
                std::size_t zones = 0;
                for (s32 i = 0; i < parser->mScenarioData.size(); ++i) {
                    const auto* data = parser->getScenarioData(i);
                    const auto sort_index = GalaxyNameSortTable::getGalaxySortIndex(data->mGalaxyName);
                    require(previous_index <= sort_index, "actual parser performs original GalaxyID ordering");
                    previous_index = sort_index;
                    require(data->getZoneNum() == data->mZoneList->getNumEntries(), "zone count is full retained authored ZoneList");
                    for (s32 zone = 0; zone < data->getZoneNum(); ++zone)
                        require(data->getZoneName(zone) != nullptr, "every authored zone has a readable original name");
                    zones += data->getZoneNum();
                    last_map = data->mZoneList->mData;
                }
                const auto* gateway = parser->getScenarioData("heavensdoorgalaxy");
                require(gateway == parser->getScenarioData("HeavensDoorGalaxy"),
                        "case-insensitive original lookup identifies the same authored object");
                require(gateway != nullptr, "real disc catalog contains the gateway galaxy");
                char gateway_path[256];
                MR::makeScenarioArchiveFileName(gateway_path, sizeof(gateway_path), gateway->mGalaxyName);
                const auto gateway_mount = mounts.retain(gateway_path);
                const auto raw = smgpc::resource::BcsvTable::from_bytes(
                    gateway_mount->source().resource_data("/ScenarioData.bcsv"));
                s32 visible_scenarios = 0;
                s32 power_stars = 0;
                for (std::size_t row = 0; row < raw.entry_count(); ++row) {
                    require(raw.get_s32(row, "ScenarioNo") == static_cast<s32>(row + 1),
                            "authored scenario numbers cover original count traversal");
                    visible_scenarios += raw.get_u32(row, "IsHidden").value_or(0) == 0;
                    power_stars += raw.get_u32(row, "PowerStarId").value_or(0) != 0;
                }
                require(gateway->getScenarioNum() == visible_scenarios && gateway->getPowerStarNum() == power_stars,
                        "original hidden/star counts agree with independently decoded authored rows");
                std::cout << "AUTHORED gateway rows=" << raw.entry_count()
                          << " visible=" << visible_scenarios << " stars=" << power_stars
                          << " zones=" << gateway->getZoneNum() << '\n';
                const auto gateway_zone_count = gateway->getZoneNum();
                require(gateway_zone_count > 0, "gateway has an actual zone catalog");

                MR::removeResourceAndFileHolderIfIsEqualHeap(&domain->heap());
                require(mounts.size() == 0 && MR::receiveArchive(paths.front().c_str()) == nullptr,
                        "heap retirement removes mount publication");
                require(gateway->getZoneNum() == gateway_zone_count && gateway->getZoneName(0),
                        "attached original tables retain metadata after mount retirement");
                first.reset();
                parser.reset();
                require(smgpc::compat::name_obj_runtime_state_count() == name_count, "actual parser unregisters before its heap retires");
                std::cout << "PASS catalog cycle=" << cycle << " archives=" << paths.size() << " authored zones=" << zones << '\n';
            }
            require(smgpc::runtime::ArchiveMountService::active() == nullptr, "mount service publication retires with owner");
            domain.reset();
            require(last_map.expired(), "original JKR disposal releases heap-owned JMap metadata");
        }
        {
            auto domain = smgpc::compat::JkrAllocationDomain::create(process.host_heaps(), 4096);
            std::optional<NameObj> object;
            std::vector<NameObj*> snapshot;
            constexpr char changed_name[] = "A retained name update longer than a native string small buffer";
            {
                smgpc::compat::JkrAllocationScope original(domain);
                const auto marker = smgpc::compat::mark_name_obj_runtime_registrations();
                object.emplace("Stack-owned object created while an original Game heap is current");
                object->setName(changed_name);
                snapshot = smgpc::compat::snapshot_name_obj_runtime_objects_since(marker);
            }
            domain.reset();
            require(std::strcmp(object->getName(), changed_name) == 0,
                    "native name-update storage survives the caller Game heap");
            require(snapshot.size() == 1 && snapshot.front() == &*object,
                    "native registration snapshots survive the caller Game heap");
            object.reset();
            require(smgpc::compat::name_obj_runtime_state_count() == name_count,
                    "retained host metadata removes precisely its NameObj identity");
        }
        std::cout << "PASS name updates and registration snapshots escape Game heap lifetime\n";
        {
            auto domain = smgpc::compat::JkrAllocationDomain::create(process.host_heaps(), 2048);
            smgpc::runtime::ArchiveMountService mounts(dvd);
            StationedArchiveLoader::loadScenarioData(&domain->heap());
            bool allocation_failed = false;
            try {
                smgpc::compat::JkrAllocationScope original(domain);
                auto parser = std::make_unique<ScenarioDataParser>("Constrained actual parser construction");
            } catch (const std::bad_alloc&) {
                allocation_failed = true;
            }
            require(allocation_failed, "constrained real Game heap exercises partial original parser construction");
            require(smgpc::compat::name_obj_runtime_state_count() == name_count,
                    "failed original parser construction unregisters its NameObj base");
            mounts.remove_for_heap(&domain->heap());
            domain.reset();
            require(mounts.size() == 0, "partial constructor allocations retire with the real original heap");
        }
        std::cout << "PASS actual parser allocation-failure unwind and heap retirement\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
