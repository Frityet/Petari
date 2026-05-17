#include "Game/Map/FileSelectEffect.hpp"

#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ObjUtil.hpp"

namespace {
    NEW_NERVE(FileSelectEffectNrvAppear, FileSelectEffect, Appear);
    NEW_NERVE(FileSelectEffectNrvWait, FileSelectEffect, Wait);
    NEW_NERVE(FileSelectEffectNrvDisappear, FileSelectEffect, Disappear);
}  // namespace

FileSelectEffect::FileSelectEffect(const char* pName) : LiveActor(pName) {
    mEffectFrame = 0.0F;
}

FileSelectEffect::~FileSelectEffect() = default;

void FileSelectEffect::init(const JMapInfoIter&) {
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
        if (MR::isFirstStep(this)) {
            kill();
            return;
        }

        J3DFrameCtrl* ctrl = MR::getBrkCtrl(this);
        mEffectFrame = ctrl != nullptr ? ctrl->mFrame : 0.0F;
    } else if (isNerve(&FileSelectEffectNrvWait::sInstance)) {
        J3DFrameCtrl* ctrl = MR::getBrkCtrl(this);
        mEffectFrame = ctrl != nullptr ? static_cast< f32 >(ctrl->mEnd) : 0.0F;
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
        J3DFrameCtrl* ctrl = MR::getBrkCtrl(this);
        if (ctrl != nullptr) {
            MR::setBrkFrame(this, static_cast< f32 >(ctrl->mEnd) - mEffectFrame);
        }
    }

    if (MR::isBrkOneTimeAndStopped(this)) {
        kill();
    }
}

void FileSelectEffect::calcAndSetBaseMtx() {
    LiveActor::calcAndSetBaseMtx();
}
