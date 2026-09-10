#include <aurora/exception.hpp>
#include "scene/AreaObjRuntime.hpp"

#include "Game/AreaObj/AreaForm.hpp"
#include "Game/AreaObj/AreaObj.hpp"
#include "Game/AreaObj/WaterArea.hpp"
#include "Game/AreaObj/WarpCube.hpp"
#include "Game/AreaObj/ImageEffectArea.hpp"
#include "Game/AreaObj/BloomArea.hpp"
#include "Game/AreaObj/SimpleBloomArea.hpp"
#include "Game/AreaObj/ScreenBlurArea.hpp"
#include "Game/AreaObj/DepthOfFieldArea.hpp"

#include "Game/AreaObj/CameraRepulsiveArea.hpp"
#include "Game/AreaObj/CubeCamera.hpp"
#include "Game/AreaObj/LightArea.hpp"
#include "Game/AreaObj/LightAreaHolder.hpp"
#include "Game/AreaObj/MessageArea.hpp"
#include "Game/AreaObj/SwitchArea.hpp"
#include "Game/Map/LightFunction.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "scene/SceneObjHolderRuntime.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <stdexcept>
#include <string>

namespace smgpc::scene {
    namespace {

        template <typename T, AreaForm::Type FormType>
        [[nodiscard]] NameObj *create_area_obj(const char *name) {
            return new T(FormType, name);
        }

        template <typename T>
        [[nodiscard]] NameObj *create_named_area_obj(const char *name) {
            return new T(name);
        }

        [[nodiscard]] AreaObjMgr *create_area_obj_manager(s32 capacity, const char *name) {
            return new AreaObjMgr(capacity, name);
        }

        [[nodiscard]] AreaObjMgr *create_cube_camera_manager(s32 capacity, const char *name) {
            return new CubeCameraMgr(capacity, name);
        }

        [[nodiscard]] AreaObjMgr *create_light_area_manager(s32 capacity, const char *name) {
            return new LightAreaHolder(capacity, name);
        }

        [[nodiscard]] AreaObjMgr *create_water_manager(s32 capacity, const char *name) {
            return new WaterAreaMgr(capacity, name);
        }

        [[nodiscard]] AreaObjMgr *create_warp_cube_manager(s32 capacity, const char *name) {
            return new WarpCubeMgr(capacity, name);
        }

        [[nodiscard]] AreaObjMgr *create_image_effect_manager(s32 capacity, const char *name) {
            return new ImageEffectAreaMgr(capacity, name);
        }

        void finalize_cube_camera_manager(AreaObjMgr &manager) {
            auto *camera_manager = dynamic_cast<CubeCameraMgr *>(&manager);
            if (camera_manager == nullptr) {
                aurora::throw_host_exception<std::logic_error>(
                    "CubeCamera descriptor did not construct its exact retail manager");
            }
            camera_manager->initAfterLoad();
        }

