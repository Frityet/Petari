#include "Game/System/ShapePacketUserData.hpp"
#include "Game/Util/MathUtil.hpp"
#include "JSystem/J3DGraphBase/J3DFifo.hpp"
#include "JSystem/J3DGraphBase/J3DPacket.hpp"
#include "JSystem/J3DGraphBase/J3DShape.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include "JSystem/JAudio2/JASHeapCtrl.hpp"
#include "JSystem/JKernel/JKRSolidHeap.hpp"
#include "JSystem/JKernel/JKRThread.hpp"
#include "JSystem/JParticle/JPABaseShape.hpp"
#include "JSystem/JParticle/JPAEmitter.hpp"
#include "JSystem/JParticle/JPAEmitterManager.hpp"
#include "JSystem/JParticle/JPAFieldBlock.hpp"
#include "JSystem/JParticle/JPAParticle.hpp"
#include "JSystem/JMath/JMATrigonometric.hpp"
#include <revolution/gx/GXVert.h>
#include <JSystem/JAudio2/JASAudioThread.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>

void JUTTexture::captureDolTexture(void* image, int width, int height, int x, int y, bool mipmap, GXTexFmt format) {
    if (mipmap) {
        GXSetTexCopySrc(x, y, width * 2, height * 2);
    } else {
        GXSetTexCopySrc(x, y, width, height);
    }
    GXSetTexCopyDst(width, height, format, mipmap);
    GXCopyTex(image, GX_FALSE);
    GXPixModeSync();
}

JASAudioThread::JASAudioThread(int stackSize, int msgCount, u32 threadPriority)
    : JKRThread(JASDram, threadPriority, msgCount, stackSize), JASGlobalInstance< JASAudioThread >(true) {
    sbPauseFlag = false;
    OSInitThreadQueue(&sThreadQueue);
}

void J3DShapeMtx::loadMtxIndx_PNGP(int slot, u16 index) const {
    J3DFifoLoadIndx(0x20, index, 0xB000 | static_cast<u16>(slot * 12));
    J3DFifoLoadNrmMtxIndx3x3(index, slot * 3);
    ShapePacketUserData* userData = MR::getJ3DShapePacketUserData(j3dSys.getShapePacket());
    if (userData != nullptr) {
        userData->loadTexMtx(j3dSys.getShapePacket()->getShape()->getMaterial(), slot, index);
    }
}

