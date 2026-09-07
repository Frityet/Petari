#include "resource/J3dMaterialTableData.hpp"
#include "compat/JkrAllocationDomain.hpp"
#include "JSystem/J3DGraphAnimator/J3DMaterialAttach.hpp"
#include "JSystem/J3DGraphBase/J3DMaterial.hpp"
#include "JSystem/J3DGraphBase/J3DTexture.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include "JSystem/JUtility/JUTNameTab.hpp"

#include <algorithm>
#include <bit>
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
    using smgpc::resource::J3dMaterialTableData;
    using Mode = J3dMaterialTableData::Mode;
    using namespace smgpc::compat;
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


    Bytes file(std::string_view type, std::initializer_list<Bytes> blocks) {
        Bytes bytes(0x20);
        tag(bytes, "J3D2");
        std::copy(type.begin(), type.end(), bytes.begin() + 4);
        put32(bytes, 0xC, static_cast<u32>(blocks.size()));
        for (const auto& block : blocks) bytes.insert(bytes.end(), block.begin(), block.end());
        put32(bytes, 8, static_cast<u32>(bytes.size()));
        return bytes;
    }
    auto domain(const std::shared_ptr<JkrHeapRuntime>& runtime) {
        return JkrAllocationDomain::create(runtime, 1 << 18);
    }
    Bytes two_materials() {
        auto bytes = material_fixture();
        put16(bytes, 8, 2);
        return bytes;
    }
    template<class F> void rejects(F&& operation, std::string_view reason) {
        try { operation(); }
        catch (const std::exception&) { return; }
        throw std::runtime_error(std::string(reason));
    }

    void test_model_and_attach(const std::shared_ptr<JkrHeapRuntime>& runtime) {
        alignas(4) std::byte source_bytes[8]{};
        alignas(4) std::byte destination_bytes[8]{};
        auto* source_order = std::construct_at(reinterpret_cast<J3DTevOrderInfo*>(source_bytes + 1));
        auto* destination_order = std::construct_at(reinterpret_cast<J3DTevOrderInfo*>(destination_bytes + 1));
        source_order->mTexCoord = 2; source_order->mTexMap = 5;
        source_order->mColorChan = 7; source_order->field_0x3 = 0xA9;
        *destination_order = *source_order;
        require(std::memcmp(source_order, destination_order, 4) == 0 && destination_order->field_0x3 == 0xA9,
                "original four-byte TEV-order assignment preserves padding on genuinely byte-aligned native values");
        std::destroy_at(source_order);
        std::destroy_at(destination_order);
        auto heap = domain(runtime);
        auto bytes = file("bmd3", {material_fixture()});
        J3dMaterialTableData data(bytes, 0x51100000, Mode::Model, heap);
        J3DMaterialTable table;
        J3DTexture texture(0, nullptr);
        JUTNameTab texture_names;
        table.mTexture = &texture;
        table.mTextureName = &texture_names;
        data.attach_to(table);
        require(table.mTexture == &texture && table.mTextureName == &texture_names,
                "material attachment preserves independently owned TEX fields");
        require(table.mMaterialNum == 3 && table.mUniqueMatNum == 1 && table.field_0x10 == nullptr && !table.isLocked(),
                "normal model retains original maximum-increasing unique count without creating the optional array");
        require(table.mMaterialName && table.mMaterialName->mNameNum == 0,
                "present empty name resource still constructs an actual name owner");
        require(JKRHeap::findFromRoot(table.mMaterialNodePointer) == &heap->heap() &&
                    JKRHeap::findFromRoot(table.mMaterialName) == &heap->heap() &&
                    !JKRHeap::findFromRoot(const_cast<ResNTAB*>(table.mMaterialName->mResource)),
                "actual SDK pointer array and name object use the domain while decoded resource backing uses host storage");
        auto* first = table.mMaterialNodePointer[0];
        auto* second = table.mMaterialNodePointer[1];
        require(first != table.mMaterialNodePointer[2] && first->mDiffFlag == table.mMaterialNodePointer[2]->mDiffFlag &&
                    first->mDiffFlag == second->mDiffFlag + 1 && (first->mDiffFlag & 0xC0000000U) == 0,
                "separate material instances share original shifted remap identities");
        require(first->mpOrigMaterial == nullptr && first->mMaterialMode == 4 && second->mMaterialMode == 1 &&
                    JKRHeap::findFromRoot(first->mTexGenBlock->getTexMtx(0)) == &heap->heap(),
                "original factories retain complete typed values and subordinate domain allocations");
        std::fill(bytes.begin(), bytes.end(), 0xEC);
        heap.reset();
        require(first->getTexNo(0) == 0x123 && first->getColorBlock()->getMatColor(0)->r == 0xA1 &&
                    data.tex_no_patch_offset(0) == 0,
                "source bytes and caller heap references can leave while the material component remains alive");
        rejects([&] { data.attach_to(table); }, "second attachment must fail");
        rejects([&] { (void)data.tex_no_patch_offset(3); }, "patch metadata must enforce the attached count");
    }

    void test_unique(const std::shared_ptr<JkrHeapRuntime>& runtime) {
        auto bytes = material_fixture();
        const auto remap = get32(bytes, 0x10);
        put16(bytes, remap, 0); put16(bytes, remap + 2, 1); put16(bytes, remap + 4, 0);
        J3dMaterialTableData data(file("bmd3", {bytes}), 0x51300000, Mode::Model, domain(runtime));
        J3DMaterialTable table;
        data.attach_to(table);
        require(table.mUniqueMatNum == 2 && table.field_0x10 && std::uintptr_t(table.field_0x10) % 32 == 0,
                "unique model constructs the original aligned array");
        require(table.mMaterialNodePointer[0]->mpOrigMaterial == &table.field_0x10[0] &&
                    table.mMaterialNodePointer[1]->mpOrigMaterial == &table.field_0x10[1] &&
                    table.mMaterialNodePointer[2]->mpOrigMaterial == &table.field_0x10[0],
                "logical remap retains actual shared original material pointers");
        require(table.field_0x10[1].mDiffFlag - table.field_0x10[0].mDiffFlag == 4 &&
                    table.mMaterialNodePointer[1]->mDiffFlag == table.field_0x10[1].mDiffFlag,
                "unique identity uses original 0x4C stride before the original right shift, independent of native class size");
    }

    void test_material_table_flags(const std::shared_ptr<JkrHeapRuntime>& runtime) {
        J3dMaterialTableData data(file("bmt3", {material_fixture()}), 0xFFFFFFFF, Mode::MaterialTable, domain(runtime));
        J3DMaterialTable table;
        data.attach_to(table);
        require(table.mMaterialNum == 3 && table.mUniqueMatNum == 0 && !table.field_0x10 && !table.isLocked(),
                "v26 material-table path preserves clear unique fields and ignores caller flags");
        auto* first = table.mMaterialNodePointer[0];
        require((first->mDiffFlag & 0xC0000000U) == 0x80000000U &&
                    first->mDiffFlag == table.mMaterialNodePointer[1]->mDiffFlag + 1,
                "BMT keeps original unshifted cached-address material identity bits");
        require(dynamic_cast<J3DIndBlockFull*>(first->mIndBlock) && first->mIndBlock->getIndTexStageNum() == 1,
                "fixed original BMT 0x51100000 enables authored indirect state despite arbitrary caller bits");
    }

    void test_binary_selection(const std::shared_ptr<JkrHeapRuntime>& runtime) {
        for (u32 flags : {0U, 0x1000U, 0x2000U, 0x3000U}) {
            J3dMaterialTableData data(file("bdl3", {two_materials(), display_list_fixture()}), flags,
                                     Mode::BinaryModel, domain(runtime));
            J3DMaterialTable table;
            data.attach_to(table);
            auto* material = table.mMaterialNodePointer[0];
            const bool locked = flags == 0x1000 || flags == 0x3000;
            require(table.mMaterialNum == 2 && table.isLocked() == locked &&
                        (dynamic_cast<J3DLockedMaterial*>(material) != nullptr) == locked &&
                        (dynamic_cast<J3DPatchedMaterial*>(material) != nullptr) == (flags == 0x2000),
                    "all four binary selection branches construct the original material class without replacement");
            require(material->mMaterialMode == (locked ? 1U : 4U) &&
                        (material->mDiffFlag == 0xC0000000U) == locked,
                    "MDL patching preserves existing MAT mode/identity and initializes locked-only values");
            require(material->mSharedDLObj && material->mSharedDLObj->getDisplayListSize() == 0x40 &&
                        data.tex_no_patch_offset(0) == 15,
                    "actual original shared display list and patch offset are retained");
            if (locked) require(material->mTevBlock->getTexNoOffset() == 0,
                                "Null TEV virtual getter remains original even when its setter stored nonzero patch metadata");
            if ((flags & 0x2000U) == 0) require(material->mCurrentMtx.mMtxIdxRegA == 0x12345678,
                                             "unmodified binary route uses authored MDL current matrix");
            else require(material->mCurrentMtx.mMtxIdxRegA != 0x12345678,
                         "patched binary route executes original MAT coordinate current-matrix reconstruction");
        }
    }

    void test_order_and_aliases(const std::shared_ptr<JkrHeapRuntime>& runtime) {
        auto first = display_list_fixture();
        auto second = first;
        second[0x80] = 0xE3;
        put16(second, 0x34 + 6, 23);
        put32(second, 0x54, 0xABCDEF01);
        J3dMaterialTableData data(file("bdl4", {first, second}), 0, Mode::BinaryModel, domain(runtime));
        J3DMaterialTable table;
        data.attach_to(table);
        auto* a = table.mMaterialNodePointer[0];
        auto* b = table.mMaterialNodePointer[1];
        const auto* commands = static_cast<const u8*>(a->mSharedDLObj->getDisplayList(0));
        require(commands == b->mSharedDLObj->getDisplayList(0) && std::uintptr_t(commands) % 32 == 0 && commands[0] == 0,
                "repeated MDL keeps the first existing display-list allocation and authored cross-material aliases");
        require(a->mCurrentMtx.mMtxIdxRegA == 0xABCDEF01 && data.tex_no_patch_offset(0) == 23,
                "later MDL updates current matrix and patch setters in authored order");
        J3dMaterialTableData later_mat(file("bdl4", {first, two_materials()}), 0, Mode::BinaryModel, domain(runtime));
        J3DMaterialTable replaced;
        later_mat.attach_to(replaced);
        require(replaced.isLocked() && !dynamic_cast<J3DLockedMaterial*>(replaced.mMaterialNodePointer[0]) &&
                    !replaced.mMaterialNodePointer[0]->mSharedDLObj && later_mat.tex_no_patch_offset(0) == 0,
                "later MAT replaces material fields but preserves the original previously set table lock flag");
    }

    void test_invalid_and_empty(const std::shared_ptr<JkrHeapRuntime>& runtime) {
        auto reject_file = [&](Bytes bytes, u32 flags, Mode mode) {
            auto heap = domain(runtime);
            rejects([&] { J3dMaterialTableData data(bytes, flags, mode, heap); }, "invalid material input unexpectedly accepted");
        };
        reject_file(file("bmd3", {material_fixture()}), 0x200000, Mode::Model);
        reject_file(file("bdl4", {material_fixture(), display_list_fixture()}), 0, Mode::BinaryModel);
        reject_file(file("bdl4", {display_list_fixture()}), 0x2000, Mode::BinaryModel);
        auto truncated = file("bmt3", {material_fixture()});
        truncated.pop_back();
        reject_file(truncated, 0, Mode::MaterialTable);
        Bytes mat2(8); tag(mat2, "MAT2"); put32(mat2, 4, 8);
        reject_file(file("bmd3", {mat2}), 0, Mode::Model);
        J3dMaterialTableData empty(file("bmt3", {}), 0, Mode::MaterialTable, domain(runtime));
        J3DMaterialTable table;
        empty.attach_to(table);
        require(table.mMaterialNum == 0 && !table.mMaterialNodePointer && !table.mTexture,
                "empty BMT retains genuine clear material fields and leaves texture fallback to the complete owner");
        J3dMaterialTableData ignored(file("bdl3", {mat2}), 0, Mode::BinaryModel, domain(runtime));
        J3DMaterialTable ignored_table;
        ignored.attach_to(ignored_table);
        require(ignored_table.mMaterialNum == 0, "binary v26 original switch ignores MAT2");
        rejects([&] { J3dMaterialTableData absent(file("bmt3", {}), 0, Mode::MaterialTable, nullptr); },
                "missing retained SDK heap is not accepted");
        auto failed_heap = domain(runtime);
        std::weak_ptr<JkrAllocationDomain> weak_failed = failed_heap;
        try {
            J3dMaterialTableData invalid(file("bmd3", {material_fixture()}), 0x200000, Mode::Model, std::move(failed_heap));
            throw std::logic_error("invalid unique remap unexpectedly accepted");
        } catch (const std::runtime_error& error) {
            require(weak_failed.expired() && std::string_view(error.what()).find("unique material remap") != std::string_view::npos,
                    "validation exception remains valid after releasing its final original domain reference");
        }
        auto exhausted = JkrAllocationDomain::create(runtime, 512);
        std::weak_ptr<JkrAllocationDomain> weak_exhausted = exhausted;
        try {
            J3dMaterialTableData incomplete(file("bmd3", {material_fixture()}), 0x51100000, Mode::Model, std::move(exhausted));
            throw std::runtime_error("tiny material domain unexpectedly fit the complete original factory");
        } catch (const std::bad_alloc&) {
            require(weak_exhausted.expired(), "partial original factory allocations retire with the failed domain");
        }
    }
}

int main() {
    try {
        auto runtime = JkrHeapRuntime::create(4 << 20);
        const auto initial = runtime->root_heap().getTotalFreeSize();
        for (auto test : {test_model_and_attach, test_unique, test_material_table_flags, test_binary_selection,
                          test_order_and_aliases, test_invalid_and_empty}) {
            test(runtime);
            require(runtime->root_heap().getTotalFreeSize() == initial && runtime->root_heap().check(),
                    "each successful and failed material owner returns its complete domain to the actual root");
        }
        std::cout << "[pass] 6 original J3D material-table groups\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[fail] " << error.what() << '\n';
        return 1;
    }
}
