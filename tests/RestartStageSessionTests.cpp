#include "Game/LiveActor/LiveActor.hpp"
#include "compat/StageScenarioMetadataResolver.hpp"
#include "compat/StageSessionState.hpp"
#include "runtime/RuntimeServices.hpp"

#include <JSystem/JAudio2/JAISound.hpp>

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
    class StatePlayer final : public LiveActor {
    public:
        StatePlayer() : LiveActor("restart-player-state") {}
        bool nerve_change_enabled = true;
    };

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
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

    const auto *configured_disc = std::getenv("SMGPC_REAL_DISC");
    const auto disc_path = std::filesystem::path(
        configured_disc != nullptr && *configured_disc != '\0' ? configured_disc : "RMGK01.iso");
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
        require(smgpc::compat::try_active_stage_session() == &file_select_session &&
                    file_select_session.metadata().comet_type == smgpc::compat::StageCometType::None,
                "the native session retains the explicit no-comet value from the retail schema");
    }
    ++passed;

    auto session = smgpc::compat::StageSessionState("Game", "HeavensDoorGalaxy", 1, JMapIdInfo(0, 0), metadata);
    const auto session_binding = smgpc::compat::StageSessionBinding(session);
    require(smgpc::compat::try_active_stage_session() == &session &&
                session.scene_name() == "Game" && session.stage_name() == "HeavensDoorGalaxy" && session.scenario_no() == 1,
            "the native binding exposes the selected full-lifetime stage session");
    require(session.initial_start_id()._0 == 0 && session.initial_start_id().mZoneID == 0,
            "the immutable initial start ID must remain queryable");
    session.set_restart_id(JMapIdInfo(3, 7));
    require(session.restart_id()._0 == 3 && session.restart_id().mZoneID == 7 &&
                session.initial_start_id()._0 == 0 && session.initial_start_id().mZoneID == 0,
            "restart mutation must not overwrite the immutable initial start ID");
    ++passed;

    session.set_metadata({});
    require(!session.metadata().comet_type.has_value(),
            "unresolved native metadata remains distinct from explicit no comet");
    auto purple_metadata = metadata;
    purple_metadata.comet_type = smgpc::compat::StageCometType::Purple;
    session.set_metadata(purple_metadata);
    require(session.metadata().comet_type == smgpc::compat::StageCometType::Purple,
            "native metadata preserves Purple without collapsing it into no comet");
    session.set_metadata(metadata);
    require(session.metadata().comet_type == smgpc::compat::StageCometType::None,
            "replacing metadata restores the exact retail no-comet value");
    ++passed;

    auto audio = smgpc::runtime::AudioEventService{};
    require(!audio.is_stage_bgm_identity_resolved(), "new native audio state has no established BGM identity");
    audio.resolve_stage_bgm_absent();
    require(audio.is_stage_bgm_identity_resolved() && !audio.has_active_stage_bgm() &&
                !audio.current_stage_bgm_id().has_value(),
            "native audio distinguishes known absence from unresolved identity");
    ++passed;

    auto player = smgpc::runtime::PlayerSystemService{};
    auto player_actor = StatePlayer{};
    player.attach_actor(player_actor);
    require(!player.player_dead_state().has_value(),
            "generic LiveActor death must not substitute for an explicit nerve-change capability");
    player.attach_actor(player_actor, {
        .read_nerve_change_enabled = +[](const LiveActor& actor) {
            return static_cast<const StatePlayer&>(actor).nerve_change_enabled;
        },
    });
    require(player.player_dead_state() == false, "the attached player nerve-change capability must be queried");
    player_actor.nerve_change_enabled = false;
    require(player.player_dead_state() == true, "a nerve change must be visible immediately without frame synchronization");
    player_actor.nerve_change_enabled = true;
    require(player.player_dead_state() == false, "returning to a changeable nerve must clear the death query");

    audio.set_cube_bgm_change_invalid(true);
    require(audio.is_cube_bgm_change_invalid(), "native audio retains cube BGM invalidation state");
    ++passed;

    audio.reset_stage_state();
    require(!audio.has_active_stage_bgm() && !audio.is_stage_bgm_identity_resolved() &&
                !audio.current_stage_bgm_id().has_value() && !audio.is_cube_bgm_change_invalid(),
            "native stage reset clears identity, activity and cube-local state");
    audio.resolve_stage_bgm_absent();
    require(audio.is_stage_bgm_identity_resolved() && !audio.has_active_stage_bgm(),
            "a second native audio lifetime reconstructs known absence without stale activity");
    ++passed;

    std::cout << "Restart/stage-session tests passed: " << passed << "/8\n";
    return 0;
}
