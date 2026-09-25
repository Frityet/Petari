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
#include "Game/AreaObj/CollisionArea.hpp"
#include "Game/AreaObj/CubeCamera.hpp"
#include "Game/AreaObj/LightArea.hpp"
#include "Game/AreaObj/MessageArea.hpp"
#include "Game/AreaObj/RestartCube.hpp"
#include "Game/AreaObj/SwitchArea.hpp"

#include <algorithm>
#include <array>
#include <cctype>
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

        constexpr auto cCompleteAreaObjPlacementDescriptors =
            std::array{
                AreaObjPlacementDescriptor{
                    .object_name = "SwitchCube",
                    .object_creator = create_area_obj<SwitchArea, AreaForm::Type_Cube2>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "SwitchSphere",
                    .object_creator = create_area_obj<SwitchArea, AreaForm::Type_Sphere>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "SwitchCylinder",
                    .object_creator = create_area_obj<SwitchArea, AreaForm::Type_Cylinder>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "CubeCameraBox",
                    .object_creator = create_area_obj<CubeCameraArea, AreaForm::Type_Cube1>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "CubeCameraCylinder",
                    .object_creator = create_area_obj<CubeCameraArea, AreaForm::Type_Cylinder>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "CubeCameraSphere",
                    .object_creator = create_area_obj<CubeCameraArea, AreaForm::Type_Sphere>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "CubeCameraBowl",
                    .object_creator = create_area_obj<CubeCameraArea, AreaForm::Type_Bowl>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "BindEndCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube1>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "EffectCylinder",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cylinder>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "WaterCube",
                    .object_creator = create_area_obj<WaterArea, AreaForm::Type_Cube2>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "WaterSphere",
                    .object_creator = create_area_obj<WaterArea, AreaForm::Type_Sphere>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "WaterCylinder",
                    .object_creator = create_area_obj<WaterArea, AreaForm::Type_Cylinder>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "PlaneModeCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "PlaneCircularModeCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "PipeModeCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "TowerModeCylinder",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cylinder>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "PullBackCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "PullBackCylinder",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cylinder>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "RestartCube",
                    .object_creator = create_area_obj<RestartCube, AreaForm::Type_Cube2>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "PlaneCollisionCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "ForbidTriangleJumpCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "ForbidWaterSearchCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "ViewGroupCtrlCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "LensFlareArea",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "CameraRepulsiveSphere",
                    .object_creator = create_named_area_obj<CameraRepulsiveSphere>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "CameraRepulsiveCylinder",
                    .object_creator = create_named_area_obj<CameraRepulsiveCylinder>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "LightCtrlCube",
                    .object_creator = create_area_obj<LightArea, AreaForm::Type_Cube2>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "LightCtrlCylinder",
                    .object_creator = create_area_obj<LightArea, AreaForm::Type_Cylinder>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "FallsCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "BloomCube",
                    .object_creator = create_area_obj<BloomArea, AreaForm::Type_Cube1>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "BloomSphere",
                    .object_creator = create_area_obj<BloomArea, AreaForm::Type_Sphere>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "BloomCylinder",
                    .object_creator = create_area_obj<BloomArea, AreaForm::Type_Cylinder>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "SimpleBloomCube",
                    .object_creator = create_area_obj<SimpleBloomArea, AreaForm::Type_Cube1>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "SimpleBloomSphere",
                    .object_creator = create_area_obj<SimpleBloomArea, AreaForm::Type_Sphere>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "SimpleBloomCylinder",
                    .object_creator = create_area_obj<SimpleBloomArea, AreaForm::Type_Cylinder>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "ScreenBlurCube",
                    .object_creator = create_area_obj<ScreenBlurArea, AreaForm::Type_Cube1>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "ScreenBlurSphere",
                    .object_creator = create_area_obj<ScreenBlurArea, AreaForm::Type_Sphere>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "ScreenBlurCylinder",
                    .object_creator = create_area_obj<ScreenBlurArea, AreaForm::Type_Cylinder>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "DepthOfFieldCube",
                    .object_creator = create_area_obj<DepthOfFieldArea, AreaForm::Type_Cube1>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "DepthOfFieldSphere",
                    .object_creator = create_area_obj<DepthOfFieldArea, AreaForm::Type_Sphere>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "DepthOfFieldCylinder",
                    .object_creator = create_area_obj<DepthOfFieldArea, AreaForm::Type_Cylinder>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "BlueStarGuidanceCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "TicoSeedGuidanceCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "MessageAreaCube",
                    .object_creator = create_area_obj<MessageArea, AreaForm::Type_Cube2>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "MessageAreaCylinder",
                    .object_creator = create_area_obj<MessageArea, AreaForm::Type_Cylinder>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "SmokeEffectColorAreaCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "BeeWallShortDistAreaCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "ExtraWallCheckArea",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "ExtraWallCheckCylinder",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cylinder>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "HeavySteeringCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "NonSleepCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "AreaMoveSphere",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Sphere>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "DodoryuClosedCylinder",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cylinder>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "DashChargeCylinder",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cylinder>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "RasterScrollCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "OnimasuCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube1>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "ForbidJumpCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "CollisionArea",
                    .object_creator = create_area_obj<CollisionArea, AreaForm::Type_Cube1>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "AstroOverlookAreaCylinder",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cylinder>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "CelestrialSphere",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Sphere>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "MirrorAreaCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "DarkMatterCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                },
                AreaObjPlacementDescriptor{
                    .object_name = "DarkMatterCylinder",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cylinder>,
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

    std::span<const AreaObjPlacementDescriptor> complete_area_obj_placement_descriptors() noexcept {
        return cCompleteAreaObjPlacementDescriptors;
    }

    const AreaObjPlacementDescriptor *find_complete_area_obj_placement_descriptor(
        std::string_view object_name) noexcept {
        const auto descriptors = complete_area_obj_placement_descriptors();
        const auto found = std::ranges::find_if(descriptors, [&](const auto &descriptor) {
            return equal_string_case(descriptor.object_name, object_name);
        });
        if (found == descriptors.end() || found->object_creator == nullptr) {
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

}  // namespace smgpc::scene
