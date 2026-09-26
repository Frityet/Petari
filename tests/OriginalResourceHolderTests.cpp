#include "NativeHeapFixture.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "Game/System/ArchiveHolder.hpp"
#include "Game/Camera/CameraAnim.hpp"
#include "Game/System/FileLoader.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "Game/Util/FileUtil.hpp"
#include "Game/Util/SystemUtil.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "OriginalStageResourceProcessFixture.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include "resource/GameResourceRuntime.hpp"
#include "resource/RarcArchive.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/JMapResource.hpp"
#include "resource/BtiTextureData.hpp"
#include "resource/BasResource.hpp"
#include "JSystem/JAudio2/JAUSoundAnimator.hpp"
#include "resource/TplTextureData.hpp"
#include "layout/LytTexMap.hpp"
#include "revolution/gx/GXGet.h"
#include "runtime/RuntimeServices.hpp"
#include "Game/Animation/MaterialAnmBuffer.hpp"
#include "Game/System/LayoutHolder.hpp"
#include "Game/System/ResourceHolderManager.hpp"
#include "Game/Util/HashUtil.hpp"
#include "Game/Animation/BpkPlayer.hpp"
#include "Game/Util/MutexHolder.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "JSystem/J3DGraphAnimator/J3DAnimation.hpp"
#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"
#include "JSystem/J3DGraphAnimator/J3DMaterialAnm.hpp"
#include "JSystem/J3DGraphBase/J3DMaterial.hpp"
#include "JSystem/J3DGraphLoader/J3DModelLoader.hpp"
#include "JSystem/JKernel/JKRExpHeap.hpp"
#include <aurora/aurora.h>
#include <aurora/dvd.h>
#include <aurora/guest_thread.hpp>
#include <dolphin/gd.h>
#include <dolphin/os.h>

#include <array>
#include <bit>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <vector>

namespace aurora { extern AuroraConfig g_config; }
namespace {
    using namespace smgpc::resource;

    using smgpc::test::on_resource_worker;
    using Bytes = std::vector<std::uint8_t>;
    void require(bool condition, const char* message) {
        if (!condition) {
            aurora::allocation::HostAllocationScope host;
            std::fprintf(stderr, "[resource-holder] assertion failed: %s\n", message);
            throw std::runtime_error(message);
        }
    }
    template<class F> void rejects(F call, const char* message) {
        bool caught = false;
        try { call(); } catch (const std::exception&) { caught = true; }
        require(caught, message);
    }
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
    constexpr auto long_name = "AuthoredAnimationNameRetainedBeyondLocalJMapReader";
    Bytes control() {
        constexpr std::array fields{"name", "interpole", "play_frame", "start_frame", "end_frame", "attribute"};
        const auto start = 16 + fields.size() * 12, stride = fields.size() * 4;
        Bytes b(start + stride * 2); put32(b, 0, 2); put32(b, 4, fields.size()); put32(b, 8, start); put32(b, 12, stride);
        for (std::size_t i = 0; i < fields.size(); ++i) {
            auto p = 16 + i * 12; put32(b, p, jmap_hash(fields[i])); put32(b, p + 4, 0xffffffff);
            put16(b, p + 8, i * 4); b[p + 11] = i == 0 ? 6 : 0;
        }
        for (int row = 0; row < 2; ++row) {
            auto p = start + row * stride; auto name = std::string_view(row ? long_name : "_default");
            put32(b, p, b.size() - (start + stride * 2));
            for (int col = 1; col < 6; ++col) put32(b, p + col * 4, row ? col + 2 : col);
            b.insert(b.end(), name.begin(), name.end()); b.push_back(0);
        }
        return b;
    }
    Bytes color(std::string_view name) {
        Bytes b(0x34); tag(b, 0, "PAK1"); b[8] = 3; put16(b, 0xa, 7); put16(b, 0xc, 7); put16(b, 0xe, 1);
        for (int p = 0x10; p < 0x18; p += 2) put16(b, p, 1);
        put32(b, 0x18, b.size()); auto p = b.size(); b.resize(p + 24);
        for (int c = 0; c < 4; ++c) put16(b, p + c * 6, 1);
        put32(b, 0x1c, b.size()); p = b.size(); b.resize(p + 4); put16(b, p, 0xffff);
        put32(b, 0x20, b.size()); p = b.size(); b.resize(p + 8 + name.size() + 1);
        put16(b, p, 1); put16(b, p + 2, 0xffff); put16(b, p + 4, RarcArchive::hash_name(name)); put16(b, p + 6, 8);
        tag(b, p + 8, name);
        for (int c = 0; c < 4; ++c) {
            put32(b, 0x24 + c * 4, b.size()); p = b.size(); b.resize(p + 4); put16(b, p, 10 + c * 20);
        }
        put32(b, 4, b.size()); return file("bpk1", std::move(b));
    }

    struct HolderFixture {
        std::shared_ptr<RarcArchive> source;
        JKRHeap::Handle domain;
        std::unique_ptr<ArchiveHolderArchiveEntry> entry;
        std::unique_ptr<ResourceHolder> object;

        HolderFixture(std::shared_ptr<RarcArchive> bytes, const char* path,
                      JKRHeap::Handle heap)
            : source(std::move(bytes)), domain(std::move(heap)) {
            const JKRHeap::CurrentHeapScope original(*(domain));
            const aurora::allocation::ClientAllocationScope originalRouting({true, true});
            entry = std::make_unique<ArchiveHolderArchiveEntry>(const_cast<u8*>(source->bytes().data()), &(*domain), path);
            object = std::make_unique<ResourceHolder>(*entry->mArchive);
        }
        ResourceHolder& holder() const { return *object; }
    };

    std::shared_ptr<RarcArchive> duplicate_archive() {
        Bytes image(64);
        image[0] = GX_TF_I4; put16(image, 2, 8); put16(image, 4, 8);
        image[0x18] = 1; put32(image, 0x1c, 32);
        Bytes camera(0x84);
        tag(camera, 0, "ANDOCANM"); put32(camera, 8, 1); put32(camera, 0x18, 1); put32(camera, 0x1c, 0x40);
        for (u32 i = 0; i < 8; ++i) {
            put32(camera, 0x20 + i * 8, 1); put32(camera, 0x24 + i * 8, i);
            put32(camera, 0x64 + i * 4, std::bit_cast<u32>(float(i + 1)));
        }
        put32(camera, 0x60, 32);
        return archive({{"Image.bti", image}, {"View.canm", camera}, {"Key.bck", transform(true)}, {"Sound.bas", Bytes(8)}});
    }