        // Retail constructs managers independently of placed area instances.
        // GlaringLightAreaMgr remains unavailable until its original type is linked.
        constexpr auto cCompleteAreaObjManagerDescriptors = std::array{
            AreaObjManagerDescriptor{"SwitchArea", 0, 0x40, create_area_obj_manager},
            AreaObjManagerDescriptor{"RaceJudgeCube", 1, 0x40, create_area_obj_manager},
            AreaObjManagerDescriptor{"NinForceWindCube", 2, 0x40, create_area_obj_manager},
            AreaObjManagerDescriptor{"NinAbyssCube", 3, 0x40, create_area_obj_manager},
            AreaObjManagerDescriptor{"CubeCamera", 4, 0xA0, create_cube_camera_manager, finalize_cube_camera_manager},
            AreaObjManagerDescriptor{"BindEndCube", 5, 0x40, create_area_obj_manager},
            AreaObjManagerDescriptor{"EffectCylinder", 6, 0x40, create_area_obj_manager},
            AreaObjManagerDescriptor{"DeathArea", 7, 0x40, create_area_obj_manager},
            AreaObjManagerDescriptor{"WarpCube", 8, 0x40, create_warp_cube_manager},
            AreaObjManagerDescriptor{"TripodBossStepStart", 9, 0x40, create_area_obj_manager},
            AreaObjManagerDescriptor{"Water", 10, 0x40, create_water_manager},
            AreaObjManagerDescriptor{"PlaneModeCube", 11, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"PlaneCircularModeCube", 12, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"PipeModeCube", 13, 0x4, create_area_obj_manager},
            AreaObjManagerDescriptor{"TowerModeCylinder", 14, 0x4, create_area_obj_manager},
            AreaObjManagerDescriptor{"ShadeCube", 15, 0x40, create_area_obj_manager},
            AreaObjManagerDescriptor{"PullBackCube", 16, 0x40, create_area_obj_manager},
            AreaObjManagerDescriptor{"PullBackCylinder", 17, 0x40, create_area_obj_manager},
            AreaObjManagerDescriptor{"RestartCube", 18, 0x40, create_area_obj_manager},
            AreaObjManagerDescriptor{"ChangeBgmCube", 19, 0x20, create_area_obj_manager},
            AreaObjManagerDescriptor{"BgmProhibitArea", 20, 0x4, create_area_obj_manager},
            AreaObjManagerDescriptor{"SoundEmitterCube", 21, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"SoundEmitterSphere", 22, 0x8, create_area_obj_manager},
            AreaObjManagerDescriptor{"PlaneCollisionCube", 23, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"ForbidTriangleJumpCube", 24, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"ForbidWaterSearchCube", 25, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"QuakeEffectArea", 26, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"HazeCube", 27, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"AudioEffectArea", 28, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"BigBubbleGoalArea", 29, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"SunLightArea", 30, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"ViewGroupCtrlCube", 32, 0x40, create_area_obj_manager},
            AreaObjManagerDescriptor{"LensFlareArea", 33, 0x40, create_area_obj_manager},
            AreaObjManagerDescriptor{"CameraRepulsiveArea", 34, 0x80, create_area_obj_manager},
            AreaObjManagerDescriptor{"LightArea", 35, 0x80, create_light_area_manager},
            AreaObjManagerDescriptor{"FallsCube", 36, 0x20, create_area_obj_manager},
            AreaObjManagerDescriptor{"MercatorCube", 37, 0x1, create_area_obj_manager},
            AreaObjManagerDescriptor{"AstroChangeStageCube", 38, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"ImageEffectArea", 39, 0x20, create_image_effect_manager},
            AreaObjManagerDescriptor{"BlueStarGuidanceCube", 40, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"TicoSeedGuidanceCube", 41, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"MessageArea", 42, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"SmokeEffectColorArea", 43, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"BeeWallShortDistArea", 44, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"ExtraWallCheckArea", 45, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"ExtraWallCheckCylinder", 46, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"SpinGuidanceCube", 47, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"HipDropGuidanceCube", 48, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"TamakoroMoveGuidanceCube", 49, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"TamakoroJumpGuidanceCube", 50, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"BigBubbleGuidanceCube", 51, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"HeavySteeringCube", 52, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"NonSleepCube", 53, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"AreaMoveSphere", 54, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"DodoryuClosedCylinder", 55, 0x8, create_area_obj_manager},
            AreaObjManagerDescriptor{"DashChargeCylinder", 56, 0x8, create_area_obj_manager},
            AreaObjManagerDescriptor{"PlayerSeArea", 57, 0x8, create_area_obj_manager},
            AreaObjManagerDescriptor{"RasterScrollCube", 58, 0x8, create_area_obj_manager},
            AreaObjManagerDescriptor{"OnimasuCube", 59, 0x20, create_area_obj_manager},
            AreaObjManagerDescriptor{"ForbidJumpCube", 60, 0x8, create_area_obj_manager},
            AreaObjManagerDescriptor{"CollisionArea", 61, 0x40, create_area_obj_manager},
            AreaObjManagerDescriptor{"AstroOverlookArea", 62, 0x8, create_area_obj_manager},
            AreaObjManagerDescriptor{"CelestrialSphere", 63, 0x4, create_area_obj_manager},
            AreaObjManagerDescriptor{"MirrorArea", 64, 0x10, create_area_obj_manager},
            AreaObjManagerDescriptor{"DarkMatterCube", 65, 0x40, create_area_obj_manager},
            AreaObjManagerDescriptor{"DarkMatterCylinder", 66, 0x20, create_area_obj_manager},
        };

