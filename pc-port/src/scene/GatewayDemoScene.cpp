#include "scene/GatewayDemoScene.hpp"

#include "Game/Gravity/GravityCreator.hpp"
#include "Game/Gravity/GravityInfo.hpp"
#include "Game/Gravity/PlanetGravity.hpp"
#include "Game/Gravity/PlanetGravityManager.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/GravityUtil.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

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
        constexpr auto cPlanetBdlName = std::string_view{"heavensdoormysteriousplanet.bdl"};
        constexpr auto cPlanetKclName = std::string_view{"heavensdoormysteriousplanet.kcl"};
        constexpr auto cPlanetPaName = std::string_view{"heavensdoormysteriousplanet.pa"};
        constexpr auto cPlanetCollisionSource =
            std::string_view{"HeavensDoorMysteriousPlanet.arc/heavensdoormysteriousplanet.kcl"};
        constexpr auto cStartPosition = std::array<float, 3U>{
            14459.978515625F, -12791.11328125F, 6059.91162109375F};
        constexpr auto cStartRotation = std::array<float, 3U>{
            124.76948547363281F, -114.13859558105469F, 47.654739379882812F};
        constexpr auto cPlanetCenter = std::array<float, 3U>{
            14760.0F, -10676.2255859375F, 6770.0F};

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

        [[nodiscard]] std::vector<std::uint8_t> require_resource_copy(
            const smgpc::resource::RarcArchive &archive, std::string_view basename) {
            const auto *entry = archive.find_by_basename(basename);
            require(entry != nullptr && entry->name == basename,
                    std::string("required archive resource is absent: ") + std::string(basename));
            const auto bytes = archive.file_data(*entry);
            require(!bytes.empty(),
                    std::string("required archive resource is empty: ") + std::string(basename));
            return std::vector<std::uint8_t>(bytes.begin(), bytes.end());
        }

        [[nodiscard]] std::size_t model_triangle_count(
            const smgpc::render::J3dModelGeometry &geometry) {
            auto count = std::size_t{};
            for (const auto &shape : geometry.shapes) {
                for (const auto &packet : shape.draw_packets) {
                    count += packet.indices.size() / 3U;
                }
            }
            return count;
        }

        [[nodiscard]] TVec3f as_vec3(const std::array<float, 3U> &value) {
            return TVec3f(value[0], value[1], value[2]);
        }

    }  // namespace

    class GatewayDemoScene::Impl final {
    public:
        explicit Impl(smgpc::runtime::DvdFileSystemService &dvd) : _dvd(dvd) {
            const auto tables = resolve_stage_placement_tables(_dvd, cStageName, 1);
            require(!tables.empty(), "HeavensDoorGalaxy scenario 1 placement tables are absent");

            const auto start = select_stage_start_info(tables, 0, 0);
            require(start.has_value(), "scenario-1 root MarioNo 0 StartInfo is absent");
            _start = *start;
            validate_start();

            const auto placements = resolve_stage_placement_objects(_dvd, tables);
            _planet_placement = require_unique_placement(
                placements, cPlanetName, cPlanetZoneName, "jmp/placement/common/objinfo");
            _gravity_placement = require_unique_placement(
                placements, cGravityName, cPlanetZoneName,
                "jmp/placement/common/planetobjinfo");
            validate_placements();

            const auto archive_path = _dvd.find_object_archive(cPlanetName);
            require(archive_path.has_value(),
                    "HeavensDoorMysteriousPlanet.arc is absent from ObjectData");
            require(archive_path->filename() == "HeavensDoorMysteriousPlanet.arc",
                    "planet archive resolved through a substituted filename");
            auto &archive = _dvd.archive_for_path(*archive_path);
            _planet_bdl = require_resource_copy(archive, cPlanetBdlName);
            _planet_kcl = require_resource_copy(archive, cPlanetKclName);
            _planet_pa = require_resource_copy(archive, cPlanetPaName);

            _planet_geometry = smgpc::render::extract_j3d_model_geometry(_planet_bdl);
            require(!_planet_geometry.shapes.empty() && model_triangle_count(_planet_geometry) != 0U,
                    "retail planet BDL has no parsed draw geometry");
            const auto attributes = smgpc::resource::BcsvTable::from_bytes(_planet_pa);
            require(attributes.entry_count() != 0U,
                    "retail planet PA has no collision-attribute rows");

            _planet_collision_host.initHitSensor(1);
            auto *planet_body_sensor =
                MR::addBodyMessageSensorMapObj(&_planet_collision_host);
            require(planet_body_sensor != nullptr,
                    "mysterious-planet collision has no real map-object body sensor");
            const auto registration = _collision.register_kcl(
                _planet_kcl, stage_collision_matrix(_planet_placement),
                std::string(cPlanetCollisionSource), nullptr, _planet_pa,
                planet_body_sensor);
            require(registration.accepted,
                    "retail mysterious-planet KCL could not be registered");
            _collision.build();
            require(_collision.stats().mesh_count == 1U &&
                        _collision.stats().triangle_count != 0U,
                    "retail mysterious-planet KCL produced no collision surface");
            _collision.activate();

            _scene_binding = std::make_unique<SceneObjHolderBinding>(_scene_obj_holder);
            auto *manager = dynamic_cast<PlanetGravityManager *>(
                MR::createSceneObj(SceneObj_PlanetGravityManager));
            require(manager != nullptr,
                    "exact PlanetGravityManager SceneObj could not be created");
            require(MR::createSceneObj(SceneObj_MarioHolder) != nullptr,
                    "exact MarioHolder SceneObj could not be created");

            auto *gravity = _gravity_creator.createFromJMap(JMapInfoIter(
                &_gravity_placement.jmap_info, _gravity_placement.jmap_entry_index));
            require(gravity != nullptr && gravity == _gravity_creator.getGravity(),
                    "exact PointGravityCreator did not create the child-zone gravity");
            _gravity.reset(gravity);
            validate_gravity();
        }

        ~Impl() {
            _collision.deactivate();
            // The exact manager keeps non-owning retail pointers. Retire it
            // before releasing the development scene's gravity instance.
            _scene_binding.reset();
            _gravity.reset();
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
        StagePlacementObject _planet_placement{};
        StagePlacementObject _gravity_placement{};
        std::vector<std::uint8_t> _planet_bdl{};
        std::vector<std::uint8_t> _planet_kcl{};
        std::vector<std::uint8_t> _planet_pa{};
        smgpc::render::J3dModelGeometry _planet_geometry{};
        // The development subset has no broad placement actor, but its exact
        // KCL still needs the same real sensor provenance Binder would receive
        // from that actor in a full stage.
        LiveActor _planet_collision_host{"Gateway mysterious-planet collision host"};
        StageCollisionService _collision{};
        SceneObjHolder _scene_obj_holder{};
        std::unique_ptr<SceneObjHolderBinding> _scene_binding{};
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

    const smgpc::render::J3dModelGeometry &GatewayDemoScene::planet_geometry() const {
        return _impl->_planet_geometry;
    }

    std::span<const std::uint8_t> GatewayDemoScene::planet_bdl() const {
        return _impl->_planet_bdl;
    }

    std::span<const std::uint8_t> GatewayDemoScene::planet_kcl() const {
        return _impl->_planet_kcl;
    }

    std::span<const std::uint8_t> GatewayDemoScene::planet_pa() const {
        return _impl->_planet_pa;
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
        require(surface.has_value() && surface->source_name == cPlanetCollisionSource &&
                    !surface->attributes.empty(),
                "scenario-1 contact did not retain the exact planet KCL/PA identity");
        proof.surface = *surface;
        proof.separation = (proof.collision.position - start).length();
        return proof;
    }

}  // namespace smgpc::scene
