#include "JSystem/J3DGraphBase/J3DMaterial.hpp"
#include "JSystem/J3DGraphLoader/J3DMaterialFactory.hpp"
#include "JSystem/JSupport/JSupport.hpp"
#include "JSystem/JUtility/JUTNameTab.hpp"
#include "resource/J3dMaterialBlockData.hpp"
#include "runtime/RuntimeServices.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <algorithm>
#include <bit>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
    using Bytes = std::vector<std::uint8_t>;
    using View = std::span<const std::uint8_t>;
    using smgpc::resource::J3dMaterialBlockData;

    void require(bool value, std::string_view message) {
        if (!value) throw std::runtime_error(std::string(message));
    }
    void put16(Bytes& bytes, std::size_t at, std::uint16_t value) {
        bytes.at(at) = static_cast<u8>(value >> 8);
        bytes.at(at + 1) = static_cast<u8>(value);
    }
    void put32(Bytes& bytes, std::size_t at, std::uint32_t value) {
        put16(bytes, at, static_cast<u16>(value >> 16));
        put16(bytes, at + 2, static_cast<u16>(value));
    }
    void put_float(Bytes& bytes, std::size_t at, float value) { put32(bytes, at, std::bit_cast<u32>(value)); }
    u16 get16(View bytes, std::size_t at) { return u16(bytes[at]) << 8 | bytes[at + 1]; }
    u32 get32(View bytes, std::size_t at) { return u32(get16(bytes, at)) << 16 | get16(bytes, at + 2); }
    void tag(Bytes& bytes, std::string_view text) { std::copy(text.begin(), text.end(), bytes.begin()); }
    std::size_t column(Bytes& bytes, std::size_t field, View values) {
        bytes.resize((bytes.size() + 3) & ~std::size_t(3));
        const auto offset = bytes.size();
        put32(bytes, field, static_cast<u32>(offset));
        bytes.insert(bytes.end(), values.begin(), values.end());
        return offset;
    }
    View block(View bytes, std::string_view tag) {
        std::size_t offset = 0x20;
        for (u32 i = 0; i < get32(bytes, 0xC); ++i) {
            const auto size = get32(bytes, offset + 4);
            if (std::memcmp(bytes.data() + offset, tag.data(), 4) == 0) return bytes.subspan(offset, size);
            offset += size;
        }
        throw std::runtime_error("The model does not contain the requested material block");
    }

    // The SDK leaves allocation ownership to the resource heap. These standalone
    // factory fixtures own each allocation explicitly, including tex matrices.
    struct MaterialOwner {
        J3DMaterial* value;
        explicit MaterialOwner(J3DMaterial* material) : value(material) {}
        ~MaterialOwner() {
            for (u32 i = 0; i < 8; ++i) delete value->mTexGenBlock->getTexMtx(i);
            delete value->mColorBlock;
            delete value->mTexGenBlock;
            delete value->mTevBlock;
            delete value->mIndBlock;
            delete value->mPEBlock;
            delete value->mSharedDLObj;  // Borrowed command bytes belong to MDL3.
            if (auto* p = dynamic_cast<J3DPatchedMaterial*>(value)) delete p;
            else if (auto* p = dynamic_cast<J3DLockedMaterial*>(value)) delete p;
            else delete value;
        }
        MaterialOwner(const MaterialOwner&) = delete;
        MaterialOwner& operator=(const MaterialOwner&) = delete;
    };

    Bytes material_fixture() {
        Bytes bytes(0x84);
        tag(bytes, "MAT3");
        put16(bytes, 8, 3);
        Bytes rows(2 * 0x14C, 0xff);
        for (std::size_t i = 0; i < 2; ++i) {
            const auto row = i * 0x14C;
            rows[row] = i == 0 ? 1 : 4;
            rows[row + 3] = 0;  // One texture coordinate.
            rows[row + 4] = 0;  // One TEV stage.
            put16(rows, row + 8, static_cast<u16>(i));
            put16(rows, row + 0x28, 0);
            put16(rows, row + 0x48, 0);
            put16(rows, row + 0x84, static_cast<u16>(i));
            put16(rows, row + 0xDC, 0);
        }
        column(bytes, 0xC, rows);
        column(bytes, 0x10, Bytes{0, 1, 0, 0, 0, 1});
        column(bytes, 0x14, Bytes{0, 0, 0xab, 0xcd}); // Present, empty name table.
        // Source alignment leaves more than four bytes between names and Ind.
        bytes.resize(bytes.size() + 12, 0xc7);
        Bytes indirect(3 * 0x138);
        indirect[0] = 1;
        indirect[1] = 1;
        put_float(indirect, 0x14, 1.25F);
        put_float(indirect, 0x18, -2.5F);
        indirect[0x2C] = 0xf9;
        indirect[0x138] = 2; // Retail cmplwi ...,1 rejects this raw byte.
        indirect[0x139] = 0xff;
        column(bytes, 0x18, indirect);
        column(bytes, 0x20, Bytes{1, 2, 3, 4, 0xa1, 0xb2, 0xc3, 0xd4});
        column(bytes, 0x34, Bytes{1});
        column(bytes, 0x38, Bytes{GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0, 0xa5});
        Bytes matrix(0x64);
        matrix[0] = 1;
        matrix[1] = 0;
        put_float(matrix, 4, 0.5F);
        put_float(matrix, 8, 0.25F);
        put_float(matrix, 0x10, 2.0F);
        put_float(matrix, 0x14, 3.0F);
        put16(matrix, 0x18, 0x9234);
        matrix[0x1A] = 0xc8;
        matrix[0x1B] = 0xd9;
        put_float(matrix, 0x1C, -0.75F);
        put_float(matrix, 0x20, 0.125F);
        for (std::size_t i = 0; i < 16; ++i) put_float(matrix, 0x24 + 4 * i, static_cast<float>(i) + 0.5F);
        column(bytes, 0x40, matrix);
        column(bytes, 0x48, Bytes{0, 0x17, 1, 0x23});
        column(bytes, 0x50, Bytes{0xff, 0x80, 0, 0x7f, 3, 0xff, 0xfc, 0x01});
        column(bytes, 0x58, Bytes{1});
        put32(bytes, 4, static_cast<u32>(bytes.size()));
        return bytes;
    }

    Bytes display_list_fixture() {
        Bytes bytes(0xC0);
        tag(bytes, "MDL3");
        put32(bytes, 4, static_cast<u32>(bytes.size()));
        put16(bytes, 8, 2);
        put32(bytes, 0xC, 0x24);
        put32(bytes, 0x10, 0x34);
        put32(bytes, 0x14, 0x54);
        put32(bytes, 0x18, 0x64);
        put32(bytes, 0x20, 0x68);
        for (std::size_t i = 0; i < 2; ++i) {
            // Both materials reference the same immutable source command image.
            put32(bytes, 0x24 + i * 8, static_cast<u32>(0x80 - (0x24 + i * 8)));
            put32(bytes, 0x28 + i * 8, 0x40);
            for (std::size_t j = 0; j < 6; ++j) put16(bytes, 0x34 + i * 16 + j * 2, static_cast<u16>(j * 5));
            bytes[0x40 + i * 16] = 0xaa;
            put32(bytes, 0x54 + i * 8, 0x12345678 + static_cast<u32>(i));
            put32(bytes, 0x58 + i * 8, 0x87654321 - static_cast<u32>(i));
            bytes[0x64 + i] = static_cast<u8>(1 + i * 3);
        }
        put16(bytes, 0x68, 0);
        put16(bytes, 0x6A, 0x2468);
        for (std::size_t i = 0; i < 0x40; ++i) bytes[0x80 + i] = static_cast<u8>(i * 7);
        return bytes;
    }

    void test_materials() {
        alignas(J3DMaterial) std::byte storage[sizeof(J3DMaterial)];
        std::fill_n(storage, sizeof(storage), std::byte{0xc7});
        auto* initialized = ::new (storage) J3DMaterial;
        require(initialized->mMaterialMode == 1 && initialized->mIndex == 0xffff && initialized->mDiffFlag == 0 &&
                    initialized->mNext == nullptr && initialized->mShape == nullptr && initialized->mJoint == nullptr &&
                    initialized->mColorBlock == nullptr && initialized->mTexGenBlock == nullptr && initialized->mTevBlock == nullptr &&
                    initialized->mIndBlock == nullptr && initialized->mPEBlock == nullptr && initialized->mpOrigMaterial == nullptr &&
                    initialized->mMaterialAnm == nullptr && initialized->mSharedDLObj == nullptr,
                "original material constructor initializes recycled allocation storage");
        std::destroy_at(initialized);
        auto bytes = material_fixture();
        J3dMaterialBlockData data(bytes);
        J3DMaterialFactory factory(data.material());
        require(factory.mMaterialNum == 3 && factory.countUniqueMaterials() == 1 && factory.getMaterialID(1) == 0,
                "original material count/remap and maximum-increasing unique-count behavior");
        require(factory.mpIndInitData != nullptr && factory.newIndTexStageNum(0) == 1 && factory.newIndTexStageNum(1) == 0,
                "empty names preserve the original indirect offset branch and byte==1 semantics");
        require(factory.mpIndInitData[0].mIndTexMtxInfo[0].field_0x18 == 0xf9,
                "indirect exponent retains its authored byte");
        auto* names = JSUConvertOffsetToPtr<ResNTAB>(&data.material(), data.material().mpNameTable);
        require(names->mEntryNum == 0 && names->_2 == 0xabcd, "present zero-count name metadata retains opaque header word");
        const auto& matrix = factory.mpTexMtxInfo[0];
        require(std::bit_cast<u32>(matrix.mSRT.mScaleX) == std::bit_cast<u32>(2.0F) &&
                    matrix.mSRT.mRotation == static_cast<s16>(0x9234), "texture SRT scalar endian conversion");
        std::fill(bytes.begin(), bytes.end(), 0xcd);
        bytes.clear();
        bytes.shrink_to_fit();
        for (auto type : {J3DMaterialFactory::MATERIAL_TYPE_NORMAL, J3DMaterialFactory::MATERIAL_TYPE_PATCHED}) {
            for (int i = 0; i < factory.mMaterialNum; ++i) {
                MaterialOwner material(factory.create(nullptr, type, i, 0x51100000));
                auto* value = material.value;
                const bool second = i != 1;
                require(value->mIndex == i && value->mMaterialMode == (second ? 4U : 1U) &&
                            value->getTexGenNum() == 1 && value->getTevStageNum() == 1 && value->getTexNo(0) == (second ? 0x123 : 0x17),
                        "actual normal and patched factories populate remapped metadata");
                require(value->getColorBlock()->getMatColor(0)->r == (second ? 0xa1 : 1) &&
                            value->getColorBlock()->getMatColor(0)->a == (second ? 0xd4 : 4), "RGBA bytes remain authored order");
                require(value->getTevBlock()->getTevColor(0)->r == -128 && value->getTevBlock()->getTevColor(0)->a == -1023,
                        "signed TEV color fields are decoded independently");
                require(value->getTexMtx(0) != nullptr && value->getTexMtx(1) == nullptr,
                        "actual factory creates texture matrices and honors ffff sentinels");
                require(factory.newMatColor(i, 1).r == 255 && factory.newTexNo(i, 7) == 0xffff,
                        "missing tables retain original per-field defaults");
            }
        }
    }

    void test_display_lists() {
        auto bytes = display_list_fixture();
        J3dMaterialBlockData data(bytes);
        J3DMaterialFactory factory(data.display_list());
        auto* lists = factory.mpDisplayListInit;
        auto* first = reinterpret_cast<u8*>(&lists[0]) + lists[0].mOffset;
        auto* second = reinterpret_cast<u8*>(&lists[1]) + lists[1].mOffset;
        require(first == second && (reinterpret_cast<std::uintptr_t>(first) & 31) == 0 &&
                    std::memcmp(first, bytes.data() + 0x80, 0x40) == 0,
                "MDL3 retains aliased, aligned, byte-identical commands with native relative descriptors");
        std::fill(bytes.begin(), bytes.end(), 0xe7);
        for (int i = 0; i < 2; ++i) {
            MaterialOwner material(factory.create(nullptr, J3DMaterialFactory::MATERIAL_TYPE_LOCKED, i, 0));
            require(dynamic_cast<J3DLockedMaterial*>(material.value) != nullptr && material.value->mIndex == i &&
                        material.value->mMaterialMode == u32(1 + i * 3), "actual locked material factory constructs its original class");
            require(material.value->getSharedDisplayListObj()->getDisplayList(0) == first &&
                        material.value->getSharedDisplayListObj()->getDisplayListSize() == 0x40,
                    "original display-list object borrows the retained source image");
            require(factory.mpPatchingInfo[i].mFogOffset == 25 && factory.mpPatchingInfo[i].field_0xc[0] == 0xaa,
                    "MDL3 patch offsets are native and opaque bytes are unchanged");
        }
    }

    void test_bad_ranges() {
        auto reject = [](Bytes bytes) {
            try { J3dMaterialBlockData data(bytes); }
            catch (const std::runtime_error&) { return; }
            throw std::runtime_error("invalid material resource unexpectedly accepted");
        };
        auto bytes = material_fixture();
        put16(bytes, get32(bytes, 0x10), 0xfffe);
        reject(bytes);
        bytes = material_fixture();
        put16(bytes, get32(bytes, 0xC) + 8, 100);
        reject(bytes);
        bytes = material_fixture();
        bytes[get32(bytes, 0x34)] = 9;
        reject(bytes);
        bytes = material_fixture();
        put32(bytes, 0x40, 0);
        reject(bytes);
        bytes = display_list_fixture();
        put32(bytes, 0x28, 0xffff);
        reject(bytes);
        bytes = display_list_fixture();
        put32(bytes, 0x14, 0);
        reject(bytes);
    }

    void test_optional_disc() {
        const auto* path = std::getenv("SMGPC_REAL_DISC");
        if (!path || !path[0]) {
            std::cout << "[skip] Mario MAT3/MDL3 (set SMGPC_REAL_DISC)\n";
            return;
        }
        aurora_dvd_close();
        require(aurora_dvd_open(path), "open original disc");
        struct Disc { ~Disc() { aurora_dvd_close(); } } disc;
        DVDInit();
        std::unique_ptr<J3dMaterialBlockData> mat, mdl;
        {
            smgpc::runtime::DvdFileSystemService dvd{"/"};
            const auto archive_path = dvd.find_object_archive("Mario");
            require(archive_path.has_value(), "find original Mario archive");
            const auto& archive = dvd.archive_for_path(*archive_path);
            const auto* entry = archive.find_by_basename("mario.bdl");
            require(entry != nullptr, "find original mario.bdl");
            const auto bytes = archive.file_data(*entry);
            mat = std::make_unique<J3dMaterialBlockData>(block(bytes, "MAT3"));
            mdl = std::make_unique<J3dMaterialBlockData>(block(bytes, "MDL3"));
        }
        J3DMaterialFactory factory(mat->material()), commands(mdl->display_list());
        require(factory.mMaterialNum > 0 && factory.mMaterialNum == commands.mMaterialNum, "actual MAT3 and MDL3 agree");
        for (int i = 0; i < factory.mMaterialNum; ++i) {
            for (auto type : {J3DMaterialFactory::MATERIAL_TYPE_NORMAL, J3DMaterialFactory::MATERIAL_TYPE_PATCHED}) {
                MaterialOwner material(factory.create(nullptr, type, i, 0x51100000));
                require(material.value->getTexGenNum() == factory.countTexGens(i) &&
                            material.value->mMaterialMode == factory.getMaterialMode(i), "original factory consumes actual retained Mario material");
                commands.create(material.value, J3DMaterialFactory::MATERIAL_TYPE_LOCKED, i, 0);
                require(material.value->getSharedDisplayListObj()->getDisplayListSize() == commands.mpDisplayListInit[i].field_0x4,
                        "MDL3 patches the actual existing material without replacing its blocks");
            }
            MaterialOwner locked(commands.create(nullptr, J3DMaterialFactory::MATERIAL_TYPE_LOCKED, i, 0));
        }
        std::cout << "[resource] mario.bdl: " << factory.mMaterialNum << " materials, all normal/patched/locked factories\n";
    }
}

int main() {
    try {
        test_materials();
        test_display_lists();
        test_bad_ranges();
        test_optional_disc();
        std::cout << "[pass] 4 original J3D material-resource groups\n";
    } catch (const std::exception& error) {
        std::cerr << "[fail] " << error.what() << '\n';
        return 1;
    }
}
