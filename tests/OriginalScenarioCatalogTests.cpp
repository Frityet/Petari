#include "OriginalStageResourceProcessFixture.hpp"
#include "resource/BcsvTable.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/System/FileLoader.hpp"
#include "Game/System/ScenarioDataParser.hpp"
#include "Game/System/StationedArchiveLoader.hpp"
#include "Game/System/GalaxyNameSortTable.hpp"
#include "Game/Util/FileUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include <cstring>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
std::vector<std::string> scenario_paths() {
    std::vector<std::string> result;
    DVDDir directory;
    DVDDirEntry entry;
    require(DVDOpenDir("/StageData", &directory), "original DVD directory enumerates the authored stage catalog");
    while (DVDReadDir(&directory, &entry)) {
        if (!entry.isDir) continue;
        char path[256];
        MR::makeScenarioArchiveFileName(path, sizeof(path), entry.name);
        if (MR::isFileExist(path, false)) result.emplace_back(path);
    }
    DVDCloseDir(&directory);
    return result;
}

void verify_catalog(std::weak_ptr<JMapInfo::DataCompat>& last_map) {
    auto* loader = SingletonHolder<FileLoader>::get();
    auto* system = SingletonHolder<GameSystem>::get();
    auto* parser = system->mSceneController->mScenarioParser;
    require(loader && parser && parser == ScenarioDataFunction::getScenarioDataParser(),
            "the actual scene controller owns the scenario facade's parser");
    const auto paths = scenario_paths();
    require(!paths.empty() && paths.size() <= 64, "authored catalog fits original fixed parser storage");
    require(GalaxyNameSortTable::getGalaxySortIndex("HeavensDoorGalaxy") == 6 &&
                GalaxyNameSortTable::getGalaxySortIndex("EggStarGalaxy") == 8 &&
                GalaxyNameSortTable::getGalaxySortIndex("") == 5 &&
                GalaxyNameSortTable::getGalaxySortIndex("NoSuchAuthoredGalaxy") == -1,
            "original GalaxyID lookup preserves spacer rows and absent-name results");

    const auto mount_count = loader->mArchiveHolder->mEntries.size();
    auto* first = loader->mArchiveHolder->findEntry(paths.front().c_str());
    require(first, "original startup preloads its first authored scenario archive");
    StationedArchiveLoader::loadScenarioData(first->mHeap);
    require(loader->mArchiveHolder->mEntries.size() == mount_count,
            "repeating the original preloader preserves all existing actual mount identities");
    require(MR::receiveArchive("/StageData/DefinitelyAbsent/DefinitelyAbsentScenario.arc") == nullptr,
            "receiving a missing original archive does not silently mount it");
    for (const auto& path : paths) {
        auto* entry = loader->mArchiveHolder->findEntry(path.c_str());
        require(entry && entry->mArchive == MR::receiveArchive(path.c_str()),
                "every authored scenario archive is owned by the real FileLoader ArchiveHolder");
        JKRArchive* archive = nullptr;
        JKRHeap* heap = nullptr;
        MR::getMountedArchiveAndHeap(path.c_str(), &archive, &heap);
        require(archive == entry->mArchive && heap == entry->mHeap &&
                    MR::mountArchive(path.c_str(), nullptr) == archive,
                "repeat mounts and metadata queries retain original archive identity and heap provenance");
    }
    require(parser->mScenarioData.size() == static_cast<s32>(paths.size()),
            "actual process parser enumerates the complete independently discovered authored catalog");
    auto previous_index = std::numeric_limits<s32>::min();
    std::size_t zones = 0;
    for (s32 i = 0; i < parser->mScenarioData.size(); ++i) {
        const auto* data = parser->getScenarioData(i);
        const auto sort_index = GalaxyNameSortTable::getGalaxySortIndex(data->mGalaxyName);
        require(previous_index <= sort_index, "actual parser performs original GalaxyID ordering");
        previous_index = sort_index;
        require(data->getZoneNum() == data->mZoneList->getNumEntries(), "zone count covers the complete authored ZoneList");
        for (s32 zone = 0; zone < data->getZoneNum(); ++zone)
            require(data->getZoneName(zone), "every authored zone retains its readable original name");
        zones += data->getZoneNum();
        last_map = data->mZoneList->mData;
    }
    const auto* gateway = parser->getScenarioData("heavensdoorgalaxy");
    require(gateway && gateway == parser->getScenarioData("HeavensDoorGalaxy"),
            "case-insensitive original lookup identifies the same authored Gateway object");
    char gateway_path[256];
    MR::makeScenarioArchiveFileName(gateway_path, sizeof(gateway_path), gateway->mGalaxyName);
    auto* archive = MR::receiveArchive(gateway_path);
    auto lease = archive->retainNativeResources();
    const auto raw = smgpc::resource::BcsvTable::from_bytes(archive->source().resource_data("/ScenarioData.bcsv"));
    s32 visible_scenarios = 0, power_stars = 0;
    for (std::size_t row = 0; row < raw.entry_count(); ++row) {
        require(raw.get_s32(row, "ScenarioNo") == static_cast<s32>(row + 1),
                "authored scenario numbers cover original count traversal");
        visible_scenarios += raw.get_u32(row, "IsHidden").value_or(0) == 0;
        power_stars += raw.get_u32(row, "PowerStarId").value_or(0) != 0;
    }
    require(gateway->getScenarioNum() == visible_scenarios && gateway->getPowerStarNum() == power_stars,
            "original hidden/star counts match independently decoded authored scenario rows");
    std::cout << "PASS authored catalogs=" << paths.size() << " zones=" << zones << " Gateway rows=" << raw.entry_count()
              << " visible=" << visible_scenarios << " stars=" << power_stars << '\n';
}

