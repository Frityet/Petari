#include "Game/Util/MathUtil.hpp"
#include "JSystem/JParticle/JPAEmitter.hpp"
#include "JSystem/JParticle/JPAFieldBlock.hpp"
#include "JSystem/JParticle/JPAParticle.hpp"

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