    void test_duplicate_holders(GameResourceRuntime& process, const JKRHeap::Handle& arena) {
        auto source = duplicate_archive();
        auto domain = smgpc::test::create_native_solid_heap(arena, 1U << 20);
        const JKRHeap::CurrentHeapScope original(*(domain));
        const aurora::allocation::ClientAllocationScope originalRouting({true, true});
        ArchiveHolderArchiveEntry entry(const_cast<u8*>(source->bytes().data()), &(*domain), "/Memory/Duplicates.arc");
        auto& mounted = *entry.mArchive;
        const auto* raw_image = mounted.getResource("Image.bti");
        auto* native_camera = static_cast<u8*>(mounted.getResource("View.canm"));
        require(native_camera != source->resource_data("View.canm").data() && CameraAnim::getAnimFrame(native_camera) == 1,
                "mounted archive publishes a native camera before any actor ResourceHolder exists");
        const auto* header = reinterpret_cast<const CanmFileHeader*>(native_camera);
        CamAnmDataAccessor camera_reader;
        camera_reader.set(native_camera + sizeof(CanmFileHeader),
                          native_camera + sizeof(CanmFileHeader) + header->mValueOffset + 4);
        TVec3f camera_pos;
        camera_reader.getPos(&camera_pos, 0);
        require(camera_pos.x == 1 && camera_pos.y == 2 && camera_pos.z == 3 && camera_reader.getFovy(0) == 8,
                "original camera accessors consume all native archive fields");
        const auto available = process.mem1_heap()->available_bytes();
        for (bool first_first : {false, true}) {
            auto first = std::make_unique<ResourceHolder>(mounted);
            auto second = std::make_unique<ResourceHolder>(mounted);
            auto* first_image = first->mFileInfoTable->getRes("Image.bti");
            auto* second_image = second->mFileInfoTable->getRes("Image.bti");
            auto* first_camera = first->mFileInfoTable->getRes("View.canm");
            auto* second_camera = second->mFileInfoTable->getRes("View.canm");
            require(first_image != second_image && first_camera == native_camera && second_camera == native_camera &&
                        mounted.getResource("Image.bti") == second_image && mounted.getResource("View.canm") == second_camera,
                    "holders own mutable texture records and share the archive's immutable camera data");
            auto* first_key = static_cast<J3DAnmTransformKey*>(first->mMotionResTable->getRes("Key"));
            auto* second_key = static_cast<J3DAnmTransformKey*>(second->mMotionResTable->getRes("Key"));
            require(first_key != second_key && first->mMotionResTable->findFileInfo("Key")->_8 ==
                        second->mMotionResTable->findFileInfo("Key")->_8,
                    "simultaneous holders have independent mutable J3D objects and identical original raw archive metadata");
            auto* first_sound = static_cast<JAUSoundAnimation*>(first->mBasResTable->getRes("Sound"));
            auto* second_sound = static_cast<JAUSoundAnimation*>(second->mBasResTable->getRes("Sound"));
            require(first_sound != second_sound && resolve_bas_animation(first_sound) == first_sound &&
                        resolve_bas_animation(second_sound) == second_sound,
                    "duplicate BAS tables retain independent valid native animation control records");
            ResourceHolder* survivor;
            if (first_first) {
                first.reset(); survivor = second.get();
                require(mounted.getResource("Image.bti") == second_image && mounted.getResource("View.canm") == second_camera,
                        "FIFO retirement preserves the newer texture and the archive camera");
            } else {
                second.reset(); survivor = first.get();
                require(mounted.getResource("Image.bti") == first_image && mounted.getResource("View.canm") == first_camera,
                        "reverse retirement restores the previous texture and preserves the archive camera");
            }
            auto* key = static_cast<J3DAnmTransformKey*>(survivor->mMotionResTable->getRes("Key"));
            J3DTransformInfo result; key->mFrame = 4; key->getTransform(0, &result);
            require(result.mScale.x == 2 && result.mRotation.x == -12 && result.mTranslate.x == 4,
                    "surviving duplicate holder still owns its actual mutable animation tables");
            first.reset(); second.reset();
            require(mounted.getResource("Image.bti") == raw_image && mounted.getResource("View.canm") == native_camera &&
                        process.mem1_heap()->available_bytes() == available,
                    "either retirement order releases converted textures while the archive camera remains available");
        }
        auto first = std::make_unique<ResourceHolder>(mounted);
        auto second = std::make_unique<ResourceHolder>(mounted);
        const auto index = source->find_resource("Image.bti")->file_entry_index;
        u8 unrelated = 0;
        mounted.mFiles[index].mFileData = &unrelated;
        first.reset(); second.reset();
        require(mounted.mFiles[index].mFileData == &unrelated,
                "retiring typed cache overrides must preserve a subsequent unrelated SDK cache replacement");
        mounted.mFiles[index].mFileData = const_cast<void*>(raw_image);
        entry.validateNativeRetirement();
    }

    void test_native_bti(GameResourceRuntime& process, const JKRHeap::Handle& arena) {
        Bytes bytes(0x2a0);
        bytes[0] = GX_TF_C8; bytes[1] = 1;
        put16(bytes, 2, 16); put16(bytes, 4, 8);
        bytes[6] = GX_REPEAT; bytes[7] = GX_MIRROR;
        bytes[8] = 1; bytes[9] = GX_TL_RGB5A3;
        put16(bytes, 0xa, 256); put32(bytes, 0xc, 0xa0);
        bytes[0x14] = GX_LINEAR; bytes[0x15] = GX_LINEAR; bytes[0x18] = 1;
        put16(bytes, 0x1a, -75); put32(bytes, 0x1c, 0x20);
        for (unsigned i = 0x20; i < bytes.size(); ++i) bytes[i] = i * 13;
        const auto available = process.mem1_heap()->available_bytes();
        {
            auto mapped = smgpc::layout::make_tex_map("image.bti", bytes, process.mem1_heap());
            auto host = smgpc::layout::make_tex_map("image.bti", bytes, nullptr);
            require(mapped.GetPaletteFormat() == GX_TL_RGB5A3 && mapped.GetPaletteEntryNum() == 256 &&
                    mapped.GetWrapModeS() == GX_REPEAT && mapped.mLODBias == static_cast<u16>(-192),
                    "SDK BTI mapping uses the true palette-format byte and retail unsigned bias storage");
            require(smgpc::layout::decode_tex_map(mapped).rgba == smgpc::layout::decode_tex_map(host).rgba,
                    "mapped and explicit host BTI retain identical encoded image and palette semantics");
        }
        require(process.mem1_heap()->available_bytes() == available, "SDK BTI objects release their retained mapped storage");
        auto source = archive({{"image.bti", bytes, true}});
        auto owner = std::make_unique<HolderFixture>(source, "/retained/Texture.arc", smgpc::test::create_native_solid_heap(arena, 1U << 20));
        const auto* entry = source->find_resource("image.bti");
        auto& holder = owner->holder();
        const auto* image = static_cast<const ResTIMG*>(holder.mFileInfoTable->getRes("image.bti"));
        require(image && image->mWidth == 16 && image->mHeight == 8 && image->mPaletteNum == 256 &&
                    image->mLodBias == -75 && image->mImageDataOffset == 0x20 && image->mPaletteDataOffset == 0xa0,
                "BTI retains native scalar fields and original relative offsets");
        require((reinterpret_cast<std::uintptr_t>(image) & 31U) == 0 &&
                    std::memcmp(reinterpret_cast<const u8*>(image) + 32, bytes.data() + 32, bytes.size() - 32) == 0,
                "BTI retains aligned, unchanged GX image and palette bytes");
        require(holder.mArchive->getResource("image.bti") == image &&
                    holder.mArchive->getResource(entry->file_id) == image &&
                    holder.mArchive->getIdxResource(entry->file_entry_index) == image &&
                    holder.mArchive->getResSize(image) == bytes.size(),
                "original holder, path, file ID and index share one retained BTI identity and size");
        source.reset();
        {
            JUTTexture texture(image, 0);
            require(texture.getWidth() == 16 && texture.getHeight() == 8 && texture.mLodBias == -75 &&
                        texture.mImage == reinterpret_cast<const u8*>(image) + 32,
                    "JUTTexture consumes the native header after caller archive retirement");
        }
        owner.reset();
        require(process.mem1_heap()->available_bytes() == available, "BTI cohort releases mapped texture storage");
        auto invalid = bytes; invalid[0x10] = 2;
        rejects([&] { BtiTextureData texture(invalid, process.mem1_heap()); }, "BTI rejects invalid native bool representations");
        invalid = bytes; put32(invalid, 0x1c, 0x280);
        rejects([&] { BtiTextureData texture(invalid, process.mem1_heap()); }, "BTI rejects truncated GX payloads before publication");
        invalid = bytes; put32(invalid, 0xc, 0xa1);
        rejects([&] { BtiTextureData texture(invalid, process.mem1_heap()); }, "BTI rejects unaligned palette data");
        require(process.mem1_heap()->available_bytes() == available, "rejected BTI does not retain mapped allocations");
    }

