#include "Game/Effect/SyncBckEffectInfo.hpp"
#include "Game/Animation/XanimePlayer.hpp"
#include "Game/Animation/XanimeResource.hpp"
#include "Game/System/ResourceInfo.hpp"
#include "JSystem/J3DGraphAnimator/J3DModel.hpp"
#include "JSystem/J3DGraphAnimator/J3DJoint.hpp"
#include "JSystem/J3DGraphAnimator/J3DMtxCalc.hpp"
#include "compat/ResourceHolderCompat.hpp"
#include "resource/GameResourceRuntime.hpp"
#include "resource/RarcArchive.hpp"
#include <aurora/aurora.h>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <vector>

namespace aurora { extern AuroraConfig g_config; }
namespace {
    using namespace smgpc::resource;
    using namespace smgpc::compat;
    using Bytes = std::vector<std::uint8_t>;
    void require(bool ok, const char* message) {
        if (!ok) { JkrHostAllocationScope host; throw std::runtime_error(message); }
    }
    // RARC/ANK1 fixture builder copied from OriginalResourceHolderTests; model
    // fixture copied from OriginalXanimePlayerTests. Both invoke actual SDK and
    // Game owners. No partially decoded archive is represented as a full model.
    void put16(Bytes& b, std::size_t p, unsigned v) { b.at(p) = v >> 8; b.at(p + 1) = v; }
    void put32(Bytes& b, std::size_t p, std::uint32_t v) {
        put16(b, p, v >> 16); put16(b, p + 2, v);
    }
    void tag(Bytes& b, std::size_t p, std::string_view s) { std::memcpy(b.data() + p, s.data(), s.size()); }
    struct File { std::string name; Bytes bytes; bool nested = false; };
    std::shared_ptr<RarcArchive> archive(std::vector<File> input) {
        // Three real directory records: root, nested, empty, and their dot links.
        // Payload IDs deliberately differ from file-table indexes.
        Bytes strings;
        auto str = [&](std::string_view s) {
            auto p = strings.size(); strings.insert(strings.end(), s.begin(), s.end()); strings.push_back(0); return p;
        };
        auto root = str("fixture_root"), nest = str("nested"), empty = str("empty"), dot = str("."), parent = str("..");
        std::vector<std::size_t> names;
        for (const auto& f : input) names.push_back(str(f.name));
        const std::size_t roots = std::ranges::count_if(input, [](const auto& f) { return !f.nested; });
        const std::size_t count = input.size() + 8, dirs = 0x40, files = dirs + 48;
        const auto string_offset = files + count * 20;
        const auto data_offset = (string_offset + strings.size() + 31) & ~std::size_t{31};
        Bytes data;
        std::vector<std::size_t> offsets;
        for (const auto& f : input) {
            while (data.size() % 32) data.push_back(0);
            offsets.push_back(data.size()); data.insert(data.end(), f.bytes.begin(), f.bytes.end());
        }
        Bytes out(data_offset + data.size());
        tag(out, 0, "RARC"); put32(out, 4, out.size()); put32(out, 8, 0x20);
        put32(out, 12, data_offset - 0x20); put32(out, 16, data.size());
        put32(out, 0x20, 3); put32(out, 0x24, dirs - 0x20);
        put32(out, 0x28, count); put32(out, 0x2c, files - 0x20);
        put32(out, 0x30, strings.size()); put32(out, 0x34, string_offset - 0x20); put16(out, 0x38, 200);
        auto directory = [&](unsigned index, unsigned name, unsigned n, unsigned first) {
            auto p = dirs + index * 16; tag(out, p, "ROOT"); put32(out, p + 4, name);
            put16(out, p + 8, RarcArchive::hash_name(reinterpret_cast<const char*>(strings.data() + name)));
            put16(out, p + 10, n); put32(out, p + 12, first);
        };
        directory(0, root, roots + 4, 0);
        directory(1, nest, input.size() - roots + 2, roots + 4);
        directory(2, empty, 2, input.size() + 6);
        auto entry = [&](unsigned index, unsigned id, unsigned name, unsigned flags, unsigned offset, unsigned size) {
            auto p = files + index * 20; put16(out, p, id);
            put16(out, p + 2, RarcArchive::hash_name(reinterpret_cast<const char*>(strings.data() + name)));
            put32(out, p + 4, (flags << 24) | name); put32(out, p + 8, offset); put32(out, p + 12, size);
        };
        unsigned index = 0;
        for (std::size_t i = 0; i < input.size(); ++i)
            if (!input[i].nested) entry(index++, 100 + i, names[i], 0x11, offsets[i], input[i].bytes.size());
        entry(index++, 0xffff, nest, 2, 1, 16); entry(index++, 0xffff, empty, 2, 2, 16);
        entry(index++, 0xffff, dot, 2, 0, 16); entry(index++, 0xffff, parent, 2, 0xffffffff, 16);
        for (std::size_t i = 0; i < input.size(); ++i)
            if (input[i].nested) entry(index++, 100 + i, names[i], 0x11, offsets[i], input[i].bytes.size());
        entry(index++, 0xffff, dot, 2, 1, 16); entry(index++, 0xffff, parent, 2, 0, 16);
        entry(index++, 0xffff, dot, 2, 2, 16); entry(index++, 0xffff, parent, 2, 0, 16);
        require(index == count, "fixture directory count");
        std::copy(strings.begin(), strings.end(), out.begin() + string_offset);
        std::copy(data.begin(), data.end(), out.begin() + data_offset);
        return std::make_shared<RarcArchive>(RarcArchive::from_bytes(std::move(out)));
    }
    Bytes file(std::string_view type, Bytes block) {
        Bytes b(0x20); tag(b, 0, "J3D1"); tag(b, 4, type); put32(b, 12, 1);
        b.insert(b.end(), block.begin(), block.end()); put32(b, 8, b.size()); return b;
    }
    Bytes transform(bool key) {
        Bytes b(0x24); tag(b, 0, key ? "ANK1" : "ANF1"); b[8] = 3; b[9] = 2;
        put16(b, 0xa, 7); put16(b, 0xc, 1);
        for (int p : {0xe, 0x10, 0x12}) put16(b, p, key ? 1 : 2);
        put32(b, 0x14, b.size()); const auto table = b.size(); b.resize(table + (key ? 54 : 36));
        for (int i = 0; i < 9; ++i) put16(b, table + i * (key ? 6 : 4), key ? 1 : 2);
        const auto f = [&](int field, float a, float c) {
            put32(b, field, b.size()); const auto p = b.size(); b.resize(p + (key ? 4 : 8));
            put32(b, p, std::bit_cast<std::uint32_t>(a)); if (!key) put32(b, p + 4, std::bit_cast<std::uint32_t>(c));
        };
        f(0x18, 2, 3); put32(b, 0x1c, b.size()); auto p = b.size(); b.resize(p + 4);
        put16(b, p, -3); put16(b, p + 2, 30); f(0x20, 4, 9); put32(b, 4, b.size());
        return file(key ? "bck1" : "bca1", std::move(b));
    }
    // Real joint-only model data and an actual J3DModel constructor. This does
    // not represent a partially decoded material/shape archive as a full BMD.
    struct Model {
        J3DModelData data;
        std::array<J3DJoint, 2> joints;
        std::array<J3DJoint*, 2> pointers{&joints[0], &joints[1]};
        std::array<u8, 2> draw_flags{};
        std::array<u16, 2> draw_indices{0, 1};
        J3DMtxCalcNoAnm<J3DMtxCalcCalcTransformBasic, J3DMtxCalcJ3DSysInitBasic> basic;
        std::unique_ptr<J3DModel> object;

