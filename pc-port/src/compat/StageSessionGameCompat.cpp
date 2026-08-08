#include "Game/Util/EventUtil.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "compat/StageSessionState.hpp"

#include <stdexcept>
#include <string_view>

namespace {
    [[nodiscard]] smgpc::compat::StageCometType require_comet_type() {
        const auto &metadata = smgpc::compat::require_active_stage_session().metadata();
        if (!metadata.comet_type.has_value()) {
            throw std::logic_error("The active scenario has no resolved comet metadata.");
        }
        return *metadata.comet_type;
    }

    [[nodiscard]] bool require_metadata_flag(const std::optional<bool> &flag, std::string_view name) {
        if (!flag.has_value()) {
            throw std::logic_error("The active scenario has no resolved " + std::string(name) + " metadata.");
        }
        return *flag;
    }
}  // namespace

namespace MR {

    JMapIdInfo *getPlayerRestartIdInfo() {
        return &smgpc::compat::require_active_stage_session().restart_id();
    }

    void setPlayerRestartIdInfo(const JMapIdInfo &restart_id) {
        smgpc::compat::require_active_stage_session().set_restart_id(restart_id);
    }

    bool isGalaxyRedCometAppearInCurrentStage() {
        return require_comet_type() == smgpc::compat::StageCometType::Red;
    }

    bool isGalaxyDarkCometAppearInCurrentStage() {
        return require_comet_type() == smgpc::compat::StageCometType::Dark;
    }

    bool isGalaxyGhostCometAppearInCurrentStage() {
        return require_comet_type() == smgpc::compat::StageCometType::Ghost;
    }

    bool isGalaxyQuickCometAppearInCurrentStage() {
        return require_comet_type() == smgpc::compat::StageCometType::Quick;
    }

    bool isGalaxyBlackCometAppearInCurrentStage() {
        return require_comet_type() == smgpc::compat::StageCometType::Black;
    }

    bool isKoopaFortressAppearInGalaxy() {
        const auto &metadata = smgpc::compat::require_active_stage_session().metadata();
        return require_metadata_flag(metadata.koopa_fortress_appeared, "Koopa-fortress appearance");
    }

    bool isOnGameEventFlagAstroGalaxyBgmBright() {
        const auto &metadata = smgpc::compat::require_active_stage_session().metadata();
        return require_metadata_flag(metadata.astro_galaxy_bgm_bright, "Astro Galaxy BGM brightness");
    }

}  // namespace MR