        // Add an entry only after its exact actor init path and every manager
        // dependency are linked. The host factory consumes this same table, so
        // a manager by itself can never make a placement appear supported.
        constexpr auto cCompleteAreaObjPlacementDescriptors =
            std::array{
                AreaObjPlacementDescriptor{
                    .object_name = "SwitchCube",
                    .object_creator = create_area_obj<SwitchArea, AreaForm::Type_Cube2>,
                    .manager_name = "SwitchArea",
                    .retail_manager_order = 0,
                    .manager_capacity = 0x40,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "SwitchSphere",
                    .object_creator = create_area_obj<SwitchArea, AreaForm::Type_Sphere>,
                    .manager_name = "SwitchArea",
                    .retail_manager_order = 0,
                    .manager_capacity = 0x40,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "SwitchCylinder",
                    .object_creator = create_area_obj<SwitchArea, AreaForm::Type_Cylinder>,
                    .manager_name = "SwitchArea",
                    .retail_manager_order = 0,
                    .manager_capacity = 0x40,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "CubeCameraBox",
                    .object_creator = create_area_obj<CubeCameraArea, AreaForm::Type_Cube1>,
                    .manager_name = "CubeCamera",
                    .retail_manager_order = 4,
                    .manager_capacity = 0xA0,
                    .manager_creator = create_cube_camera_manager,
                    .manager_finalize = finalize_cube_camera_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "CubeCameraCylinder",
                    .object_creator = create_area_obj<CubeCameraArea, AreaForm::Type_Cylinder>,
                    .manager_name = "CubeCamera",
                    .retail_manager_order = 4,
                    .manager_capacity = 0xA0,
                    .manager_creator = create_cube_camera_manager,
                    .manager_finalize = finalize_cube_camera_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "CubeCameraSphere",
                    .object_creator = create_area_obj<CubeCameraArea, AreaForm::Type_Sphere>,
                    .manager_name = "CubeCamera",
                    .retail_manager_order = 4,
                    .manager_capacity = 0xA0,
                    .manager_creator = create_cube_camera_manager,
                    .manager_finalize = finalize_cube_camera_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "CubeCameraBowl",
                    .object_creator = create_area_obj<CubeCameraArea, AreaForm::Type_Bowl>,
                    .manager_name = "CubeCamera",
                    .retail_manager_order = 4,
                    .manager_capacity = 0xA0,
                    .manager_creator = create_cube_camera_manager,
                    .manager_finalize = finalize_cube_camera_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "BindEndCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube1>,
                    .manager_name = "BindEndCube",
                    .retail_manager_order = 5,
                    .manager_capacity = 0x40,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "EffectCylinder",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cylinder>,
                    .manager_name = "EffectCylinder",
                    .retail_manager_order = 6,
                    .manager_capacity = 0x40,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "WaterCube",
                    .object_creator = create_area_obj<WaterArea, AreaForm::Type_Cube2>,
                    .manager_name = "Water",
                    .retail_manager_order = 10,
                    .manager_capacity = 0x40,
                    .manager_creator = create_water_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "WaterSphere",
                    .object_creator = create_area_obj<WaterArea, AreaForm::Type_Sphere>,
                    .manager_name = "Water",
                    .retail_manager_order = 10,
                    .manager_capacity = 0x40,
                    .manager_creator = create_water_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "WaterCylinder",
                    .object_creator = create_area_obj<WaterArea, AreaForm::Type_Cylinder>,
                    .manager_name = "Water",
                    .retail_manager_order = 10,
                    .manager_capacity = 0x40,
                    .manager_creator = create_water_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "PlaneModeCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                    .manager_name = "PlaneModeCube",
                    .retail_manager_order = 11,
                    .manager_capacity = 0x10,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "PlaneCircularModeCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                    .manager_name = "PlaneCircularModeCube",
                    .retail_manager_order = 12,
                    .manager_capacity = 0x10,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "PipeModeCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                    .manager_name = "PipeModeCube",
                    .retail_manager_order = 13,
                    .manager_capacity = 0x4,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "TowerModeCylinder",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cylinder>,
                    .manager_name = "TowerModeCylinder",
                    .retail_manager_order = 14,
                    .manager_capacity = 0x4,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "PullBackCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                    .manager_name = "PullBackCube",
                    .retail_manager_order = 16,
                    .manager_capacity = 0x40,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "PullBackCylinder",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cylinder>,
                    .manager_name = "PullBackCylinder",
                    .retail_manager_order = 17,
                    .manager_capacity = 0x40,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "PlaneCollisionCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                    .manager_name = "PlaneCollisionCube",
                    .retail_manager_order = 23,
                    .manager_capacity = 0x10,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "ForbidTriangleJumpCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                    .manager_name = "ForbidTriangleJumpCube",
                    .retail_manager_order = 24,
                    .manager_capacity = 0x10,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "ForbidWaterSearchCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                    .manager_name = "ForbidWaterSearchCube",
                    .retail_manager_order = 25,
                    .manager_capacity = 0x10,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "ViewGroupCtrlCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                    .manager_name = "ViewGroupCtrlCube",
                    .retail_manager_order = 32,
                    .manager_capacity = 0x40,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "LensFlareArea",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                    .manager_name = "LensFlareArea",
                    .retail_manager_order = 33,
                    .manager_capacity = 0x40,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "CameraRepulsiveSphere",
                    .object_creator = create_named_area_obj<CameraRepulsiveSphere>,
                    .manager_name = "CameraRepulsiveArea",
                    .retail_manager_order = 34,
                    .manager_capacity = 0x80,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "CameraRepulsiveCylinder",
                    .object_creator = create_named_area_obj<CameraRepulsiveCylinder>,
                    .manager_name = "CameraRepulsiveArea",
                    .retail_manager_order = 34,
                    .manager_capacity = 0x80,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "LightCtrlCube",
                    .object_creator = create_area_obj<LightArea, AreaForm::Type_Cube2>,
                    .manager_name = "LightArea",
                    .retail_manager_order = 35,
                    .manager_capacity = 0x80,
                    .manager_creator = create_light_area_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "LightCtrlCylinder",
                    .object_creator = create_area_obj<LightArea, AreaForm::Type_Cylinder>,
                    .manager_name = "LightArea",
                    .retail_manager_order = 35,
                    .manager_capacity = 0x80,
                    .manager_creator = create_light_area_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "FallsCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                    .manager_name = "FallsCube",
                    .retail_manager_order = 36,
                    .manager_capacity = 0x20,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "BloomCube",
                    .object_creator = create_area_obj<BloomArea, AreaForm::Type_Cube1>,
                    .manager_name = "ImageEffectArea",
                    .retail_manager_order = 39,
                    .manager_capacity = 0x20,
                    .manager_creator = create_image_effect_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "BloomSphere",
                    .object_creator = create_area_obj<BloomArea, AreaForm::Type_Sphere>,
                    .manager_name = "ImageEffectArea",
                    .retail_manager_order = 39,
                    .manager_capacity = 0x20,
                    .manager_creator = create_image_effect_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "BloomCylinder",
                    .object_creator = create_area_obj<BloomArea, AreaForm::Type_Cylinder>,
                    .manager_name = "ImageEffectArea",
                    .retail_manager_order = 39,
                    .manager_capacity = 0x20,
                    .manager_creator = create_image_effect_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "SimpleBloomCube",
                    .object_creator = create_area_obj<SimpleBloomArea, AreaForm::Type_Cube1>,
                    .manager_name = "ImageEffectArea",
                    .retail_manager_order = 39,
                    .manager_capacity = 0x20,
                    .manager_creator = create_image_effect_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "SimpleBloomSphere",
                    .object_creator = create_area_obj<SimpleBloomArea, AreaForm::Type_Sphere>,
                    .manager_name = "ImageEffectArea",
                    .retail_manager_order = 39,
                    .manager_capacity = 0x20,
                    .manager_creator = create_image_effect_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "SimpleBloomCylinder",
                    .object_creator = create_area_obj<SimpleBloomArea, AreaForm::Type_Cylinder>,
                    .manager_name = "ImageEffectArea",
                    .retail_manager_order = 39,
                    .manager_capacity = 0x20,
                    .manager_creator = create_image_effect_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "ScreenBlurCube",
                    .object_creator = create_area_obj<ScreenBlurArea, AreaForm::Type_Cube1>,
                    .manager_name = "ImageEffectArea",
                    .retail_manager_order = 39,
                    .manager_capacity = 0x20,
                    .manager_creator = create_image_effect_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "ScreenBlurSphere",
                    .object_creator = create_area_obj<ScreenBlurArea, AreaForm::Type_Sphere>,
                    .manager_name = "ImageEffectArea",
                    .retail_manager_order = 39,
                    .manager_capacity = 0x20,
                    .manager_creator = create_image_effect_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "ScreenBlurCylinder",
                    .object_creator = create_area_obj<ScreenBlurArea, AreaForm::Type_Cylinder>,
                    .manager_name = "ImageEffectArea",
                    .retail_manager_order = 39,
                    .manager_capacity = 0x20,
                    .manager_creator = create_image_effect_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "DepthOfFieldCube",
                    .object_creator = create_area_obj<DepthOfFieldArea, AreaForm::Type_Cube1>,
                    .manager_name = "ImageEffectArea",
                    .retail_manager_order = 39,
                    .manager_capacity = 0x20,
                    .manager_creator = create_image_effect_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "DepthOfFieldSphere",
                    .object_creator = create_area_obj<DepthOfFieldArea, AreaForm::Type_Sphere>,
                    .manager_name = "ImageEffectArea",
                    .retail_manager_order = 39,
                    .manager_capacity = 0x20,
                    .manager_creator = create_image_effect_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "DepthOfFieldCylinder",
                    .object_creator = create_area_obj<DepthOfFieldArea, AreaForm::Type_Cylinder>,
                    .manager_name = "ImageEffectArea",
                    .retail_manager_order = 39,
                    .manager_capacity = 0x20,
                    .manager_creator = create_image_effect_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "BlueStarGuidanceCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                    .manager_name = "BlueStarGuidanceCube",
                    .retail_manager_order = 40,
                    .manager_capacity = 0x10,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "TicoSeedGuidanceCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                    .manager_name = "TicoSeedGuidanceCube",
                    .retail_manager_order = 41,
                    .manager_capacity = 0x10,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "MessageAreaCube",
                    .object_creator = create_area_obj<MessageArea, AreaForm::Type_Cube2>,
                    .manager_name = "MessageArea",
                    .retail_manager_order = 42,
                    .manager_capacity = 0x10,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "MessageAreaCylinder",
                    .object_creator = create_area_obj<MessageArea, AreaForm::Type_Cylinder>,
                    .manager_name = "MessageArea",
                    .retail_manager_order = 42,
                    .manager_capacity = 0x10,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "SmokeEffectColorAreaCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                    .manager_name = "SmokeEffectColorArea",
                    .retail_manager_order = 43,
                    .manager_capacity = 0x10,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "BeeWallShortDistAreaCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                    .manager_name = "BeeWallShortDistArea",
                    .retail_manager_order = 44,
                    .manager_capacity = 0x10,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "ExtraWallCheckArea",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                    .manager_name = "ExtraWallCheckArea",
                    .retail_manager_order = 45,
                    .manager_capacity = 0x10,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "ExtraWallCheckCylinder",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cylinder>,
                    .manager_name = "ExtraWallCheckCylinder",
                    .retail_manager_order = 46,
                    .manager_capacity = 0x10,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "HeavySteeringCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                    .manager_name = "HeavySteeringCube",
                    .retail_manager_order = 52,
                    .manager_capacity = 0x10,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "NonSleepCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                    .manager_name = "NonSleepCube",
                    .retail_manager_order = 53,
                    .manager_capacity = 0x10,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "AreaMoveSphere",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Sphere>,
                    .manager_name = "AreaMoveSphere",
                    .retail_manager_order = 54,
                    .manager_capacity = 0x10,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "DodoryuClosedCylinder",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cylinder>,
                    .manager_name = "DodoryuClosedCylinder",
                    .retail_manager_order = 55,
                    .manager_capacity = 0x8,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "DashChargeCylinder",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cylinder>,
                    .manager_name = "DashChargeCylinder",
                    .retail_manager_order = 56,
                    .manager_capacity = 0x8,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "RasterScrollCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                    .manager_name = "RasterScrollCube",
                    .retail_manager_order = 58,
                    .manager_capacity = 0x8,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "OnimasuCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube1>,
                    .manager_name = "OnimasuCube",
                    .retail_manager_order = 59,
                    .manager_capacity = 0x20,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "ForbidJumpCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                    .manager_name = "ForbidJumpCube",
                    .retail_manager_order = 60,
                    .manager_capacity = 0x8,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "AstroOverlookAreaCylinder",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cylinder>,
                    .manager_name = "AstroOverlookArea",
                    .retail_manager_order = 62,
                    .manager_capacity = 0x8,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "CelestrialSphere",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Sphere>,
                    .manager_name = "CelestrialSphere",
                    .retail_manager_order = 63,
                    .manager_capacity = 0x4,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "MirrorAreaCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                    .manager_name = "MirrorArea",
                    .retail_manager_order = 64,
                    .manager_capacity = 0x10,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "DarkMatterCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                    .manager_name = "DarkMatterCube",
                    .retail_manager_order = 65,
                    .manager_capacity = 0x40,
                    .manager_creator = create_area_obj_manager,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "DarkMatterCylinder",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cylinder>,
                    .manager_name = "DarkMatterCylinder",
                    .retail_manager_order = 66,
                    .manager_capacity = 0x20,
                    .manager_creator = create_area_obj_manager,
                },
            };

