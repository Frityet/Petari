#include <aurora/exception.hpp>
#include "Game/Util/MapUtil.hpp"

#include "Game/Map/CollisionCode.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Util/TriangleFilter.hpp"
#include "Game/Util/MathUtil.hpp"
#include "compat/HitInfoCompat.hpp"
#include "Game/Map/CollisionCategorizedKeeper.hpp"
#include "Game/Map/CollisionDirector.hpp"
#include "Game/Util/CollisionPartsFilter.hpp"
#include "scene/StageCollisionService.hpp"
#include "aurora/allocation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string_view>

namespace MR {
    u32 createAreaPolygonList(Triangle* triangles, u32 maximum, const TVec3f& point_a, const TVec3f& point_b) {
        // Both retail entry points use the same world bounds and part order.
        // checkArea3D sorts its two local inputs, making the array path with
        // exactly these two points equivalent after the full inverse transform.
        auto points = std::array{point_a, point_b};
        return createAreaPolygonListArray(triangles, maximum, points.data(), 2U);
    }
    u32 createAreaPolygonListArray(Triangle* triangles, u32 maximum, TVec3f* points, u32 point_count) {
        const aurora::allocation::HostAllocationScope host_allocations;
        const auto& collision = smgpc::scene::StageCollisionService::active();
        if (collision == nullptr) {
            aurora::throw_host_exception<std::logic_error>("Area polygon queries require a scene-owned collision service.");
        }
        if ((maximum != 0U && triangles == nullptr) || (point_count != 0U && points == nullptr)) {
            aurora::throw_host_exception<std::invalid_argument>("Area polygon queries require the supplied output and point buffers.");
        }
        const auto identities = collision->area_polygons({points, point_count}, maximum);
        for (auto i = std::size_t{}; i < identities.size(); ++i) {
            triangles[i] = smgpc::compat::make_collision_triangle(*collision, identities[i]);
        }
        return static_cast<u32>(identities.size());
    }
}

namespace {
    constexpr auto cWallDot = 0.34202015F;
    constexpr auto cMaximumStrikeInfos = std::size_t{32U};

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

    [[nodiscard]] smgpc::scene::StageCollisionService& require_stage_collision() {
        auto* collision = smgpc::scene::StageCollisionService::active();
        if (collision == nullptr) {
            aurora::throw_host_exception<std::logic_error>("Map-collision queries require a scene-owned CollisionDirector equivalent.");
        }
        return *collision;
    }

    [[nodiscard]] HitInfo make_hit_info(const smgpc::scene::StageCollisionContact& contact) {
        auto info = HitInfo{};
        info.mParentTriangle = smgpc::compat::make_collision_triangle(
            require_stage_collision(), contact.triangle_index);
        info._60 = contact.penetration;
        info.mHitPos.set(contact.position);
        info._7C.set(contact.moving_reaction);
        info._88 = 1U;
        return info;
    }

    smgpc::scene::StageCollisionTriangleFilter make_query_filter(
        const smgpc::scene::StageCollisionService& collision,
        const CollisionPartsFilterBase* parts_filter, const TriangleFilterBase* triangle_filter) {
        const aurora::allocation::HostAllocationScope host;
        if (!parts_filter) return smgpc::compat::make_collision_triangle_filter(collision, triangle_filter);
        return [&collision, parts_filter, triangle_filter](std::uint32_t index) {
            const auto source = collision.surface(index);
            if (!source) return false;
            const aurora::allocation::ClientAllocationScope client;
            if (source->parts) {
                if (parts_filter->isInvalidParts(source->parts)) return false;
            } else if (const auto* sensor_filter = dynamic_cast<const CollisionPartsFilterSensor*>(parts_filter)) {
                if (source->sensor == sensor_filter->mSensor) return false;
            } else if (const auto* actor_filter = dynamic_cast<const CollisionPartsFilterActor*>(parts_filter)) {
                if (source->sensor && source->sensor->mHost == actor_filter->mActor) return false;
            } else {
                aurora::throw_host_exception<std::logic_error>("A custom CollisionParts filter requires an actual collision part.");
            }
            if (!triangle_filter) return true;
            const auto triangle = smgpc::compat::make_collision_triangle(collision, index);
            return !triangle_filter->isInvalidTriangle(&triangle);
        };
    }

