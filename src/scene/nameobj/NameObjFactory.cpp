#include <aurora/exception.hpp>
#include "scene/nameobj/NameObjFactory.hpp"

#include "Game/Demo/DemoCastSubGroup.hpp"
#include "Game/Demo/DemoExecutor.hpp"
#include "Game/Demo/PrologueDirector.hpp"
#include "Game/Effect/SimpleEffectObj.hpp"
#include "Game/Gravity/GlobalGravityObj.hpp"
#include "Game/Map/Air.hpp"
#include "Game/Map/GroupSwitchWatcher.hpp"
#include "Game/Map/PlanetMap.hpp"
#include "Game/Map/Sky.hpp"
#include "Game/Map/SwitchSynchronizer.hpp"
#include "Game/MapObj/BrightObj.hpp"
#include "Game/MapObj/CollisionBlocker.hpp"
#include "Game/MapObj/EarthenPipe.hpp"
#include "Game/MapObj/Coin.hpp"
#include "Game/MapObj/CrystalCage.hpp"
#include "Game/MapObj/DummyDisplayModel.hpp"
#include "Game/MapObj/FlipPanel.hpp"
#include "Game/MapObj/HeavensDoorDemoObj.hpp"
#include "Game/MapObj/InvisiblePolygonObj.hpp"
#include "Game/MapObj/InvisiblePolygonObjGCapture.hpp"
#include "Game/MapObj/PlantGroup.hpp"
#include "Game/MapObj/PowerStar.hpp"
#include "Game/Map/Butterfly.hpp"
#include "Game/MapObj/PunchingKinoko.hpp"
#include "Game/MapObj/PowerStarAppearPoint.hpp"
#include "Game/MapObj/StarPiece.hpp"
#include "Game/MapObj/StarPieceGroup.hpp"
#include "Game/MapObj/ShockWaveGenerator.hpp"
#include "Game/MapObj/SimpleMapObj.hpp"
#include "Game/MapObj/WarpPod.hpp"
#include "Game/NameObj/NameObj.hpp"
#include "Game/NameObj/NameObjArchiveListCollector.hpp"
#include "Game/NameObj/NameObjFactory.hpp"
#include "Game/NameObj/ModelChangableObjFactory.hpp"
#include "Game/Util/MapPartsUtil.hpp"
#include "Game/NPC/DemoRabbit.hpp"
#include "Game/NPC/RunawayRabbitCollect.hpp"
#include "Game/NPC/Rosetta.hpp"
#include "Game/NPC/RunawayTico.hpp"
#include "Game/NPC/Tico.hpp"
#include "Game/Player/MarioActor.hpp"
#include "Game/Util/FileUtil.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "runtime/RuntimeServices.hpp"
#include "compat/GlobalGravityOwnership.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include <aurora/allocation.hpp>
#include "scene/AreaObjRuntime.hpp"
#include "scene/nameobj/PlanetMapCatalog.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

    constexpr auto cOriginalNames = std::to_array<std::string_view>({
#include "scene/nameobj/OriginalNameObjNames.inc"
    });

    template <typename T>
    NameObj *create_supported_name_obj(const char *pName) {
        return new T(pName);
    }

    NameObj *create_supported_planet_map(const char *pName) {
        return new PlanetMap(pName, nullptr);
    }

    struct UniquePlanetCreator {
        std::string_view creator_class;
        CreatorFuncPtr creator;
    };

    // Availability is registered by original actor class. Every authored
    // unique-planet row for that class therefore uses its actual constructor.
    constexpr auto cSupportedUniquePlanetCreators = std::to_array<UniquePlanetCreator>({
        {"SimpleMapObj", create_supported_name_obj<SimpleMapObj>},
    });

    [[nodiscard]] CreatorFuncPtr planet_map_creator(
        const smgpc::scene::nameobj::PlanetMapCatalogEntry &entry) {
        using Kind = smgpc::scene::nameobj::PlanetMapCatalogCreatorKind;
        if (entry.creator_kind == Kind::OrdinaryPlanetMap) {
            return create_supported_planet_map;
        }
        if (entry.creator_kind == Kind::UniqueCreator) {
            const auto found = std::ranges::find(cSupportedUniquePlanetCreators, entry.unique_creator_class,
                                                 &UniquePlanetCreator::creator_class);
            if (found != cSupportedUniquePlanetCreators.end()) {
                return found->creator;
            }
        }
        return nullptr;
    }

    // Retail ModelChangableObjFactory has twelve rows, including the repeated
    // TripodBossRotateParts entry. Keep membership separate from availability:
    // absent native actor closures must never become an unknown retail name.
    constexpr auto cModelChangableObjCreatorTable = std::to_array<Model2CreateFunc>({
        {nullptr, "AssemblyBlock", nullptr},
        {nullptr, "ClipFieldMapParts", nullptr},
        {nullptr, "FlexibleSphere", nullptr},
        {nullptr, "MercatorFixParts", nullptr},
        {nullptr, "MercatorRailMoveParts", nullptr},
        {nullptr, "MercatorRotateParts", nullptr},
        {nullptr, "TripodBossFixParts", nullptr},
        {nullptr, "TripodBossRailMoveParts", nullptr},
        {nullptr, "TripodBossRotateParts", nullptr},
        {nullptr, "TripodBossRotateParts", nullptr},
        {nullptr, "SimpleNormalMapObj", nullptr},
        {nullptr, "SunshadeMapParts", nullptr},
    });

    [[nodiscard]] const Model2CreateFunc *find_model_changing_entry(std::string_view object_name) {
        for (const auto &entry : cModelChangableObjCreatorTable) {
            if (entry._0 != nullptr ? object_name.starts_with(entry._0) : object_name == entry.mArchiveName) {
                return &entry;
            }
        }
        return nullptr;
    }

    // This is a compiled subset of the retail cCreateTable, not an alternate
    // placement policy. An entry is present only when its normal init path has
    // no known mandatory dependency on an unavailable host subsystem.
    constexpr auto cSupportedCreateTable = std::array{
        NameObjFactory::Name2CreateFunc{
            "Steam",
            create_supported_name_obj<SimpleEffectObj>,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "Mario",
            create_supported_name_obj<MarioActor>,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "MarioActor",
            create_supported_name_obj<MarioActor>,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "Coin",
            MR::createDirectSetCoin,
            "Coin",
        },
        NameObjFactory::Name2CreateFunc{
            "PurpleCoin",
            MR::createDirectSetPurpleCoin,
            "PurpleCoin",
        },
        NameObjFactory::Name2CreateFunc{
            "ShockWaveGenerator",
            create_supported_name_obj<ShockWaveGenerator>,
            "ShockWaveGenerator",
        },
        NameObjFactory::Name2CreateFunc{
            "FlipPanel",
            create_supported_name_obj<FlipPanel>,
            "FlipPanel",
        },
        NameObjFactory::Name2CreateFunc{
            "FlipPanelObserver",
            create_supported_name_obj<FlipPanelObserver>,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "FlipPanelReverse",
            create_supported_name_obj<FlipPanel>,
            "FlipPanelReverse",
        },
        NameObjFactory::Name2CreateFunc{
            "CrystalCageM",
            create_supported_name_obj<CrystalCage>,
            "CrystalCageM",
        },
        NameObjFactory::Name2CreateFunc{
            "StarPiece",
            create_supported_name_obj<StarPiece>,
            "StarPiece",
        },
        NameObjFactory::Name2CreateFunc{
            "StarPieceFlow",
            create_supported_name_obj<StarPieceGroup>,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "StarPieceGroup",
            create_supported_name_obj<StarPieceGroup>,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "PrologueDirector",
            create_supported_name_obj<PrologueDirector>,
            "DemoLetter",
        },
        NameObjFactory::Name2CreateFunc{
            "GroupSwitchWatcher",
            create_supported_name_obj<GroupSwitchWatcher>,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "SwitchSynchronizerReverse",
            create_supported_name_obj<SwitchSynchronizer>,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "VROrbit",
            create_supported_name_obj<ProjectionMapSky>,
            "VROrbit",
        },
        NameObjFactory::Name2CreateFunc{
            "VRDarkSpace",
            create_supported_name_obj<Sky>,
            "VRDarkSpace",
        },
        NameObjFactory::Name2CreateFunc{
            "VRSandwichSun",
            create_supported_name_obj<ProjectionMapSky>,
            "VRSandwichSun",
        },
        NameObjFactory::Name2CreateFunc{
            "GalaxySky",
            create_supported_name_obj<Sky>,
            "GalaxySky",
        },
        NameObjFactory::Name2CreateFunc{
            "MilkyWaySky",
            create_supported_name_obj<Sky>,
            "MilkyWaySky",
        },
        NameObjFactory::Name2CreateFunc{
            "HalfGalaxySky",
            create_supported_name_obj<ProjectionMapSky>,
            "HalfGalaxySky",
        },
        NameObjFactory::Name2CreateFunc{
            "GreenPlanetOrbitSky",
            create_supported_name_obj<ProjectionMapSky>,
            "GreenPlanetOrbitSky",
        },
        NameObjFactory::Name2CreateFunc{
            "PhantomSky",
            create_supported_name_obj<Sky>,
            "PhantomSky",
        },
        NameObjFactory::Name2CreateFunc{
            "KoopaVS1Sky",
            create_supported_name_obj<ProjectionMapSky>,
            "KoopaVS1Sky",
        },
        NameObjFactory::Name2CreateFunc{
            "KoopaVS2Sky",
            create_supported_name_obj<Sky>,
            "KoopaVS2Sky",
        },
        NameObjFactory::Name2CreateFunc{
            "FamicomMarioSky",
            create_supported_name_obj<Sky>,
            "FamicomMarioSky",
        },
        NameObjFactory::Name2CreateFunc{
            "DesertSky",
            create_supported_name_obj<Sky>,
            "DesertSky",
        },
        NameObjFactory::Name2CreateFunc{
            "ChildRoomSky",
            create_supported_name_obj<Sky>,
            "ChildRoomSky",
        },
        NameObjFactory::Name2CreateFunc{
            "AuroraSky",
            create_supported_name_obj<Sky>,
            "AuroraSky",
        },
        NameObjFactory::Name2CreateFunc{
            "CloudSky",
            create_supported_name_obj<ProjectionMapSky>,
            "CloudSky",
        },
        NameObjFactory::Name2CreateFunc{
            "RockPlanetOrbitSky",
            create_supported_name_obj<ProjectionMapSky>,
            "RockPlanetOrbitSky",
        },
        NameObjFactory::Name2CreateFunc{
            "StarrySky",
            create_supported_name_obj<Sky>,
            "StarrySky",
        },
        NameObjFactory::Name2CreateFunc{
            "SummerSky",
            create_supported_name_obj<Sky>,
            "SummerSky",
        },
        NameObjFactory::Name2CreateFunc{
            "AstroDomeSkyA",
            create_supported_name_obj<Sky>,
            "AstroDomeSkyA",
        },
        NameObjFactory::Name2CreateFunc{
            "HomeAir",
            create_supported_name_obj<Air>,
            "HomeAir",
        },
        NameObjFactory::Name2CreateFunc{
            "SphereAir",
            create_supported_name_obj<PriorDrawAir>,
            "SphereAir",
        },
        NameObjFactory::Name2CreateFunc{
            "SunsetAir",
            create_supported_name_obj<ProjectionMapAir>,
            "SunsetAir",
        },
        NameObjFactory::Name2CreateFunc{
            "FineAir",
            create_supported_name_obj<ProjectionMapAir>,
            "FineAir",
        },
        NameObjFactory::Name2CreateFunc{
            "DimensionAir",
            create_supported_name_obj<AirFar100m>,
            "DimensionAir",
        },
        NameObjFactory::Name2CreateFunc{
            "DarknessRoomAir",
            create_supported_name_obj<Air>,
            "DarknessRoomAir",
        },
        NameObjFactory::Name2CreateFunc{
            "TwilightAir",
            create_supported_name_obj<Air>,
            "TwilightAir",
        },
        NameObjFactory::Name2CreateFunc{
            "BrightObj",
            create_supported_name_obj<BrightObj>,
            "LensFlare",
        },
        NameObjFactory::Name2CreateFunc{
            "PowerStar",
            create_supported_name_obj<PowerStar>,
            "PowerStar",
        },
        NameObjFactory::Name2CreateFunc{
            "GrandStar",
            create_supported_name_obj<PowerStar>,
            "GrandStar",
        },
        NameObjFactory::Name2CreateFunc{
            "PowerStarAppearPoint",
            create_supported_name_obj<PowerStarAppearPoint>,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "BrightSun",
            create_supported_name_obj<BrightSun>,
            "LensFlare",
        },
        NameObjFactory::Name2CreateFunc{
            "CollisionBlocker",
            create_supported_name_obj<CollisionBlocker>,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "GhostShipCavePipeCollision",
            create_supported_name_obj<InvisiblePolygonObj>,
            "GhostShipCavePipeCollision",
        },
        NameObjFactory::Name2CreateFunc{
            "InvisibleWall10x10",
            create_supported_name_obj<InvisiblePolygonObj>,
            "InvisibleWall10x10",
        },
        NameObjFactory::Name2CreateFunc{
            "InvisibleWall10x20",
            create_supported_name_obj<InvisiblePolygonObj>,
            "InvisibleWall10x20",
        },
        NameObjFactory::Name2CreateFunc{
            "InvisibleWallJump10x10",
            create_supported_name_obj<InvisiblePolygonObj>,
            "InvisibleWallJump10x10",
        },
        NameObjFactory::Name2CreateFunc{
            "InvisibleWallJump10x20",
            create_supported_name_obj<InvisiblePolygonObj>,
            "InvisibleWallJump10x20",
        },
        NameObjFactory::Name2CreateFunc{
            "InvisibleWallGCapture10x10",
            create_supported_name_obj<InvisiblePolygonObjGCapture>,
            "InvisibleWallGCapture10x10",
        },
        NameObjFactory::Name2CreateFunc{
            "InvisibleWallGCapture10x20",
            create_supported_name_obj<InvisiblePolygonObjGCapture>,
            "InvisibleWallGCapture10x20",
        },
        NameObjFactory::Name2CreateFunc{
            "PolygonCodeRecoveryPlate",
            create_supported_name_obj<InvisiblePolygonObj>,
            "PolygonCodeRecoveryPlate",
        },
        NameObjFactory::Name2CreateFunc{
            "PolygonCodeRecoveryBowl",
            create_supported_name_obj<InvisiblePolygonObj>,
            "PolygonCodeRecoveryBowl",
        },
        NameObjFactory::Name2CreateFunc{
            "DemoGroup",
            create_supported_name_obj<DemoExecutor>,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "FlowerGroup",
            create_supported_name_obj<PlantGroup>,
            "Flower",
        },
        NameObjFactory::Name2CreateFunc{
            "FlowerBlueGroup",
            create_supported_name_obj<PlantGroup>,
            "FlowerBlue",
        },
        NameObjFactory::Name2CreateFunc{
            "CutBushGroup",
            create_supported_name_obj<PlantGroup>,
            "CutBush",
        },
        NameObjFactory::Name2CreateFunc{
            "WarpPod",
            create_supported_name_obj<WarpPod>,
            "WarpPod",
        },
        NameObjFactory::Name2CreateFunc{
            "EarthenPipe",
            create_supported_name_obj<EarthenPipe>,
            "EarthenPipe",
        },
        NameObjFactory::Name2CreateFunc{
            "EarthenPipeInWater",
            create_supported_name_obj<EarthenPipe>,
            "EarthenPipe",
        },
        NameObjFactory::Name2CreateFunc{
            "DemoSubGroup",
            create_supported_name_obj<DemoCastSubGroup>,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "HeavensDoorAppearStepA",
            create_supported_name_obj<HeavensDoorDemoObj>,
            "HeavensDoorAppearStepA",
        },
        NameObjFactory::Name2CreateFunc{
            "HeavensDoorAppearStepAAfter",
            create_supported_name_obj<SimpleMapObj>,
            "HeavensDoorAppearStepAAfter",
        },
        NameObjFactory::Name2CreateFunc{
            "HeavensDoorHouseDoor",
            create_supported_name_obj<SimpleMapObj>,
            "HeavensDoorHouseDoor",
        },
        NameObjFactory::Name2CreateFunc{
            "HeavensDoorFlowerA",
            create_supported_name_obj<SimpleMapObjNoSilhouetted>,
            "HeavensDoorFlowerA",
        },
        NameObjFactory::Name2CreateFunc{
            "HeavensDoorInsideCage",
            create_supported_name_obj<HeavensDoorDemoObj>,
            "HeavensDoorInsideCage",
        },
        NameObjFactory::Name2CreateFunc{
            "HeavensDoorInsidePlanetPartsA",
            create_supported_name_obj<HeavensDoorDemoObj>,
            "HeavensDoorInsidePlanetPartsA",
        },
        NameObjFactory::Name2CreateFunc{
            "Rosetta",
            create_supported_name_obj<Rosetta>,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "Butterfly",
            create_supported_name_obj<Butterfly>,
            "Butterfly",
        },
        NameObjFactory::Name2CreateFunc{
            "PunchingKinoko",
            create_supported_name_obj<PunchingKinoko>,
            "PunchingKinoko",
        },
        NameObjFactory::Name2CreateFunc{
            "RunawayRabbitCollect",
            create_supported_name_obj<RunawayRabbitCollect>,
            "TrickRabbit",
        },
        NameObjFactory::Name2CreateFunc{
            "DemoRabbit",
            create_supported_name_obj<DemoRabbit>,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "Tico",
            create_supported_name_obj<Tico>,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "TicoBaby",
            create_supported_name_obj<Tico>,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "GlobalCubeGravity",
            MR::createGlobalCubeGravityObj,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "GlobalConeGravity",
            MR::createGlobalConeGravityObj,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "GlobalDiskGravity",
            MR::createGlobalDiskGravityObj,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "GlobalDiskTorusGravity",
            MR::createGlobalDiskTorusGravityObj,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "GlobalPlaneGravity",
            MR::createGlobalPlaneGravityObj,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "GlobalPlaneGravityInBox",
            MR::createGlobalPlaneInBoxGravityObj,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "GlobalPlaneGravityInCylinder",
            MR::createGlobalPlaneInCylinderGravityObj,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "GlobalPointGravity",
            MR::createGlobalPointGravityObj,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "GlobalSegmentGravity",
            MR::createGlobalSegmentGravityObj,
            nullptr,
        },
        NameObjFactory::Name2CreateFunc{
            "GlobalWireGravity",
            MR::createGlobalWireGravityObj,
            nullptr,
        },
    };

    struct UnavailableCreatorRecord {
        std::string_view object_name;
        std::string_view reason;
    };

    constexpr auto cUnavailableCreatorTable = std::array{
        UnavailableCreatorRecord{
            "AstroDomeSky",
            "exact_astro_dome_sky_actor_and_arg_archive_runtime_unavailable",
        },
        UnavailableCreatorRecord{"FileSelector", "retail_file_select_actor_runtime_unavailable"},
        UnavailableCreatorRecord{
            "SphereSelectorHandle",
            "me_and_multi_stage_bgm_playback_runtime_unavailable",
        },
        UnavailableCreatorRecord{"RailCoin", "shadow_area_and_mercator_runtime_unavailable"},
        UnavailableCreatorRecord{"PurpleRailCoin", "shadow_area_and_mercator_runtime_unavailable"},
        UnavailableCreatorRecord{"PurpleCoinStarter", "event_power_star_and_scene_layout_runtime_unavailable"},
    };

    struct OriginalArchiveRecord {
        std::string_view object_name;
        std::string_view archive_name;
    };

    constexpr OriginalArchiveRecord cOriginalArchiveRecords[] = {
#include "NameObjArchiveTable.inc"
    };

    // This is the supported subset of retail cName2MakeArchiveListFuncTable.
    // Keeping placement-dependent archive callbacks beside the creator subset
    // makes actor construction and its complete preload description one
    // atomic compatibility capability.
    constexpr auto cSupportedMakeArchiveListFuncTable = std::array{
        NameObjFactory::Name2MakeArchiveListFunc{
            "CrystalCageM",
            MR::makeArchiveListDummyDisplayModel,
        },
        NameObjFactory::Name2MakeArchiveListFunc{
            "Coin",
            Coin::makeArchiveList,
        },
        NameObjFactory::Name2MakeArchiveListFunc{
            "PurpleCoin",
            Coin::makeArchiveList,
        },
        NameObjFactory::Name2MakeArchiveListFunc{
            "PowerStar",
            PowerStar::makeArchiveList,
        },
        NameObjFactory::Name2MakeArchiveListFunc{
            "Rosetta",
            Rosetta::makeArchiveList,
        },
        NameObjFactory::Name2MakeArchiveListFunc{
            "DemoRabbit",
            DemoRabbit::makeArchiveList,
        },
        NameObjFactory::Name2MakeArchiveListFunc{
            "RunawayTico",
            RunawayTico::makeArchiveList,
        },
        NameObjFactory::Name2MakeArchiveListFunc{
            "Tico",
            Tico::makeArchiveList,
        },
    };

    constexpr auto cPlayerArchiveLoaderObjTable = std::array<std::string_view, 8>{
        "Hopper",
        "BenefitItemInvincible",
        "MorphItemNeoBee",
        "MorphItemNeoFire",
        "MorphItemNeoFoo",
        "MorphItemNeoHopper",
        "MorphItemNeoIce",
        "MorphItemNeoTeresa",
    };

    [[nodiscard]] bool equal_string_case(std::string_view lhs, std::string_view rhs) {
        return lhs.size() == rhs.size() && std::ranges::equal(lhs, rhs, [](char left, char right) {
                   return std::tolower(static_cast<unsigned char>(left)) ==
                          std::tolower(static_cast<unsigned char>(right));
               });
    }

    [[nodiscard]] const NameObjFactory::Name2CreateFunc *find_supported_entry(std::string_view object_name) {
        const auto found = std::ranges::find_if(cSupportedCreateTable, [&](const auto &entry) {
            return equal_string_case(entry.mName, object_name);
        });
        return found != cSupportedCreateTable.end() ? &*found : nullptr;
    }

    [[nodiscard]] const std::vector<NameObjFactory::Name2CreateFunc> &area_obj_create_table() {
        static const auto table = [] {
            // This native catalog outlives every original Game heap, including
            // whichever scene first asks the factory for an area creator.
            const aurora::allocation::HostAllocationScope host;
            auto result = std::vector<NameObjFactory::Name2CreateFunc>{};
            const auto descriptors = smgpc::scene::complete_area_obj_placement_descriptors();
            result.reserve(descriptors.size());
            for (const auto &descriptor : descriptors) {
                result.push_back(NameObjFactory::Name2CreateFunc{
                    descriptor.object_name.data(),
                    descriptor.object_creator,
                    nullptr,
                });
            }
            return result;
        }();
        return table;
    }

    [[nodiscard]] const NameObjFactory::Name2CreateFunc *find_area_obj_entry(std::string_view object_name) {
        const auto &table = area_obj_create_table();
        const auto found = std::ranges::find_if(table, [&](const auto &entry) {
            return equal_string_case(entry.mName, object_name);
        });
        return found != table.end() ? &*found : nullptr;
    }

    [[nodiscard]] const UnavailableCreatorRecord *find_unavailable_entry(std::string_view object_name) {
        const auto found = std::ranges::find_if(cUnavailableCreatorTable, [&](const auto &entry) {
            return equal_string_case(entry.object_name, object_name);
        });
        return found != cUnavailableCreatorTable.end() ? &*found : nullptr;
    }

    [[nodiscard]] const smgpc::scene::nameobj::PlanetMapCatalogEntry *
    find_planet_map_entry(std::string_view object_name) {
        const auto *catalog = smgpc::scene::nameobj::PlanetMapCatalog::active();
        return catalog != nullptr ? catalog->find(object_name) : nullptr;
    }

    [[nodiscard]] std::string_view planet_map_support_reason(
        const smgpc::scene::nameobj::PlanetMapCatalogEntry &entry) {
        using Kind = smgpc::scene::nameobj::PlanetMapCatalogCreatorKind;
        switch (entry.creator_kind) {
        case Kind::OrdinaryPlanetMap:
            return "compiled_retail_planet_map_creator";
        case Kind::ForceLowRuntimeUnavailable:
            return "planet_force_low_creator_runtime_unavailable";
        case Kind::UniqueCreator:
            return planet_map_creator(entry) != nullptr ? "compiled_retail_unique_planet_creator" :
                                                         "planet_unique_creator_runtime_unavailable";
        }
        return "planet_creator_runtime_unavailable";
    }

    [[nodiscard]] smgpc::scene::nameobj::NameObjCreatorSupport describe_creator_support(std::string_view object_name) {
        using smgpc::scene::nameobj::NameObjCreatorSupport;
        using smgpc::scene::nameobj::NameObjCreatorSupportKind;

        if (const auto *planet = find_planet_map_entry(object_name); planet != nullptr) {
            const auto supported = planet_map_creator(*planet) != nullptr;
            return NameObjCreatorSupport{
                .kind = supported ? NameObjCreatorSupportKind::Supported :
                                    NameObjCreatorSupportKind::RuntimeClosureUnavailable,
                .reason = std::string(planet_map_support_reason(*planet)),
            };
        }
        if (find_supported_entry(object_name) != nullptr) {
            return NameObjCreatorSupport{
                .kind = NameObjCreatorSupportKind::Supported,
                .reason = "compiled_retail_creator",
            };
        }
        if (find_area_obj_entry(object_name) != nullptr) {
            return NameObjCreatorSupport{
                .kind = NameObjCreatorSupportKind::Supported,
                .reason = "compiled_retail_area_creator_and_manager",
            };
        }
        if (const auto *unavailable = find_unavailable_entry(object_name); unavailable != nullptr) {
            return NameObjCreatorSupport{
                .kind = NameObjCreatorSupportKind::RuntimeClosureUnavailable,
                .reason = std::string(unavailable->reason),
            };
        }
        return NameObjCreatorSupport{
            .kind = NameObjCreatorSupportKind::NotLinked,
            .reason = "retail_creator_not_linked",
        };
    }

    [[nodiscard]] std::filesystem::path disc_relative(const std::filesystem::path &root,
                                                      const std::filesystem::path &path) {
        std::error_code error{};
        auto relative = std::filesystem::relative(path, root, error);
        if (!error && !relative.empty()) {
            return relative;
        }
        return path.filename();
    }

    [[nodiscard]] smgpc::scene::nameobj::NameObjArchiveRequest describe_archive(
        smgpc::runtime::DvdFileSystemService &dvd, std::string_view archive_name) {
        using smgpc::scene::nameobj::NameObjArchiveKind;
        using smgpc::scene::nameobj::NameObjArchiveRequest;

        auto request = NameObjArchiveRequest{
            .archive_name = std::string(archive_name),
            .disc_path = "",
            .resolved_path = "",
            .kind = NameObjArchiveKind::Missing,
            .loaded = false,
        };

        auto archive_path = dvd.find_object_archive(archive_name);
        if (archive_path.has_value()) {
            request.kind = NameObjArchiveKind::Object;
        } else {
            archive_path = dvd.find_layout_archive(archive_name);
            if (archive_path.has_value()) {
                request.kind = NameObjArchiveKind::Layout;
            }
        }

        if (!archive_path.has_value()) {
            return request;
        }

        request.resolved_path = archive_path->generic_string();
        request.disc_path = disc_relative(dvd.root(), *archive_path).generic_string();
        request.loaded = dvd.archive_load_count_for_path(*archive_path) > 0U;
        return request;
    }

    void add_archive_request(std::vector<smgpc::scene::nameobj::NameObjArchiveRequest> &requests,
                             smgpc::scene::nameobj::NameObjArchiveRequest request) {
        const auto duplicate = std::ranges::find_if(requests, [&](const auto &existing) {
            return existing.archive_name == request.archive_name && existing.kind == request.kind &&
                   existing.resolved_path == request.resolved_path;
        });
        if (duplicate == requests.end()) {
            requests.push_back(std::move(request));
        }
    }

    [[nodiscard]] bool contains_path_component(std::string_view path, std::string_view component) {
        return path.find(component) != std::string_view::npos;
    }

    [[nodiscard]] bool is_proven_non_actor_helper_table(std::string_view table_path) {
        // StageObjInfo rows describe zone composition and DemoObjInfo rows feed
        // the separate demo-sheet loader. Actor-bearing AreaObjInfo,
        // CameraCubeInfo, and PlanetObjInfo must pass through the factory
        // boundary and remain blocked until their real owners exist.
        return contains_path_component(table_path, "/stageobjinfo") ||
               contains_path_component(table_path, "/demoobjinfo");
    }

}  // namespace