void verify_native_name_and_failed_constructor_lifetimes() {
    const auto baseline = smgpc::compat::name_obj_runtime_state_count();
    auto domain = smgpc::compat::JkrAllocationDomain::create(MR::getSceneObjHolder()->nativeAllocationDomain(), 4096);
    std::optional<NameObj> object;
    std::vector<NameObj*> snapshot;
    constexpr char changed_name[] = "An explicitly retained original NameObj name longer than a string small buffer";
    {
        const smgpc::compat::JkrAllocationScope game(domain);
        const auto marker = smgpc::compat::mark_name_obj_runtime_registrations();
        object.emplace("Stack-owned NameObj while the actual Game heap is current");
        object->setName(changed_name);
        snapshot = smgpc::compat::snapshot_name_obj_runtime_objects_since(marker);
    }
    domain.reset();
    require(std::strcmp(object->getName(), changed_name) == 0 && snapshot.size() == 1 && snapshot.front() == &*object,
            "externally owned name bytes and host registration snapshots survive the temporary Game heap");
    object.reset();
    require(smgpc::compat::name_obj_runtime_state_count() == baseline,
            "stack object retirement removes precisely its original NameObj registration");

    auto constrained = smgpc::compat::JkrAllocationDomain::create(MR::getSceneObjHolder()->nativeAllocationDomain(), 2048);
    const auto archive_count = SingletonHolder<FileLoader>::get()->mArchiveHolder->mEntries.size();
    bool allocation_failed = false;
    try {
        const smgpc::compat::JkrAllocationScope game(constrained);
        auto parser = std::make_unique<ScenarioDataParser>("Constrained original parser construction");
    } catch (const std::bad_alloc&) {
        allocation_failed = true;
    }
    require(allocation_failed, "a constrained actual child heap exercises original parser construction failure");
    require(smgpc::compat::name_obj_runtime_state_count() == baseline,
            "failed original parser construction unregisters its NameObj base");
    constrained.reset();
    require(SingletonHolder<FileLoader>::get()->mArchiveHolder->mEntries.size() == archive_count,
            "failed borrower construction leaves process-owned archive entries intact");
}
}

int main() {
    std::weak_ptr<JMapInfo::DataCompat> last_map;
    const int result = smgpc::test::run_stage_resource_process("original-scenario-catalog", [&] {
        verify_catalog(last_map);
        verify_native_name_and_failed_constructor_lifetimes();
    });
    if (result != 0) return result;
    if (!last_map.expired()) {
        std::cerr << "FAIL original process retirement retained scenario JMap metadata\n";
        return 1;
    }
    std::cout << "PASS actual scenario catalog, typed metadata and normal process teardown\n";
    return 0;
}
