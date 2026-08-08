#pragma once

#include "Game/AreaObj/AreaObj.hpp"

#include <revolution/types.h>

#include <memory>
#include <span>
#include <string_view>
#include <vector>

class AreaObjContainer;
class NameObj;

namespace smgpc::scene {

    using AreaObjCreator = NameObj *(*)(const char *);
    using AreaObjManagerCreator = AreaObjMgr *(*)(s32, const char *);

    // A descriptor is complete only when both halves of the retail placement
    // route are linked: the actor creator and the manager it enters from
    // AreaObj::init. Keep this as the single support registry consumed by the
    // host NameObjFactory and AreaObjContainer compatibility boundary.
    struct AreaObjPlacementDescriptor {
        std::string_view object_name;
        AreaObjCreator object_creator = nullptr;
        std::string_view manager_name;
        s32 retail_manager_order = -1;
        s32 manager_capacity = 0;
        AreaObjManagerCreator manager_creator = nullptr;
    };

    [[nodiscard]] std::span<const AreaObjPlacementDescriptor> complete_area_obj_placement_descriptors() noexcept;
    [[nodiscard]] const AreaObjPlacementDescriptor *find_complete_area_obj_placement_descriptor(
        std::string_view object_name) noexcept;
    [[nodiscard]] bool is_area_obj_placement_table(std::string_view table_path) noexcept;
    [[nodiscard]] bool placement_has_complete_area_obj_runtime(
        std::string_view object_name, std::string_view table_path,
        bool factory_supported) noexcept;
    [[nodiscard]] AreaObjMgr *find_area_obj_manager_by_retail_prefix(
        std::span<AreaObjMgr *const> managers,
        std::string_view requested_name) noexcept;

    class AreaObjRuntime final {
    public:
        AreaObjRuntime();
        ~AreaObjRuntime();

        AreaObjRuntime(const AreaObjRuntime &) = delete;
        AreaObjRuntime &operator=(const AreaObjRuntime &) = delete;
        AreaObjRuntime(AreaObjRuntime &&) = delete;
        AreaObjRuntime &operator=(AreaObjRuntime &&) = delete;

        [[nodiscard]] AreaObjMgr *adopt_manager(std::unique_ptr<AreaObjMgr> manager);
        void adopt_managers(std::vector<std::unique_ptr<AreaObjMgr>> managers);
        void init_after_placement();

    private:
        std::vector<std::unique_ptr<AreaObjMgr>> _owned_managers;
        bool _did_init_after_placement = false;
    };

    [[nodiscard]] AreaObjRuntime *current_area_obj_runtime() noexcept;

}  // namespace smgpc::scene
