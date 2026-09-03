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