    void test_original_csv_reader(GameResourceRuntime& process, const JKRHeap::Handle& arena) {
        // A raw filename deliberately has no recognized table extension. The
        // original parser decides its type; the archive service supplies bounds.
        constexpr std::array fields{"name", "frame", "flag", "value", "vectorX", "vectorY"};
        constexpr unsigned start = 16 + fields.size() * 12, stride = fields.size() * 4;
        Bytes bytes(start + 2 * stride);
        put32(bytes, 0, 2); put32(bytes, 4, fields.size()); put32(bytes, 8, start); put32(bytes, 12, stride);
        for (unsigned i = 0; i < fields.size(); ++i) {
            const auto p = 16 + i * 12;
            put32(bytes, p, jmap_hash(fields[i]));
            put32(bytes, p + 4, i == 2 ? 0x40 : 0xffffffff);
            put16(bytes, p + 8, i * 4);
            bytes[p + 11] = i == 0 ? 6 : (i == 1 || i >= 4) ? 2 : 0;
        }
        put32(bytes, start + 4, std::bit_cast<std::uint32_t>(12.5f));
        put32(bytes, start + 8, 0x40); put32(bytes, start + 12, 0x1fe);
        put32(bytes, start + 16, std::bit_cast<std::uint32_t>(1.25f));
        put32(bytes, start + 20, std::bit_cast<std::uint32_t>(-0.5f));
        put32(bytes, start + stride, std::strlen(long_name) + 1);
        put32(bytes, start + stride + 4, std::bit_cast<std::uint32_t>(-2.25f));
        bytes.insert(bytes.end(), long_name, long_name + std::strlen(long_name) + 1);
        bytes.push_back(0);
        auto source = archive({{"authored.table", std::move(bytes)}, {"unrelated.raw", {1, 2, 3}}});
        const std::weak_ptr<const RarcArchive> weak_source = source;
        auto domain = smgpc::test::create_native_solid_heap(arena, 1U << 20);
        auto owner = std::make_unique<HolderFixture>(source, "/retained/Csv.arc", domain);
        auto& holder = owner->holder();
        require(MR::isExistFileInArc(&holder, "%s.%s", "authored", "table"), "original variadic filename lookup");
        require(MR::tryCreateCsvParser(&holder, "%s.table", "missing") == nullptr, "missing optional table remains null");
        std::unique_ptr<JMapInfo> parser(MR::createCsvParser(&holder, "%s.%s", "authored", "table"));
        require(parser && MR::getCsvDataElementNum(parser.get()) == 2, "original parser attaches raw archive table");
        const void* identity = holder.mFileInfoTable->getRes("authored.table");
        const char* name = nullptr;
        MR::getCsvDataStr(&name, parser.get(), "name", 0);
        require(name && std::string_view(name) == long_name, "original string field");
        const char* empty = "initial";
        MR::getCsvDataStrOrNULL(&empty, parser.get(), "name", 1);
        require(empty == nullptr, "original empty string becomes null");
        float frame = 0;
        MR::getCsvDataF32(&frame, parser.get(), "frame", 0);
        require(frame == 12.5f, "authored fractional frame decoded without integer conversion");
        MR::getCsvDataF32(&frame, parser.get(), "missing", 0);
        require(frame == 12.5f, "missing float preserves original caller default");
        MR::getCsvDataF32(&frame, parser.get(), "frame", 1);
        require(frame == -2.25f, "second row reads negative authored float");
        Vec vector{7, 8, 9};
        MR::getCsvDataVec(&vector, parser.get(), "vector", 0);
        require(vector.x == 1.25f && vector.y == -0.5f && vector.z == 9,
                "original vector reader formats XYZ keys and preserves missing component defaults");
        bool flag = false;
        MR::getCsvDataBool(&flag, parser.get(), "flag", 0);
        require(flag, "original masked boolean");
        MR::getCsvDataBool(&flag, parser.get(), "missing", 0);
        require(flag, "missing boolean preserves original caller default");
        MR::getCsvDataBool(&flag, parser.get(), "flag", 1);
        require(!flag, "zero masked boolean");
        u8 small = 0;
        MR::getCsvDataU8(&small, parser.get(), "value", 0);
        require(small == 0xfe, "original byte conversion truncates to eight bits");
        MR::getCsvDataU8(&small, parser.get(), "missing", 0);
        require(small == 0, "original missing byte uses zero local");
        rejects([&] { JMapInfo invalid; invalid.attach(holder.mFileInfoTable->getRes("unrelated.raw")); },
                "unrelated bytes are parsed only when explicitly attached and then rejected");
        source.reset(); domain.reset();
        rejects([&] { holder.mArchive->validateNativeRetirement(holder.nativeArchiveReferenceCount()); },
                "an attached parser prevents retiring its original fixed archive bytes");
        const char* surviving = nullptr;
        MR::getCsvDataStr(&surviving, parser.get(), "name", 0);
        require(!weak_source.expired() && surviving == name && std::string_view(surviving) == long_name,
                "live original archive ownership preserves decoded names and exact source bytes");
        parser.reset();
        holder.mArchive->validateNativeRetirement(holder.nativeArchiveReferenceCount());
        owner.reset();
        require(weak_source.expired() && !find_jmap_resource(identity),
                "parser retirement permits archive unpublication and releases its byte owner");
    }

