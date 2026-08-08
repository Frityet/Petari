#pragma once

#include "Game/Util/JMapIdInfo.hpp"

#include <optional>
#include <string>
#include <string_view>

#include <revolution/types.h>

namespace smgpc::compat {

    enum class StageCometType {
        None,
        Red,
        Dark,
        Ghost,
        Quick,
        Black,
        Purple,
    };

    struct StageScenarioMetadata {
        // An empty value means that scenario metadata has not established the
        // active comet. `StageCometType::None` is the explicit retail "none"
        // result and is deliberately distinct from missing data.
        std::optional<StageCometType> comet_type;
        std::optional<bool> koopa_fortress_appeared;
        std::optional<bool> astro_galaxy_bgm_bright;
    };

    class StageSessionState final {
    public:
        StageSessionState(std::string_view scene_name, std::string_view stage_name, s32 scenario_no,
                          const JMapIdInfo &initial_start_id, StageScenarioMetadata metadata = {});

        [[nodiscard]] const std::string &scene_name() const;
        [[nodiscard]] const std::string &stage_name() const;
        [[nodiscard]] s32 scenario_no() const;
        [[nodiscard]] const JMapIdInfo &initial_start_id() const;
        [[nodiscard]] JMapIdInfo &restart_id();
        [[nodiscard]] const JMapIdInfo &restart_id() const;
        void set_restart_id(const JMapIdInfo &restart_id);

        [[nodiscard]] const StageScenarioMetadata &metadata() const;
        void set_metadata(StageScenarioMetadata metadata);

        [[nodiscard]] bool is_power_star_get_demo_active() const;
        void set_power_star_get_demo_active(bool active);

    private:
        std::string _scene_name;
        std::string _stage_name;
        s32 _scenario_no = 0;
        const JMapIdInfo _initial_start_id;
        JMapIdInfo _restart_id;
        StageScenarioMetadata _metadata;
        bool _power_star_get_demo_active = false;
    };

    class StageSessionBinding final {
    public:
        explicit StageSessionBinding(StageSessionState &session);
        ~StageSessionBinding();

        StageSessionBinding(const StageSessionBinding &) = delete;
        StageSessionBinding &operator=(const StageSessionBinding &) = delete;

    private:
        StageSessionBinding *_previous = nullptr;
        StageSessionState *_session = nullptr;
    };

    [[nodiscard]] StageSessionState *try_active_stage_session();
    [[nodiscard]] StageSessionState &require_active_stage_session();

}  // namespace smgpc::compat
