#include "resource/KCollisionResource.hpp"
#include "resource/JMapResource.hpp"
#include "resource/BcsvTable.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
    using Bytes = std::vector<std::uint8_t>;

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    template<class Exception = std::invalid_argument, class Function>
    void rejects(Function function) {
        try {
            function();
        } catch (const Exception&) {
            return;
        }
        throw std::runtime_error("Invalid native resource was accepted.");
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
        for (std::size_t i = 0; i < 3; ++i) {
            put_float(bytes, offset + i * 4, value[i]);
        }
    }

    Bytes attributes() {
        Bytes bytes(36);
        put32(bytes, 0, 2);
        put32(bytes, 4, 1);
        put32(bytes, 8, 28);
        put32(bytes, 12, 4);
        put32(bytes, 16, smgpc::resource::jmap_hash("Floor_code"));
        put32(bytes, 20, 0xffffffffU);
        put32(bytes, 28, 7);
        put32(bytes, 32, 19);
        return bytes;
    }

    Bytes leaf_tree() {
        Bytes tree(12);
        put32(tree, 0, 0x80000004U);
        put16(tree, 4, 0x1234);
        put16(tree, 6, 1);
        put16(tree, 8, 2);
        return tree;
    }

    Bytes kcl(Bytes tree = leaf_tree()) {
        constexpr std::size_t first_prism = 116;
        constexpr std::size_t octree = first_prism + 32;
        Bytes bytes(octree + tree.size());
        put32(bytes, 0, 56);
        put32(bytes, 4, 68);
        put32(bytes, 8, first_prism - 16);
        put32(bytes, 12, octree);
        put_float(bytes, 16, 0.75f);
        put_vector(bytes, 20, {-2, -3, -4});
        put32(bytes, 32, 0xfffffffcU);
        put32(bytes, 36, 0xfffffffcU);
        put32(bytes, 40, 0xfffffffcU);
        put32(bytes, 44, 2);
        put_vector(bytes, 56, {3, 4, 5});
        put_vector(bytes, 68, {0, 0, 1});
        put_vector(bytes, 80, {-1, 0, 0});
        put_vector(bytes, 92, {0, -1, 0});
        put_vector(bytes, 104, {0.5f, 0.5f, 0});
        for (std::size_t i = 0; i < 2; ++i) {
            const auto offset = first_prism + i * 16;
            put_float(bytes, offset, i == 0 ? 1.0f : -2.0f);
            put16(bytes, offset + 8, 1);
            put16(bytes, offset + 10, 2);
            put16(bytes, offset + 12, 3);
            put16(bytes, offset + 14, i == 0 ? 1 : 0);
        }
        std::copy(tree.begin(), tree.end(), bytes.begin() + octree);
        return bytes;
    }

    void equal_vector(const TVec3f& actual, std::array<float, 3> expected) {
        require(std::abs(actual.x - expected[0]) < 0.00001f && std::abs(actual.y - expected[1]) < 0.00001f &&
                std::abs(actual.z - expected[2]) < 0.00001f, "Original KCollision vertex reconstruction differs.");
    }

    void actual_constructor_init_and_geometry() {
        auto bytes = kcl();
        const auto pa = attributes();
        smgpc::resource::KCollisionResource resource(bytes, pa);
        KCollisionServer server;
        std::unique_ptr<JMapInfo> map_owner(server.mapInfo);
        require(server.mFile == nullptr && server.mapInfo != nullptr && server.mMaxVertexDistance == 1,
                "Original constructor state is preserved.");
        server.init(resource.native_file(), resource.attributes_data());
        require(server.mFile == resource.native_file(), "Server attaches the actual decoded KCL header.");
        require(server.getTriangleNum() == 2, "Prism sentinel is excluded from triangle count.");
        require(server.getPrismData(0) == server.mFile->mPrisms + 1, "Original local prism zero follows its sentinel.");
        require(server.toIndex(server.getPrismData(1)) == 1, "Original toIndex returns a local prism index.");
        equal_vector(server.getPos(server.getPrismData(0), 0), {3, 4, 5});
        equal_vector(server.getPos(server.getPrismData(0), 1), {5, 4, 5});
        equal_vector(server.getPos(server.getPrismData(0), 2), {3, 6, 5});
        equal_vector(server.getPos(server.getPrismData(1), 1), {-1, 4, 5});
        equal_vector(server.getPos(server.getPrismData(1), 2), {3, 0, 5});
        equal_vector(*server.getFaceNormal(server.getPrismData(0)), {0, 0, 1});
        s32 code = -1;
        require(server.getAttributes(0).getValue("Floor_code", &code) && code == 19, "PA uses each local prism's attribute index.");
        require(server.getAttributes(1).getValue("Floor_code", &code) && code == 7, "Second prism reaches a different PA row.");
        require(server.mFile->mThickness == 0.75f && server.mFile->mBlockWidthShift == 2 &&
                server.mFile->mXMask == -4, "Header metadata is decoded without clamping.");
        equal_vector(server.mFile->mMin, {-2, -3, -4});
        require(resource.source_offsets() == std::array<std::uint32_t, 4>{56, 68, 100, 148}, "Raw offsets remain available separately.");
        require(KCollisionServer::isBinaryInitialized(resource.native_file()), "Native typed header is recognized.");
        require(!KCollisionServer::isBinaryInitialized(bytes.data()), "Raw binary bytes are not mistaken for a native header.");
        rejects([&] { server.setData(bytes.data()); });
        require(server.mFile == resource.native_file(), "Rejected raw attachment preserves prior server data.");
    }

    void retained_mutable_resource_lifetime() {
        auto server = [] {
            auto bytes = kcl();
            auto pa = attributes();
            smgpc::resource::KCollisionResource resource(bytes, pa);
            auto shared = resource;
            require(shared.native_file() == resource.native_file(), "Resource copies share typed storage.");
            shared.native_file()->mPrisms[1].mHeight = 3;
            require(resource.source_bytes()[119] == 0, "Prism mutation does not rewrite big-endian source bytes.");
            require(std::equal(bytes.begin(), bytes.end(), resource.source_bytes().begin()), "All source bytes remain immutable.");
            std::fill(bytes.begin(), bytes.end(), 0xff);
            std::fill(pa.begin(), pa.end(), 0xff);
            return std::make_unique<smgpc::resource::OwnedKCollisionServer>(shared);
        }();
        equal_vector(server->server().getPos(server->server().getPrismData(0), 1), {9, 4, 5});
        s32 code = -1;
        require(server->server().getAttributes(0).getValue("Floor_code", &code) && code == 19,
                "Actual server retains decoded KCL and PA after caller resources and buffers are destroyed.");
        JMapInfo retained = *server->server().mapInfo;
        server.reset();
        require(retained.getValue(1, "Floor_code", &code) && code == 19, "Attached JMap data owns its decoded table independently.");
    }

    void jmap_attach_identity_and_null_semantics() {
        JMapInfo first;
        first.setName("retained name");
        {
            smgpc::resource::JMapResource table(attributes());
            JMapInfo second;
            require(first.attach(table.data()) && second.attach(table.data()), "Both original attaches accept the bounded resource.");
            require(first == second && first.mData == second.mData, "Repeated attachment shares original table identity.");
            require(!first.attach(nullptr) && first == second, "Null attachment leaves existing data intact.");
            auto unregistered = attributes();
            rejects([&] { first.attach(unregistered.data()); });
        }
        require(std::string_view(first.getName()) == "retained name", "Attachment does not overwrite JMap name.");
        s32 code = -1;
        require(first.getValue(0, "Floor_code", &code) && code == 7, "Attached table survives its file-handle lifetime.");
    }

    void original_octree_search_and_leaf_indices() {
        Bytes tree(48);
        put32(tree, 0, 4);
        for (std::size_t child = 0; child < 8; ++child) {
            put32(tree, 4 + child * 4, 0x80000000U | (child == 7 ? 38 : 32));
        }
        put16(tree, 36, 0xbeef);
        put16(tree, 38, 1);
        put16(tree, 42, 0xabcd);
        put16(tree, 44, 2);
        smgpc::resource::OwnedKCollisionServer owner{smgpc::resource::KCollisionResource(kcl(tree))};
        auto& server = owner.server();
        for (u32 child = 0; child < 8; ++child) {
            const u32 x = (child & 1) * 2, y = ((child >> 1) & 1) * 2, z = ((child >> 2) & 1) * 2;
            s32 shift = -1;
            const auto* leaf = reinterpret_cast<const u16*>(server.searchBlock(&shift, x, y, z));
            require(shift == 1 && leaf[1] == (child == 7 ? 2 : 1) && leaf[2] == 0,
                    "Original searchBlock follows decoded relative child offsets and native u16 lists.");
            require(leaf[0] == (child == 7 ? 0xabcd : 0xbeef), "Unused leaf prefixes retain their native halfword value.");
        }
    }

    void single_root_and_overlapping_unused_prefix() {
        Bytes tree(8);
        put32(tree, 0, 0x80000002U);
        put16(tree, 4, 1);
        auto bytes = kcl(tree);
        put32(bytes, 48, 0xffffffffU);
        put32(bytes, 52, 0xffffffffU);
        smgpc::resource::OwnedKCollisionServer owner{smgpc::resource::KCollisionResource(bytes)};
        auto& server = owner.server();
        const u32 x = 3, y = 2, z = 1;
        s32 shift = -1;
        const auto* leaf = reinterpret_cast<const u16*>(server.searchBlock(&shift, x, y, z));
        require(server.mFile->mBlockXShift == -1 && server.mFile->mBlockXYShift == -1, "Special layout shift metadata is preserved.");
        require(reinterpret_cast<const std::uint8_t*>(leaf) - static_cast<const std::uint8_t*>(server.mFile->mOctree) == 2,
                "Leaf prefix can overlap the preceding node word without relocating its offset.");
        require(shift == 2 && leaf[1] == 1 && leaf[2] == 0, "Original leaf traversal skips the overlapping prefix.");
    }

    void multiple_root_blocks() {
        Bytes tree(44);
        for (std::size_t root = 0; root < 8; ++root) {
            put32(tree, root * 4, 0x80000000U | (root == 7 ? 38 : 32));
        }
        put16(tree, 34, 1);
        put16(tree, 40, 2);
        auto bytes = kcl(tree);
        put32(bytes, 44, 1);
        put32(bytes, 48, 1);
        put32(bytes, 52, 2);
        smgpc::resource::OwnedKCollisionServer owner{smgpc::resource::KCollisionResource(bytes)};
        const u32 lower = 0, upper = 3;
        s32 shift = -1;
        auto& server = owner.server();
        auto* leaf = reinterpret_cast<u16*>(server.searchBlock(&shift, lower, lower, lower));
        require(shift == 1 && leaf[1] == 1, "First root block selects its own list.");
        leaf = reinterpret_cast<u16*>(server.searchBlock(&shift, upper, upper, upper));
        require(shift == 1 && leaf[1] == 2, "XYZ root indexing preserves authored shift axes.");
    }

    void malformed_resource_boundaries() {
        rejects([] { smgpc::resource::KCollisionResource resource(Bytes(55)); });
        auto bad = kcl();
        put32(bad, 4, 69);
        rejects([&] { smgpc::resource::KCollisionResource resource(bad); });
        bad = kcl();
        put16(bad, 120, 5);
        rejects([&] { smgpc::resource::KCollisionResource resource(bad); });
        bad = kcl();
        bad.resize(bad.size() - 2);
        rejects([&] { smgpc::resource::KCollisionResource resource(bad); });
        bad = kcl();
        put16(bad, 154, 3);
        rejects([&] { smgpc::resource::KCollisionResource resource(bad); });
        bad = kcl();
        put32(bad, 148, 0);
        rejects([&] { smgpc::resource::KCollisionResource resource(bad); });
        bad = kcl();
        put32(bad, 44, 32);
        rejects([&] { smgpc::resource::KCollisionResource resource(bad); });
        rejects<std::runtime_error>([] { smgpc::resource::JMapResource resource(Bytes(4)); });
    }

    void absent_and_out_of_range_attributes() {
        smgpc::resource::OwnedKCollisionServer absent{smgpc::resource::KCollisionResource(kcl())};
        require(!absent.server().getAttributes(0).isValid(), "Absent PA remains absent without fabricated default rows.");
        auto bytes = kcl();
        put16(bytes, 130, 99);
        smgpc::resource::OwnedKCollisionServer sparse{smgpc::resource::KCollisionResource(bytes, attributes())};
        require(!sparse.server().getAttributes(0).isValid() && sparse.server().getAttributes(1).isValid(),
                "An original out-of-range PA index remains invalid without clamping or changing other prisms.");
    }

    void empty_prism_array() {
        auto bytes = kcl();
        bytes.resize(124);
        put32(bytes, 12, 116);
        std::fill(bytes.begin() + 116, bytes.end(), 0);
        put32(bytes, 116, 0x80000004U);
        smgpc::resource::OwnedKCollisionServer owner{smgpc::resource::KCollisionResource(bytes)};
        const u32 coordinate = 0;
        s32 shift = -1;
        const auto* leaf = reinterpret_cast<const u16*>(owner.server().searchBlock(&shift, coordinate, coordinate, coordinate));
        require(owner.server().getTriangleNum() == 0 && leaf[1] == 0,
                "An empty original prism array retains its sentinel and terminating octree list.");
    }

    void incompatible_live_node_leaf_alias_is_explicit() {
        Bytes tree(4);
        // The Wii could read the low halfword of this same node as an empty
        // leaf. A flat little-endian image cannot give it both byte orders.
        put32(tree, 0, 0x80000000U);
        rejects([&] { smgpc::resource::KCollisionResource resource(kcl(tree)); });
    }
}

int main() {
    const std::array<std::pair<std::string_view, std::function<void()>>, 10> tests{{
        {"actual constructor, init, and geometry", actual_constructor_init_and_geometry},
        {"retained mutable resource lifetime", retained_mutable_resource_lifetime},
        {"JMap attach identity and null semantics", jmap_attach_identity_and_null_semantics},
        {"original octree search and leaf indices", original_octree_search_and_leaf_indices},
        {"single root and overlapping unused prefix", single_root_and_overlapping_unused_prefix},
        {"multiple root blocks", multiple_root_blocks},
        {"malformed resource boundaries", malformed_resource_boundaries},
        {"absent and out-of-range attributes", absent_and_out_of_range_attributes},
        {"empty original prism array", empty_prism_array},
        {"incompatible live node/leaf alias is explicit", incompatible_live_node_leaf_alias_is_explicit},
    }};
    std::size_t failures = 0;
    for (const auto& [name, test] : tests) {
        try {
            test();
            std::cout << "PASS " << name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "FAIL " << name << ": " << error.what() << '\n';
        }
    }
    return failures == 0 ? 0 : 1;
}
