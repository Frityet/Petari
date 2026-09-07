#include "compat/MetrowerksStdCompat.hpp"
#include "JSystem/J3DGraphBase/J3DMaterial.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
    void require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }

    struct Commands {
        alignas(32) std::array<u8, 1024> bytes;
        GDLObj dl;
        Commands() {
            bytes.fill(0xCD);
            GDInitGDLObj(&dl, bytes.data(), bytes.size());
            GDSetCurrent(&dl);
        }
        ~Commands() { GDSetCurrent(nullptr); }
        u32 word(std::size_t offset) const {
            return u32(bytes[offset]) << 24 | u32(bytes[offset + 1]) << 16 |
                   u32(bytes[offset + 2]) << 8 | bytes[offset + 3];
        }
    };

    void original_factory_selection() {
        for (auto [flags, type] : std::array<std::pair<u32, u32>, 3>{{
                 {0, 'CLOF'}, {0x40000000, 'CLON'}, {0x80000000, 'CLAB'}}}) {
            std::unique_ptr<J3DColorBlock> block(J3DMaterial::createColorBlock(flags));
            require(block && block->getType() == type, "color factory chooses the original block");
            const auto* color = block->getMatColor(1);
            require(color->r == 255 && color->g == 255 && color->b == 255 && color->a == 255,
                    "actual material defaults are white");
            require(block->getColorChanNum() == 0, "new blocks have no authored color channels yet");
        }
        require(J3DMaterial::createColorBlock(1) == nullptr, "unknown color factory flags retain null outcome");
        for (auto [stages, type] : std::array<std::pair<int, u32>, 5>{{
                 {0, 'TVB1'}, {2, 'TVB2'}, {3, 'TVB4'}, {4, 'TVB4'}, {16, 'TV16'}}}) {
            std::unique_ptr<J3DTevBlock> block(J3DMaterial::createTevBlock(stages));
            require(block && block->getType() == type && block->getTexNo(0) == 0xFFFF,
                    "TEV stage capacity and unbound texture sentinel come from the original factory");
        }
        require(J3DMaterial::createTevBlock(17) == nullptr, "original TEV capacity limit");
        std::unique_ptr<J3DTexGenBlock> basic(J3DMaterial::createTexGenBlock(0));
        std::unique_ptr<J3DTexGenBlock> four(J3DMaterial::createTexGenBlock(0x8000000));
        require(basic->getType() == 'TGBC' && four->getType() == 'TGB4', "texture generator variants");
        require(J3DMaterial::calcSizeTexGenBlock(0) == sizeof(J3DTexGenBlockBasic),
                "original sizeof allocation calculation follows the native ABI");
    }

    void color_commands_and_patch_extent() {
        Commands commands;
        J3DColorBlockAmbientOn block;
        *block.getMatColor(0) = GXColor{0x12, 0x34, 0x56, 0x78};
        *block.getMatColor(1) = GXColor{0x9A, 0xBC, 0xDE, 0xF0};
        *block.getAmbColor(0) = GXColor{1, 3, 5, 7};
        *block.getAmbColor(1) = GXColor{2, 4, 6, 8};
        block.load();
        require(GDGetCurrOffset() == 47, "original color and channel display-list extent");
        require(commands.bytes[0] == 0x10 && commands.word(1) == 0x0001100C &&
                    commands.word(5) == 0x12345678 && commands.word(9) == 0x9ABCDEF0,
                "packed material colors keep RGBA order in XF commands");
        require(commands.bytes[13] == 0x10 && commands.word(14) == 0x0001100A &&
                    commands.word(18) == 0x01030507 && commands.word(22) == 0x02040608,
                "ambient colors retain separate XF registers and byte order");
        const auto before = commands.bytes;
        *block.getMatColor(0) = GXColor{8, 6, 4, 2};
        block.patchMatColor();
        require(GDGetCurrOffset() == 13 && commands.word(5) == 0x08060402,
                "patch rewrites original material-color location");
        require(std::equal(before.begin() + 13, before.end(), commands.bytes.begin() + 13),
                "material patch preserves ambient data, channel data and guard bytes");
    }

    void packed_stage_and_texture_number_commands() {
        Commands commands;
        J3DTevStage stage;
        stage.setStageNo(3);
        stage.setTevColorAB(2, 5);
        stage.setTevColorCD(7, 9);
        stage.setTevColorOp(0, 1, 2, 1, 3);
        stage.setAlphaABCD(1, 2, 3, 4);
        stage.setTevAlphaOp(1, 2, 1, 0, 2);
        stage.setRasSel(2);
        stage.setTexSel(1);
        stage.load(3);
        constexpr std::array<u8, 10> expected{0x61, 0xC6, 0xE9, 0x25, 0x79, 0x61, 0xC7, 0x96, 0x29, 0xC6};
        require(std::equal(expected.begin(), expected.end(), commands.bytes.begin()),
                "original packed TEV bytes produce the selected color/alpha arithmetic");
        // The register starts at offset1: this deliberately covers unaligned input.
        std::array<u8, 5> texture{0x61, 0x94, 0x12, 0xAB, 0xCD};
        require(isTexNoReg(texture.data()) && getTexNoReg(texture.data()) == 0xABCD,
                "native texture-number relocation reads unaligned big-endian BP data");
    }

    void depth_and_fog_commands() {
        Commands commands;
        J3DPEBlockXlu translucent;
        translucent.load();
        require(GDGetCurrOffset() == 30, "original translucent pixel block has six BP writes");
        require(commands.bytes[15] == 0x61 && commands.word(16) == 0x40000007,
                "translucent materials test LEQUAL depth without writing it");
        GDSetCurrOffset(0);
        GXFogAdjTable table{};
        for (u16 i = 0; i < 10; ++i) table.r[i] = 100 + 3 * i;
        J3DGDSetFogRangeAdj(1, 320, &table);
        require(GDGetCurrOffset() == 30, "enabled range adjustment has five coefficient pairs and control");
        for (u32 i = 0; i < 5; ++i) {
            require(commands.bytes[i * 5] == 0x61 && commands.word(i * 5 + 1) ==
                        ((0xE9 + i) << 24 | u32(table.r[2 * i + 1]) << 12 | table.r[2 * i]),
                    "fog adjustment command pairs preserve all ten authored coefficients");
        }
        require(commands.word(26) == 0xE8000696, "fog center includes the original342 pixel bias");
        GDSetCurrOffset(0);
        J3DGDSetFogRangeAdj(0, 1, nullptr);
        require(GDGetCurrOffset() == 5 && commands.word(1) == 0xE8000157,
                "disabled range adjustment does not read a table");
    }

    void structure_assignment_boundaries() {
        J3DIndTexMtxInfo source{}, destination{};
        for (u32 i = 0; i < 2; ++i) for (u32 j = 0; j < 3; ++j) source.field_0x0[i][j] = float(i * 3 + j) - 2.5F;
        source.field_0x18 = 0xFD;
        std::memset(reinterpret_cast<u8*>(&destination) + 25, 0xAB, 3);
        destination = source;
        require(std::memcmp(source.field_0x0, destination.field_0x0, 24) == 0 && destination.field_0x18 == 0xFD,
                "indirect matrix assignment copies six components and raw exponent");
        require(reinterpret_cast<u8*>(&destination)[25] == 0xAB && reinterpret_cast<u8*>(&destination)[27] == 0xAB,
                "indirect assignment preserves padding bytes");
        destination = destination;
        require(destination.field_0x0[1][2] == 2.5F, "indirect matrix self assignment");
        J3DTexCoord from, to;
        from.mTexMtxReg = 17;
        to.mTexMtxReg = 42;
        from.mTexGenSrc = 3;
        to = from;
        require(to.mTexMtxReg == 42 && to.mTexGenSrc == 3, "texture-coordinate assignment preserves cached matrix register");
        J3DNBTScaleInfo nbt{}, nbtCopy{};
        std::memset(reinterpret_cast<u8*>(&nbtCopy) + 1, 0xAB, 3);
        nbt.mbHasScale = 1;
        nbt.mScale = {1.5F, -2.5F, 3.5F};
        nbtCopy = nbt;
        require(nbtCopy.mbHasScale == 1 && nbtCopy.mScale.y == -2.5F && reinterpret_cast<u8*>(&nbtCopy)[2] == 0xAB,
                "NBT assignment copies flag and vector while preserving padding");
        J3DFogInfo fog{}, fogCopy{};
        fog.mType = 6; fog.mAdjEnable = 1; fog.mCenter = 640;
        fog.mStartZ = 2; fog.mEndZ = 200; fog.mNearZ = 1; fog.mFarZ = 1000;
        fog.mColor = {11, 22, 33, 44};
        for (u16 i = 0; i < 10; ++i) fog.mFogAdjTable.r[i] = 100 + i;
        fogCopy = fog;
        require(std::memcmp(&fogCopy, &fog, sizeof(fog)) == 0, "fog assignment retains every typed component");
    }
}

int main() {
    try {
        original_factory_selection();
        color_commands_and_patch_extent();
        packed_stage_and_texture_number_commands();
        depth_and_fog_commands();
        structure_assignment_boundaries();
        std::cout << "5/5 original J3D material-block groups passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[fail] " << error.what() << '\n';
        return 1;
    }
}