    void test_original_constructor(GameResourceRuntime& process, const JKRHeap::Handle& arena) {
        auto source = archive({{"Key.bck", transform(true)}, {"Full.bca", transform(false), true},
                               {"fixture_root.banmt", control()}, {"Mixed.BCK", {7, 8}, true}, {"raw.pa", {9}, true},
                               {"Empty.bck", {}}});
        const std::weak_ptr<const RarcArchive> weak_source = source;
        auto domain = smgpc::test::create_native_solid_heap(arena, 1U << 20); const std::weak_ptr<JKRHeap> weak_domain = domain;
        auto owner = std::make_unique<HolderFixture>(source, "/retained/Fixture.arc", domain);
        auto& h = owner->holder();
        require(h.mHeap == &(*domain) && JKRHeap::findFromRoot(&h) == h.mHeap, "actual holder allocated in mounted cohort");
        require(h.mMotionResTable->mCount == 3 && h.mBanmtResTable->mCount == 1 && h.mFileInfoTable->mCount == 2,
                "original case-sensitive dispatch and recursive counts");
        const auto* empty_info = h.mMotionResTable->findFileInfo("Empty");
        require(empty_info && empty_info->mResource == nullptr && empty_info->_8 == nullptr && empty_info->_4 == 0,
                "original zero-length BCK preserves its explicit null resource entry");
        require(h.mModelResTable == &h.mDefaultTable && h.mMaterialBuf == nullptr && h.mBackupMaterialData == nullptr,
                "actual default table and absent model state");
        const auto* key_info = h.mMotionResTable->findFileInfo("KEY");
        require(key_info && key_info->_C == 100 && key_info->_8 == source->resource_data("Key.bck").data() &&
                key_info->_4 == transform(true).size() && key_info->mResource != key_info->_8,
                "actual hashed names, non-index file ID, raw metadata and separate typed object");
        require(h.mFileInfoTable->getRes("raw.pa") == source->resource_data("raw.pa").data(), "raw resources preserve byte identity");
        auto* key = dynamic_cast<J3DAnmTransformKey*>(static_cast<J3DAnmBase*>(key_info->mResource));
        auto* full = dynamic_cast<J3DAnmTransformFull*>(static_cast<J3DAnmBase*>(h.mMotionResTable->getRes("Full")));
        require(key && full && key->getFrameMax() == 7 && key->mAttribute == 3, "original concrete animation classes and metadata");
        J3DTransformInfo result; key->mFrame = 4; key->getTransform(0, &result);
        require(result.mScale.x == 2 && result.mRotation.x == -12 && result.mTranslate.x == 4, "original key sampler");
        full->mFrame = 1; full->getTransform(0, &result);
        require(result.mScale.x == 3 && result.mRotation.x == 30 && result.mTranslate.x == 9, "original full sampler");
        require(h.mBckCtrl && h.mBckCtrl->mDefaultCtrlData.mPlayFrame == 2, "actual original BckCtrl default row");
        const auto* row = h.mBckCtrl->find(long_name);
        require(row && row->mPlayFrame == 4 && row->mInterpole == 3 && row->mLoopMode == 7, "actual authored BckCtrl settings");
        auto* borrowed = row->mName;
        require(JKRHeap::findFromRoot(const_cast<char*>(borrowed)) == nullptr, "shared JMap string cache is host-owned");
        source.reset(); domain.reset();
        for (int i = 0; i < 16; ++i) {
            auto churn = JMapResource(control());
            require(std::string_view(borrowed) == long_name && h.mBckCtrl->find(long_name)->mName == borrowed,
                    "borrowed name survives local reader/source destruction and other tables");
        }
        require(!weak_source.expired() && !weak_domain.expired(), "typed owner retains source and Game arena");
        const auto* raw_map = h.mBanmtResTable->getRes("fixture_root");
        owner.reset();
        require(weak_source.expired() && weak_domain.expired() && !find_jmap_resource(raw_map), "full teardown removes aliases before source and arena expire");
    }

    Bytes indexed_tpl() {
        // Two descriptors alias the same native image/header/CLUT. Both mip
        // levels and palette remain in the original encoded GX block layout.
        Bytes bytes(0xe0);
        put32(bytes, 0, 0x0020af30); put32(bytes, 4, 2); put32(bytes, 8, 0xc);
        for (int i = 0; i < 2; ++i) { put32(bytes, 0xc + i * 8, 0x20); put32(bytes, 0x10 + i * 8, 0x50); }
        put16(bytes, 0x20, 8); put16(bytes, 0x22, 8); put32(bytes, 0x24, GX_TF_C4); put32(bytes, 0x28, 0xa0);
        put32(bytes, 0x2c, GX_REPEAT); put32(bytes, 0x30, GX_MIRROR);
        put32(bytes, 0x34, GX_LIN_MIP_LIN); put32(bytes, 0x38, GX_LINEAR);
        put32(bytes, 0x3c, std::bit_cast<std::uint32_t>(0.5F)); bytes[0x40] = 1; bytes[0x42] = 1;
        put16(bytes, 0x50, 16); put32(bytes, 0x54, GX_TL_RGB565); put32(bytes, 0x58, 0x80);
        put16(bytes, 0x80, 0xf800);
        return bytes;
    }

    void test_sdk_tex_map(GameResourceRuntime& process) {
        using namespace smgpc::layout;
        rejects([] { (void)MR::createLytTexMap(nullptr, "Picture.bti"); },
                "original texture factory rejects an invalid archive name before dispatch");
        auto bytes = indexed_tpl();
        const auto available = process.mem1_heap()->available_bytes();
        {
            TplTextureData owner(bytes, process.mem1_heap());
            auto palette = owner.palette();
            const auto& a = palette.descriptorArray[0];
            require(a.textureHeader == palette.descriptorArray[1].textureHeader &&
                    a.CLUTHeader == palette.descriptorArray[1].CLUTHeader,
                    "native TPL descriptors preserve aliased header and CLUT identity");
            require(std::memcmp(a.textureHeader->data, bytes.data() + 0xa0, 64) == 0 &&
                    std::memcmp(a.CLUTHeader->data, bytes.data() + 0x80, 32) == 0,
                    "native TPL retains full mip chain and palette bytes");
            nw4r::lyt::TexMap image;
            image.SetWrapMode(GX_MIRROR, GX_REPEAT); image.SetFilter(GX_NEAR, GX_LINEAR);
            image.ReplaceImage(&palette, 3);
            require(image.GetWrapModeS() == GX_MIRROR && image.GetMinFilter() == GX_NEAR &&
                    image.mImage == a.textureHeader->data,
                    "original ReplaceImage wraps descriptor IDs and preserves the existing sampler");
        }
        auto texture = make_tex_map("archive:Picture.tpl", bytes, process.mem1_heap());
        require(texture.GetTexelFormat() == GX_TF_C4 && texture.mWidth == 8 && texture.mHeight == 8 &&
                texture.GetWrapModeS() == GX_REPEAT && texture.GetWrapModeT() == GX_MIRROR &&
                texture.GetMinFilter() == GX_LIN_MIP_LIN && texture.GetPaletteEntryNum() == 16 &&
                texture.GetLODBias() == 0.5F && texture.GetMaxLOD() == 1 && texture.IsMipMap(),
                "SDK TexMap receives native TPL image, palette, sampler and mip metadata");
        GXTexObj gx{};
        GXInitTexObjTlut(&gx, GX_TLUT3);
        texture.Get(&gx);
        nw4r::lyt::TexMap roundtrip;
        roundtrip.Set(gx);
        require(roundtrip.mImage == texture.mImage && roundtrip.mWidth == 8 && roundtrip.GetTexelFormat() == GX_TF_C4 &&
                roundtrip.GetMinFilter() == texture.GetMinFilter() && roundtrip.GetWrapModeT() == GX_MIRROR &&
                roundtrip.GetLODBias() == 0.5F && roundtrip.IsEdgeLODEnable() && GXGetTexObjTlut(&gx) == GX_TLUT3,
                "actual SDK Get/Set round trip uses full native GX image pointers and sampler state");
        auto copy = texture;
        const std::weak_ptr<const nw4r::lyt::HostTextureResourceState> weak = texture.GetHostResourceState();
        texture = {};
        bytes.clear();
        const auto decoded = decode_tex_map(copy);
        require(!weak.expired() && decoded.rgba[0] == 255 && decoded.rgba[1] == 0 && decoded.rgba[3] == 255,
                "copied SDK TexMap retains and decodes indexed GX bytes after its original source retires");
        copy = {};
        require(weak.expired() && process.mem1_heap()->available_bytes() == available,
                "last SDK TexMap copy releases mapped encoded storage");
        auto host = make_tex_map("Picture.tpl", indexed_tpl(), nullptr);
        require(decode_tex_map(host).rgba == decoded.rgba && process.mem1_heap()->available_bytes() == available,
                "explicit standalone host backing uses identical descriptor and indexed decoding semantics");
        auto invalid = indexed_tpl(); invalid.resize(0xc0);
        rejects([&] { (void)make_tex_map("bad.tpl", invalid, process.mem1_heap()); },
                "native TPL rejects a missing mip level before publishing image pointers");
        auto maximum_lod = indexed_tpl(); maximum_lod[0x42] = 10; maximum_lod.resize(0x120);
        require(read_tpl_palette(maximum_lod).descriptors[0].image_levels.size() == 4,
                "TPL mip storage stops at 1x1 even when sampler maximum LOD is larger");
        invalid = indexed_tpl(); put32(invalid, 8, 0xfffffff8);
        rejects([&] { (void)make_tex_map("bad.tpl", invalid, process.mem1_heap()); },
                "native TPL rejects overflowed descriptor offsets before publication");
        require(process.mem1_heap()->available_bytes() == available, "rejected TPL resources release all native storage");
    }

