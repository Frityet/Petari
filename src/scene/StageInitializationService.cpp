#include "scene/StageInitializationService.hpp"
#include <aurora/exception.hpp>

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/LightFunction.hpp"
#include "Game/Map/SleepControllerHolder.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/NameObj/NameObjHolder.hpp"
#include "scene/SceneNameObjRegistry.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneNameObjListExecutor.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "compat/StageResourceBinding.hpp"
#include "compat/StageScenarioMetadataResolver.hpp"
#include "compat/StageSessionState.hpp"
#include "compat/StageZoneMatrixRegistry.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/ArchiveMountService.hpp"
#include "resource/RarcArchive.hpp"
#include "scene/AreaObjRuntime.hpp"
#include "scene/NameObjLifecycleService.hpp"
#include "scene/SceneExecutionBinding.hpp"
#include "scene/SceneLifetimeBinding.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/StageLightSceneBinding.hpp"
#include "scene/StagePlacementResolver.hpp"
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
        thread_local StageInitializationService *sInitializer = nullptr;

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
            aurora::throw_host_exception<std::runtime_error>(unsupported_placement_error(stage_name, blocked_placements));
        }
    }

    StageInitializationService::StageInitializationService(
        smgpc::runtime::RuntimeContext &runtime, Scene &scene, StageHostRequest request,
        std::shared_ptr<smgpc::compat::JkrAllocationDomain> domain)
        : _runtime(runtime), _scene(scene), _scene_domain(std::move(domain)),
          _request(std::move(request)) {
        const auto host_allocations = smgpc::compat::JkrHostAllocationScope{};
        if (!_scene_domain) {
            aurora::throw_host_exception<std::invalid_argument>(
                "Stage initialization requires the actual Scene allocation domain");
        }
        _lifetime_binding = std::make_unique<SceneLifetimeBinding>(
            scene, [](void *context) noexcept {
                static_cast<StageInitializationService *>(context)->retire();
            },
            this);
        _registration_scope_id = runtime.begin_scene_registration_scope();
        _previous = sInitializer;
        sInitializer = this;
    }

    StageInitializationService::~StageInitializationService() {
        retire();
    }

    void StageInitializationService::retire() noexcept {
        if (_retired)
            return;
        _retired = true;
        const auto host_allocations = smgpc::compat::JkrHostAllocationScope{};
        // Scheduler registrations retain raw object pointers, so remove the scene
        // scope while its roots and child objects are still alive.
        (void)_runtime.end_scene_registration_scope(_registration_scope_id);
        _runtime.player_system().clear_stage_state();
        destroy_roots();
        // Placement teardown releases cast memberships while the one
        // pre-placement DemoDirector counterpart is still available.
        _demo_scene_runtime.reset();
        // Exact actor destruction releases every owned KCL registration while
        // the stage collision service is still the active scene owner.
        _collision.deactivate();
        if (_execution_binding)
            _execution_binding->prepare_retirement();
        _scene_obj_holder_binding.reset();
        _execution_binding.reset();
        _planet_map_catalog.reset();
        _stage_light_binding.reset();
        _zone_matrix_binding.reset();
        _stage_resource_binding.reset();
        _authored_data.reset();
        if (_stage_audio_started) {
            smgpc::compat::end_stage_audio(_runtime.audio());
            _stage_audio_started = false;
        }
        _stage_session_binding.reset();
        _stage_session.reset();
        _runtime.archive_mounts().remove_for_heap(&_scene_domain->heap());
        _stage_archive_names.clear();
        _lifetime_binding.reset();
        if (sInitializer != this) std::terminate();
        sInitializer = _previous;
    }

    StageInitializationService *current_stage_initialization_service() noexcept { return sInitializer; }

    StageInitializationService &require_stage_initialization_service() {
        if (!sInitializer)
            aurora::throw_host_exception<std::logic_error>("Original stage initialization requires its actual Scene service");
        return *sInitializer;
    }

    Scene &StageInitializationService::scene() const noexcept { return _scene; }

    void StageInitializationService::pre_scene_init() {
        initialize_session();
        bind_scene_objects();
    }

    void StageInitializationService::initialize_effect_system(unsigned particles, unsigned emitters) {
        require_live();
        if (!_scene_obj_holder_binding)
            aurora::throw_host_exception<std::logic_error>("Effect initialization requires the bound SceneObjHolder");
        _scene_obj_holder_binding->initialize_effect_system(particles, emitters);
    }

    void StageInitializationService::allocate_draw_buffer_actor_list() {
        require_live();
        if (!_execution_binding)
            aurora::throw_host_exception<std::logic_error>("Scene list allocation requires the original scene executor");
        _execution_binding->complete_initialization();
    }

    void StageInitializationService::complete_camera_parameters() {
        require_live();
        if (!_scene_obj_holder_binding)
            aurora::throw_host_exception<std::logic_error>("Camera parameter completion requires its actual Scene binding");
        _scene_obj_holder_binding->complete_camera_parameters();
    }

    void StageInitializationService::require_live() const {
        if (_retired) {
            aurora::throw_host_exception<std::logic_error>("A retired stage has no active Scene owner");
        }
    }

    void StageInitializationService::initialize_host_scene() {
        const auto host_allocations = smgpc::compat::JkrHostAllocationScope{};
        if (_retired) {
            aurora::throw_host_exception<std::logic_error>("A retired stage cannot be initialized again");
        }
        if (_initialized) {
            return;
        }
        if (_stage_session_binding != nullptr || _scene_obj_holder_binding != nullptr) {
            aurora::throw_host_exception<std::logic_error>(
                "A partially initialized stage must be destroyed before another initialization attempt.");
        }
        pre_scene_init();
        initialize_host_scene_objects();
        load_stage_files();
        initialize_scenario_resources();
        if (_request.object_name.empty()) {
            _scene_obj_holder_binding->initialize_camera_system();
        }
        prepare_actor_files();
        init_stage_audio();
        place_actors();
        complete_camera_parameters();
        finish_actor_placement();
        complete_initialization();
        _stage_session->set_execution_phase(
            smgpc::compat::StageSessionState::ExecutionPhase::Gameplay);
    }

    void StageInitializationService::initialize_session() {
        require_live();
        const auto host_allocations = smgpc::compat::JkrHostAllocationScope{};
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
    }

    void StageInitializationService::bind_scene_objects() {
        require_live();
        const auto host_allocations = smgpc::compat::JkrHostAllocationScope{};
        if (_stage_session_binding == nullptr || _scene_obj_holder_binding != nullptr ||
            _scene.mSceneObjHolder != nullptr || _scene.mListExecutor != nullptr) {
            aurora::throw_host_exception<std::logic_error>(
                "Stage initialization requires one unbound SceneObjHolder owner.");
        }
        {
            const smgpc::compat::JkrAllocationScope game(_scene_domain);
            _scene.initSceneObjHolder();
            _scene.initNameObjListExecutor();
        }
        _scene_obj_holder_binding = std::make_unique<SceneObjHolderBinding>(
            *_scene.mSceneObjHolder, nullptr, nullptr, _scene_domain);
        _execution_binding = std::make_unique<SceneExecutionBinding>(
            _runtime.scheduler(), *_scene.mListExecutor, _scene_domain);
    }

    void StageInitializationService::initialize_host_scene_objects() {
        const auto host_allocations = smgpc::compat::JkrHostAllocationScope{};
        initialize_effect_system(3072, 256);
        constexpr auto required_scene_objects = std::array{
            SceneObj_NameObjGroup,
            SceneObj_SceneWipeHolder,
            SceneObj_ScenePlayingResult,
            SceneObj_MessageSensorHolder,
            SceneObj_ClippingDirector,
            SceneObj_LightDirector,
            SceneObj_CaptureScreenActor,
            SceneObj_FurDrawManager,
            SceneObj_PlanetGravityManager,
            SceneObj_MarioHolder,
            SceneObj_AudBgmConductor,
            SceneObj_EventSequencer,
            SceneObj_StageSwitchContainer,
            SceneObj_SwitchWatcherHolder,
            SceneObj_SleepControllerHolder,
            SceneObj_AreaObjContainer,
            SceneObj_PlacementStateChecker,
            SceneObj_BaseMatrixFollowTargetHolder,
            SceneObj_GroupCheckManager,
            SceneObj_TalkDirector,
            SceneObj_GameSceneLayoutHolder,
        };
        for (const auto id : required_scene_objects) {
            if (MR::createSceneObj(id) == nullptr) {
                aurora::throw_host_exception<std::runtime_error>("required retail stage SceneObj is unavailable: " + std::to_string(id));
            }
        }
        LightFunction::initLightRegisterAll();
        // Stage metadata is useful independently of MarioActor. Clear any
        // prior scene's actor pointer here; the real player implementation
        // will attach itself through the player service when it is linked.
        _runtime.player_system().clear_stage_state();
    }

    void StageInitializationService::finish_actor_placement() {
        require_live();
        const auto host_allocations = smgpc::compat::JkrHostAllocationScope{};
        if (_postpass_started || _scene_obj_holder_binding == nullptr || _authored_placements == nullptr ||
            _authored_placements->report().state != AuthoredPlacementRuntimeState::Instantiated) {
            aurora::throw_host_exception<std::logic_error>(
                "The stage post-placement pass requires completed actor construction.");
        }
        // Exact actors register CollisionParts during init. Retail
        // initAfterPlacement callbacks may immediately query those parts, so
        // publish the first complete registry before dispatching callbacks.
        _collision.build();
        auto *registry = current_scene_name_obj_registry();
        if (!registry)
            aurora::throw_host_exception<std::logic_error>("The stage postpass requires its actual NameObjHolder");
        const auto objects = registry->snapshot();
        _postpass_started = true;
        {
            const SceneInitializationScope phase(SceneInitializeState_AfterPlacement);
            const smgpc::compat::JkrAllocationScope game(_scene_domain);
            registry->holder().callMethodAllObj(&NameObj::initAfterPlacement);
        }
        for (auto &graph : _root_registration_graphs)
            graph->acknowledge_scene_postpass(objects);
        _authored_placements->acknowledge_scene_postpass(objects);
        _scene_obj_holder_binding->acknowledge_scene_postpass(objects);
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
    }

    void StageInitializationService::complete_initialization() {
        require_live();
        const auto host_allocations = smgpc::compat::JkrHostAllocationScope{};
        if (_initialized || _scene_obj_holder_binding == nullptr || _authored_placements == nullptr ||
            _authored_placements->report().state != AuthoredPlacementRuntimeState::InitializedAfterPlacement) {
            aurora::throw_host_exception<std::logic_error>(
                "Stage completion requires its one completed post-placement pass.");
        }
        SleepControlFunc::initSyncSleepController();
        allocate_draw_buffer_actor_list();
        appear_roots();
        finalize_scene_initialization();
    }

    void StageInitializationService::finalize_scene_initialization() {
        require_live();
        if (_initialized || !_scene_obj_holder_binding || !_execution_binding || !_execution_binding->initialized())
            aurora::throw_host_exception<std::logic_error>("Scene initialization End requires completed original list allocation");
        _scene_obj_holder_binding->complete_initialization();
        _initialized = true;
    }

    void StageInitializationService::construct_root_object(std::string_view object_name, const char *actor_name,
                                                           const NameObjPlacementContext *placement,
                                                           bool apply_host_appear) {
        if (!smgpc::scene::nameobj::can_create_name_obj(object_name)) {
            aurora::throw_host_exception<std::runtime_error>("Unsupported stage host request object: " + std::string(object_name) + " for stage " + _request.stage_name);
        }
        if (find_complete_area_obj_placement_descriptor(object_name) != nullptr &&
            (placement == nullptr || placement->source != NameObjPlacementSource::StagePlacement)) {
            aurora::throw_host_exception<std::runtime_error>(
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
                aurora::throw_host_exception<std::runtime_error>(
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

    void StageInitializationService::prepare_actor_plan() {
        require_live();
        const auto host_allocations = smgpc::compat::JkrHostAllocationScope{};
        if (_authored_data == nullptr) {
            aurora::throw_host_exception<std::logic_error>(
                "Actor file preparation requires completed stage loading.");
        }
        if (_request.object_name.empty()) {
            prepare_authored_placements();
            preflight_stage_start_or_throw();
        } else {
            const auto placements = _authored_data->placements();
            const auto explicit_placement = std::ranges::find_if(
                placements, [this](const auto &placement) {
                    return authored_placement_identifier(placement) ==
                               _request.object_name &&
                           placement_has_complete_runtime(placement);
                });
            prepare_authored_placements(explicit_placement != placements.end() ?
                                            &*explicit_placement :
                                            nullptr);
        }
    }

    void StageInitializationService::place_actors() {
        require_live();
        const auto host_allocations = smgpc::compat::JkrHostAllocationScope{};
        if (_authored_placements == nullptr ||
            _authored_placements->report().state != AuthoredPlacementRuntimeState::Preloaded) {
            aurora::throw_host_exception<std::logic_error>(
                "Actor placement requires one completed preload pass.");
        }
        if (_request.object_name.empty()) {
            construct_stage_start_root();
        } else if (_explicit_placement_source == nullptr) {
            const auto phase = SceneInitializationScope(SceneInitializeState_Placement);
            const auto *actor_name = !_request.actor_name.empty() ?
                                         _request.actor_name.c_str() :
                                         nullptr;
            construct_root_object(_request.object_name, actor_name, nullptr, true);
        }
        construct_authored_placements();
    }

    void StageInitializationService::preflight_stage_start_or_throw() const {
        if (_authored_data == nullptr ||
            !_authored_data->start_info().has_value()) {
            aurora::throw_host_exception<std::runtime_error>(
                "No active StartInfo matches stage " + _request.stage_name + ";start_id=" +
                std::to_string(_request.start_id) + ";start_zone_id=" +
                std::to_string(_request.start_zone_id));
        }

        const auto &start = *_authored_data->start_info();
        if (start.object_name.empty()) {
            aurora::throw_host_exception<std::runtime_error>(
                "StartInfo is missing its retail object name: " + start.table_path +
                ";row=" + std::to_string(start.jmap_entry_index));
        }
        if (!smgpc::scene::nameobj::can_create_name_obj(start.object_name)) {
            const auto support = smgpc::scene::nameobj::describe_name_obj_creator_support(start.object_name);
            aurora::throw_host_exception<std::runtime_error>(
                "Unsupported stage StartInfo object: " + start.object_name + " for stage " +
                _request.stage_name + " (" + support.reason + ")");
        }
    }

    void StageInitializationService::construct_stage_start_root() {
        const auto phase = SceneInitializationScope(SceneInitializeState_PlacementPlayer);
        const auto &start = *_authored_data->start_info();
        const auto context = _authored_data->start_context();
        // StageDataHolder::initPlacementMario passes this retail actor name
        // directly; StartInfo does not use ObjNameTable display-name lookup.
        construct_root_object(start.object_name, "マリオアクター", &context, false);
    }

    void StageInitializationService::prepare_authored_placements(
        const StagePlacementObject *explicit_placement) {
        if (_authored_data == nullptr || _authored_placements != nullptr) {
            aurora::throw_host_exception<std::logic_error>(
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

    void StageInitializationService::prepare_actor_files() {
        start_actor_file_load_common();
        start_actor_file_load_scenario();
    }

    void StageInitializationService::start_actor_file_load_common() {
        require_live();
        const compat::JkrHostAllocationScope host;
        prepare_actor_plan();
        _authored_placements->preload_common();
    }

    void StageInitializationService::start_actor_file_load_scenario() {
        require_live();
        const compat::JkrHostAllocationScope host;
        if (!_stage_resource_binding || !_authored_placements)
            aurora::throw_host_exception<std::logic_error>("Scenario actor loading requires selected scenario resources and the common archive pass");
        _authored_placements->preload_scenario();
    }

    void StageInitializationService::construct_authored_placements() {
        if (_authored_placements == nullptr) {
            aurora::throw_host_exception<std::logic_error>(
                "Authored stage placements were not prepared before construction.");
        }

        const auto &report = _authored_placements->instantiate();
        if (_explicit_placement_source != nullptr) {
            const auto found = std::ranges::find_if(
                _authored_placements->instances(), [this](const auto &instance) {
                    return instance.placement == _explicit_placement_source;
                });
            if (found == _authored_placements->instances().end()) {
                aurora::throw_host_exception<std::logic_error>(
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

    void StageInitializationService::load_stage_files() {
        start_stage_file_load();
        wait_done_stage_file_load();
    }

    void StageInitializationService::start_stage_file_load() {
        require_live();
        const compat::JkrHostAllocationScope host;
        if (!_stage_session_binding || _stage_file_load_started)
            aurora::throw_host_exception<std::logic_error>("Start stage file loading once within the actual stage session");
        _stage_file_load_started = true;
        _stage_archive_names = resolve_stage_archive_names(_runtime.dvd(), _request.stage_name);
        for (const auto &name : _stage_archive_names)
            (void)_runtime.archive_mounts().mount(name, &_scene_domain->heap());
        if (!MR::isStageDisablePauseMenu())
            (void)_runtime.archive_mounts().mount("/LayoutData/PauseMenu.arc", &_scene_domain->heap());
        _stage_files_mounted = true;
    }

    void StageInitializationService::wait_done_stage_file_load() {
        require_live();
        const compat::JkrHostAllocationScope host;
        if (!_stage_files_mounted || _authored_data || _planet_map_catalog)
            aurora::throw_host_exception<std::logic_error>("Wait for stage files once after starting their load");
        // File loading currently completes synchronously. The original wait
        // boundary still controls nested archive publication and authored rows.
        for (const auto &name : _stage_archive_names) {
            const auto archive = _runtime.archive_mounts().retain(name);
            if (!archive)
                aurora::throw_host_exception<std::logic_error>("A requested stage archive did not finish mounting");
            for (const auto &entry : archive->source().entries()) {
                if (entry.directory != "/arc" && entry.directory != "arc") continue;
                if (!_runtime.archive_mounts().receive(entry.name))
                    (void)_runtime.archive_mounts().mount_memory(
                        entry.name, archive->source().file_data(entry), archive->heap());
            }
        }
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
    }

    void StageInitializationService::initialize_scenario_resources() {
        require_live();
        const auto host_allocations = smgpc::compat::JkrHostAllocationScope{};
        if (_scene_obj_holder_binding == nullptr || _authored_data == nullptr ||
            _stage_resource_binding != nullptr) {
            aurora::throw_host_exception<std::logic_error>(
                "Scenario initialization requires one retained authored-data owner.");
        }
        _stage_resource_binding = std::make_unique<smgpc::compat::StageResourceBinding>(
            _runtime.dvd(), _authored_data->holders(), _authored_data->tables());
        _zone_matrix_binding = std::make_unique<smgpc::compat::StageZoneMatrixBinding>(
            _authored_data->holders(), _authored_data->tables());
        _stage_light_binding = std::make_unique<StageLightSceneBinding>(
            _runtime.dvd(), _request.stage_name, _authored_data->tables());
        // The original DemoDirector/executors exist before placement actors
        // initialize and attempt to join their zone-scoped groups.
        _demo_scene_runtime = std::make_unique<smgpc::compat::DemoSceneRuntime>(
            _runtime.dvd(), _authored_data->placements(),
            _authored_data->general_positions());
        smgpc::compat::claim_name_obj_runtime_ownership(_demo_scene_runtime.get(), this);
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

    void StageInitializationService::init_stage_audio() {
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

    void StageInitializationService::trace_placement_object(const StagePlacementObject &placement) const {
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

    void StageInitializationService::appear_roots() {
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

    void StageInitializationService::destroy_roots() {
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

    NameObj *StageInitializationService::root() const {
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

    std::string_view StageInitializationService::scene_name() const {
        return _request.scene_name;
    }

    std::string_view StageInitializationService::stage_name() const {
        return _request.stage_name;
    }

    s32 StageInitializationService::scenario_no() const {
        return _request.scenario_no;
    }

    const char *StageInitializationService::resolve_actor_name(
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
