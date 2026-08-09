#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/CollisionCode.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Player/MarioMapCode.hpp"
#include "Game/Util/MapUtil.hpp"
#include "resource/BcsvTable.hpp"
#include "runtime/SceneScheduler.hpp"
#include "scene/StageCollisionService.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void write_be32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 24U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value >> 16U);
        bytes[offset + 2U] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 3U] = static_cast<std::uint8_t>(value);
    }

    void write_be16(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint16_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value);
    }

    void write_be_float(std::vector<std::uint8_t>& bytes, std::size_t offset, float value) {
        write_be32(bytes, offset, std::bit_cast<std::uint32_t>(value));
    }

    void write_bcsv_field(std::vector<std::uint8_t>& bytes, std::size_t index,
                          std::string_view name, std::uint16_t offset) {
        const auto field_offset = 0x10U + index * 0x0cU;
        write_be32(bytes, field_offset, smgpc::resource::jmap_hash(name));
        write_be32(bytes, field_offset + 0x04U, 0xffffffffU);
        write_be16(bytes, field_offset + 0x08U, offset);
        bytes[field_offset + 0x0aU] = 0U;
        bytes[field_offset + 0x0bU] =
            static_cast<std::uint8_t>(smgpc::resource::BcsvFieldType::StringOffset);
    }

    std::vector<std::uint8_t> make_collision_attributes(std::string_view floor,
                                                        std::string_view wall,
                                                        std::string_view sound) {
        constexpr auto field_count = std::size_t{3U};
        constexpr auto data_offset = 0x10U + field_count * 0x0cU;
        constexpr auto entry_size = std::size_t{12U};
        const auto string_bytes = floor.size() + wall.size() + sound.size() + 3U;
        auto bytes = std::vector<std::uint8_t>(data_offset + entry_size + string_bytes, 0U);
        write_be32(bytes, 0x00U, 1U);
        write_be32(bytes, 0x04U, static_cast<std::uint32_t>(field_count));
        write_be32(bytes, 0x08U, static_cast<std::uint32_t>(data_offset));
        write_be32(bytes, 0x0cU, static_cast<std::uint32_t>(entry_size));
        write_bcsv_field(bytes, 0U, "Floor_code", 0U);
        write_bcsv_field(bytes, 1U, "Wall_code", 4U);
        write_bcsv_field(bytes, 2U, "Sound_code", 8U);

        const auto strings = data_offset + entry_size;
        const auto wall_offset = floor.size() + 1U;
        const auto sound_offset = wall_offset + wall.size() + 1U;
        write_be32(bytes, data_offset + 0U, 0U);
        write_be32(bytes, data_offset + 4U, static_cast<std::uint32_t>(wall_offset));
        write_be32(bytes, data_offset + 8U, static_cast<std::uint32_t>(sound_offset));
        std::memcpy(bytes.data() + strings, floor.data(), floor.size());
        std::memcpy(bytes.data() + strings + wall_offset, wall.data(), wall.size());
        std::memcpy(bytes.data() + strings + sound_offset, sound.data(), sound.size());
        return bytes;
    }

    std::vector<std::uint8_t> make_single_triangle_kcl(float thickness = 2.0F,
                                                       std::uint16_t attribute = 0U) {
        constexpr auto position_offset = 0x38U;
        constexpr auto normal_offset = 0x44U;
        constexpr auto prism_offset = 0x74U;
        constexpr auto octree_offset = 0x84U;
        auto bytes = std::vector<std::uint8_t>(0x88U, 0U);
        write_be32(bytes, 0x00U, position_offset);
        write_be32(bytes, 0x04U, normal_offset);
        write_be32(bytes, 0x08U, prism_offset - 0x10U);
        write_be32(bytes, 0x0cU, octree_offset);
        write_be_float(bytes, 0x10U, thickness);

        const auto write_vec3 = [&](std::size_t offset, float x, float y, float z) {
            write_be_float(bytes, offset, x);
            write_be_float(bytes, offset + 4U, y);
            write_be_float(bytes, offset + 8U, z);
        };
        write_vec3(position_offset, 0.0F, 0.0F, 0.0F);
        write_vec3(normal_offset + 0x00U, 0.0F, 1.0F, 0.0F);
        write_vec3(normal_offset + 0x0cU, 1.0F, 0.0F, 0.0F);
        write_vec3(normal_offset + 0x18U, 0.0F, 0.0F, 1.0F);
        constexpr auto diagonal = 0.70710678118F;
        write_vec3(normal_offset + 0x24U, diagonal, 0.0F, diagonal);

        write_be_float(bytes, prism_offset, diagonal);
        write_be16(bytes, prism_offset + 4U, 0U);
        write_be16(bytes, prism_offset + 6U, 0U);
        write_be16(bytes, prism_offset + 8U, 1U);
        write_be16(bytes, prism_offset + 10U, 2U);
        write_be16(bytes, prism_offset + 12U, 3U);
        write_be16(bytes, prism_offset + 14U, attribute);
        return bytes;
    }

    class MarioMovementShapeProbe final : public LiveActor {
    public:
        MarioMovementShapeProbe() : LiveActor("MarioMovementShapeProbe") {
        }

        void control() override {
            mVelocity.set(0.0F, -0.5F, 0.0F);
        }

        void movement() override {
            LiveActor::movement();
            position_after_base.set(mPosition);
            if (mBinder == nullptr || !mBinder->isBindedGround()) {
                return;
            }
            ground_seen_after_base = true;
            ground_triangle = mBinder->mGroundInfo.mParentTriangle;
            attributes_valid = ground_triangle.getAttributes().isValid();
            auto floor_code = FloorCode{};
            exact_floor_code = floor_code.getCode(&ground_triangle);
            exact_sound_code = MR::getSoundCodeIndex(ground_triangle.getAttributes());
        }

        bool ground_seen_after_base = false;
        bool attributes_valid = false;
        u32 exact_floor_code = static_cast<u32>(-1);
        s32 exact_sound_code = -1;
        TVec3f position_after_base{};
        Triangle ground_triangle{};
    };
}  // namespace

