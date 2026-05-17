#include "Game/compat/StageHostService.hpp"

#include "Game/NameObj/NameObj.hpp"
#include "Game/compat/RuntimeContext.hpp"

#include <stdexcept>

namespace smgpc::game {

    StageHostService::StageHostService(RuntimeContext &runtime) : _runtime(runtime) {
    }

    StageHostService::~StageHostService() = default;

    void StageHostService::request_stage(const StageHostRequest &request) {
        if (request.stage_name == "FileSelect") {
            create_file_select_stage(request);
            return;
        }

        throw std::runtime_error("Unsupported stage host request: " + request.stage_name);
    }

    bool StageHostService::has_active_stage(std::string_view stage_name) const {
        return _stage_root != nullptr && _active_stage_name == stage_name;
    }

    std::string_view StageHostService::active_scene_name() const {
        return _active_scene_name;
    }

    std::string_view StageHostService::active_stage_name() const {
        return _active_stage_name;
    }

    s32 StageHostService::active_scenario_no() const {
        return _active_scenario_no;
    }

#ifndef NDEBUG
    std::optional<FileSelectStageState> StageHostService::file_select_state() const {
        if (_stage_root == nullptr) {
            return std::nullopt;
        }

        return file_select_stage_state(*_stage_root);
    }
#endif

    void StageHostService::create_file_select_stage(const StageHostRequest &request) {
        if (has_active_stage(request.stage_name)) {
            return;
        }

        _active_scene_name = request.scene_name;
        _active_stage_name = request.stage_name;
        _active_scenario_no = request.scenario_no;
#ifndef NDEBUG
        _runtime.emit_semantic_trace_event("sequence", "temporary_file_select_stage_host_started",
                                           "stage host factory created FileSelector until GameScene placement is available");
        _runtime.emit_sequence_state_trace_event("temporary_stage_host_started", "host=FileSelector;stage=FileSelect");
#endif
        auto stage_root = create_name_obj("FileSelector", "ファイルセレクタ");
#ifndef NDEBUG
        _runtime.emit_semantic_trace_event("file_select", "file_selector_constructed", "stage host factory");
#endif
        stage_root->initWithoutIter();
#ifndef NDEBUG
        _runtime.emit_semantic_trace_event("file_select", "file_selector_initialized", "stage host factory initWithoutIter");
        _runtime.emit_semantic_trace_event("title", "title_product_created", "source=StageHostService;layouts=TitleLogo,PressStart");
#endif
        _stage_root = std::move(stage_root);
    }

}  // namespace smgpc::game
