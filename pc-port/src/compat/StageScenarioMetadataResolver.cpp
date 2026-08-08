#include "compat/StageScenarioMetadataResolver.hpp"

#include "Game/GameAudio/AudStageBgmWrap.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "compat/AudioFacadeCompat.hpp"
#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeServices.hpp"

#include <JSystem/JAudio2/JAISound.hpp>

#include <filesystem>
#include <stdexcept>
#include <string>

namespace {
    [[nodiscard]] smgpc::compat::StageCometType parse_comet_type(std::string_view name) {
        using smgpc::compat::StageCometType;
        if (name.empty()) {
            return StageCometType::None;
        }
        if (name == "Red") {
            return StageCometType::Red;
        }
        if (name == "Dark") {
            return StageCometType::Dark;
        }
        if (name == "Ghost") {
            return StageCometType::Ghost;
        }
        if (name == "Quick") {
            return StageCometType::Quick;
        }
        if (name == "Black") {
            return StageCometType::Black;
        }
        if (name == "Purple") {
            return StageCometType::Purple;
        }
        throw std::runtime_error("ScenarioData contains an unknown retail Comet value: " + std::string(name));
    }
}  // namespace

namespace smgpc::compat {

    StageScenarioMetadata resolve_stage_scenario_metadata(smgpc::runtime::DvdFileSystemService &dvd,
                                                          std::string_view stage_name, s32 scenario_no) {
        if (stage_name.empty() || scenario_no <= 0) {
            throw std::invalid_argument("Scenario metadata resolution requires a stage and positive scenario number.");
        }

        const auto archive_name = std::string(stage_name) + "Scenario.arc";
        const auto archive_path = dvd.find_first({
            std::filesystem::path("StageData") / std::string(stage_name) / archive_name,
        });
        if (!archive_path.has_value()) {
            throw std::runtime_error("The retail scenario archive is unavailable for " + std::string(stage_name) + ".");
        }

        auto &archive = dvd.archive_for_path(*archive_path);
        if (!archive.contains_resource("/ScenarioData.bcsv")) {
            throw std::runtime_error("The retail scenario archive has no ScenarioData.bcsv: " + archive_path->string());
        }

        const auto scenario_info = JMapInfo::from_bcsv(archive.resource_data("/ScenarioData.bcsv"));
        const auto scenario = scenario_info.findElement<s32>("ScenarioNo", scenario_no, 0);
        if (!scenario.isValid()) {
            throw std::runtime_error("ScenarioData has no requested ScenarioNo row.");
        }
        if (scenario_info.searchItemInfo("Comet") < 0) {
            throw std::runtime_error("ScenarioData has no Comet field; absence cannot be inferred.");
        }

        const char *comet_name = nullptr;
        if (!scenario.getValue("Comet", &comet_name)) {
            throw std::runtime_error("ScenarioData Comet value could not be decoded.");
        }

        auto metadata = StageScenarioMetadata{};
        metadata.comet_type = parse_comet_type(comet_name != nullptr ? std::string_view(comet_name) : std::string_view{});
        return metadata;
    }

    void begin_stage_audio(smgpc::runtime::AudioEventService &audio, std::string_view scene_name,
                           std::string_view stage_name, s32 scenario_no) {
        auto &session = require_active_stage_session();
        if (session.scene_name() != scene_name || session.stage_name() != stage_name || session.scenario_no() != scenario_no) {
            throw std::logic_error("Stage audio identity does not match the active stage session.");
        }

        audio.reset_stage_state();
        const auto audio_binding = ScopedAudioEventServiceOverride{audio};
        const auto sound_id = AudStageBgmWrap::changeStageNameToSoundID(
            session.scene_name().c_str(), session.stage_name().c_str(), session.scenario_no());
        if (static_cast<u32>(sound_id) == static_cast<u32>(-1)) {
            audio.resolve_stage_bgm_absent();
        } else {
            audio.start_stage_bgm(static_cast<u32>(sound_id));
        }
        synchronize_audio_facade_state();
    }

    void end_stage_audio(smgpc::runtime::AudioEventService &audio) {
        audio.reset_stage_state();
        const auto audio_binding = ScopedAudioEventServiceOverride{audio};
        synchronize_audio_facade_state();
    }

}  // namespace smgpc::compat
