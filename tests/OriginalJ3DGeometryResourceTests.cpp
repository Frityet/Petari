#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"
#include "JSystem/J3DGraphBase/J3DShape.hpp"
#include "JSystem/J3DGraphBase/J3DShapeDraw.hpp"
#include "JSystem/J3DGraphBase/J3DShapeMtx.hpp"
#include "JSystem/J3DGraphLoader/J3DShapeFactory.hpp"
#include "JSystem/JSupport/JSupport.hpp"
#include "JSystem/JUtility/JUTNameTab.hpp"
#include "resource/J3dGeometryData.hpp"
#include "runtime/RuntimeServices.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
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
    using smgpc::resource::J3dGeometryData;

    void require(bool condition, std::string_view message) {
        if (!condition) throw std::runtime_error(std::string(message));
    }

    void write16(Bytes& bytes, std::size_t offset, std::uint16_t value) {
        bytes.at(offset) = static_cast<std::uint8_t>(value >> 8U);
        bytes.at(offset + 1) = static_cast<std::uint8_t>(value);
    }
    void write32(Bytes& bytes, std::size_t offset, std::uint32_t value) {
        write16(bytes, offset, static_cast<std::uint16_t>(value >> 16U));
        write16(bytes, offset + 2, static_cast<std::uint16_t>(value));
    }
    void write_float(Bytes& bytes, std::size_t offset, float value) {
        write32(bytes, offset, std::bit_cast<std::uint32_t>(value));
    }
    std::uint16_t read16(View bytes, std::size_t offset) {
        return static_cast<std::uint16_t>((std::uint16_t{bytes[offset]} << 8U) | bytes[offset + 1]);
    }
    std::uint32_t read32(View bytes, std::size_t offset) {
        return (std::uint32_t{read16(bytes, offset)} << 16U) | read16(bytes, offset + 2);
    }
    void tag(Bytes& bytes, std::size_t offset, std::string_view value) {
        std::copy(value.begin(), value.end(), bytes.begin() + offset);
    }
    std::size_t block_offset(View bytes, std::string_view name) {
        std::size_t offset = 0x20;
        for (std::size_t i = 0; i < read32(bytes, 0xc); ++i) {
            if (std::memcmp(bytes.data() + offset, name.data(), 4) == 0) return offset;
            offset += read32(bytes, offset + 4);
        }
        throw std::runtime_error("fixture block is absent");
    }
    View block(View bytes, std::string_view name) {
        const auto offset = block_offset(bytes, name);
        return bytes.subspan(offset, read32(bytes, offset + 4));
    }
    void same_float(float actual, View bytes, std::size_t offset) {
        require(std::bit_cast<std::uint32_t>(actual) == read32(bytes, offset), "shape float must preserve its authored bits");
    }

    Bytes fixture() {
        Bytes info(0x40);
        tag(info, 0, "INF1");
        write16(info, 8, 0x10); // This flag is not passed to readShape's factory.
        write32(info, 0xc, 4);
        write32(info, 0x10, 2);
        write32(info, 0x14, 0x18);
        constexpr std::array<std::array<u16, 2>, 7> commands{{
            {0x10, 0}, {0x11, 0}, {0x12, 2}, {0x12, 0}, {0x12, 3}, {0x12, 1}, {0, 0}}};
        for (std::size_t i = 0; i < commands.size(); ++i) {
            write16(info, 0x18 + i * 4, commands[i][0]);
            write16(info, 0x1a + i * 4, commands[i][1]);
        }
        Bytes vertex(0x120);
        tag(vertex, 0, "VTX1");
        write32(vertex, 8, 0x40);
        write32(vertex, 0xc, 0xa0);
        write32(vertex, 0x10, 0xb0);
        write32(vertex, 0x14, 0xc0);
        write32(vertex, 0x18, 0xe0);
        write32(vertex, 0x20, 0xf0);
        constexpr std::array<GXVtxAttrFmtList, 5> formats{{
            {GX_VA_POS, GX_POS_XYZ, GX_S16, 8}, {GX_VA_NRM, GX_NRM_XYZ, GX_S16, 14},
            {GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0}, {GX_VA_TEX0, GX_TEX_ST, GX_S16, 14},
            {GX_VA_NULL, GX_POS_XY, GX_U8, 0}}};
        for (std::size_t i = 0; i < formats.size(); ++i) {
            write32(vertex, 0x40 + i * 16, formats[i].attr);
            write32(vertex, 0x44 + i * 16, formats[i].cnt);
            write32(vertex, 0x48 + i * 16, formats[i].type);
            vertex[0x4c + i * 16] = formats[i].frac;
            vertex[0x4d + i * 16] = 0xa5;
        }
        for (std::size_t i = 0xa0; i < vertex.size(); i += 2) write16(vertex, i, static_cast<u16>(0x1200 + i));
        write16(vertex, 0xa2, 0xff00);
        write16(vertex, 0xc0, 0x3344);
        for (std::size_t i = 0; i < 16; ++i) vertex[0xe0 + i] = static_cast<u8>(0x80 + i);

        Bytes shape(0x240);
        tag(shape, 0, "SHP1");
        write16(shape, 8, 4);
        write32(shape, 0xc, 0x2c);
        write32(shape, 0x10, 0xcc);
        write32(shape, 0x14, 0xd4);
        write32(shape, 0x18, 0x100);
        write32(shape, 0x1c, 0x160);
        write32(shape, 0x20, 0x180);
        write32(shape, 0x24, 0x200);
        write32(shape, 0x28, 0x220);
        for (std::size_t i = 0; i < 4; ++i) {
            write16(shape, 0xcc + i * 2, static_cast<u16>(3 - i));
            const auto offset = 0x2c + i * 40;
            shape[offset] = static_cast<u8>(i);
            shape[offset + 1] = 0x7a;
            write16(shape, offset + 2, 1);
            write16(shape, offset + 4, i == 3 ? 48 : 0);
            write16(shape, offset + 6, static_cast<u16>(i));
            write16(shape, offset + 8, static_cast<u16>(i));
            write16(shape, offset + 10, 0xbabe);
            write_float(shape, offset + 12, 10.5F + static_cast<float>(i));
            for (std::size_t axis = 0; axis < 3; ++axis) {
                write_float(shape, offset + 16 + axis * 4, -static_cast<float>(i + axis + 1));
                write_float(shape, offset + 28 + axis * 4, static_cast<float>(i + axis + 2));
            }
            write16(shape, 0x200 + i * 8, static_cast<u16>(5 + i));
            write16(shape, 0x202 + i * 8, i == 3 ? 3 : 0xffff);
            write32(shape, 0x204 + i * 8, i == 3 ? 1 : 0xffffffff);
            write32(shape, 0x220 + i * 8, 32);
            write32(shape, 0x224 + i * 8, static_cast<u32>(i * 32));
            const auto dl = 0x180 + i * 32;
            shape[dl] = GX_TRIANGLEFAN;
            write16(shape, dl + 1, 2);
            for (std::size_t j = 0; j < 14; ++j) shape[dl + 3 + j] = static_cast<u8>(i * 17 + j);
        }
        write16(shape, 0xd4, 2);
        write16(shape, 0xd6, 0xbeef);
        write16(shape, 0xd8, 0x1);
        write16(shape, 0xda, 12);
        write16(shape, 0xdc, 0x2);
        write16(shape, 0xde, 16);
        tag(shape, 0xe0, "One");
        tag(shape, 0xe4, "Two");
        for (std::size_t layout = 0; layout < 2; ++layout) {
            const std::array<GXVtxDescList, 6> descriptors{{
                {GX_VA_PNMTXIDX, GX_DIRECT}, {GX_VA_POS, GX_INDEX16},
                {layout == 0 ? GX_VA_NRM : GX_VA_NBT, GX_INDEX8}, {GX_VA_CLR0, GX_INDEX8},
                {GX_VA_TEX0, GX_INDEX16}, {GX_VA_NULL, GX_NONE}}};
            for (std::size_t i = 0; i < descriptors.size(); ++i) {
                write32(shape, 0x100 + layout * 48 + i * 8, descriptors[i].attr);
                write32(shape, 0x104 + layout * 48 + i * 8, descriptors[i].type);
            }
        }
        write16(shape, 0x160, 9);
        write16(shape, 0x162, 2);
        write16(shape, 0x164, 0xffff);
        write16(shape, 0x166, 7);
        Bytes result(0x20);
        tag(result, 0, "J3D2");
        tag(result, 4, "bmd3");
        write32(result, 0xc, 3);
        for (auto* section : {&info, &vertex, &shape}) {
            write32(*section, 4, static_cast<u32>(section->size()));
            result.insert(result.end(), section->begin(), section->end());
        }
        write32(result, 8, static_cast<u32>(result.size()));
        return result;
    }

    void test_original_factory_and_lifetime() {
        for (const auto flags : {0U, 0x10U}) {
            auto bytes = fixture();
            J3dGeometryData owner(bytes, flags);
            J3DModelData model;
            owner.attach_to(model);
            require(model.getShapeNum() == 4 && model.getMaterialNum() == 0 && model.getJointNum() == 0 && model.mpRawData == nullptr,
                    "the construction component does not publish invented model tables or raw data");
            J3DShapeFactory factory(owner.shape_block());
            constexpr std::array<u32, 4> plain{0x534d4d4c, 0x534d5458, 0x534d5458, 0x534d5458};
            constexpr std::array<u32, 4> concat{0x534d4d43, 0x534d5942, 0x534d4242, 0x534d4356};
            const auto source_shape = block(bytes, "SHP1");
            auto* logical0 = model.getShapeNodePointer(0);
            for (u16 i = 0; i < 4; ++i) {
                auto* actual = model.getShapeNodePointer(i);
                require(actual->getIndex() == i && actual->getMtxGroupNum() == 1 &&
                            actual->getShapeMtx(0)->getType() == (flags ? concat : plain)[i],
                        "all four shape types use actual original matrix classes and caller flags, independent of INF flags");
                require(actual->mMaterial == nullptr && actual->mVertexData == nullptr && actual->mDrawMtxData == nullptr,
                        "original readShape leaves links and VCD generation for model finalization");
                const auto init = 0x2c + (3 - i) * 40;
                same_float(actual->mRadius, source_shape, init + 12);
                same_float(actual->mMin.z, source_shape, init + 24);
                same_float(actual->mMax.y, source_shape, init + 32);
                require(std::memcmp(actual->getShapeDraw(0)->getDisplayList(), source_shape.data() + 0x180 + (3 - i) * 32, 32) == 0,
                        "original draws retain their complete big-endian byte stream and remapped record");
                require(actual->getShapeDraw(0)->countVertex(7) == 2,
                        "actual original draw reader sees the retained big-endian primitive count");
                const auto expected_matrix_size = i == 0 ? (flags ? sizeof(J3DShapeMtxMultiConcatView) : sizeof(J3DShapeMtxMulti)) :
                    flags ? (i == 1 ? sizeof(J3DShapeMtxYBBoardConcatView) : i == 2 ? sizeof(J3DShapeMtxBBoardConcatView) : sizeof(J3DShapeMtxConcatView)) : sizeof(J3DShapeMtx);
                require(factory.calcSize(i, flags) == sizeof(J3DShape) + sizeof(J3DShapeMtx*) + sizeof(J3DShapeDraw*) +
                            expected_matrix_size + sizeof(J3DShapeDraw),
                        "source-backed size queries use actual native pointer and class sizes");
            }
            require(logical0->getShapeMtx(0)->getUseMtxNum() == 3 && logical0->getShapeMtx(0)->getUseMtxIndex(0) == 2 &&
                        logical0->getShapeMtx(0)->getUseMtxIndex(1) == 0xffff && logical0->getShapeMtx(0)->getUseMtxIndex(2) == 7,
                    "the matrix table retains original carry slots and ordered indices");
            require(model.getShapeNodePointer(1)->getVtxDesc() == model.getShapeNodePointer(3)->getVtxDesc() &&
                        logical0->getVtxDesc() != model.getShapeNodePointer(1)->getVtxDesc(),
                    "byte-offset descriptor aliases remain aliases through the actual factory");
            const auto* ntab = JSUConvertOffsetToPtr<ResNTAB>(&owner.shape_block(), owner.shape_block().mpNameTable);
            JUTNameTab names(ntab);
            require(ntab->_2 == 0xbeef && ntab->mEntries[0].mKeyCode == 1 && std::strcmp(names.getName(1), "Two") == 0 && names.getName(2) == nullptr,
                    "native shape names preserve independent stored count, opaque header and authored key codes");
            std::fill(bytes.begin(), bytes.end(), 0xcc);
            bytes.clear(); bytes.shrink_to_fit();
            auto moved = std::make_unique<J3dGeometryData>(std::move(owner));
            require(logical0->mRadius == 13.5F && logical0->getShapeMtx(0)->getUseMtxIndex(2) == 7 &&
                        logical0->getShapeDraw(0)->countVertex(7) == 2 && std::strcmp(names.getName(0), "One") == 0,
                    "all actual shape objects and relative data survive source retirement and owner move");
            bool rejected = false;
            try { moved->attach_to(model); } catch (const std::logic_error&) { rejected = true; }
            require(rejected && model.getShapeNodePointer(0) == logical0, "reattachment cannot replace a live shape table");
            J3DShape::sOldVcdVatCmd = logical0->getVcdVatCmd();
            moved.reset();
            require(J3DShape::sOldVcdVatCmd == nullptr, "retiring a command allocation invalidates its original shared cache");
        }
    }

    void test_native_vertices_and_original_finalizers() {
        auto bytes = fixture();
        J3dGeometryData owner(bytes);
        J3DModelData model;
        owner.attach_to(model);
        auto& vertices = model.getVertexData();
        require(vertices.mVtxNum == 2 && vertices.mPacketNum == 4 && vertices.mNrmNum == 3 &&
                    vertices.mColNum == 5 && vertices.mTexCoordNum == 7,
                "original INF counts and retail readVertex successor/+1 rules are retained");
        const auto* position = static_cast<const s16*>(vertices.mVtxPosArray);
        const auto* normal = static_cast<const u16*>(vertices.mVtxNrmArray);
        const auto* texture = static_cast<const u16*>(vertices.mVtxTexCoordArray[0]);
        require(position[0] == 0x12a0 && position[1] == -256 && normal[8] == 0x3344,
                "native S16 scalars and normal +1 footprint preserve actual next-table bytes");
        require(texture[23] == 0x131e && texture[24] == 0x5348 && texture[25] == 0x5031 && texture[27] == 0x0240,
                "full texture pool and last-table +1 footprint survive including actual next-block bytes");
        const auto* colors = reinterpret_cast<const u8*>(vertices.mVtxColorArray[0]);
        require(colors[0] == 0x80 && colors[15] == 0x8f && colors[16] == 0x12 && colors[17] == 0xf0,
                "byte-channel colors stay untouched, including the source-backed lookahead bytes");
        require(reinterpret_cast<const u8*>(vertices.mVtxAttrFmtList)[13] == 0xa5 && vertices.mVtxPosType == GX_F32 && vertices.mVtxPosFrac == 0,
                "format padding is retained and vertex type state is not advanced before the original finalizer");
        model.getShapeTable()->initShapeNodes(model.getDrawMtxData(), &vertices);
        require(vertices.mVtxPosType == GX_S16 && vertices.mVtxPosFrac == 8 && vertices.mVtxNrmType == GX_S16 && vertices.mVtxNrmFrac == 14,
                "actual original VCD generation sets position and normal formats from the retained table");
        require(model.getShapeNodePointer(0)->getNBTFlag() && !model.getShapeNodePointer(1)->getNBTFlag() &&
                    model.getShapeNodePointer(0)->mHasPNMTXIdx && model.getShapeNodePointer(0)->mVertexData == &vertices,
                "original shape initialization binds actual vertex/draw owners and selects NBT/PN matrix semantics");
        require(model.getShapeNodePointer(1)->getVcdVatCmd() != model.getShapeNodePointer(2)->getVcdVatCmd(),
                "factory allocation gives every logical shape its own command slice before sorting");
        model.getShapeTable()->sortVcdVatCmd();
        require(model.getShapeNodePointer(1)->getVcdVatCmd() == model.getShapeNodePointer(2)->getVcdVatCmd() &&
                    model.getShapeNodePointer(2)->getVcdVatCmd() == model.getShapeNodePointer(3)->getVcdVatCmd() &&
                    model.getShapeNodePointer(0)->getVcdVatCmd() != model.getShapeNodePointer(1)->getVcdVatCmd(),
                "actual original command comparison/sorting aliases identical layouts and preserves NBT differences");
    }

    void test_packed_colors_and_absence() {
        for (const auto type : {GX_RGB565, GX_RGB8, GX_RGBX8, GX_RGBA4, GX_RGBA6, GX_RGBA8}) {
            auto bytes = fixture();
            const auto vertex = block_offset(bytes, "VTX1");
            write32(bytes, vertex + 0x68, type);
            J3dGeometryData owner(bytes);
            J3DModelData model;
            owner.attach_to(model);
            const auto* color = reinterpret_cast<const u8*>(model.getVertexData().mVtxColorArray[0]);
            const auto packed = type == GX_RGB565 || type == GX_RGBA4 ? 2U : type == GX_RGBA6 ? 3U : 1U;
            for (std::size_t record = 0; record < 4; ++record) {
                for (std::size_t component = 0; component < 4; ++component) {
                    const auto source_component = std::endian::native == std::endian::little && component < packed ? packed - 1 - component : component;
                    require(color[record * 4 + component] == 0x80 + record * 4 + source_component,
                            "all packed/byte-channel color formats preserve original fixed four-byte array stride");
                }
            }
        }
        auto bytes = fixture();
        const auto vertex = block_offset(bytes, "VTX1");
        const auto shape = block_offset(bytes, "SHP1");
        for (std::size_t field = 0xc; field < 0x40; field += 4) write32(bytes, vertex + field, 0);
        write32(bytes, shape + 0x14, 0);
        J3dGeometryData owner(bytes);
        J3DModelData model;
        owner.attach_to(model);
        require(model.getNrmNum() == 0 && model.getVertexData().mColNum == 0 && model.getVertexData().mTexCoordNum == 0 &&
                    model.getVertexData().mVtxPosArray == nullptr && model.getVertexData().mVtxNBTArray == nullptr && owner.shape_block().mpNameTable == nullptr,
                "absent vertex/name arrays remain absent without fabricated resources");

        for (const auto flags : {0U, 0x10U}) {
            auto carried = fixture();
            const auto shape_offset = block_offset(carried, "SHP1");
            write16(carried, shape_offset + 0x21a, 11);
            for (std::size_t slot = 3; slot < 11; ++slot) write16(carried, shape_offset + 0x162 + slot * 2, 0xffff);
            J3dGeometryData carried_owner(carried, flags);
            J3DModelData carried_model;
            carried_owner.attach_to(carried_model);
            const auto* matrix = carried_model.getShapeNodePointer(0)->getShapeMtx(0);
            require(matrix->getUseMtxNum() == 11 && matrix->getUseMtxIndex(0) == 2 && matrix->getUseMtxIndex(2) == 7 &&
                        matrix->getUseMtxIndex(10) == 0xffff,
                    "original multi-matrix carry suffix beyond slot nine is preserved without clamping the declared count");
        }
    }

    void test_bad_ranges() {
        const auto reject = [](auto mutate, std::string_view message) {
            auto bytes = fixture();
            mutate(bytes);
            bool rejected = false;
            try { J3dGeometryData owner(bytes); } catch (const std::runtime_error&) { rejected = true; }
            require(rejected, message);
        };
        reject([](Bytes& b) { write32(b, 8, static_cast<u32>(b.size() + 1)); }, "file extent must bound all construction blocks");
        reject([](Bytes& b) { write32(b, block_offset(b, "VTX1") + 8, 0); }, "original format search requires a readable terminator table");
        reject([](Bytes& b) { write32(b, block_offset(b, "VTX1") + 0x48, 99); }, "invalid active numeric component formats must be rejected");
        reject([](Bytes& b) { write32(b, block_offset(b, "VTX1") + 0x10, 0x110); }, "original normal count must not wrap a negative successor distance");
        reject([](Bytes& b) { write32(b, block_offset(b, "INF1") + 0x10, 0xffffffff); }, "actual position CPU-copy footprint must remain readable");
        reject([](Bytes& b) { write16(b, block_offset(b, "SHP1") + 0xcc, 0xffff); }, "remapped shape initializer must be readable");
        reject([](Bytes& b) { b[block_offset(b, "SHP1") + 0x2c] = 4; }, "unknown matrix type must not produce a null virtual owner");
        reject([](Bytes& b) { write16(b, block_offset(b, "SHP1") + 0x30, 1); }, "native descriptor byte offset must preserve alignment");
        reject([](Bytes& b) { write32(b, block_offset(b, "SHP1") + 0x104, 4); }, "descriptor type cannot index original size tables out of bounds");
        reject([](Bytes& b) { write32(b, block_offset(b, "SHP1") + 0x21c, 0xffffffff); }, "multi-matrix array extent must be readable");
        reject([](Bytes& b) { write16(b, block_offset(b, "SHP1") + 0x21a, 11); }, "multi-matrix slots must fit original ten-slot cache");
        reject([](Bytes& b) { write32(b, block_offset(b, "SHP1") + 0x224, 0xffffffff); }, "draw display-list offset and size must fit SHP1");
        reject([](Bytes& b) { write16(b, block_offset(b, "INF1") + 0x22, 4); }, "hierarchy shape reference must fit the real shape table");
        reject([](Bytes& b) { write16(b, block_offset(b, "INF1") + 0x2e, 0); }, "hierarchy cannot leave a shape pointer uninitialized");
        reject([](Bytes& b) { write16(b, block_offset(b, "SHP1") + 0xda, 0xffff); }, "shape names must terminate within the source block");
    }

    void check_real_data(J3DModelData& model, View bytes) {
        const auto source = block(bytes, "SHP1");
        const auto vertex = block(bytes, "VTX1");
        const auto init = read32(source, 0xc), remap = read32(source, 0x10);
        const auto mtx_init = read32(source, 0x24), draw_init = read32(source, 0x28);
        std::size_t groups = 0;
        for (u16 i = 0; i < model.getShapeNum(); ++i) {
            auto* shape = model.getShapeNodePointer(i);
            const auto row = init + read16(source, remap + i * 2) * 40U;
            require(shape->getIndex() == i && shape->getMtxGroupNum() == read16(source, row + 2), "real original shape numbering/group counts match authored records");
            same_float(shape->mRadius, source, row + 12);
            same_float(shape->mMin.y, source, row + 20);
            same_float(shape->mMax.z, source, row + 36);
            for (u16 group = 0; group < shape->getMtxGroupNum(); ++group) {
                ++groups;
                auto* matrix = shape->getShapeMtx(group);
                auto* draw = shape->getShapeDraw(group);
                const auto mtx = mtx_init + (read16(source, row + 6) + group) * 8U;
                const auto dl = draw_init + (read16(source, row + 8) + group) * 8U;
                require(draw->getDisplayListSize() == read32(source, dl) &&
                            std::memcmp(draw->getDisplayList(), source.data() + read32(source, 0x20) + read32(source, dl + 4), read32(source, dl)) == 0,
                        "every original real display-list byte remains unmodified");
                if (source[row] == 3) {
                    require(matrix->getType() == 0x534d4d4c && matrix->getUseMtxNum() == read16(source, mtx + 2), "real multi-matrix original class/count are preserved");
                    for (u16 index = 0; index < matrix->getUseMtxNum(); ++index) {
                        require(matrix->getUseMtxIndex(index) == read16(source, read32(source, 0x1c) + (read32(source, mtx + 4) + index) * 2),
                                "each real matrix carry/index entry remains authored");
                    }
                } else {
                    require(matrix->getType() == 0x534d5458 && matrix->getUseMtxIndex(0) == read16(source, mtx), "real single matrix class/index are preserved");
                }
            }
        }
        require(model.getShapeNum() == 9 && groups == 12 && model.getNrmNum() == 2742 &&
                    model.getVertexData().mColNum == 41 && model.getVertexData().mTexCoordNum == 977,
                "RMGK01 Mario matches independently extracted shape/group and exact retail count metadata");
        const auto* tex = static_cast<const u16*>(model.getVertexData().mVtxTexCoordArray[0]);
        require(tex[3903] == read16(vertex, read32(vertex, 0x20) + 3903 * 2),
                "real Mario's full 1952 S16/ST texture records survive despite original mTexCoordNum=977");
        auto& vertices = model.getVertexData();
        model.getShapeTable()->initShapeNodes(model.getDrawMtxData(), &vertices);
        model.getShapeTable()->sortVcdVatCmd();
        require(vertices.mVtxPosType == GX_S16 && vertices.mVtxPosFrac == 8 && vertices.mVtxNrmType == GX_S16 && vertices.mVtxNrmFrac == 14,
                "actual original VCD finalization accepts real Mario's retained native vertex formats");
        require(model.getShapeNodePointer(0)->getVcdVatCmd() == model.getShapeNodePointer(7)->getVcdVatCmd() &&
                    model.getShapeNodePointer(8)->getVcdVatCmd() != model.getShapeNodePointer(0)->getVcdVatCmd(),
                "actual original sort groups real Mario's two authored VCD layouts");
    }

    void test_optional_real_disc() {
        const auto* path = std::getenv("SMGPC_REAL_DISC");
        if (path == nullptr || path[0] == '\0') {
            std::cout << "[skip] original Mario geometry resources (set SMGPC_REAL_DISC)\n";
            return;
        }
        aurora_dvd_close();
        require(aurora_dvd_open(path), "SMGPC_REAL_DISC must be a readable original disc");
        struct DiscOwner { ~DiscOwner() { aurora_dvd_close(); } } disc;
        DVDInit();
        std::unique_ptr<J3dGeometryData> resource;
        J3DModelData model;
        Bytes retained_draw;
        {
            smgpc::runtime::DvdFileSystemService dvd{"/"};
            const auto archive_path = dvd.find_object_archive("Mario");
            require(archive_path.has_value(), "actual Mario archive must exist");
            const auto& archive = dvd.archive_for_path(*archive_path);
            const auto* entry = archive.find_by_basename("mario.bdl");
            require(entry != nullptr, "actual Mario model must be present");
            const auto bytes = archive.file_data(*entry);
            resource = std::make_unique<J3dGeometryData>(bytes);
            resource->attach_to(model);
            check_real_data(model, bytes);
            const auto* draw = model.getShapeNodePointer(8)->getShapeDraw(3);
            retained_draw.assign(draw->getDisplayList(), draw->getDisplayList() + draw->getDisplayListSize());
        }
        const auto* draw = model.getShapeNodePointer(8)->getShapeDraw(3);
        require(std::memcmp(retained_draw.data(), draw->getDisplayList(), retained_draw.size()) == 0 && model.getShapeNodePointer(8)->getIndex() == 8,
                "actual native geometry survives retirement of its DVD/archive source");
        require(model.getMaterialNum() == 0 && model.getJointNum() == 0 && model.mpRawData == nullptr,
                "real geometry component fixture makes no claim of complete material/model decoding");
        std::cout << "[resource] mario.bdl: shapes=9, groups=12, normals=2742, colors=41, tex metadata=977, physical S16/ST=1952\n";
    }
}

int main() {
    try {
        test_original_factory_and_lifetime();
        test_native_vertices_and_original_finalizers();
        test_packed_colors_and_absence();
        test_bad_ranges();
        test_optional_real_disc();
        std::cout << "[pass] 5 original J3D geometry-resource groups\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[fail] original J3D geometry resource: " << error.what() << '\n';
        return 1;
    }
}
