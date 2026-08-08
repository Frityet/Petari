#pragma once

#include "compat/StageSessionState.hpp"

#include <string_view>

#include <revolution/types.h>

namespace smgpc::runtime {
    class AudioEventService;
    class DvdFileSystemService;
}  // namespace smgpc::runtime

namespace smgpc::compat {

    [[nodiscard]] StageScenarioMetadata resolve_stage_scenario_metadata(
        smgpc::runtime::DvdFileSystemService &dvd, std::string_view stage_name, s32 scenario_no);

    // Call after installing the matching StageSessionBinding. The exact
    // AudStageBgmWrap retail table resolves a raw initial BGM ID or proves
    // that the scenario starts with no stage BGM.
    void begin_stage_audio(smgpc::runtime::AudioEventService &audio, std::string_view scene_name,
                           std::string_view stage_name, s32 scenario_no);
    void end_stage_audio(smgpc::runtime::AudioEventService &audio);

}  // namespace smgpc::compat