namespace NameObjFactory {

    CreatorFuncPtr getCreator(const char *pName) {
        const auto name = pName != nullptr ? std::string_view(pName) : std::string_view{};
        if (const auto *planet = find_planet_map_entry(name)) {
            return planet_map_creator(*planet);
        }
        if (const auto *entry = find_supported_entry(name); entry != nullptr) {
            return entry->mCreateFunc;
        }
        if (const auto *area_entry = find_area_obj_entry(name); area_entry != nullptr) {
            return area_entry->mCreateFunc;
        }
        return nullptr;
    }

    void requestMountObjectArchives(const char *pName, const JMapInfoIter &rIter) {
        auto archive_list = NameObjArchiveListCollector{};
        getMountObjectArchiveList(&archive_list, pName, rIter);
        for (auto index = s32{}; index < archive_list.getArchiveNum(); ++index) {
            MR::mountAsyncArchiveByObjectOrLayoutName(archive_list.getArchive(index), nullptr);
        }
    }

    bool isReadResourceFromDVD(const char *pName, const JMapInfoIter &rIter) {
        auto archive_list = NameObjArchiveListCollector{};
        getMountObjectArchiveList(&archive_list, pName, rIter);
        for (auto index = s32{}; index < archive_list.getArchiveNum(); ++index) {
            if (!MR::isLoadedObjectOrLayoutArchive(archive_list.getArchive(index))) {
                return true;
            }
        }
        return false;
    }