        Model() {
            joints[0].mJntNo = 0;
            joints[0].mChild = &joints[1];
            joints[0].mTransformInfo.mTranslate = {7, 8, 9};
            joints[1].mJntNo = 1;
            joints[1].mTransformInfo.mTranslate = {0, 3, 0};
            auto& tree = data.getJointTree();
            tree.mJointNum = 2;
            tree.mJointNodePointer = pointers.data();
            tree.mRootNode = &joints[0];
            tree.mBasicMtxCalc = &basic;
            tree.mDrawMtxData.mEntryNum = 2;
            tree.mDrawMtxData.mDrawFullWgtMtxNum = 2;
            tree.mDrawMtxData.mDrawMtxFlag = draw_flags.data();
            tree.mDrawMtxData.mDrawMtxIndex = draw_indices.data();
            object = std::make_unique<J3DModel>(&data, 0, 1);
        }

        ~Model() {
            // The original destructor does not own arena allocations.
            auto* buffer = object->getMtxBuffer();
            for (auto bank = 0; bank < 2; ++bank) {
                ::operator delete[](buffer->mpDrawMtxArr[bank][0], 0x20);
                ::operator delete[](buffer->mpNrmMtxArr[bank][0], 0x20);
                delete[] buffer->mpDrawMtxArr[bank];
                delete[] buffer->mpNrmMtxArr[bank];
            }
            delete[] buffer->mpScaleFlagArr;
            delete[] buffer->mpAnmMtx;
            delete buffer;
        }
    };


