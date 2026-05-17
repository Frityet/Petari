#include "Game/Map/FileSelectModel.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util/EffectUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ObjUtil.hpp"

namespace {
    NEW_NERVE(FileSelectModelNrvOpen, FileSelectModel, Open);
    NEW_NERVE(FileSelectModelNrvBlinkOnce, FileSelectModel, BlinkOnce);
    NEW_NERVE(FileSelectModelNrvClose, FileSelectModel, Close);
    NEW_NERVE(FileSelectModelNrvBlink, FileSelectModel, Blink);
}  // namespace

FileSelectModel::FileSelectModel(const char* pModelName, MtxPtr pMtx, const char* pName) : LiveActor(pName), _8C(pMtx) {
    initModelManagerWithAnm(pModelName, nullptr, false);
    MR::connectToSceneNpc(this);
    initEffectKeeper(0, nullptr, false);
    MR::initLightCtrl(this);
    mScale.x = 30.0f;
    mScale.y = 30.0f;
    mScale.z = 30.0f;
    initNerve(&FileSelectModelNrvOpen::sInstance);
    MR::invalidateClipping(this);
    makeActorDead();
}

FileSelectModel::~FileSelectModel() {}

void FileSelectModel::calcAnim() {
    LiveActor::calcAnim();
}

void FileSelectModel::calcAndSetBaseMtx() {
    mPosition.set< f32 >(_8C[0][3], _8C[1][3], _8C[2][3]);
    MR::setBaseTRMtx(this, _8C);
}

void FileSelectModel::open() {
    if (!isNerve(&FileSelectModelNrvOpen::sInstance)) {
        setNerve(&FileSelectModelNrvOpen::sInstance);
    }
}

void FileSelectModel::blinkOnce() {
    setNerve(&FileSelectModelNrvBlinkOnce::sInstance);
}

void FileSelectModel::close() {
    setNerve(&FileSelectModelNrvClose::sInstance);
}

void FileSelectModel::blink() {
    setNerve(&FileSelectModelNrvBlink::sInstance);
}

bool FileSelectModel::isOpen() const {
    return isNerve(&FileSelectModelNrvOpen::sInstance);
}

void FileSelectModel::emitOpen() {
    MR::emitEffect(this, "Open");
}

void FileSelectModel::emitVanish() {
    MR::emitEffect(this, "Vanish");
}

void FileSelectModel::emitCopy() {
    MR::emitEffect(this, "Copy");
}

void FileSelectModel::emitCompleteEffect() {
    MR::emitEffect(this, "Complete");
}

void FileSelectModel::deleteCompleteEffect() {
    MR::deleteEffect(this, "Complete");
}

void FileSelectModel::exeOpen() {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "normal");
    }
}

void FileSelectModel::exeBlinkOnce() {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "blink");
    }

    if (MR::isBtpStopped(this)) {
        setNerve(&FileSelectModelNrvOpen::sInstance);
    }
}

void FileSelectModel::exeClose() {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "close");
    }
}

void FileSelectModel::exeBlink() {
    if (MR::isFirstStep(this)) {
        MR::startAction(this, "open");
    }

    if (MR::isBtpStopped(this)) {
        setNerve(&FileSelectModelNrvOpen::sInstance);
    }
}