    bool isPlayerArchiveLoaderObj(const char *pArchive) {
        if (pArchive == nullptr) {
            return false;
        }
        return std::ranges::any_of(cPlayerArchiveLoaderObjTable, [&](std::string_view entry) {
            return equal_string_case(entry, pArchive);
        });
    }

    const Name2CreateFunc *getName2CreateFunc(const char *pName, const Name2CreateFunc *pTable) {
        if (pTable != nullptr && pTable != cSupportedCreateTable.data()) {
            aurora::throw_host_exception<std::invalid_argument>(
                "External NameObj creator tables are unavailable without an explicit table extent.");
        }
        const auto name = pName != nullptr ? std::string_view(pName) : std::string_view{};
        if (const auto *entry = find_supported_entry(name); entry != nullptr || pTable != nullptr) {
            return entry;
        }
        return find_area_obj_entry(name);
    }

    void getMountObjectArchiveList(NameObjArchiveListCollector *pArchiveList, const char *pName,
                                   const JMapInfoIter &rIter) {
        if (pArchiveList == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("NameObj archive collection requires a real collector.");
        }

        const auto object_name = pName != nullptr ? std::string_view(pName) : std::string_view{};
        const auto *creator_entry = find_supported_entry(object_name);
        const auto *planet_entry = find_planet_map_entry(object_name);
        // Retail archive description is independent of creator availability:
        // PlacementInfoOrdered ranks every SameIdSet before creator lookup.
        // Retain catalog/static metadata for blocked groups so they remain
        // exact sort participants even when this PC tranche cannot create
        // their actors yet.
        if (planet_entry != nullptr) {
            const auto *catalog = smgpc::scene::nameobj::PlanetMapCatalog::active();
            if (catalog != nullptr) {
                for (const auto &archive_name :
                     catalog->archive_names(*planet_entry)) {
                    pArchiveList->addArchive(archive_name.c_str());
                }
            }
            return;
        }

        if (creator_entry != nullptr && creator_entry->mArchiveName != nullptr) {
            pArchiveList->addArchive(creator_entry->mArchiveName);
        }
        for (const auto &archive : cOriginalArchiveRecords) {
            if (archive.object_name != object_name) {
                continue;
            }
            pArchiveList->addArchive(archive.archive_name.data());
        }
        // Retail archive callbacks also cover children constructed directly by
        // their parent, for which the placement creator table has no entry.
        for (const auto &callback : cSupportedMakeArchiveListFuncTable) {
            if (callback.mName != object_name) {
                continue;
            }
            callback.mArchiveFunc(pArchiveList, rIter);
        }
    }

}  // namespace NameObjFactory

