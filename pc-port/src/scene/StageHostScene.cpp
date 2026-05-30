#include "scene/StageHostScene.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/NameObjLifecycleService.hpp"
#include "scene/SceneExecutionService.hpp"
#include "scene/StagePlacementResolver.hpp"
#include "scene/nameobj/NameObjFactory.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace smgpc::scene {
    namespace {

#ifndef NDEBUG
        [[nodiscard]] std::filesystem::path debug_stage_placement_report_path() {
            const auto *path = std::getenv("SMGPC_STAGE_PLACEMENT_REPORT_PATH");
            if (path == nullptr || path[0] == '\0') {
                return {};
            }

            return std::filesystem::path(path);
        }

        [[nodiscard]] bool placement_is_created(const StagePlacementObject &placement) {
            return placement.factory_supported || placement.model_fallback_supported || placement.alias_model_fallback_supported;
        }

        [[nodiscard]] std::string_view placement_status_name(const StagePlacementObject &placement) {
            if (placement.factory_supported) {
                return "created";
            }
            if (placement.model_fallback_supported) {
                return "created_model_fallback";
            }
            if (placement.alias_model_fallback_supported) {
                return "created_alias_model_fallback";
            }
            if (placement.intentionally_ignored) {
                return "ignored";
            }
            return "blocked";
        }

        void write_stage_placement_report(std::string_view stage_name, s32 scenario_no, const std::vector<StagePlacementObject> &placements,
                                          const std::vector<const StagePlacementObject *> &blocked_placements) {
            const auto report_path = debug_stage_placement_report_path();
            if (report_path.empty()) {
                return;
            }

            std::error_code error;
            if (!report_path.parent_path().empty()) {
                std::filesystem::create_directories(report_path.parent_path(), error);
            }

            auto out = std::ofstream(report_path);
            if (!out) {
                return;
            }

            const auto created_count = std::ranges::count_if(placements, placement_is_created);
            const auto ignored_count = std::ranges::count_if(placements, [](const auto &placement) { return placement.intentionally_ignored; });
            out << "# Stage Placement Report\n";
            out << "stage: " << stage_name << "\n";
            out << "scenario: " << scenario_no << "\n";
            out << "total_objects: " << placements.size() << "\n";
            out << "created_objects: " << created_count << "\n";
            out << "blocked_objects: " << blocked_placements.size() << "\n";
            out << "intentionally_ignored_objects: " << ignored_count << "\n\n";
            out << "## Objects\n";
            for (const auto &placement : placements) {
                out << "- status: " << placement_status_name(placement) << "\n";
                out << "  object: " << placement.object_name << "\n";
                out << "  zone: " << placement.zone_name << "\n";
                out << "  table: " << placement.table_path << "\n";
                out << "  row: " << placement.jmap_entry_index << "\n";
                out << "  child_count: " << placement.child_object_count << "\n";
                out << "  support_reason: " << placement.support_reason << "\n";
                out << "  model_archive: " << placement.model_archive_name << "\n";
                out << "  archive: " << placement.object_archive_path << "\n";
            }
        }
#endif

        [[nodiscard]] std::string unsupported_placement_error(std::string_view stage_name,
                                                              const std::vector<const StagePlacementObject *> &blocked_placements) {
            auto out = std::ostringstream();
            out << "Unsupported placement objects for " << stage_name << ": " << blocked_placements.size() << " blocked";
            if (!blocked_placements.empty()) {
                out << "; first=" << blocked_placements.front()->object_name << " in " << blocked_placements.front()->table_path;
            }
            return out.str();
        }

    }  // namespace

    StageHostScene::StageHostScene(smgpc::runtime::RuntimeContext &runtime, StageHostRequest request)
        : Scene(!request.stage_name.empty() ? request.stage_name.c_str() : "StageHostScene"), _runtime(runtime), _request(std::move(request)) {
    }

    StageHostScene::~StageHostScene() {
        destroy_roots();
        if (MR::getSceneObjHolder() == mSceneObjHolder) {
            MR::setCurrentSceneObjHolder(nullptr);
        }
    }

    void StageHostScene::init() {
        if (!_roots.empty()) {
            return;
        }

        initSceneObjHolder();
        MR::setCurrentSceneObjHolder(mSceneObjHolder);
        _runtime.player_system().show_player();

        if (!_request.object_name.empty()) {
            init_explicit_root();
        } else {
            init_placement_roots();
        }

        init_roots_after_placement();
        appear_roots();
    }

    void StageHostScene::construct_root_object(std::string_view object_name, std::string_view actor_name, const StagePlacementObject *placement) {
        if (!smgpc::scene::nameobj::can_create_name_obj(_runtime.dvd(), object_name)) {
            throw std::runtime_error("Unsupported stage host request object: " + std::string(object_name) + " for stage " + _request.stage_name);
        }

        auto &lifecycle = _runtime.name_obj_lifecycle();
        lifecycle.preload_archives(object_name);
        auto root = lifecycle.construct(object_name, actor_name);
#ifndef NDEBUG
        _runtime.emit_semantic_trace_event("sequence", "stage_host_constructed",
                                           "host=" + std::string(object_name) + ";stage=" + _request.stage_name);
#endif
        lifecycle.init(*root, placement);
#ifndef NDEBUG
        _runtime.emit_semantic_trace_event("sequence", "stage_host_initialized",
                                           "host=" + std::string(object_name) + ";stage=" + _request.stage_name);
#endif
        _roots.push_back(std::move(root));
    }

    void StageHostScene::init_explicit_root() {
        construct_root_object(_request.object_name, resolve_actor_name(_request.object_name), nullptr);
    }

    void StageHostScene::init_placement_roots() {
        const auto placements = resolve_stage_placement_objects(_runtime.dvd(), _request.stage_name, _request.scenario_no);
        auto blocked_placements = std::vector<const StagePlacementObject *>{};
        for (const auto &placement : placements) {
            trace_placement_object(placement);
            if (placement.intentionally_ignored) {
                continue;
            }
            if (!placement.factory_supported && !placement.model_fallback_supported && !placement.alias_model_fallback_supported &&
                !placement.intentionally_ignored) {
                blocked_placements.push_back(&placement);
                continue;
            }
            construct_root_object(placement.object_name, resolve_actor_name(placement.object_name), &placement);
        }

#ifndef NDEBUG
        _runtime.emit_semantic_trace_event("placement", "stage_placement_summary",
                                           "stage=" + _request.stage_name + ";scenario=" + std::to_string(_request.scenario_no) +
                                               ";objects=" + std::to_string(placements.size()) +
                                               ";created=" + std::to_string(std::ranges::count_if(placements, placement_is_created)) +
                                               ";ignored=" +
                                               std::to_string(std::ranges::count_if(placements, [](const auto &placement) {
                                                   return placement.intentionally_ignored;
                                               })) +
                                               ";blocked=" + std::to_string(blocked_placements.size()));
        write_stage_placement_report(_request.stage_name, _request.scenario_no, placements, blocked_placements);
#endif

        if (_request.fail_unsupported_placement && !blocked_placements.empty()) {
            throw std::runtime_error(unsupported_placement_error(_request.stage_name, blocked_placements));
        }

        if (_roots.empty()) {
            construct_root_object(_request.stage_name, resolve_actor_name(_request.stage_name), nullptr);
        }
    }

    void StageHostScene::trace_placement_object(const StagePlacementObject &placement) const {
#ifndef NDEBUG
        _runtime.emit_semantic_trace_event("placement", "stage_object",
                                           "stage=" + placement.stage_name + ";zone=" + placement.zone_name +
                                               ";scenario=" + std::to_string(_request.scenario_no) +
                                               ";layer=" + placement.layer_name + ";table=" + placement.table_path +
                                               ";object=" + placement.object_name +
                                               ";factory=" + std::string(placement_status_name(placement)) +
                                               ";support_reason=" + placement.support_reason +
                                               ";model_archive=" + placement.model_archive_name +
                                               ";object_archive=" + placement.object_archive_path);
#else
        (void)placement;
#endif
    }

    void StageHostScene::init_roots_after_placement() {
        auto &lifecycle = _runtime.name_obj_lifecycle();
        for (auto &root : _roots) {
            lifecycle.init_after_placement(*root);
        }
    }

    void StageHostScene::appear_roots() {
        if (!_request.appear_after_init) {
            return;
        }

        auto &lifecycle = _runtime.name_obj_lifecycle();
        for (auto &root : _roots) {
            lifecycle.appear(*root);
        }
    }

    void StageHostScene::destroy_roots() {
        auto &lifecycle = _runtime.name_obj_lifecycle();
        for (auto &root : _roots) {
            lifecycle.destroy(*root);
        }
        _roots.clear();
    }

    void StageHostScene::start() {
    }

    void StageHostScene::update() {
        _runtime.scene_execution().execute_movement();
    }

    void StageHostScene::calcAnim() {
        _runtime.scene_execution().execute_calc_anim_and_view();
    }

    void StageHostScene::draw3DNormal(const smgpc::camera::CameraPose &camera_pose) {
        _runtime.scene_execution().draw_3d_normal(camera_pose);
    }

    void StageHostScene::draw2DNormal() {
        _runtime.scene_execution().draw_2d_normal();
    }

    NameObj *StageHostScene::root() const {
        return !_roots.empty() ? _roots.front().get() : nullptr;
    }

    std::string_view StageHostScene::scene_name() const {
        return _request.scene_name;
    }

    std::string_view StageHostScene::stage_name() const {
        return _request.stage_name;
    }

    s32 StageHostScene::scenario_no() const {
        return _request.scenario_no;
    }

    std::string StageHostScene::resolve_actor_name(std::string_view object_name) const {
        return !_request.actor_name.empty() ? _request.actor_name : std::string(object_name);
    }

}  // namespace smgpc::scene
