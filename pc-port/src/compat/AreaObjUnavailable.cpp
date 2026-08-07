#include "Game/AreaObj/AreaObjContainer.hpp"
#include "Game/AreaObj/MercatorTransformCube.hpp"

#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/ObjUtil.hpp"

#include <stdexcept>

namespace {

    [[noreturn]] void throw_area_obj_unavailable() {
        throw std::logic_error(
            "AreaObj queries are unavailable until the active scene owns a real container populated from stage placement data.");
    }

}  // namespace

AreaObjMgr* AreaObjContainer::getManager(const char*) const {
    throw_area_obj_unavailable();
}

AreaObj* AreaObjContainer::getAreaObj(const char*, const TVec3f&) const {
    throw_area_obj_unavailable();
}

namespace MR {

    AreaObjContainer* getAreaObjContainer() {
        auto* holder = MR::getSceneObjHolder();
        if (holder == nullptr || !holder->isExist(SceneObj_AreaObjContainer)) {
            throw_area_obj_unavailable();
        }

        auto* container = static_cast< AreaObjContainer* >(holder->getObj(SceneObj_AreaObjContainer));
        if (container == nullptr) {
            throw_area_obj_unavailable();
        }
        return container;
    }

    bool isInWater(const TVec3f&) {
        throw std::logic_error(
            "water-volume queries are unavailable until real WaterArea and WaterAreaHolder scene data are installed.");
    }

    bool isInDeath(const TVec3f&) {
        throw_area_obj_unavailable();
    }

    bool isInDarkMatter(const TVec3f&) {
        throw_area_obj_unavailable();
    }

    void getDivideMercatorRailPosition(DivideMercatorRailPosInfo*, const LiveActor*, u32, f32, u32) {
        throw std::logic_error(
            "Mercator rail division is unavailable because the retail transformation routine has not been decompiled.");
    }

}  // namespace MR