namespace MR {

    CreatorFuncPtr getModelChangableObjCreator(const char *pName) {
        const auto *entry = find_model_changing_entry(pName);
        if (entry == nullptr) {
            return nullptr;
        }
        if (entry->mCreatorFunc == nullptr) {
            aurora::throw_host_exception<std::runtime_error>(
                "Retail model-changing creator requires an unavailable actor runtime: " + std::string(pName));
        }
        return entry->mCreatorFunc;
    }

    void requestMountModelChangableObjArchives(const char *pName, s32 modelNo) {
        char objectName[128];
        MR::getMapPartsObjectName(objectName, sizeof(objectName), pName, modelNo);
        MR::mountAsyncArchiveByObjectOrLayoutName(objectName, nullptr);
    }

    bool isReadResourceFromDVDAtModelChangableObj(const char *pName, s32 modelNo) {
        char objectName[128];
        char archiveName[128];
        MR::getMapPartsObjectName(objectName, sizeof(objectName), pName, modelNo);
        MR::makeObjectArchiveFileNameFromPrefix(archiveName, sizeof(archiveName), objectName, true);
        return !MR::isLoadedFile(archiveName);
    }

}  // namespace MR

namespace smgpc::scene::nameobj {

    bool original_name_obj_registered(std::string_view object_name) {
        return find_planet_map_entry(object_name) != nullptr ||
               std::ranges::any_of(cOriginalNames, [&](auto name) {
                   return equal_string_case(name, object_name);
               });
    }

