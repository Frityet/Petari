#include "Game/System/GameDataFunction.hpp"
#include "Game/System/GameDataConst.hpp"
#include "Game/System/GameDataGalaxyStorage.hpp"
#include "Game/System/GameDataHolder.hpp"
#include "Game/System/GameEventFlag.hpp"
#include "Game/System/GameEventFlagTable.hpp"
#include "Game/System/GalaxyStatusAccessor.hpp"
#include "Game/System/ScenarioDataParser.hpp"
#include "Game/System/UserFile.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "compat/GameDataFunctionCompat.hpp"
#include "compat/GameDataOwnership.hpp"
#include "compat/GameDataSession.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "resource/GameResourceRuntime.hpp"
#include "runtime/ArchiveMountService.hpp"
#include "runtime/RuntimeServices.hpp"
#include "runtime/ScenarioCatalogOwnership.hpp"

#include <aurora/aurora.h>
#include <aurora/dvd.h>
#include <array>
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
        auto catalog = std::make_shared<smgpc::runtime::ScenarioCatalogOwnership>(
            process.host_heaps(), 8U * 1024U * 1024U, mounts);
        auto& parser = catalog->parser();
        auto first_name = std::string{};
        s32 first_count = 0;
        s32 expected_total = 0;
        s32 hidden_stars = 0;
        s32 galaxy_count = 0;
        smgpc::compat::game_data::initialize_event_table();
        const auto before_sessions = process.host_heaps()->root_heap().getTotalFreeSize();
        const std::weak_ptr<smgpc::runtime::ScenarioCatalogOwnership> retained_catalog = catalog;
        {
            auto source = std::make_unique<smgpc::compat::GameDataSession>(2U, process, catalog);
            require(source->user_file().mGameDataHolder == &source->holder() &&
                        source->holder().mUserFile == &source->user_file(),
                    "the selected file must contain the actual mutually associated UserFile and GameDataHolder");
            require(GameDataFunction::calcCurrentPowerStarNum() == 0 && MR::getPowerStarNum() == 0,
                    "original SceneUtil counter reads zero from the actual new selected file");
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
                    // Queries inside an unrelated Game heap must retain the selected file records.
                    auto scene = smgpc::compat::JkrAllocationDomain::create(process.host_heaps(), 4096);
                    {
                        smgpc::compat::JkrAllocationScope scope(scene);
                        require(accessor.getPowerStarNumOwned() == 0, "original preallocated galaxy records are initially empty");
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

            }
            require(galaxy_count > 1 && hidden_stars > 0, "actual catalog exercises multiple galaxies and hidden stars");
            require(GameDataFunction::calcCurrentPowerStarNum() == expected_total && MR::getPowerStarNum() == expected_total,
                    "original SceneUtil counter sums the selected file's real galaxy bits, including hidden stars");

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
                require(!GameDataFunction::hasGrandStar(id) && GameDataFunction::calcCurrentPowerStarNum() == expected_total - 1 &&
                            MR::getPowerStarNum() == expected_total - 1,
                        "clearing a Grand Star changes both its derived flag and total exactly once");
                GameDataFunction::setGameFlagPowerStarSuccess(flag->mGalaxyName, flag->mStarID, true);
                ++grand_stars;
            }
            require(grand_stars > 0, "actual event table exercises Grand Star ownership");
            auto original_scenario = source->holder().makeGalaxyScenarioAccessor(first_name.c_str(), 1);
            original_scenario.updateMaxCoinNum(1234);
            original_scenario.updateMaxCoinNum(12);
            original_scenario.setFlagAlreadyVisited(true);
            require(original_scenario.getMaxCoinNum() == 999 && original_scenario.isAlreadyVisited(),
                    "original scenario records clamp coins, preserve maxima, and retain visited flags");
            source->holder().addPlayerLeft(30);
            source->holder().addStockedStarPiece(321);
            source->store_scene_start();
            require(&source->holder() != &source->scene_start_holder() &&
                        source->scene_start_holder().calcCurrentPowerStarNum() == expected_total,
                    "scene-start storage must be a separate actual holder containing the serialized star records");
            require(source->holder().getPlayerLeft() == 34 && source->scene_start_holder().getPlayerLeft() == 4 &&
                        source->scene_start_holder().getStockedStarPieceNum() == 321,
                    "original PLAY loading resets backup lives to four while preserving serialized star bits");
            const auto backup_scenario = source->scene_start_holder().makeGalaxyScenarioAccessor(first_name.c_str(), 1);
            require(backup_scenario.mSomeGalaxyStorage != original_scenario.mSomeGalaxyStorage &&
                        backup_scenario.getMaxCoinNum() == 999 && backup_scenario.isAlreadyVisited(),
                    "scene-start galaxy records must own a deep serialized copy of coins and visit state");

            std::array<u8, 4096> binary{};
            const auto binary_size = source->holder().makeFileBinary(binary.data(), binary.size());
            require(binary_size > 0 && binary_size <= binary.size(), "actual original chunks must fit their file buffer");
            {
                auto other = smgpc::compat::GameDataSession{6U, process, catalog};
                require(GameDataFunction::getPowerStarNumOwned(first_name.c_str()) == 0 && MR::getPowerStarNum() == 0 &&
                            other.scene_start_holder().calcCurrentPowerStarNum() == 0,
                        "another selected file starts with independent current and scene-start star records");
                GameDataFunction::setGameFlagPowerStarSuccess(first_name.c_str(), 1, true);
                require(GameDataFunction::calcCurrentPowerStarNum() == 1 && MR::getPowerStarNum() == 1 &&
                            other.scene_start_holder().calcCurrentPowerStarNum() == 0,
                        "current-file awards must not mutate the separate initial snapshot");
                other.user_file().loadFromGameDataBinary("mario6", binary.data(), binary_size);
                require(!other.user_file().mIsGameDataCorrupted &&
                            other.holder().calcCurrentPowerStarNum() == expected_total && MR::getPowerStarNum() == expected_total,
                        "original UserFile loading must restore all serialized per-galaxy ownership records");
                require(other.holder().getPlayerLeft() == 4 && other.holder().getStockedStarPieceNum() == 321,
                        "loaded PLAY payload must keep the original new-session lives rule");

                source->holder().resetAllData();
                require(source->holder().calcCurrentPowerStarNum() == 0 &&
                            source->scene_start_holder().calcCurrentPowerStarNum() == expected_total &&
                            other.holder().calcCurrentPowerStarNum() == expected_total && MR::getPowerStarNum() == expected_total,
                        "resetting the outer file leaves the selected SceneUtil counter and both independent copies unchanged");
                require(!original_scenario.hasPowerStar() && !original_scenario.isAlreadyVisited() &&
                            original_scenario.getMaxCoinNum() == 0,
                        "reset clears the existing records without invalidating original accessors");

                catalog.reset();
                require(!retained_catalog.expired() && smgpc::runtime::ScenarioCatalogOwnership::active() != nullptr,
                        "selected files must retain the actual scenario catalog after its external handle is released");
                auto scenario = GameDataFunction::makeGalaxyScenarioAccessor(first_name.c_str(), 1);
                require(std::string_view(scenario.mSomeGalaxyStorage->mGalaxyName) == first_name && scenario.hasPowerStar() &&
                            scenario.isAlreadyVisited() && scenario.getMaxCoinNum() == 999,
                        "loaded records retain authored identity, coins and visits with their real catalog owner");
                GameDataFunction::setGameFlagPowerStarSuccess(first_name.c_str(), 1, false);
                require(GameDataFunction::getPowerStarNumOwned(first_name.c_str()) == first_count - 1 &&
                            MR::getPowerStarNum() == expected_total - 1,
                        "loaded individual ownership remains mutable without an external catalog handle");
                other.holder().resetAllData();
                require(other.holder().calcCurrentPowerStarNum() == 0 && MR::getPowerStarNum() == 0 &&
                            !scenario.isAlreadyVisited() && scenario.getMaxCoinNum() == 0,
                        "loaded records support a real reset of stars, coins and visits");
            }
            require(GameDataFunction::getCurrentGameDataHolder() == &source->holder() &&
                        GameDataFunction::getSceneStartGameDataHolder() == &source->scene_start_holder() &&
                        GameDataFunction::calcCurrentPowerStarNum() == 0 && MR::getPowerStarNum() == 0,
                    "nested file teardown restores both distinct outer bindings");
            require(process.host_heaps()->root_heap().getTotalFreeSize() < before_sessions,
                    "the outer file and its retained catalog remain owned until session teardown");
            std::cout << "PASS original galaxy records=" << galaxy_count << " stars=" << expected_total
                      << " hidden=" << hidden_stars << " grand=" << grand_stars << '\n';
        }
        require(retained_catalog.expired() && smgpc::runtime::ScenarioCatalogOwnership::active() == nullptr,
                "final selected-file teardown releases the retained catalog owner");
        require_throws<std::logic_error>([] { GameDataFunction::getCurrentGameDataHolder(); },
                                        "current data binding must retire with the actual selected file");
        require_throws<std::logic_error>([] { GameDataFunction::getSceneStartGameDataHolder(); },
                                        "scene-start data binding must retire with the actual selected file");
        std::cout << "PASS actual UserFile isolation, serialized copies, reset and retained catalog lifetime\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
