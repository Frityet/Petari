#pragma once

#include <revolution/mtx.h>

namespace smgpc::compat {
    // Original model view calculation and Game draw callbacks share J3DSys.
    // MR::loadViewMtx restores that actual state at draw-list boundaries.
    void load_j3d_view_matrix(const f32 (*view_matrix)[4]);
    [[nodiscard]] const MtxPtr j3d_view_matrix();
}
