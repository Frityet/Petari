#include <aurora/exception.hpp>
#include "Game/Util/MapUtil.hpp"

#include "Game/Map/CollisionCode.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Util/TriangleFilter.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Map/CollisionCategorizedKeeper.hpp"
#include "Game/Map/CollisionDirector.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/CollisionPartsFilter.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string_view>

namespace {
    bool validateAreaQuery(Triangle* output, u32 capacity, const TVec3f* points, u32 pointCount) {
        // Keep the existing native buffer contract at the public boundary;
        // the query itself belongs to the original Keeper/Parts graph.
        if (!MR::isExistSceneObj(SceneObj_CollisionDirector)) {
            aurora::throw_host_exception<std::logic_error>("Area polygon queries require the original scene CollisionDirector.");
        }
        if (capacity > 512U || pointCount > 32U || (capacity != 0U && !output) || (pointCount != 0U && !points)) {
            aurora::throw_host_exception<std::invalid_argument>("Area polygon query exceeds the original buffer contract.");
        }
        for (u32 i = 0; i < pointCount; ++i) {
            if (!std::isfinite(points[i].x) || !std::isfinite(points[i].y) || !std::isfinite(points[i].z)) {
                aurora::throw_host_exception<std::invalid_argument>("Area polygon queries require finite points.");
            }
        }
        return capacity != 0U && pointCount != 0U;
    }
}

namespace MR {
    u32 createAreaPolygonList(Triangle* pTriangle, u32 param2, const TVec3f& rParam3, const TVec3f& rParam4) {
        const TVec3f points[] = {rParam3, rParam4};
        if (!validateAreaQuery(pTriangle, param2, points, 2U)) return 0;
        return getCollisionDirector()->getCategoryKeeper(0)->createAreaPolygonList(pTriangle, param2, rParam3, rParam4);
    }
    u32 createAreaPolygonListArray(Triangle* pTriangle, u32 param2, TVec3f* pParam3, u32 param4) {
        if (!validateAreaQuery(pTriangle, param2, pParam3, param4)) return 0;
        return getCollisionDirector()->getCategoryKeeper(0)->createAreaPolygonListArray(pTriangle, param2, pParam3, param4);
    }
}

namespace {
    constexpr auto cWallDot = 0.34202015F;

    constexpr auto cFloorCodeNames = std::array{
        "Normal", "Death", "Slip", "NoSlip", "DamageNormal", "Ice", "JumpLow",
        "JumpMiddle", "JumpHigh", "Slider", "DamageFire", "JumpNormal", "FireDance",
        "Sand", "Glass", "DamageElectric", "PullBack", "Sink", "SinkPoison", "Slide",
        "WaterBottomH", "WaterBottomM", "WaterBottomL", "Wet", "Needle", "SinkDeath",
        "Snow", "RailMove", "AreaMove", "Press", "NoStampSand", "SinkDeathMud", "Brake",
        "GlassIce", "JumpParasol",
    };
    constexpr auto cWallCodeNames = std::array{
        "Normal", "NotWallJump", "NotWallSlip", "NotGrab", "GhostThroughCode",
        "NotSideStep", "Rebound", "Fur", "NoAction",
    };
    constexpr auto cSoundCodeNames = std::array{
        "null", "Soil", "Lawn", "Stone", "Marble", "WoodThick", "WoodThin", "Metal",
        "Snow", "Ice", "Shallow", "Sand", "Beach", "Carpet", "Mud", "Honey",
    };
    constexpr auto cCameraCodeNames = std::array{"NoThrough", "Through"};






    template <std::size_t Size>
    [[nodiscard]] s32 code_index(const JMapInfoIter& iter, const char* field,
                                 const std::array<const char*, Size>& names) {
        if (!iter.isValid()) {
            return 0;
        }

        auto numeric = u32{};
        if (iter.getValue(field, &numeric)) {
            return static_cast<s32>(numeric);
        }

        const char* text = nullptr;
        if (!iter.getValue(field, &text) || text == nullptr) {
            return 0;
        }
        for (auto index = std::size_t{}; index < names.size(); ++index) {
            if (std::strcmp(text, names[index]) == 0) {
                return static_cast<s32>(index);
            }
        }
        return 0;
    }

    template <std::size_t Size>
    [[nodiscard]] const char* code_string(const JMapInfoIter& iter, const char* field,
                                          const std::array<const char*, Size>& names) {
        if (iter.isValid()) {
            const char* text = nullptr;
            if (iter.getValue(field, &text) && text != nullptr) {
                return text;
            }
            auto numeric = u32{};
            if (iter.getValue(field, &numeric) && numeric < names.size()) {
                return names[numeric];
            }
        }
        return names.front();
    }
}  // namespace

