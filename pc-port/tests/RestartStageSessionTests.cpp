#include "Game/AreaObj/RestartCube.hpp"
#include "Game/AudioLib/AudBgmMgr.hpp"
#include "Game/AudioLib/AudSoundId.hpp"
#include "Game/AudioLib/AudWrap.hpp"
#include "Game/GameAudio/AudStageBgmWrap.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/SoundUtil.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "compat/AudioFacadeCompat.hpp"
#include "compat/PlayerUtilCompat.hpp"
#include "compat/StageScenarioMetadataResolver.hpp"
#include "compat/StageSessionState.hpp"
#include "runtime/RuntimeServices.hpp"

#include <JSystem/JAudio2/JAISound.hpp>

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <filesystem>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void require_unavailable(const std::function<void()> &operation, std::string_view message) {
        try {
            operation();
        } catch (const std::exception &) {
            return;
        }
        throw std::runtime_error(std::string(message));
    }
}  // namespace

int main() {
    auto passed = 0;

    {
        const auto constructed = JAISoundID(0x12U, 0x34U, 0x5678U);
        require(static_cast<u32>(constructed) == 0x12345678U, "JAISoundID section/group/wave composition must be endian-correct");
        const auto composite = JAISoundID(0x89ABCDEFU);
        require(composite.getSectionID() == 0x89U && composite.getGroupID() == 0xABU && composite.getWaveID() == 0xCDEFU,
                "JAISoundID composite accessors must round-trip on the host");
        ++passed;
    }

    const auto disc_path = std::filesystem::path("../RMGK01.iso");
    require(std::filesystem::is_regular_file(disc_path), "the real RMGK01 disc image is required for this focused test");
    auto metadata = smgpc::compat::StageScenarioMetadata{};
    auto file_select_metadata = smgpc::compat::StageScenarioMetadata{};
    [&] {
        aurora_dvd_close();
        require(aurora_dvd_open(disc_path.c_str()), "the real RMGK01 disc image must open");
        struct DiscCloseGuard {
            ~DiscCloseGuard() {
                aurora_dvd_close();
            }
        } close_guard;
        DVDInit();
        auto dvd = smgpc::runtime::DvdFileSystemService("/");
        metadata = smgpc::compat::resolve_stage_scenario_metadata(dvd, "HeavensDoorGalaxy", 1);
        file_select_metadata = smgpc::compat::resolve_stage_scenario_metadata(dvd, "FileSelect", 1);
    }();
    require(metadata.comet_type == smgpc::compat::StageCometType::None,
            "HeavensDoor scenario 1 must resolve explicit no-comet metadata from ScenarioData.bcsv");
    ++passed;

    require(file_select_metadata.comet_type == smgpc::compat::StageCometType::None,
            "a retail ScenarioData row without a Comet field must resolve the retail no-active-comet result");
    {
        auto file_select_session = smgpc::compat::StageSessionState(
            "Game", "FileSelect", 1, JMapIdInfo(0, 0), file_select_metadata);
        const auto file_select_session_binding =
            smgpc::compat::StageSessionBinding(file_select_session);
        require(!MR::isGalaxyRedCometAppearInCurrentStage() &&
                    !MR::isGalaxyDarkCometAppearInCurrentStage() &&
                    !MR::isGalaxyGhostCometAppearInCurrentStage() &&
                    !MR::isGalaxyQuickCometAppearInCurrentStage() &&
                    !MR::isGalaxyBlackCometAppearInCurrentStage() &&
                    static_cast<u32>(AudStageBgmWrap::getCometEventBgm(
                        MR::getCurrentStageName())) == static_cast<u32>(-1),
                "the schema-level absence must flow through the exact retail comet queries as no event");

        auto file_select_audio = smgpc::runtime::AudioEventService{};
        smgpc::compat::begin_stage_audio(
            file_select_audio, "Game", "FileSelect", 1);
        require(file_select_audio.is_stage_bgm_identity_resolved() &&
                    !file_select_audio.has_active_stage_bgm(),
                "the exact stage BGM table must prove FileSelect has no initial stage BGM");
        smgpc::compat::end_stage_audio(file_select_audio);
    }
    ++passed;

    auto session = smgpc::compat::StageSessionState("Game", "HeavensDoorGalaxy", 1, JMapIdInfo(0, 0), metadata);
    const auto session_binding = smgpc::compat::StageSessionBinding(session);
    require(std::string_view(MR::getCurrentStageName()) == "HeavensDoorGalaxy" && MR::getCurrentScenarioNo() == 1,
            "SceneUtil queries must use the full-lifetime stage session");
    require(MR::getInitializeStartIdInfo()._0 == 0 && MR::getInitializeStartIdInfo().mZoneID == 0,
            "the immutable initial start ID must remain queryable");
    MR::setPlayerRestartIdInfo(JMapIdInfo(3, 7));
    require(MR::getPlayerRestartIdInfo()->_0 == 3 && MR::getPlayerRestartIdInfo()->mZoneID == 7 &&
                MR::getInitializeStartIdInfo()._0 == 0 && MR::getInitializeStartIdInfo().mZoneID == 0,
            "restart mutation must not overwrite the immutable initial start ID");
    require(!MR::isGalaxyRedCometAppearInCurrentStage() && !MR::isGalaxyDarkCometAppearInCurrentStage() &&
                !MR::isGalaxyGhostCometAppearInCurrentStage() && !MR::isGalaxyQuickCometAppearInCurrentStage() &&
                !MR::isGalaxyBlackCometAppearInCurrentStage() &&
                static_cast<u32>(AudStageBgmWrap::getCometEventBgm(MR::getCurrentStageName())) == static_cast<u32>(-1),
            "an explicit no-comet row must produce the retail no-override result");
    ++passed;

    session.set_metadata({});
    require_unavailable([] { (void)MR::isGalaxyDarkCometAppearInCurrentStage(); },
                        "missing comet metadata must throw rather than become false");
    session.set_metadata(metadata);
    auto purple_metadata = metadata;
    purple_metadata.comet_type = smgpc::compat::StageCometType::Purple;
    session.set_metadata(purple_metadata);
    require(!MR::isGalaxyRedCometAppearInCurrentStage() && !MR::isGalaxyDarkCometAppearInCurrentStage() &&
                !MR::isGalaxyGhostCometAppearInCurrentStage() && !MR::isGalaxyQuickCometAppearInCurrentStage() &&
                !MR::isGalaxyBlackCometAppearInCurrentStage() &&
                static_cast<u32>(AudStageBgmWrap::getCometEventBgm(MR::getCurrentStageName())) == static_cast<u32>(-1),
            "Purple must remain a resolved comet type while producing no five-predicate BGM override");
    session.set_metadata(metadata);
    require(!MR::isPowerStarGetDemoActive(), "a new stage session must explicitly begin outside the Power Star get demo");
    session.set_power_star_get_demo_active(true);
    require(MR::isPowerStarGetDemoActive(), "Power Star get demo state must be an explicit mutable stage-session value");
    session.set_power_star_get_demo_active(false);
    ++passed;

    auto audio = smgpc::runtime::AudioEventService{};
    const auto audio_binding = smgpc::compat::ScopedAudioEventServiceOverride(audio);
    smgpc::compat::begin_stage_audio(audio, "Game", "HeavensDoorGalaxy", 1);
    require(audio.is_stage_bgm_identity_resolved() && !audio.has_active_stage_bgm() && !MR::isPlayingStageBgmID(STM_STAR_EXIST),
            "the exact stage table must prove HeavensDoor scenario 1 starts with no current BGM");
    require(AudWrap::getBgmMgr()->mCurrentBGM[AudBgmMgr::BgmType_Stage] == static_cast<u32>(-1),
            "the genuine AudBgmMgr facade must expose the proven known-empty current ID");
    ++passed;

    auto player = smgpc::runtime::PlayerSystemService{};
    auto player_actor = LiveActor("restart-player-state");
    player.attach_actor(player_actor);
    const auto player_binding = smgpc::compat::ScopedPlayerSystemServiceOverride(player);
    require_unavailable([] { (void)MR::isPlayerDead(); },
                        "generic LiveActor death must not substitute for Mario nerve-change/death state");
    player.set_player_dead_state(false);
    require(!MR::isPlayerDead(), "an explicitly supplied live-player nerve state must be observable");

    auto restart_cube = RestartCube(0, "RestartCube");
    restart_cube._40 = 1;
    restart_cube._44 = -1;
    require_unavailable(
        [&] { restart_cube.changeBgm(); },
        "RestartCube multi-BGM must stay unavailable without the concrete retail scheduler");
    require(!audio.current_stage_bgm_id().has_value() &&
                !audio.has_active_stage_bgm() &&
                AudWrap::getBgmMgr()->mCurrentBGM[AudBgmMgr::BgmType_Stage] ==
                    static_cast<u32>(-1),
            "failed multi-BGM playback must not leave an event-only current ID");
    MR::setCubeBgmChangeInvalid();
    require(MR::isCubeBgmChangeInvalid(), "cube BGM invalidation must be retained as real stage-audio state");
    ++passed;

    smgpc::compat::end_stage_audio(audio);
    require(!audio.has_active_stage_bgm() && !audio.is_stage_bgm_identity_resolved() && !audio.is_cube_bgm_change_invalid(),
            "stage-audio teardown must clear active IDs, resolution, handles, and cube-local state");
    require(!MR::isPlayingStageBgmID(MBGM_GALAXY_25),
            "a torn-down stage must report no concrete raw-ID playback");
    require_unavailable([] { (void)AudWrap::getStageBgm(); },
                        "a torn-down stage must not turn unknown facade identity into a null BGM fallback");
    smgpc::compat::begin_stage_audio(audio, "Game", "HeavensDoorGalaxy", 1);
    require(audio.is_stage_bgm_identity_resolved() && !audio.has_active_stage_bgm() && AudWrap::getStageBgm() == nullptr,
            "a second stage-audio lifetime must reconstruct the exact known-empty state without stale BGM objects");
    ++passed;

    std::cout << "Restart/stage-session tests passed: " << passed << "/8\n";
    return 0;
}
