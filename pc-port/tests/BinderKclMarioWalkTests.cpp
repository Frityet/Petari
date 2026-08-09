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
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
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

    void test_binder_contact_skin_across_convex_seam() {
        constexpr auto cRadius = 0.5F;
        constexpr auto cContactSkin = 1.2F;
        constexpr auto cInitialSkinPenetration = 0.1F;
        constexpr auto cScale = 100.0F;
        constexpr auto cSinTenDegrees = 0.17364818F;
        constexpr auto cCosTenDegrees = 0.98480775F;
        constexpr auto cLocalAcross = 0.1F;
        constexpr auto cLocalAlong = 0.2F;

        // The two transformed copies share the local z edge. The right face
        // slopes down away from that edge, making it a convex support seam.
        constexpr auto left_face = std::array<float, 12U>{
            -cScale, 0.0F, 0.0F, 0.0F,
            0.0F, 1.0F, 0.0F, 0.0F,
            0.0F, 0.0F, cScale, 0.0F,
        };
        constexpr auto right_face = std::array<float, 12U>{
            cScale * cCosTenDegrees, cSinTenDegrees, 0.0F, 0.0F,
            -cScale * cSinTenDegrees, cCosTenDegrees, 0.0F, 0.0F,
            0.0F, 0.0F, cScale, 0.0F,
        };

        auto collision = smgpc::scene::StageCollisionService{};
        const auto kcl = make_single_triangle_kcl();
        require(collision.add_kcl(kcl, left_face, "skin-seam-left.kcl") &&
                    collision.add_kcl(kcl, right_face, "skin-seam-right.kcl"),
                "the convex skin-seam fixture must register both KCL faces");
        collision.build();
        collision.activate();

        const auto left_normal = TVec3f{0.0F, 1.0F, 0.0F};
        const auto right_normal =
            TVec3f{cSinTenDegrees, cCosTenDegrees, 0.0F};
        const auto left_surface = TVec3f{
            -cScale * cLocalAcross,
            0.0F,
            cScale * cLocalAlong,
        };
        const auto right_surface = TVec3f{
            cScale * cCosTenDegrees * cLocalAcross,
            -cScale * cSinTenDegrees * cLocalAcross,
            cScale * cLocalAlong,
        };
        const auto skin_query_distance =
            cRadius + cContactSkin - cInitialSkinPenetration;
        const auto left_skin_center =
            left_surface + left_normal * skin_query_distance;
        const auto right_skin_center =
            right_surface + right_normal * skin_query_distance;

        require(collision.sphere_contacts(left_skin_center, cRadius).empty() &&
                    collision.sphere_contacts(right_skin_center, cRadius).empty(),
                "Binder's contact skin must not change public exact-radius sphere queries");

        auto position = left_skin_center;
        const auto gravity = TVec3f{0.0F, -1.0F, 0.0F};
        auto binder = Binder(nullptr, &position, &gravity, cRadius, 0.0F, 8U);

        const auto settle = binder.bind(TVec3f{});
        position.add(settle);
        const auto left_shell_center =
            left_surface + left_normal * (cRadius + cContactSkin);
        require(binder.isBindedGround() &&
                    position.epsilonEquals(left_shell_center, 0.0005F) &&
                    binder.mGroundInfo.mParentTriangle.getNormal(0)->epsilonEquals(
                        left_normal, 0.0001F) &&
                    collision.sphere_contacts(position, cRadius).empty(),
                "a skin-only left-face contact must classify as ground without changing exact queries");

        // Mario's retail Binder skips the initial overlap check. Starting on
        // the left shell must therefore reach and classify the next face in a
        // single short sweep, rather than retaining the old support plane.
        binder._1EC._3 = true;
        const auto cross_seam = right_skin_center - position;
        const auto crossed = binder.bind(cross_seam);
        position.add(crossed);
        const auto right_shell_center =
            right_surface + right_normal * (cRadius + cContactSkin);
        require(binder.isBindedGround() && binder.mPlaneNum == 1 &&
                    position.epsilonEquals(right_shell_center, 0.0005F) &&
                    binder.mGroundInfo.mParentTriangle.getNormal(0)->epsilonEquals(
                        right_normal, 0.0001F),
                "skip-initial Binder motion must carry ground classification across the convex seam");
        require(collision.sphere_contacts(position, cRadius).empty() &&
                    binder.mFixReactionVector.epsilonEquals(
                        right_normal * cInitialSkinPenetration, 0.0005F),
                "the seam contact must use one 1.2-skin correction while exact queries remain empty");

        collision.deactivate();
    }

    void test_minimum_norm_gateway_contact_reaction() {
        constexpr auto cRadius = 60.0F;
        constexpr auto cContactSkin = 1.2F;
        constexpr auto cSurfaceScale = 400.0F;
        constexpr auto cLocalX = 0.1F;
        constexpr auto cLocalZ = 0.2F;
        constexpr auto normals = std::array<TVec3f, 8U>{
            TVec3f{0.0131106F, -0.799246F, -0.600861F},
            TVec3f{-0.00212449F, -0.804581F, -0.593839F},
            TVec3f{0.00979495F, -0.759085F, -0.650917F},
            TVec3f{0.0155864F, -0.765469F, -0.643284F},
            TVec3f{-0.0966284F, -0.739551F, -0.666129F},
            TVec3f{-0.107744F, -0.743717F, -0.659755F},
            TVec3f{-0.129962F, -0.772808F, -0.621190F},
            TVec3f{-0.118979F, -0.783882F, -0.609403F},
        };
        constexpr auto penetrations = std::array<float, 8U>{
            1.09743F, 1.09850F, 1.09686F, 1.09692F,
            1.30540F, 1.93950F, 1.44741F, 1.93824F,
        };

        const auto make_surface_matrix = [](TVec3f normal, float penetration) {
            normal.scale(1.0F / normal.length());
            auto tangent_x = TVec3f{0.0F, normal.z, -normal.y};
            tangent_x.scale(1.0F / tangent_x.length());
            const auto tangent_z = TVec3f{
                tangent_x.y * normal.z - tangent_x.z * normal.y,
                tangent_x.z * normal.x - tangent_x.x * normal.z,
                tangent_x.x * normal.y - tangent_x.y * normal.x,
            };
            const auto plane_distance = cRadius + cContactSkin - penetration;
            const auto interior = tangent_x * (cSurfaceScale * cLocalX) +
                                  tangent_z * (cSurfaceScale * cLocalZ);
            const auto translation = normal * -plane_distance - interior;
            return std::array<float, 12U>{
                tangent_x.x * cSurfaceScale, normal.x * cSurfaceScale,
                tangent_z.x * cSurfaceScale, translation.x,
                tangent_x.y * cSurfaceScale, normal.y * cSurfaceScale,
                tangent_z.y * cSurfaceScale, translation.y,
                tangent_x.z * cSurfaceScale, normal.z * cSurfaceScale,
                tangent_z.z * cSurfaceScale, translation.z,
            };
        };

        auto collision = smgpc::scene::StageCollisionService{};
        const auto kcl = make_single_triangle_kcl();
        for (auto index = std::size_t{}; index < normals.size(); ++index) {
            require(collision.add_kcl(
                        kcl, make_surface_matrix(normals[index], penetrations[index]),
                        "gateway-reaction-" + std::to_string(index) + ".kcl"),
                    "each traced Gateway contact plane must register");
        }
        collision.build();

        const auto seated = collision.move_sphere(
            TVec3f{}, TVec3f{}, cRadius, normals.size(), true);
        require(seated.contacts.size() == normals.size(),
                "the traced Gateway manifold must reproduce all eight floor contacts");
        for (const auto& contact : seated.contacts) {
            require(contact.normal.dot(seated.fix_reaction) >=
                        contact.penetration - 0.001F,
                    "the minimum-norm correction must satisfy every traced contact half-space");
        }
        require(seated.fix_reaction.epsilonEquals(
                    TVec3f{-0.21672F, -1.47067F, -1.24651F}, 0.01F) &&
                    seated.fix_reaction.length() < 1.95F &&
                    seated.displacement.epsilonEquals(seated.fix_reaction, 0.0001F),
                "the traced eight-plane response must choose the bounded minimum-norm correction");

        const auto idle = collision.move_sphere(
            seated.displacement, TVec3f{}, cRadius, normals.size(), true);
        require(!idle.contacts.empty() && idle.displacement.length() < 0.01F,
                "the minimum-norm seating correction must retain a zero-motion support manifold");
    }
}  // namespace

