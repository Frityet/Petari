#include "scene/GatewayDemoScene.hpp"

#include "Game/Gravity/GravityInfo.hpp"
#include "Game/Gravity/GlobalGravityObj.hpp"
#include "Game/Gravity/PlanetGravity.hpp"
#include "Game/Gravity/PlanetGravityManager.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/Air.hpp"
#include "Game/Map/LightFunction.hpp"
#include "Game/Map/PlanetMap.hpp"
#include "Game/Map/Sky.hpp"
#include "Game/MapObj/BrightObj.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/GravityUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/CollisionPartsCompat.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "compat/StageScenarioMetadataResolver.hpp"
#include "compat/StageSessionState.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/AuthoredPlacementInstantiator.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/StageAuthoredData.hpp"
#include "compat/StageZoneMatrixRegistry.hpp"
#include "scene/StageEventCameraBinding.hpp"
#include "scene/StageLightSceneBinding.hpp"
#include "scene/nameobj/NameObjFactory.hpp"
#include "scene/nameobj/ObjectNameTable.hpp"
#include "scene/nameobj/PlanetMapCatalog.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace smgpc::scene {
    namespace {

        constexpr auto cStageName = std::string_view{"HeavensDoorGalaxy"};
        constexpr auto cSceneName = std::string_view{"Game"};
        constexpr auto cPlanetZoneName = std::string_view{"HeavensDoorMysteriousZone"};
        constexpr auto cPlanetName = std::string_view{"HeavensDoorMysteriousPlanet"};
        constexpr auto cGravityName = std::string_view{"GlobalPointGravity"};
        constexpr auto cSkyName = std::string_view{"VROrbit"};
        constexpr auto cPlanetCollisionSource =
            std::string_view{"HeavensDoorMysteriousPlanet.arc:/heavensdoormysteriousplanet.kcl"};
        constexpr auto cStartPosition = std::array<float, 3U>{
            14459.978515625F, -12791.11328125F, 6059.91162109375F};
        constexpr auto cStartRotation = std::array<float, 3U>{
            124.76948547363281F, -114.13859558105469F, 47.654739379882812F};
        constexpr auto cPlanetCenter = std::array<float, 3U>{
            14760.0F, -10676.2255859375F, 6770.0F};
        constexpr auto cSkyPosition = std::array<float, 3U>{
            0.0F, -3532.498046875F, -1040.0F};

        [[noreturn]] void reject(std::string_view detail) {
            throw std::runtime_error("Gateway demo scene rejected non-exact retail data: " +
                                     std::string(detail));
        }

        void require(bool condition, std::string_view detail) {
            if (!condition) {
                reject(detail);
            }
        }

        void require_near(float actual, float expected, float tolerance, std::string_view detail) {
            if (!std::isfinite(actual) || std::fabs(actual - expected) > tolerance) {
                reject(std::string(detail) + ";actual=" + std::to_string(actual) +
                       ";expected=" + std::to_string(expected));
            }
        }

        void require_vec_near(const std::array<float, 3U> &actual,
                              const std::array<float, 3U> &expected, float tolerance,
                              std::string_view detail) {
            for (auto axis = std::size_t{}; axis < actual.size(); ++axis) {
                require_near(actual[axis], expected[axis], tolerance,
                             std::string(detail) + ";axis=" + std::to_string(axis));
            }
        }

        [[nodiscard]] const StagePlacementObject &require_unique_placement(
            std::span<const StagePlacementObject> placements,
            std::string_view object_name,
            std::string_view zone_name, std::string_view table_path) {
            const auto first = std::ranges::find_if(placements, [&](const auto &placement) {
                return authored_placement_identifier(placement) == object_name &&
                       placement.zone_name == zone_name &&
                       placement.table_path == table_path;
            });
            require(first != placements.end(),
                    std::string(object_name) + " exact placement row is absent");
            const auto duplicate = std::ranges::find_if(std::next(first), placements.end(),
                                                        [&](const auto &placement) {
                                                            return authored_placement_identifier(placement) == object_name &&
                                                                   placement.zone_name == zone_name &&
                                                                   placement.table_path == table_path;
                                                        });
            require(duplicate == placements.end(),
                    std::string(object_name) + " exact placement row is ambiguous");
            return *first;
        }

        [[nodiscard]] TVec3f as_vec3(const std::array<float, 3U> &value) {
            return TVec3f(value[0], value[1], value[2]);
        }

    }  // namespace

    class GatewayDemoScene::Impl final {
    public:
        explicit Impl(smgpc::runtime::DvdFileSystemService &dvd)
            : _dvd(dvd) {
            const auto scenario_metadata =
                smgpc::compat::resolve_stage_scenario_metadata(
                    _dvd, cStageName, 1);
            _stage_session =
                std::make_unique<smgpc::compat::StageSessionState>(
                    cSceneName, cStageName, 1, JMapIdInfo(0, 0),
                    scenario_metadata);
            _stage_session_binding =
                std::make_unique<smgpc::compat::StageSessionBinding>(
                    *_stage_session);
            _planet_map_catalog =
                std::make_unique<smgpc::scene::nameobj::PlanetMapCatalog>(_dvd);
            _object_name_table =
                std::make_unique<smgpc::scene::nameobj::ObjectNameTable>(_dvd);
            // Ordinary PlanetMap support is catalog-derived, so resolve the
            // retained authored set only after publishing that shared catalog.
            _authored_data = std::make_unique<StageAuthoredData>(
                StageAuthoredData::resolve(_dvd, cStageName, 1, 0, 0));
            _zone_matrix_binding = std::make_unique<smgpc::compat::StageZoneMatrixBinding>(
                _authored_data->holders(), _authored_data->tables());
            require(!_authored_data->tables().empty(),
                    "HeavensDoorGalaxy scenario 1 placement tables are absent");
            _stage_light_binding = std::make_unique<StageLightSceneBinding>(
                _dvd, cStageName, _authored_data->tables());

            require(_authored_data->start_info().has_value(),
                    "scenario-1 root MarioNo 0 StartInfo is absent");
            _start = &*_authored_data->start_info();
            validate_start();

            // GameScene creates its one DemoDirector from initForLiveActor
            // before SceneDataInitializer starts placement. Install the same
            // scene owner before any Gateway placement actor can register a
            // simple cast, DemoGroup cast, or demo action.
            _demo_scene_runtime =
                std::make_unique<smgpc::compat::DemoSceneRuntime>(
                    _dvd, _authored_data->placements(),
                    _authored_data->general_positions());
            _planet_placement = &require_unique_placement(
                _authored_data->placements(), cPlanetName, cPlanetZoneName,
                "jmp/placement/common/objinfo");
            _gravity_placement = &require_unique_placement(
                _authored_data->placements(), cGravityName, cPlanetZoneName,
                "jmp/placement/common/planetobjinfo");
            _sky_placement = &require_unique_placement(
                _authored_data->placements(), cSkyName, cStageName,
                "jmp/placement/common/objinfo");
            validate_placements();

            const auto archive_path = _dvd.find_object_archive(cPlanetName);
            require(archive_path.has_value(),
                    "HeavensDoorMysteriousPlanet.arc is absent from ObjectData");
            require(archive_path->filename() == "HeavensDoorMysteriousPlanet.arc",
                    "planet archive resolved through a substituted filename");

            // Exact PlanetMap::init owns all KCL registrations. Publish the
            // empty scene service first so the ordinary actor can append its
            // main and auxiliary CollisionParts through the shared boundary.
            _collision.clear();
            _collision.build();
            _collision.activate();

            _scene_binding = std::make_unique<SceneObjHolderBinding>(_scene_obj_holder);
            constexpr auto required_scene_objects = std::array{
                SceneObj_MessageSensorHolder,
                SceneObj_PlacementStateChecker,
                SceneObj_ClippingDirector,
                SceneObj_LightDirector,
                SceneObj_StageSwitchContainer,
                SceneObj_SwitchWatcherHolder,
                SceneObj_SleepControllerHolder,
                SceneObj_AreaObjContainer,
                SceneObj_BaseMatrixFollowTargetHolder,
                SceneObj_PlanetGravityManager,
                SceneObj_MarioHolder,
                SceneObj_GroupCheckManager,
                SceneObj_TalkDirector,
            };
            for (const auto id : required_scene_objects) {
                require(MR::createSceneObj(id) != nullptr,
                        "required Gateway placement SceneObj could not be created");
            }
            auto *manager = dynamic_cast<PlanetGravityManager *>(
                _scene_obj_holder.getObj(SceneObj_PlanetGravityManager));
            require(manager != nullptr,
                    "exact PlanetGravityManager SceneObj could not be created");
            LightFunction::initLightRegisterAll();

            _runtime = smgpc::runtime::RuntimeContext::try_instance();
            require(_runtime != nullptr,
                    "Gateway placement construction requires the active RuntimeContext lifecycle");
            _event_camera_binding =
                std::make_unique<StageEventCameraBinding>(
                    _runtime->camera_system(), _dvd,
                    _authored_data->tables());
            _authored_placements =
                std::make_unique<AuthoredPlacementInstantiator>(
                    *_authored_data, _runtime->name_obj_lifecycle(),
                    AuthoredPlacementInstantiationOptions{
                        .mode = AuthoredPlacementMode::
                            SupportedSubsetForDevelopment,
                        .actor_name_resolver = [this](
                                                   const auto &placement) {
                            const auto *localized =
                                _object_name_table->lookup(
                                    authored_placement_identifier(placement));
                            return localized != nullptr
                                       ? std::optional<std::string>{*localized}
                                       : std::optional<std::string>{
                                             std::string(
                                                 authored_placement_identifier(
                                                     placement))};
                        },
                    });
            (void)_authored_placements->preload();
            _state = GatewayDemoSceneState::Preloaded;
        }

        void finalize_placements(LiveActor &player) {
            if (_state != GatewayDemoSceneState::Preloaded) {
                throw std::logic_error(
                    "Gateway placements can only be finalized once from the preloaded boundary.");
            }

            _state = GatewayDemoSceneState::Finalizing;
            try {
                require(smgpc::runtime::RuntimeContext::try_instance() == _runtime,
                        "Gateway finalization lost its active RuntimeContext owner");
                require(_runtime->player_system().attached_actor() == &player,
                        "Gateway finalization requires the supplied external player to be attached");
                require(smgpc::compat::has_actor_runtime_state(&player) &&
                            _runtime->scheduler()
                                .light_type_for_actor(player)
                                .has_value(),
                        "Gateway finalization requires an initialized external player scene registration");
                require(StageCollisionService::active() == &_collision,
                        "Gateway collision service is no longer the active scene owner");

                const auto &report = _authored_placements->instantiate();
                derive_authored_actors();
                validate_gravity();

                // Exact placement init has registered its collision and
                // SceneObjs. Publish that first registry before any retail
                // initAfterPlacement callback can query it.
                _collision.build();
                _scene_binding->init_after_placement();
                _runtime->name_obj_lifecycle().init_after_placement(player);
                require(_runtime->player_system().attached_actor() == &player,
                        "the external player detached during initAfterPlacement");
                _runtime->player_system().synchronize_attached_actor();
                (void)_authored_placements->init_after_placement();

                validate_planet_collision();
                _collision.build();
                _state = GatewayDemoSceneState::Active;

#ifndef NDEBUG
                emit_placement_report(report);
#endif
            } catch (...) {
                retire();
                throw;
            }
        }

        void derive_authored_actors() {
            for (const auto &instance : _authored_placements->instances()) {
                const auto visual_kind =
                    smgpc::scene::nameobj::scene_visual_kind(
                        authored_placement_identifier(
                            *instance.placement));
                if (visual_kind !=
                    smgpc::scene::nameobj::NameObjSceneVisualKind::None) {
                    const auto has_exact_type =
                        (visual_kind == smgpc::scene::nameobj::
                                            NameObjSceneVisualKind::Sky &&
                         dynamic_cast<Sky *>(instance.actor) != nullptr) ||
                        (visual_kind == smgpc::scene::nameobj::
                                            NameObjSceneVisualKind::Air &&
                         dynamic_cast<Air *>(instance.actor) != nullptr) ||
                        (visual_kind == smgpc::scene::nameobj::
                                            NameObjSceneVisualKind::Planet &&
                         dynamic_cast<PlanetMap *>(instance.actor) != nullptr) ||
                        (visual_kind == smgpc::scene::nameobj::
                                            NameObjSceneVisualKind::Bright &&
                         dynamic_cast<BrightObjBase *>(instance.actor) != nullptr);
                    require(has_exact_type,
                            "authored visual creator kind differs from its exact Game actor");
                    _visual_views.push_back(GatewayDemoVisual{
                        .placement = instance.placement,
                        .actor = instance.actor,
                    });
                }

                if (instance.placement == _sky_placement) {
                    _sky_actor =
                        dynamic_cast<ProjectionMapSky *>(instance.actor);
                }
                if (instance.placement == _planet_placement) {
                    _planet_actor = dynamic_cast<PlanetMap *>(instance.actor);
                }
                if (instance.placement == _gravity_placement) {
                    _gravity_actor =
                        dynamic_cast<GlobalGravityObj *>(instance.actor);
                }
            }
            require(_sky_actor != nullptr,
                    "authored Gateway sky was absent from the shared placement lifecycle");
            require(_planet_actor != nullptr,
                    "authored Gateway planet was absent from the shared placement lifecycle");
            require(_gravity_actor != nullptr &&
                        _gravity_actor->getGravity() != nullptr,
                    "authored Gateway point gravity was absent from the shared placement lifecycle");
            _gravity = _gravity_actor->getGravity();
        }

        void validate_planet_collision() const {
            const auto planet_collision =
                smgpc::compat::actor_collision_parts_resources(_planet_actor);
            require(planet_collision.size() == 2U &&
                        planet_collision[0].resource_name == cPlanetName &&
                        planet_collision[1].resource_name == "MoveLimit" &&
                        !planet_collision[0].attributes_source.empty() &&
                        !planet_collision[1].attributes_source.empty() &&
                        _collision.stats().mesh_count >=
                            planet_collision.size() &&
                        _collision.stats().triangle_count != 0U,
                    "ordinary PlanetMap did not retain main and MoveLimit KCL/PA registrations");
        }

        void retire() noexcept {
            if (_state == GatewayDemoSceneState::Retired) {
                return;
            }
            _state = GatewayDemoSceneState::Retired;
            _visual_views.clear();
            _sky_actor = nullptr;
            _planet_actor = nullptr;
            _gravity_actor = nullptr;
            _gravity = nullptr;
            // PriorDrawAirHolder owns exact non-owning actor pointers. No
            // scene execution occurs between reverse placement teardown and
            // destroying the SceneObj binding that owns that holder.
            if (_authored_placements != nullptr) {
                try {
                    _authored_placements->clear();
                } catch (...) {
                    // The lifecycle still resets every actor at its reverse
                    // retirement point before propagating a destroy failure.
                }
            }
            _collision.clear();
            _collision.deactivate();
        }

        ~Impl() {
            retire();
            // The exact manager keeps non-owning pointers to authored gravity
            // instances and is retired after their actor wrappers.
            _scene_binding.reset();
            _stage_light_binding.reset();
            _event_camera_binding.reset();
            _planet_map_catalog.reset();
        }

#ifndef NDEBUG
        void emit_placement_report(
            const AuthoredPlacementInstantiationReport &report) const {
            _runtime->emit_semantic_trace_event(
                "placement", "gateway_development_subset_summary",
                "stage=" + std::string(cStageName) +
                    ";scenario=1;objects=" +
                    std::to_string(report.entries.size()) +
                    ";ready=" + std::to_string(report.ready_count) +
                    ";ignored=" + std::to_string(report.ignored_count) +
                    ";blocked=" + std::to_string(report.blocked_count) +
                    ";created=" + std::to_string(report.created_count) +
                    ";mode=supported_subset_for_development");
            for (const auto &entry : report.entries) {
                if (entry.support.kind !=
                        AuthoredPlacementSupportKind::Blocked ||
                    entry.placement == nullptr) {
                    continue;
                }
                _runtime->emit_semantic_trace_event(
                    "placement", "gateway_development_subset_blocked",
                    "object=" + std::string(authored_placement_identifier(
                                    *entry.placement)) +
                        ";raw_name=" + entry.placement->object_name +
                        ";zone=" + entry.placement->zone_name +
                        ";table=" + entry.placement->table_path +
                        ";row=" +
                        std::to_string(entry.placement->jmap_entry_index) +
                        ";reason=" + entry.support.reason);
            }
        }
#endif

        void validate_start() const {
            require(_start != nullptr && _start->object_name == "Mario" &&
                        _start->stage_name == cStageName &&
                        _start->zone_name == cStageName &&
                        _start->layer_name == "layera" &&
                        _start->table_path == "jmp/start/layera/startinfo" &&
                        _start->start_id == 0 && _start->zone_id == 0 &&
                        _start->camera_id == 78 &&
                        _start->jmap_entry_index == 0,
                    "scenario-1 StartInfo identity differs from RMGK01");
            require_vec_near(_start->local_position, cStartPosition, 0.0005F,
                             "scenario-1 StartInfo position differs from RMGK01");
            require_vec_near(_start->world_position, cStartPosition, 0.0005F,
                             "scenario-1 StartInfo world transform differs from RMGK01");
            require_vec_near(_start->local_rotation, cStartRotation, 0.0005F,
                             "scenario-1 StartInfo rotation differs from RMGK01");
        }

        void validate_placements() const {
            require(_planet_placement != nullptr &&
                        _planet_placement->jmap_entry_index == 24 &&
                        _planet_placement->l_id == 69 &&
                        _planet_placement->zone_id == 5 &&
                        _planet_placement->layer_name == "common" &&
                        _planet_placement->object_archive_path.ends_with(
                            "/ObjectData/HeavensDoorMysteriousPlanet.arc"),
                    "mysterious-planet placement identity differs from RMGK01");
            require_vec_near(_planet_placement->translation, cPlanetCenter, 0.001F,
                             "mysterious-planet child-zone translation differs from RMGK01");

            require(_gravity_placement != nullptr &&
                        _gravity_placement->jmap_entry_index == 0 &&
                        _gravity_placement->l_id == 0 &&
                        _gravity_placement->zone_id == 5 &&
                        _gravity_placement->layer_name == "common",
                    "child-zone GlobalPointGravity identity differs from RMGK01");
            require_vec_near(_gravity_placement->translation, cPlanetCenter, 0.001F,
                             "child-zone GlobalPointGravity center differs from RMGK01");
            require_vec_near(_gravity_placement->scale, {2.88F, 2.88F, 2.88F}, 0.0001F,
                             "child-zone GlobalPointGravity scale differs from RMGK01");

            require(_sky_placement != nullptr &&
                        _sky_placement->jmap_entry_index == 0 &&
                        _sky_placement->l_id == 0 &&
                        _sky_placement->zone_id == 0 &&
                        _sky_placement->zone_name == cStageName &&
                        _sky_placement->layer_name == "common" &&
                        _sky_placement->object_archive_path.ends_with(
                            "/ObjectData/VROrbit.arc") &&
                        _sky_placement->switch_appear_id == -1 &&
                        _sky_placement->switch_a_id == -1 &&
                        _sky_placement->switch_b_id == -1 &&
                        _sky_placement->object_args[0] == -1,
                    "Gateway sky placement identity differs from RMGK01");
            require_vec_near(_sky_placement->translation, cSkyPosition, 0.0005F,
                             "Gateway sky placement position differs from RMGK01");
            require_vec_near(_sky_placement->rotation, {0.0F, 0.0F, 0.0F}, 0.0001F,
                             "Gateway sky placement rotation differs from RMGK01");
            require_vec_near(_sky_placement->scale, {1.0F, 1.0F, 1.0F}, 0.0001F,
                             "Gateway sky placement scale differs from RMGK01");

            const auto gravity_iter = JMapInfoIter(
                &_gravity_placement->jmap_info,
                _gravity_placement->jmap_entry_index);
            auto range = float{};
            auto distant = float{};
            const char *gravity_type = nullptr;
            require(gravity_iter.getValue("Range", &range) &&
                        gravity_iter.getValue("Distant", &distant) &&
                        gravity_iter.getValue("Gravity_type", &gravity_type) &&
                        gravity_type != nullptr && std::string_view(gravity_type) == "Normal",
                    "child-zone GlobalPointGravity parameters are incomplete");
            require_near(range, 8200.0F, 0.001F,
                         "child-zone GlobalPointGravity range differs from RMGK01");
            require_near(distant, 0.0F, 0.001F,
                         "child-zone GlobalPointGravity distant differs from RMGK01");
        }

        void validate_gravity() const {
            require(_gravity != nullptr && _gravity->mGravityType == GRAVITY_TYPE_NORMAL &&
                        _gravity->mGravityPower == GRAVITY_POWER_NORMAL &&
                        _gravity->mPriority == 0 && _gravity->mIsRegistered,
                    "exact point-gravity runtime parameters differ from the retail row");
            require_near(_gravity->mRange, 8200.0F, 0.001F,
                         "exact point-gravity runtime range differs from the retail row");
            require_near(_gravity->mDistant, 0.0F, 0.001F,
                         "exact point-gravity runtime distant differs from the retail row");
        }

        void require_active(std::string_view surface) const {
            if (_state != GatewayDemoSceneState::Active) {
                throw std::logic_error(
                    "Gateway placement surface requires an active placement lease: " +
                    std::string(surface));
            }
        }

        smgpc::runtime::DvdFileSystemService &_dvd;
        smgpc::runtime::RuntimeContext *_runtime = nullptr;
        GatewayDemoSceneState _state = GatewayDemoSceneState::Preloaded;
        // Session state precedes its binding and all stage owners so reverse
        // destruction keeps it active through Talk and placement teardown.
        std::unique_ptr<smgpc::compat::StageSessionState> _stage_session{};
        std::unique_ptr<smgpc::compat::StageSessionBinding>
            _stage_session_binding{};
        std::unique_ptr<StageAuthoredData> _authored_data{};
        std::unique_ptr<smgpc::compat::StageZoneMatrixBinding> _zone_matrix_binding{};
        std::unique_ptr<StageEventCameraBinding> _event_camera_binding{};
        const StageStartInfo *_start = nullptr;
        std::unique_ptr<smgpc::compat::DemoSceneRuntime> _demo_scene_runtime{};
        const StagePlacementObject *_planet_placement = nullptr;
        const StagePlacementObject *_gravity_placement = nullptr;
        const StagePlacementObject *_sky_placement = nullptr;
        StageCollisionService _collision{};
        SceneObjHolder _scene_obj_holder{};
        std::unique_ptr<SceneObjHolderBinding> _scene_binding{};
        std::unique_ptr<StageLightSceneBinding> _stage_light_binding{};
        std::unique_ptr<smgpc::scene::nameobj::PlanetMapCatalog> _planet_map_catalog{};
        std::unique_ptr<smgpc::scene::nameobj::ObjectNameTable>
            _object_name_table{};
        std::unique_ptr<AuthoredPlacementInstantiator> _authored_placements{};
        std::vector<GatewayDemoVisual> _visual_views{};
        ProjectionMapSky *_sky_actor = nullptr;
        PlanetMap *_planet_actor = nullptr;
        GlobalGravityObj *_gravity_actor = nullptr;
        PlanetGravity *_gravity = nullptr;
    };

    GatewayDemoScene::PlacementLease::PlacementLease(
        std::weak_ptr<Impl> impl) noexcept
        : _impl(std::move(impl)), _armed(true) {
    }

    GatewayDemoScene::PlacementLease::~PlacementLease() {
        reset();
    }

    GatewayDemoScene::PlacementLease::PlacementLease(
        PlacementLease &&other) noexcept
        : _impl(std::move(other._impl)),
          _armed(std::exchange(other._armed, false)) {
    }

    GatewayDemoScene::PlacementLease &
    GatewayDemoScene::PlacementLease::operator=(PlacementLease &&other) noexcept {
        if (this == &other) {
            return *this;
        }
        reset();
        _impl = std::move(other._impl);
        _armed = std::exchange(other._armed, false);
        return *this;
    }

    GatewayDemoScene::PlacementLease::operator bool() const noexcept {
        const auto impl = _impl.lock();
        return _armed && impl != nullptr &&
               impl->_state == GatewayDemoSceneState::Active;
    }

    void GatewayDemoScene::PlacementLease::reset() noexcept {
        if (!_armed) {
            return;
        }
        _armed = false;
        if (const auto impl = _impl.lock(); impl != nullptr) {
            impl->retire();
        }
        _impl.reset();
    }

    GatewayDemoScene::GatewayDemoScene(smgpc::runtime::DvdFileSystemService &dvd)
        : _impl(std::make_shared<Impl>(dvd)) {
    }

    GatewayDemoScene::~GatewayDemoScene() = default;

    GatewayDemoScene::PlacementLease GatewayDemoScene::finalize_placements(
        LiveActor &player) {
        _impl->finalize_placements(player);
        return PlacementLease{_impl};
    }

    GatewayDemoSceneState GatewayDemoScene::state() const noexcept {
        return _impl->_state;
    }

    const StageStartInfo &GatewayDemoScene::start_info() const {
        return *_impl->_start;
    }

    JMapInfoIter GatewayDemoScene::player_start_iter() const & {
        return _impl->_start->iter();
    }

    const StagePlacementObject &GatewayDemoScene::planet_placement() const {
        return *_impl->_planet_placement;
    }

    const StagePlacementObject &GatewayDemoScene::gravity_placement() const {
        return *_impl->_gravity_placement;
    }

    const StagePlacementObject &GatewayDemoScene::sky_placement() const {
        return *_impl->_sky_placement;
    }

    ProjectionMapSky *GatewayDemoScene::sky() {
        _impl->require_active("sky");
        return _impl->_sky_actor;
    }

    const ProjectionMapSky *GatewayDemoScene::sky() const {
        _impl->require_active("sky");
        return _impl->_sky_actor;
    }

    PlanetMap *GatewayDemoScene::planet() {
        _impl->require_active("planet");
        return _impl->_planet_actor;
    }

    const PlanetMap *GatewayDemoScene::planet() const {
        _impl->require_active("planet");
        return _impl->_planet_actor;
    }

    std::span<const GatewayDemoVisual> GatewayDemoScene::visuals() const {
        _impl->require_active("visuals");
        return _impl->_visual_views;
    }

    const AuthoredPlacementInstantiationReport &
    GatewayDemoScene::authored_placement_report() const {
        return _impl->_authored_placements->report();
    }

    const smgpc::compat::StageSessionState &
    GatewayDemoScene::stage_session() const {
        return *_impl->_stage_session;
    }

    std::span<const StagePlacementObject> GatewayDemoScene::placements() const {
        return _impl->_authored_data->placements();
    }

    std::span<const StageGeneralPos> GatewayDemoScene::general_positions() const {
        return _impl->_authored_data->general_positions();
    }

    smgpc::compat::DemoSceneRuntime &GatewayDemoScene::demo_runtime() {
        return *_impl->_demo_scene_runtime;
    }

    const smgpc::compat::DemoSceneRuntime &GatewayDemoScene::demo_runtime() const {
        return *_impl->_demo_scene_runtime;
    }

    SceneObjHolder &GatewayDemoScene::scene_obj_holder() {
        return _impl->_scene_obj_holder;
    }

    StageCollisionService &GatewayDemoScene::collision() {
        _impl->require_active("collision");
        return _impl->_collision;
    }

    const StageCollisionService &GatewayDemoScene::collision() const {
        _impl->require_active("collision");
        return _impl->_collision;
    }

    const PlanetGravity &GatewayDemoScene::gravity() const {
        _impl->require_active("gravity");
        return *_impl->_gravity;
    }

    bool GatewayDemoScene::resolve_gravity(const NameObj &requester, const TVec3f &position,
                                           TVec3f *destination, GravityInfo *info) const {
        _impl->require_active("gravity query");
        return MR::calcGravityVector(&requester, position, destination, info, 0U);
    }

    GatewayDemoStartContact GatewayDemoScene::prove_start_contact(
        const NameObj &requester) const {
        _impl->require_active("start contact");
        auto proof = GatewayDemoStartContact{};
        const auto start = as_vec3(_impl->_start->world_position);
        require(resolve_gravity(requester, start, &proof.gravity),
                "child-zone gravity does not reach the scenario-1 start point");

        // Start just outside the radial surface and cast back along the exact
        // resolved gravity. KCL arrows are intentionally one-sided.
        constexpr auto cOutwardLift = 10.0F;
        constexpr auto cInwardLength = 20.0F;
        const auto ray_start = start - proof.gravity * cOutwardLift;
        require(_impl->_collision.line_cast(ray_start, proof.gravity * cInwardLength,
                                            &proof.collision),
                "scenario-1 start point has no mysterious-planet KCL face along gravity");
        const auto surface = _impl->_collision.surface(proof.collision.triangle_index);
        require(surface.has_value() &&
                    std::string_view(surface->source_name).ends_with(cPlanetCollisionSource) &&
                    !surface->attributes.empty(),
                "scenario-1 contact did not retain the exact planet KCL/PA identity");
        proof.surface = *surface;
        proof.separation = (proof.collision.position - start).length();
        return proof;
    }

}  // namespace smgpc::scene
