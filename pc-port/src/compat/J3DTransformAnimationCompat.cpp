#include "JSystem/J3DGraphAnimator/J3DAnimation.hpp"

// Original J3D transform animation object layout and construction.
J3DAnmTransform::J3DAnmTransform(s16 frameMax, f32* pScaleData, s16* pRotData, f32* pTransData) : J3DAnmBase(frameMax) {
    mScaleData = pScaleData;
    mRotData = pRotData;
    mTransData = pTransData;
    field_0x18 = 0;
    field_0x1a = 0;
    field_0x1c = 0;
    field_0x1e = 0;
}
