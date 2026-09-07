#include "Game/Map/HitInfo.hpp"
#include "Game/Util/MapUtil.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "resource/BcsvTable.hpp"
#include "scene/StageCollisionService.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
    using Bytes = std::vector<std::uint8_t>;
    using Collision = smgpc::scene::StageCollisionService;
    using Registration = smgpc::scene::StageCollisionRegistrationState;
    constexpr auto identity = std::array<float, 12>{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};

    void require(bool condition, std::string_view message) {
        if (!condition) throw std::runtime_error(std::string(message));
    }
    template<class Exception = std::invalid_argument, class Function>
    void rejects(Function function) {
        try { function(); } catch (const Exception&) { return; }
        throw std::runtime_error("Invalid area query was accepted.");
    }
    void put16(Bytes& bytes, std::size_t offset, std::uint16_t value) {
        bytes.at(offset) = value >> 8;
        bytes.at(offset + 1) = value;
    }
    void put32(Bytes& bytes, std::size_t offset, std::uint32_t value) {
        bytes.at(offset) = value >> 24;
        bytes.at(offset + 1) = value >> 16;
        bytes.at(offset + 2) = value >> 8;
        bytes.at(offset + 3) = value;
    }
    void put_float(Bytes& bytes, std::size_t offset, float value) {
        put32(bytes, offset, std::bit_cast<std::uint32_t>(value));
    }
    void put_vector(Bytes& bytes, std::size_t offset, std::array<float, 3> value) {
        for (std::size_t i = 0; i < 3; ++i) put_float(bytes, offset + i * 4, value[i]);
    }
    Bytes attributes() {
        Bytes bytes(40);
        put32(bytes, 0, 3);
        put32(bytes, 4, 1);
        put32(bytes, 8, 28);
        put32(bytes, 12, 4);
        put32(bytes, 16, smgpc::resource::jmap_hash("Floor_code"));
        put32(bytes, 20, 0xffffffffU);
        put32(bytes, 28, 7);
        put32(bytes, 32, 19);
        put32(bytes, 36, 3);
        return bytes;
    }

    Bytes kcl(bool branch = false) {
        // All four prisms use (0,0,0), +Z and three edge axes. Positive
        // prism vertices span 4, 2 and 3 units respectively; prism four is
        // authored inactive. The leaf order deliberately differs from file
        // order and repeats a prism both within and across octree leaves.
        constexpr std::size_t first_prism = 116;
        constexpr std::size_t octree = first_prism + 64;
        Bytes tree(branch ? 56 : 18);
        if (branch) {
            put32(tree, 0, 4);
            for (std::size_t child = 0; child < 8; ++child)
                put32(tree, 4 + child * 4, 0x80000000U | (child == 7 ? 44 : 32));
            put16(tree, 36, 0x1234);
            for (std::size_t i = 0; i < 4; ++i) put16(tree, 38 + i * 2, std::array{3U, 1U, 3U, 4U}[i]);
            put16(tree, 48, 0x5678);
            put16(tree, 50, 2);
            put16(tree, 52, 3);
        } else {
            put32(tree, 0, 0x80000004U);
            put16(tree, 4, 0x1234);
            for (std::size_t i = 0; i < 5; ++i) put16(tree, 6 + i * 2, std::array{3U, 1U, 3U, 4U, 2U}[i]);
        }
        Bytes bytes(octree + tree.size());
        put32(bytes, 0, 56);
        put32(bytes, 4, 68);
        put32(bytes, 8, first_prism - 16);
        put32(bytes, 12, octree);
        put_float(bytes, 16, 20.0F); // Area queries must not include the slab thickness.
        put_vector(bytes, 20, {-16, -16, -16});
        put32(bytes, 32, 0xffffffe0U);
        put32(bytes, 36, 0xffffffe0U);
        put32(bytes, 40, 0xffffffe0U);
        put32(bytes, 44, 5);
        put_vector(bytes, 56, {0, 0, 0});
        put_vector(bytes, 68, {0, 0, 1});
        put_vector(bytes, 80, {-1, 0, 0});
        put_vector(bytes, 92, {0, -1, 0});
        put_vector(bytes, 104, {0.5F, 0.5F, 0});
        for (std::size_t i = 0; i < 4; ++i) {
            const auto offset = first_prism + i * 16;
            put_float(bytes, offset, std::array{2.0F, 1.0F, 1.5F, -1.0F}[i]);
            put16(bytes, offset + 8, 1);
            put16(bytes, offset + 10, 2);
            put16(bytes, offset + 12, 3);
            put16(bytes, offset + 14, i % 3);
        }
        std::copy(tree.begin(), tree.end(), bytes.begin() + octree);
        return bytes;
    }
    std::array<TVec3f, 2> full_box() { return {TVec3f(-1, -1, -1), TVec3f(5, 5, 1)}; }
    std::vector<std::uint32_t> local_indices(const Collision& collision, const std::vector<std::uint32_t>& identities) {
        std::vector<std::uint32_t> result;
        for (const auto id : identities) result.push_back(collision.surface(id)->prism_index);
        return result;
    }
    void octree_order_capacity_and_boundary_geometry() {
        for (const bool branch : {false, true}) {
            Collision collision;
            require(collision.add_kcl(kcl(branch), identity, "authored order"), "Actual KCL must register.");
            collision.build();
            const auto box = full_box();
            require(local_indices(collision, collision.area_polygons(box, 512)) == std::vector<std::uint32_t>{2, 0, 1},
                    "Octree encounter order, repeated leaf deduplication and negative-height filtering differ.");
            require(local_indices(collision, collision.area_polygons(box, 2)) == std::vector<std::uint32_t>{2, 0},
                    "Capacity must stop at the original first encounters, not file/BVH order.");
            require(collision.area_polygons(box, 0).empty(), "Zero capacity must never write a prism.");
        }
        Collision collision;
        require(collision.add_kcl(kcl(), identity), "Area geometry must register.");
        const auto point = std::array{TVec3f(3, 3, 0)};
        require(local_indices(collision, collision.area_polygons(point, 512)) == std::vector<std::uint32_t>{2, 0, 1},
                "Collapsed local axes expand by one; AABB corner overlap is not convex clipping.");
        const auto far_z = std::array{TVec3f(1, 1, -2.01F)};
        require(collision.area_polygons(far_z, 512).empty(), "Area query must not include KCL thickness behind the face.");
        const auto edge = std::array{TVec3f(5, 0, 0)};
        require(collision.area_polygons(edge, 512).empty(), "Original part bounding sphere culls before collapsed-axis expansion.");
        const auto outside = std::array{TVec3f(-100, -100, -100), TVec3f(-90, -90, -90)};
        require(collision.area_polygons(outside, 512).empty(), "A box outside the KCL integer grid must miss.");
    }
    void zone_order_and_owner_lifetime() {
        Collision collision;
        auto dead = std::make_unique<bool>(false);
        const auto first = std::make_shared<Registration>(dead.get());
        const auto second = std::make_shared<Registration>();
        const auto global = std::make_shared<Registration>();
        const auto fourth = std::make_shared<Registration>();
        const auto bytes = kcl();
        require(collision.register_kcl(bytes, identity, "zone7", first, {}, nullptr, 7).accepted, "Zone seven registers.");
        require(collision.register_kcl(bytes, identity, "zone2-first", second, {}, nullptr, 2).accepted, "Zone two registers.");
        require(collision.register_kcl(bytes, identity, "global", global, {}, nullptr, 0).accepted, "Global zone registers.");
        require(collision.register_kcl(bytes, identity, "zone2-second", fourth, {}, nullptr, 2).accepted, "Second zone two registers.");
        collision.activate();
        auto box = full_box();
        auto output = std::array<Triangle, 16>{};
        require(MR::createAreaPolygonListArray(output.data(), output.size(), box.data(), box.size()) == 12,
                "Every enabled part must contribute its three active prisms.");
        const auto expected = std::array<std::string_view, 4>{"global", "zone2-first", "zone2-second", "zone7"};
        for (std::size_t i = 0; i < expected.size(); ++i)
            require(collision.surface(output[i * 3].mIdx)->source_name == expected[i], "Numeric zone then part insertion order differs.");
        const auto retained = output[9];
        *dead = true;
        require(!retained.isValid() && collision.area_polygons(box, 512).size() == 9, "Original actor death must disable cached and newly queried surfaces.");
        *dead = false;
        require(retained.isValid(), "A live unchanged owner revalidates the same surface identity.");
        global->set_enabled(false);
        require(collision.surface(collision.area_polygons(box, 1).front())->source_name == "zone2-first",
                "Disabled parts must be rejected before applying capacity.");
        first->release_owner();
        dead.reset();
        require(!retained.isValid() && collision.area_polygons(box, 512).size() == 6,
                "Released owner flags must never be read after actor storage disappears.");
        collision.clear();
        require(!retained.isValid(), "Scene clear must retire previously returned Triangle identities.");
        require(collision.add_kcl(bytes, identity), "Fresh resource may register after clear.");
        require(!retained.isValid(), "New registration must not recycle the retired surface identity.");
    }
    void zone_erase_swap_and_revalidation_append() {
        Collision collision;
        const auto a = std::make_shared<Registration>();
        const auto b = std::make_shared<Registration>();
        const auto c = std::make_shared<Registration>();
        for (const auto& [name, state] : std::array{
                 std::pair{"A", a}, std::pair{"B", b}, std::pair{"C", c}}) {
            require(collision.register_kcl(kcl(), identity, name, state, {}, nullptr, 4).accepted,
                    "Ordered zone membership must register.");
        }
        const auto names = [&] {
            const auto ids = collision.area_polygons(full_box(), 512);
            std::vector<std::string> names;
            for (std::size_t i = 0; i < ids.size(); i += 3)
                names.emplace_back(collision.surface(ids[i])->source_name);
            return names;
        };
        require(names() == std::vector<std::string>{"A", "B", "C"}, "Initial zone membership must append.");
        a->set_enabled(false);
        require(names() == std::vector<std::string>{"C", "B"}, "Original erase must swap the last member into the removed slot.");
        a->set_enabled(true);
        require(names() == std::vector<std::string>{"C", "B", "A"}, "Original revalidation must append to current membership order.");
        // No query between these transitions: their original ordering still
        // matters to the next capacity-limited collection.
        c->set_enabled(false);
        c->set_enabled(true);
        require(names() == std::vector<std::string>{"A", "B", "C"}, "Membership transitions must be applied synchronously.");
        a->release_owner();
        require(names() == std::vector<std::string>{"C", "B"}, "Owner release must remove the same original zone membership.");
        require(collision.surface(collision.area_polygons(full_box(), 1).front())->source_name == "C",
                "Capacity must observe the mutated zone order.");
        collision.clear();
        // A registration may outlive and be reused with a later collision
        // scene; expired membership observers must not reference old vectors.
        require(collision.register_kcl(kcl(), identity, "new", b, {}, nullptr, 4).accepted,
                "A surviving registration may attach to the cleared service.");
        b->set_enabled(false);
        require(collision.area_polygons(full_box(), 512).empty(), "Retired membership observers must not affect the new registration.");
    }
    void affine_transform_attributes_and_retention() {
        Collision collision;
        // Rotation about Z plus unequal scale and translation. Bounds must
        // be formed after each point is transformed into part space.
        const auto matrix = std::array<float, 12>{0, -2, 0, 20, 3, 0, 0, -10, 0, 0, 0.5F, 7};
        const auto state = std::make_shared<Registration>();
        auto bytes = kcl();
        auto pa = attributes();
        require(collision.register_kcl(bytes, matrix, "transformed", state, pa, nullptr, 5).accepted, "Affine KCL must register.");
        std::fill(bytes.begin(), bytes.end(), 0xff);
        std::fill(pa.begin(), pa.end(), 0xff);
        collision.activate();
        auto box = std::array{TVec3f(21, -11, 6.9F), TVec3f(11, 3, 7.1F)};
        auto output = std::array<Triangle, 4>{};
        output[3].mIdx = 123456;
        require(MR::createAreaPolygonListArray(output.data(), output.size(), box.data(), box.size()) == 3,
                "Part-local AABB must collect all transformed prisms from retained source bytes.");
        require(output[3].mIdx == 123456, "Unwritten capacity must remain unchanged.");
        const auto& first = output[0];
        require(first.isValid() && first.mParts == nullptr && first.getHostPlacementZoneID() == 5,
                "The original Triangle API must use genuine source identity and zone provenance.");
        require(first.mPos[0].epsilonEquals(TVec3f(20, -10, 7), 0.00001F) &&
                    first.mPos[1].epsilonEquals(TVec3f(20, -1, 7), 0.00001F) &&
                    first.mPos[2].epsilonEquals(TVec3f(14, -10, 7), 0.00001F), "Returned original vertices must remain in world space.");
        s32 code = -1;
        require(first.getAttributes().getValue("Floor_code", &code) && code == 3,
                "Original local prism attributes must survive source-buffer retirement.");
    }
    void host_allocation_and_invalid_contracts() {
        using namespace smgpc::compat;
        Collision collision;
        auto heaps = JkrHeapRuntime::create(2U << 20);
        auto game = JkrAllocationDomain::create(heaps, 64U << 10);
        const auto bytes = kcl();
        const auto box = full_box();
        std::vector<std::uint32_t> retained;
        {
            JkrAllocationScope callback(game);
            const auto free_before = game->heap().getFreeSize();
            require(collision.add_kcl(bytes, identity, "host"), "Registration from Game callback must work.");
            collision.build();
            retained = collision.area_polygons(box, 512);
            require(JKRHeap::findFromRoot(retained.data()) == nullptr && game->heap().getFreeSize() == free_before,
                    "Retained collision bookkeeping, decoded octree and query output must escape the Game heap.");
        }
        game.reset();
        require(collision.area_polygons(box, 512) == retained, "Collision geometry must survive the caller Game arena.");
        const auto huge = std::array{TVec3f(-1.0e20F, -1.0e20F, -1.0e20F), TVec3f(1.0e20F, 1.0e20F, 1.0e20F)};
        require(collision.area_polygons(huge, 512) == retained,
                "Gekko coordinate conversion must saturate and clamp large finite queries to the original grid.");
        rejects([&] { (void)collision.area_polygons(box, 513); });
        const auto excess = std::array<TVec3f, 33>{};
        rejects([&] { (void)collision.area_polygons(excess, 1); });
        const auto nan = std::array{TVec3f(std::numeric_limits<float>::quiet_NaN(), 0, 0)};
        rejects([&] { (void)collision.area_polygons(nan, 1); });
        Collision malformed;
        auto invalid = kcl();
        put32(invalid, 180, 0x7ffffffcU);
        require(malformed.add_kcl(invalid, identity), "Geometry-only registration need not decode the octree.");
        rejects([&] { (void)malformed.area_polygons(box, 1); });
        rejects<std::logic_error>([&] { (void)MR::createAreaPolygonListArray(nullptr, 0, nullptr, 0); });
    }
}

int main() {
    struct Test { const char* name; void (*run)(); };
    const auto tests = std::array{
        Test{"octree order, capacity and boundary geometry", octree_order_capacity_and_boundary_geometry},
        Test{"zone order and owner lifetime", zone_order_and_owner_lifetime},
        Test{"zone erase swap and revalidation append", zone_erase_swap_and_revalidation_append},
        Test{"affine geometry, attributes and retention", affine_transform_attributes_and_retention},
        Test{"host allocation and invalid query contracts", host_allocation_and_invalid_contracts},
    };
    auto failures = 0;
    for (const auto& test : tests) {
        try { test.run(); std::cout << "[ok] " << test.name << '\n'; }
        catch (const std::exception& error) { ++failures; std::cerr << "[fail] " << test.name << ": " << error.what() << '\n'; }
    }
    return failures == 0 ? 0 : 1;
}
