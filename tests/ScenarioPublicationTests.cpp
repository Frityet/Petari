#include "OriginalStageResourceProcessFixture.hpp"
#include "Game/System/FileLoader.hpp"
#include "Game/System/ScenarioDataParser.hpp"
#include "Game/System/GalaxyStatusAccessor.hpp"
#include "Game/Util/FileUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
void require(bool pass, const char* message) { if (!pass) throw std::runtime_error(message); }
}

int main(int argc, char** argv) {
    return smgpc::test::run_stage_resource_generations(argc, argv, "scenario-publication", 2, [](unsigned cycle) {
        const auto baseline = smgpc::compat::name_obj_runtime_state_count();
        std::weak_ptr<JMapInfo::DataCompat> map;
        std::weak_ptr<const void> archive_lifetime;
        const auto label = std::string("scenario-publication-") + std::to_string(cycle);
        const auto result = smgpc::test::run_stage_resource_process(label.c_str(), [&] {
            auto* system = SingletonHolder<GameSystem>::get();
            auto* loader = SingletonHolder<FileLoader>::get();
            auto* parser = ScenarioDataFunction::getScenarioDataParser();
            require(system && loader && parser == system->mSceneController->mScenarioParser,
                    "scenario facade returns the actual original scene controller's parser");
            const auto* gateway = parser->getScenarioData("HeavensDoorGalaxy");
            require(gateway && gateway->getZoneNum() == 7, "actual process retains the complete Gateway ZoneList");
            require(MR::makeGalaxyStatusAccessor("heavensdoorgalaxy").getZoneNum() == gateway->getZoneNum(),
                    "original galaxy facade queries the same actual parser");
            const auto archive_count = loader->mArchiveHolder->mEntries.size();
            for (s32 i = 0; i < parser->mScenarioData.size(); ++i) {
                const auto* galaxy = parser->getScenarioData(i);
                char path[256];
                MR::makeScenarioArchiveFileName(path, sizeof(path), galaxy->mGalaxyName);
                auto* entry = loader->mArchiveHolder->findEntry(path);
                require(entry && entry->mArchive == MR::receiveArchive(path),
                        "every parsed galaxy borrows the actual FileLoader ArchiveHolder mount");
                auto retained = entry->retainNativeResources();
                auto copy = retained;
                bool rejected = false;
                try { entry->validateNativeRetirement(); }
                catch (const std::logic_error&) { rejected = true; }
                require(rejected && MR::mountArchive(path, entry->mHeap) == entry->mArchive,
                        "live archive leases reject retirement while repeat mounts preserve original identity");
                retained.reset();
                require(copy && ScenarioDataFunction::getScenarioDataParser() == parser &&
                            (galaxy->getZoneNum() == 0 || galaxy->getZoneName(0)),
                        "a retained source borrower leaves actual parser publication and authored tables intact");
                if (galaxy == gateway) archive_lifetime = copy;
            }
            require(loader->mArchiveHolder->mEntries.size() == archive_count,
                    "repeat scenario requests do not create a second mount registry or duplicate entries");
            map = gateway->mZoneList->mData;
            std::cout << "PASS actual process publication cycle=" << cycle
                      << " galaxies=" << parser->mScenarioData.size() << " gateway zones=" << gateway->getZoneNum() << '\n';
        });
        if (result != 0) return result;
        try {
            require(!SingletonHolder<GameSystem>::get() && !SingletonHolder<FileLoader>::get(),
                    "process retirement clears its real system and file-loader singletons");
            require(map.expired() && archive_lifetime.expired(),
                    "actual process heap disposal retires scenario metadata and archive lifetimes");
            require(smgpc::compat::name_obj_runtime_state_count() == baseline,
                    "actual process retirement restores the original NameObj registry baseline");
        } catch (const std::exception& error) {
            std::cerr << "FAIL scenario publication: " << error.what() << '\n';
            return 1;
        }
        return 0;
    });
}
