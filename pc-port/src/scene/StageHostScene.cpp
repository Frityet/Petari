#include "scene/StageHostScene.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/LightFunction.hpp"
#include "Game/Map/SleepControllerHolder.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "camera/StageStartCamera.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "compat/StageScenarioMetadataResolver.hpp"
#include "compat/StageSessionState.hpp"
#include "compat/StageZoneMatrixRegistry.hpp"
#include "runtime/RuntimeContext.hpp"
#include "scene/AreaObjRuntime.hpp"
#include "scene/NameObjLifecycleService.hpp"
#include "scene/SceneExecutionService.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/StagePlacementResolver.hpp"
#include "scene/StageEventCameraBinding.hpp"
#include "scene/StageLightSceneBinding.hpp"
#include "scene/nameobj/NameObjFactory.hpp"
#include "scene/nameobj/ObjectNameTable.hpp"
#include "scene/nameobj/PlanetMapCatalog.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace smgpc::scene {
    namespace {

        [[nodiscard]] bool placement_has_complete_runtime(const StagePlacementObject &placement) {
            return classify_authored_placement(placement).kind ==
                   AuthoredPlacementSupportKind::Ready;
        }

        [[nodiscard]] std::string placement_runtime_support_reason(
            const StagePlacementObject &placement) {
            return classify_authored_placement(placement).reason;
        }

#ifndef NDEBUG
        [[nodiscard]] std::filesystem::path debug_stage_placement_report_path() {
            const auto *path = std::getenv("SMGPC_STAGE_PLACEMENT_REPORT_PATH");
            if (path == nullptr || path[0] == '\0') {
                return {};
            }

            return std::filesystem::path(path);
        }

        [[nodiscard]] bool placement_runtime_is_complete(const StagePlacementObject &placement) {
            return placement_has_complete_runtime(placement);
        }

        [[nodiscard]] std::string_view placement_preflight_status_name(const StagePlacementObject &placement) {
            if (placement_has_complete_runtime(placement)) {
                return "complete";
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

        void write_stage_placement_report(std::string_view stage_name, s32 scenario_no,
                                          std::span<const StagePlacementObject> placements,
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

            const auto complete_count = std::ranges::count_if(placements, placement_runtime_is_complete);
            const auto ignored_count = std::ranges::count_if(placements, [](const auto &placement) { return placement.intentionally_ignored; });
            out << "# Stage Placement Report\n";
            out << "phase: preflight\n";
            out << "stage: " << stage_name << "\n";
            out << "scenario: " << scenario_no << "\n";
            out << "total_objects: " << placements.size() << "\n";
            out << "complete_objects: " << complete_count << "\n";
            out << "blocked_objects: " << blocked_placements.size() << "\n";
            out << "intentionally_ignored_objects: " << ignored_count << "\n\n";
            out << "## Objects\n";
            for (const auto &placement : placements) {
                const auto rail = placement_rail_summary(placement);
                out << "- status: " << placement_preflight_status_name(placement) << "\n";
                out << "  object: "
                    << authored_placement_identifier(placement) << "\n";
                out << "  authored_name: " << placement.object_name << "\n";
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
                out << "  support_reason: " << placement_runtime_support_reason(placement) << "\n";
                out << "  archive:";
                if (!placement.object_archive_path.empty()) {
                    out << " " << placement.object_archive_path;
                }
                out << "\n";
            }
        }
#endif

        [[nodiscard]] std::string unsupported_placement_error(std::string_view stage_name,
                                                              const std::vector<const StagePlacementObject *> &blocked_placements) {
            auto out = std::ostringstream();
            out << "Unsupported placement objects for " << stage_name << ": " << blocked_placements.size() << " blocked";
            if (!blocked_placements.empty()) {
                out << "; first="
                    << authored_placement_identifier(
                           *blocked_placements.front())
                    << "; raw_name="
                    << blocked_placements.front()->object_name << " in "
                    << blocked_placements.front()->table_path;
            }
            return out.str();
        }

    }  // namespace

    bool should_apply_host_appear(const StagePlacementObject *placement, bool explicit_root) {
        return explicit_root || placement == nullptr;
    }

    void preflight_stage_placements_or_throw(
        std::string_view stage_name, s32 scenario_no,
        std::span<const StagePlacementObject> placements,
        const StagePlacementObject *explicit_placement) {
        auto blocked_placements = std::vector<const StagePlacementObject *>{};
        for (const auto &placement : placements) {
            const auto support = classify_authored_placement(placement);
            if (support.kind != AuthoredPlacementSupportKind::Blocked) {
                continue;
            }
            blocked_placements.push_back(&placement);
        }
        (void)explicit_placement;
#ifndef NDEBUG
        write_stage_placement_report(stage_name, scenario_no, placements, blocked_placements);
#else
        (void)scenario_no;
#endif
        if (!blocked_placements.empty()) {
            throw std::runtime_error(unsupported_placement_error(stage_name, blocked_placements));
        }
    }

    StageHostScene::StageHostScene(smgpc::runtime::RuntimeContext &runtime, StageHostRequest request)
        : Scene(!request.stage_name.empty() ? request.stage_name.c_str() : "StageHostScene"), _runtime(runtime),
          _registration_scope_id(runtime.begin_scene_registration_scope()), _request(std::move(request)) {
    }

    StageHostScene::~StageHostScene() {
        _runtime.camera_system().clear_stage_start_camera(
            _stage_start_camera_owner_generation);
        _stage_start_camera_owner_generation = 0U;
        // Scheduler registrations retain raw object pointers, so remove the scene
        // scope while its roots and child objects are still alive.
        (void)_runtime.end_scene_registration_scope(_registration_scope_id);
        _runtime.player_system().clear_stage_state();
        destroy_roots();
        _runtime.scheduler().retire_draw_buffers();
        // Placement teardown releases cast memberships while the one
        // pre-placement DemoDirector counterpart is still available.
        _demo_scene_runtime.reset();
        // Exact actor destruction releases every owned KCL registration while
        // the stage collision service is still the active scene owner.
        _collision.deactivate();
        _scene_obj_holder_binding.reset();
        _planet_map_catalog.reset();
        _stage_light_binding.reset();
        _event_camera_binding.reset();
        _zone_matrix_binding.reset();
        _authored_data.reset();
        if (_stage_audio_started) {
            smgpc::compat::end_stage_audio(_runtime.audio());
            _stage_audio_started = false;
        }
        _stage_session_binding.reset();
        _stage_session.reset();
    }

    void StageHostScene::init() {
        if (_initialized) {
            return;
        }
        if (_stage_session_binding != nullptr || _scene_obj_holder_binding != nullptr) {
            throw std::logic_error(
                "A partially initialized stage host must be destroyed before another initialization attempt.");
        }

        if (_stage_session_binding == nullptr) {
            const auto scenario_metadata = smgpc::compat::resolve_stage_scenario_metadata(
                _runtime.dvd(), _request.stage_name, _request.scenario_no);
            auto stage_session = std::make_unique<smgpc::compat::StageSessionState>(
                _request.scene_name, _request.stage_name, _request.scenario_no,
                JMapIdInfo(_request.start_id, _request.start_zone_id), scenario_metadata);
            auto stage_session_binding =
                std::make_unique<smgpc::compat::StageSessionBinding>(*stage_session);
            _stage_session = std::move(stage_session);
            _stage_session_binding = std::move(stage_session_binding);
        }

        _runtime.begin_scene_draw_buffer_registration();
        initSceneObjHolder();
        _scene_obj_holder_binding = std::make_unique<SceneObjHolderBinding>(*mSceneObjHolder);
        constexpr auto required_scene_objects = std::array{
            SceneObj_MessageSensorHolder,
            SceneObj_ClippingDirector,
            SceneObj_LightDirector,
            SceneObj_PlanetGravityManager,
            SceneObj_MarioHolder,
            SceneObj_StageSwitchContainer,
            SceneObj_SwitchWatcherHolder,
            SceneObj_SleepControllerHolder,
            SceneObj_AreaObjContainer,
            SceneObj_PlacementStateChecker,
            SceneObj_BaseMatrixFollowTargetHolder,
            SceneObj_GroupCheckManager,
            SceneObj_TalkDirector,
        };
        for (const auto id : required_scene_objects) {
            if (MR::createSceneObj(id) == nullptr) {
                throw std::runtime_error("required retail stage SceneObj is unavailable: " + std::to_string(id));
            }
        }
        LightFunction::initLightRegisterAll();
        // Stage metadata is useful independently of MarioActor. Clear any
        // prior scene's actor pointer here; the real player implementation
        // will attach itself through the player service when it is linked.
        _runtime.player_system().clear_stage_state();

        if (!_request.object_name.empty()) {
            init_explicit_root();
        } else {
            init_placement_roots();
        }

        // Exact actors register CollisionParts during init. Retail
        // initAfterPlacement callbacks may immediately query those parts, so
        // publish the first complete registry before dispatching callbacks.
        _collision.build();
        _scene_obj_holder_binding->init_after_placement();
        init_roots_after_placement();
        // Exact Game actors register CollisionParts while they initialize.
        // Rebuild the generalized query structure only after every actor and
        // SceneObj has completed the retail post-placement pass.
        _collision.build();
#ifndef NDEBUG
        const auto &post_placement_collision = _collision.stats();
        _runtime.emit_semantic_trace_event(
            "collision", "stage_collision_registry_finalized",
            "stage=" + _request.stage_name + ";scenario=" + std::to_string(_request.scenario_no) +
                ";meshes=" + std::to_string(post_placement_collision.mesh_count) +
                ";triangles=" + std::to_string(post_placement_collision.triangle_count) +
                ";rejected_triangles=" +
                std::to_string(post_placement_collision.rejected_triangle_count));
#endif
#ifndef NDEBUG
        if (_runtime.player_system().attached_actor() != nullptr) {
            _runtime.emit_semantic_trace_event(
                "player", "stage_player_attached",
                "stage=" + _request.stage_name + ";scenario=" + std::to_string(_request.scenario_no) +
                    ";source=real_actor_attachment");
        } else if (_authored_data != nullptr &&
                   _authored_data->start_info().has_value()) {
            const auto &start = *_authored_data->start_info();
            _runtime.emit_semantic_trace_event(
                "player", "stage_player_unavailable",
                "stage=" + _request.stage_name + ";scenario=" + std::to_string(_request.scenario_no) +
                    ";start_id=" + std::to_string(start.start_id) + ";start_zone_id=" + std::to_string(start.zone_id) +
                    ";start_layer=" + start.layer_name + ";start_table=" + start.table_path +
                    ";start_row=" + std::to_string(start.jmap_entry_index) +
                    ";reason=real_mario_actor_not_linked");
        } else {
            _runtime.emit_semantic_trace_event(
                "player", "stage_player_unavailable",
                "stage=" + _request.stage_name + ";scenario=" + std::to_string(_request.scenario_no) +
                    ";start_id=" + std::to_string(_request.start_id) +
                    ";start_zone_id=" + std::to_string(_request.start_zone_id) + ";reason=start_info_not_found");
        }
#endif
        init_stage_start_camera();
        SleepControlFunc::initSyncSleepController();
        _runtime.scheduler().allocate_draw_buffers();
        appear_roots();
        _initialized = true;
    }

    void StageHostScene::construct_root_object(std::string_view object_name, const char *actor_name,
                                               const NameObjPlacementContext *placement,
                                               bool apply_host_appear) {
        if (!smgpc::scene::nameobj::can_create_name_obj(object_name)) {
            throw std::runtime_error("Unsupported stage host request object: " + std::string(object_name) + " for stage " + _request.stage_name);
        }
        if (find_complete_area_obj_placement_descriptor(object_name) != nullptr &&
            (placement == nullptr || placement->source != NameObjPlacementSource::StagePlacement)) {
            throw std::runtime_error(
                "An exact AreaObj requires its retail placement row: " + std::string(object_name));
        }

        auto &lifecycle = _runtime.name_obj_lifecycle();
        lifecycle.preload_archives(object_name, placement);
        _roots.reserve(_roots.size() + 1U);
        _root_registration_graphs.reserve(
            _root_registration_graphs.size() + 1U);
        _root_host_appear.reserve(_root_host_appear.size() + 1U);
        auto registration_graph = std::make_unique<NameObjChildOwner>();
        const auto capture =
            smgpc::compat::NameObjRuntimeRegistrationCapture{};
        auto root = std::unique_ptr<NameObj>{};
        try {
            root = lifecycle.construct_and_init(
                object_name, actor_name, placement);
            if (root == nullptr) {
                throw std::runtime_error(
                    "Stage root lifecycle returned a null actor.");
            }
            registration_graph->adopt_root_registration_suffix(
                capture.marker(), *root, this);
        } catch (...) {
            const auto construction_failure = std::current_exception();
            registration_graph.reset();
            NameObjChildOwner::rollback_registration_suffix(
                capture.marker());
            std::rethrow_exception(construction_failure);
        }
#ifndef NDEBUG
        _runtime.emit_semantic_trace_event("sequence", "stage_host_initialized",
                                           "host=" + std::string(object_name) + ";stage=" + _request.stage_name);
#endif
        _roots.push_back(std::move(root));
        _root_registration_graphs.push_back(
            std::move(registration_graph));
        _root_host_appear.push_back(apply_host_appear);
    }

    void StageHostScene::init_explicit_root() {
        init_stage_environment();

        const auto placements = _authored_data->placements();
        const auto explicit_placement = std::ranges::find_if(placements, [this](const auto &placement) {
            return authored_placement_identifier(placement) ==
                       _request.object_name &&
                   placement_has_complete_runtime(placement);
        });
        const auto *placement = explicit_placement != placements.end() ?
                                    &*explicit_placement :
                                    nullptr;
        prepare_authored_placements(placement);
        preload_authored_placements();
        init_stage_audio();
        if (placement == nullptr) {
            const auto *actor_name = !_request.actor_name.empty() ?
                                         _request.actor_name.c_str() :
                                         nullptr;
            construct_root_object(
                _request.object_name, actor_name, nullptr, true);
        }
        construct_authored_placements();
    }

    void StageHostScene::init_placement_roots() {
        init_stage_environment();
        prepare_authored_placements();
        preflight_stage_start_or_throw();
        // PlacementInfoOrdered ranks and requests every holder before retail
        // inserts initPlacementMario at the actor-construction boundary.
        preload_authored_placements();
        init_stage_audio();
        construct_stage_start_root();
        construct_authored_placements();
    }

    void StageHostScene::preflight_stage_start_or_throw() const {
        if (_authored_data == nullptr ||
            !_authored_data->start_info().has_value()) {
            throw std::runtime_error(
                "No active StartInfo matches stage " + _request.stage_name + ";start_id=" +
                std::to_string(_request.start_id) + ";start_zone_id=" +
                std::to_string(_request.start_zone_id));
        }

        const auto &start = *_authored_data->start_info();
        if (start.object_name.empty()) {
            throw std::runtime_error(
                "StartInfo is missing its retail object name: " + start.table_path +
                ";row=" + std::to_string(start.jmap_entry_index));
        }
        if (!smgpc::scene::nameobj::can_create_name_obj(start.object_name)) {
            const auto support = smgpc::scene::nameobj::describe_name_obj_creator_support(start.object_name);
            throw std::runtime_error(
                "Unsupported stage StartInfo object: " + start.object_name + " for stage " +
                _request.stage_name + " (" + support.reason + ")");
        }
    }

    void StageHostScene::construct_stage_start_root() {
        const auto &start = *_authored_data->start_info();
        const auto context = _authored_data->start_context();
        // StageDataHolder::initPlacementMario passes this retail actor name
        // directly; StartInfo does not use ObjNameTable display-name lookup.
        construct_root_object(start.object_name, "マリオアクター", &context, false);
    }

    void StageHostScene::prepare_authored_placements(
        const StagePlacementObject *explicit_placement) {
        if (_authored_data == nullptr || _authored_placements != nullptr) {
            throw std::logic_error(
                "Authored stage placement preparation requires one retained data owner.");
        }

        _explicit_placement_source = explicit_placement;
        auto options = AuthoredPlacementInstantiationOptions{
            .mode = AuthoredPlacementMode::Strict,
            .actor_name_resolver = [this, explicit_placement](
                                       const StagePlacementObject &placement)
                -> std::optional<std::string> {
                if (explicit_placement == &placement &&
                    !_request.actor_name.empty()) {
                    return _request.actor_name;
                }
                const auto *localized_name = resolve_actor_name(
                    authored_placement_identifier(placement), &placement);
                if (localized_name == nullptr) {
                    return std::string(
                        authored_placement_identifier(placement));
                }
                return std::string(localized_name);
            },
        };
        _authored_placements =
            std::make_unique<AuthoredPlacementInstantiator>(
                *_authored_data, _runtime.name_obj_lifecycle(),
                std::move(options));

        preflight_stage_placements_or_throw(
            _request.stage_name, _request.scenario_no,
            _authored_data->placements(), explicit_placement);
        (void)_authored_placements->preflight();

        for (const auto &entry : _authored_placements->report().entries) {
            if (entry.placement == nullptr) {
                continue;
            }
            const auto &placement = *entry.placement;
            trace_placement_object(placement);
        }
    }

    void StageHostScene::preload_authored_placements() {
        if (_authored_placements == nullptr) {
            throw std::logic_error(
                "Authored stage placements were not prepared before preload.");
        }
        (void)_authored_placements->preload();
    }

    void StageHostScene::construct_authored_placements() {
        if (_authored_placements == nullptr) {
            throw std::logic_error(
                "Authored stage placements were not prepared before construction.");
        }

        const auto &report = _authored_placements->instantiate();
        if (_explicit_placement_source != nullptr) {
            const auto found = std::ranges::find_if(
                _authored_placements->instances(), [this](const auto &instance) {
                    return instance.placement == _explicit_placement_source;
                });
            if (found == _authored_placements->instances().end()) {
                throw std::logic_error(
                    "The explicit placement root was accepted but not constructed.");
            }
            _explicit_placement_root = found->actor;
        }

#ifndef NDEBUG
        _runtime.emit_semantic_trace_event("placement", "stage_placement_summary",
                                           "stage=" + _request.stage_name + ";scenario=" + std::to_string(_request.scenario_no) +
                                               ";objects=" + std::to_string(report.entries.size()) +
                                               ";complete=" + std::to_string(report.ready_count) +
                                               ";ignored=" + std::to_string(report.ignored_count) +
                                               ";blocked=" + std::to_string(report.blocked_count) +
                                               ";created=" + std::to_string(report.created_count) +
                                               ";mode=strict");
#endif
    }

    void StageHostScene::init_stage_environment() {
        if (_object_name_table == nullptr) {
            _object_name_table = std::make_unique<smgpc::scene::nameobj::ObjectNameTable>(_runtime.dvd());
        }
        _planet_map_catalog =
            std::make_unique<smgpc::scene::nameobj::PlanetMapCatalog>(_runtime.dvd());
        // Planet rows acquire factory support from the active catalog. Publish
        // it before the single retained authored-data resolution pass.
        _authored_data = std::make_unique<StageAuthoredData>(
            StageAuthoredData::resolve(
                _runtime.dvd(), _request.stage_name, _request.scenario_no,
                _request.start_id, _request.start_zone_id));
        _zone_matrix_binding = std::make_unique<smgpc::compat::StageZoneMatrixBinding>(
            _authored_data->holders(), _authored_data->tables());
        _event_camera_binding = std::make_unique<StageEventCameraBinding>(
            _runtime.camera_system(), _runtime.dvd(), _authored_data->tables());
        _stage_light_binding = std::make_unique<StageLightSceneBinding>(
            _runtime.dvd(), _request.stage_name, _authored_data->tables());
        // The original DemoDirector/executors exist before placement actors
        // initialize and attempt to join their zone-scoped groups.
        _demo_scene_runtime = std::make_unique<smgpc::compat::DemoSceneRuntime>(
            _runtime.dvd(), _authored_data->placements(),
            _authored_data->general_positions());
        // Collision remains absent until source Game code issues an exact
        // CollisionParts registration. Placement/archive discovery must not
        // synthesize collision for actors that did not request it.
        _collision.clear();
        _collision.build();
        _collision.activate();
        const auto &collision_stats = _collision.stats();
#ifndef NDEBUG
        _runtime.emit_semantic_trace_event(
            "collision", "stage_collision_registry_ready",
            "stage=" + _request.stage_name + ";scenario=" + std::to_string(_request.scenario_no) +
                ";placement_rows=" +
                std::to_string(_authored_data->placements().size()) +
                ";registration=explicit_collision_parts;meshes=" + std::to_string(collision_stats.mesh_count) +
                ";triangles=" + std::to_string(collision_stats.triangle_count) +
                ";rejected_triangles=" + std::to_string(collision_stats.rejected_triangle_count));
#endif
    }

    void StageHostScene::init_stage_audio() {
        if (_stage_audio_started) {
            return;
        }
        try {
            smgpc::compat::begin_stage_audio(
                _runtime.audio(), _request.scene_name, _request.stage_name,
                _request.scenario_no);
        } catch (...) {
            smgpc::compat::end_stage_audio(_runtime.audio());
            throw;
        }
        _stage_audio_started = true;
    }

    void StageHostScene::trace_placement_object(const StagePlacementObject &placement) const {
#ifndef NDEBUG
        const auto rail = placement_rail_summary(placement);
        _runtime.emit_semantic_trace_event("placement", "stage_object",
                                           "stage=" + placement.stage_name + ";zone=" + placement.zone_name +
                                               ";zone_id=" + std::to_string(placement.zone_id) +
                                               ";scenario=" + std::to_string(_request.scenario_no) +
                                               ";layer=" + placement.layer_name + ";table=" + placement.table_path +
                                               ";object=" + std::string(authored_placement_identifier(placement)) +
                                               ";raw_name=" + placement.object_name +
                                               ";runtime_support=" + std::string(placement_preflight_status_name(placement)) +
                                               ";support_reason=" + std::string(placement_runtime_support_reason(placement)) +
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
        for (auto &registration_graph : _root_registration_graphs) {
            registration_graph->init_registration_suffix_after_placement();
        }
        if (_authored_placements != nullptr) {
            (void)_authored_placements->init_after_placement();
        }
    }

    void StageHostScene::init_stage_start_camera() {
        _runtime.camera_system().clear_stage_start_camera(
            _stage_start_camera_owner_generation);
        _stage_start_camera_owner_generation = 0U;

        if (_authored_data == nullptr ||
            !_authored_data->start_info().has_value()) {
            return;
        }

        auto resolved = smgpc::camera::resolve_stage_start_camera(
            _runtime.dvd(), *_authored_data->start_info());
        if (!resolved.camera.has_value()) {
#ifndef NDEBUG
            _runtime.emit_semantic_trace_event(
                "camera", "stage_start_camera_unavailable",
                "stage=" + _request.stage_name + ";scenario=" + std::to_string(_request.scenario_no) +
                    ";start_id=" + std::to_string(_request.start_id) + ";start_zone_id=" + std::to_string(_request.start_zone_id) +
                    ";status=" + std::string(smgpc::camera::stage_start_camera_status_name(resolved.status)) +
                    ";detail=" + resolved.detail + ";camera=absent");
#endif
            return;
        }

        _stage_start_camera_owner_generation =
            _runtime.camera_system().set_stage_start_camera(
                std::move(*resolved.camera));
#ifndef NDEBUG
        const auto *retained_camera =
            _runtime.camera_system().stage_start_camera();
        if (retained_camera == nullptr) {
            throw std::logic_error(
                "Stage-start camera service lost its retained resolved camera.");
        }
        const auto &camera = *retained_camera;
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
        if (_explicit_placement_root != nullptr) {
            lifecycle.appear(*_explicit_placement_root);
        }
    }

    void StageHostScene::destroy_roots() {
        auto &lifecycle = _runtime.name_obj_lifecycle();
        if (_authored_placements != nullptr) {
            // The instantiator destructor performs the same reverse teardown
            // while containing any actor-specific destruction exception; this
            // scene destructor must remain noexcept.
            _authored_placements.reset();
        }
        _explicit_placement_root = nullptr;
        _explicit_placement_source = nullptr;
        for (auto index = _roots.size(); index > 0U; --index) {
            auto &root = _roots[index - 1U];
            _root_registration_graphs[index - 1U]->clear();
            if (root == nullptr) {
                continue;
            }
            try {
                lifecycle.destroy(*root);
            } catch (...) {
                // Scene destruction cannot propagate, but actor deletion must
                // still happen at this exact reverse-order retirement point.
            }
            root.reset();
        }
        _roots.clear();
        _root_registration_graphs.clear();
        _root_host_appear.clear();
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
        if (!_roots.empty()) {
            return _roots.front().get();
        }
        if (_explicit_placement_root != nullptr) {
            return _explicit_placement_root;
        }
        if (_authored_placements != nullptr &&
            !_authored_placements->instances().empty()) {
            return _authored_placements->instances().front().actor;
        }
        return nullptr;
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

    const char *StageHostScene::resolve_actor_name(
        std::string_view object_name,
        const StagePlacementObject *placement) const {
        const auto *localized_name = _object_name_table->lookup(object_name);
#ifndef NDEBUG
        if (localized_name == nullptr) {
            _runtime.emit_semantic_trace_event(
                "placement", "object_name_table_absent",
                "stage=" + _request.stage_name + ";object=" + std::string(object_name) +
                    ";source=placement" +
                    (placement != nullptr ?
                         ";table=" + std::string(placement->table_path) +
                             ";row=" +
                             std::to_string(placement->jmap_entry_index) :
                         ""));
        }
#endif
        return localized_name != nullptr ? localized_name->c_str() : nullptr;
    }

}  // namespace smgpc::scene