    void test_original_layout_holder(GameResourceRuntime& process, const JKRHeap::Handle& arena) {
        Bytes layout(16), animation(16), texture = indexed_tpl(), font(16);
        tag(layout, 0, "RLYT"); put32(layout, 4, 0xFEFF0008);
        tag(animation, 0, "RLAN"); put32(animation, 4, 0xFEFF0008);
        auto source = archive({{"Window.brlyt", layout}, {"Appear.brlan", animation, true},
                               {"Picture.tpl", texture, true}, {"Font.brfnt", font}});
        auto domain = smgpc::test::create_native_solid_heap(arena, 1U << 20);
        auto& manager = *SingletonHolder<ResourceHolderManager>::get();
        auto& files = *SingletonHolder<FileLoader>::get();
        // The process observer deliberately routes test scaffolding to host.
        // Restore only original callback allocation policy for synchronous
        // factories; CurrentHeapRestorer still chooses the mounted Game heap.
        auto raw_layout = [&](const char* name) {
            const aurora::allocation::ClientAllocationScope game;
            return manager.createAndAddLayoutHolderRawData(name);
        };
        auto stationed_layout = [&](const char* name) {
            const aurora::allocation::ClientAllocationScope game;
            return manager.createAndAddLayoutHolderStationed(name);
        };
        auto stationed_resource = [&](const char* name) {
            const aurora::allocation::ClientAllocationScope game;
            return manager.createAndAddStationed(name);
        };
        std::vector<Bytes> mounted_bytes;
        auto mount = [&](const char* name, JKRHeap* heap) {
            mounted_bytes.emplace_back(source->bytes().begin(), source->bytes().end());
            aurora::allocation::HostAllocationScope host;
            return files.createAndAddArchive(mounted_bytes.back().data(), heap, name);
        };
        auto* mounted = mount("/Memory/LayoutFixture.arc", &(*domain));
        const std::weak_ptr<const RarcArchive> weak_archive = mounted->retainSource();
        auto* holder = raw_layout("/Memory/LayoutFixture.arc");
        require(holder && holder->mArchive == mounted, "layout holder borrows the original mounted archive identity");
        require(JKRHeap::findFromRoot(holder) == &(*domain), "original layout holder uses its mounted archive heap");
        auto* stationed = stationed_layout("/Memory/LayoutFixture.arc");
        require(stationed != holder && stationed->mArchive == mounted,
                "original stationed creation makes a separate holder over the same actual archive");
        require(holder->mLayoutRes.mCount == 1 && holder->mAnimRes.mCount == 1 && holder->getResOtherNum() == 2,
                "original layout enumeration traverses nested directories and excludes dot links");
        u32 header_word = 0;
        auto* layout_bytes = holder->GetResource('blyt', "window.BRLYT", &header_word);
        require(layout_bytes == mounted->getResource("Window.brlyt") && header_word == 0xFEFF0008,
                "original layout accessor preserves archive identity and the retail big-endian second header word");
        require(holder->GetResource('anim', "APPEAR.BRLAN", nullptr) == mounted->getResource("Appear.brlan"),
                "original animation lookup preserves extension and case-insensitive resource hash");
        require(holder->isAnimationHashEqual(MR::getHashCodeLower("Appear.brlan"), 0),
                "original animation hash lookup sees nested animation resources");
        require(holder->GetResource(0, "Picture.tpl", nullptr) == mounted->getResource("Picture.tpl") &&
                holder->isExistResOther("Font.brfnt") && holder->GetResource(0, "Font.brfnt", nullptr) == nullptr,
                "original accessor keeps font resources in the raw table but routes fonts through GetFont");
        header_word = 42;
        require(holder->GetResource('blyt', "Absent.brlyt", &header_word) == nullptr && header_word == 0,
                "missing original layout resources return null and clear the size output");
        require(holder->GetFont("MenuFont64.brfnt") == MR::getMenuFontNW4R(),
                "layout fonts resolve the actual original process font owner");
        const auto texture_available = process.mem1_heap()->available_bytes();
        auto texture_copy = smgpc::layout::make_tex_map("Picture.tpl",
            holder->nativeResourceSource().resource_data("Picture.tpl"), process.mem1_heap());
        const std::weak_ptr<const nw4r::lyt::HostTextureResourceState> texture_lifetime = texture_copy.GetHostResourceState();
        auto retained = holder->retainNativeResources();
        rejects([&] { MR::removeResourceAndFileHolderIfIsEqualHeap(&(*domain)); },
                "live layout borrowers reject the entire original holder/file removal");
        rejects([&] { files.removeHolderIfIsEqualHeap(&(*domain)); },
                "FileLoader rejects archive retirement before its actual holders");
        require(files.receiveArchive("/Memory/LayoutFixture.arc") == mounted &&
                holder->GetResource('blyt', "Window.brlyt", nullptr) == layout_bytes,
                "failed retirement leaves the complete original registry and byte identities intact");
        retained.reset();
        MR::removeResourceAndFileHolderIfIsEqualHeap(&(*domain));
        require(weak_archive.expired() && !files.isMountedArchive("/Memory/LayoutFixture.arc"),
                "layout retirement releases holders before the actual mounted archive");
        require(!texture_lifetime.expired() && smgpc::layout::decode_tex_map(texture_copy).rgba[0] == 255,
                "copied native TPL remains valid after the mounted layout owner retires");
        texture_copy = {};
        require(texture_lifetime.expired() && process.mem1_heap()->available_bytes() == texture_available,
                "final copied TPL releases all mapped backing");
        mounted = mount("/Memory/LayoutFixture.arc", &(*domain));
        require(raw_layout("/Memory/LayoutFixture.arc")->mArchive == mounted,
                "retired layout names may bind to their new actual mount");
        MR::removeResourceAndFileHolderIfIsEqualHeap(&(*domain));
        rejects([&] { raw_layout("/Memory/Missing.arc"); },
                "missing original raw layout mounts remain explicit failures");

        auto duplicate_source = duplicate_archive();
        mounted_bytes.emplace_back(duplicate_source->bytes().begin(), duplicate_source->bytes().end());
        auto* duplicate_mount = files.createAndAddArchive(mounted_bytes.back().data(), &(*domain), "/Memory/Duplicates.arc");
        const auto* raw_image = duplicate_mount->getResource("Image.bti");
        const auto* raw_camera = duplicate_mount->getResource("View.canm");
        auto* first_resource = stationed_resource("/Memory/Duplicates.arc");
        auto* second_resource = stationed_resource("/Memory/Duplicates.arc");
        require(first_resource != second_resource && first_resource->mArchive == duplicate_mount &&
                    second_resource->mArchive == duplicate_mount && first_resource->mMotionResTable->getRes("Key") !=
                    second_resource->mMotionResTable->getRes("Key"),
                "original stationed requests publish distinct native holders over the same actual mounted archive");
        manager.removeIfIsEqualHeap(&(*domain));
        require(duplicate_mount->getResource("Image.bti") == raw_image && duplicate_mount->getResource("View.canm") == raw_camera,
                "original manager FIFO retirement restores typed overrides before FileLoader removes its archive");
        files.removeHolderIfIsEqualHeap(&(*domain));

        auto foreign_domain = smgpc::test::create_native_solid_heap(arena, 1U << 20);
        auto sibling_domain = smgpc::test::create_native_solid_heap(arena, 1U << 20);
        const std::weak_ptr<JKRHeap> weak_foreign = foreign_domain;
        auto* foreign_heap = &(*foreign_domain);
        mount("/Memory/ForeignLayout.arc", foreign_heap);
        auto* foreign = raw_layout("/Memory/ForeignLayout.arc");
        require(JKRHeap::findFromRoot(foreign) == foreign_heap,
                "a layout on another registered heap resolves its actual owner outside any Game scope");
        mount("/Memory/SiblingLayout.arc", foreign_heap);
        {
            const JKRHeap::CurrentHeapScope original(*(sibling_domain));
            const aurora::allocation::ClientAllocationScope originalRouting({true, true});
            auto* other = raw_layout("/Memory/SiblingLayout.arc");
            require(JKRHeap::findFromRoot(other) == foreign_heap && JKRHeap::sCurrentHeap == &(*sibling_domain),
                    "opening a foreign layout under a sibling scope retains the mounted heap and restores the current heap");
        }
        foreign_domain.reset();
        require(!weak_foreign.expired(), "actual archive and layout owners retain their foreign heap");
        MR::removeResourceAndFileHolderIfIsEqualHeap(foreign_heap);
        require(weak_foreign.expired(), "retiring original layouts and archives releases their actual heap owner");
    }

