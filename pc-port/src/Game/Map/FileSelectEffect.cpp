#include "Game/Map/FileSelectEffect.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/ObjUtil.hpp"

namespace {
    NEW_NERVE(FileSelectEffectNrvAppear, FileSelectEffect, Appear);
    NEW_NERVE(FileSelectEffectNrvWait, FileSelectEffect, Wait);
    NEW_NERVE(FileSelectEffectNrvDisappear, FileSelectEffect, Disappear);
};  // namespace

FileSelectEffect::FileSelectEffect(const char* pName) : LiveActor(pName) {
    mEffectFrame = 0.0f;
}

void FileSelectEffect::init(const JMapInfoIter& rIter) {
    initModelManagerWithAnm("MiniatureGalaxySelect", nullptr, false);
    MR::connectToSceneMapObj(this);
    MR::invalidateClipping(this);
    initNerve(&FileSelectEffectNrvWait::sInstance);
    makeActorDead();
}

void FileSelectEffect::appear() {
    LiveActor::appear();
    setNerve(&FileSelectEffectNrvAppear::sInstance);
}

void FileSelectEffect::disappear() {
    if (MR::isDead(this) || isNerve(&FileSelectEffectNrvDisappear::sInstance)) {
        return;
    }

    if (isNerve(&FileSelectEffectNrvAppear::sInstance)) {
        if (MR::isNewNerve(this)) {
            kill();
            return;
        }

        mEffectFrame = MR::getBrkCtrl(this)->getFrame();
    } else if (isNerve(&FileSelectEffectNrvWait::sInstance)) {
        mEffectFrame = MR::getBrkCtrl(this)->getEnd();
    }

    setNerve(&FileSelectEffectNrvDisappear::sInstance);
}

void FileSelectEffect::exeAppear() {
    if (MR::isFirstStep(this)) {
        MR::startBrk(this, "Appear");
        MR::startBtk(this, "MiniatureGalaxySelect");
    }

    if (MR::isBrkOneTimeAndStopped(this)) {
        setNerve(&FileSelectEffectNrvWait::sInstance);
    }
}

void FileSelectEffect::exeWait() {
}

void FileSelectEffect::exeDisappear() {
    if (MR::isFirstStep(this)) {
        MR::startBrk(this, "Disappear");
        MR::setBrkFrame(this, MR::getBrkCtrl(this)->getEnd() - mEffectFrame);
    }

    if (MR::isBrkOneTimeAndStopped(this)) {
        kill();
    }
}

void FileSelectEffect::calcAndSetBaseMtx() {
    TVec3f zDir = MR::getCamPos() - mPosition;

    if (MR::isNearZero(zDir, 0.001f)) {
        return;
    }

    MR::normalize(&zDir);

    TVec3f yDir(MR::getCamYdir());
    TVec3f xDir(yDir.cross(zDir));

    if (MR::isNearZero(xDir, 0.001f)) {
        return;
    }

    MR::normalize(&xDir);
    yDir.cross(zDir, xDir);

    TPos3f baseMtx;
    baseMtx.setXYZDir(xDir, yDir, zDir);
    baseMtx.setTrans(mPosition);
    MR::setBaseTRMtx(this, baseMtx);
}

FileSelectEffect::~FileSelectEffect() {
}
