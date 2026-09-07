#include "Game/Util/DirectDrawUtil.hpp"
#include "../aurora/lib/dolphin/gx/__gx.h"
#include "../aurora/lib/gx/fifo.hpp"

#include <array>
#include <bit>
#include <cstdio>
#include <cstdlib>

namespace {
    void require(bool value, const char* message) {
        if (!value) { std::fprintf(stderr, "%s\n", message); std::abort(); }
    }
    void check_float(const u8* bytes, float expected) {
        const u32 word = u32(bytes[0]) << 24 | u32(bytes[1]) << 16 | u32(bytes[2]) << 8 | bytes[3];
        require(word == std::bit_cast<u32>(expected), "original direct draw must emit exact big-endian float payloads");
    }
    void check_mode(u32 mode, bool three_vectors) {
        *__gx = {};
        alignas(32) std::array<u8, 4096> bytes{};
        GXBeginDisplayList(bytes.data(), bytes.size());
        MR::ddLightingOff();
        MR::ddSetVtxFormat(mode);
        GXAttrType type;
        GXGetVtxDesc(GX_VA_POS, &type);
        require(type == GX_DIRECT, "every original direct-draw format supplies position directly");
        GXGetVtxDesc(GX_VA_NRM, &type);
        require(type == ((mode & 1) ? GX_DIRECT : GX_NONE), "normal descriptor follows the original mode bit");
        GXGetVtxDesc(GX_VA_TEX0, &type);
        require(type == ((mode & 2) ? GX_DIRECT : GX_NONE), "UV descriptor follows the original mode bit");
        GXGetVtxDesc(GX_VA_CLR0, &type);
        require(type == ((mode & 4) ? GX_DIRECT : GX_NONE), "color descriptor follows the original mode bit");
        require((__gx->genMode & 15U) == ((mode & (2U | 8U)) ? 1U : 0U),
                "UV and normal-generated texture coordinates select a real texture generator");
        GXCompCnt count;
        GXCompType component;
        u8 shift;
        GXGetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, &count, &component, &shift);
        require(count == GX_POS_XYZ && component == GX_F32 && shift == 0,
                "direct position components use the original three unshifted floats");

        MR::ddLightingOn(GX_LIGHT0);
        MR::ddSetVtxFormat(mode);
        MR::ddLightingOff();
        MR::ddChangeTev();
        // Finish the state commands before inspecting only the vertex payload.
        __GXSetDirtyState();
        const auto begin = aurora::gx::fifo::detail::sDlWritePos;
        const TVec3f position{1.25F, -2.5F, 8.0F};
        const TVec3f normal{0.5F, -0.25F, 1.0F};
        const TVec2f uv{-0.75F, 0.125F};
        if (three_vectors) MR::ddSendVtxData(position, normal, uv);
        else MR::ddSendVtxData(position, uv);
        auto cursor = begin;
        for (float value : {position.x, position.y, position.z}) { check_float(bytes.data() + cursor, value); cursor += 4; }
        if (three_vectors && (mode & 1)) {
            for (float value : {normal.x, normal.y, normal.z}) { check_float(bytes.data() + cursor, value); cursor += 4; }
        }
        if (mode & 2) {
            for (float value : {uv.x, uv.y}) { check_float(bytes.data() + cursor, value); cursor += 4; }
        }
        require(aurora::gx::fifo::detail::sDlWritePos == cursor,
                "original vertex overloads emit exactly their selected position, normal and UV fields");
        require(GXEndDisplayList() >= cursor, "actual Aurora display-list recording retains all original state and payload bytes");
    }
}

int main() {
    for (u32 mode : {0U, 1U, 2U, 3U, 4U, 8U}) check_mode(mode, true);
    check_mode(0, false);
    check_mode(2, false);
    std::puts("original_direct_draw_formats=6 vertex_overload_paths=8 exact_fifo_floats=pass actual_gx_display_list=pass gpu=not_started");
}
