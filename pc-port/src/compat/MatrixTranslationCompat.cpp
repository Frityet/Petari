#include "Game/Util/MtxUtil.hpp"

// Original translation-column setters; no basis or scale changes.
// Correspondence: notes/xanime-core-matrix-calculation-20260903/helpers-math.md.

namespace MR {
    void setMtxTrans(MtxPtr mtx, f32 x, f32 y, f32 z) {
        mtx[0][3] = x;
        mtx[1][3] = y;
        mtx[2][3] = z;
    }

    void setMtxTrans(MtxPtr mtx, const TVec3f& rVec) {
        MR::setMtxTrans(mtx, rVec.x, rVec.y, rVec.z);
    }
}  // namespace MR
