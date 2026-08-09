#include "compat/J3dSystemCompat.hpp"

#include <dolphin/mtx.h>

namespace {
    Mtx sViewMatrix = {
        {1.0F, 0.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F, 0.0F},
        {0.0F, 0.0F, 1.0F, 0.0F},
    };
}

namespace smgpc::compat {
    void load_j3d_view_matrix(const MtxPtr view_matrix) {
        PSMTXCopy(view_matrix, sViewMatrix);
    }

    const MtxPtr j3d_view_matrix() {
        return sViewMatrix;
    }
}
