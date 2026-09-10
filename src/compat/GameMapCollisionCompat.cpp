#include <aurora/exception.hpp>
#include "Game/Util/MapUtil.hpp"

#include "Game/Map/CollisionCode.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/Util/TriangleFilter.hpp"
#include "Game/Util/MathUtil.hpp"
#include "compat/HitInfoCompat.hpp"
#include "compat/CollisionDirectorOwnership.hpp"
#include "scene/SceneObjHolderRuntime.hpp"
#include "Game/Util/CollisionPartsFilter.hpp"
#include "scene/StageCollisionService.hpp"
#include "aurora/allocation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string_view>
#include <vector>

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
        const auto& collision = require_stage_collision();
        const auto filter = make_query_filter(collision, parts_filter, triangle_filter);
        auto hit = smgpc::scene::StageCollisionHit{};
        if (!collision.line_cast(start, offset, &hit, filter)) {
            return false;
        }

        const auto candidate = smgpc::compat::make_collision_triangle(collision, hit.triangle_index);
        if (position != nullptr) {
            position->set(hit.position);
        }
        if (triangle != nullptr) {
            *triangle = candidate;
        }
        return true;
    }

    [[nodiscard]] std::vector<HitInfo>& strike_infos() {
        static thread_local auto infos = std::vector<HitInfo>{};
        return infos;
    }

    struct SortedLineHits {
        std::array<Triangle, cMaximumStrikeInfos> triangles{};
        std::array<TVec3f, cMaximumStrikeInfos> positions{};
        u32 count = 0U;
    };

    [[nodiscard]] SortedLineHits& sorted_line_hits() {
        static thread_local SortedLineHits hits;
        return hits;
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

        auto& infos = strike_infos();
        infos.clear();
        infos.reserve(contacts.size());
        for (const auto& contact : contacts) {
            infos.push_back(make_hit_info(contact));
        }
        return static_cast<s32>(infos.size());
    }

    s32 store_line_hits(const smgpc::scene::StageCollisionService& collision, const TVec3f& start, const TVec3f& offset, s32 maximum,
                        const CollisionPartsFilterBase* parts_filter, const TriangleFilterBase* triangle_filter) {
        const aurora::allocation::HostAllocationScope host_allocations;
        if (maximum < 0 || maximum > static_cast<s32>(cMaximumStrikeInfos)) {
            aurora::throw_host_exception<std::invalid_argument>("Line strike queries exceed the original 32-hit storage.");
        }
        const auto filter = make_query_filter(collision, parts_filter, triangle_filter);
        const auto hits = collision.line_hits(start, offset,
            maximum == 0 ? cMaximumStrikeInfos : static_cast<std::size_t>(maximum), filter);
        auto& infos = strike_infos();
        infos.clear();
        infos.reserve(hits.size());
        const auto length = PSVECMag(&offset);
        for (const auto& hit : hits) {
            auto& info = infos.emplace_back();
            info.mParentTriangle = smgpc::compat::make_collision_triangle(collision, hit.triangle_index);
            info._60 = length * hit.fraction;
            info.mHitPos = hit.position;
            // Retail checkArrow's all-hit branch does not write the flag
            // array consumed by CollisionParts. Retain the native HitInfo
            // constructor's initialized value for those unspecified bytes.
        }
        return static_cast<s32>(infos.size());
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
    u32 getNearPolyOnLineSort(const TVec3f& reference, const TVec3f& start, const TVec3f& offset,
                              const HitSensor* except_sensor) {
        const aurora::allocation::HostAllocationScope host_allocations;
        const auto hit_count = Collision::checkStrikeLineToMap(start, offset, 0, nullptr, nullptr);
        const auto& hits = strike_infos();
        // The original early return deliberately retains the previous sorted buffer.
        if (hit_count == 0) {
            return 0U;
        }
        auto candidates = std::array<bool, cMaximumStrikeInfos>{};
        auto excluded = u32{};
        for (std::size_t i = 0U; i < hits.size(); ++i) {
            candidates[i] = except_sensor == nullptr || hits[i].mParentTriangle.mSensor != except_sensor;
            excluded += !candidates[i];
        }
        auto& sorted = sorted_line_hits();
        sorted.count = static_cast<u32>(hits.size()) - excluded;
        for (u32 i = 0U; i < sorted.count; ++i) {
            auto nearest_distance = 1000000.0F;
            auto nearest_index = std::size_t{};
            for (std::size_t j = 0U; j < hits.size(); ++j) {
                if (!candidates[j]) {
                    continue;
                }
                TVec3f difference = reference;
                difference.sub(hits[j].mHitPos);
                const auto distance = PSVECMag(&difference);
                if (nearest_distance > distance) {
                    nearest_distance = distance;
                    nearest_index = j;
                }
            }
            const auto& hit = hits[nearest_index];
            sorted.triangles[i] = hit.mParentTriangle;
            sorted.positions[i] = hit.mHitPos;
            candidates[nearest_index] = false;
        }
        return sorted.count;
    }

    bool getSortedPoly(TVec3f* position, Triangle* triangle, u32 index) {
        const auto& sorted = sorted_line_hits();
        if (sorted.count <= index) {
            return false;
        }
        if (triangle != nullptr) {
            *triangle = sorted.triangles[index];
        }
        if (position != nullptr) {
            *position = sorted.positions[index];
        }
        return true;
    }

    const Triangle* getSortedPoly(u32 index) {
        const auto& sorted = sorted_line_hits();
        return sorted.count <= index ? nullptr : &sorted.triangles[index];
    }

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

    bool getFirstPolyOnLineToMap(TVec3f* position, Triangle* triangle, const TVec3f& start,
                                 const TVec3f& offset) {
        return first_line_hit(position, triangle, start, offset, nullptr, nullptr);
    }

    bool getFirstPolyOnLineToMap(TVec3f* position, Triangle* triangle, const TVec3f& start,
                                 const TVec3f& offset, const CollisionPartsFilterBase* parts_filter,
                                 const TriangleFilterBase* triangle_filter) {
        return first_line_hit(position, triangle, start, offset, parts_filter, triangle_filter);
    }

    bool getFirstPolyOnLineToWaterSurface(TVec3f* position, Triangle* triangle, const TVec3f& start,
                                          const TVec3f& offset, const CollisionPartsFilterBase* parts_filter,
                                          const TriangleFilterBase* triangle_filter) {
        const aurora::allocation::HostAllocationScope host_allocations;
        auto* owner = smgpc::scene::current_collision_director_ownership();
        if (owner == nullptr) {
            aurora::throw_host_exception<std::logic_error>("Water-surface collision queries require the scene category owner.");
        }
        // The original category helper admits at most 32 ordered hits after
        // part filtering. Triangle filtering happens afterward, so rejected
        // triangles still consume those original hit slots.
        const auto count = store_line_hits(owner->category_service(2), start, offset, 0, parts_filter, nullptr);
        auto distance = 1000000.0F;
        auto nearest = s32{-1};
        for (s32 i = 0; i < count; ++i) {
            const auto& hit = strike_infos()[static_cast<std::size_t>(i)];
            if (triangle_filter != nullptr) {
                const aurora::allocation::ClientAllocationScope client_allocations;
                if (triangle_filter->isInvalidTriangle(&hit.mParentTriangle)) continue;
            }
            if (distance > hit._60) {
                nearest = i;
                distance = hit._60;
            }
        }
        if (nearest == -1) return false;
        const auto& hit = strike_infos()[static_cast<std::size_t>(nearest)];
        if (position != nullptr) *position = hit.mHitPos;
        if (triangle != nullptr) *triangle = hit.mParentTriangle;
        return true;
    }

    bool getFirstPolyOnLineToWaterSurface(TVec3f* position, Triangle* triangle, const TVec3f& start,
                                          const TVec3f& offset) {
        return getFirstPolyOnLineToWaterSurface(position, triangle, start, offset, nullptr, nullptr);
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

    bool isExistMapCollision(const TVec3f& start, const TVec3f& offset) {
        return require_stage_collision().line_cast(start, offset);
    }

    bool isExistMapCollisionExceptActor(const TVec3f& start, const TVec3f& offset,
                                        const LiveActor* actor) {
        const aurora::allocation::HostAllocationScope host_allocations;
        const auto& collision = require_stage_collision();
        // The original CollisionPartsFilterActor compares the owning sensor's
        // host. Keep every other part eligible before the single-hit limit.
        const auto filter = [&collision, actor](std::uint32_t index) {
            const auto surface = collision.surface(index);
            return surface && (surface->sensor == nullptr || surface->sensor->mHost != actor);
        };
        return !collision.line_hits(start, offset, 1U, filter).empty();
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
    s32 checkStrikeLineToMap(const TVec3f& start, const TVec3f& offset, s32 maximum,
                            const CollisionPartsFilterBase* parts_filter, const TriangleFilterBase* triangle_filter) {
        return store_line_hits(require_stage_collision(), start, offset, maximum, parts_filter, triangle_filter);
    }
    s32 checkStrikeLineToSunshade(const TVec3f& start, const TVec3f& offset, s32 maximum,
                                 const CollisionPartsFilterBase* parts_filter, const TriangleFilterBase* triangle_filter) {
        auto* owner = smgpc::scene::current_collision_director_ownership();
        if (!owner) aurora::throw_host_exception<std::logic_error>("Sunshade collision queries require the scene category owner.");
        return store_line_hits(owner->category_service(1), start, offset, maximum, parts_filter, triangle_filter);
    }

    s32 checkStrikePointToMap(const TVec3f& point, HitInfo* output) {
        const aurora::allocation::HostAllocationScope host_allocations;
        const auto contacts = require_stage_collision().sphere_contacts(point, 0.0F, 1U);
        auto& infos = strike_infos();
        infos.clear();
        if (contacts.empty()) {
            return 0;
        }
        infos.push_back(make_hit_info(contacts.front()));
        if (output != nullptr) {
            *output = infos.front();
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

    const HitInfo* getStrikeInfoMap(u32 index) {
        const auto& infos = strike_infos();
        return index < infos.size() ? &infos[index] : nullptr;
    }

    u32 getStrikeInfoNumMap() {
        return static_cast<u32>(strike_infos().size());
    }
}  // namespace Collision
