#include "Game/Util/FurShader.hpp"
#include "JSystem/J3DGraphAnimator/J3DModelData.hpp"
#include "JSystem/J3DGraphBase/J3DMaterial.hpp"
#include "JSystem/J3DGraphBase/J3DShapeDraw.hpp"
#include "JSystem/JUtility/JUTTexture.hpp"
#include "resource/J3dGeometryData.hpp"
#include "resource/J3dNameData.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
    using Bytes = std::vector<std::uint8_t>;
    void require(bool condition, std::string_view message) {
        if (!condition) throw std::runtime_error(std::string(message));
    }
    void near(float actual, float expected, std::string_view message) {
        require(std::abs(actual - expected) < 0.00001F, message);
    }
    void put16(Bytes& bytes, std::size_t offset, std::uint16_t value) {
        bytes.at(offset) = value >> 8; bytes.at(offset + 1) = value;
    }
    void put32(Bytes& bytes, std::size_t offset, std::uint32_t value) {
        put16(bytes, offset, value >> 16); put16(bytes, offset + 2, value);
    }
    void tag(Bytes& bytes, std::size_t offset, std::string_view text) {
        std::copy(text.begin(), text.end(), bytes.begin() + offset);
    }
    struct Image {
        ResTIMG header;
        std::array<std::uint8_t, 128> pixels;
        Image() {
            static_assert(sizeof(ResTIMG) == 32 && offsetof(Image, pixels) == 32);
            header.mFormat = GX_TF_I8; header.mWidth = 16; header.mHeight = 8;
            for (std::size_t i = 0; i < pixels.size(); ++i) pixels[i] = static_cast<std::uint8_t>(i);
            pixels.back() = 255;
        }
    };
    std::array<GXVtxAttrFmtList, 4> formats(GXCompType type) {
        return {{{GX_VA_POS, GX_POS_XYZ, type, 4}, {GX_VA_NRM, GX_NRM_XYZ, type, 8},
                 {GX_VA_TEX0, GX_TEX_ST, type, 8}, {GX_VA_NULL, GX_POS_XY, GX_U8, 0}}};
    }
    void tiled_i8_and_original_wrap() {
        Image image;
        CShader::CLengthMap map(&image.header);
        struct Sample { float s, t; int value; };
        // Four 8x4 tiles, with deliberately different values at all seams.
        for (const auto sample : std::array{
                 Sample{0, 0, 0}, Sample{7.1F / 15, 0, 7}, Sample{8.1F / 15, 0, 32},
                 Sample{0, 3.1F / 7, 24}, Sample{0, 4.1F / 7, 64},
                 Sample{8.1F / 15, 4.1F / 7, 96}, Sample{1, 1, 255}})
            near(map.refer(sample.s, sample.t), sample.value / 255.0F, "Original tiled I8 sampling differs.");
        for (const auto mode : {GX_CLAMP, GX_REPEAT, GX_MIRROR}) {
            require(map.getTexelOrder(16, 0, mode) == 0 && map.getTexelOrder(16, 1, mode) == 15,
                    "Original coordinate endpoints must include the last texel.");
            require(map.getTexelOrder(16, -0.5F, mode) == -7,
                    "Original wrapper does not enter its wrapping branch for negative input.");
        }
        require(map.getTexelOrder(16, 2.5F, GX_CLAMP) == 15 &&
                    map.getTexelOrder(16, 2.5F, GX_REPEAT) == 7 &&
                    map.getTexelOrder(16, 2.5F, GX_MIRROR) == 7,
                "Original repeat and mirror branches both reduce coordinates greater than one.");
        image.header.mWrapS = GX_REPEAT;
        near(map.refer(2.5F, 0), 7 / 255.0F, "Texture wrap metadata must reach the actual coordinate helper.");
        const auto* previous = map._0;
        map.setLengthMap(nullptr);
        require(map._8 && map._0 == previous && map.refer(0, 0) == 1, "Null map disables sampling without rewriting the retained pointer.");
        image.header.mFormat = GX_TF_RGBA8;
        map.setLengthMap(&image.header);
        require(map._8 && map.refer(0, 0) == 1, "Unsupported format returns original unit length.");
        image.header.mFormat = GX_TF_I8;
        map.setLengthMap(&image.header);
        image.header.mFormat = GX_TF_I4;
        require(!map._8 && map.refer(0, 0) == 1, "Sampling rechecks the live image format.");
        CShader::CLengthMap absent(nullptr);
        require(absent.refer(0, 0) == 1, "Original absent map is disabled.");
    }
    void raw_display_list_index_mapping() {
        J3DModelData data;
        auto format = formats(GX_F32);
        data.mVertexData.mVtxNum = 768;
        data.mVertexData.mVtxAttrFmtList = format.data();
        CShader shader(&data, nullptr);
        require(shader._4 == 0 && shader._8 == 0 && shader._1C == 0 && shader._24 == GX_F32 &&
                    shader._21 == 4 && shader._22 == 8 && shader._20 == 8, "Original constructor metadata differs.");
        auto descriptor = std::array<GXVtxDescList, 5>{{
            {GX_VA_PNMTXIDX, GX_DIRECT}, {GX_VA_POS, GX_INDEX16}, {GX_VA_NRM, GX_INDEX16},
            {GX_VA_TEX0, GX_INDEX16}, {GX_VA_NULL, GX_NONE}}};
        Bytes bytes(32);
        // Offset one makes the first position/normal/UV halfwords unaligned;
        // the seven-byte stride alternates alignment for successive vertices.
        bytes[1] = GX_TRIANGLESTRIP; put16(bytes, 2, 2);
        bytes[4] = 9; put16(bytes, 5, 0x0102); put16(bytes, 7, 0x0123); put16(bytes, 9, 0x0234);
        bytes[11] = 4; put16(bytes, 12, 2); put16(bytes, 14, 3); put16(bytes, 16, 4);
        bytes[18] = GX_TRIANGLEFAN; put16(bytes, 19, 1);
        bytes[21] = 8; put16(bytes, 22, 0x0102); put16(bytes, 24, 0x0201); put16(bytes, 26, 0x0103);
        J3DShapeDraw draw(bytes.data() + 1, bytes.size() - 1);
        auto draws = std::array{&draw};
        J3DShape shape;
        shape.mVtxDesc = descriptor.data(); shape.mMtxGroupNum = 1; shape.mShapeDraw = draws.data();
        shader.makeIndexData(&shape);
        require(shader.mIndexArray[0x0102]._0 == 0x0201 && shader.mIndexArray[0x0102]._2 == 0x0103,
                "Raw big-endian halfwords and later command overwrite must preserve actual normal/UV indices.");
        require(shader.mIndexArray[2]._0 == 3 && shader.mIndexArray[2]._2 == 4 &&
                    shader.mIndexArray[1]._0 == 0xffff && shader.mIndexArray[1]._2 == 0xffff,
                "Only authored display-list positions receive mappings.");
        descriptor[3].attr = GX_VA_NULL;
        shader.mIndexArray[2]._0 = 99;
        shader.makeIndexData(&shape);
        require(shader.mIndexArray[2]._0 == 99, "A shape missing UV descriptors must leave mapping untouched.");
    }
    Bytes border_model_bytes() {
        Bytes info(0x30); tag(info, 0, "INF1"); put32(info, 0xc, 4); put32(info, 0x10, 4); put32(info, 0x14, 0x18);
        for (std::size_t i = 0; i < 4; ++i) { put16(info, 0x18 + i * 4, 0x12); put16(info, 0x1a + i * 4, i); }
        Bytes vertex(0x50); tag(vertex, 0, "VTX1"); put32(vertex, 8, 0x40); put32(vertex, 0x40, GX_VA_NULL);
        Bytes shape(0x144); tag(shape, 0, "SHP1"); put16(shape, 8, 4);
        put32(shape, 0xc, 0x2c); put32(shape, 0x10, 0xcc); put32(shape, 0x18, 0xd4);
        put32(shape, 0x20, 0x124); put32(shape, 0x24, 0xe4); put32(shape, 0x28, 0x104);
        put32(shape, 0xd4, GX_VA_POS); put32(shape, 0xd8, GX_INDEX16); put32(shape, 0xdc, GX_VA_NULL);
        for (std::size_t i = 0; i < 4; ++i) {
            put16(shape, 0xcc + i * 2, i);
            const auto init = 0x2c + i * 40;
            put16(shape, init + 2, 1); put16(shape, init + 4, i == 3 ? 8 : 0);
            put16(shape, init + 6, i); put16(shape, init + 8, i);
            put32(shape, 0x104 + i * 8, 8); put32(shape, 0x108 + i * 8, i * 8);
            const auto dl = 0x124 + i * 8;
            shape[dl] = GX_TRIANGLEFAN; put16(shape, dl + 1, 1); put16(shape, dl + 3, i);
        }
        Bytes file(0x20); tag(file, 0, "J3D2bmd3"); put32(file, 0xc, 3);
        for (auto* block : {&info, &vertex, &shape}) {
            put32(*block, 4, block->size()); file.insert(file.end(), block->begin(), block->end());
        }
        put32(file, 8, file.size()); return file;
    }
    void actual_shape_border_exclusion() {
        smgpc::resource::J3dGeometryData owner(border_model_bytes());
        J3DModelData data; owner.attach_to(data);
        Bytes raw_names(96); put16(raw_names, 0, 4);
        const auto names = std::array<std::string_view, 4>{"CurrentFur", "OtherFur", "Body", "NoPosition"};
        std::size_t offset = 20;
        for (std::size_t i = 0; i < names.size(); ++i) {
            put16(raw_names, 6 + i * 4, offset); tag(raw_names, offset, names[i]); offset += names[i].size() + 1;
        }
        smgpc::resource::J3dNameData material_names(raw_names);
        data.mMaterialTable.mMaterialName = material_names.table();
        std::array<J3DMaterial, 4> materials;
        for (std::size_t i = 0; i < materials.size(); ++i) {
            materials[i].mIndex = i; data.getShapeNodePointer(i)->setMaterial(&materials[i]);
        }
        CShader shader(&data, nullptr);
        for (std::size_t i = 0; i < 4; ++i) { shader.mIndexArray[i]._0 = i + 10; shader.mIndexArray[i]._2 = i + 20; }
        shader.checkBorderVtx(&data, 0);
        require(shader.mIndexArray[0]._0 == 10 && shader.mIndexArray[1]._0 == 11,
                "Current shape and other Fur materials must retain mappings.");
        require(shader.mIndexArray[2]._0 == 0xffff && shader.mIndexArray[2]._2 == 0xffff,
                "Positions shared with an ordinary material must be excluded from displacement.");
        require(shader.mIndexArray[3]._0 == 13 && shader.mIndexArray[3]._2 == 23,
                "Shapes without a position descriptor must not alter mappings.");
    }
    void float_displacement_and_double_buffer() {
        Image image; image.pixels.fill(255); image.pixels[0] = 0; image.pixels[31] = 128;
        auto format = formats(GX_F32);
        auto positions = std::array{TVec3f(1, 2, 3), TVec3f(-1, 0, 2), TVec3f(4, 5, 6), TVec3f(7, 8, 9)};
        auto normals = std::array{TVec3f(0, 0, 3), TVec3f(0, 4, 0), TVec3f(-2, 0, 0)};
        auto uv = std::array{TVec2f(0, 0), TVec2f(1, 1), TVec2f(0.5F, 0.5F)};
        auto first = std::array<TVec3f, 4>{}; auto second = first;
        first.fill(TVec3f(88, 88, 88)); second.fill(TVec3f(77, 77, 77));
        J3DModelData data;
        data.mVertexData.mVtxNum = positions.size(); data.mVertexData.mVtxAttrFmtList = format.data();
        data.mVertexData.mVtxPosArray = positions.data(); data.mVertexData.mVtxNrmArray = normals.data();
        data.mVertexData.mVtxTexCoordArray[0] = uv.data();
        J3DModel model; auto* buffer = model.getVertexBuffer(); buffer->setVertexData(&data.mVertexData);
        buffer->mTransformedVtxPosArray[0] = first.data(); buffer->mTransformedVtxPosArray[1] = second.data();
        CShader shader(&data, &image.header); shader._1C = 10;
        shader.mIndexArray[0]._0 = 1; shader.mIndexArray[0]._2 = 0;
        shader.mIndexArray[1]._0 = 0; shader.mIndexArray[1]._2 = 1;
        shader.mIndexArray[2]._0 = 2; shader.mIndexArray[2]._2 = 2;
        J3DUnkCalc1* callback = &shader; callback->calc(&model);
        require(buffer->getCurrentVtxPos() == second.data() && buffer->getTransformedVtxPos(1) == first.data(),
                "Actual shader callback must swap and publish the transformed position buffer.");
        require(second[0].epsilonEquals(TVec3f(1, 1, 3), 0.00001F) && second[1].epsilonEquals(TVec3f(-1, 0, 12), 0.00001F),
                "Zero mask retracts one unit; full mask displaces by _1C along normalized mapped normals.");
        near(second[2].x, 4 - 10 * (128 / 255.0F), "Partial I8 mask must scale displacement.");
        require(second[3].x == 77 && positions[0].y == 2, "Sentinels and original source positions remain untouched.");
        callback->calc(&model);
        require(buffer->getCurrentVtxPos() == first.data() && buffer->getTransformedVtxPos(1) == second.data() && first[3].x == 88,
                "Subsequent calculation must alternate actual buffers and preserve excluded destinations.");
        near(first[0].y, 0, "Subsequent retraction must read the current deformed position.");
        near(first[1].z, 22, "Subsequent extension must read the current deformed position.");
    }
    void signed_fixed_displacement_and_uv_range() {
        Image image; image.pixels.fill(128); image.pixels[0] = 0; image.pixels[98] = 255;
        auto format = formats(GX_S16);
        auto positions = std::array{TVec3s(16, 32, 48), TVec3s(-16, 0, 32), TVec3s(64, 80, 96)};
        auto normals = std::array{TVec3s(0, 0, 512), TVec3s(0, 1024, 0)};
        auto uv = std::array{TVec2s(0, 0), TVec2s(512, 512), TVec2s(-256, -256)};
        auto output = positions; auto other = positions;
        J3DModelData data; data.mVertexData.mVtxNum = 3; data.mVertexData.mVtxAttrFmtList = format.data();
        data.mVertexData.mVtxPosArray = positions.data(); data.mVertexData.mVtxNrmArray = normals.data();
        data.mVertexData.mVtxTexCoordArray[0] = uv.data();
        J3DModel model; auto* buffer = model.getVertexBuffer(); buffer->setVertexData(&data.mVertexData);
        buffer->mTransformedVtxPosArray[0] = other.data(); buffer->mTransformedVtxPosArray[1] = output.data();
        CShader shader(&data, &image.header); shader._1C = 2;
        for (int i = 0; i < 3; ++i) { shader.mIndexArray[i]._0 = i == 0 ? 1 : 0; shader.mIndexArray[i]._2 = i; }
        shader.calc(&model);
        require(output[0].x == 16 && output[0].y == 16 && output[0].z == 48,
                "Signed fixed positions/normals must use independent original fractional scales.");
        require(output[1].x == -16 && output[1].z == 64 && output[2].z == 112,
                "Fixed UV coordinates use integer span and preserve the one-texel float rounding difference after negative wrap.");
        require(buffer->getCurrentVtxPos() == output.data(), "Fixed output must publish its actual transformed buffer.");
    }
}

int main() {
    struct Test { const char* name; void (*run)(); };
    const auto tests = std::array{
        Test{"tiled I8 and original wrapping", tiled_i8_and_original_wrap},
        Test{"raw display-list index mapping", raw_display_list_index_mapping},
        Test{"actual shape border exclusion", actual_shape_border_exclusion},
        Test{"float displacement and double buffer", float_displacement_and_double_buffer},
        Test{"signed fixed displacement and UV range", signed_fixed_displacement_and_uv_range},
    };
    auto failures = 0;
    for (const auto& test : tests) {
        try { test.run(); std::cout << "[ok] " << test.name << '\n'; }
        catch (const std::exception& error) { ++failures; std::cerr << "[fail] " << test.name << ": " << error.what() << '\n'; }
    }
    return failures == 0 ? 0 : 1;
}
