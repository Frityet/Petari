#include "Game/Util/MathUtil.hpp"

namespace MR {
    bool normalizeOrZero(TVec2f* pVec) {
        if (isNearZero(*pVec)) {
            pVec->zero();
            return true;
        }

        normalize(pVec);
        return false;
    }

    void separateScalarAndDirection(f32* pScalar, TVec2f* pDir, const TVec2f& rVec) {
        *pScalar = rVec.length();

        if (isNearZero(rVec)) {
            pDir->zero();
        } else {
            normalize(rVec, pDir);
        }
    }

    void normalize(const TVec2f& rSrc, TVec2f* pDst) {
        *pDst = rSrc;
        normalize(pDst);
    }

    f32 normalizeAbs(f32 x, f32 min, f32 max) {
        if (x >= 0.0f) {
            return normalize(x, min, max);
        } else {
            return -normalize(-x, min, max);
        }
    }

}