namespace {
void noLoadPrj(JPAEmitterWorkData const* work, const Mtx srt) {
    /* empty function */
}

void loadPrj(JPAEmitterWorkData const* work, const Mtx srt) {
    Mtx mtx;
    PSMTXConcat(work->mPrjMtx, srt, mtx);
    GXLoadTexMtxImm(mtx, GX_TEXMTX0, GX_MTX3x4);
}

void loadPrjAnm(JPAEmitterWorkData const* work, const Mtx srt) {
    JPABaseShape* shape = work->mpRes->getBsp();
    f32 dVar16 = work->mpEmtr->getAge();
    f32 dVar15 = 0.5f * (1.0f + shape->getTilingS());
    f32 dVar14 = 0.5f * (1.0f + shape->getTilingT());
    f32 dVar11 = (dVar16 * shape->getIncTransX()) + shape->getInitTransX();
    f32 dVar10 = (dVar16 * shape->getIncTransY()) + shape->getInitTransY();
    f32 dVar13 = (dVar16 * shape->getIncScaleX()) + shape->getInitScaleX();
    f32 dVar12 = (dVar16 * shape->getIncScaleY()) + shape->getInitScaleY();
    s32 local_c0 = (dVar16 * shape->getIncRot()) + shape->getInitRot();
    f32 dVar8 = JMASSin(local_c0);
    f32 dVar9 = JMASCos(local_c0);
    Mtx local_108;
    local_108[0][0] = dVar13 * dVar9;
    local_108[0][1] = -dVar13 * dVar8;
    local_108[0][2] = (dVar15 + (dVar13 * ((dVar8 * (dVar14 + dVar10)) - (dVar9 * (dVar15 + dVar11)))));
    local_108[0][3] = 0.0f;
    local_108[1][0] = dVar12 * dVar8;
    local_108[1][1] = dVar12 * dVar9;
    local_108[1][2] = (dVar14 + (-dVar12 * ((dVar8 * (dVar15 + dVar11)) + (dVar9 * (dVar14 + dVar10)))));
    local_108[1][3] = 0.0f;
    local_108[2][0] = 0.0f;
    local_108[2][1] = 0.0f;
    local_108[2][2] = 1.0f;
    local_108[2][3] = 0.0f;
    PSMTXConcat(local_108, work->mPrjMtx, local_108);
    PSMTXConcat(local_108, srt, local_108);
    GXLoadTexMtxImm(local_108, 0x1e, GX_MTX3x4);
}

void dirTypeVel(JPAEmitterWorkData const* work, JPABaseParticle const* param_1, JGeometry::TVec3< f32 >* param_2) {
    param_1->getVelVec(*param_2);
}

void dirTypePos(JPAEmitterWorkData const* work, JPABaseParticle const* param_1, JGeometry::TVec3< f32 >* param_2) {
    param_1->getLocalPosition(*param_2);
}

void dirTypePosInv(JPAEmitterWorkData const* work, JPABaseParticle const* param_1, JGeometry::TVec3< f32 >* param_2) {
    param_1->getLocalPosition(*param_2);
    param_2->negate();
}

void dirTypeEmtrDir(JPAEmitterWorkData const* work, JPABaseParticle const* param_1, JGeometry::TVec3< f32 >* param_2) {
    param_2->set(work->mGlobalEmtrDir);
}

void dirTypePrevPtcl(JPAEmitterWorkData const* work, JPABaseParticle const* param_1, JGeometry::TVec3< f32 >* param_2) {
    JGeometry::TVec3< f32 > aTStack_24;
    param_1->getGlobalPosition(aTStack_24);
    JPANode< JPABaseParticle >* end = work->mpAlivePtcl->getEnd();
    JPANode< JPABaseParticle >* prev = work->mpCurNode->getPrev();

    if (end != prev) {
        JPABaseParticle* particle = work->mpCurNode->getPrev()->getObject();
        particle->getGlobalPosition(*param_2);
    } else {
        work->mpEmtr->calcEmitterGlobalPosition(param_2);
    }
    param_2->sub(aTStack_24);
}

void rotTypeY(f32 param_0, f32 param_1, Mtx& param_2) {
    param_2[0][0] = param_1;
    param_2[0][1] = 0.0f;
    param_2[0][2] = -param_0;
    param_2[0][3] = 0.0f;
    param_2[1][0] = 0.0f;
    param_2[1][1] = 1.0f;
    param_2[1][2] = 0.0f;
    param_2[1][3] = 0.0f;
    param_2[2][0] = param_0;
    param_2[2][1] = 0.0f;
    param_2[2][2] = param_1;
    param_2[2][3] = 0.0f;
}

void rotTypeX(f32 param_0, f32 param_1, Mtx& param_2) {
    param_2[0][0] = 1.0f;
    param_2[0][1] = 0.0f;
    param_2[0][2] = 0.0f;
    param_2[0][3] = 0.0f;
    param_2[1][0] = 0.0f;
    param_2[1][1] = param_1;
    param_2[1][2] = -param_0;
    param_2[1][3] = 0.0f;
    param_2[2][0] = 0.0f;
    param_2[2][1] = param_0;
    param_2[2][2] = param_1;
    param_2[2][3] = 0.0f;
}

void rotTypeZ(f32 param_0, f32 param_1, Mtx& param_2) {
    param_2[0][0] = param_1;
    param_2[0][1] = -param_0;
    param_2[0][2] = 0.0f;
    param_2[0][3] = 0.0f;
    param_2[1][0] = param_0;
    param_2[1][1] = param_1;
    param_2[1][2] = 0.0f;
    param_2[1][3] = 0.0f;
    param_2[2][0] = 0.0f;
    param_2[2][1] = 0.0f;
    param_2[2][2] = 1.0f;
    param_2[2][3] = 0.0f;
}

void rotTypeXYZ(f32 sinRot, f32 cosRot, Mtx& mtx) {
    f32 oneMinusCos = 0.33333298563957214f * (1.0f - cosRot);
    f32 sinScale = 0.5773500204086304f * sinRot;
    f32 diagonal = oneMinusCos + cosRot;
    f32 plus = oneMinusCos + sinScale;
    f32 minus = oneMinusCos - sinScale;
    mtx[0][3] = mtx[1][3] = mtx[2][3] = 0.0f;
    mtx[0][0] = diagonal;
    mtx[0][1] = minus;
    mtx[0][2] = plus;
    mtx[1][0] = plus;
    mtx[1][1] = diagonal;
    mtx[1][2] = minus;
    mtx[2][0] = minus;
    mtx[2][1] = plus;
    mtx[2][2] = diagonal;
}

void basePlaneTypeXY(MtxPtr param_0, f32 param_1, f32 param_2) {
    param_0[0][0] *= param_1;
    param_0[1][0] *= param_1;
    param_0[2][0] *= param_1;
    param_0[0][1] *= param_2;
    param_0[1][1] *= param_2;
    param_0[2][1] *= param_2;
}

void basePlaneTypeXZ(MtxPtr param_0, f32 param_1, f32 param_2) {
    param_0[0][0] *= param_1;
    param_0[1][0] *= param_1;
    param_0[2][0] *= param_1;
    param_0[0][2] *= param_2;
    param_0[1][2] *= param_2;
    param_0[2][2] *= param_2;
}

void basePlaneTypeX(MtxPtr param_0, f32 param_1, f32 param_2) {
    param_0[0][0] *= param_1;
    param_0[1][0] *= param_1;
    param_0[2][0] *= param_1;
    param_0[0][1] *= param_2;
    param_0[1][1] *= param_2;
    param_0[2][1] *= param_2;
    param_0[0][2] *= param_1;
    param_0[1][2] *= param_1;
    param_0[2][2] *= param_1;
}

JPANode< JPABaseParticle >* getNext(JPANode< JPABaseParticle >* pNode) { return pNode->getNext(); }
JPANode< JPABaseParticle >* getPrev(JPANode< JPABaseParticle >* pNode) { return pNode->getPrev(); }

static u8 jpa_dl[32] = {
    0x80, 0x00, 0x04, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static u8 jpa_dl_x[32] = {
    0x80, 0x00, 0x08, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x48, 0x00, 0x49, 0x01, 0x4A,
    0x02, 0x4B, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

typedef void (*projectionFunc)(JPAEmitterWorkData const*, const Mtx);
typedef void (*dirTypeFunc)(JPAEmitterWorkData const*, JPABaseParticle const*, TVec3f*);
typedef void (*rotTypeFunc)(f32, f32, Mtx&);
typedef void (*planeFunc)(MtxPtr, f32, f32);
u8* p_dl[2] = {jpa_dl, jpa_dl_x};
projectionFunc p_prj[3] = {noLoadPrj, loadPrj, loadPrjAnm};
dirTypeFunc p_direction[5] = {dirTypeVel, dirTypePos, dirTypePosInv, dirTypeEmtrDir, dirTypePrevPtcl};
rotTypeFunc p_rot[5] = {rotTypeY, rotTypeX, rotTypeZ, rotTypeXYZ, rotTypeY};
planeFunc p_plane[3] = {basePlaneTypeXY, basePlaneTypeXZ, basePlaneTypeX};
}  // namespace

void JPABaseEmitter::init(JPAEmitterManager* pManager, JPAResource* pResource) {
    mpEmtrMgr = pManager;
    mpRes = pResource;
    mpRes->getDyn()->getEmitterScl(&mLocalScl);
    mpRes->getDyn()->getEmitterTrs(&mLocalTrs);
    mpRes->getDyn()->getEmitterDir(&mLocalDir);
    if (!MR::isNearZero(mLocalDir, 0.001f)) {
        MR::normalize(&mLocalDir);
    }
    mpRes->getDyn()->getEmitterRot(&mLocalRot);
    mMaxFrame = mpRes->getDyn()->getMaxFrame();
    mLifeTime = mpRes->getDyn()->getLifetime();
    mVolumeSize = mpRes->getDyn()->getVolumeSize();
    mRate = mpRes->getDyn()->getRate();
    mRateStep = mpRes->getDyn()->getRateStep();
    mVolumeSweep = mpRes->getDyn()->getVolumeSweep();
    mVolumeMinRad = mpRes->getDyn()->getVolumeMinRad();
    mAwayFromCenterSpeed = mpRes->getDyn()->getInitVelOmni();
    mAwayFromAxisSpeed = mpRes->getDyn()->getInitVelAxis();
    mDirSpeed = mpRes->getDyn()->getInitVelDir();
    mSpread = mpRes->getDyn()->getInitVelDirSp();
    mRndmDirSpeed = mpRes->getDyn()->getInitVelRndm();
    mAirResist = mpRes->getDyn()->getAirRes();
    mRndm.set_seed(mpEmtrMgr->pWd->mRndm.get_rndm_u());
    PSMTXIdentity(mGlobalRot);
    mGlobalScl.set(1.0f);
    mGlobalTrs.zero();
    mGlobalPScl.set(1.0f, 1.0f);
    mGlobalPrmClr.r = mGlobalPrmClr.g = mGlobalPrmClr.b = mGlobalPrmClr.a =
        mGlobalEnvClr.r = mGlobalEnvClr.g = mGlobalEnvClr.b = mGlobalEnvClr.a = 255;
    pResource->getBsp()->getPrmClr(&mPrmClr);
    pResource->getBsp()->getEnvClr(&mEnvClr);
    mpUserWork = 0;
    mScaleOut = 1.0f;
    mEmitCount = 0.0f;
    mStatus = JPAEmtrStts_FirstEmit | JPAEmtrStts_RateStepEmit;
    mDrawTimes = 1;
    mTick = 0;
    mWaitTime = 0;
    mRateStepTimer = 0;
    mTexAnmIdx = 0;
}

void JPAFieldAir::prepare(JPAEmitterWorkData* pWork, JPAFieldBlock* pBlock) {
    TVec3f dir(pBlock->getDir());
    MR::normalizeOrZero(&dir);
    if (pBlock->checkStatus(2)) {
        mAccel.scale(pBlock->getMag(), dir);
    } else {
        PSMTXMultVecSR(pWork->mRotationMtx, &dir, &mAccel);
        mAccel.scale(pBlock->getMag());
    }
}

void JPAFieldVortex::prepare(JPAEmitterWorkData* pWork, JPAFieldBlock* pBlock) {
    PSMTXMultVecSR(pWork->mGlobalRot, &pBlock->getDir(), &field_0x10);
    MR::normalizeOrZero(&field_0x10);
    field_0x1c = pBlock->getPos().z * pBlock->getPos().z;
    field_0x20 = 1.0f / field_0x1c;
}

void JPAFieldVortex::calc(JPAEmitterWorkData* pWork, JPAFieldBlock* pBlock, JPABaseParticle* pParticle) {
    TVec3f dir;
    dir.scale(field_0x10.dot(pParticle->mLocalPosition), field_0x10);
    dir.sub(pParticle->mLocalPosition, dir);
    f32 distSquared = dir.squared();
    f32 mag;
    if (distSquared > field_0x1c) {
        mag = pBlock->getMagRndm();
    } else {
        f32 ratio = distSquared * field_0x20;
        mag = (1.0f - ratio) * pBlock->getMag() + ratio * pBlock->getMagRndm();
    }
    MR::normalizeOrZero(&dir);
    PSVECCrossProduct(&dir, &field_0x10, &mAccel);
    mAccel.scale(mag);
    calcAffect(pBlock, pParticle);
}

void JPAFieldConvection::prepare(JPAEmitterWorkData* pWork, JPAFieldBlock* pBlock) {
    TVec3f side, front;
    PSVECCrossProduct(&pBlock->getPos(), &pBlock->getDir(), &side);
    PSVECCrossProduct(&pBlock->getDir(), &side, &front);
    PSMTXMultVecSR(pWork->mGlobalRot, &front, &field_0x10);
    PSMTXMultVecSR(pWork->mGlobalRot, &pBlock->getDir(), &field_0x1c);
    PSMTXMultVecSR(pWork->mGlobalRot, &side, &field_0x28);
    MR::normalizeOrZero(&field_0x10);
    MR::normalizeOrZero(&field_0x1c);
    MR::normalizeOrZero(&field_0x28);
}

void JPAFieldSpin::prepare(JPAEmitterWorkData* pWork, JPAFieldBlock* pBlock) {
    TVec3f dir;
    Mtx mtx;
    PSMTXMultVecSR(pWork->mGlobalRot, &pBlock->getDir(), &dir);
    MR::normalizeOrZero(&dir);
    PSMTXRotAxisRad(mtx, &dir, pBlock->getMag());
    field_0x10.set(mtx[0][0], mtx[1][0], mtx[2][0]);
    field_0x1c.set(mtx[0][1], mtx[1][1], mtx[2][1]);
    field_0x28.set(mtx[0][2], mtx[1][2], mtx[2][2]);
}

void JPADrawDirection(JPAEmitterWorkData* pWork, JPABaseParticle* pParticle) {
    if (pParticle->checkStatus(8) != 0)
        return;
    TVec3f dir, side;
    p_direction[pWork->mDirType](pWork, pParticle, &dir);
    if (MR::isNearZero(dir, 0.001f))
        return;
    MR::normalize(&dir);
    PSVECCrossProduct(&pParticle->mBaseAxis, &dir, &side);
    if (MR::isNearZero(side, 0.001f))
        return;
    MR::normalize(&side);
    PSVECCrossProduct(&dir, &side, &pParticle->mBaseAxis);
    MR::normalize(&pParticle->mBaseAxis);
    f32 scaleX = pWork->mGlobalPtclScl.x * pParticle->mParticleScaleX;
    f32 scaleY = pWork->mGlobalPtclScl.y * pParticle->mParticleScaleY;
    Mtx mtx;
    mtx[0][0] = pParticle->mBaseAxis.x;
    mtx[0][1] = dir.x;
    mtx[0][2] = side.x;
    mtx[0][3] = pParticle->mPosition.x;
    mtx[1][0] = pParticle->mBaseAxis.y;
    mtx[1][1] = dir.y;
    mtx[1][2] = side.y;
    mtx[1][3] = pParticle->mPosition.y;
    mtx[2][0] = pParticle->mBaseAxis.z;
    mtx[2][1] = dir.z;
    mtx[2][2] = side.z;
    mtx[2][3] = pParticle->mPosition.z;
    p_plane[pWork->mPlaneType](mtx, scaleX, scaleY);
    PSMTXConcat(pWork->mPosCamMtx, mtx, mtx);
    GXLoadPosMtxImm(mtx, GX_PNMTX0);
    p_prj[pWork->mPrjType](pWork, mtx);
    GXCallDisplayList(p_dl[pWork->mDLType], sizeof(jpa_dl));
}

void JPADrawRotDirection(JPAEmitterWorkData* pWork, JPABaseParticle* pParticle) {
    if (pParticle->checkStatus(8) != 0)
        return;
    f32 sinRot = JMASSin(pParticle->mRotateAngle);
    f32 cosRot = JMASCos(pParticle->mRotateAngle);
    TVec3f dir, side;
    p_direction[pWork->mDirType](pWork, pParticle, &dir);
    if (MR::isNearZero(dir, 0.001f))
        return;
    MR::normalize(&dir);
    PSVECCrossProduct(&pParticle->mBaseAxis, &dir, &side);
    if (MR::isNearZero(side, 0.001f))
        return;
    MR::normalize(&side);
    PSVECCrossProduct(&dir, &side, &pParticle->mBaseAxis);
    MR::normalize(&pParticle->mBaseAxis);
    f32 scaleX = pWork->mGlobalPtclScl.x * pParticle->mParticleScaleX;
    f32 scaleY = pWork->mGlobalPtclScl.y * pParticle->mParticleScaleY;
    Mtx base, mtx;
    p_rot[pWork->mRotType](sinRot, cosRot, mtx);
    p_plane[pWork->mPlaneType](mtx, scaleX, scaleY);
    base[0][0] = pParticle->mBaseAxis.x;
    base[0][1] = dir.x;
    base[0][2] = side.x;
    base[0][3] = pParticle->mPosition.x;
    base[1][0] = pParticle->mBaseAxis.y;
    base[1][1] = dir.y;
    base[1][2] = side.y;
    base[1][3] = pParticle->mPosition.y;
    base[2][0] = pParticle->mBaseAxis.z;
    base[2][1] = dir.z;
    base[2][2] = side.z;
    base[2][3] = pParticle->mPosition.z;
    PSMTXConcat(base, mtx, mtx);
    PSMTXConcat(pWork->mPosCamMtx, mtx, base);
    GXLoadPosMtxImm(base, GX_PNMTX0);
    p_prj[pWork->mPrjType](pWork, base);
    GXCallDisplayList(p_dl[pWork->mDLType], sizeof(jpa_dl));
}

void JPADrawDBillboard(JPAEmitterWorkData* pWork, JPABaseParticle* pParticle) {
    if (pParticle->checkStatus(8) != 0)
        return;
    TVec3f dir;
    p_direction[pWork->mDirType](pWork, pParticle, &dir);
    TVec3f camDir(pWork->mPosCamMtx[2][0], pWork->mPosCamMtx[2][1], pWork->mPosCamMtx[2][2]);
    PSVECCrossProduct(&dir, &camDir, &dir);
    if (MR::isNearZero(dir, 0.001f))
        return;
    MR::normalize(&dir);
    PSMTXMultVecSR(pWork->mPosCamMtx, &dir, &dir);
    TVec3f pos;
    PSMTXMultVec(pWork->mPosCamMtx, &pParticle->mPosition, &pos);
    f32 scaleX = pWork->mGlobalPtclScl.x * pParticle->mParticleScaleX;
    f32 scaleY = pWork->mGlobalPtclScl.y * pParticle->mParticleScaleY;
    Mtx mtx;
    mtx[0][0] = dir.x * scaleX;
    mtx[0][1] = -dir.y * scaleY;
    mtx[0][3] = pos.x;
    mtx[1][0] = dir.y * scaleX;
    mtx[1][1] = dir.x * scaleY;
    mtx[1][3] = pos.y;
    mtx[2][2] = 1.0f;
    mtx[2][3] = pos.z;
    mtx[2][1] = mtx[2][0] = mtx[1][2] = mtx[0][2] = 0.0f;
    GXLoadPosMtxImm(mtx, GX_PNMTX0);
    p_prj[pWork->mPrjType](pWork, mtx);
    GXCallDisplayList(jpa_dl, sizeof(jpa_dl));
}

void JPADrawYBillboard(JPAEmitterWorkData* pWork, JPABaseParticle* pParticle) {
    if (pParticle->checkStatus(8) != 0)
        return;
    TVec3f dir(0.0f, pWork->mPosCamMtx[1][1], pWork->mPosCamMtx[2][1]);
    if (MR::isNearZero(dir, 0.001f))
        return;
    TVec3f pos;
    PSMTXMultVec(pWork->mPosCamMtx, &pParticle->mPosition, &pos);
    f32 scaleX = pWork->mGlobalPtclScl.x * pParticle->mParticleScaleX;
    f32 scaleY = pWork->mGlobalPtclScl.y * pParticle->mParticleScaleY;
    Mtx mtx;
    mtx[0][0] = scaleX;
    mtx[0][3] = pos.x;
    mtx[1][1] = pWork->mYBBCamMtx[1][1] * scaleY;
    mtx[1][2] = pWork->mYBBCamMtx[1][2];
    mtx[1][3] = pos.y;
    mtx[2][1] = pWork->mYBBCamMtx[2][1] * scaleY;
    mtx[2][2] = pWork->mYBBCamMtx[2][2];
    mtx[2][3] = pos.z;
    mtx[2][0] = mtx[1][0] = mtx[0][2] = mtx[0][1] = 0.0f;
    GXLoadPosMtxImm(mtx, GX_PNMTX0);
    p_prj[pWork->mPrjType](pWork, mtx);
    GXCallDisplayList(jpa_dl, sizeof(jpa_dl));
}

void JPADrawRotYBillboard(JPAEmitterWorkData* pWork, JPABaseParticle* pParticle) {
    if (pParticle->checkStatus(8) != 0)
        return;
    TVec3f dir(0.0f, pWork->mPosCamMtx[1][1], pWork->mPosCamMtx[2][1]);
    if (MR::isNearZero(dir, 0.001f))
        return;
    TVec3f pos;
    PSMTXMultVec(pWork->mPosCamMtx, &pParticle->mPosition, &pos);
    f32 sinRot = JMASSin(pParticle->mRotateAngle);
    f32 cosRot = JMASCos(pParticle->mRotateAngle);
    f32 scaleX = pWork->mGlobalPtclScl.x * pParticle->mParticleScaleX;
    f32 scaleY = pWork->mGlobalPtclScl.y * pParticle->mParticleScaleY;
    f32 sinX = sinRot * scaleX;
    f32 cosY = cosRot * scaleY;
    f32 camY = pWork->mYBBCamMtx[1][1];
    f32 camZ = pWork->mYBBCamMtx[2][1];
    Mtx mtx;
    mtx[0][0] = cosRot * scaleX;
    mtx[0][1] = -sinRot * scaleY;
    mtx[0][2] = 0.0f;
    mtx[0][3] = pos.x;
    mtx[1][0] = sinX * camY;
    mtx[1][1] = cosY * camY;
    mtx[1][2] = -camZ;
    mtx[1][3] = pos.y;
    mtx[2][0] = sinX * camZ;
    mtx[2][1] = cosY * camZ;
    mtx[2][2] = camY;
    mtx[2][3] = pos.z;
    GXLoadPosMtxImm(mtx, GX_PNMTX0);
    p_prj[pWork->mPrjType](pWork, mtx);
    GXCallDisplayList(jpa_dl, sizeof(jpa_dl));
}

void JPAEmitterManager::calcYBBCam() {
    TVec3f dir(0.0f, pWd->mPosCamMtx[1][1], pWd->mPosCamMtx[2][1]);
    if (!MR::isNearZero(dir, 0.001f)) {
        MR::normalize(&dir);
        pWd->mYBBCamMtx[0][0] = 1.0f;
        pWd->mYBBCamMtx[0][1] = 0.0f;
        pWd->mYBBCamMtx[0][2] = 0.0f;
        pWd->mYBBCamMtx[0][3] = pWd->mPosCamMtx[0][3];
        pWd->mYBBCamMtx[1][0] = 0.0f;
        pWd->mYBBCamMtx[1][1] = dir.y;
        pWd->mYBBCamMtx[1][2] = -dir.z;
        pWd->mYBBCamMtx[1][3] = pWd->mPosCamMtx[1][3];
        pWd->mYBBCamMtx[2][0] = 0.0f;
        pWd->mYBBCamMtx[2][1] = dir.z;
        pWd->mYBBCamMtx[2][2] = dir.y;
        pWd->mYBBCamMtx[2][3] = pWd->mPosCamMtx[2][3];
    }
}

void JPADrawLine(JPAEmitterWorkData* pWork, JPABaseParticle* pParticle) {
    if (pParticle->checkStatus(8) != 0)
        return;
    TVec3f pos(pParticle->mPosition);
    TVec3f vel;
    pParticle->getVelVec(vel);
    if (MR::isNearZero(vel, 0.001f))
        return;
    vel.setLength(pWork->mGlobalPtclScl.y * (25.0f * pParticle->mParticleScaleY));
    vel.sub(pos, vel);
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXBegin(GX_LINES, GX_VTXFMT1, 2);
    GXPosition3f32(pos.x, pos.y, pos.z);
    GXTexCoord2f32(0.0f, 0.0f);
    GXPosition3f32(vel.x, vel.y, vel.z);
    GXTexCoord2f32(0.0f, 1.0f);
    GXEnd();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX8);
    GXSetVtxDesc(GX_VA_TEX0, GX_INDEX8);
}

void JPADrawStripe(JPAEmitterWorkData* pWork) {
    u32 count = pWork->mpAlivePtcl->getNum();
    JPABaseShape* pShape = pWork->mpRes->getBsp();
    if (count < 2)
        return;
    f32 step = 1.0f / (count - 1.0f);
    f32 texV = 0.0f;
    f32 leftX = (1.0f + pWork->mPivot.x) * (25.0f * pWork->mGlobalPtclScl.x);
    f32 rightX = (1.0f - pWork->mPivot.x) * (25.0f * pWork->mGlobalPtclScl.x);
    JPANode< JPABaseParticle >* node;
    JPANode< JPABaseParticle >* (*nextNode)(JPANode< JPABaseParticle >*);
    if (pShape->isDrawFwdAhead()) {
        texV = 1.0f;
        step = -step;
        node = pWork->mpAlivePtcl->getLast();
        nextNode = getPrev;
    } else {
        node = pWork->mpAlivePtcl->getFirst();
        nextNode = getNext;
    }
    GXLoadPosMtxImm(pWork->mPosCamMtx, GX_PNMTX0);
    p_prj[pWork->mPrjType](pWork, pWork->mPosCamMtx);
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT1, count * 2);
    for (; node != pWork->mpAlivePtcl->getEnd(); node = nextNode(node), texV += step) {
        pWork->mpCurNode = node;
        JPABaseParticle* pParticle = node->getObject();
        TVec3f pos(pParticle->mPosition);
        f32 sinRot = JMASSin(pParticle->mRotateAngle);
        f32 cosRot = JMASCos(pParticle->mRotateAngle);
        TVec3f edges[2];
        edges[0].set(-pParticle->mParticleScaleX * leftX, 0.0f, 0.0f);
        edges[0].set(edges[0].x * cosRot, 0.0f, edges[0].x * sinRot);
        edges[1].set(pParticle->mParticleScaleX * rightX, 0.0f, 0.0f);
        edges[1].set(edges[1].x * cosRot, 0.0f, edges[1].x * sinRot);
        TVec3f dir, side;
        p_direction[pWork->mDirType](pWork, pParticle, &dir);
        if (MR::isNearZero(dir, 0.001f))
            dir.set(0.0f, 1.0f, 0.0f);
        else
            MR::normalize(&dir);
        PSVECCrossProduct(&pParticle->mBaseAxis, &dir, &side);
        if (MR::isNearZero(side, 0.001f))
            side.set(1.0f, 0.0f, 0.0f);
        else
            MR::normalize(&side);
        PSVECCrossProduct(&dir, &side, &pParticle->mBaseAxis);
        MR::normalize(&pParticle->mBaseAxis);
        Mtx mtx;
        mtx[0][0] = side.x;
        mtx[0][1] = dir.x;
        mtx[0][2] = pParticle->mBaseAxis.x;
        mtx[0][3] = 0.0f;
        mtx[1][0] = side.y;
        mtx[1][1] = dir.y;
        mtx[1][2] = pParticle->mBaseAxis.y;
        mtx[1][3] = 0.0f;
        mtx[2][0] = side.z;
        mtx[2][1] = dir.z;
        mtx[2][2] = pParticle->mBaseAxis.z;
        mtx[2][3] = 0.0f;
        PSMTXMultVecArraySR(mtx, edges, edges, 2);
        GXPosition3f32(edges[0].x + pos.x, edges[0].y + pos.y, edges[0].z + pos.z);
        GXTexCoord2f32(0.0f, texV);
        GXPosition3f32(edges[1].x + pos.x, edges[1].y + pos.y, edges[1].z + pos.z);
        GXTexCoord2f32(1.0f, texV);
    }
    GXEnd();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX8);
    GXSetVtxDesc(GX_VA_TEX0, GX_INDEX8);
}