    [[nodiscard]] bool first_line_hit(TVec3f* position, Triangle* triangle, const TVec3f& start,
                                      const TVec3f& offset, const CollisionPartsFilterBase* parts_filter,
                                      const TriangleFilterBase* triangle_filter) {
        return MR::getFirstPolyOnLineToMap(position, triangle, start, offset, parts_filter, triangle_filter);
    }

    [[nodiscard]] s32 store_sphere_contacts(const TVec3f& center, float radius,
                                            const CollisionPartsFilterBase* parts_filter,
                                            const TriangleFilterBase* triangle_filter,
                                            const float* thickness) {
        const aurora::allocation::HostAllocationScope host_allocations;
        const auto& collision = require_stage_collision();
        const auto filter = make_query_filter(collision, parts_filter, triangle_filter);
        auto contacts = thickness == nullptr
                            ? collision.sphere_contacts(center, radius, cMaximumStrikeInfos, filter)
                            : collision.sphere_contacts_with_thickness(
                                  center, radius, *thickness, cMaximumStrikeInfos, filter);

        auto* keeper = MR::getCollisionDirector()->getCategoryKeeper(0);
        keeper->_10 = 0;
        for (const auto& contact : contacts) {
            keeper->mHitInfoArray[keeper->_10++] = make_hit_info(contact);
        }
        return keeper->_10;
    }

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

    bool getFirstPolyNormalOnLineToMap(TVec3f* normal, const TVec3f& start, const TVec3f& offset,
                                       TVec3f* position, const HitSensor* except_sensor) {
        auto triangle = Triangle{};
        if (!first_line_hit(position, &triangle, start, offset, nullptr, nullptr) ||
            (except_sensor != nullptr && triangle.mSensor == except_sensor)) {
            return false;
        }
        if (normal != nullptr) {
            normal->set(*triangle.getNormal(0));
        }
        return true;
    }

    bool getFirstPolyOnLineBFast(const TVec3f& start, const TVec3f& offset, TVec3f* position,
                                 Triangle* triangle) {
        return first_line_hit(position, triangle, start, offset, nullptr, nullptr);
    }

    bool checkStrikePointToMap(const TVec3f& point, HitInfo* output) {
        return Collision::checkStrikePointToMap(point, output) != 0;
    }

    bool checkStrikeBallToMap(const TVec3f& center, f32 radius) {
        return Collision::checkStrikeBallToMap(center, radius, nullptr, nullptr) != 0;
    }

    const Triangle* getCameraPolyFast(const TVec3f& start, const TVec3f& offset,
                                      const HitSensor* except_sensor) {
        static thread_local auto triangle = Triangle{};
        if (!first_line_hit(nullptr, &triangle, start, offset, nullptr, nullptr) ||
            (except_sensor != nullptr && triangle.mSensor == except_sensor)) {
            return nullptr;
        }
        return &triangle;
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

    s32 checkStrikePointToMap(const TVec3f& point, HitInfo* output) {
        const aurora::allocation::HostAllocationScope host_allocations;
        const auto contacts = require_stage_collision().sphere_contacts(point, 0.0F, 1U);
        auto* keeper = MR::getCollisionDirector()->getCategoryKeeper(0);
        keeper->_10 = 0;
        if (contacts.empty()) {
            return 0;
        }
        keeper->mHitInfoArray[0] = make_hit_info(contacts.front());
        keeper->_10 = 1;
        if (output != nullptr) {
            *output = keeper->mHitInfoArray[0];
        }
        return 1;
    }

    s32 checkStrikeBallToMap(const TVec3f& center, f32 radius,
                             const CollisionPartsFilterBase* parts_filter,
                             const TriangleFilterBase* triangle_filter) {
        return store_sphere_contacts(center, radius, parts_filter, triangle_filter, nullptr);
    }

    s32 checkStrikeBallToMapWithMovingReaction(const TVec3f& center, f32 radius,
                                               const CollisionPartsFilterBase* parts_filter,
                                               const TriangleFilterBase* triangle_filter) {
        return store_sphere_contacts(center, radius, parts_filter, triangle_filter, nullptr);
    }

    s32 checkStrikeBallToMapWithThickness(const TVec3f& center, f32 radius, f32 thickness,
                                          const CollisionPartsFilterBase* parts_filter,
                                          const TriangleFilterBase* triangle_filter) {
        return store_sphere_contacts(center, radius, parts_filter, triangle_filter, &thickness);
    }

}  // namespace Collision
