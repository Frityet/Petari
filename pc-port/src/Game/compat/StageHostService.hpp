#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <revolution.h>

#include "Game/compat/NameObjFactoryCompat.hpp"

class NameObj;

namespace smgpc::game {

    class RuntimeContext;

    struct StageHostRequest {
        std::string scene_name;
        std::string stage_name;
        s32 scenario_no = 1;
    };

    class StageHostService final {
    public:
        explicit StageHostService(RuntimeContext &runtime);
        ~StageHostService();

        StageHostService(const StageHostService &) = delete;
        StageHostService &operator=(const StageHostService &) = delete;

        void request_stage(const StageHostRequest &request);

        [[nodiscard]] bool has_active_stage(std::string_view stage_name) const;
        [[nodiscard]] std::string_view active_scene_name() const;
        [[nodiscard]] std::string_view active_stage_name() const;
        [[nodiscard]] s32 active_scenario_no() const;
#ifndef NDEBUG
        [[nodiscard]] std::optional<FileSelectStageState> file_select_state() const;
#endif

    private:
        void create_file_select_stage(const StageHostRequest &request);

        RuntimeContext &_runtime;
        std::string _active_scene_name;
        std::string _active_stage_name;
        s32 _active_scenario_no = 0;
        std::unique_ptr<NameObj> _stage_root;
    };

}  // namespace smgpc::game
