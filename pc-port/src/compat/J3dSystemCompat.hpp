#pragma once

#include <revolution/mtx.h>

namespace smgpc::compat {
    // Host-side equivalent of the view matrix retained by retail's J3DSys.
    // The PC renderer normally receives a CameraPose explicitly, while exact
    // Game draw callbacks still use MR::loadViewMtx() to restore this shared
    // J3D state at draw-list boundaries.
    void load_j3d_view_matrix(const f32 (*view_matrix)[4]);
    [[nodiscard]] const MtxPtr j3d_view_matrix();
}