        [[nodiscard]] bool equal_string_case(std::string_view left, std::string_view right) noexcept {
            return left.size() == right.size() &&
                   std::ranges::equal(left, right, [](char left_character, char right_character) {
                       return std::tolower(static_cast<unsigned char>(left_character)) ==
                              std::tolower(static_cast<unsigned char>(right_character));
                   });
        }

        [[nodiscard]] std::string_view path_basename(std::string_view path) noexcept {
            const auto slash = path.find_last_of("/\\");
            return slash == std::string_view::npos ? path : path.substr(slash + 1U);
        }

    }  // namespace

    std::span<const AreaObjManagerDescriptor> complete_area_obj_manager_descriptors() noexcept {
        return cCompleteAreaObjManagerDescriptors;
    }

    std::span<const AreaObjPlacementDescriptor> complete_area_obj_placement_descriptors() noexcept {
        return cCompleteAreaObjPlacementDescriptors;
    }

    const AreaObjPlacementDescriptor *find_complete_area_obj_placement_descriptor(
        std::string_view object_name) noexcept {
        const auto descriptors = complete_area_obj_placement_descriptors();
        const auto found = std::ranges::find_if(descriptors, [&](const auto &descriptor) {
            return equal_string_case(descriptor.object_name, object_name);
        });
        if (found == descriptors.end() || found->object_creator == nullptr ||
            found->manager_creator == nullptr || found->manager_name.empty() ||
            found->retail_manager_order < 0 || found->manager_capacity <= 0) {
            return nullptr;
        }
        const auto managers = complete_area_obj_manager_descriptors();
        const auto manager = std::ranges::find(managers, found->manager_name, &AreaObjManagerDescriptor::name);
        if (manager == managers.end() || manager->retail_order != found->retail_manager_order ||
            manager->capacity != found->manager_capacity || manager->creator != found->manager_creator ||
            manager->finalize != found->manager_finalize) {
            return nullptr;
        }
        return &*found;
    }

