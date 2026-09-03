#include "resource/J3dTextureData.hpp"
#include "resource/Mem1ResourceHeap.hpp"
#include "resource/TplTexture.hpp"
#include "runtime/RuntimeServices.hpp"
#include "JSystem/J3DGraphAnimator/J3DMaterialAttach.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include "JSystem/J3DGraphBase/J3DTexture.hpp"
#include "JSystem/J3DGraphBase/J3DTevs.hpp"
#include "JSystem/JUtility/JUTNameTab.hpp"

#include <aurora/aurora.h>
#include <aurora/dvd.h>
#include <dolphin/dvd.h>
#include <dolphin/gd.h>
#include <dolphin/os.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace aurora { extern AuroraConfig g_config; }

namespace {
    using Bytes = std::vector<std::uint8_t>;
    using View = std::span<const std::uint8_t>;
    using smgpc::resource::J3dTextureData;
    using smgpc::resource::Mem1ResourceHeap;
    void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
    template <typename F> void rejects(F f) {
        bool rejected = false;
        try { f(); } catch (const std::exception&) { rejected = true; }
        require(rejected, "invalid texture resource/setup must be rejected");
    }
    void put16(Bytes& b, std::size_t o, std::uint16_t v) { b.at(o) = v >> 8; b.at(o + 1) = v; }
    void put32(Bytes& b, std::size_t o, std::uint32_t v) { put16(b, o, v >> 16); put16(b, o + 2, v); }
    std::uint16_t be16(View b, std::size_t o) { return (std::uint16_t{b[o]} << 8) | b[o + 1]; }
    std::uint32_t be32(View b, std::size_t o) { return (std::uint32_t{be16(b, o)} << 16) | be16(b, o + 2); }
    std::uint16_t hash(const char* name) {
        std::uint16_t result = 0;
        for (; *name; ++name) result = result * 3 + *name;
        return result;
    }
    Bytes fixture(u8 format = GX_TF_C4, bool aliases = false) {
        const auto image_size = smgpc::resource::gx_texture_data_size(
            8, 8, static_cast<smgpc::resource::TplTextureFormat>(format));
        const bool indexed = format == GX_TF_C4 || format == GX_TF_C8 || format == GX_TF_C14X2;
        const u16 palette_count = format == GX_TF_C14X2 ? 16384 : format == GX_TF_C8 ? 256 : indexed ? 16 : 0;
        const std::size_t palette = 0x80 + image_size;
        const std::size_t names = palette + palette_count * 2;
        Bytes bytes(names + 0x40);
        put32(bytes, 0, 0x54455831); put32(bytes, 4, bytes.size()); put16(bytes, 8, aliases ? 2 : 1);
        put32(bytes, 0xC, 0x20); put32(bytes, 0x10, names);
        for (std::size_t i = 0; i < (aliases ? 2U : 1U); ++i) {
            const auto h = 0x20 + i * 0x20;
            bytes[h] = format; bytes[h + 1] = 0x17;
            put16(bytes, h + 2, 8); put16(bytes, h + 4, 8);
            bytes[h + 6] = GX_REPEAT; bytes[h + 7] = GX_MIRROR;
            bytes[h + 8] = indexed; bytes[h + 9] = GX_TL_RGB565;
            put16(bytes, h + 0xA, palette_count);
            put32(bytes, h + 0xC, indexed ? (aliases && i == 1 ? 0x80 : palette) - h : 0);
            bytes[h + 0x10] = 1; bytes[h + 0x11] = 1; bytes[h + 0x12] = 1;
            bytes[h + 0x13] = GX_ANISO_2; bytes[h + 0x14] = GX_LINEAR; bytes[h + 0x15] = GX_NEAR;
            bytes[h + 0x18] = 1; bytes[h + 0x19] = 0xEF; put16(bytes, h + 0x1A, 0xFF85);
            put32(bytes, h + 0x1C, 0x80 - h);
        }
        std::fill(bytes.begin() + 0x80, bytes.begin() + palette, 0x31);
        put16(bytes, names, aliases ? 2 : 1); put16(bytes, names + 2, 0xA5C3);
        put16(bytes, names + 4, hash("texture")); put16(bytes, names + 6, 16);
        if (aliases) { put16(bytes, names + 8, hash("texture")); put16(bytes, names + 10, 16); }
        std::memcpy(bytes.data() + names + 16, "texture", 8);
        return bytes;
    }

