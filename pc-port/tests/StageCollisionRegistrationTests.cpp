#include "scene/StageCollisionService.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <exception>
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

    void write_be16(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint16_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value);
    }

    void write_be32(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint32_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 24U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value >> 16U);
        bytes[offset + 2U] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 3U] = static_cast<std::uint8_t>(value);
    }

    void write_be_float(std::vector<std::uint8_t> &bytes, std::size_t offset, float value) {
        write_be32(bytes, offset, std::bit_cast<std::uint32_t>(value));
    }

    [[nodiscard]] std::vector<std::uint8_t> make_single_triangle_kcl() {
        constexpr auto position_offset = 0x38U;
        constexpr auto normal_offset = 0x44U;
        constexpr auto prism_offset = 0x74U;
        constexpr auto octree_offset = 0x84U;
        auto bytes = std::vector<std::uint8_t>(0x88U, 0U);
        write_be32(bytes, 0x00U, position_offset);
        write_be32(bytes, 0x04U, normal_offset);
        write_be32(bytes, 0x08U, prism_offset - 0x10U);
        write_be32(bytes, 0x0cU, octree_offset);
        write_be_float(bytes, 0x10U, 2.0F);

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
        write_be16(bytes, prism_offset + 14U, 7U);
        return bytes;
    }

    constexpr auto cIdentity = std::array<float, 12U>{
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
        0.0F,
        0.0F,
        0.0F,
        1.0F,
        0.0F,
    };

    void test_collision_is_absent_without_explicit_registration() {
        auto collision = smgpc::scene::StageCollisionService{};
        collision.build();

        const auto movement = TVec3f{3.0F, -4.0F, 5.0F};
        const auto moved = collision.move_sphere(TVec3f{}, movement, 50.0F);
        require(collision.empty(), "a new stage collision registry must remain empty");
        require(collision.stats().mesh_count == 0U && collision.stats().triangle_count == 0U &&
                    collision.stats().rejected_triangle_count == 0U,
                "absence must not be represented as a synthesized or rejected collision mesh");
        require(!collision.line_cast(TVec3f{0.0F, 1.0F, 0.0F}, TVec3f{0.0F, -2.0F, 0.0F}),
                "line queries must miss when CollisionParts has registered nothing");
        require(collision.sphere_contacts(TVec3f{}, 50.0F).empty(),
                "sphere queries must have no contacts when collision is absent");
        require(moved.contacts.empty() && moved.displacement.epsilonEquals(movement, 0.0001F),
                "binder movement must remain unobstructed when collision is absent");
    }

    void test_only_explicit_valid_kcl_registration_adds_collision() {
        auto collision = smgpc::scene::StageCollisionService{};
        const auto malformed = std::array<std::uint8_t, 4U>{};
        require(!collision.add_kcl(malformed, cIdentity, "/ObjectData/Missing.arc:/Missing.kcl") &&
                    collision.empty(),
                "a missing or malformed exact CollisionParts resource must remain absent");

        const auto kcl = make_single_triangle_kcl();
        require(collision.add_kcl(kcl, cIdentity, "/ObjectData/Exact.arc:/Exact/Exact.kcl"),
                "an explicit valid CollisionParts KCL registration should be accepted");
        require(!collision.line_cast(TVec3f{0.25F, 1.0F, 0.25F}, TVec3f{0.0F, -2.0F, 0.0F}),
                "an explicit registration should not be queryable until its owner completes the build");

        collision.build();
        auto hit = smgpc::scene::StageCollisionHit{};
        require(collision.stats().mesh_count == 1U && collision.stats().triangle_count == 1U &&
                    collision.line_cast(TVec3f{0.25F, 1.0F, 0.25F}, TVec3f{0.0F, -2.0F, 0.0F}, &hit) &&
                    hit.attribute == 7U,
                "only the explicitly registered exact KCL should become stage collision");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };
}  // namespace

int main() {
    constexpr auto tests = std::array{
        TestCase{"collision absent without registration", test_collision_is_absent_without_explicit_registration},
        TestCase{"only explicit valid KCL registers", test_only_explicit_valid_kcl_registration_adds_collision},
    };

    auto failures = 0;
    for (const auto &test : tests) {
        try {
            test.run();
            std::cout << "[ok] " << test.name << '\n';
        } catch (const std::exception &error) {
            ++failures;
            std::cerr << "[fail] " << test.name << ": " << error.what() << '\n';
        }
    }
    if (failures != 0) {
        std::cerr << failures << " stage collision registration test(s) failed\n";
        return 1;
    }
    std::cout << tests.size() << " stage collision registration test(s) passed\n";
    return 0;
}
