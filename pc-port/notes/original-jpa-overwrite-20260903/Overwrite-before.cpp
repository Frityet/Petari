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
