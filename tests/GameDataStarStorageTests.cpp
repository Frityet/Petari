#include "Game/System/GameDataFunction.hpp"
#include "Game/System/GameDataConst.hpp"
#include "Game/System/GameDataGalaxyStorage.hpp"
#include "Game/System/GameDataHolder.hpp"
#include "Game/System/GameEventFlag.hpp"
#include "Game/System/GameEventFlagTable.hpp"
#include "Game/System/GalaxyStatusAccessor.hpp"
#include "Game/System/ScenarioDataParser.hpp"
#include "compat/GameDataFunctionCompat.hpp"
#include "compat/GameDataHolderCompat.hpp"
#include "compat/GameDataSession.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "resource/GameResourceRuntime.hpp"
#include "runtime/ArchiveMountService.hpp"
#include "runtime/RuntimeServices.hpp"
#include "runtime/ScenarioCatalogOwnership.hpp"

#include <aurora/aurora.h>
#include <aurora/dvd.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace aurora { extern AuroraConfig g_config; }
namespace {
void require(bool pass, const char* message) {
    if (!pass) throw std::runtime_error(message);
}
template <typename Exception, typename F>
void require_throws(F&& operation, const char* message) {
    bool rejected = false;
    try { operation(); } catch (const Exception&) { rejected = true; }
    require(rejected, message);
}
}