void JPADrawStripeX(JPAEmitterWorkData* pWork) {
    u32 count = pWork->mpAlivePtcl->getNum();
    JPABaseShape* pShape = pWork->mpRes->getBsp();
    if (count < 2)
        return;
    f32 step = 1.0f / (count - 1.0f);
    f32 texV = 0.0f;
    f32 leftX = (1.0f + pWork->mPivot.x) * (25.0f * pWork->mGlobalPtclScl.x);
    f32 rightX = (1.0f - pWork->mPivot.x) * (25.0f * pWork->mGlobalPtclScl.x);
    f32 leftY = (1.0f + pWork->mPivot.y) * (25.0f * pWork->mGlobalPtclScl.y);
    f32 rightY = (1.0f - pWork->mPivot.y) * (25.0f * pWork->mGlobalPtclScl.y);
    JPANode< JPABaseParticle >* node;
    JPANode< JPABaseParticle >* (*nextNode)(JPANode< JPABaseParticle >*);
    if (pShape->isDrawFwdAhead()) {
        texV = 1.0f;
        step = -step;
        node = pWork->mpAlivePtcl->getLast();
        nextNode = getPrev;
    } else {
        node = pWork->mpAlivePtcl->getFirst();
        nextNode = getNext;
    }
    JPANode< JPABaseParticle >* first = node;
    f32 firstTexV = texV;
    GXLoadPosMtxImm(pWork->mPosCamMtx, GX_PNMTX0);
    p_prj[pWork->mPrjType](pWork, pWork->mPosCamMtx);
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT1, count * 2);
    for (; node != pWork->mpAlivePtcl->getEnd(); node = nextNode(node), texV += step) {
        pWork->mpCurNode = node;
        JPABaseParticle* pParticle = node->getObject();
        TVec3f pos(pParticle->mPosition);
        f32 sinRot = JMASSin(pParticle->mRotateAngle);
        f32 cosRot = JMASCos(pParticle->mRotateAngle);
        TVec3f edges[2];
        edges[0].set(-pParticle->mParticleScaleX * leftX, 0.0f, 0.0f);
        edges[0].set(edges[0].x * cosRot, 0.0f, edges[0].x * sinRot);
        edges[1].set(pParticle->mParticleScaleX * rightX, 0.0f, 0.0f);
        edges[1].set(edges[1].x * cosRot, 0.0f, edges[1].x * sinRot);
        TVec3f dir, side;
        p_direction[pWork->mDirType](pWork, pParticle, &dir);
        if (MR::isNearZero(dir, 0.001f))
            dir.set(0.0f, 1.0f, 0.0f);
        else
            MR::normalize(&dir);
        PSVECCrossProduct(&pParticle->mBaseAxis, &dir, &side);
        if (MR::isNearZero(side, 0.001f))
            side.set(1.0f, 0.0f, 0.0f);
        else
            MR::normalize(&side);
        if (MR::isSameDirection(dir, side, 0.01f))
            side.set(0.0f, 1.0f, 0.0f);
        PSVECCrossProduct(&dir, &side, &pParticle->mBaseAxis);
        MR::normalize(&pParticle->mBaseAxis);
        Mtx mtx;
        mtx[0][0] = side.x;
        mtx[0][1] = dir.x;
        mtx[0][2] = pParticle->mBaseAxis.x;
        mtx[0][3] = 0.0f;
        mtx[1][0] = side.y;
        mtx[1][1] = dir.y;
        mtx[1][2] = pParticle->mBaseAxis.y;
        mtx[1][3] = 0.0f;
        mtx[2][0] = side.z;
        mtx[2][1] = dir.z;
        mtx[2][2] = pParticle->mBaseAxis.z;
        mtx[2][3] = 0.0f;
        PSMTXMultVecArraySR(mtx, edges, edges, 2);
        GXPosition3f32(edges[0].x + pos.x, edges[0].y + pos.y, edges[0].z + pos.z);
        GXTexCoord2f32(0.0f, texV);
        GXPosition3f32(edges[1].x + pos.x, edges[1].y + pos.y, edges[1].z + pos.z);
        GXTexCoord2f32(1.0f, texV);
    }
    GXEnd();
    texV = firstTexV;
    GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT1, count * 2);
    for (; first != pWork->mpAlivePtcl->getEnd(); first = nextNode(first), texV += step) {
        pWork->mpCurNode = first;
        JPABaseParticle* pParticle = first->getObject();
        TVec3f pos(pParticle->mPosition);
        f32 sinRot = -JMASSin(pParticle->mRotateAngle);
        f32 cosRot = JMASCos(pParticle->mRotateAngle);
        TVec3f edges[2];
        edges[0].set(-pParticle->mParticleScaleY * leftY, 0.0f, 0.0f);
        edges[0].set(edges[0].x * sinRot, 0.0f, edges[0].x * cosRot);
        edges[1].set(pParticle->mParticleScaleY * rightY, 0.0f, 0.0f);
        edges[1].set(edges[1].x * sinRot, 0.0f, edges[1].x * cosRot);
        TVec3f dir, side;
        p_direction[pWork->mDirType](pWork, pParticle, &dir);
        if (MR::isNearZero(dir, 0.001f))
            dir.set(0.0f, 1.0f, 0.0f);
        else
            MR::normalize(&dir);
        PSVECCrossProduct(&pParticle->mBaseAxis, &dir, &side);
        if (MR::isNearZero(side, 0.001f))
            side.set(1.0f, 0.0f, 0.0f);
        else
            MR::normalize(&side);
        if (MR::isSameDirection(dir, side, 0.01f))
            side.set(0.0f, 1.0f, 0.0f);
        PSVECCrossProduct(&dir, &side, &pParticle->mBaseAxis);
        MR::normalize(&pParticle->mBaseAxis);
        Mtx mtx;
        mtx[0][0] = side.x;
        mtx[0][1] = dir.x;
        mtx[0][2] = pParticle->mBaseAxis.x;
        mtx[0][3] = 0.0f;
        mtx[1][0] = side.y;
        mtx[1][1] = dir.y;
        mtx[1][2] = pParticle->mBaseAxis.y;
        mtx[1][3] = 0.0f;
        mtx[2][0] = side.z;
        mtx[2][1] = dir.z;
        mtx[2][2] = pParticle->mBaseAxis.z;
        mtx[2][3] = 0.0f;
        PSMTXMultVecArraySR(mtx, edges, edges, 2);
        GXPosition3f32(edges[0].x + pos.x, edges[0].y + pos.y, edges[0].z + pos.z);
        GXTexCoord2f32(0.0f, texV);
        GXPosition3f32(edges[1].x + pos.x, edges[1].y + pos.y, edges[1].z + pos.z);
        GXTexCoord2f32(1.0f, texV);
    }
    GXEnd();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX8);
    GXSetVtxDesc(GX_VA_TEX0, GX_INDEX8);
}