namespace MR {

    const TVec3f* getNormal(const Triangle* triangle) {
        return triangle != nullptr ? triangle->getNormal(0) : nullptr;
    }

    bool isWallPolygon(const TVec3f& rParam1, const TVec3f& rParam2) {
        if (isNearZero(rParam1)) {
            return false;
        }

        return isWallPolygon(rParam1.dot(rParam2));
    }

    bool isFloorPolygon(const TVec3f& rParam1, const TVec3f& rParam2) {
        if (isNearZero(rParam1)) {
            return false;
        }

        return isFloorPolygon(rParam1.dot(rParam2));
    }

    bool isFloorPolygonCos(const TVec3f& rParam1, const TVec3f& rParam2, f32 param3) {
        if (isNearZero(rParam1)) {
            return false;
        }

        if (-rParam1.dot(rParam2) < param3) {
            return false;
        }

        return isFloorPolygon(rParam1.dot(rParam2));
    }

    bool isWallPolygon(f32 gravity_dot) {
        return std::abs(gravity_dot) < cWallDot;
    }

    bool isFloorPolygon(f32 gravity_dot) {
        return !isWallPolygon(gravity_dot) && gravity_dot < 0.0F;
    }

    bool isCeilingPolygon(f32 gravity_dot) {
        return !isWallPolygon(gravity_dot) && !isFloorPolygon(gravity_dot);
    }



    bool checkStrikePointToMap(const TVec3f& rParam1, HitInfo* pParam2) {
        return getCollisionDirector()->getCategoryKeeper(0)->checkStrikePoint(rParam1, pParam2) != 0;
    }

    bool checkStrikeBallToMap(const TVec3f& center, f32 radius) {
        return Collision::checkStrikeBallToMap(center, radius, nullptr, nullptr) != 0;
    }


    const char* getFloorCodeString(const Triangle* triangle) {
        return triangle != nullptr
                   ? code_string(triangle->getAttributes(), "Floor_code", cFloorCodeNames)
                   : cFloorCodeNames.front();
    }

    const char* getWallCodeString(const Triangle* triangle) {
        return triangle != nullptr
                   ? code_string(triangle->getAttributes(), "Wall_code", cWallCodeNames)
                   : cWallCodeNames.front();
    }

    const char* getSoundCodeString(const Triangle* triangle) {
        return triangle != nullptr
                   ? code_string(triangle->getAttributes(), "Sound_code", cSoundCodeNames)
                   : cSoundCodeNames.front();
    }

    s32 getFloorCodeIndex(const JMapInfoIter& iter) {
        return code_index(iter, "Floor_code", cFloorCodeNames);
    }

    s32 getSoundCodeIndex(const JMapInfoIter& iter) {
        return code_index(iter, "Sound_code", cSoundCodeNames);
    }

    s32 getFloorCodeIndex(const Triangle* triangle) {
        return triangle != nullptr ? getFloorCodeIndex(triangle->getAttributes()) : 0;
    }

    s32 getWallCodeIndex(const Triangle* triangle) {
        return triangle != nullptr ? code_index(triangle->getAttributes(), "Wall_code", cWallCodeNames) : 0;
    }

    s32 getSoundCodeIndex(const Triangle* triangle) {
        return triangle != nullptr ? getSoundCodeIndex(triangle->getAttributes()) : 0;
    }

    s32 getCameraCodeIndex(const Triangle* triangle) {
        return triangle != nullptr ? code_index(triangle->getAttributes(), "Camera_through", cCameraCodeNames) : 0;
    }

    u32 getCameraID(const Triangle* triangle) {
        if (triangle == nullptr) {
            return static_cast<u32>(-1);
        }
        const auto attributes = triangle->getAttributes();
        auto id = u32{static_cast<u32>(-1)};
        (void)attributes.getValue("camera_id", &id);
        return id;
    }

    bool isWaterPolygon(const Triangle* triangle) {
        const auto code = std::string_view(getFloorCodeString(triangle));
        return code == "Water" || code == "Shallow";
    }

    bool isThroughPolygon(const Triangle* triangle) {
        const auto code = std::string_view(getFloorCodeString(triangle));
        return code == "Water" || code == "Shallow" || code == "PullBack";
    }

    bool isGroundCodeWaterIter(const JMapInfoIter& iter) {
        const auto code = getFloorCodeIndex(iter);
        return code == CollisionFloorCode_WaterBottomH || code == CollisionFloorCode_WaterBottomM ||
               code == CollisionFloorCode_WaterBottomL || code == CollisionFloorCode_Wet;
    }

