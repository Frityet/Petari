#include "Game/Map/LightFunction.hpp"

#include <algorithm>
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

    [[nodiscard]] u8 blend_channel(u8 from, u8 to, f32 rate) {
        const auto clamped = std::clamp(rate, 0.0F, 1.0F);
        return static_cast< u8 >(static_cast< f32 >(from) + ((static_cast< f32 >(to) - static_cast< f32 >(from)) * clamped));
    }

    [[nodiscard]] f32 blend_float(f32 from, f32 to, f32 rate) {
        const auto clamped = std::clamp(rate, 0.0F, 1.0F);
        return from + ((to - from) * clamped);
    }

    void blendColor(_GXColor* pOut, const _GXColor& rFrom, const _GXColor& rTo, f32 rate) {
        pOut->r = blend_channel(rFrom.r, rTo.r, rate);
        pOut->g = blend_channel(rFrom.g, rTo.g, rate);
        pOut->b = blend_channel(rFrom.b, rTo.b, rate);
        pOut->a = blend_channel(rFrom.a, rTo.a, rate);
    }

    void blendVec(TVec3f* pOut, const TVec3f& rFrom, const TVec3f& rTo, f32 rate) {
        pOut->x = blend_float(rFrom.x, rTo.x, rate);
        pOut->y = blend_float(rFrom.y, rTo.y, rate);
        pOut->z = blend_float(rFrom.z, rTo.z, rate);
    }

    void blendLightInfo(LightInfo* pOut, const LightInfo& rFrom, const LightInfo& rTo, f32 rate) {
        blendColor(&pOut->mColor, rFrom.mColor, rTo.mColor, rate);
        blendVec(&pOut->mPos, rFrom.mPos, rTo.mPos, rate);
        pOut->mIsFollowCamera = rTo.mIsFollowCamera;
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

s32 LightFunction::getDefaultStepInterpolate() {
    return 0x1E;
}

bool LightFunction::tryFindNewAreaLightID(const TVec3f&, ZoneLightID* pLightID) {
    if (pLightID == nullptr) {
        return false;
    }

    ZoneLightID candidate;
    if (smgpc::render::light::StageLightData::instance().area_light_info(candidate) == nullptr) {
        return false;
    }

    if (pLightID->_0 == candidate._0 && pLightID->mLightID == candidate.mLightID) {
        return false;
    }

    *pLightID = candidate;
    return true;
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

void LightFunction::blendActorLightInfo(ActorLightInfo* pOut, const ActorLightInfo& rFrom, const ActorLightInfo& rTo, f32 rate) {
    if (pOut == nullptr) {
        return;
    }

    blendLightInfo(&pOut->mInfo0, rFrom.mInfo0, rTo.mInfo0, rate);
    blendLightInfo(&pOut->mInfo1, rFrom.mInfo1, rTo.mInfo1, rate);
    pOut->mAlpha2 = blend_channel(rFrom.mAlpha2, rTo.mAlpha2, rate);
    blendColor(&pOut->mColor, rFrom.mColor, rTo.mColor, rate);
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