    bool is_area_obj_placement_table(std::string_view table_path) noexcept {
        auto basename = path_basename(table_path);
        const auto extension = basename.find_last_of('.');
        if (extension != std::string_view::npos) {
            basename = basename.substr(0U, extension);
        }
        return equal_string_case(basename, "areaobjinfo");
    }

    bool placement_has_complete_area_obj_runtime(
        std::string_view object_name, std::string_view table_path,
        bool factory_supported) noexcept {
        if (!factory_supported) {
            return false;
        }
        return !is_area_obj_placement_table(table_path) ||
               find_complete_area_obj_placement_descriptor(object_name) != nullptr;
    }

    AreaObjMgr *find_area_obj_manager_by_retail_prefix(
        std::span<AreaObjMgr *const> managers,
        std::string_view requested_name) noexcept {
        const auto found = std::ranges::find_if(managers, [&](const auto *manager) {
            return manager != nullptr && manager->mName != nullptr &&
                   requested_name.starts_with(manager->mName);
        });
        return found != managers.end() ? *found : nullptr;
    }

    AreaObjRuntime::AreaObjRuntime() = default;

    AreaObjRuntime::~AreaObjRuntime() {
        for (const auto &owned : _owned_managers) {
            if (const auto *holder = dynamic_cast<const LightAreaHolder *>(owned.manager.get());
                holder != nullptr) {
                LightFunction::unregisterLightAreaHolder(holder);
            }
        }
    }

