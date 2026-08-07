#include "scene/StageHostScene.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/LightFunction.hpp"
#include "Game/Map/SleepControllerHolder.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "camera/StageStartCamera.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "compat/StagePlayerRuntime.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/NameObjLifecycleService.hpp"
#include "scene/SceneExecutionService.hpp"
#include "scene/StagePlacementResolver.hpp"
#include "scene/nameobj/NameObjFactory.hpp"
#include "scene/nameobj/ObjectNameTable.hpp"

#include <algorithm>
#include <array>
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
            return placement.factory_supported;
        }

        [[nodiscard]] std::string_view placement_status_name(const StagePlacementObject &placement) {
            if (placement.factory_supported) {
                return "created";
            }
            if (placement.intentionally_ignored) {
                return "ignored";
            }
            return "blocked";
        }

        struct PlacementRailSummary {
            bool attached = false;
            s32 path_info_index = -1;
            s32 point_count = 0;
            bool has_first_point = false;
            std::array<f32, 3U> first_point{};
        };

        [[nodiscard]] PlacementRailSummary placement_rail_summary(const StagePlacementObject &placement) {
            const JMapInfo *point_info = nullptr;
            auto path_info_index = s32{-1};
            const auto attached = placement.jmap_info.getRailInfo(placement.jmap_entry_index, nullptr, &point_info, &path_info_index);
            auto first_point = std::array<f32, 3U>{};
            const auto has_first_point = point_info != nullptr && point_info->getValue(0, "pnt0_x", &first_point[0]) &&
                                         point_info->getValue(0, "pnt0_y", &first_point[1]) &&
                                         point_info->getValue(0, "pnt0_z", &first_point[2]);
            return PlacementRailSummary{
                .attached = attached,
                .path_info_index = path_info_index,
                .point_count = point_info != nullptr ? point_info->getNumEntries() : 0,
                .has_first_point = has_first_point,
                .first_point = first_point,
            };
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
                const auto rail = placement_rail_summary(placement);
                out << "- status: " << placement_status_name(placement) << "\n";
                out << "  object: " << placement.object_name << "\n";
                out << "  zone: " << placement.zone_name << "\n";
                out << "  zone_id: " << placement.zone_id << "\n";
                out << "  table: " << placement.table_path << "\n";
                out << "  row: " << placement.jmap_entry_index << "\n";
                out << "  child_count: " << placement.child_object_count << "\n";
                out << "  common_path_id: " << placement.common_path_id << "\n";
                out << "  rail_info_attached: " << (rail.attached ? "true" : "false") << "\n";
                out << "  rail_path_row: " << rail.path_info_index << "\n";
                out << "  rail_point_count: " << rail.point_count << "\n";
                if (rail.has_first_point) {
                    out << "  rail_first_point: [" << rail.first_point[0] << ", " << rail.first_point[1] << ", " << rail.first_point[2] << "]\n";
                }
                out << "  support_reason: " << placement.support_reason << "\n";
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

        [[nodiscard]] bool same_placement_identity(const StagePlacementObject &left,
                                                   const StagePlacementObject &right) {
            return left.archive_path == right.archive_path && left.table_path == right.table_path &&
                   left.zone_id == right.zone_id && left.jmap_entry_index == right.jmap_entry_index;
        }

    }  // namespace

    bool should_apply_host_appear(const StagePlacementObject *placement, bool explicit_root) {
        return explicit_root || placement == nullptr;
    }

    StageHostScene::StageHostScene(smgpc::runtime::RuntimeContext &runtime, StageHostRequest request)
        : Scene(!request.stage_name.empty() ? request.stage_name.c_str() : "StageHostScene"), _runtime(runtime),
          _registration_scope_id(runtime.begin_scene_registration_scope()), _request(std::move(request)) {
    }

    StageHostScene::~StageHostScene() {
        _runtime.camera_system().clear_game_camera_pose();
        _collision.deactivate();
        _gravity.deactivate();
        // Scheduler registrations retain raw object pointers, so remove the scene
        // scope while its roots and child objects are still alive.
        (void)_runtime.end_scene_registration_scope(_registration_scope_id);
        // Demo definitions own cast memberships and callback clones. Release
        // them while all actor pointers are still valid and after their
        // scheduler entries can no longer run.
        _demo_scene_runtime.reset();
        if (_stage_player != nullptr) {
            _stage_player.reset();
            _runtime.player_system().clear_stage_state();
        }
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
        LightFunction::initLightRegisterAll();
        _runtime.player_system().show_player();

        if (!_request.object_name.empty()) {
            init_explicit_root();
        } else {
            init_placement_roots();
        }

        init_roots_after_placement();
        init_stage_start_camera();
        SleepControlFunc::initSyncSleepController();
        appear_roots();
    }

    void StageHostScene::construct_root_object(std::string_view object_name, std::string_view actor_name,
                                               const StagePlacementObject *placement, bool explicit_root) {
        if (!smgpc::scene::nameobj::can_create_name_obj(object_name)) {
            throw std::runtime_error("Unsupported stage host request object: " + std::string(object_name) + " for stage " + _request.stage_name);
        }

        auto &lifecycle = _runtime.name_obj_lifecycle();
        lifecycle.preload_archives(object_name, placement);
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
        _root_host_appear.push_back(should_apply_host_appear(placement, explicit_root));
    }

    void StageHostScene::init_explicit_root() {
        init_stage_environment();

        const auto explicit_placement = std::ranges::find_if(_placements, [this](const auto &placement) {
            return placement.object_name == _request.object_name && placement.factory_supported;
        });
        const auto *placement = explicit_placement != _placements.end() ? &*explicit_placement : nullptr;
        const auto actor_name = !_request.actor_name.empty() ? std::string_view(_request.actor_name) :
                                placement != nullptr         ? resolve_placement_actor_name(*placement) :
                                                               std::string_view(_request.object_name);
        construct_root_object(_request.object_name, actor_name, placement, true);
        construct_placement_roots(placement);
    }

    void StageHostScene::init_placement_roots() {
        init_stage_environment();
        construct_placement_roots();
    }

    void StageHostScene::construct_placement_roots(const StagePlacementObject *explicit_placement) {
        auto blocked_placements = std::vector<const StagePlacementObject *>{};
        for (const auto &placement : _placements) {
            trace_placement_object(placement);
            if (explicit_placement != nullptr && same_placement_identity(placement, *explicit_placement)) {
#ifndef NDEBUG
                _runtime.emit_semantic_trace_event(
                    "placement", "stage_object_deduplicated",
                    "stage=" + placement.stage_name + ";object=" + placement.object_name +
                        ";table=" + placement.table_path + ";row=" + std::to_string(placement.jmap_entry_index) +
                        ";reason=explicit_root_same_data_identity");
#endif
                continue;
            }
            if (placement.intentionally_ignored) {
                continue;
            }
            if (!placement.factory_supported && !placement.intentionally_ignored) {
                blocked_placements.push_back(&placement);
                continue;
            }
            // A request actor-name override identifies only its explicit root;
            // data-driven placement actors use the original localized runtime
            // name while their factory/archive/model identifier stays English.
            construct_root_object(placement.object_name, resolve_placement_actor_name(placement), &placement);
        }

#ifndef NDEBUG
        _runtime.emit_semantic_trace_event("placement", "stage_placement_summary",
                                           "stage=" + _request.stage_name + ";scenario=" + std::to_string(_request.scenario_no) +
                                               ";objects=" + std::to_string(_placements.size()) +
                                               ";created=" + std::to_string(std::ranges::count_if(_placements, placement_is_created)) +
                                               ";ignored=" +
                                               std::to_string(std::ranges::count_if(_placements, [](const auto &placement) {
                                                   return placement.intentionally_ignored;
                                               })) +
                                               ";blocked=" + std::to_string(blocked_placements.size()));
        write_stage_placement_report(_request.stage_name, _request.scenario_no, _placements, blocked_placements);
#endif

        if (_request.fail_unsupported_placement && !blocked_placements.empty()) {
            throw std::runtime_error(unsupported_placement_error(_request.stage_name, blocked_placements));
        }
    }

    void StageHostScene::init_stage_environment() {
        if (_object_name_table == nullptr) {
            _object_name_table = std::make_unique<smgpc::scene::nameobj::ObjectNameTable>(_runtime.dvd());
        }
        _placements = resolve_stage_placement_objects(_runtime.dvd(), _request.stage_name, _request.scenario_no);
        // The original DemoDirector/executors exist before placement actors
        // initialize and attempt to join their zone-scoped groups.
        _demo_scene_runtime = std::make_unique<smgpc::compat::DemoSceneRuntime>(_runtime.dvd(), _placements);
        const auto collision_stats = _collision.load(_runtime.dvd(), _placements);
        _collision.activate();
        const auto gravity_stats = _gravity.load(_placements);
        _gravity.activate();
        _stage_start_info = resolve_stage_start_info(_runtime.dvd(), _request.stage_name, _request.scenario_no,
                                                     _request.start_id, _request.start_zone_id);
        init_stage_player();

#ifndef NDEBUG
        _runtime.emit_semantic_trace_event(
            "collision", "stage_collision_loaded",
            "stage=" + _request.stage_name + ";scenario=" + std::to_string(_request.scenario_no) +
                ";placement_rows=" + std::to_string(_placements.size()) +
                ";factory_placements=" + std::to_string(collision_stats.placement_count) +
                ";archives=" + std::to_string(collision_stats.archive_count) +
                ";meshes=" + std::to_string(collision_stats.mesh_count) +
                ";triangles=" + std::to_string(collision_stats.triangle_count) +
                ";rejected_triangles=" + std::to_string(collision_stats.rejected_triangle_count));
        _runtime.emit_semantic_trace_event(
            "gravity", "stage_gravity_loaded",
            "stage=" + _request.stage_name + ";scenario=" + std::to_string(_request.scenario_no) +
                ";placements=" + std::to_string(gravity_stats.placement_count) +
                ";gravities=" + std::to_string(gravity_stats.gravity_count) +
                ";unsupported=" + std::to_string(gravity_stats.unsupported_count));
#endif
    }

    void StageHostScene::init_stage_player() {
        _stage_player.reset();
        if (_stage_start_info.has_value()) {
            _stage_player = std::make_unique<smgpc::compat::StagePlayerRuntime>(_runtime, *_stage_start_info);
#ifndef NDEBUG
            const auto &start = *_stage_start_info;
            _runtime.emit_semantic_trace_event(
                "player", "stage_player_created",
                "stage=" + _request.stage_name + ";scenario=" + std::to_string(_request.scenario_no) +
                    ";start_id=" + std::to_string(start.start_id) + ";start_zone_id=" + std::to_string(start.zone_id) +
                    ";start_layer=" + start.layer_name + ";start_table=" + start.table_path +
                    ";start_row=" + std::to_string(start.jmap_entry_index) +
                    ";position=" + std::to_string(start.world_position[0U]) + "," +
                    std::to_string(start.world_position[1U]) + "," + std::to_string(start.world_position[2U]) +
                    ";model=Mario");
#endif
        } else {
            _runtime.player_system().reset_stage_state();
#ifndef NDEBUG
            _runtime.emit_semantic_trace_event(
                "player", "stage_player_unavailable",
                "stage=" + _request.stage_name + ";scenario=" + std::to_string(_request.scenario_no) +
                    ";start_id=" + std::to_string(_request.start_id) +
                    ";start_zone_id=" + std::to_string(_request.start_zone_id) + ";reason=start_info_not_found");
#endif
        }
    }

    void StageHostScene::trace_placement_object(const StagePlacementObject &placement) const {
#ifndef NDEBUG
        const auto rail = placement_rail_summary(placement);
        _runtime.emit_semantic_trace_event("placement", "stage_object",
                                           "stage=" + placement.stage_name + ";zone=" + placement.zone_name +
                                               ";zone_id=" + std::to_string(placement.zone_id) +
                                               ";scenario=" + std::to_string(_request.scenario_no) +
                                               ";layer=" + placement.layer_name + ";table=" + placement.table_path +
                                               ";object=" + placement.object_name +
                                               ";factory=" + std::string(placement_status_name(placement)) +
                                               ";support_reason=" + placement.support_reason +
                                               ";common_path_id=" + std::to_string(placement.common_path_id) +
                                               ";rail_info_attached=" + (rail.attached ? "true" : "false") +
                                               ";rail_path_row=" + std::to_string(rail.path_info_index) +
                                               ";rail_point_count=" + std::to_string(rail.point_count) +
                                               ";rail_first_point=" +
                                               (rail.has_first_point ? std::to_string(rail.first_point[0]) + "," +
                                                                           std::to_string(rail.first_point[1]) + "," +
                                                                           std::to_string(rail.first_point[2]) :
                                                                       "none") +
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

    void StageHostScene::init_stage_start_camera() {
        _runtime.camera_system().clear_game_camera_pose();
        _stage_start_camera.reset();

        if (!_stage_start_info.has_value()) {
            return;
        }

        auto resolved = smgpc::camera::resolve_stage_start_camera(_runtime.dvd(), *_stage_start_info);
        if (!resolved.camera.has_value()) {
            if (_stage_player != nullptr) {
                _stage_player->use_fallback_follow_camera();
            }
#ifndef NDEBUG
            _runtime.emit_semantic_trace_event(
                "camera", "stage_start_camera_unavailable",
                "stage=" + _request.stage_name + ";scenario=" + std::to_string(_request.scenario_no) +
                    ";start_id=" + std::to_string(_request.start_id) + ";start_zone_id=" + std::to_string(_request.start_zone_id) +
                    ";status=" + std::string(smgpc::camera::stage_start_camera_status_name(resolved.status)) +
                    ";detail=" + resolved.detail +
                    ";fallback=" + (_stage_player != nullptr ? "start_orientation_follow" : "none"));
#endif
            return;
        }

        _stage_start_camera = std::move(*resolved.camera);
        if (_stage_player != nullptr) {
            _stage_player->use_follow_camera_pose(_stage_start_camera->calculation.pose);
        } else {
            _runtime.camera_system().set_game_camera_pose(_stage_start_camera->calculation.pose);
        }
#ifndef NDEBUG
        const auto &camera = *_stage_start_camera;
        const auto &pose = camera.calculation.pose;
        _runtime.emit_semantic_trace_event(
            "camera", "stage_start_camera_selected",
            "stage=" + _request.stage_name + ";scenario=" + std::to_string(_request.scenario_no) +
                ";start_id=" + std::to_string(camera.start_info.start_id) +
                ";start_zone_id=" + std::to_string(camera.start_info.zone_id) +
                ";start_layer=" + camera.start_info.layer_name + ";start_table=" + camera.start_info.table_path +
                ";start_row=" + std::to_string(camera.start_info.jmap_entry_index) +
                ";camera_id=" + std::to_string(camera.start_info.camera_id) + ";camera_key=" + camera.camera_key +
                ";camera_type=" + camera.camera_param.camera_type + ";camera_archive=" + camera.start_info.archive_path +
                ";eye=" + std::to_string(pose.eye.x) + "," + std::to_string(pose.eye.y) + "," + std::to_string(pose.eye.z) +
                ";watch=" + std::to_string(pose.watch.x) + "," + std::to_string(pose.watch.y) + "," + std::to_string(pose.watch.z) +
                ";up=" + std::to_string(pose.up.x) + "," + std::to_string(pose.up.y) + "," + std::to_string(pose.up.z) +
                ";fovy=" + std::to_string(pose.fovy_degrees));
        _runtime.emit_semantic_trace_event(
            "camera", "stage_player_follow_camera_initialized",
            "stage=" + _request.stage_name + ";start_id=" + std::to_string(camera.start_info.start_id) +
                ";start_zone_id=" + std::to_string(camera.start_info.zone_id) +
                ";gravity_source=active_stage_gravity_or_start_orientation");
#endif
    }

    void StageHostScene::appear_roots() {
        if (!_request.appear_after_init) {
            return;
        }

        auto &lifecycle = _runtime.name_obj_lifecycle();
        for (auto index = std::size_t{}; index < _roots.size(); ++index) {
            if (_root_host_appear[index]) {
                lifecycle.appear(*_roots[index]);
            }
        }
    }

    void StageHostScene::destroy_roots() {
        auto &lifecycle = _runtime.name_obj_lifecycle();
        for (auto &root : _roots) {
            lifecycle.destroy(*root);
        }
        _roots.clear();
        _root_host_appear.clear();
    }

    void StageHostScene::start() {
    }

    void StageHostScene::update() {
        _runtime.scene_execution().execute_movement();
        if (_stage_player != nullptr) {
            _stage_player->synchronize_after_movement();
        }
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

    std::string_view StageHostScene::resolve_placement_actor_name(const StagePlacementObject &placement) const {
        const auto *localized_name = _object_name_table->lookup(placement.object_name);
#ifndef NDEBUG
        if (localized_name == nullptr) {
            _runtime.emit_semantic_trace_event(
                "placement", "object_name_table_fallback",
                "stage=" + placement.stage_name + ";object=" + placement.object_name +
                    ";table=" + placement.table_path + ";row=" + std::to_string(placement.jmap_entry_index));
        }
#endif
        return localized_name != nullptr ? std::string_view(*localized_name) :
                                           std::string_view(placement.object_name);
    }

}  // namespace smgpc::scene
