#include "compat/StageSessionState.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/GameDataFunctionCompat.hpp"
#include "compat/GameDataSession.hpp"
#include "Game/System/AlreadyDoneFlagInGalaxy.hpp"
#include "Game/System/GameDataTemporaryInGalaxy.hpp"
#include "Game/System/GameDataFunction.hpp"
#include "Game/System/GameDataHolder.hpp"
#include "Game/Scene/ScenePlayingResult.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SequenceUtil.hpp"
#include "Game/Util/ActorCameraUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/HashUtil.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/GameResourceRuntime.hpp"
#include "runtime/ArchiveMountService.hpp"
#include "runtime/RuntimeServices.hpp"
#include "runtime/ScenarioCatalogOwnership.hpp"

#include <aurora/aurora.h>
#include <aurora/dvd.h>
#include <aurora/exception.hpp>
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string_view>

namespace aurora { extern AuroraConfig g_config; }
namespace {
using namespace smgpc::compat;
void require(bool condition, const char* message) {
    if (!condition) aurora::throw_host_exception<std::runtime_error>(message);
}
JMapInfo placement(s32 zone, s32 link) {
    std::array<u8, 0x20> bytes{};
    const auto put = [&](std::size_t offset, u32 value) {
        for (std::size_t i = 0; i < 4; ++i)
            bytes[offset + i] = static_cast<u8>(value >> (24 - 8 * i));
    };
    put(0, 1); put(4, 1); put(8, 0x1c); put(12, 4);
    put(16, smgpc::resource::jmap_hash("l_id")); put(20, 0xffffffff);
    put(28, static_cast<u32>(link));
    auto info = JMapInfo::from_bcsv(bytes);
    info.setPlacedZoneId(zone);
    return info;
}
template<class F> void rejected(F&& f) {
    bool failed = false;
    try { f(); } catch (const std::logic_error&) { failed = true; }
    require(failed, "unowned or out-of-capacity state must fail explicitly");
}
void test_actual_temporary_owner() {
    require(MR::getInitializeStartIdInfo()._0 == 0 && MR::getInitializeStartIdInfo().mZoneID == 0,
            "original initialize-start constant exists without an active scene");
    rejected([] { (void)GameDataFunction::getStarPieceNum(); });
    auto heaps = JkrHeapRuntime::create(8 * 1024 * 1024);
    auto domain = JkrAllocationDomain::create(heaps, 2 * 1024 * 1024);
    std::weak_ptr<JkrAllocationDomain> lease = domain;
    std::unique_ptr<StageSessionState> session;
    {
        JkrAllocationScope game(domain);
        // The caller owns the native wrapper outside the arena; the original
        // temporary-data object and both of its children use the Game domain.
        JkrHostAllocationScope host;
        session = std::make_unique<StageSessionState>("Game", "OwnerFixture", 1, JMapIdInfo(17, 4));
        auto& data = session->temporary_data();
        require(JKRHeap::findFromRoot(&data) == &domain->heap() &&
                JKRHeap::findFromRoot(data.mAlreadyDoneFlag) == &domain->heap() &&
                JKRHeap::findFromRoot(data.mAlreadyDoneFlag->mDoneInfos.begin()) == &domain->heap() &&
                JKRHeap::findFromRoot(data.mPlayerRestartIdInfo) == &domain->heap(),
                "all original temporary owners and records use the actual Game heap");
        require(data.mPlayerRestartIdInfo == &session->restart_id() && session->restart_id()._0 == 0 &&
                session->restart_id().mZoneID == 0 && data.mAlreadyDoneFlag->mDoneInfos.size() == 64,
                "original constructor uses constant restart key (0, 0) and owns 64 flag entries");
        require(try_active_stage_session() == nullptr, "constructor preserves the caller's unbound scene");
    }
    domain.reset();
    require(!lease.expired(), "native owner retains the Game domain until typed child destruction");
    {
        StageSessionBinding binding(*session);
        require(MR::getCurrentMarioStartIdInfo()._0 == 17 && MR::getCurrentMarioStartIdInfo().mZoneID == 4 &&
                MR::getInitializeStartIdInfo()._0 == 0 && MR::getInitializeStartIdInfo().mZoneID == 0,
                "selected scene-entry start is distinct from the original reset constant");
        using Phase = StageSessionState::ExecutionPhase;
        require(session->execution_phase() == Phase::Initialization && !MR::isExecScenarioStarter() && !MR::isStageStateScenarioOpeningCamera(),
                "scene initialization does not imply a running introduction");
        session->set_execution_phase(Phase::ScenarioOpeningCamera);
        require(MR::isStageStateScenarioOpeningCamera() && !MR::isExecScenarioStarter(), "opening camera phase has its own original predicate");
        session->set_execution_phase(Phase::ScenarioStarter);
        require(MR::isExecScenarioStarter() && !MR::isStageStateScenarioOpeningCamera(), "starter phase is independent from camera mode flags");
        session->set_execution_phase(Phase::Gameplay);
        require(!MR::isExecScenarioStarter() && !MR::isStageStateScenarioOpeningCamera(), "ordinary gameplay executes neither introduction runner");
        require(GameDataFunction::getStarPieceNum() == 0 && GameDataFunction::getLast1upStarPieceNum() == 0,
                "original constructor initializes stage counts");
        GameDataFunction::addStarPiece(998);
        GameDataFunction::addStarPiece(9);
        require(GameDataFunction::getStarPieceNum() == 999 && session->temporary_data().mStarPieceNum == 999,
                "public query and original object share the saturating Star Bit count");
        GameDataFunction::addStarPiece(-1200);
        require(GameDataFunction::getStarPieceNum() == 0, "original Star Bit decrement clamps at zero");
        GameDataFunction::setLast1upStarPieceNum(150);
        require(session->temporary_data().mLast1upStarPieceNum == 150 && GameDataFunction::getLast1upStarPieceNum() == 150,
                "one-up threshold uses original storage");
        session->set_restart_id(JMapIdInfo(23, 8));
        require(session->temporary_data().mPlayerRestartIdInfo->_0 == 23 && session->restart_id().mZoneID == 8,
                "restart writes route through original GameDataTemporaryInGalaxy");
        u32 value = 99;
        const auto first_placement = placement(5, -1);
        const JMapInfoIter first_iter(&first_placement, 0);
        const auto index = MR::setupAlreadyDoneFlag("original message", first_iter, &value);
        auto& flags = *session->temporary_data().mAlreadyDoneFlag;
        require(index == 0 && value == 0 && flags._8 == 1 &&
                flags.mDoneInfos[0].mask() == (MR::getHashCode("original message") & 0x7fff) &&
                flags.mDoneInfos[0]._2 == 5 && flags.mDoneInfos[0]._4 == 0xffff,
                "original setup hashes the message and reads zone/link identity from JMapInfo");
        flags.updateValue(0, 1);
        require(MR::setupAlreadyDoneFlag("original message", first_iter, &value) == 0 && value == 1,
                "original setup finds the same identity after the packed value bit changes");
        MR::updateAlreadyDoneFlag(0, 0);
        require((flags.mDoneInfos[0]._0 & 0x8000) == 0, "original forwarding updates the actual original bit");
        const auto next_link = placement(5, 0);
        require(MR::setupAlreadyDoneFlag("original message", JMapInfoIter(&next_link, 0), &value) == 1 && value == 0,
                "the original link ID distinguishes otherwise identical placements");
        const auto next_zone = placement(6, 0);
        require(MR::setupAlreadyDoneFlag("original message", JMapInfoIter(&next_zone, 0), &value) == 2 && value == 0,
                "the original zone ID distinguishes otherwise identical placements");
        flags.clear();
        for (s32 i = 0; i < 64; ++i) {
            const auto entry = placement(i, i);
            require(MR::setupAlreadyDoneFlag("original message", JMapInfoIter(&entry, 0), &value) == i && value == 0,
                    "cleared original owner admits all 64 distinct placement records");
        }
        GameDataFunction::addStarPiece(13);
        {
            StageSessionState nested("Game", "NestedFixture", 2, JMapIdInfo(7, 2));
            require(try_active_stage_session() == session.get(), "nested constructor preserves the outer active stage");
            StageSessionBinding inner(nested);
            require(GameDataFunction::getStarPieceNum() == 0, "different stage owner has separate original counts");
            GameDataFunction::addStarPiece(42);
        }
        require(GameDataFunction::getStarPieceNum() == 13, "nested teardown restores the original count owner");
    }
    session.reset();
    require(lease.expired(), "typed scene cleanup releases its final retained Game arena");
    rejected([] { (void)GameDataFunction::getStarPieceNum(); });
}
void test_original_counter_leaves(GameDataHolder& profile) {
    profile.addPlayerLeft(17 - profile.getPlayerLeft());
    ScopedGameDataHolderOverride current(profile);
    require(MR::getPlayerLeft() == 17, "HUD life query reads the actual selected profile");
    MR::incPlayerLeft();
    require(MR::getPlayerLeft() == 18 && GameDataFunction::getPlayerLeft() == 18,
            "life increment and query use the same original profile owner");
    profile.setGameEventValue("MissPointForLetter", 19);
    profile.setGameEventValue("MissNum", 9998);
    MR::decPlayerLeft();
    require(MR::getPlayerLeft() == 17 && profile.getGameEventValue("MissPointForLetter") == 20 &&
                profile.getPlayerMissNum() == 9999,
            "one death must update lives, letter miss points and miss count in the same selected profile");
    MR::decPlayerLeft();
    require(MR::getPlayerLeft() == 16 && profile.getGameEventValue("MissPointForLetter") == 20 &&
                profile.getPlayerMissNum() == 9999,
            "death counters retain original upper saturation");
    GameDataFunction::addMissPoint(-99);
    require(profile.getGameEventValue("MissPointForLetter") == 0, "letter miss points clamp below zero");
    profile.setGameEventValue("MissPointForLetter", 1);
    GameDataFunction::addMissPoint(std::numeric_limits<s32>::max());
    require(profile.getGameEventValue("MissPointForLetter") == 0,
            "letter miss point addition preserves PPC unsigned wrap followed by signed clamp");
    profile.addPlayerLeft(-profile.getPlayerLeft());
    MR::decPlayerLeft();
    require(MR::getPlayerLeft() == 0 && profile.getGameEventValue("MissPointForLetter") == 1,
            "zero remaining lives still records the real death counters");
    require(MR::getPowerStarNum() == profile.calcCurrentPowerStarNum(), "HUD Power Star query uses actual profile state");
    ScenePlayingResult result;
    require(result.getCoinNum() == 0 && result.mPurpleCoinNum == 0, "original scene result initializes both coin counters");
    result.mCoinNum = -3; require(result.getCoinNum() == 0, "original coin query clamps its lower bound");
    result.mCoinNum = 1234; require(result.getCoinNum() == 999, "original coin query clamps its upper bound");
    for (const auto& [stage, expected] : std::array{
             std::pair{"AstroGalaxy", true}, std::pair{"AstroDome", true},
             std::pair{"LibraryRoom", true}, std::pair{"OwnerFixture", false}}) {
        StageSessionState state("Game", stage, 1, JMapIdInfo(0, 0));
        StageSessionBinding binding(state);
        require(MR::isStageAstroLocation() == expected, "stage category keeps the original exact stage-name predicate");
    }
}
void test_actual_profile_owner() {
    const char* disc = std::getenv("SMGPC_REAL_DISC");
    require(disc && aurora_dvd_open(disc), "Original profile counters require the authored scenario catalog");
    struct DiscGuard { ~DiscGuard() { aurora_dvd_close(); } } disc_guard;
    DVDInit();
    aurora::g_config.mem1Size = 24U * 1024U * 1024U;
    smgpc::resource::GameResourceRuntime resources;
    smgpc::runtime::DvdFileSystemService dvd({});
    smgpc::runtime::ArchiveMountService mounts(dvd);
    auto catalog = std::make_shared<smgpc::runtime::ScenarioCatalogOwnership>(
        resources.host_heaps(), resources.budget().scenario_catalog_bytes, mounts);
    GameDataSession profile(1, resources, catalog);
    test_original_counter_leaves(profile.holder());
}
}
int main() {
    try { test_actual_temporary_owner(); test_actual_profile_owner(); std::cout << "Original scene counter ownership passed\n"; return 0; }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