    void test_original_texture_factory(GameResourceRuntime& process, const JKRHeap::Handle& arena) {
        auto source = duplicate_archive();
        auto domain = smgpc::test::create_native_solid_heap(arena, 1U << 20);
        auto& files = *SingletonHolder<FileLoader>::get();
        auto& manager = *SingletonHolder<ResourceHolderManager>::get();
        Bytes bytes(source->bytes().begin(), source->bytes().end());
        const auto available = process.mem1_heap()->available_bytes();
        auto* mounted = files.createAndAddArchive(bytes.data(), &(*domain), "FactoryFixture.arc");
        auto first = std::unique_ptr<nw4r::lyt::TexMap>(MR::createLytTexMap("FactoryFixture.arc", "Image.bti"));
        auto* holder = manager.createAndAdd("FactoryFixture.arc", nullptr);
        require(holder && holder->mArchive == mounted &&
                    MR::loadTexFromArc("FactoryFixture.arc", "Image.bti") == holder->mFileInfoTable->getRes("Image.bti"),
                "a texture-first factory request publishes the original ResourceHolder rather than poisoning its registry type");
        rejects([] { (void)MR::createLytTexMap("FactoryFixture.arc", "Absent.bti"); },
                "the original factory rejects a missing texture in its actual mounted archive");
        const auto allocated = process.mem1_heap()->available_bytes();
        for (unsigned i = 0; i < 520; ++i) {
            auto repeated = std::unique_ptr<nw4r::lyt::TexMap>(MR::createLytTexMap("FactoryFixture.arc", "Image.bti"));
            require(repeated->mImage == first->mImage && manager.createAndAdd("FactoryFixture.arc", nullptr) == holder &&
                        process.mem1_heap()->available_bytes() == allocated,
                    "repeated texture requests beyond the registry capacity reuse one holder and its native image");
        }
        const auto expected = smgpc::layout::decode_tex_map(*first).rgba;
        auto copy = *first;
        const std::weak_ptr<const nw4r::lyt::HostTextureResourceState> lifetime = copy.GetHostResourceState();
        {
            // The holder is already published: this cached query does not wait
            // for a main-thread resource task while selecting the fixture heap.
            const JKRHeap::CurrentHeapScope original(*(domain));
            const aurora::allocation::ClientAllocationScope originalRouting({true, true});
            auto* heap_texture = MR::createLytTexMap("FactoryFixture.arc", "Image.bti");
            auto* heap_copy = new nw4r::lyt::TexMap(copy);
            require(JKRHeap::findFromRoot(heap_texture) == &(*domain) &&
                        JKRHeap::findFromRoot(heap_copy) == &(*domain),
                    "the original factory and copied TexMap use the caller's current heap");
        }
        first.reset();
        MR::removeResourceAndFileHolderIfIsEqualHeap(&(*domain));
        require(!files.isMountedArchive("FactoryFixture.arc") && smgpc::layout::decode_tex_map(copy).rgba == expected,
                "copied texture backing survives original holder and archive retirement independently");
        copy = {};
        require(!lifetime.expired(), "the heap-owned TexMap copy retains its native backing until heap reuse");
        (*domain).freeAll();
        require(lifetime.expired() && process.mem1_heap()->available_bytes() == available,
                "bulk JKR heap reuse finalizes original and copied TexMaps and releases every mapped texture allocation");
    }

