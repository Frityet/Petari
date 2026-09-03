#include "Game/LiveActor/Binder.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Util/MapUtil.hpp"
#include "Game/Util/TriangleFilter.hpp"
#include "scene/StageCollisionService.hpp"

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
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

    void require_vector(const TVec3f& actual, const TVec3f& expected, std::string_view message) {
        require(std::isfinite(actual.x) && std::isfinite(actual.y) && std::isfinite(actual.z) &&
                    actual.epsilonEquals(expected, 0.0001F), message);
    }

    void write_u32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value) {
        for (auto index = 0U; index < 4U; ++index) {
            bytes[offset + index] = static_cast<std::uint8_t>(value >> ((3U - index) * 8U));
        }
    }

    void write_u16(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint16_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value);
    }

    void write_float(std::vector<std::uint8_t>& bytes, std::size_t offset, float value) {
        write_u32(bytes, offset, std::bit_cast<std::uint32_t>(value));
    }

    std::vector<std::uint8_t> make_kcl() {
        // One source prism: positions (0,0,0), (1,0,0), (0,0,1).
        auto bytes = std::vector<std::uint8_t>(0x88U, 0U);
        write_u32(bytes, 0x00U, 0x38U);
        write_u32(bytes, 0x04U, 0x44U);
        write_u32(bytes, 0x08U, 0x64U);
        write_u32(bytes, 0x0cU, 0x84U);
        write_float(bytes, 0x10U, 10.0F);
        const auto normal = [&](std::size_t offset, const TVec3f& vector) {
            write_float(bytes, offset, vector.x);
            write_float(bytes, offset + 4U, vector.y);
            write_float(bytes, offset + 8U, vector.z);
        };
        normal(0x44U, TVec3f(0.0F, 1.0F, 0.0F));
        normal(0x50U, TVec3f(1.0F, 0.0F, 0.0F));
        normal(0x5cU, TVec3f(0.0F, 0.0F, 1.0F));
        normal(0x68U, TVec3f(0.70710678118F, 0.0F, 0.70710678118F));
        write_float(bytes, 0x74U, 0.70710678118F);
        write_u16(bytes, 0x7cU, 1U);
        write_u16(bytes, 0x7eU, 2U);
        write_u16(bytes, 0x80U, 3U);
        return bytes;
    }

    std::array<float, 12U> floor_matrix(float height = 0.0F) {
        return {100.0F, 0.0F, 0.0F, 0.0F,
                0.0F, 1.0F, 0.0F, height,
                0.0F, 0.0F, 100.0F, 0.0F};
    }

    struct CollisionOwner {
        explicit CollisionOwner(const char* name)
            : actor(name), sensor(0U, 0U, 1.0F, &actor),
              registration(std::make_shared<smgpc::scene::StageCollisionRegistrationState>(&actor.mFlag.mIsDead)) {
            actor.makeActorAppeared();
        }

        ~CollisionOwner() {
            registration->release_owner();
        }

        bool add(smgpc::scene::StageCollisionService& collision,
                 std::span<const std::uint8_t> kcl, const std::array<float, 12U>& matrix,
                 std::int32_t zone_id = 0) {
            return collision.register_kcl(kcl, matrix, "/ObjectData/Shared.arc:/Shared.kcl",
                                          registration, {}, &sensor, zone_id).accepted;
        }

        LiveActor actor;
        HitSensor sensor;
        std::shared_ptr<smgpc::scene::StageCollisionRegistrationState> registration;
    };

    class RejectAll final : public TriangleFilterBase {
    public:
        bool isInvalidTriangle(const Triangle* triangle) const override {
            require(triangle != nullptr && triangle->isValid(), "the filter must receive a real live triangle");
            ++calls;
            return true;
        }
        mutable std::size_t calls = 0U;
    };

    class RejectNamed final : public TriangleFilterBase {
    public:
        explicit RejectNamed(std::string_view name) : _name(name) {}
        bool isInvalidTriangle(const Triangle* triangle) const override {
            const auto* name = triangle->getHostName();
            return name != nullptr && std::string_view(name) == _name;
        }
    private:
        std::string_view _name;
    };

    void test_filtered_floor_and_wall_do_not_resolve() {
        const auto kcl = make_kcl();
        const auto gravity = TVec3f(0.0F, -1.0F, 0.0F);
        for (const auto wall : {false, true}) {
            auto collision = smgpc::scene::StageCollisionService{};
            const auto matrix = wall
                                    ? std::array<float, 12U>{0.0F, 1.0F, 0.0F, 0.0F,
                                                            -100.0F, 0.0F, 0.0F, 0.0F,
                                                            0.0F, 0.0F, 100.0F, 0.0F}
                                    : floor_matrix();
            require(collision.add_kcl(kcl, matrix, "rejected.kcl"), "the floor/wall prism must register");
            collision.build();
            collision.activate();
            auto position = wall ? TVec3f(2.0F, -10.0F, 10.0F) : TVec3f(10.0F, 2.0F, 10.0F);
            const auto movement = wall ? TVec3f(-1.0F, 0.0F, 0.0F) : TVec3f(0.0F, -1.0F, 0.0F);
            auto binder = Binder(nullptr, &position, &gravity, 1.0F, 0.0F, 1U);
            auto filter = RejectAll{};
            binder.setTriangleFilter(&filter);
            require_vector(binder.bind(movement), movement, "a rejected prism must not push or stop Binder movement");
            require_vector(binder.mFixReactionVector, TVec3f{}, "a rejected prism must not contribute a reaction");
            require(filter.calls != 0U && binder.mPlaneNum == 0 &&
                        !binder.isBindedGround() && !binder.isBindedWall() && !binder.isBindedRoof(),
                    "a rejected prism must leave no classified or retained contact");
            binder.setTriangleFilter(nullptr);
            const auto resolved = binder.bind(movement);
            require(!resolved.epsilonEquals(movement, 0.0001F) && binder.mPlaneNum != 0 &&
                        (wall ? binder.isBindedWall() : binder.isBindedGround()),
                    "the same accepted prism must still resolve and classify the collision");
        }
    }

    void test_filters_preserve_later_contacts() {
        auto collision = smgpc::scene::StageCollisionService{};
        const auto kcl = make_kcl();
        auto rejected = CollisionOwner("RejectedOwner");
        auto accepted = CollisionOwner("AcceptedOwner");
        // Separate registrations mirror the category keeper's output capacity:
        // rejected parts must not consume the accepted HitInfo slots.
        for (auto index = 0U; index < 32U; ++index) {
            require(rejected.add(collision, kcl, floor_matrix()), "the rejected prism must register");
        }
        require(accepted.add(collision, kcl, floor_matrix()), "the later accepted prism must register");
        collision.build();
        collision.activate();
        auto filter = RejectNamed("RejectedOwner");
        const auto center = TVec3f(10.0F, 0.5F, 10.0F);
        const auto assert_strike = [&] {
            require(Collision::getStrikeInfoNumMap() == 1U &&
                        std::string_view(Collision::getStrikeInfoMap(0U)->mParentTriangle.getHostName()) == "AcceptedOwner",
                    "the original strike-info API must retain the later accepted prism");
        };
        require(Collision::checkStrikeBallToMap(center, 1.0F, nullptr, &filter) == 1,
                "filtered sphere contacts must not exhaust the output capacity");
        assert_strike();
        require(Collision::checkStrikeBallToMapWithThickness(center, 1.0F, 2.0F, nullptr, &filter) == 1,
                "thickness queries must use the same triangle filter before capacity");
        assert_strike();
        const auto gravity = TVec3f(0.0F, -1.0F, 0.0F);
        auto binder = Binder(nullptr, &center, &gravity, 1.0F, 0.0F, 1U);
        binder.setTriangleFilter(&filter);
        (void)binder.bind(TVec3f{});
        require(binder.isBindedGround() && binder.mPlaneNum == 1 &&
                    std::string_view(binder.mGroundInfo.mParentTriangle.getHostName()) == "AcceptedOwner",
                "Binder's one-plane capacity must remain available to the accepted floor");
    }

    void test_filtered_nearest_line_returns_farther_hit() {
        auto collision = smgpc::scene::StageCollisionService{};
        const auto kcl = make_kcl();
        auto nearest = CollisionOwner("NearestOwner");
        auto farther = CollisionOwner("FartherOwner");
        require(nearest.add(collision, kcl, floor_matrix(1.0F)) &&
                    farther.add(collision, kcl, floor_matrix()), "both line-query floors must register");
        collision.build();
        collision.activate();
        auto position = TVec3f{};
        auto triangle = Triangle{};
        const auto start = TVec3f(10.0F, 3.0F, 10.0F);
        const auto offset = TVec3f(0.0F, -5.0F, 0.0F);
        require(MR::getFirstPolyOnLineToMap(&position, &triangle, start, offset), "the unfiltered ray must hit");
        require_vector(position, TVec3f(10.0F, 1.0F, 10.0F), "the unfiltered ray must hit the nearest floor");
        auto filter = RejectNamed("NearestOwner");
        require(MR::getFirstPolyOnLineToMap(&position, &triangle, start, offset, nullptr, &filter),
                "rejecting the nearest floor must still find the farther accepted floor");
        require_vector(position, TVec3f(10.0F, 0.0F, 10.0F), "the filtered ray must return the accepted hit position");
        require(std::string_view(triangle.getHostName()) == "FartherOwner", "the filtered ray must return the accepted actor");
    }

    void test_owner_name_and_zone_remain_separate_from_resources() {
        auto collision = smgpc::scene::StageCollisionService{};
        const auto kcl = make_kcl();
        auto lower = CollisionOwner("LowerActor");
        auto upper = std::make_unique<CollisionOwner>("UpperActor");
        require(lower.add(collision, kcl, floor_matrix(), 5) &&
                    upper->add(collision, kcl, floor_matrix(2.0F), 9),
                "two actors in different zones must be able to share the exact same KCL resource");
        collision.build();
        collision.activate();
        const auto read_triangle = [](const TVec3f& start) {
            auto triangle = Triangle{};
            auto position = TVec3f{};
            require(MR::getFirstPolyOnLineToMap(&position, &triangle, start, TVec3f(0.0F, -1.5F, 0.0F)),
                    "the owner fixture must return its live floor triangle");
            return triangle;
        };
        auto lower_triangle = read_triangle(TVec3f(10.0F, 1.0F, 10.0F));
        auto upper_triangle = read_triangle(TVec3f(10.0F, 3.0F, 10.0F));
        require(std::string_view(lower_triangle.getHostName()) == "LowerActor" &&
                    std::string_view(upper_triangle.getHostName()) == "UpperActor" &&
                    lower_triangle.getHostPlacementZoneID() == 5 && upper_triangle.getHostPlacementZoneID() == 9,
                "Triangle must retain each actual sensor host and its creation zone");
        const auto lower_surface = collision.surface(lower_triangle.mIdx);
        const auto upper_surface = collision.surface(upper_triangle.mIdx);
        require(lower_surface.has_value() && upper_surface.has_value() &&
                    lower_surface->source_name == "/ObjectData/Shared.arc:/Shared.kcl" &&
                    lower_surface->source_name == upper_surface->source_name &&
                    lower_surface->sensor == &lower.sensor && upper_surface->sensor == &upper->sensor &&
                    lower_surface->placement_zone_id == 5 && upper_surface->placement_zone_id == 9,
                "diagnostic resource identity must remain unchanged and independent of actor/zone metadata");

        lower.actor.setName("RenamedLowerActor");
        require(std::string_view(lower_triangle.getHostName()) == "RenamedLowerActor",
                "the host getter must read the current NameObj name rather than a stale copied label");
        lower.sensor.mHost = nullptr;
        require(lower_triangle.getHostName() == nullptr && lower_triangle.getHostPlacementZoneID() == 5,
                "a missing sensor host has no name and does not change the part's retained zone");
        lower.sensor.mHost = &lower.actor;
        collision.build();
        require(lower_triangle.getHostPlacementZoneID() == 5 && upper_triangle.getHostPlacementZoneID() == 9,
                "rebuilding the query structure must preserve triangle owner provenance");

        upper->actor.makeActorDead();
        require(!upper_triangle.isValid() && upper_triangle.getHostName() == nullptr,
                "inactive actor collision must not publish stale owner pointers");
        upper->actor.makeActorAppeared();
        require(upper_triangle.isValid() && upper_triangle.getHostPlacementZoneID() == 9,
                "reappearing an actor must recover its original zone identity");
        upper.reset();
        require(!upper_triangle.isValid() && upper_triangle.getHostName() == nullptr,
                "released registrations must not dereference a destroyed sensor or actor");

        collision.clear();
        require(!lower_triangle.isValid() && lower_triangle.getHostName() == nullptr,
                "clearing the stage must withdraw previous triangle provenance");
        require(collision.add_kcl(kcl, floor_matrix(), "geometry-only.kcl"), "an unowned geometry fixture must still register");
        collision.build();
        const auto unowned = read_triangle(TVec3f(10.0F, 1.0F, 10.0F));
        require(unowned.getHostName() == nullptr && lower_triangle.getHostName() == nullptr,
                "geometry-only registrations must not pretend resource names are actor names or revive old triangles");
        auto zone_unavailable = false;
        try {
            (void)unowned.getHostPlacementZoneID();
        } catch (const std::logic_error&) {
            zone_unavailable = true;
        }
        require(zone_unavailable, "an absent authored zone must fail explicitly when requested");
    }

    void verify_transformed_geometry(const Triangle& triangle) {
        constexpr auto inverse_sqrt_five = 0.4472135955F;
        const auto vertices = std::array{TVec3f(10.0F, 20.0F, 30.0F),
                                         TVec3f(10.0F, 20.0F, 28.0F),
                                         TVec3f(14.0F, 20.0F, 30.0F)};
        const auto normals = std::array{TVec3f(0.0F, 1.0F, 0.0F),
                                        TVec3f(0.0F, 0.0F, -1.0F),
                                        TVec3f(1.0F, 0.0F, 0.0F),
                                        TVec3f(2.0F * inverse_sqrt_five, 0.0F, -inverse_sqrt_five)};
        for (auto index = 0; index < 3; ++index) {
            require_vector(*triangle.getPos(index), vertices[index], "Triangle::getPos must expose the transformed source vertex");
            require_vector(*triangle.getEdgeNormal(index), normals[index + 1],
                           "Triangle::getEdgeNormal must expose the transformed normalized source edge axis");
        }
        require_vector(*triangle.getFaceNormal(), normals[0], "Triangle::getFaceNormal must preserve the source axis");
        auto centroid = *triangle.getPos(0) + *triangle.getPos(1) + *triangle.getPos(2);
        centroid.scale(1.0F / 3.0F);
        require_vector(centroid, TVec3f(34.0F / 3.0F, 20.0F, 88.0F / 3.0F),
                       "original vertex-reading consumers must see the prism centroid, separate from the contact point");
    }

    class GeometryFilter final : public TriangleFilterBase {
    public:
        bool isInvalidTriangle(const Triangle* triangle) const override {
            verify_transformed_geometry(*triangle);
            ++calls;
            return false;
        }
        mutable std::size_t calls = 0U;
    };

    void test_original_queries_expose_source_geometry() {
        auto collision = smgpc::scene::StageCollisionService{};
        constexpr auto matrix = std::array<float, 12U>{0.0F, 0.0F, 4.0F, 10.0F,
                                                      0.0F, 3.0F, 0.0F, 20.0F,
                                                      -2.0F, 0.0F, 0.0F, 30.0F};
        require(collision.add_kcl(make_kcl(), matrix, "transformed.kcl"), "the rotated, scaled, translated prism must register");
        collision.build();
        collision.activate();
        auto filter = GeometryFilter{};
        auto position = TVec3f{};
        auto triangle = Triangle{};
        require(MR::getFirstPolyOnLineToMap(&position, &triangle, TVec3f(11.0F, 23.0F, 29.5F),
                                           TVec3f(0.0F, -6.0F, 0.0F), nullptr, &filter),
                "the geometry-reading original line filter must accept the transformed prism");
        verify_transformed_geometry(triangle);
        require_vector(position, TVec3f(11.0F, 20.0F, 29.5F), "the line contact must stay separate from the three vertices");
        require_vector(*triangle.calcAndGetPos(1), TVec3f(10.0F, 20.0F, 28.0F), "the position refresh API must preserve source geometry");
        require_vector(*triangle.calcAndGetEdgeNormal(1), TVec3f(1.0F, 0.0F, 0.0F), "the edge refresh API must preserve source geometry");
        const auto center = TVec3f(11.0F, 20.5F, 29.5F);
        require(Collision::checkStrikeBallToMap(center, 1.0F, nullptr, &filter) == 1,
                "the geometry-reading original sphere filter must accept the transformed prism");
        verify_transformed_geometry(Collision::getStrikeInfoMap(0)->mParentTriangle);
        const auto gravity = TVec3f(0.0F, -1.0F, 0.0F);
        auto binder = Binder(nullptr, &center, &gravity, 1.0F, 0.0F, 1U);
        binder.setTriangleFilter(&filter);
        (void)binder.bind(TVec3f{});
        require(binder.isBindedGround() && filter.calls >= 3U, "Binder must use the same geometry-aware filter");
        verify_transformed_geometry(binder.mGroundInfo.mParentTriangle);
        require_vector(binder.mGroundInfo.mHitPos, TVec3f(11.0F, 20.0F, 29.5F), "Binder must retain the actual contact position");
    }
}  // namespace

int main() {
    try {
        test_filtered_floor_and_wall_do_not_resolve();
        test_filters_preserve_later_contacts();
        test_filtered_nearest_line_returns_farther_hit();
        test_owner_name_and_zone_remain_separate_from_resources();
        test_original_queries_expose_source_geometry();
        std::cout << "[pass] collision triangle filters, source geometry, and owner provenance\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "[fail] " << exception.what() << '\n';
        return 1;
    }
}