    bool isGroundCodeDeath(const Triangle* triangle) { return getFloorCodeIndex(triangle) == CollisionFloorCode_Death; }
    bool isGroundCodeDamage(const Triangle* triangle) { return getFloorCodeIndex(triangle) == CollisionFloorCode_DamageNormal; }
    bool isGroundCodeIce(const Triangle* triangle) { return getFloorCodeIndex(triangle) == CollisionFloorCode_Ice; }
    bool isGroundCodeDamageFire(const Triangle* triangle) { return getFloorCodeIndex(triangle) == CollisionFloorCode_DamageFire; }
    bool isGroundCodeFireDance(const Triangle* triangle) { return getFloorCodeIndex(triangle) == CollisionFloorCode_FireDance; }
    bool isGroundCodeSand(const Triangle* triangle) { return getFloorCodeIndex(triangle) == CollisionFloorCode_Sand; }
    bool isGroundCodeDamageElectric(const Triangle* triangle) { return getFloorCodeIndex(triangle) == CollisionFloorCode_DamageElectric; }
    bool isGroundCodeWaterBottomH(const Triangle* triangle) { return getFloorCodeIndex(triangle) == CollisionFloorCode_WaterBottomH; }
    bool isGroundCodeWaterBottomM(const Triangle* triangle) { return getFloorCodeIndex(triangle) == CollisionFloorCode_WaterBottomM; }
    bool isGroundCodeSinkDeath(const Triangle* triangle) { return getFloorCodeIndex(triangle) == CollisionFloorCode_SinkDeath; }
    bool isGroundCodeRailMove(const Triangle* triangle) { return getFloorCodeIndex(triangle) == CollisionFloorCode_RailMove; }
    bool isGroundCodeAreaMove(const Triangle* triangle) { return getFloorCodeIndex(triangle) == CollisionFloorCode_AreaMove; }
    bool isGroundCodeNoStampSand(const Triangle* triangle) { return getFloorCodeIndex(triangle) == CollisionFloorCode_NoStampSand; }
    bool isGroundCodeSinkDeathMud(const Triangle* triangle) { return getFloorCodeIndex(triangle) == CollisionFloorCode_SinkDeathMud; }
    bool isGroundCodeBrake(const Triangle* triangle) { return getFloorCodeIndex(triangle) == CollisionFloorCode_Brake; }
    bool isWallCodeGhostThrough(const Triangle* triangle) { return getWallCodeIndex(triangle) == CollisionWallCode_GhostThroughCode; }
    bool isWallCodeRebound(const Triangle* triangle) { return getWallCodeIndex(triangle) == CollisionWallCode_Rebound; }
    bool isWallCodeNoAction(const Triangle* triangle) { return getWallCodeIndex(triangle) == CollisionWallCode_NoAction; }
    bool isSoundCodeSand(const Triangle* triangle) { return getSoundCodeIndex(triangle) == CollisionSoundCode_Sand; }
    bool isCameraCodeThrough(const Triangle* triangle) { return getCameraCodeIndex(triangle) == CollisionCameraCode_Through; }
    bool isCodeSand(const Triangle* triangle) {
        return isSoundCodeSand(triangle) || isGroundCodeSand(triangle) || isGroundCodeNoStampSand(triangle);
    }
}  // namespace MR

namespace Collision {

    s32 checkStrikePointToMap(const TVec3f& rPosition, HitInfo* pInfo) {
        return MR::getCollisionDirector()->getCategoryKeeper(0)->checkStrikePoint(rPosition, pInfo);
    }

    s32 checkStrikeBallToMap(const TVec3f& rPosition, f32 radius, const CollisionPartsFilterBase* pPartsFilter,
                              const TriangleFilterBase* pTriangleFilter) {
        return MR::getCollisionDirector()->getCategoryKeeper(0)->checkStrikeBall(rPosition, radius, false, pPartsFilter, pTriangleFilter);
    }

    s32 checkStrikeBallToMapWithMovingReaction(const TVec3f& rPosition, f32 radius, const CollisionPartsFilterBase* pPartsFilter,
                                                const TriangleFilterBase* pTriangleFilter) {
        return MR::getCollisionDirector()->getCategoryKeeper(0)->checkStrikeBall(rPosition, radius, true, pPartsFilter, pTriangleFilter);
    }

    s32 checkStrikeBallToMapWithThickness(const TVec3f& rPosition, f32 radius, f32 thickness,
                                           const CollisionPartsFilterBase* pPartsFilter, const TriangleFilterBase* pTriangleFilter) {
        return MR::getCollisionDirector()->getCategoryKeeper(0)->checkStrikeBallWithThickness(rPosition, radius, thickness, pPartsFilter,
                                                                                            pTriangleFilter);
    }

}  // namespace Collision
