#include "Game/Map/FileSelectModel.hpp"

#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ObjUtil.hpp"

namespace {
    [[nodiscard]] smgpc::game::J3dMatrix3x4 matrix_from_mtx(MtxPtr matrix, const TVec3f& scale) {
        if (matrix == nullptr) {
            return {};
        }

        return smgpc::game::J3dMatrix3x4{{
            matrix[0][0] * scale.x,
            matrix[0][1] * scale.y,
            matrix[0][2] * scale.z,
            matrix[0][3],
            matrix[1][0] * scale.x,
            matrix[1][1] * scale.y,
            matrix[1][2] * scale.z,
            matrix[1][3],
            matrix[2][0] * scale.x,
            matrix[2][1] * scale.y,
            matrix[2][2] * scale.z,
            matrix[2][3],
        }};
    }

    NEW_NERVE(FileSelectModelNrvOpen, FileSelectModel, Open);
    NEW_NERVE(FileSelectModelNrvBlinkOnce, FileSelectModel, BlinkOnce);
    NEW_NERVE(FileSelectModelNrvClose, FileSelectModel, Close);
    NEW_NERVE(FileSelectModelNrvBlink, FileSelectModel, Blink);
}  // namespace

FileSelectModel::FileSelectModel(const char* pModelName, MtxPtr pMtx, const char* pName) : LiveActor(pName), _8C(pMtx) {
    initModelManagerWithAnm(pModelName, nullptr, false);
    MR::connectToSceneNpc(this);
    initEffectKeeper(0, nullptr, false);
    mScale.x = 30.0F;
    mScale.y = 30.0F;
    mScale.z = 30.0F;
    initNerve(&FileSelectModelNrvOpen::sInstance);
    MR::invalidateClipping(this);
    makeActorDead();
}

void FileSelectModel::calcAnim() {
    LiveActor::calcAnim();
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

void FileSelectModel::calcAndSetBaseMtx() {
    if (_8C == nullptr) {
        LiveActor::calcAndSetBaseMtx();
        return;
    }

    mPosition.x = _8C[0][3];
    mPosition.y = _8C[1][3];
    mPosition.z = _8C[2][3];
    MR::setBaseTRMtx(this, matrix_from_mtx(_8C, mScale));
}
