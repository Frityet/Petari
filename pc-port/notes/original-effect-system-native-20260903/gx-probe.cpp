#include "pc-port/aurora/lib/dolphin/gx/__gx.h"
#include <cassert>
#include <array>
#include <cstring>
#include <iostream>

int main() {
    const std::array<u32,6> values{0,1,8,65535,65536,0xffffffff};
    unsigned checks = 0;
    for (auto value: values) {
        *__gx = {};
        __gx->dirtyState = 0x40;
        __gx->bpSent = 1;
        GXSetMisc(GX_MT_XF_FLUSH, value);
        assert(__gx->vNum == static_cast<u16>(value));
        assert(__gx->bpSent == 0);
        assert(__gx->dirtyState == (0x40U | (static_cast<u16>(value) ? 8U : 0U)));
        GXSetMisc(GX_MT_DL_SAVE_CONTEXT,value);
        assert(__gx->dlSaveContext == (value != 0));
        GXSetMisc(GX_MT_ABORT_WAIT_COPYOUT,value);
        assert(__gx->abtWaitPECopy == (value != 0));
        const auto before = *__gx;
        GXSetMisc(GX_MT_NULL,value);
        assert(std::memcmp(&before,__gx,sizeof(before)) == 0);
        checks += 6;
    }
    for (u16 vertex_count: {u16(0),u16(8)}) {
        for (u16 pending_bp: {u16(0),u16(1)}) {
            *__gx = {};
            GXSetMisc(GX_MT_DL_SAVE_CONTEXT,0);
            alignas(32) std::array<u8,256> bytes{};
            GXBeginDisplayList(bytes.data(),bytes.size());
            __gx->vNum = vertex_count;
            __gx->bpSent = pending_bp;
            GXBegin(GX_TRIANGLES,GX_VTXFMT0,0);
            GXEnd();
            assert(__gx->bpSent == (vertex_count ? 0 : pending_bp));
            (void)GXEndDisplayList();
            ++checks;
        }
    }
    for (u32 save: {0U,1U}) {
        *__gx = {};
        GXSetMisc(GX_MT_DL_SAVE_CONTEXT,save);
        __gx->lpSize = 0x22000005;
        const auto before = __gx->lpSize;
        alignas(32) std::array<u8,256> bytes{};
        GXBeginDisplayList(bytes.data(),bytes.size());
        GXSetLineWidth(27,GX_TO_ONE);
        const auto changed = __gx->lpSize;
        assert(changed != before);
        assert(GXEndDisplayList() > 0);
        assert(__gx->lpSize == (save ? before : changed));
        ++checks;
    }
    std::cout << "gx_misc_state_checks=" << checks << " xf_count_truncation=pass dirty_state=pass pending_bp_guard=pass display_list_context=pass abort_setting_retained=pass gpu=not_started\n";
}