    void check_headers(const J3dTextureData& owner, View source) {
        const auto count = be16(source, 8);
        const auto headers = be32(source, 0xC);
        require(owner.texture().getNum() == count, "original texture count is preserved");
        require(std::equal(source.begin(), source.end(), owner.source_bytes().begin()), "retained source bytes are unchanged");
        for (u16 i = 0; i < count; ++i) {
            const auto h = headers + i * 0x20;
            const auto* r = owner.texture().getResTIMG(i);
            require(r->mFormat == source[h] && r->mTransparency == source[h + 1] &&
                        r->mWidth == be16(source, h + 2) && r->mHeight == be16(source, h + 4) &&
                        r->mWrapS == source[h + 6] && r->mWrapT == source[h + 7] &&
                        r->mPaletteName == source[h + 8] && r->mPaletteFormat == source[h + 9] &&
                        r->mPaletteNum == be16(source, h + 0xA) &&
                        r->mMipmap == (source[h + 0x10] != 0) && r->mDoEdgeLod == (source[h + 0x11] != 0) &&
                        r->mBiasClamp == (source[h + 0x12] != 0) && r->mMaxAnisotropy == source[h + 0x13] &&
                        r->mMinType == source[h + 0x14] && r->mMagType == source[h + 0x15] &&
                        r->mMinLod == source[h + 0x16] && r->mMaxLod == source[h + 0x17] &&
                        r->mImageNum == source[h + 0x18] && r->_19 == source[h + 0x19] &&
                        static_cast<u16>(r->mLodBias) == be16(source, h + 0x1A),
                    "every authored ResTIMG scalar and opaque byte is preserved");
            const auto* image = reinterpret_cast<const u8*>(r) + r->mImageDataOffset;
            const auto* palette = reinterpret_cast<const u8*>(r) + r->mPaletteDataOffset;
            require(image == owner.source_bytes().data() + h + be32(source, h + 0x1C), "image offset preserves source correspondence");
            require(palette == owner.source_bytes().data() + h + be32(source, h + 0xC), "palette offset preserves source correspondence");
            const auto physical = OSCachedToPhysical(const_cast<u8*>(image));
            require((physical & 31) == 0 && OSPhysicalToCached((physical >> 5) << 5) == image,
                    "actual MEM1 texture pointer survives the original physical-address encoding");
        }
    }

    void test_formats(const std::shared_ptr<Mem1ResourceHeap>& heap) {
        for (u8 format : std::array<u8, 11>{GX_TF_I4, GX_TF_I8, GX_TF_IA4, GX_TF_IA8, GX_TF_RGB565,
                 GX_TF_RGB5A3, GX_TF_RGBA8, GX_TF_C4, GX_TF_C8, GX_TF_C14X2, GX_TF_CMPR}) {
            const auto bytes = fixture(format);
            J3dTextureData owner(bytes, heap);
            check_headers(owner, bytes);
            require(owner.names() && std::strcmp(owner.names()->getName(0), "texture") == 0, "original name owner is attached");
        }
        auto bytes = fixture(GX_TF_I4);
        bytes[0x20] |= 0xA0;
        J3dTextureData owner(bytes, heap);
        check_headers(owner, bytes); // Original loadTexNo masks the format nibble.
    }

    void test_aliases_and_commands(const std::shared_ptr<Mem1ResourceHeap>& heap) {
        auto source = fixture(GX_TF_C4, true);
        J3dTextureData owner(source, heap);
        check_headers(owner, source);
        source.clear(); source.shrink_to_fit();
        const auto* first = owner.texture().getResTIMG(0);
        const auto* second = owner.texture().getResTIMG(1);
        const auto* data = reinterpret_cast<const u8*>(first) + first->mImageDataOffset;
        require(data == reinterpret_cast<const u8*>(second) + second->mImageDataOffset &&
                    data == reinterpret_cast<const u8*>(second) + second->mPaletteDataOffset,
                "shared image and cross-type palette aliases survive source retirement");
        require(owner.names()->getName(0) == owner.names()->getName(1), "duplicate name offsets remain aliased");
        J3DMaterialTable table;
        owner.attach_to(table);
        require(table.getTexture() == &owner.texture() && table.getTextureName() == owner.names(), "actual material table receives actual texture/name owners");
        rejects([&] { owner.attach_to(table); });

        alignas(32) std::array<u8, 128> commands{};
        GDLObj dl;
        GDInitGDLObj(&dl, commands.data(), commands.size());
        auto* previous_dl = __GDCurrentDL;
        auto* previous_texture = j3dSys.getTexture();
        GDSetCurrent(&dl); j3dSys.setTexture(&owner.texture());
        const u16 index = 0;
        loadTexNo(3, index); // Actual original J3D GD writer path.
        const auto count = GDGetCurrOffset();
        GDSetCurrent(previous_dl); j3dSys.setTexture(previous_texture);
        require(count == 55, "original paletted texture records exactly 20+35 command bytes");
        require(commands[0] == GX_LOAD_BP_REG && commands[1] == 0x97 &&
                    (be32(commands, 1) & 0xFFFFFFU) == (OSCachedToPhysical(const_cast<u8*>(data)) >> 5),
                "original writer emits the real mapped payload address without extra metadata");
    }