    void test_failure_scope(GameResourceRuntime& process, const JKRHeap::Handle& arena) {
        Bytes bad(0x20); tag(bad, 0, "J3D2bdl4"); put32(bad, 8, bad.size());
        auto source = archive({{"bad.bdl", bad}});
        auto domain = smgpc::test::create_native_solid_heap(arena, 1U << 20);
        alignas(32) std::array<u8, 64> bytes{}; GDLObj prior{}; GDInitGDLObj(&prior, bytes.data(), bytes.size());
        auto* old_gd = __GDCurrentDL; GDSetCurrent(&prior);
        auto* old_heap = JKRHeap::sCurrentHeap;
        OSLockMutex(&MR::MutexHolder<0>::sMutex); OSLockMutex(&MR::MutexHolder<0>::sMutex);
        rejects([&] { HolderFixture failed(source, "bad.arc", domain); }, "malformed real loader must fail construction");
        require(MR::MutexHolder<0>::sMutex.thread == OSGetCurrentThread() && MR::MutexHolder<0>::sMutex.count == 2,
                "host exception restores exactly caller's recursive load-mutex depth");
        require(__GDCurrentDL == &prior && JKRHeap::sCurrentHeap == old_heap, "host exception restores GD and current heap");
        OSUnlockMutex(&MR::MutexHolder<0>::sMutex); OSUnlockMutex(&MR::MutexHolder<0>::sMutex);
        auto previous = OSDisableInterrupts(); require(previous, "failure releases added interrupt suppression"); OSRestoreInterrupts(previous);
        require(OSDisableScheduler() == 0, "failure releases added scheduler suppression"); OSEnableScheduler();
        GDSetCurrent(old_gd);
        bool acquired = false;
        std::thread other([&] { acquired = OSTryLockMutex(&MR::MutexHolder<0>::sMutex); if (acquired) OSUnlockMutex(&MR::MutexHolder<0>::sMutex); });
        {
            // This callback owns the guest CPU; let the SDK probe execute while
            // the host join waits, then restore the callback's CPU ownership.
            const aurora::os::GuestThreadWaitScope wait;
            other.join();
        }
        require(acquired, "another SDK thread can acquire load mutex after failure");
        HolderFixture valid(archive({{"raw", {1}}}), "valid.arc", domain);
        require(valid.holder().mFileInfoTable->mCount == 1, "failed holder never prevents subsequent valid construction");
    }

    void test_original_manager_lifetime(GameResourceRuntime& process, const JKRHeap::Handle& arena) {
        smgpc::runtime::DvdFileSystemService dvd("/");
        auto temporary_domain = smgpc::test::create_native_solid_heap(arena, 1U << 20);
        const std::weak_ptr<JKRHeap> weak_temporary = temporary_domain;
        RarcArchive* cached = nullptr;
        {
            const JKRHeap::CurrentHeapScope original(*(temporary_domain));
            const aurora::allocation::ClientAllocationScope originalRouting({true, true});
            cached = &dvd.archive("/ObjectData/InvisibleWall10x10.arc");
            require(JKRHeap::findFromRoot(cached) == nullptr &&
                        JKRHeap::findFromRoot(const_cast<u8*>(cached->bytes().data())) == nullptr,
                    "direct DVD cache entry and retained byte buffer escape temporary Game allocations");
        }
        temporary_domain.reset();
        require(weak_temporary.expired() && cached->resource_data("InvisibleWall10x10.kcl").size() == 1222,
                "DVD cache survives the original scope and allocation domain that first requested it");
        auto& manager = *SingletonHolder<ResourceHolderManager>::get();
        auto* holder = manager.createAndAdd("InvisibleWall10x10.arc", nullptr);
        require(holder == manager.createAndAdd("InvisibleWall10x10.arc", nullptr),
                "original basename-hash lookup preserves the already-published holder");
        auto retained = holder->retainNativeResources();
        rejects([&] { manager.validateHeapRetirement(&holder->heap()); },
                "a live model borrower rejects original manager heap retirement");
        require(holder->mArchive->getResSize(holder->mFileInfoTable->getRes("CollisionVersion")) == 7,
                "rejected retirement leaves the original resource readable");
        retained.reset();
        require(dvd.archive_load_count("/ObjectData/InvisibleWall10x10.arc") == 1,
                "DVD resource caching remains independent of original holder ownership");
    }

