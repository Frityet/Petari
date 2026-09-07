#include "runtime/ScenarioCatalogOwnership.hpp"
#include "runtime/ArchiveMountService.hpp"
#include "runtime/RuntimeServices.hpp"
#include "resource/GameResourceRuntime.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "Game/System/ScenarioDataParser.hpp"
#include "Game/System/GalaxyStatusAccessor.hpp"
#include <aurora/aurora.h>
#include <aurora/dvd.h>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace aurora { extern AuroraConfig g_config; }
namespace {
void require(bool pass, const char* message) { if (!pass) throw std::runtime_error(message); }
bool accessor_rejects_absence() {
    try { (void)ScenarioDataFunction::getScenarioDataParser(); }
    catch (const std::logic_error&) { return true; }
    return false;
}
}
int main() {
    try {
        require(accessor_rejects_absence(), "unpublished parser must fail explicitly");
        const char* disc = std::getenv("SMGPC_REAL_DISC");
        require(disc && aurora_dvd_open(disc), "actual disc required");
        struct DiscGuard { ~DiscGuard() { aurora_dvd_close(); } } disc_guard;
        DVDInit();
        aurora::g_config.mem1Size = 24U * 1024U * 1024U;
        smgpc::resource::GameResourceRuntime process;
        smgpc::runtime::DvdFileSystemService dvd({});
        smgpc::runtime::ArchiveMountService mounts(dvd);
        const auto baseline = smgpc::compat::name_obj_runtime_state_count();
        for (unsigned cycle = 0; cycle < 2; ++cycle) {
            auto catalog = std::make_shared<smgpc::runtime::ScenarioCatalogOwnership>(
                process.host_heaps(), 8U * 1024U * 1024U, mounts);
            auto retained = catalog;
            auto* parser = ScenarioDataFunction::getScenarioDataParser();
            require(parser == &catalog->parser(), "published pointer is exact original parser identity");
            require(smgpc::compat::name_obj_runtime_ownership_is_claimed(parser), "parser has a single explicit owner");
            require(parser->mScenarioData.size() == static_cast<s32>(mounts.size()), "actual parser covers complete authored mount set");
            const auto* gateway = parser->getScenarioData("HeavensDoorGalaxy");
            require(gateway && gateway->getZoneNum() == 7, "actual retained gateway ZoneList");
            require(MR::makeGalaxyStatusAccessor("heavensdoorgalaxy").getZoneNum() == gateway->getZoneNum(),
                    "complete original accessor passes through actual parser publication");
            std::weak_ptr<JMapInfo::DataCompat> map = gateway->mZoneList->mData;
            bool duplicate_rejected = false;
            try {
                smgpc::runtime::ScenarioCatalogOwnership duplicate(process.host_heaps(), 4096, mounts);
            } catch (const std::logic_error&) { duplicate_rejected = true; }
            require(duplicate_rejected && ScenarioDataFunction::getScenarioDataParser() == parser,
                    "duplicate owner cannot replace live parser publication");
            catalog.reset();
            require(ScenarioDataFunction::getScenarioDataParser() == parser && gateway->getZoneName(6),
                    "scene retention preserves actual catalog and table storage");
            std::cout << "PASS actual publication cycle=" << cycle << " archives=" << mounts.size()
                      << " gateway zones=" << gateway->getZoneNum() << '\n';
            retained.reset();
            require(accessor_rejects_absence() && mounts.size() == 0 && map.expired(),
                    "owner retires publication, mounts and actual JKR JMap metadata");
            require(smgpc::compat::name_obj_runtime_state_count() == baseline,
                    "parser NameObj registry returns to baseline");
        }
        bool failed = false;
        try {
            smgpc::runtime::ScenarioCatalogOwnership constrained(process.host_heaps(), 2048, mounts);
        } catch (const std::bad_alloc&) { failed = true; }
        require(failed && accessor_rejects_absence() && mounts.size() == 0,
                "failed original constructor never publishes and retires mounts");
        require(smgpc::compat::name_obj_runtime_state_count() == baseline,
                "partial original parser constructor unregisters its base");
        std::cout << "PASS strict missing-owner, duplicate-owner, retained scene lease and constructor-failure retirement\n";
    } catch (const std::exception& e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
