#include "JSystem/J3DGraphBase/J3DStruct.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "resource/J3dAnimationResource.hpp"
#include "resource/J3dTransformAnimation.hpp"
#include "resource/RarcArchive.hpp"

#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <vector>

namespace {
    using smgpc::resource::J3dAnimationResource;
    using Bytes = std::vector<std::uint8_t>;
    void require(bool value, const char *message) {
        if (!value)
            throw std::runtime_error(message);
    }
    void near(float actual, float expected, const char *message) {
        require(std::abs(actual - expected) < 1e-6f, message);
    }
    template <class F>
    void rejects(F call, const char *message) {
        bool caught = false;
        try {
            call();
        } catch (const std::exception &) {
            caught = true;
        }
        require(caught, message);
    }
    void u16(Bytes &b, std::size_t at, std::uint16_t v) {
        b[at] = v >> 8;
        b[at + 1] = v;
    }
    void u32(Bytes &b, std::size_t at, std::uint32_t v) {
        b[at] = v >> 24;
        b[at + 1] = v >> 16;
        b[at + 2] = v >> 8;
        b[at + 3] = v;
    }
    void tag(Bytes &b, std::size_t at, std::string_view v) {
        std::copy(v.begin(), v.end(), b.begin() + at);
    }
    struct Block {
        Bytes bytes;
        Block(std::string_view type, std::size_t header) : bytes(header) {
            tag(bytes, 0, type);
            bytes[8] = 3;
            u16(bytes, 0xa, 7);
        }
        std::size_t append(Bytes data, std::size_t alignment = 4) {
            while (bytes.size() % alignment)
                bytes.push_back(0);
            auto offset = bytes.size();
            bytes.insert(bytes.end(), data.begin(), data.end());
            return offset;
        }
        void at(std::size_t field, Bytes data, std::size_t alignment = 4) {
            u32(bytes, field, static_cast<std::uint32_t>(append(std::move(data), alignment)));
        }
        Bytes finish() {
            u32(bytes, 4, static_cast<std::uint32_t>(bytes.size()));
            return std::move(bytes);
        }
    };
    Bytes shorts(std::initializer_list<int> values) {
        Bytes b(values.size() * 2);
        std::size_t i = 0;
        for (auto v : values)
            u16(b, i++ * 2, v);
        return b;
    }
    Bytes floats(std::initializer_list<float> values) {
        Bytes b(values.size() * 4);
        std::size_t i = 0;
        for (auto v : values)
            u32(b, i++ * 4, std::bit_cast<std::uint32_t>(v));
        return b;
    }
    Bytes name() {
        Bytes b(12);
        u16(b, 0, 1);
        u16(b, 2, 0xffff);
        u16(b, 4, 0x5a8);
        u16(b, 6, 8);
        tag(b, 8, "mat");
        return b;
    }
    Bytes file(std::string_view type, std::vector<Bytes> blocks) {
        Bytes b(0x20);
        tag(b, 0, "J3D1");
        tag(b, 4, type);
        u32(b, 12, blocks.size());
        for (auto &part : blocks)
            b.insert(b.end(), part.begin(), part.end());
        u32(b, 8, b.size());
        return b;
    }
    Bytes transform(bool key) {
        Block b(key ? "ANK1" : "ANF1", 0x24);
        b.bytes[9] = 2;
        u16(b.bytes, 0xc, 1);
        Bytes tables(key ? 54 : 36);
        for (std::size_t i = 0; i < 3; ++i)
            for (std::size_t c = 0; c < 3; ++c)
                u16(tables, i * (key ? 18 : 12) + c * (key ? 6 : 4), key ? 1 : 2);
        b.at(0x14, std::move(tables));
        if (key) {
            u16(b.bytes, 0xe, 1);
            u16(b.bytes, 0x10, 1);
            u16(b.bytes, 0x12, 1);
            b.at(0x18, floats({2}));
            b.at(0x1c, shorts({-3}));
            b.at(0x20, floats({4}));
        } else {
            b.at(0x18, floats({1, 3}));
            b.at(0x1c, shorts({10, 30}));
            b.at(0x20, floats({5, 9}));
        }
        return b.finish();
    }
    Bytes color(bool key, bool vertex = false) {
        Block b(vertex ? (key ? "VCK1" : "VCF1") : (key ? "PAK1" : "PAF1"), vertex ? 0x40 : 0x34);
        if (vertex) {
            u16(b.bytes, 0xc, 1);
            u16(b.bytes, 0xe, 1);
        } else {
            u16(b.bytes, 0xc, 7);
            u16(b.bytes, 0xe, 1);
        }
        Bytes table(key ? 24 : 16);
        for (std::size_t c = 0; c < 4; ++c)
            u16(table, c * (key ? 6 : 4), key ? 1 : 2);
        b.at(0x18, table);
        if (vertex) {
            b.at(0x1c, table);
            for (std::size_t i = 0; i < 2; ++i) {
                Bytes indices(8);
                u16(indices, 0, 2);
                u32(indices, 4, 1);
                b.at(0x20 + i * 4, std::move(indices));
                b.at(0x28 + i * 4, shorts({99, 7, 9}));
            }
        } else {
            b.at(0x1c, shorts({0xffff}));
            b.at(0x20, name());
        }
        for (std::size_t c = 0; c < 4; ++c) {
            u16(b.bytes, 0x10 + c * 2, key ? 1 : 0x1234);
            b.at((vertex ? 0x30 : 0x24) + c * 4, key ? shorts({int(10 + c * 20)}) : Bytes{std::uint8_t(10 + c * 20), std::uint8_t(11 + c * 20)});
        }
        return b.finish();
    }
    Bytes texture_srt() {
        Block b("TTK1", 0x60);
        b.bytes[9] = 1;
        for (int post = 0; post < 2; ++post) {
            auto c = post ? 0x34 : 0xc;
            u16(b.bytes, c, 3);
            u16(b.bytes, c + 2, 2);
            u16(b.bytes, c + 4, 1);
            u16(b.bytes, c + 6, 2);
            Bytes t(54);
            u16(t, 0, 1);
            u16(t, 12, 1);
            u16(t, 18, 1);
            u16(t, 20, 1);
            u16(t, 30, 1);
            u16(t, 32, 1);
            u16(t, 42, 1);
            // Unused Z translation is not read by the actual BTK sampler.
            u16(t, 48, 0xffff);
            u16(t, 50, 0xffff);
            b.at(post ? 0x3c : 0x14, t);
            b.at(post ? 0x40 : 0x18, shorts({0xffff}));
            b.at(post ? 0x44 : 0x1c, name());
            b.at(post ? 0x48 : 0x20, Bytes{std::uint8_t(post ? 9 : 4)});
            b.at(post ? 0x4c : 0x24, floats({.25f, .5f, .75f}));
            b.at(post ? 0x50 : 0x28, post ? floats({4, 5}) : floats({2, 3}));
            b.at(post ? 0x54 : 0x2c, shorts({post ? 7 : 16}));
            b.at(post ? 0x58 : 0x30, post ? floats({19, 23}) : floats({11, 13}));
        }
        u32(b.bytes, 0x5c, 1);
        return b.finish();
    }
    Bytes tev() {
        Block b("TRK1", 0x58);
        u16(b.bytes, 0xc, 1);
        u16(b.bytes, 0xe, 1);
        for (std::size_t i = 0; i < 8; ++i)
            u16(b.bytes, 0x10 + i * 2, 1);
        for (int group = 0; group < 2; ++group) {
            Bytes t(28);
            for (int c = 0; c < 4; ++c)
                u16(t, c * 6, 1);
            t[24] = group ? 2 : 3;
            t[25] = 0xa5;
            b.at(group ? 0x24 : 0x20, t);
            b.at(group ? 0x2c : 0x28, shorts({0xffff}));
            b.at(group ? 0x34 : 0x30, name());
        }
        std::array<int, 8> values{-200, 300, -12, 1023, 5, 6, 7, 8};
        for (std::size_t i = 0; i < 8; ++i)
            b.at(0x38 + i * 4, shorts({values[i]}));
        return b.finish();
    }
    Bytes pattern() {
        Block b("TPT1", 0x20);
        u16(b.bytes, 0xc, 1);
        u16(b.bytes, 0xe, 2);
        Bytes t(8);
        u16(t, 0, 2);
        t[4] = 4;
        u16(t, 6, 0xabcd);
        b.at(0x10, t);
        b.at(0x14, shorts({17, 29}));
        b.at(0x18, shorts({0xffff}));
        b.at(0x1c, name());
        return b.finish();
    }
    Bytes visibility() {
        Block b("VAF1", 0x18);
        u16(b.bytes, 0xc, 1);
        u16(b.bytes, 0xe, 2);
        b.at(0x10, shorts({2, 0}));
        b.at(0x14, Bytes{3, 9});
        return b.finish();
    }
    Bytes cluster(bool key) {
        Block b(key ? "CLK1" : "CLF1", 0x18);
        u16(b.bytes, 0xc, 1);
        u16(b.bytes, 0xe, key ? 1 : 2);
        b.at(0x10, key ? shorts({1, 0, 0}) : shorts({2, 0}));
        b.at(0x14, key ? floats({1.25f}) : floats({.25f, .75f}));
        return b.finish();
    }
    void transform_test() {
        for (bool key : {true, false}) {
            auto raw = file(key ? "bck1" : "bca1", {transform(key)});
            J3dAnimationResource resource(raw);
            auto *original = dynamic_cast<J3DAnmTransform *>(resource.load());
            require(original, "actual transform class");
            auto old = smgpc::resource::load_j3d_transform_animation(raw);
            original->mFrame = .5f;
            old->mFrame = .5f;
            J3DTransformInfo actual{}, expected{};
            original->getTransform(0, &actual);
            old->getTransform(0, &expected);
            require(actual.mScale.x == expected.mScale.x && actual.mScale.y == expected.mScale.y && actual.mScale.z == expected.mScale.z &&
                        actual.mRotation.x == expected.mRotation.x && actual.mRotation.y == expected.mRotation.y && actual.mRotation.z == expected.mRotation.z &&
                        actual.mTranslate.x == expected.mTranslate.x && actual.mTranslate.y == expected.mTranslate.y && actual.mTranslate.z == expected.mTranslate.z,
                    "new dispatch and retained transform decoder agree at every output field");
            near(actual.mScale.x, key ? 2 : 3, "independent transform scale");
            require(actual.mRotation.x == (key ? -12 : 30), "stored shift or full rotation");
            near(actual.mTranslate.z, key ? 4 : 9, "stored transform translation");
            require(original->mAttribute == 3 && original->mFrameMax == 7, "authored header fields are preserved");
            if (!key) {
                auto *lerp = dynamic_cast<J3DAnmTransformFullWithLerp *>(resource.load(J3DLOADER_UNK_FLAG1));
                require(lerp, "original flag selects actual interpolating Full class");
                lerp->mFrame = .5f;
                lerp->getTransform(0, &actual);
                near(actual.mScale.x, 2, "BCA lerp scale midpoint");
                require(actual.mRotation.x == 20, "BCA lerp rotation midpoint");
                near(actual.mTranslate.x, 7, "BCA lerp position midpoint");
            }
        }
    }
    void color_test() {
        for (bool key : {true, false}) {
            J3dAnimationResource resource(file(key ? "bpk1" : "bpa1", {color(key)}));
            auto *a = dynamic_cast<J3DAnmColor *>(resource.load());
            require(a, "actual material color class");
            a->mFrame = .6f;
            GXColor out{};
            a->getColor(0, &out);
            require(out.r == (key ? 10 : 11) && out.g == (key ? 30 : 31) && out.b == (key ? 50 : 51) && out.a == (key ? 70 : 71), "key constants versus full rounded sample");
            require(a->mUpdateMaterialNum == 1 && a->mUpdateMaterialID[0] == 0xffff && std::string_view(a->mUpdateMaterialName.getName(0)) == "mat", "material ID and name ownership");
            a->mUpdateMaterialID[0] = 23;
            require(a->mUpdateMaterialID[0] == 23, "ID table remains genuinely writable");
        }
    }
    void key_interpolation_test() {
        Block b("PAK1", 0x34);
        u16(b.bytes, 0xc, 4);
        u16(b.bytes, 0xe, 1);
        Bytes tables(24);
        for (std::size_t c = 0; c < 4; ++c) {
            u16(tables, c * 6, 2);
            u16(tables, c * 6 + 4, 7);
            u16(b.bytes, 0x10 + c * 2, 8);
        }
        b.at(0x18, tables);
        b.at(0x1c, shorts({0xffff}));
        b.at(0x20, name());
        for (int c = 0; c < 4; ++c)
            b.at(0x24 + c * 4, shorts({0, 10 + c * 20, 0, 0, 2, 30 + c * 20, 0, 0}));
        auto raw = file("bpk1", {b.finish()});
        J3dAnimationResource owner(raw);
        auto *a = static_cast<J3DAnmColorKey *>(owner.load());
        a->mFrame = 1;
        GXColor out{};
        a->getColor(0, &out);
        require(out.r == 20 && out.g == 40 && out.b == 60 && out.a == 80, "nonzero tangent type retains exact original Hermite midpoint");
        const auto offset = (std::uint32_t(raw[0x44]) << 24) | (std::uint32_t(raw[0x45]) << 16) | (std::uint32_t(raw[0x46]) << 8) | raw[0x47];
        u16(raw, 0x20 + offset + 8, 0xffff);
        J3dAnimationResource backwards(raw);
        rejects([&] { (void)backwards.load(); }, "descending key times rejected before original search");
    }
    void texture_test() {
        J3dAnimationResource resource(file("btk1", {texture_srt()}));
        auto *a = dynamic_cast<J3DAnmTextureSRTKey *>(resource.load());
        require(a, "actual BTK class");
        J3DTextureSRTInfo out{};
        a->getTransform(0, &out);
        near(out.mScaleX, 2, "BTK scale X");
        near(out.mScaleY, 3, "BTK scale Y");
        require(out.mRotation == 32, "BTK original stored rotation shift");
        near(out.mTranslationX, 11, "BTK translation X");
        near(out.mTranslationY, 13, "BTK translation Y");
        require(a->mTrackNum == 3 && a->field_0x4a == 3 && a->mPostUpdateTexMtxID[0] == 9 && a->mUpdateTexMtxID[0] == 4, "pre/post metadata retained");
        near(static_cast<float *>(a->field_0x4c)[1], 5, "post scale owner");
        near(static_cast<float *>(a->field_0x54)[0], 19, "post translation owner");
        require(std::string_view(a->mPostUpdateMaterialName.getName(0)) == "mat" && a->getTexMtxCalcType() == 1, "post names and original matrix mode");
        auto raw = file("btk1", {texture_srt()});
        u32(raw, 0x20 + 0x5c, 9);
        J3dAnimationResource invalid_mode(raw);
        auto *mode = static_cast<J3DAnmTextureSRTKey *>(invalid_mode.load());
        require(mode->getTexMtxCalcType() == 0, "original loader normalizes unsupported matrix mode");
    }
    void tev_test() {
        J3dAnimationResource resource(file("brk1", {tev()}));
        auto *a = dynamic_cast<J3DAnmTevRegKey *>(resource.load());
        require(a, "actual BRK class");
        GXColorS10 c{};
        GXColor k{};
        a->getTevColorReg(0, &c);
        a->getTevKonstReg(0, &k);
        require(c.r == -200 && c.g == 300 && c.b == -12 && c.a == 1023, "signed constant register values are not clamped by loader");
        require(k.r == 5 && k.g == 6 && k.b == 7 && k.a == 8, "independent konst register values");
        require(a->mAnmCRegKeyTable[0].mColorId == 3 && a->mAnmKRegKeyTable[0].padding[0] == 0xa5, "register destination and padding metadata retained");
    }
    void pattern_visibility_test() {
        J3dAnimationResource tex(file("btp1", {pattern()})), vis(file("bva1", {visibility()}));
        auto *t = static_cast<J3DAnmTexPattern *>(tex.load());
        auto *v = static_cast<J3DAnmVisibilityFull *>(vis.load());
        std::uint16_t index = 0;
        std::uint8_t visibility = 0;
        t->mFrame = .75f;
        t->getTexNo(0, &index);
        require(index == 17, "BTP truncates fractional frames");
        t->mFrame = 20;
        t->getTexNo(0, &index);
        require(index == 29, "BTP terminal sample");
        v->mFrame = .49f;
        v->getVisibility(0, &visibility);
        require(visibility == 3, "BVA before rounding boundary");
        v->mFrame = .5f;
        v->getVisibility(0, &visibility);
        require(visibility == 9, "BVA rounds at half frame");
        v->mFrame = -2;
        v->getVisibility(0, &visibility);
        require(visibility == 3, "BVA negative integer sample uses first");
        require(t->mAnmTable[0]._6 == 0xabcd && t->mAnmTable[0].mTexNo == 4, "BTP descriptor fields preserved");
    }
    void cluster_test() {
        for (bool key : {true, false}) {
            J3dAnimationResource resource(file(key ? "blk1" : "bla1", {cluster(key)}));
            auto *a = dynamic_cast<J3DAnmCluster *>(resource.load());
            require(a, "actual cluster class");
            a->mFrame = .5f;
            near(a->getWeight(0), key ? 1.25f : .75f, "cluster key constant versus full rounded value");
        }
    }
    void vertex_test() {
        for (bool key : {true, false}) {
            J3dAnimationResource resource(file(key ? "bxk1" : "bxa1", {color(key, true)}));
            auto *a = dynamic_cast<J3DAnmVtxColor *>(resource.load());
            require(a, "actual vertex-color class");
            a->mFrame = .6f;
            GXColor c{};
            a->getColor(1, 0, &c);
            require(c.r == (key ? 10 : 11) && c.a == (key ? 70 : 71), "second vertex-color channel samples actual native arrays");
            for (int channel = 0; channel < 2; ++channel) {
                const auto &i = a->mAnmVtxColorIndexData[channel][0];
                const auto *p = static_cast<const std::uint16_t *>(i.mpData);
                require(i.mNum == 2 && p[0] == 7 && p[1] == 9, "original index-unit pointer relocation uses native-width retained addresses");
            }
        }
    }
    void order_test() {
        auto first = visibility(), last = visibility();
        const auto offset = (std::uint32_t(last[0x14]) << 24) | (std::uint32_t(last[0x15]) << 16) | (std::uint32_t(last[0x16]) << 8) | last[0x17];
        last[offset] = 41;
        Bytes unknown(8);
        tag(unknown, 0, "TEST");
        u32(unknown, 4, 8);
        J3dAnimationResource resource(file("bva1", {first, unknown, last}));
        auto *a = static_cast<J3DAnmVisibilityFull *>(resource.load());
        std::uint8_t out = 0;
        a->getVisibility(0, &out);
        require(out == 41, "original block traversal skips unknown blocks and last matching block wins");
    }
    void ownership_test() {
        auto raw = file("btp1", {pattern()});
        const void *stale = nullptr;
        std::optional<J3dAnimationResource> retained;
        J3DAnmTexPattern *a = nullptr;
        {
            J3dAnimationResource original(raw);
            retained = original;
            stale = original.data();
            a = static_cast<J3DAnmTexPattern *>(J3DAnmLoaderDataBase::load(original.data()));
            std::fill(raw.begin(), raw.end(), 0);
        }
        std::uint16_t tex = 0;
        a->getTexNo(0, &tex);
        require(tex == 17, "actual animation outlives input bytes and first handle");
        auto *b = static_cast<J3DAnmTexPattern *>(retained->load());
        require(a != b && a->mAnmTable != b->mAnmTable, "separate loads retain independently mutable native data");
        retained.reset();
        rejects([&] { J3DAnmLoaderDataBase::load(stale); }, "expired owned source identity is unregistered");
    }
    void registration_test() {
        auto raw = file("btp1", {pattern()});
        J3dAnimationResource owner(raw);
        rejects([&] { J3DAnmLoaderDataBase::load(raw.data()); }, "borrowed input identity is never implicitly published");
        {
            auto first = owner.register_source(raw);
            {
                auto second = owner.register_source(raw);
                require(J3DAnmLoaderDataBase::load(raw.data()) != nullptr, "nested explicit alias");
            }
            require(J3DAnmLoaderDataBase::load(raw.data()) != nullptr, "one alias release preserves another registration");
        }
        rejects([&] { J3DAnmLoaderDataBase::load(raw.data()); }, "last alias release removes exact identity");
        auto wrong = raw;
        wrong.back() ^= 1;
        rejects([&] { (void)owner.register_source(wrong); }, "alias byte identity validated");
        rejects([&] { (void)owner.register_source(std::span(raw).first(raw.size() - 1)); }, "alias complete extent validated");
        std::optional<smgpc::resource::J3dAnimationSourceRegistration> surviving;
        {
            J3dAnimationResource temporary(raw);
            surviving.emplace(temporary.register_source(raw));
        }
        require(J3DAnmLoaderDataBase::load(raw.data()) != nullptr, "alias itself retains actual resource lifetime");
        surviving.reset();
    }
    void malformed_test() {
        require(J3DAnmLoaderDataBase::load(nullptr) == nullptr, "original null input result");
        J3dAnimationResource unknown(Bytes{'J', '3', 'D', '1', 'N', 'O', 'P', 'E'});
        require(unknown.load() == nullptr, "unknown original type returns null");
        auto raw = file("bva1", {visibility()});
        u32(raw, 0x20 + 0x14, 0xfffffffc);
        J3dAnimationResource bad(raw);
        rejects([&] { (void)bad.load(); }, "values outside block rejected before original sampler");
        auto empty = file("bva1", {visibility()});
        u32(empty, 0x20 + 0x10, 0);
        J3dAnimationResource missing(empty);
        rejects([&] { (void)missing.load(); }, "nonempty null table rejected");
        auto mismatch = file("bva1", {cluster(true)});
        J3dAnimationResource wrong(mismatch);
        rejects([&] { (void)wrong.load(); }, "mismatched original block would cast wrong class and is rejected");
    }
    void concurrent_test() {
        J3dAnimationResource resource(file("bva1", {visibility()}));
        std::array<J3DAnmBase *, 4> results{};
        std::array<std::thread, 4> workers;
        for (std::size_t i = 0; i < 4; ++i)
            workers[i] = std::thread([&, i] { results[i] = resource.load(); });
        for (auto &t : workers)
            t.join();
        for (std::size_t i = 0; i < 4; ++i) {
            require(results[i] && results[i]->getKind() == 6, "thread-local original traversal and retained resource load");
            for (std::size_t j = 0; j < i; ++j)
                require(results[i] != results[j], "concurrent loads have distinct actual objects");
        }
    }
    void heap_domain_test() {
        using namespace smgpc::compat;
        auto runtime = JkrHeapRuntime::create(2 * 1024 * 1024);
        auto domain = JkrAllocationDomain::create(runtime, 256 * 1024);
        std::weak_ptr<JkrAllocationDomain> weak_domain = domain;
        std::weak_ptr<JkrHeapRuntime> weak_runtime = runtime;
        std::optional<J3dAnimationResource> resource;
        const auto raw = file("bck1", {transform(true)});
        J3DAnmTransform *animation;
        {
            JkrAllocationScope original(domain);
            resource.emplace(raw);
            animation = static_cast<J3DAnmTransform *>(resource->load());
            require(JKRHeap::findFromRoot(animation) == &domain->heap(),
                    "actual loader constructs its actual animation in the selected original heap");
            require(JKRHeap::findFromRoot(const_cast<void *>(resource->data())) == nullptr,
                    "retained raw parser bytes remain independent of Game heap allocation");
            require(JKRHeap::findFromRoot(static_cast<J3DAnmTransformKey *>(animation)->mAnmTable) == nullptr,
                    "native decoded tables remain host owned during original constructor allocation");
        }
        domain.reset();
        runtime.reset();
        require(!weak_domain.expired() && !weak_runtime.expired(),
                "loaded animation retains its original heap after caller scope and owners leave");
        J3DTransformInfo sampled{};
        animation->getTransform(0, &sampled);
        near(sampled.mScale.x, 2, "animation remains readable after external heap ownership release");
        require(sampled.mRotation.x == -12, "retained animation still uses original signed rotation sampling");
        resource.reset();
        require(weak_domain.expired() && weak_runtime.expired() && JKRHeap::sRootHeap == nullptr,
                "last resource release destroys actual animation before disposing its retained original heap");
    }
    void archive_test(const char *path) {
        auto archive = smgpc::resource::RarcArchive::from_file(path);
        std::map<std::string, std::size_t> families;
        for (const auto &entry : archive.entries()) {
            auto bytes = archive.file_data(entry);
            if (bytes.size() < 8 || std::memcmp(bytes.data(), "J3D1", 4) != 0)
                continue;
            J3dAnimationResource resource(bytes);
            try {
                auto *animation = resource.load();
                if (!animation)
                    continue;
                auto registration = resource.register_source(bytes);
                require(J3DAnmLoaderDataBase::load(bytes.data()) != nullptr, "real archive identity reaches original dispatch");
                if (auto *transform = dynamic_cast<J3DAnmTransform *>(animation); transform && transform->field_0x1e != 0) {
                    for (float frame : {0.0f, animation->mFrameMax * .5f, float(animation->mFrameMax)}) {
                        J3DTransformInfo value{};
                        animation->setFrame(frame);
                        transform->getTransform(0, &value);
                    }
                }
                ++families[std::string(reinterpret_cast<const char *>(bytes.data() + 4), 4)];
            } catch (const std::exception &error) {
                throw std::runtime_error(entry.path + ": " + error.what());
            }
        }
        for (const auto &[family, count] : families)
            std::cout << "ARCHIVE " << family << ' ' << count << '\n';
    }
}  // namespace
int main(int argc, char **argv) {
    const std::array tests{std::pair{"BCK and BCA original dispatch", transform_test}, std::pair{"BPK and BPA material color", color_test}, std::pair{"material keyed interpolation", key_interpolation_test}, std::pair{"BTK pre and post texture data", texture_test}, std::pair{"BRK signed and konst registers", tev_test}, std::pair{"BTP and BVA frame rules", pattern_visibility_test}, std::pair{"BLK and BLA cluster weights", cluster_test}, std::pair{"BXK and BXA index relocation", vertex_test}, std::pair{"original block order", order_test}, std::pair{"retained ownership and repeat load", ownership_test}, std::pair{"explicit archive alias lifetime", registration_test}, std::pair{"bounded malformed resources", malformed_test}, std::pair{"concurrent native load scopes", concurrent_test}, std::pair{"actual JKR allocation domain", heap_domain_test}};
    for (const auto &[label, test] : tests) {
        try {
            test();
            std::cout << "PASS " << label << '\n';
        } catch (const std::exception &e) {
            std::cerr << "FAIL " << label << ": " << e.what() << '\n';
            return 1;
        }
    }
    if (argc > 1) {
        try {
            archive_test(argv[1]);
        } catch (const std::exception &error) {
            std::cerr << "FAIL real archive: " << error.what() << '\n';
            return 1;
        }
    }
}
