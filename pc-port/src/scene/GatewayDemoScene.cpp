#include "scene/GatewayDemoScene.hpp"

#include "Game/Gravity/GravityCreator.hpp"
#include "Game/Gravity/GravityInfo.hpp"
#include "Game/Gravity/PlanetGravity.hpp"
#include "Game/Gravity/PlanetGravityManager.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/Air.hpp"
#include "Game/Map/LightFunction.hpp"
#include "Game/Map/PlanetMap.hpp"
#include "Game/Map/Sky.hpp"
#include "Game/MapObj/BrightObj.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/NameObj/NameObjFactory.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/GravityUtil.hpp"
#include "compat/CollisionPartsCompat.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "runtime/RuntimeContext.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/NameObjLifecycleService.hpp"
#include "scene/AreaObjRuntime.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "scene/StageLightSceneBinding.hpp"
#include "scene/nameobj/NameObjFactory.hpp"
#include "scene/nameobj/PlanetMapCatalog.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace smgpc::scene {
    namespace {

        constexpr auto cStageName = std::string_view{"HeavensDoorGalaxy"};
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
            const std::vector<StagePlacementObject> &placements, std::string_view object_name,
            std::string_view zone_name, std::string_view table_path) {
            const auto first = std::ranges::find_if(placements, [&](const auto &placement) {
                return placement.object_name == object_name && placement.zone_name == zone_name &&
                       placement.table_path == table_path;
            });
            require(first != placements.end(),
                    std::string(object_name) + " exact placement row is absent");
            const auto duplicate = std::ranges::find_if(std::next(first), placements.end(),
                                                        [&](const auto &placement) {
                                                            return placement.object_name == object_name &&
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

        [[nodiscard]] NameObjPlacementContext placement_context(
            const StagePlacementObject &placement) {
            return NameObjPlacementContext{
                .iter = JMapInfoIter(&placement.jmap_info, placement.jmap_entry_index),
                .source = NameObjPlacementSource::StagePlacement,
                .stage_name = placement.stage_name,
                .zone_name = placement.zone_name,
                .table_path = placement.table_path,
                .row = placement.jmap_entry_index,
                .local_id = placement.l_id,
            };
        }

    }  // namespace

    class GatewayDemoScene::Impl final {
    public:
        explicit Impl(smgpc::runtime::DvdFileSystemService &dvd) : _dvd(dvd) {
            const auto tables = resolve_stage_placement_tables(_dvd, cStageName, 1);
            require(!tables.empty(), "HeavensDoorGalaxy scenario 1 placement tables are absent");
            _planet_map_catalog =
                std::make_unique<smgpc::scene::nameobj::PlanetMapCatalog>(_dvd);
            _stage_light_binding = std::make_unique<StageLightSceneBinding>(
                _dvd, cStageName, tables);

            const auto start = select_stage_start_info(tables, 0, 0);
            require(start.has_value(), "scenario-1 root MarioNo 0 StartInfo is absent");
            _start = *start;
            validate_start();

            _placements = resolve_stage_placement_objects(_dvd, tables);
            _general_positions = select_stage_general_positions(tables);
            // GameScene creates its one DemoDirector from initForLiveActor
            // before SceneDataInitializer starts placement. Install the same
            // scene owner before any Gateway placement actor can register a
            // simple cast, DemoGroup cast, or demo action.
            _demo_scene_runtime =
                std::make_unique<smgpc::compat::DemoSceneRuntime>(
                    _dvd, _placements, _general_positions);
            _planet_placement_source = &require_unique_placement(
                _placements, cPlanetName, cPlanetZoneName, "jmp/placement/common/objinfo");
            _planet_placement = *_planet_placement_source;
            _gravity_placement = require_unique_placement(
                _placements, cGravityName, cPlanetZoneName,
                "jmp/placement/common/planetobjinfo");
            _sky_placement_source = &require_unique_placement(
                _placements, cSkyName, cStageName, "jmp/placement/common/objinfo");
            _sky_placement = *_sky_placement_source;
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
                SceneObj_PlacementStateChecker,
                SceneObj_ClippingDirector,
                SceneObj_StageSwitchContainer,
                SceneObj_SwitchWatcherHolder,
                SceneObj_SleepControllerHolder,
                SceneObj_AreaObjContainer,
                SceneObj_BaseMatrixFollowTargetHolder,
                SceneObj_PlanetGravityManager,
                SceneObj_MarioHolder,
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

            for (const auto &placement : _placements) {
                if (!is_area_obj_placement_table(placement.table_path) ||
                    !placement_has_complete_area_obj_runtime(
                        placement.object_name, placement.table_path,
                        placement.factory_supported)) {
                    continue;
                }
                const auto *descriptor = find_complete_area_obj_placement_descriptor(
                    placement.object_name);
                const auto creator = NameObjFactory::getCreator(
                    placement.object_name.c_str());
                require(descriptor != nullptr && creator != nullptr &&
                            creator == descriptor->object_creator,
                        "complete Gateway AreaObj placement lost its shared descriptor/factory closure");
                auto object = std::unique_ptr<NameObj>{
                    creator(placement.object_name.c_str())};
                require(object != nullptr,
                        "complete Gateway AreaObj creator returned null");
                object->init(JMapInfoIter(&placement.jmap_info,
                                          placement.jmap_entry_index));
                _area_objects.push_back(std::move(object));
            }
            if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
                runtime != nullptr) {
                auto &lifecycle = runtime->name_obj_lifecycle();
                for (const auto &placement : _placements) {
                    const auto visual_kind =
                        smgpc::scene::nameobj::scene_visual_kind(placement.object_name);
                    if (visual_kind ==
                        smgpc::scene::nameobj::NameObjSceneVisualKind::None) {
                        continue;
                    }

                    const auto context = placement_context(placement);
                    const auto requests = lifecycle.preload_archives(
                        placement.object_name, &context);
                    require(!requests.empty() &&
                                std::ranges::all_of(requests, [](const auto &request) {
                                    return request.loaded;
                                }),
                            "authored visual archives were not accepted by the retail factory lifecycle");
                    auto object = lifecycle.construct_and_init(
                        placement.object_name, placement.object_name.c_str(), &context);
                    const auto has_exact_type =
                        (visual_kind ==
                             smgpc::scene::nameobj::NameObjSceneVisualKind::Sky &&
                         dynamic_cast<Sky *>(object.get()) != nullptr) ||
                        (visual_kind ==
                             smgpc::scene::nameobj::NameObjSceneVisualKind::Air &&
                         dynamic_cast<Air *>(object.get()) != nullptr) ||
                        (visual_kind ==
                             smgpc::scene::nameobj::NameObjSceneVisualKind::Planet &&
                         dynamic_cast<PlanetMap *>(object.get()) != nullptr) ||
                        (visual_kind ==
                             smgpc::scene::nameobj::NameObjSceneVisualKind::Bright &&
                         dynamic_cast<BrightObjBase *>(object.get()) != nullptr);
                    require(has_exact_type,
                            "authored visual creator kind differs from its exact Game actor");

                    auto *actor = object.get();
                    _visual_objects.push_back(std::move(object));
                    _visual_views.push_back(GatewayDemoVisual{
                        .placement = &placement,
                        .actor = actor,
                    });
                    if (&placement == _sky_placement_source) {
                        _sky_actor = dynamic_cast<ProjectionMapSky *>(actor);
                        require(_sky_actor != nullptr,
                                "authored Gateway sky row did not create ProjectionMapSky");
                    }
                    if (&placement == _planet_placement_source) {
                        _planet_actor = dynamic_cast<PlanetMap *>(actor);
                        require(_planet_actor != nullptr,
                                "authored Gateway planet row did not create PlanetMap");
                    }
                }
                require(_sky_actor != nullptr,
                        "authored Gateway sky was absent from the generic visual lifecycle");
                require(_planet_actor != nullptr,
                        "authored Gateway planet was absent from the generic visual lifecycle");

                _collision.build();
                _scene_binding->init_after_placement();
                for (auto &object : _area_objects) {
                    object->initAfterPlacement();
                }
                for (auto &object : _visual_objects) {
                    lifecycle.init_after_placement(*object);
                }
                _collision.build();
                const auto planet_collision =
                    smgpc::compat::actor_collision_parts_resources(_planet_actor);
                require(planet_collision.size() == 2U &&
                            planet_collision[0].resource_name == cPlanetName &&
                            planet_collision[1].resource_name == "MoveLimit" &&
                            !planet_collision[0].attributes_source.empty() &&
                            !planet_collision[1].attributes_source.empty() &&
                            _collision.stats().mesh_count >= planet_collision.size() &&
                            _collision.stats().triangle_count != 0U,
                        "ordinary PlanetMap did not retain main and MoveLimit KCL/PA registrations");
            } else {
                _scene_binding->init_after_placement();
                for (auto &object : _area_objects) {
                    object->initAfterPlacement();
                }
            }

            auto *gravity = _gravity_creator.createFromJMap(JMapInfoIter(
                &_gravity_placement.jmap_info, _gravity_placement.jmap_entry_index));
            require(gravity != nullptr && gravity == _gravity_creator.getGravity(),
                    "exact PointGravityCreator did not create the child-zone gravity");
            _gravity.reset(gravity);
            validate_gravity();
        }

        ~Impl() {
            _visual_views.clear();
            _sky_actor = nullptr;
            _planet_actor = nullptr;
            if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
                runtime != nullptr) {
                auto &lifecycle = runtime->name_obj_lifecycle();
                for (auto iter = _visual_objects.rbegin();
                     iter != _visual_objects.rend(); ++iter) {
                    if (*iter != nullptr) {
                        lifecycle.destroy(**iter);
                    }
                }
            }
            // PriorDrawAirHolder owns exact non-owning actor pointers. No
            // scene execution occurs between retiring the visual actors and
            // destroying the SceneObj binding that owns that holder.
            _visual_objects.clear();
            _collision.deactivate();
            _area_objects.clear();
            // The exact manager keeps non-owning retail pointers. Retire it
            // before releasing the development scene's gravity instance.
            _scene_binding.reset();
            _gravity.reset();
            _stage_light_binding.reset();
            _planet_map_catalog.reset();
        }

        void validate_start() const {
            require(_start.object_name == "Mario" && _start.stage_name == cStageName &&
                        _start.zone_name == cStageName && _start.layer_name == "layera" &&
                        _start.table_path == "jmp/start/layera/startinfo" &&
                        _start.start_id == 0 && _start.zone_id == 0 &&
                        _start.camera_id == 78 && _start.jmap_entry_index == 0,
                    "scenario-1 StartInfo identity differs from RMGK01");
            require_vec_near(_start.local_position, cStartPosition, 0.0005F,
                             "scenario-1 StartInfo position differs from RMGK01");
            require_vec_near(_start.world_position, cStartPosition, 0.0005F,
                             "scenario-1 StartInfo world transform differs from RMGK01");
            require_vec_near(_start.local_rotation, cStartRotation, 0.0005F,
                             "scenario-1 StartInfo rotation differs from RMGK01");
        }

        void validate_placements() const {
            require(_planet_placement.jmap_entry_index == 24 &&
                        _planet_placement.l_id == 69 && _planet_placement.zone_id == 5 &&
                        _planet_placement.layer_name == "common" &&
                        _planet_placement.object_archive_path.ends_with(
                            "/ObjectData/HeavensDoorMysteriousPlanet.arc"),
                    "mysterious-planet placement identity differs from RMGK01");
            require_vec_near(_planet_placement.translation, cPlanetCenter, 0.001F,
                             "mysterious-planet child-zone translation differs from RMGK01");

            require(_gravity_placement.jmap_entry_index == 0 &&
                        _gravity_placement.l_id == 0 && _gravity_placement.zone_id == 5 &&
                        _gravity_placement.layer_name == "common",
                    "child-zone GlobalPointGravity identity differs from RMGK01");
            require_vec_near(_gravity_placement.translation, cPlanetCenter, 0.001F,
                             "child-zone GlobalPointGravity center differs from RMGK01");
            require_vec_near(_gravity_placement.scale, {2.88F, 2.88F, 2.88F}, 0.0001F,
                             "child-zone GlobalPointGravity scale differs from RMGK01");

            require(_sky_placement.jmap_entry_index == 0 &&
                        _sky_placement.l_id == 0 && _sky_placement.zone_id == 0 &&
                        _sky_placement.zone_name == cStageName &&
                        _sky_placement.layer_name == "common" &&
                        _sky_placement.object_archive_path.ends_with(
                            "/ObjectData/VROrbit.arc") &&
                        _sky_placement.switch_appear_id == -1 &&
                        _sky_placement.switch_a_id == -1 &&
                        _sky_placement.switch_b_id == -1 &&
                        _sky_placement.object_args[0] == -1,
                    "Gateway sky placement identity differs from RMGK01");
            require_vec_near(_sky_placement.translation, cSkyPosition, 0.0005F,
                             "Gateway sky placement position differs from RMGK01");
            require_vec_near(_sky_placement.rotation, {0.0F, 0.0F, 0.0F}, 0.0001F,
                             "Gateway sky placement rotation differs from RMGK01");
            require_vec_near(_sky_placement.scale, {1.0F, 1.0F, 1.0F}, 0.0001F,
                             "Gateway sky placement scale differs from RMGK01");

            const auto gravity_iter = JMapInfoIter(
                &_gravity_placement.jmap_info, _gravity_placement.jmap_entry_index);
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

        smgpc::runtime::DvdFileSystemService &_dvd;
        StageStartInfo _start{};
        std::vector<StagePlacementObject> _placements{};
        std::vector<StageGeneralPos> _general_positions{};
        std::unique_ptr<smgpc::compat::DemoSceneRuntime> _demo_scene_runtime{};
        StagePlacementObject _planet_placement{};
        const StagePlacementObject *_planet_placement_source = nullptr;
        StagePlacementObject _gravity_placement{};
        StagePlacementObject _sky_placement{};
        const StagePlacementObject *_sky_placement_source = nullptr;
        StageCollisionService _collision{};
        SceneObjHolder _scene_obj_holder{};
        std::unique_ptr<SceneObjHolderBinding> _scene_binding{};
        std::unique_ptr<StageLightSceneBinding> _stage_light_binding{};
        std::unique_ptr<smgpc::scene::nameobj::PlanetMapCatalog> _planet_map_catalog{};
        std::vector<std::unique_ptr<NameObj>> _area_objects{};
        std::vector<std::unique_ptr<NameObj>> _visual_objects{};
        std::vector<GatewayDemoVisual> _visual_views{};
        ProjectionMapSky *_sky_actor = nullptr;
        PlanetMap *_planet_actor = nullptr;
        PointGravityCreator _gravity_creator{};
        std::unique_ptr<PlanetGravity> _gravity{};
    };

    GatewayDemoScene::GatewayDemoScene(smgpc::runtime::DvdFileSystemService &dvd)
        : _impl(std::make_unique<Impl>(dvd)) {
    }

    GatewayDemoScene::~GatewayDemoScene() = default;

    const StageStartInfo &GatewayDemoScene::start_info() const {
        return _impl->_start;
    }

    JMapInfoIter GatewayDemoScene::player_start_iter() const & {
        return _impl->_start.iter();
    }

    const StagePlacementObject &GatewayDemoScene::planet_placement() const {
        return _impl->_planet_placement;
    }

    const StagePlacementObject &GatewayDemoScene::gravity_placement() const {
        return _impl->_gravity_placement;
    }

    const StagePlacementObject &GatewayDemoScene::sky_placement() const {
        return _impl->_sky_placement;
    }

    ProjectionMapSky *GatewayDemoScene::sky() {
        return _impl->_sky_actor;
    }

    const ProjectionMapSky *GatewayDemoScene::sky() const {
        return _impl->_sky_actor;
    }

    PlanetMap *GatewayDemoScene::planet() {
        return _impl->_planet_actor;
    }

    const PlanetMap *GatewayDemoScene::planet() const {
        return _impl->_planet_actor;
    }

    std::span<const GatewayDemoVisual> GatewayDemoScene::visuals() const {
        return _impl->_visual_views;
    }

    std::span<const StagePlacementObject> GatewayDemoScene::placements() const {
        return _impl->_placements;
    }

    std::span<const StageGeneralPos> GatewayDemoScene::general_positions() const {
        return _impl->_general_positions;
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
        return _impl->_collision;
    }

    const StageCollisionService &GatewayDemoScene::collision() const {
        return _impl->_collision;
    }

    const PlanetGravity &GatewayDemoScene::gravity() const {
        return *_impl->_gravity;
    }

    bool GatewayDemoScene::resolve_gravity(const NameObj &requester, const TVec3f &position,
                                           TVec3f *destination, GravityInfo *info) const {
        return MR::calcGravityVector(&requester, position, destination, info, 0U);
    }

    GatewayDemoStartContact GatewayDemoScene::prove_start_contact(
        const NameObj &requester) const {
        auto proof = GatewayDemoStartContact{};
        const auto start = as_vec3(_impl->_start.world_position);
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