    AreaObjMgr *AreaObjRuntime::adopt_manager(
        std::unique_ptr<AreaObjMgr> manager,
        AreaObjManagerFinalize finalize) {
        if (manager == nullptr) {
            aurora::throw_host_exception<std::invalid_argument>("AreaObjRuntime cannot own a null retail manager");
        }
        if (_did_init_after_placement) {
            aurora::throw_host_exception<std::logic_error>("AreaObjRuntime cannot adopt a manager after the scene post-placement phase");
        }
        _owned_managers.reserve(_owned_managers.size() + 1U);
        auto *result = manager.get();
        smgpc::compat::claim_name_obj_runtime_ownership(result, this);
        _owned_managers.push_back(OwnedManager{
            .manager = std::move(manager),
            .finalize = finalize,
        });
        return result;
    }

    void AreaObjRuntime::adopt_managers(
        std::vector<std::unique_ptr<AreaObjMgr>> managers,
        std::vector<AreaObjManagerFinalize> finalizers) {
        if (_did_init_after_placement) {
            aurora::throw_host_exception<std::logic_error>("AreaObjRuntime cannot adopt managers after the scene post-placement phase");
        }
        if (std::ranges::any_of(managers, [](const auto &manager) { return manager == nullptr; })) {
            aurora::throw_host_exception<std::invalid_argument>("AreaObjRuntime cannot own a null retail manager");
        }
        if (finalizers.empty()) {
            finalizers.resize(managers.size(), nullptr);
        } else if (finalizers.size() != managers.size()) {
            aurora::throw_host_exception<std::invalid_argument>(
                "AreaObjRuntime manager finalizers must match the adopted manager count");
        }

        _owned_managers.reserve(_owned_managers.size() + managers.size());
        for (const auto &manager : managers) {
            smgpc::compat::claim_name_obj_runtime_ownership(
                manager.get(), this);
        }
        for (auto index = std::size_t{}; index < managers.size(); ++index) {
            _owned_managers.push_back(OwnedManager{
                .manager = std::move(managers[index]),
                .finalize = finalizers[index],
            });
        }
    }