int main() {
    constexpr auto identity = std::array<float, 12U>{
        1.0F, 0.0F, 0.0F, 0.0F,
        0.0F, 1.0F, 0.0F, 0.0F,
        0.0F, 0.0F, 1.0F, 0.0F,
    };
    constexpr auto wall = std::array<float, 12U>{
        0.0F, 1.0F, 0.0F, 10.0F,
        1.0F, 0.0F, 0.0F, 5.0F,
        0.0F, 0.0F, 1.0F, 0.0F,
    };
    constexpr auto ceiling = std::array<float, 12U>{
        1.0F, 0.0F, 0.0F, 0.0F,
        0.0F, -1.0F, 0.0F, 10.0F,
        0.0F, 0.0F, 1.0F, 0.0F,
    };

    auto collision = smgpc::scene::StageCollisionService{};
    const auto kcl = make_single_triangle_kcl();
    const auto damage_fire = make_collision_attributes("DamageFire", "Normal", "Sand");
    require(collision.register_kcl(kcl, identity, "walk-floor.kcl", nullptr, damage_fire).accepted,
            "the real floor KCL and its .pa row must register together");
    require(collision.register_kcl(kcl, wall, "walk-wall.kcl", nullptr, damage_fire).accepted,
            "the real wall KCL must register");
    require(collision.register_kcl(kcl, ceiling, "walk-ceiling.kcl", nullptr, damage_fire).accepted,
            "the real ceiling KCL must register");
    collision.build();
    collision.activate();

    auto floor_triangle = Triangle{};
    auto wall_triangle = Triangle{};
    auto ceiling_triangle = Triangle{};
    auto hit_position = TVec3f{};
    require(MR::getFirstPolyOnLineToMap(&hit_position, &floor_triangle,
                                        TVec3f{0.25F, 2.0F, 0.25F}, TVec3f{0.0F, -4.0F, 0.0F}) &&
                floor_triangle.getNormal(0)->epsilonEquals(TVec3f{0.0F, 1.0F, 0.0F}, 0.0001F),
            "Mario's ground ray must receive a real floor Triangle");
    require(MR::getFirstPolyOnLineToMap(&hit_position, &wall_triangle,
                                        TVec3f{12.0F, 5.25F, 0.25F}, TVec3f{-4.0F, 0.0F, 0.0F}) &&
                MR::isWallPolygon(*wall_triangle.getNormal(0), TVec3f{0.0F, -1.0F, 0.0F}),
            "Mario's wall ray must receive a real wall Triangle");
    require(MR::getFirstPolyOnLineToMap(&hit_position, &ceiling_triangle,
                                        TVec3f{0.25F, 8.0F, 0.25F}, TVec3f{0.0F, 4.0F, 0.0F}) &&
                ceiling_triangle.getNormal(0)->epsilonEquals(TVec3f{0.0F, -1.0F, 0.0F}, 0.0001F),
            "Mario's ceiling ray must receive a real ceiling Triangle");

    auto point_info = HitInfo{};
    require(MR::checkStrikePointToMap(TVec3f{0.25F, 0.0F, 0.25F}, &point_info) &&
                point_info.mParentTriangle.mIdx == floor_triangle.mIdx,
            "Mario's point query must retain the real KCL Triangle identity");
    require(Collision::checkStrikeBallToMap(TVec3f{10.25F, 5.25F, 0.25F}, 0.5F,
                                            nullptr, nullptr) > 0 &&
                Collision::getStrikeInfoMap(0) != nullptr,
            "Mario's wall sphere query must publish a real HitInfo buffer");

    auto scheduler = smgpc::runtime::SceneScheduler{};
    auto actor = MarioMovementShapeProbe{};
    actor.mPosition.set(0.25F, 0.75F, 0.25F);
    actor.makeActorAppeared();
    actor.initBinder(0.5F, 0.0F, 8U);
    scheduler.connect_name_obj(actor, 0, -1, -1, -1);
    scheduler.execute_movement();

    require(actor.ground_seen_after_base && actor.attributes_valid,
            "the derived MarioActor frame must see a fresh Binder ground Triangle and its real .pa row");
    require(actor.exact_floor_code == CollisionFloorCode_DamageFire &&
                actor.exact_sound_code == CollisionSoundCode_Sand,
            "MarioMapCode and the immediate sound caller must consume the contacted .pa row");
    require(actor.mPosition.epsilonEquals(actor.position_after_base, 0.0001F),
            "the scheduler must not integrate again after the virtual MarioActor-shaped movement call");

    const auto stale_triangle = actor.ground_triangle;
    collision.clear();
    require(!stale_triangle.isValid() && !stale_triangle.getAttributes().isValid(),
            "a retained Triangle must lose its attribute owner when stage collision is cleared");
    collision.deactivate();

    const auto ice = make_collision_attributes("Ice", "Normal", "Stone");
    auto replacement_collision = smgpc::scene::StageCollisionService{};
    require(replacement_collision.register_kcl(
                kcl, identity, "replacement-floor.kcl", nullptr, ice).accepted,
            "replacement KCL must register after lifecycle reset");
    replacement_collision.build();
    replacement_collision.activate();
    auto replacement = Triangle{};
    require(MR::getFirstPolyOnLineToMap(nullptr, &replacement, TVec3f{0.25F, 2.0F, 0.25F},
                                        TVec3f{0.0F, -4.0F, 0.0F}) &&
                replacement.mIdx != stale_triangle.mIdx &&
                FloorCode{}.getCode(&replacement) == CollisionFloorCode_Ice,
            "stable Triangle identities must not alias a new stage's .pa ownership");

    replacement_collision.deactivate();
    std::cout << "Binder/KCL Mario walking surface passed\n";
    return 0;
}
