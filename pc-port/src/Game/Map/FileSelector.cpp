#include "Game/Map/FileSelector.hpp"

#include "Game/LiveActor/Nerve.hpp"
#include "Game/NPC/MiiFacePartsHolder.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Screen/TitleSequenceProduct.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/NerveUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/StarPointerUtil.hpp"

namespace NrvFileSelector {
    NEW_NERVE(FileSelectorNrvWaitBind, FileSelector, WaitBind);
    NEW_NERVE(FileSelectorNrvTitle, FileSelector, Title);
    NEW_NERVE(FileSelectorNrvTitleEnd, FileSelector, TitleEnd);
    NEW_NERVE(FileSelectorNrvRFLError, FileSelector, RFLError);
    NEW_NERVE(FileSelectorNrvRFLWait, FileSelector, RFLWait);
};  // namespace NrvFileSelector

namespace {
    void make_identity(Mtx matrix) {
        matrix[0][0] = 1.0F;
        matrix[0][1] = 0.0F;
        matrix[0][2] = 0.0F;
        matrix[0][3] = 0.0F;
        matrix[1][0] = 0.0F;
        matrix[1][1] = 1.0F;
        matrix[1][2] = 0.0F;
        matrix[1][3] = 0.0F;
        matrix[2][0] = 0.0F;
        matrix[2][1] = 0.0F;
        matrix[2][2] = 1.0F;
        matrix[2][3] = 0.0F;
    }
}  // namespace

FileSelector::FileSelector() : NerveExecutor("ファイルセレクタ") {
    initNerve(&NrvFileSelector::FileSelectorNrvWaitBind::sInstance);
    createTitle();
    createSky();
}

FileSelector::~FileSelector() = default;

void FileSelector::update() {
    updateNerve();
}

void FileSelector::draw(smgpc::render::IRendererEngine& renderer) {
    mTitleBackground.draw(renderer, mSkyStep);
    if (mSkyAppeared) {
        ++mSkyStep;
    }
}

bool FileSelector::receiveOtherMsg(u32 msg, HitSensor*, HitSensor*) {
    if (MR::isMsgAutoRushBegin(msg) && isNerve(&NrvFileSelector::FileSelectorNrvWaitBind::sInstance)) {
        MR::hidePlayer();
        setNerve(&NrvFileSelector::FileSelectorNrvTitle::sInstance);
        return true;
    }

    if (MR::isMsgUpdateBaseMtx(msg)) {
        Mtx base_mtx{};
        make_identity(base_mtx);
        MR::setPlayerBaseMtx(base_mtx);
        return true;
    }

    return false;
}

void FileSelector::createTitle() {
    mTitleSeq = std::make_unique< TitleSequenceProduct >();
    mTitleSeq->kill();
}

void FileSelector::createSky() {
    mSkyAppeared = true;
}

void FileSelector::exeWaitBind() {
}

void FileSelector::exeTitle() {
    if (MR::isFirstStep(this)) {
        mTitleStarted = true;
        mTitleSeq->appear();
        MR::deactivateDefaultGameLayout();
        MR::startStarPointerModeTitle(this);
        MR::resetCameraMan();
    }

    mTitleSeq->updateNerve();

    if (!mTitleSeq->isActive()) {
        if (MR::getSceneObj< MiiFacePartsHolder >(SceneObj_MiiFacePartsHolder)->isInitEnd()) {
            if (MR::getSceneObj< MiiFacePartsHolder >(SceneObj_MiiFacePartsHolder)->isError()) {
                setNerve(&NrvFileSelector::FileSelectorNrvRFLError::sInstance);
            } else {
                setNerve(&NrvFileSelector::FileSelectorNrvTitleEnd::sInstance);
            }
        } else {
            setNerve(&NrvFileSelector::FileSelectorNrvRFLWait::sInstance);
        }
    }
}

void FileSelector::exeTitleEnd() {
    if (MR::isFirstStep(this)) {
        mTitleEnded = true;
    }
}

void FileSelector::exeRFLError() {
    mTitleEnded = true;
}

void FileSelector::exeRFLWait() {
    if (MR::getSceneObj< MiiFacePartsHolder >(SceneObj_MiiFacePartsHolder)->isInitEnd()) {
        setNerve(&NrvFileSelector::FileSelectorNrvTitleEnd::sInstance);
    }
}

std::uint64_t FileSelector::getSkyStep() const {
    return mSkyStep;
}

bool FileSelector::isTitleActive() const {
    return mTitleSeq != nullptr && mTitleSeq->isActive();
}

bool FileSelector::isTitleStarted() const {
    return mTitleStarted;
}

bool FileSelector::isTitleEnded() const {
    return mTitleEnded;
}