    void test_real_model_and_material(GameResourceRuntime& process, const JKRHeap::Handle& arena) {
        smgpc::runtime::DvdFileSystemService dvd("/");
        auto& manager = *SingletonHolder<ResourceHolderManager>::get();
        for (const auto& [archive_name, image_name] : std::array{
                std::pair{"MarineSnow.arc", "MarineSnow.bti"}, std::pair{"StarPointerBlur.arc", "Blur.bti"}}) {
            auto* h = manager.createAndAdd(archive_name, nullptr);
            const auto raw = h->nativeResourceSource().resource_data(image_name);
            const auto* image = MR::loadTexFromArc(archive_name, image_name);
            const auto u16_at = [&](unsigned offset) { return (unsigned(raw[offset]) << 8) | raw[offset + 1]; };
            require(raw.size() >= 32 && image && image->mWidth == u16_at(2) && image->mHeight == u16_at(4) &&
                        image->mPaletteNum == u16_at(0xa) &&
                        std::memcmp(reinterpret_cast<const u8*>(image) + 32, raw.data() + 32, raw.size() - 32) == 0,
                    "real BTI archive query retains original dimensions, palette and full payload");
            if (std::string_view(archive_name) == "MarineSnow.arc")
                require(MR::loadTexFromArc("MarineSnow") == image, "single-name original texture query shares holder identity");
            JUTTexture texture(image, 0);
            require(texture.getWidth() == u16_at(2) && texture.getHeight() == u16_at(4), "real BTI consumed by JUTTexture");
            std::cout << "PASS real BTI " << archive_name << '/' << image_name << ' '
                      << image->mWidth << 'x' << image->mHeight << '\n';
        }
        {
            auto* layout = manager.createAndAddLayoutHolder("SysInfoWindowMini.arc", nullptr);
            const auto& source = layout->nativeResourceSource();
            unsigned count = 0;
            for (const auto& entry : source.entries()) {
                if (!entry.path.ends_with(".tpl")) continue;
                auto texture = smgpc::layout::make_tex_map(entry.path, source.file_data(entry), process.mem1_heap());
                const auto expected = decode_tpl_texture(source.file_data(entry));
                require(smgpc::layout::decode_tex_map(texture).rgba == expected.rgba &&
                        texture.mWidth == expected.width && texture.mHeight == expected.height,
                        "actual layout TPL preserves the encoded image and palette colors");
                ++count;
            }
            require(count > 0, "real layout contains actual TPL textures");
            std::cout << "PASS real layout TPL " << layout->nativeResourcePath().generic_string() << " textures=" << count << '\n';
        }
        auto* original = manager.createAndAdd("Mario.arc", nullptr);
        auto* model = static_cast<J3DModelData*>(original->mModelResTable->getRes(original->getModelName()));
        require(model && model->getMaterialNum() == 9, "real Mario holder contains actual complete nine-material model");
        const auto source = original->nativeResourceSource().resource_data("Mario.bdl");
        const auto material_name = std::string_view(model->getMaterialName()->getName(0));
        const auto before = process.mem1_heap()->available_bytes();
        {
            auto mixed = archive({{"Model.bdl", Bytes(source.begin(), source.end())}, {"Color.bpk", color(material_name)}});
            // The retail archive's heap is sized for its loaded resources.
            // Synthetic model/material duplicates belong to the test arena.
            HolderFixture owner(mixed, "Mixed.arc", smgpc::test::create_native_solid_heap(arena, 2U << 20));
            auto& h = owner.holder(); auto* m = static_cast<J3DModelData*>(h.mModelResTable->getRes("Model"));
            std::vector<std::array<float, 16>> initial_effect_matrices(m->getMaterialNum() * 8);
            for (unsigned i = 0; i < m->getMaterialNum(); ++i) for (unsigned j = 0; j < 8; ++j) {
                auto* tex = m->getMaterialNodePointer(i)->getTexGenBlock()->getTexMtx(j);
                for (unsigned row = 0; row < 4; ++row) for (unsigned col = 0; col < 4; ++col) {
                    const auto expected = tex ? tex->getTexMtxInfo().mEffectMtx[row][col] : float(row == col);
                    initial_effect_matrices[i * 8 + j][row * 4 + col] = expected;
                    require(h.getInitEffectMtx(i, j)[row][col] == expected, "original effect-matrix copy/identity backup");
                }
            }
            auto* animation = static_cast<J3DAnmColorKey*>(h.mBpkResTable->getRes("Color"));
            require(!h.isCreatedAtSameHeap(original) && h.mMaterialBuf != nullptr,
                    "synthetic resources use their bounded test heap and original combined material-animation constructor");
            {
                const JKRHeap::CurrentHeapScope game(*(owner.domain));
                const aurora::allocation::ClientAllocationScope gameRouting({true, true});
                ResourceHolder duplicate(*h.mArchive);
                auto* duplicate_model = static_cast<J3DModelData*>(duplicate.mModelResTable->getRes("Model"));
                require(duplicate.isCreatedAtSameHeap(&h) && duplicate_model != m &&
                            duplicate_model->getMaterialNum() == m->getMaterialNum() &&
                            duplicate.mModelResTable->findFileInfo("Model")->_8 == h.mModelResTable->findFileInfo("Model")->_8 &&
                            duplicate.mMaterialBuf != h.mMaterialBuf,
                        "duplicate actual model holders preserve raw archive identity with independent loaded materials and animation buffers");
            }
            require(animation->getUpdateMaterialID(0) == 0 && h.mMaterialBuf->getDiffFlag(0) != 0 &&
                    m->getMaterialNodePointer(0)->getMaterialAnm() == h.mMaterialBuf->_0,
                    "actual material-name lookup, diff flags and attached original material-animation array");
            BpkPlayer player(&h, m);
            player.start("Color");
            player.update();
            require(player.isPlaying("color") && !player.isStop() && player.mFrameCtrl.getFrame() == 1 && animation->mFrame == 0,
                    "original player advances its controller before reflecting the resource frame");
            player.beginDiff();
            auto* material = m->getMaterialNodePointer(0);
            material->getMaterialAnm()->calc(material);
            auto* rgba = material->getColorBlock()->getMatColor(0);
            require(animation->mFrame == 1 && rgba->r == 10 && rgba->g == 30 && rgba->b == 50 && rgba->a == 70,
                    "original player attachment and material calc apply the authored BPK channels");
            player.endDiff();
            rgba->r = 201; rgba->g = 202; rgba->b = 203; rgba->a = 204;
            material->getMaterialAnm()->calc(material);
            require(rgba->r == 201 && rgba->g == 202 && rgba->b == 203 && rgba->a == 204,
                    "original endDiff removes the material animator");
            player.stop();
            player.update();
            require(player.isStop() && player.mAnmRes == animation && player.mFrameCtrl.getFrame() == 1,
                    "original stop retains the selected resource while stopping its frame controller");
            auto* first_tex = m->getMaterialNodePointer(0)->getTexGenBlock()->getTexMtx(0);
            require(first_tex != nullptr, "real Mario material has an authored texture matrix");
            first_tex->getTexMtxInfo().mEffectMtx[0][0] = 7.0f;
            for (unsigned i = 0; i < m->getMaterialNum(); ++i) for (unsigned j = 0; j < 8; ++j) {
                for (unsigned row = 0; row < 4; ++row) for (unsigned col = 0; col < 4; ++col) {
                    require(h.getInitEffectMtx(i, j)[row][col] == initial_effect_matrices[i * 8 + j][row * 4 + col],
                            "original backup remains independent of live material animation and effect-matrix mutation");
                }
            }
        }
        require(process.mem1_heap()->available_bytes() == before, "actual model texture owner releases its mapped MEM1 allocations");
        std::cout << "resource cohort used=" << original->heap().mSize - original->heap().getTotalFreeSize()
                  << " MEM1 available=" << process.mem1_heap()->available_bytes() << '\n';
    }
}
int main() {
    std::cout << std::unitbuf;
    return smgpc::test::run_stage_resource_process("original-resource-holder", [] {
        auto& process = *GameResourceRuntime::active();
        const auto scene = MR::getSceneObjHolder()->nativeAllocationHeap();
        require(scene != nullptr, "the actual GameScene supplies the fixture allocation parent");
        std::unique_ptr<JKRExpHeap, void (*)(JKRExpHeap*)> test_heap(
            JKRExpHeap::create(4U << 20, &(*scene), false),
            +[](JKRExpHeap* heap) { heap->destroy(); });
        require(test_heap != nullptr, "the bounded holder fixture arena fits within the actual scene heap");
        const auto arena = (*test_heap).retainNativeLifetime();
        if (const auto* filter = std::getenv("SMGPC_RESOURCE_HOLDER_CASE")) {
            require(std::string_view(filter) == "archive-camera", "unknown resource-holder fixture case");
            test_duplicate_holders(process, arena);
            std::cout << "PASS mounted camera, original accessors, duplicate holders and archive retirement\n";
            return;
        }
        test_sdk_tex_map(process); std::cout << "PASS actual SDK TexMap descriptors, GX roundtrip and retained encoded storage\n";
        test_duplicate_holders(process, arena); std::cout << "PASS duplicate J3D/BAS owners, texture retirement and archive-owned CANM\n";
        test_native_bti(process, arena); std::cout << "PASS retained BTI native header, all archive identities, GX payload and JUT consumer\n";
        test_original_constructor(process, arena); std::cout << "PASS original holder, typed animation, control table and lifetime\n";
        test_original_csv_reader(process, arena); std::cout << "PASS original CSV helpers and fixed archive borrow preflight\n";
        test_failure_scope(process, arena); std::cout << "PASS original loader exception restoration\n";
        on_resource_worker([&] { test_original_texture_factory(process, arena); }); std::cout << "PASS original texture factory deduplication, caller heap and native copy lifetime\n";
        test_original_layout_holder(process, arena); std::cout << "PASS actual layout holder enumeration, duplicate creation and retirement\n";
        on_resource_worker([&] { test_original_manager_lifetime(process, arena); }); std::cout << "PASS actual manager, FileLoader and archive borrow preflight\n";
        on_resource_worker([&] { test_real_model_and_material(process, arena); }); std::cout << "PASS real model/material holder\n";
    });
}
