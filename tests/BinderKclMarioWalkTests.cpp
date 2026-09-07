#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/HitSensor.hpp"
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
#include <memory>
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

    void test_binder_contact_margin_across_convex_seam() {
        constexpr auto cRadius = 0.5F;
        constexpr auto cContactMargin = 1.2F;
        constexpr auto cPenetration = 0.1F;
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
        require(collision.add_kcl(kcl, left_face, "seam-left.kcl") &&
                    collision.add_kcl(kcl, right_face, "seam-right.kcl"),
                "the convex seam fixture must register both KCL faces");
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
        const auto contact_distance = cRadius - cPenetration;
        const auto left_contact_center =
            left_surface + left_normal * contact_distance;
        const auto right_contact_center =
            right_surface + right_normal * contact_distance;

        require(!collision.sphere_contacts(left_contact_center, cRadius).empty() &&
                    !collision.sphere_contacts(right_contact_center, cRadius).empty(),
                "each seam endpoint must overlap the true collision radius");

        auto position = left_contact_center;
        const auto gravity = TVec3f{0.0F, -1.0F, 0.0F};
        auto binder = Binder(nullptr, &position, &gravity, cRadius, 0.0F, 8U);

        const auto settle = binder.bind(TVec3f{});
        position.add(settle);
        const auto left_shell_center =
            left_surface + left_normal * (cRadius + cContactMargin);
        require(binder.isBindedGround() &&
                    position.epsilonEquals(left_shell_center, 0.0005F) &&
                    binder.mGroundInfo.mParentTriangle.getNormal(0)->epsilonEquals(
                        left_normal, 0.0001F) &&
                    collision.sphere_contacts(position, cRadius).empty(),
                "a true left-face contact receives the original 1.2 post-contact margin");
        const auto idle = binder.bind(TVec3f{});
        require(!binder.isBindedGround() && binder.mPlaneNum == 0U &&
                    idle.epsilonEquals(TVec3f{}, 0.0F),
                "zero motion outside the true radius must not retain a margin-only ground contact");

        // Mario's retail Binder skips the initial overlap check. Starting on
        // the left shell must therefore reach and classify the next face in a
        // single short sweep, rather than retaining the old support plane.
        binder._1EC._3 = true;
        const auto cross_seam = right_contact_center - position;
        const auto crossed = binder.bind(cross_seam);
        position.add(crossed);
        const auto right_shell_center =
            right_surface + right_normal * (cRadius + cContactMargin);
        require(binder.isBindedGround() && binder.mPlaneNum == 1 &&
                    position.epsilonEquals(right_shell_center, 0.0005F) &&
                    binder.mGroundInfo.mParentTriangle.getNormal(0)->epsilonEquals(
                        right_normal, 0.0001F),
                "skip-initial Binder motion must carry ground classification across the convex seam");
        require(collision.sphere_contacts(position, cRadius).empty() &&
                    binder._1EC._3 &&
                    binder.mFixReactionVector.epsilonEquals(
                        right_normal * (cPenetration + cContactMargin), 0.0005F),
                "the new true-radius contact adds penetration and margin while skip-initial persists");

        collision.deactivate();
    }

    void test_original_margin_and_retry_flags() {
        auto collision = smgpc::scene::StageCollisionService{};
        constexpr auto floor = std::array<float, 12U>{
            100.0F, 0.0F, 0.0F, 0.0F,
            0.0F, 1.0F, 0.0F, 0.0F,
            0.0F, 0.0F, 100.0F, 0.0F,
        };
        require(collision.add_kcl(make_single_triangle_kcl(), floor, "margin-flags.kcl"),
                "margin flag fixture must load a real floor");
        collision.build();
        collision.activate();
        const auto position = TVec3f{10.0F, 0.4F, 20.0F};
        const auto gravity = TVec3f{0.0F, -1.0F, 0.0F};
        const auto requested = TVec3f{2.0F, 1.0F, 0.0F};
        auto binder = Binder(nullptr, &position, &gravity, 0.5F, 0.0F, 4U);
        binder._1EC._5 = true;
        const auto without_margin = binder.bind(requested);
        require(without_margin.epsilonEquals(TVec3f{0.0F, 0.1F, 0.0F}, 0.0001F) &&
                    binder.isBindedGround() && !binder._1EC._5 &&
                    std::abs(binder.mGroundInfo._60 - 0.1F) < 0.0001F,
                "the one-shot no-margin flag disables both margin and unconsumed movement retry");
        const auto with_margin = binder.bind(requested);
        require(with_margin.epsilonEquals(TVec3f{2.0F, 2.3F, 0.0F}, 0.0001F) &&
                    std::abs(binder.mGroundInfo._60 - 1.3F) < 0.0001F,
                "the next bind restores margin and the original projected retry");
        collision.deactivate();
    }

    void test_original_plane_copy_storage_modes() {
        auto collision = smgpc::scene::StageCollisionService{};
        auto owner = LiveActor("plane-copy-owner");
        owner.makeActorAppeared();
        auto registration = std::make_shared<smgpc::scene::StageCollisionRegistrationState>(&owner.mFlag.mIsDead);
        auto sensors = std::array{HitSensor(0U, 0U, 1.0F, &owner),
                                  HitSensor(0U, 0U, 1.0F, &owner),
                                  HitSensor(0U, 0U, 1.0F, &owner)};
        constexpr auto floor = std::array<float, 12U>{
            1.0F, 0.0F, 0.0F, 0.0F,
            0.0F, 1.0F, 0.0F, 0.0F,
            0.0F, 0.0F, 1.0F, 0.0F,
        };
        constexpr auto wall = std::array<float, 12U>{
            0.0F, 1.0F, 0.0F, 0.0F,
            1.0F, 0.0F, 0.0F, 0.0F,
            0.0F, 0.0F, 1.0F, 0.0F,
        };
        constexpr auto roof = std::array<float, 12U>{
            1.0F, 0.0F, 0.0F, 0.0F,
            0.0F, -1.0F, 0.0F, 0.5F,
            0.0F, 0.0F, 1.0F, 0.0F,
        };
        const auto kcl = make_single_triangle_kcl();
        require(collision.register_kcl(kcl, floor, "copy-floor.kcl", registration, {}, &sensors[0]).accepted &&
                    collision.register_kcl(kcl, wall, "copy-wall.kcl", registration, {}, &sensors[2]).accepted &&
                    collision.register_kcl(kcl, roof, "copy-roof.kcl", registration, {}, &sensors[1]).accepted,
                "plane-copy fixture must retain three real source sensors");
        collision.build();
        collision.activate();
        const auto position = TVec3f{0.25F, 0.25F, 0.25F};
        const auto gravity = TVec3f{0.0F, -1.0F, 0.0F};
        for (const auto capacity : {0U, 3U}) {
            auto binder = Binder(nullptr, &position, &gravity, 0.5F, 0.0F, capacity);
            binder._1EC._5 = true;
            (void)binder.bind(TVec3f{});
            require(binder.mPlaneNum == 3U && binder.isBindedGround() &&
                        binder.isBindedWall() && binder.isBindedRoof() &&
                        ((capacity == 0U) == (binder.mPlane == nullptr)),
                    "zero allocation still records temporary hits and cached ground, wall, and roof");
            // The original second argument does not bound writes. Always
            // allocate for the full result even while proving it is ignored.
            auto copied = std::array<HitInfo*, 4U>{};
            copied[3] = &binder.mGroundInfo;
            require(binder.copyPlaneArrayAndSortingSensor(copied.data(), 1U) == 3U &&
                        copied[0]->mParentTriangle.mSensor == &sensors[2] &&
                        copied[1]->mParentTriangle.mSensor == &sensors[1] &&
                        copied[2]->mParentTriangle.mSensor == &sensors[0] &&
                        copied[3] == &binder.mGroundInfo,
                    "original plane copying returns every hit sorted by descending sensor pointer");
            if (capacity == 0U) {
                require(copied[0] == &binder.mWallInfo && copied[1] == &binder.mRoofInfo &&
                            copied[2] == &binder.mGroundInfo,
                        "zero-allocation copying must return retained category caches");
            } else {
                require(copied[0] == &binder.mPlane[1] && copied[1] == &binder.mPlane[2] &&
                            copied[2] == &binder.mPlane[0],
                        "allocated-plane copying must return the actual plane array entries");
            }
        }
        registration->release_owner();
        collision.deactivate();
    }

    void test_component_extrema_contact_reaction() {
        constexpr auto cRadius = 10.0F;
        constexpr auto cContactMargin = 1.2F;
        constexpr auto cSurfaceScale = 400.0F;
        constexpr auto cLocalX = 0.1F;
        constexpr auto cLocalZ = 0.2F;
        struct Plane {
            TVec3f normal;
            float penetration;
        };
        struct Fixture {
            std::array<Plane, 3U> planes;
            std::size_t count;
            TVec3f expected;
            const char* name;
        };
        const auto fixtures = std::array{
            // Stored depths include the 1.2 post-contact margin. Reactions
            // are (1.2,1.6,0), (0,1.8,2.4), (-1.6,0,1.2).
            // Each component retains its largest positive and negative value.
            Fixture{{Plane{{0.6F, 0.8F, 0.0F}, 2.0F},
                     Plane{{0.0F, 0.6F, 0.8F}, 3.0F},
                     Plane{{-0.8F, 0.0F, 0.6F}, 2.0F}},
                    3U, {-0.4F, 1.8F, 2.4F}, "nonorthogonal"},
            // Opposing penetrations intentionally cannot both be resolved by
            // a displacement. Retail still adds the component extrema.
            Fixture{{Plane{{1.0F, 0.0F, 0.0F}, 3.0F},
                     Plane{{-1.0F, 0.0F, 0.0F}, 1.5F}, Plane{}},
                    2U, {1.5F, 0.0F, 0.0F}, "opposing"},
        };

        const auto make_surface_matrix = [](TVec3f normal, float penetration) {
            normal.scale(1.0F / normal.length());
            const auto reference = std::fabs(normal.x) < 0.9F
                                       ? TVec3f{1.0F, 0.0F, 0.0F}
                                       : TVec3f{0.0F, 1.0F, 0.0F};
            auto tangent_x = normal.cross(reference);
            tangent_x.scale(1.0F / tangent_x.length());
            const auto tangent_z = tangent_x.cross(normal);
            const auto plane_distance = cRadius + cContactMargin - penetration;
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

        const auto kcl = make_single_triangle_kcl();
        const auto position = TVec3f{};
        const auto gravity = TVec3f{0.0F, -1.0F, 0.0F};
        auto binder = Binder(nullptr, &position, &gravity, cRadius, 0.0F, 3U);
        for (const auto& fixture : fixtures) {
            auto collision = smgpc::scene::StageCollisionService{};
            for (auto index = std::size_t{}; index < fixture.count; ++index) {
                const auto& plane = fixture.planes[index];
                require(collision.add_kcl(
                            kcl, make_surface_matrix(plane.normal, plane.penetration),
                            std::string(fixture.name) + std::to_string(index) + ".kcl"),
                        "each synthetic contact plane must register");
            }
            collision.build();
            const auto moved = collision.move_sphere(
                position, TVec3f{}, cRadius, fixture.count, true);
            require(moved.contacts.size() == fixture.count,
                    "the synthetic fixture must retain every contact");
            require(moved.fix_reaction.epsilonEquals(fixture.expected, 0.001F) &&
                        moved.displacement.epsilonEquals(fixture.expected, 0.001F),
                    "contact response must add positive maxima and negative minima per component");

            auto hits = std::array<HitInfo, 3U>{};
            for (auto index = std::size_t{}; index < fixture.count; ++index) {
                const auto& contact = moved.contacts[index];
                require(contact.moving_reaction.epsilonEquals(TVec3f{}, 0.0F),
                        "a static contact must have no collision-part movement reaction");
                hits[index].mParentTriangle.mNormals[0].set(contact.normal);
                hits[index]._60 = contact.penetration;
                hits[index]._7C.set(contact.moving_reaction);
            }
            binder.mPlaneNum = static_cast<u32>(fixture.count);
            auto original_reaction = TVec3f{};
            binder.obtainMomentFixReaction(hits.data(), 0U, &original_reaction, 0U);
            require(original_reaction.epsilonEquals(moved.fix_reaction, 0.00001F),
                    "shared static collision response must agree with the actual original Binder method");
        }
    }

    void test_original_reaction_range_and_moving_parts() {
        const auto position = TVec3f{};
        const auto gravity = TVec3f{0.0F, -1.0F, 0.0F};
        auto binder = Binder(nullptr, &position, &gravity, 1.0F, 0.0F, 4U);
        auto planes = std::array<HitInfo, 4U>{};
        // Both sentinels would dominate every component if included.
        for (const auto index : {0U, 3U}) {
            planes[index].mParentTriangle.mNormals[0].set(1.0F, 1.0F, 1.0F);
            planes[index]._60 = 1000.0F;
        }
        planes[1].mParentTriangle.mNormals[0].set(0.6F, 0.8F, 0.0F);
        planes[1]._60 = 2.0F;
        planes[1]._7C.set(4.0F, -3.0F, 0.5F);
        planes[2].mParentTriangle.mNormals[0].set(-0.8F, 0.0F, 0.6F);
        planes[2]._60 = 1.0F;
        planes[2]._7C.set(-2.0F, 1.0F, -5.0F);
        binder.mPlaneNum = 3;

        auto reaction = TVec3f{99.0F, 99.0F, 99.0F};
        binder._1EC._1 = false;
        binder.obtainMomentFixReaction(planes.data(), 1U, &reaction, 1U);
        require(reaction.epsilonEquals(TVec3f{0.4F, 1.6F, 0.6F}, 0.00001F),
                "original reaction uses start through mPlaneNum and ignores the second argument");

        binder._1EC._1 = true;
        binder.obtainMomentFixReaction(planes.data(), 0U, &reaction, 1U);
        require(reaction.epsilonEquals(TVec3f{2.0F, -1.4F, -4.4F}, 0.00001F),
                "enabled moving-part reactions share the face reaction component extrema");

        binder.obtainMomentFixReaction(planes.data(), 4U, &reaction, 3U);
        require(reaction.epsilonEquals(TVec3f{}, 0.0F),
                "an empty original reaction range replaces previous output with zero");
    }
}  // namespace

