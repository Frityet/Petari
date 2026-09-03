#include "JSystem/J3DGraphBase/J3DTransform.hpp"
#include "JSystem/J3DGraphAnimator/J3DMtxBuffer.hpp"
#include "JSystem/JMath/JMath.hpp"

extern "C" {
void native_inverse(float* src, float* dst) { J3DPSCalcInverseTranspose(reinterpret_cast<MtxPtr>(src), reinterpret_cast<Mtx3P>(dst)); }
void native_concat(float* a, float* b, float* dst, unsigned count) { J3DPSMtxArrayConcat(reinterpret_cast<MtxPtr>(a), reinterpret_cast<MtxPtr>(b), reinterpret_cast<MtxPtr>(dst), count); }
void native_proj(float* a, float* b, float* dst) { J3DMtxProjConcat(reinterpret_cast<MtxPtr>(a), reinterpret_cast<MtxPtr>(b), reinterpret_cast<MtxPtr>(dst)); }
void native_copy(float* src, float* dst) { J3DPSMtx33CopyFrom34(reinterpret_cast<MtxPtr>(src), reinterpret_cast<Mtx3P>(dst)); }
void native_scale34(float* mtx, float* scale) { J3DScaleNrmMtx(reinterpret_cast<MtxPtr>(mtx), *reinterpret_cast<Vec*>(scale)); }
void native_scale33(float* mtx, float* scale) { J3DScaleNrmMtx33(reinterpret_cast<Mtx3P>(mtx), *reinterpret_cast<Vec*>(scale)); }
float native_sqrt(float x) { return JMAFastSqrt(x); }
void native_envelope(unsigned short count, unsigned char* mix_counts, unsigned short* indices, float* weights,
                     float* world, float* inverse, unsigned char* scales, float* output, unsigned char* output_scales) {
    J3DJointTree tree;
    tree.mWEvlpMtxNum = count;
    tree.mWEvlpMixMtxNum = mix_counts;
    tree.mWEvlpMixMtxIndex = indices;
    tree.mWEvlpMixWeight = weights;
    tree.mInvJointMtx = reinterpret_cast<Mtx*>(inverse);
    J3DMtxBuffer buffer;
    buffer.mJointTree = &tree;
    buffer.mpAnmMtx = reinterpret_cast<Mtx*>(world);
    buffer.mpScaleFlagArr = scales;
    buffer.mpWeightEvlpMtx = reinterpret_cast<Mtx*>(output);
    buffer.mpEvlpScaleFlagArr = output_scales;
    buffer.calcWeightEnvelopeMtx();
}
}
