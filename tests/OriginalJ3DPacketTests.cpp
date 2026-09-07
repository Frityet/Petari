#include "compat/MetrowerksStdCompat.hpp"

#include "JSystem/J3DGraphBase/J3DGD.hpp"
#include "JSystem/J3DGraphBase/J3DPacket.hpp"
#include "JSystem/J3DGraphBase/J3DShapeDraw.hpp"
#include "JSystem/J3DGraphBase/J3DShapeMtx.hpp"
#include "JSystem/J3DGraphBase/J3DVertex.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
    void require(bool condition, const char* message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }

    // The original display-list destructor does not own its allocations.
    struct DisplayListOwner {
        J3DDisplayListObj list;

        ~DisplayListOwner() {
            if (list.mpDisplayList[1] != list.mpDisplayList[0]) {
                ::operator delete[](list.mpDisplayList[1], 0x20);
            }
            ::operator delete[](list.mpDisplayList[0], 0x20);
        }
    };

    void display_list_recording() {
        DisplayListOwner owner;
        auto& list = owner.list;
        require(list.newSingleDisplayList(63) == kJ3DError_Success, "allocate actual single display list");
        require(list.mMaxSize == 64 && list.mpDisplayList[0] == list.mpDisplayList[1], "single buffer alignment and identity");

        list.beginDL();
        J3DGDWriteBPCmd(0x11223344);
        J3DGDWriteCPCmd(0x30, 0x55667788);
        J3DGDWriteXFCmd(0x1009, 0x12345678);
        J3DGDWriteXFCmdHdr(0x1040, 2);
        J3DGDWrite_u32(0xABCDEF01);
        J3DGDWrite_u32(0x23456789);
        require(list.endDL() == 64, "33 command bytes pad to 64");
        require(__GDCurrentDL == nullptr, "endDL releases actual GD current list");
        constexpr std::array<u8, 33> expected{
            0x61, 0x11, 0x22, 0x33, 0x44,
            0x08, 0x30, 0x55, 0x66, 0x77, 0x88,
            0x10, 0x00, 0x00, 0x10, 0x09, 0x12, 0x34, 0x56, 0x78,
            0x10, 0x00, 0x01, 0x10, 0x40, 0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45, 0x67, 0x89};
        const auto* first = list.getDisplayList(0);
        require(std::equal(expected.begin(), expected.end(), first), "original BP, CP and XF command bytes");
        require(std::all_of(first + expected.size(), first + list.mSize, [](u8 value) { return value == 0; }), "display-list padding is zero");

        require(list.single_To_Double() == kJ3DError_Success, "convert actual list to two buffers");
        require(list.mpDisplayList[0] != list.mpDisplayList[1], "conversion allocates a distinct buffer");
        require(std::memcmp(list.mpDisplayList[0], list.mpDisplayList[1], list.mMaxSize) == 0, "conversion preserves recorded bytes");
        auto* original = list.mpDisplayList[0];
        list.beginDL();
        require(list.mpDisplayList[1] == original, "recording swaps buffers");
        J3DGDWriteBPCmd(0x99887766);
        require(list.endDL() == 32, "short command sequence pads independently");
        require(std::equal(expected.begin(), expected.end(), static_cast<u8*>(original)), "previous frame's commands remain intact");
    }

    void primitive_count_and_matrix_index_insertion() {
        constexpr u32 stride = 3;
        constexpr u32 insertion = 2;
        constexpr u32 base = 30;
        std::vector<u8> original;
        const auto primitive = [&](u8 command, u16 count) {
            original.push_back(command);
            original.push_back(static_cast<u8>(count >> 8));
            original.push_back(static_cast<u8>(count));
            for (u16 i = 0; i < count; ++i) {
                original.push_back(static_cast<u8>((i % 10) * 3));
                original.push_back(static_cast<u8>(i));
                original.push_back(static_cast<u8>(i >> 8));
            }
        };
        primitive(GX_TRIANGLEFAN, 3);
        primitive(GX_TRIANGLESTRIP, 258);
        original.push_back(0);
        original.resize((original.size() + 31) & ~std::size_t(31), 0);
        const auto retained = original;
        J3DShapeDraw draw(original.data(), original.size());
        require(draw.countVertex(stride) == 261, "unaligned big-endian counts include values above 255");
        draw.addTexMtxIndexInDL(stride, insertion, base);
        auto* expanded = draw.getDisplayList();
        require(expanded != original.data(), "insertion allocates the expanded list");
        require(reinterpret_cast<std::uintptr_t>(expanded) % 32 == 0, "expanded list remains aligned");
        require(draw.countVertex(stride + 1) == 261, "expanded list preserves both primitive counts");
        require(original == retained, "borrowed source commands remain unchanged");

        std::size_t old_offset = 0;
        std::size_t new_offset = 0;
        for (u16 count : {u16(3), u16(258)}) {
            require(std::memcmp(expanded + new_offset, original.data() + old_offset, 3) == 0, "primitive header byte order is preserved");
            old_offset += 3;
            new_offset += 3;
            for (u16 i = 0; i < count; ++i) {
                require(expanded[new_offset] == original[old_offset] && expanded[new_offset + 1] == original[old_offset + 1], "attributes before insertion are preserved");
                require(expanded[new_offset + 2] == static_cast<u8>(base + original[old_offset]), "inserted matrix index uses the original position index");
                require(expanded[new_offset + 3] == original[old_offset + 2], "attributes after insertion are preserved");
                old_offset += stride;
                new_offset += stride + 1;
            }
        }
        require(draw.getDisplayListSize() == ((new_offset + 1 + 31) & ~std::size_t(31)), "expanded command extent rounds the terminator to 32 bytes");
        require(std::all_of(expanded + new_offset, expanded + draw.getDisplayListSize(), [](u8 value) { return value == 0; }), "expanded terminator and padding are zero");
        ::operator delete[](expanded, 0x20);
    }

    void original_shape_array_recording() {
        alignas(32) std::array<u8, J3DShape::kVcdVatDLSize + 32> commands;
        commands.fill(0xCD);
        std::array<std::array<float, 12>, 12> arrays{};
        std::array<std::array<GXColor, 4>, 2> colors{};
        std::array<const void*, 12> pointers{};
        std::array<GXVtxAttrFmtList, 13> formats{};
        std::array<GXVtxDescList, 13> descriptors{};
        for (u32 i = 0; i < 12; ++i) {
            const auto attr = static_cast<GXAttr>(GX_VA_POS + i);
            const bool color = attr == GX_VA_CLR0 || attr == GX_VA_CLR1;
            const auto count = attr == GX_VA_POS ? GX_POS_XYZ : attr == GX_VA_NRM ? GX_NRM_XYZ : color ? GX_CLR_RGBA : GX_TEX_ST;
            formats[i] = {attr, count, color ? GX_RGBA8 : GX_F32, 0};
            descriptors[i] = {attr, GX_INDEX8};
            pointers[i] = color ? static_cast<const void*>(colors[i - 2].data()) : arrays[i].data();
        }
        formats[12].attr = GX_VA_NULL;
        descriptors[12].attr = GX_VA_NULL;
        J3DVertexData data;
        data.mVtxAttrFmtList = formats.data();
        data.mVtxPosArray = arrays[0].data();
        data.mVtxNrmArray = arrays[1].data();
        for (u32 i = 0; i < 2; ++i) data.mVtxColorArray[i] = colors[i].data();
        for (u32 i = 0; i < 8; ++i) data.mVtxTexCoordArray[i] = arrays[4 + i].data();
        J3DShape shape;
        shape.setVertexDataPointer(&data);
        shape.mVtxDesc = descriptors.data();
        shape.setVcdVatCmd(commands.data());
        for (int repeat = 0; repeat < 2; ++repeat) {
            shape.makeVcdVatCmd();
            require(__GDCurrentDL == nullptr, "shape recording releases the current GD list on every call");
            require(std::all_of(commands.end() - 32, commands.end(), [](u8 b) { return b == 0xCD; }), "full-pointer commands stay within the original shape's native buffer");
            for (const auto* pointer : pointers) {
                const auto address = reinterpret_cast<std::uintptr_t>(pointer);
                std::array<u8, 8> encoded{};
                for (u32 byte = 0; byte < 8; ++byte) encoded[byte] = static_cast<u8>(address >> ((7 - byte) * 8));
                require(std::search(commands.begin(), commands.end() - 32, encoded.begin(), encoded.end()) != commands.end() - 32,
                        "the original shape records every full native array pointer");
            }
        }
    }

    void original_multi_matrix_nbt_scale() {
        Mtx33 input[3];
        Mtx33 output[3];
        for (u32 matrix = 0; matrix < 3; ++matrix) {
            for (u32 row = 0; row < 3; ++row) {
                for (u32 column = 0; column < 3; ++column) {
                    input[matrix][row][column] = static_cast<float>(matrix * 9 + row * 3 + column + 1);
                    output[matrix][row][column] = -1;
                }
            }
        }
        u16 indices[]{2, 0xFFFF, 0};
        J3DShapeMtxMulti matrices(0, 3, indices);
        const Vec scale{2, 3, 4};
        matrices.calcNBTScale(scale, input, output);
        const float factors[]{2, 3, 4};
        for (u32 matrix = 0; matrix < 3; ++matrix) {
            for (u32 row = 0; row < 3; ++row) {
                for (u32 column = 0; column < 3; ++column) {
                    require(output[matrix][row][column] == (matrix == 1 ? -1 : input[matrix][row][column] * factors[column]),
                            "original multi NBT scaling follows its index table and skips 0xFFFF");
                }
            }
        }
    }
}

int main() {
    try {
        display_list_recording();
        primitive_count_and_matrix_index_insertion();
        original_shape_array_recording();
        original_multi_matrix_nbt_scale();
        std::cout << "4 original J3D packet/display-list groups passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
