#pragma once

#include <span>
#include <string_view>

class NameObj;

namespace smgpc::scene {
    using AreaObjCreator = NameObj *(*)(const char *);
    struct AreaObjPlacementDescriptor {
        std::string_view object_name;
        AreaObjCreator object_creator = nullptr;
    };
    [[nodiscard]] std::span<const AreaObjPlacementDescriptor> complete_area_obj_placement_descriptors() noexcept;
    [[nodiscard]] const AreaObjPlacementDescriptor *find_complete_area_obj_placement_descriptor(std::string_view object_name) noexcept;
    [[nodiscard]] bool is_area_obj_placement_table(std::string_view table_path) noexcept;
    [[nodiscard]] bool placement_has_complete_area_obj_runtime(std::string_view object_name, std::string_view table_path, bool factory_supported) noexcept;
} // namespace smgpc::scene