int main() {
    test_binder_contact_margin_across_convex_seam();
    test_original_margin_and_retry_flags();
    test_original_plane_copy_storage_modes();
    test_component_extrema_contact_reaction();
    test_original_reaction_range_and_moving_parts();

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
                point_info.mParentTriangle.mIdx == floor_triangle.mIdx &&
                point_info.mParentTriangle.getNormal(0)->epsilonEquals(
                    TVec3f{0.0F, 1.0F, 0.0F}, 0.0001F) &&
                point_info._7C.epsilonEquals(TVec3f{}, 0.0F),
            "a static point query must retain triangle face normal and zero moving reaction");
    require(Collision::checkStrikeBallToMap(TVec3f{10.25F, 5.25F, 0.25F}, 0.5F,
                                            nullptr, nullptr) > 0 &&
                Collision::getStrikeInfoMap(0) != nullptr,
            "Mario's wall sphere query must publish a real HitInfo buffer");
    const auto* sphere_info = Collision::getStrikeInfoMap(0);
    require(sphere_info->mParentTriangle.getNormal(0)->epsilonEquals(
                TVec3f{1.0F, 0.0F, 0.0F}, 0.0001F) &&
                sphere_info->_7C.epsilonEquals(TVec3f{}, 0.0F),
            "a static sphere strike stores its normal in Triangle and no moving reaction in HitInfo");

    constexpr auto binder_radius = 0.5F;
    const auto separated_center = TVec3f{0.25F, 1.69F, 0.25F};
    const auto separated = collision.move_sphere(
        separated_center, TVec3f{}, binder_radius, 8U);
    require(collision.sphere_contacts(separated_center, binder_radius).empty() &&
                separated.contacts.empty() &&
                separated.displacement.epsilonEquals(TVec3f{}, 0.0F),
            "Binder must not detect a separated floor by inflating its query radius");
    const auto shallow_contact = collision.move_sphere(
        TVec3f{0.25F, 0.49F, 0.25F}, TVec3f{}, binder_radius, 8U);
    require(shallow_contact.contacts.size() == 1U &&
                std::abs(shallow_contact.contacts.front().penetration - 1.21F) < 0.0001F &&
                shallow_contact.displacement.epsilonEquals(TVec3f{0.0F, 1.21F, 0.0F}, 0.0001F),
            "a true 0.01 overlap receives 1.2 additional penetration after detection");

    auto large_world_matrix = identity;
    large_world_matrix[3U] = 14760.0F;
    large_world_matrix[7U] = -10676.2255859375F;
    large_world_matrix[11U] = 6770.0F;
    auto large_world_collision = smgpc::scene::StageCollisionService{};
    require(large_world_collision.add_kcl(
                kcl, large_world_matrix, "large-world-radius.kcl"),
            "the large-coordinate radius fixture must register");
    large_world_collision.build();
    const auto exact_contact_y = large_world_matrix[7U] + binder_radius;
    const auto outside_radius = TVec3f{
        large_world_matrix[3U] + 0.25F,
        std::nextafter(exact_contact_y, std::numeric_limits<float>::infinity()),
        large_world_matrix[11U] + 0.25F,
    };
    require(large_world_collision.sphere_contacts(
                outside_radius, binder_radius).empty(),
            "a sphere one large-coordinate ulp outside its radius is separated");
    const auto outside_move = large_world_collision.move_sphere(
        outside_radius, TVec3f{}, binder_radius, 8U, true);
    require(outside_move.contacts.empty() &&
                outside_move.displacement.epsilonEquals(TVec3f{}, 0.0F),
            "original Binder must not add a host epsilon to recover separated support");

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
