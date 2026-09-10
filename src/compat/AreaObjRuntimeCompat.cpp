#include <aurora/exception.hpp>
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
        aurora::throw_host_exception<std::logic_error>(
            "AreaObj queries require the active scene-owned retail manager container.");
    }

}  // namespace

AreaObjContainer::AreaObjContainer(const char *pName) : NameObj(pName) {
    mNumManagers = 0;
}

void AreaObjContainer::init(const JMapInfoIter &) {
    auto *runtime = smgpc::scene::current_area_obj_runtime();
    if (runtime == nullptr) {
        aurora::throw_host_exception<std::logic_error>("AreaObjContainer initialization requires an active scene owner");
    }
    if (mNumManagers != 0U) {
        aurora::throw_host_exception<std::logic_error>("AreaObjContainer cannot initialize its manager registry twice");
    }

    const auto manager_specs = smgpc::scene::complete_area_obj_manager_descriptors();
    if (manager_specs.size() > std::size(mManagerArray)) {
        aurora::throw_host_exception<std::length_error>("AreaObj manager registry exceeds the retail container capacity");
    }
    auto previous_retail_order = s32{-1};
    for (const auto &manager : manager_specs) {
        if (manager.name.empty() || manager.capacity <= 0 || manager.creator == nullptr ||
            manager.retail_order <= previous_retail_order ||
            std::ranges::count(manager_specs, manager.name, &smgpc::scene::AreaObjManagerDescriptor::name) != 1) {
            aurora::throw_host_exception<std::logic_error>("AreaObj manager registry is incomplete or not in unique retail order");
        }
        previous_retail_order = manager.retail_order;
    }

    // Manager ownership and placement support are separate original lifetimes.
    // An available empty manager must not authorize an unavailable area actor.
    previous_retail_order = -1;
    for (const auto &descriptor : smgpc::scene::complete_area_obj_placement_descriptors()) {
        const auto manager = std::ranges::find(manager_specs, descriptor.manager_name,
                                              &smgpc::scene::AreaObjManagerDescriptor::name);
        if (descriptor.object_name.empty() || descriptor.object_creator == nullptr ||
            descriptor.retail_manager_order < previous_retail_order || manager == manager_specs.end() ||
            manager->retail_order != descriptor.retail_manager_order ||
            manager->capacity != descriptor.manager_capacity || manager->creator != descriptor.manager_creator ||
            manager->finalize != descriptor.manager_finalize) {
            aurora::throw_host_exception<std::logic_error>("AreaObj placement registry disagrees with its original manager: " +
                                                           std::string(descriptor.object_name));
        }
        previous_retail_order = descriptor.retail_manager_order;
    }

    auto constructed_managers = std::vector<std::unique_ptr<AreaObjMgr>>{};
    auto manager_finalizers = std::vector<smgpc::scene::AreaObjManagerFinalize>{};
    constructed_managers.reserve(manager_specs.size());
    manager_finalizers.reserve(manager_specs.size());
    for (const auto &spec : manager_specs) {
        auto manager = std::unique_ptr<AreaObjMgr>(
            spec.creator(spec.capacity, spec.name.data()));
        if (manager == nullptr) {
            aurora::throw_host_exception<std::runtime_error>("AreaObj manager creator returned null for " +
                                     std::string(spec.name));
        }
        manager->init(JMapInfoIter{});
        constructed_managers.push_back(std::move(manager));
        manager_finalizers.push_back(spec.finalize);
    }

    auto manager_pointers = std::vector<AreaObjMgr *>{};
    manager_pointers.reserve(constructed_managers.size());
    for (const auto &manager : constructed_managers) {
        manager_pointers.push_back(manager.get());
    }
    runtime->adopt_managers(std::move(constructed_managers), std::move(manager_finalizers));
    for (auto *manager : manager_pointers) {
        mManagerArray[mNumManagers] = manager;
        ++mNumManagers;
    }
}

AreaObjMgr *AreaObjContainer::getManager(const char *pName) const {
    if (pName == nullptr) {
        aurora::throw_host_exception<std::invalid_argument>("AreaObj manager lookup requires a non-null retail name");
    }

    const auto requested_name = std::string_view(pName);
    for (auto index = u32{}; index < mNumManagers; ++index) {
        auto *manager = mManagerArray[index];
        if (manager == nullptr || manager->mName == nullptr) {
            aurora::throw_host_exception<std::logic_error>("AreaObjContainer contains an invalid manager entry");
        }
    }
    if (auto *manager = smgpc::scene::find_area_obj_manager_by_retail_prefix(
            std::span<AreaObjMgr *const>(mManagerArray, mNumManagers), requested_name);
        manager != nullptr) {
        return manager;
    }

    aurora::throw_host_exception<std::logic_error>("No complete retail AreaObj manager is installed for " +
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
        aurora::throw_host_exception<std::logic_error>(
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
        aurora::throw_host_exception<std::logic_error>(
            "Mercator rail division is unavailable because the retail transformation routine has not been decompiled.");
    }

}  // namespace MR
