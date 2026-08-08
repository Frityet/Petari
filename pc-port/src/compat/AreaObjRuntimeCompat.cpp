#include "Game/AreaObj/AreaObj.hpp"
#include "Game/AreaObj/AreaObjContainer.hpp"
#include "Game/AreaObj/MercatorTransformCube.hpp"

#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "scene/AreaObjRuntime.hpp"

#include <algorithm>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

    [[noreturn]] void throw_area_obj_scene_unavailable() {
        throw std::logic_error(
            "AreaObj queries require the active scene-owned retail manager container.");
    }

}  // namespace

AreaObjContainer::AreaObjContainer(const char *pName) : NameObj(pName) {
    mNumManagers = 0;
}

void AreaObjContainer::init(const JMapInfoIter &) {
    auto *runtime = smgpc::scene::current_area_obj_runtime();
    if (runtime == nullptr) {
        throw std::logic_error("AreaObjContainer initialization requires an active scene owner");
    }
    if (mNumManagers != 0U) {
        throw std::logic_error("AreaObjContainer cannot initialize its manager registry twice");
    }

    struct InstalledManager {
        std::string_view name;
        s32 retail_order;
        s32 capacity;
        smgpc::scene::AreaObjManagerCreator creator;
    };
    auto manager_specs = std::vector<InstalledManager>{};
    auto previous_retail_order = s32{-1};

    for (const auto &descriptor : smgpc::scene::complete_area_obj_placement_descriptors()) {
        if (descriptor.object_name.empty() || descriptor.object_creator == nullptr ||
            descriptor.manager_name.empty() || descriptor.retail_manager_order < 0 ||
            descriptor.manager_capacity <= 0 ||
            descriptor.manager_creator == nullptr) {
            throw std::logic_error("AreaObj placement registry contains an incomplete descriptor");
        }
        if (descriptor.retail_manager_order < previous_retail_order) {
            throw std::logic_error("AreaObj placement registry is not in retail manager-table order");
        }
        previous_retail_order = descriptor.retail_manager_order;

        const auto existing = std::ranges::find_if(manager_specs, [&](const auto &manager) {
            return manager.name == descriptor.manager_name;
        });
        if (existing != manager_specs.end()) {
            if (existing->retail_order != descriptor.retail_manager_order ||
                existing->capacity != descriptor.manager_capacity ||
                existing->creator != descriptor.manager_creator) {
                throw std::logic_error("AreaObj placement registry disagrees about manager construction for " +
                                       std::string(descriptor.manager_name));
            }
            continue;
        }

        if (manager_specs.size() >= std::size(mManagerArray)) {
            throw std::length_error("AreaObj placement registry exceeds the retail container capacity");
        }

        manager_specs.push_back(InstalledManager{
            .name = descriptor.manager_name,
            .retail_order = descriptor.retail_manager_order,
            .capacity = descriptor.manager_capacity,
            .creator = descriptor.manager_creator,
        });
    }

    auto constructed_managers = std::vector<std::unique_ptr<AreaObjMgr>>{};
    constructed_managers.reserve(manager_specs.size());
    for (const auto &spec : manager_specs) {
        const auto manager_name = std::string(spec.name);
        auto manager = std::unique_ptr<AreaObjMgr>(
            spec.creator(spec.capacity, manager_name.c_str()));
        if (manager == nullptr) {
            throw std::runtime_error("AreaObj manager creator returned null for " +
                                     manager_name);
        }
        manager->init(JMapInfoIter{});
        constructed_managers.push_back(std::move(manager));
    }

    auto manager_pointers = std::vector<AreaObjMgr *>{};
    manager_pointers.reserve(constructed_managers.size());
    for (const auto &manager : constructed_managers) {
        manager_pointers.push_back(manager.get());
    }
    runtime->adopt_managers(std::move(constructed_managers));
    for (auto *manager : manager_pointers) {
        mManagerArray[mNumManagers] = manager;
        ++mNumManagers;
    }
}

AreaObjMgr *AreaObjContainer::getManager(const char *pName) const {
    if (pName == nullptr) {
        throw std::invalid_argument("AreaObj manager lookup requires a non-null retail name");
    }

    const auto requested_name = std::string_view(pName);
    for (auto index = u32{}; index < mNumManagers; ++index) {
        auto *manager = mManagerArray[index];
        if (manager == nullptr || manager->mName == nullptr) {
            throw std::logic_error("AreaObjContainer contains an invalid manager entry");
        }
    }
    if (auto *manager = smgpc::scene::find_area_obj_manager_by_retail_prefix(
            std::span<AreaObjMgr *const>(mManagerArray, mNumManagers), requested_name);
        manager != nullptr) {
        return manager;
    }

    throw std::logic_error("No complete retail AreaObj manager is installed for " +
                           std::string(requested_name));
}

AreaObj *AreaObjContainer::getAreaObj(const char *pName, const TVec3f &rPos) const {
    return getManager(pName)->find_in(rPos);
}

namespace MR {

    AreaObjContainer *getAreaObjContainer() {
        auto *holder = MR::getSceneObjHolder();
        if (holder == nullptr || !holder->isExist(SceneObj_AreaObjContainer)) {
            throw_area_obj_scene_unavailable();
        }

        auto *container = static_cast<AreaObjContainer *>(holder->getObj(SceneObj_AreaObjContainer));
        if (container == nullptr) {
            throw_area_obj_scene_unavailable();
        }
        return container;
    }

    bool isInWater(const TVec3f &) {
        throw std::logic_error(
            "water-volume queries are unavailable until real WaterArea and WaterAreaHolder scene data are installed.");
    }

    bool isInDeath(const TVec3f &rPos) {
        return MR::getAreaObjContainer()->getManager("DeathArea")->find_in(rPos) != nullptr;
    }

    bool isInDarkMatter(const TVec3f &rPos) {
        auto *container = MR::getAreaObjContainer();
        auto *cube_manager = container->getManager("DarkMatterCube");
        auto *cylinder_manager = container->getManager("DarkMatterCylinder");
        return cube_manager->find_in(rPos) != nullptr ||
               cylinder_manager->find_in(rPos) != nullptr;
    }

    void getDivideMercatorRailPosition(DivideMercatorRailPosInfo *, const LiveActor *, u32, f32, u32) {
        throw std::logic_error(
            "Mercator rail division is unavailable because the retail transformation routine has not been decompiled.");
    }

}  // namespace MR
