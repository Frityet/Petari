#include "scene/AreaObjRuntime.hpp"

#include "Game/AreaObj/AreaForm.hpp"
#include "Game/AreaObj/AreaObj.hpp"
#include "Game/AreaObj/CubeCamera.hpp"
#include "Game/AreaObj/MessageArea.hpp"

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

        [[nodiscard]] AreaObjMgr *create_area_obj_manager(s32 capacity, const char *name) {
            return new AreaObjMgr(capacity, name);
        }

        [[nodiscard]] AreaObjMgr *create_cube_camera_manager(s32 capacity, const char *name) {
            return new CubeCameraMgr(capacity, name);
        }

        void finalize_cube_camera_manager(AreaObjMgr &manager) {
            auto *camera_manager = dynamic_cast<CubeCameraMgr *>(&manager);
            if (camera_manager == nullptr) {
                throw std::logic_error(
                    "CubeCamera descriptor did not construct its exact retail manager");
            }
            camera_manager->initAfterLoad();
        }

        // Add an entry only after its exact actor init path and every manager
        // dependency are linked. The host factory consumes this same table, so
        // a manager by itself can never make a placement appear supported.
        constexpr auto cCompleteAreaObjPlacementDescriptors =
            std::array{
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
                    .object_name = "PullBackCylinder",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cylinder>,
                    .manager_name = "PullBackCylinder",
                    .retail_manager_order = 17,
                    .manager_capacity = 0x40,
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
                    .object_name = "BlueStarGuidanceCube",
                    .object_creator = create_area_obj<AreaObj, AreaForm::Type_Cube2>,
                    .manager_name = "BlueStarGuidanceCube",
                    .retail_manager_order = 40,
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
        if (found == descriptors.end() || found->object_creator == nullptr ||
            found->manager_creator == nullptr || found->manager_name.empty() ||
            found->retail_manager_order < 0 || found->manager_capacity <= 0) {
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

    AreaObjRuntime::~AreaObjRuntime() = default;

    AreaObjMgr *AreaObjRuntime::adopt_manager(
        std::unique_ptr<AreaObjMgr> manager,
        AreaObjManagerFinalize finalize) {
        if (manager == nullptr) {
            throw std::invalid_argument("AreaObjRuntime cannot own a null retail manager");
        }
        if (_did_init_after_placement) {
            throw std::logic_error("AreaObjRuntime cannot adopt a manager after the scene post-placement phase");
        }
        auto *result = manager.get();
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
            throw std::logic_error("AreaObjRuntime cannot adopt managers after the scene post-placement phase");
        }
        if (std::ranges::any_of(managers, [](const auto &manager) { return manager == nullptr; })) {
            throw std::invalid_argument("AreaObjRuntime cannot own a null retail manager");
        }
        if (finalizers.empty()) {
            finalizers.resize(managers.size(), nullptr);
        } else if (finalizers.size() != managers.size()) {
            throw std::invalid_argument(
                "AreaObjRuntime manager finalizers must match the adopted manager count");
        }

        _owned_managers.reserve(_owned_managers.size() + managers.size());
        for (auto index = std::size_t{}; index < managers.size(); ++index) {
            _owned_managers.push_back(OwnedManager{
                .manager = std::move(managers[index]),
                .finalize = finalizers[index],
            });
        }
    }

    void AreaObjRuntime::init_after_placement() {
        if (_did_init_after_placement) {
            return;
        }
        for (auto &owned : _owned_managers) {
            if (!owned.did_init_after_placement) {
                owned.manager->initAfterPlacement();
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