    void test_rejection_and_reuse(const std::shared_ptr<Mem1ResourceHeap>& heap) {
        const auto available = heap->available_bytes();
        rejects([&] { (void)Mem1ResourceHeap::create(1024); });
        for (const auto field : {4U, 0xCU, 0x10U, 0x3CU}) {
            auto bytes = fixture(); put32(bytes, field, 0xFFFFFFFF);
            rejects([&] { J3dTextureData owner(bytes, heap); });
        }
        auto bytes = fixture(); bytes[0x30] = 2;
        rejects([&] { J3dTextureData owner(bytes, heap); });
        bytes = fixture(); put32(bytes, 0x3C, 0x61);
        rejects([&] { J3dTextureData owner(bytes, heap); });
        bytes = fixture(); bytes[0x38] = 4; put16(bytes, 0x22, 16); put16(bytes, 0x24, 16);
        rejects([&] { J3dTextureData owner(bytes, heap); });
        rejects([&] { auto allocation = heap->allocate(heap->reserved_bytes()); });
        require(heap->available_bytes() == available, "rejected parsing and allocation retain no heap blocks");
        { J3dTextureData owner(fixture(), heap); require(heap->available_bytes() < available, "live resources consume retained heap space"); }
        require(heap->available_bytes() == available, "retired resource allocation returns to the actual OS heap");
    }

    View tex1(View model) {
        std::size_t cursor = 0x20;
        for (u32 i = 0; i < be32(model, 0xC); ++i) {
            const auto size = be32(model, cursor + 4);
            require(size >= 8 && cursor <= model.size() && size <= model.size() - cursor, "real model block bounds");
            if (be32(model, cursor) == 0x54455831U) return model.subspan(cursor, size);
            cursor += size;
        }
        throw std::runtime_error("real model lacks TEX1");
    }
    void test_optional_disc(const std::shared_ptr<Mem1ResourceHeap>& heap) {
        const auto* path = std::getenv("SMGPC_REAL_DISC");
        if (!path || !*path) { std::cout << "[skip] original Mario TEX1 (set SMGPC_REAL_DISC)\n"; return; }
        aurora_dvd_close(); require(aurora_dvd_open(path), "disc must open");
        struct Disc { ~Disc() { aurora_dvd_close(); } } disc;
        DVDInit();
        std::unique_ptr<J3dTextureData> owner;
        {
            smgpc::runtime::DvdFileSystemService dvd{"/"};
            const auto archive_path = dvd.find_object_archive("Mario");
            require(archive_path.has_value(), "Mario archive must exist");
            const auto& archive = dvd.archive_for_path(*archive_path);
            const auto* file = archive.find_by_basename("mario.bdl");
            require(file != nullptr, "Mario model must exist");
            const auto source = tex1(archive.file_data(*file));
            owner = std::make_unique<J3dTextureData>(source, heap);
            check_headers(*owner, source);
            std::cout << "[resource] mario.bdl TEX1: textures=" << owner->texture().getNum() << ", bytes=" << source.size() << '\n';
        }
        check_headers(*owner, owner->source_bytes());
    }
}

int main() {
    try {
        rejects([] { (void)Mem1ResourceHeap::create(1024); }); // OSInit is an explicit prerequisite.
        aurora::g_config.mem1Size = 24 * 1024 * 1024;
        OSInit();
        const auto previous_current_heap = __OSCurrHeap;
        auto heap = Mem1ResourceHeap::create(8 * 1024 * 1024);
        require(__OSCurrHeap == previous_current_heap, "mapped resource setup does not select a global current heap");
        test_formats(heap);
        test_aliases_and_commands(heap);
        test_rejection_and_reuse(heap);
        test_optional_disc(heap);
        std::weak_ptr<Mem1ResourceHeap> weak = heap;
        auto retained = std::make_unique<J3dTextureData>(fixture(), heap);
        heap.reset();
        require(!weak.expired(), "texture allocation retains its actual heap owner");
        retained.reset();
        require(weak.expired() && AuroraOSIsAllocatorInitialized(), "last allocation retires heap but preserves process OS descriptor reservation");
        std::cout << "[pass] 5 original J3D texture-resource groups\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[fail] original J3D texture resource: " << e.what() << '\n';
        return 1;
    }
}