    NameObjCreatorSupport describe_model_changing_creator_support(std::string_view object_name) {
        const auto *entry = find_model_changing_entry(object_name);
        if (entry == nullptr) {
            return {.kind = NameObjCreatorSupportKind::NotLinked,
                    .reason = "unknown_retail_model_changing_creator"};
        }
        return entry->mCreatorFunc != nullptr
                   ? NameObjCreatorSupport{.kind = NameObjCreatorSupportKind::Supported,
                                           .reason = "compiled_retail_model_changing_creator"}
                   : NameObjCreatorSupport{.kind = NameObjCreatorSupportKind::RuntimeClosureUnavailable,
                                           .reason = "original_model_changing_actor_runtime_unavailable"};
    }

    bool can_create_name_obj(std::string_view object_name) {
        if (const auto *planet = find_planet_map_entry(object_name)) {
            return planet_map_creator(*planet) != nullptr;
        }
        return find_supported_entry(object_name) != nullptr || find_area_obj_entry(object_name) != nullptr;
    }

    NameObjSceneVisualKind scene_visual_kind(std::string_view object_name) {
        if (const auto *planet = find_planet_map_entry(object_name)) {
            return planet_map_creator(*planet) != nullptr ? NameObjSceneVisualKind::Planet :
                                                           NameObjSceneVisualKind::None;
        }
        const auto *entry = find_supported_entry(object_name);
        if (entry == nullptr) {
            return NameObjSceneVisualKind::None;
        }

        const auto creator = entry->mCreateFunc;
        if (creator == create_supported_name_obj<Sky> ||
            creator == create_supported_name_obj<ProjectionMapSky>) {
            return NameObjSceneVisualKind::Sky;
        }
        if (creator == create_supported_name_obj<Air> ||
            creator == create_supported_name_obj<AirFar100m> ||
            creator == create_supported_name_obj<ProjectionMapAir> ||
            creator == create_supported_name_obj<PriorDrawAir>) {
            return NameObjSceneVisualKind::Air;
        }
        if (creator == create_supported_name_obj<BrightObj> ||
            creator == create_supported_name_obj<BrightSun>) {
            return NameObjSceneVisualKind::Bright;
        }
        return NameObjSceneVisualKind::None;
    }

