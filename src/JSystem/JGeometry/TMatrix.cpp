#include "JSystem/JGeometry/TMatrix.hpp"

namespace JGeometry {
    template <>
    void TRotation3< TMatrix34< SMatrix34C< f32 > > >::getQuat(TQuat4f& rDest) const {
        f32 trace = this->mMtx[2][2] + (this->mMtx[0][0] + this->mMtx[1][1]);
        if (trace >= 0.0f) {
            f32 root = TUtil< f32 >::sqrt(trace + 1.0f);
            f32 scale = 0.5f / root;
            rDest.w = 0.5f * root;
            rDest.x = scale * (this->mMtx[2][1] - this->mMtx[1][2]);
            rDest.y = scale * (this->mMtx[0][2] - this->mMtx[2][0]);
            rDest.z = scale * (this->mMtx[1][0] - this->mMtx[0][1]);
            return;
        }

        f32 maximum = this->mMtx[0][0] >= this->mMtx[1][1] ? this->mMtx[0][0] : this->mMtx[1][1];
        maximum = maximum >= this->mMtx[2][2] ? maximum : this->mMtx[2][2];

        if (maximum == this->mMtx[0][0]) {
            f32 root = TUtil< f32 >::sqrt(1.0f + (this->mMtx[0][0] - (this->mMtx[1][1] + this->mMtx[2][2])));
            f32 scale = 0.5f / root;
            rDest.x = 0.5f * root;
            rDest.y = scale * (this->mMtx[0][1] + this->mMtx[1][0]);
            rDest.z = scale * (this->mMtx[2][0] + this->mMtx[0][2]);
            rDest.w = scale * (this->mMtx[2][1] - this->mMtx[1][2]);
        } else if (maximum == this->mMtx[1][1]) {
            f32 root = TUtil< f32 >::sqrt(1.0f + (this->mMtx[1][1] - (this->mMtx[2][2] + this->mMtx[0][0])));
            f32 scale = 0.5f / root;
            rDest.y = 0.5f * root;
            rDest.z = scale * (this->mMtx[1][2] + this->mMtx[2][1]);
            rDest.x = scale * (this->mMtx[0][1] + this->mMtx[1][0]);
            rDest.w = scale * (this->mMtx[0][2] - this->mMtx[2][0]);
        } else {
            f32 root = TUtil< f32 >::sqrt(1.0f + (this->mMtx[2][2] - (this->mMtx[0][0] + this->mMtx[1][1])));
            f32 scale = 0.5f / root;
            rDest.z = 0.5f * root;
            rDest.x = scale * (this->mMtx[2][0] + this->mMtx[0][2]);
            rDest.y = scale * (this->mMtx[1][2] + this->mMtx[2][1]);
            rDest.w = scale * (this->mMtx[1][0] - this->mMtx[0][1]);
        }
    }
}  // namespace JGeometry

namespace JGeometry {
    template <>
    void TRotation3< TMatrix34< SMatrix34C< f32 > > >::getScale(TVec3f& rDest) const {
        rDest.x = TUtil< f32 >::sqrt(this->mMtx[2][0] * this->mMtx[2][0] +
                                    (this->mMtx[0][0] * this->mMtx[0][0] + this->mMtx[1][0] * this->mMtx[1][0]));
        rDest.y = TUtil< f32 >::sqrt(this->mMtx[2][1] * this->mMtx[2][1] +
                                    (this->mMtx[0][1] * this->mMtx[0][1] + this->mMtx[1][1] * this->mMtx[1][1]));
        rDest.z = TUtil< f32 >::sqrt(this->mMtx[2][2] * this->mMtx[2][2] +
                                    (this->mMtx[0][2] * this->mMtx[0][2] + this->mMtx[1][2] * this->mMtx[1][2]));
    }
}  // namespace JGeometry