int main() {
    try {
        require_throws<std::logic_error>([] { GameDataFunction::getPowerStarNumOwned("UnboundGalaxy"); },
                                        "a global star count requires an actual current holder");
        const char* disc = std::getenv("SMGPC_REAL_DISC");
        require(disc && aurora_dvd_open(disc), "the star storage fixture requires the actual disc");
        struct DiscGuard { ~DiscGuard() { aurora_dvd_close(); } } disc_guard;
        DVDInit();
        aurora::g_config.mem1Size = 24U * 1024U * 1024U;
        smgpc::resource::GameResourceRuntime process;
        smgpc::runtime::DvdFileSystemService dvd({});
        smgpc::runtime::ArchiveMountService mounts(dvd);
        auto catalog = std::make_unique<smgpc::runtime::ScenarioCatalogOwnership>(
            process.host_heaps(), 8U * 1024U * 1024U, mounts);
        auto& parser = catalog->parser();
        auto first_name = std::string{};
        s32 first_count = 0;
        s32 expected_total = 0;
        s32 hidden_stars = 0;
        s32 galaxy_count = 0;
        const auto baseline = smgpc::compat::game_data::holder_state_count();
        {
            auto source = std::make_unique<smgpc::compat::GameDataSession>(2U);
            require(GameDataFunction::calcCurrentPowerStarNum() == 0, "new selected file has no owned stars");
            for (s32 index = 0; index < parser.mScenarioData.size(); ++index) {
                const auto* data = parser.getScenarioData(index);
                const auto accessor = GalaxyStatusAccessor(data);
                const auto count = accessor.getPowerStarNum();
                if (count == 0) {
                    require(accessor.getPowerStarNumOwned() == 0 && !accessor.isCompleted(),
                            "galaxies without authored stars remain uncompleted with no storage record");
                    continue;
                }
                if (first_name.empty()) {
                    first_name = accessor.getName();
                    first_count = count;
                    // Force lazy record allocation inside an unrelated original Game heap.
                    auto scene = smgpc::compat::JkrAllocationDomain::create(process.host_heaps(), 4096);
                    {
                        smgpc::compat::JkrAllocationScope scope(scene);
                        require(accessor.getPowerStarNumOwned() == 0, "lazy initialized original records are empty");
                    }
                    scene.reset();
                }
                ++galaxy_count;
                for (s32 star = 1; star <= count; ++star) {
                    require(!accessor.hasPowerStar(star), "each unset authored ownership bit is initially clear");
                    GameDataFunction::setGameFlagPowerStarSuccess(accessor.getName(), star, true);
                    require(accessor.hasPowerStar(star) && accessor.getPowerStarNumOwned() == star,
                            "original galaxy accessor counts actual individual ownership writes");
                    GameDataFunction::setGameFlagPowerStarSuccess(accessor.getName(), star, true);
                    require(accessor.getPowerStarNumOwned() == star, "repeated awards do not duplicate an ownership bit");
                    hidden_stars += accessor.isHiddenStar(star);
                }
                expected_total += count;
                require(accessor.isCompleted(), "all authored star bits complete the original galaxy accessor");
                if (count < 8) {
                    GameDataFunction::setGameFlagPowerStarSuccess(accessor.getName(), 8, true);
                    require(accessor.hasPowerStar(8) && accessor.getPowerStarNumOwned() == count,
                            "original storage preserves high bits but counts only the authored star range");
                    GameDataFunction::setGameFlagPowerStarSuccess(accessor.getName(), 8, false);
                }
            }
            require(galaxy_count > 1 && hidden_stars > 0, "actual catalog exercises multiple galaxies and hidden stars");
            require(GameDataFunction::calcCurrentPowerStarNum() == expected_total,
                    "aggregate count is the sum of real galaxy bit counts, including hidden stars");

            s32 grand_stars = 0;
            for (s32 i = 0; i < GameEventFlagTable::getTableSize(); ++i) {
                const auto* flag = GameEventFlagTable::getFlag(i);
                constexpr std::string_view prefix = "SpecialStarGrand";
                if (flag->mType != GameEventFlag::Type_SpecialStar || !std::string_view(flag->mName).starts_with(prefix)) continue;
                const auto id = std::stoi(std::string(flag->mName + prefix.size()));
                require(GameDataConst::isGrandStar(flag->mGalaxyName, flag->mStarID) &&
                            MR::makeGalaxyStatusAccessor(flag->mGalaxyName).isExistGrandStar(),
                        "original Grand Star classification reads the actual event table");
                require(GameDataFunction::hasGrandStar(id), "original Grand Star predicates read actual per-galaxy bits");
                GameDataFunction::setGameFlagPowerStarSuccess(flag->mGalaxyName, flag->mStarID, false);
                require(!GameDataFunction::hasGrandStar(id) && GameDataFunction::calcCurrentPowerStarNum() == expected_total - 1,
                        "clearing a Grand Star changes both its derived flag and total exactly once");
                GameDataFunction::setGameFlagPowerStarSuccess(flag->mGalaxyName, flag->mStarID, true);
                ++grand_stars;
            }
            require(grand_stars > 0, "actual event table exercises Grand Star ownership");
            require_throws<std::out_of_range>([&] { source->holder().hasPowerStar(first_name.c_str(), 0); },
                                              "scenario zero cannot shift outside original storage");
            require_throws<std::out_of_range>([&] { source->holder().setPowerStar(first_name.c_str(), 9, true); },
                                              "a ninth scenario cannot overflow the eight-bit flags");
            require_throws<std::invalid_argument>([&] { source->holder().getPowerStarNumOwned("NoAuthoredGalaxy"); },
                                                  "an unknown galaxy must not receive invented storage");

            {
                auto other = smgpc::compat::GameDataSession{6U};
                require(GameDataFunction::getPowerStarNumOwned(first_name.c_str()) == 0,
                        "another selected file does not inherit the first file's galaxy flags");
                GameDataFunction::setGameFlagPowerStarSuccess(first_name.c_str(), 1, true);
                require(GameDataFunction::calcCurrentPowerStarNum() == 1, "nested selected-file writes have their own total");
            }
            require(GameDataFunction::calcCurrentPowerStarNum() == expected_total,
                    "leaving a nested file restores the original complete holder state");

            auto copy = GameDataHolder(nullptr);
            struct HolderGuard {
                GameDataHolder& holder;
                ~HolderGuard() { smgpc::compat::game_data::destroy_holder_state(holder); }
            } copy_guard{copy};
            smgpc::compat::game_data::copy_holder_state(copy, source->holder());
            const auto retained_scenario = source->holder().makeGalaxyScenarioAccessor(first_name.c_str(), 1);
            source->holder().resetAllData();
            require(source->holder().calcCurrentPowerStarNum() == 0 && copy.calcCurrentPowerStarNum() == expected_total,
                    "holder copies deeply retain original storage independently of source resets");
            require(!retained_scenario.hasPowerStar(), "reset clears existing original records without invalidating accessors");
            source.reset();
            catalog.reset();
            require(smgpc::runtime::ScenarioCatalogOwnership::active() == nullptr,
                    "the process catalog is retired before retained-holder queries");
            {
                const auto binding = smgpc::compat::ScopedGameDataHolderOverride(copy);
                auto scenario = GameDataFunction::makeGalaxyScenarioAccessor(first_name.c_str(), 1);
                require(std::string_view(scenario.mSomeGalaxyStorage->mGalaxyName) == first_name && scenario.hasPowerStar(),
                        "copied storage owns its galaxy identity after source and catalog teardown");
                scenario.updateMaxCoinNum(1234);
                scenario.updateMaxCoinNum(12);
                scenario.setFlagAlreadyVisited(true);
                require(scenario.getMaxCoinNum() == 999 && scenario.isAlreadyVisited(),
                        "original scenario storage retains bounded coin records and visit flags");
                GameDataFunction::setGameFlagPowerStarSuccess(first_name.c_str(), 1, false);
                require(GameDataFunction::getPowerStarNumOwned(first_name.c_str()) == first_count - 1,
                        "retained original ownership remains mutable after catalog retirement");
            }
            smgpc::compat::game_data::set_holder_save_counts(copy, 37, 500, 0);
            require(copy.calcCurrentPowerStarNum() == 37, "aggregate-only save input preserves its actual total");
            require_throws<std::logic_error>([&] { copy.getPowerStarNumOwned(first_name.c_str()); },
                                            "a positive aggregate alone cannot invent per-galaxy counts");
            require_throws<std::logic_error>([&] { copy.setPowerStar(first_name.c_str(), 1, true); },
                                            "partial ownership cannot be overwritten by a fabricated fresh map");
            copy.resetAllData();
            require(copy.calcCurrentPowerStarNum() == 0, "explicit reset clears an aggregate-only saved total");
            std::cout << "PASS original galaxy records=" << galaxy_count << " stars=" << expected_total
                      << " hidden=" << hidden_stars << " grand=" << grand_stars << '\n';
        }
        require(smgpc::compat::game_data::holder_state_count() == baseline, "selected-file storage owners fully retire");
        std::cout << "PASS selected-file isolation, copy/reset, host lifetime and aggregate-only rejection\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