int main() {
    test_binder_contact_skin_across_convex_seam();
    test_minimum_norm_gateway_contact_reaction();

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

    constexpr auto binder_radius = 0.5F;
    const auto skin_support_center = TVec3f{0.25F, 1.69F, 0.25F};
    require(collision.sphere_contacts(skin_support_center, binder_radius).empty(),
            "public exact sphere queries must not inflate their physical radius by Binder skin");
    const auto skin_support = collision.move_sphere(
        skin_support_center, TVec3f{}, binder_radius, 8U);
    require(!skin_support.contacts.empty() &&
                skin_support.contacts.front().normal.epsilonEquals(
                    TVec3f{0.0F, 1.0F, 0.0F}, 0.0001F) &&
                skin_support.contacts.front().penetration >= 0.0F &&
                skin_support.contacts.front().penetration < 0.02F &&
                skin_support.displacement.length() < 0.02F,
            "Binder motion queries must retain a real floor within the 1.2-unit contact skin");

    auto large_world_matrix = identity;
    large_world_matrix[3U] = 14760.0F;
    large_world_matrix[7U] = -10676.2255859375F;
    large_world_matrix[11U] = 6770.0F;
    auto large_world_collision = smgpc::scene::StageCollisionService{};
    require(large_world_collision.add_kcl(
                kcl, large_world_matrix, "large-world-shell.kcl"),
            "the Gateway-scale shell fixture must register");
    large_world_collision.build();
    constexpr auto contact_skin = 1.2F;
    const auto exact_shell_y =
        large_world_matrix[7U] + binder_radius + contact_skin;
    const auto large_world_shell = TVec3f{
        large_world_matrix[3U] + 0.25F,
        std::nextafter(exact_shell_y, std::numeric_limits<float>::infinity()),
        large_world_matrix[11U] + 0.25F,
    };
    require(large_world_collision.sphere_contacts(
                large_world_shell, binder_radius).empty(),
            "large-coordinate Binder tolerance must not change public exact sphere queries");
    const auto large_world_support = large_world_collision.move_sphere(
        large_world_shell, TVec3f{}, binder_radius, 8U, true);
    require(!large_world_support.contacts.empty() &&
                large_world_support.contacts.front().penetration == 0.0F &&
                large_world_support.displacement.epsilonEquals(TVec3f{}, 0.0001F),
            "an exact Binder skin shell one large-coordinate ulp outside must remain support");

    const auto overlapping_center = TVec3f{0.25F, 0.4F, 0.25F};
    const auto moving_away = TVec3f{0.0F, 2.0F, 0.0F};
    const auto checked_initial = collision.move_sphere(
        overlapping_center, moving_away, binder_radius, 8U, false);
    const auto skipped_initial = collision.move_sphere(
        overlapping_center, moving_away, binder_radius, 8U, true);
    require(!checked_initial.contacts.empty() && skipped_initial.contacts.empty() &&
                skipped_initial.displacement.epsilonEquals(moving_away, 0.0001F),
            "Mario's retail skip-initial Binder flag must ignore takeoff contacts and test the endpoint");

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