    NameObjCreatorSupport describe_name_obj_creator_support(std::string_view object_name) {
        return describe_creator_support(object_name);
    }

    NameObjPlacementSupport describe_name_obj_placement_support(smgpc::runtime::DvdFileSystemService &,
                                                                std::string_view object_name,
                                                                std::string_view table_path) {
        if (is_proven_non_actor_helper_table(table_path)) {
            return NameObjPlacementSupport{
                .kind = NameObjPlacementSupportKind::IntentionallyIgnored,
                .reason = "non_actor_helper_table",
            };
        }

        const auto creator_support = describe_creator_support(object_name);
        if (creator_support.kind == NameObjCreatorSupportKind::Supported) {
            return NameObjPlacementSupport{
                .kind = NameObjPlacementSupportKind::OriginalFactory,
                .reason = "original_factory",
            };
        }

        return NameObjPlacementSupport{
            .kind = NameObjPlacementSupportKind::Unsupported,
            .reason = creator_support.reason,
        };
    }

    std::vector<NameObjArchiveRequest> collect_name_obj_archive_requests(
        smgpc::runtime::DvdFileSystemService &dvd, std::string_view object_name,
        const JMapInfoIter *placement_iter) {
        auto requests = std::vector<NameObjArchiveRequest>{};
        if (!can_create_name_obj(object_name)) {
            return requests;
        }

        auto collector = NameObjArchiveListCollector{};
        const auto invalid_iter = JMapInfoIter{};
        const auto object = std::string(object_name);
        NameObjFactory::getMountObjectArchiveList(
            &collector, object.c_str(), placement_iter != nullptr ? *placement_iter : invalid_iter);
        for (auto index = s32{}; index < collector.getArchiveNum(); ++index) {
            add_archive_request(requests, describe_archive(dvd, collector.getArchive(index)));
        }
        return requests;
    }

