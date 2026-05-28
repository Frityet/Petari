#include "Game/Map/LightFunction.hpp"

#include <array>
#include <cstddef>

#include "render/GXState.hpp"
#include "render/light/LightData.hpp"
#include "runtime/RuntimeContext.hpp"

namespace {
    constexpr auto cLightDataIDs =
        std::array< GXLightID, 8U >{GX_LIGHT0, GX_LIGHT1, GX_LIGHT2, GX_LIGHT3, GX_LIGHT4, GX_LIGHT5, GX_LIGHT6, GX_LIGHT7};

    [[nodiscard]] std::array< std::uint8_t, 4U > color_value(_GXColor color) {
        return {color.r, color.g, color.b, color.a};
    }

    [[nodiscard]] std::array< float, 3U > vec3_value(const TVec3f& value) {
        return {value.x, value.y, value.z};
    }

    [[nodiscard]] std::size_t light_slot(_GXLightID lightID) {
        for (auto index = std::size_t{}; index < cLightDataIDs.size(); ++index) {
            if (cLightDataIDs[index] == lightID) {
                return index;
            }
        }

        return cLightDataIDs.size();
    }

    void loadLightDiffuse(_GXColor color, const TVec3f& rPos, _GXLightID lightID) {
        auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
        const auto slot = light_slot(lightID);
        if (runtime == nullptr || slot >= cLightDataIDs.size()) {
            return;
        }

        auto light = smgpc::render::GXLightState{};
        light.loaded = true;
        light.color = color_value(color);
        light.position = vec3_value(rPos);
        light.cosine_attenuation = {1.0F, 0.0F, 0.0F};
        light.distance_attenuation = {1.0F, 0.0F, 0.0F};
        runtime->scene_lights().set_light(slot, light);
    }

    void loadLightInfoDiffuse(const LightInfo& rInfo, _GXLightID lightID) {
        auto position = TVec3f{};
        LightFunction::calcLightWorldPos(&position, rInfo);
        loadLightDiffuse(rInfo.mColor, position, lightID);
    }
}  // namespace

void LightFunction::initLightRegisterAll() {
    loadAllLightWhite();
}

void LightFunction::initLightData() {
    (void)smgpc::render::light::StageLightData::instance().area_light_info(ZoneLightID{});
}

ResourceHolder* LightFunction::loadLightArchive() {
    return nullptr;
}

s32 LightFunction::createLightDataParser(JMapInfo**) {
    return 0;
}

s32 LightFunction::createZoneDataParser(const char*, JMapInfo**) {
    return 0;
}

void LightFunction::loadAllLightWhite() {
    auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
    if (runtime == nullptr) {
        return;
    }

    auto light = smgpc::render::GXLightState{};
    light.loaded = true;
    light.color = {255U, 255U, 255U, 255U};
    light.position = {1.0F, 1.0F, 1.0F};
    light.cosine_attenuation = {1.0F, 0.0F, 0.0F};
    light.distance_attenuation = {1.0F, 0.0F, 0.0F};
    for (auto lightID : cLightDataIDs) {
        runtime->scene_lights().set_light(light_slot(lightID), light);
    }
}

AreaLightInfo* LightFunction::getAreaLightInfo(const ZoneLightID& rZoneID) {
    return smgpc::render::light::StageLightData::instance().area_light_info(rZoneID);
}

void LightFunction::loadActorLightInfo(const ActorLightInfo* pInfo) {
    if (pInfo == nullptr) {
        return;
    }

    loadLightInfoDiffuse(pInfo->mInfo0, GX_LIGHT0);
    loadLightInfoDiffuse(pInfo->mInfo1, GX_LIGHT1);
    const auto alpha = pInfo->mAlpha2;
    loadLightDiffuse(_GXColor{0U, 0U, 0U, alpha}, TVec3f{}, GX_LIGHT2);
}

void LightFunction::getAreaLightLightData(JMapInfo*, int, AreaLightInfo*) {
}

const char* LightFunction::getDefaultAreaLightName() {
    return smgpc::render::light::StageLightData::instance().default_area_light_name();
}

void LightFunction::loadPointLightInfo(const PointLightInfo* pInfo) {
    if (pInfo == nullptr) {
        return;
    }

    auto* runtime = smgpc::runtime::RuntimeContext::try_instance();
    if (runtime == nullptr) {
        return;
    }

    auto light = smgpc::render::GXLightState{};
    light.loaded = true;
    light.color = color_value(pInfo->mColor);
    light.position = vec3_value(pInfo->mPosition);
    light.cosine_attenuation = {1.0F, 0.0F, 0.0F};
    light.distance_attenuation = {1.0F, pInfo->mRadius, pInfo->mBrightness};
    runtime->scene_lights().set_light(light_slot(GX_LIGHT4), light);
}

void LightFunction::loadLightInfoCoin(const LightInfoCoin* pInfo) {
    if (pInfo == nullptr) {
        return;
    }

    loadLightInfoDiffuse(*pInfo, GX_LIGHT1);
    loadLightDiffuse(_GXColor{pInfo->_14, pInfo->_15, pInfo->_16, pInfo->_17}, TVec3f{0.0F, 0.0F, 1.0F}, GX_LIGHT3);
}

void LightFunction::registerLightAreaHolder(LightAreaHolder*) {
}

void LightFunction::calcLightWorldPos(TVec3f* pOut, const LightInfo& rInfo) {
    if (pOut == nullptr) {
        return;
    }

    *pOut = rInfo.mPos;
}

void LightFunction::registerPlayerLightCtrl(const ActorLightCtrl*) {
}