    void test_sync_info(GameResourceRuntime& process) {
        auto key = transform(true);
        // Real binary ANK1 header attributes, consumed by the actual loader.
        auto loop = key; loop[0x28] = 2;
        auto mirror = key; mirror[0x28] = 4;
        auto once = key; once[0x28] = 0;
        auto source = archive({{"Loop.bck", loop}, {"Mirror.bck", mirror}, {"Once.bck", once}});
        std::weak_ptr<const RarcArchive> source_weak = source;
        auto domain = process.create_cohort();
        auto owner = std::make_unique<ResourceArchiveOwner>(source, "SyncFixture.arc", domain, process.mem1_heap());
        source.reset();
        require(!source_weak.expired(), "resource owner retains input archive");
        {
            JkrAllocationScope allocations(domain);
            Model model;
            XanimeResourceTable table(&owner->holder());
            XanimePlayer player(model.object.get(), &table);
            SyncBckEffectInfo info(&player, "Loop", 4, 2.5f, -1.0f, true);
            require(info.mBckResources.size() == 1 && info.mBckResources.capacity() == 4,
                    "constructor allocates authored capacity and registers initial clip");
            require(info.mStartFrame == 2.5f && info.mEndFrame == -1.0f && info.mContinueBckEnd,
                    "constructor retains fractional start/end and continuation");
            auto* resource = static_cast<J3DAnmTransform*>(owner->holder().mMotionResTable->getRes("Loop"));
            require(info.mBckResources[0]->mResource == resource,
                    "sync record references exact typed original resource, not raw ANK1 bytes");
            info.addBck(&player, "Mirror");
            info.addBck(&player, "Once");
            require(info.isRegisteredBck("lOoP") && info.isRegisteredBck("MIRROR") && info.isRegisteredBck("once"),
                    "original registration queries compare names without case");
            require(!info.isRegisteredBck(nullptr) && !info.isRegisteredBck("missing"), "null/missing membership returns false");
            require(info.isBckLoop("LOOP") && info.isBckLoop("mirror") && !info.isBckLoop("once"),
                    "only authored repeat and mirror-repeat modes loop");
            require(!info.isBckLoop(nullptr) && !info.isBckLoop("missing"), "null/missing loop query returns false");
            // Lookup returns the first matching record; registration itself does
            // not collapse duplicate authored names or consume extra capacity.
            info.addBck(&player, "loop");
            auto* duplicate = info.mBckResources[3];
            duplicate->mResource = info.mBckResources[2]->mResource;
            require(info.mBckResources.size() == 4 && info.isBckLoop("Loop"), "first duplicate match preserves original lookup order");
            const auto attribute = resource->mAttribute;
            for (unsigned value = 0; value < 256; ++value) {
                resource->mAttribute = static_cast<u8>(value);
                require(info.mBckResources[0]->isLoop() == (value == 2 || value == 4), "all 256 actual resource attributes");
            }
            resource->mAttribute = attribute;
            constexpr std::array frames{-std::numeric_limits<float>::infinity(), -1.0f, -0.001f, -0.0f, 0.0f,
                                        0.001f, 7.5f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()};
            for (const float frame : frames) {
                info.mEndFrame = frame;
                const bool expected = !std::isnan(frame) && !std::signbit(frame) || frame == 0.0f;
                require(MR::Effect::isExistSyncBckDeleteFrame(&info) == expected, "delete-frame predicate handles signed zero/infinity/unordered");
            }
            // Original pointer-container destruction frees its array; its
            // original heap owns the separately allocated BckResourceInfo nodes.
        }
        owner.reset();
        require(source_weak.expired(), "last resource owner releases source archive after all consumers end");
    }
}
int main() {
    try {
        aurora::g_config.mem1Size = 24U * 1024U * 1024U;
        GameResourceRuntime process;
        test_sync_info(process);
        std::cout << "PASS actual ResourceHolder/J3DModel/XanimePlayer sync records, 256 attributes, 9 frame boundaries, resource lifetime\n";
    } catch (const std::exception& e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
}