    NameObjFactoryDescription describe_name_obj_factory(smgpc::runtime::DvdFileSystemService &dvd,
                                                        std::string_view object_name) {
        const auto creator_support = describe_creator_support(object_name);
        return NameObjFactoryDescription{
            .object_name = std::string(object_name),
            .creator_supported = creator_support.kind == NameObjCreatorSupportKind::Supported,
            .creator_support = creator_support,
            .archives = collect_name_obj_archive_requests(dvd, object_name, nullptr),
        };
    }

    std::vector<NameObjArchiveRequest> preload_name_obj_archives(
        smgpc::runtime::DvdFileSystemService &dvd, std::string_view object_name,
        const JMapInfoIter *placement_iter) {
        auto requests = collect_name_obj_archive_requests(dvd, object_name, placement_iter);
        for (auto &request : requests) {
            if (request.kind == NameObjArchiveKind::Missing || request.resolved_path.empty()) {
                aurora::throw_host_exception<std::runtime_error>("Required retail archive is unavailable for " +
                                         std::string(object_name) + ": " + request.archive_name);
            }
            auto &archive = dvd.archive_for_path(request.resolved_path);
            (void)archive;
            request.loaded = true;
        }
        return requests;
    }

    static std::unique_ptr<NameObj> create_with_creator(std::string_view object_name, const char *actor_name,
                                                         CreatorFuncPtr creator) {
        const auto object = std::string(object_name);
        if (creator == nullptr) {
            const auto support = describe_creator_support(object_name);
            aurora::throw_host_exception<std::runtime_error>("Unsupported NameObj factory request: " + object +
                                     " (" + support.reason + ")");
        }

        // Game constructors borrow their names, including aliases retained by
        // child objects. Host-generated labels need storage before construction.
        auto retained_name = std::shared_ptr<const std::string>{};
        if (actor_name != nullptr) {
            const aurora::allocation::HostAllocationScope host;
            retained_name = std::make_shared<const std::string>(actor_name);
        }
        auto result = std::unique_ptr<NameObj>(creator(
            retained_name != nullptr ? retained_name->c_str() : nullptr));
        if (result == nullptr) {
            aurora::throw_host_exception<std::runtime_error>("Retail NameObj creator returned null: " + object);
        }
        if (retained_name != nullptr) {
            smgpc::compat::retain_name_obj_host_name(result.get(), std::move(retained_name));
        }
        if (auto *gravity = dynamic_cast<GlobalGravityObj *>(result.get());
            gravity != nullptr) {
            smgpc::compat::adopt_global_gravity_children(*gravity);
        }
        return result;
    }

    std::unique_ptr<NameObj> create_name_obj(smgpc::runtime::DvdFileSystemService &,
                                             std::string_view object_name, const char *actor_name) {
        const auto name = std::string(object_name);
        return create_with_creator(object_name, actor_name, NameObjFactory::getCreator(name.c_str()));
    }

    std::unique_ptr<NameObj> create_model_changing_name_obj(std::string_view object_name, const char *actor_name) {
        const auto name = std::string(object_name);
        const auto creator = MR::getModelChangableObjCreator(name.c_str());
        if (creator == nullptr) {
            aurora::throw_host_exception<std::runtime_error>("Unknown retail model-changing creator: " + name);
        }
        return create_with_creator(object_name, actor_name, creator);
    }

}  // namespace smgpc::scene::nameobj