    void AreaObjRuntime::acknowledge_scene_postpass(std::span<NameObj *const> objects) {
        for (auto &owned : _owned_managers) {
            if (std::ranges::find(objects, owned.manager.get()) == objects.end())
                continue;
            owned.did_init_after_placement = true;
            if (!owned.did_finalize) {
                if (owned.finalize)
                    owned.finalize(*owned.manager);
                owned.did_finalize = true;
            }
        }
        _did_init_after_placement = true;
    }

    void AreaObjRuntime::init_after_placement() {
        if (_did_init_after_placement) {
            return;
        }
        for (auto &owned : _owned_managers) {
            if (!owned.did_init_after_placement) {
                // Managers constructed synchronously by the real
                // AreaObjContainer participate in the SceneObjHolder's one
                // ordered NameObj postpass. Standalone test/runtime owners do
                // not, so retain the direct path for them without running a
                // holder-owned manager twice.
                if (!current_scene_obj_holder_binding_owns(
                        owned.manager.get()) &&
                    !smgpc::compat::
                        name_obj_runtime_postpass_is_delegated(
                            owned.manager.get())) {
                    owned.manager->initAfterPlacement();
                }
                owned.did_init_after_placement = true;
            }
        }
        for (auto &owned : _owned_managers) {
            if (!owned.did_finalize) {
                if (owned.finalize != nullptr) {
                    owned.finalize(*owned.manager);
                }
                owned.did_finalize = true;
            }
        }
        _did_init_after_placement = true;
    }

}  // namespace smgpc::scene
