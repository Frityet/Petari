#include "compat/J3dSystemCompat.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"

#include <dolphin/mtx.h>

namespace smgpc::compat {
    void load_j3d_view_matrix(const f32 (*view_matrix)[4]) {
        PSMTXCopy(view_matrix, j3dSys.mViewMtx);
    }

    const MtxPtr j3d_view_matrix() {
        return j3dSys.mViewMtx;
    }
}
