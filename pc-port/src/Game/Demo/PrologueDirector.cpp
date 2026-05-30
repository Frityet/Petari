#include "Game/Demo/PrologueDirector.hpp"

#include "Game/Camera/CameraTargetArg.hpp"
#include "Game/Camera/CameraTargetMtx.hpp"
#include "Game/LiveActor/ActorCameraInfo.hpp"
#include "Game/LiveActor/ModelObj.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Screen/PrologueLetter.hpp"
#include "Game/Screen/ProloguePictureBook.hpp"
#include "Game/System/StorySequenceExecutor.hpp"
#include "Game/Util/ActorCameraUtil.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/EventUtil.hpp"
#include "Game/Util/LayoutUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/NerveUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "Game/Util/ScreenUtil.hpp"
#include "Game/Util/SequenceUtil.hpp"
#include "Game/Util/SoundUtil.hpp"

namespace {
    static const char* sPictureBookDemoName = "プロローグデモ";
    static const char* sArriveDemoName = "主人公ピーチ城に到着";
    static const s32 sPicBookStartWipeFrame = 60;
    static const s32 sPeachLetterWait = 20;
    static const s32 sPeachLetterStartWipeFrame = 60;
    static const s32 sPeachLetterEndWipeFrame = 60;
    static const s32 sArriveStartWipeFrame = 30;
    static const s32 sArriveEndWipeFrame = 30;
    static const s32 sGameStartWipeFrame = 30;
    static const s32 sGameStartFrame = 15;
    static const s32 sFallingStarStep = 130;
};  // namespace

namespace {
    NEW_NERVE(PrologueDirectorNrvWait, PrologueDirector, Wait);
    NEW_NERVE(PrologueDirectorNrvPictureBook, PrologueDirector, PictureBook);
    NEW_NERVE(PrologueDirectorNrvPeachLetterStart, PrologueDirector, PeachLetterStart);
    NEW_NERVE(PrologueDirectorNrvPeachLetter, PrologueDirector, PeachLetter);
    NEW_NERVE(PrologueDirectorNrvPeachLetterWait, PrologueDirector, PeachLetterWait);
    NEW_NERVE(PrologueDirectorNrvPeachLetterEnd, PrologueDirector, PeachLetterEnd);
    NEW_NERVE(PrologueDirectorNrvBindWait, PrologueDirector, BindWait);
    NEW_NERVE(PrologueDirectorNrvArrive, PrologueDirector, Arrive);
    NEW_NERVE(PrologueDirectorNrvGameStart, PrologueDirector, GameStart);
};  // namespace

PrologueDirector::PrologueDirector(const char* pName)
    : LiveActor(pName), mPictureBook(nullptr), mLetter(nullptr), mScenery(nullptr), mMarioPosDummyModel(nullptr), mCameraTarget(nullptr), _D0(false) {
    _A0[0][0] = 1.0F;
    _A0[0][1] = 0.0F;
    _A0[0][2] = 0.0F;
    _A0[0][3] = 0.0F;
    _A0[1][0] = 0.0F;
    _A0[1][1] = 1.0F;
    _A0[1][2] = 0.0F;
    _A0[1][3] = 0.0F;
    _A0[2][0] = 0.0F;
    _A0[2][1] = 0.0F;
    _A0[2][2] = 1.0F;
    _A0[2][3] = 0.0F;
}

void PrologueDirector::init(const JMapInfoIter& rIter) {
    if (MR::useStageSwitchWriteA(this, rIter)) {
        _D0 = true;
    }

    MR::connectToSceneMapObjMovement(this);
    MR::invalidateClipping(this);
    initNerve(&PrologueDirectorNrvWait::sInstance);
    createPictureBook();
    createLetter();
    createScenery();
    createMarioPosDummyModel();
    createCameraTarget();
    makeActorDead();

    MR::createSceneObj(SceneObj_PrologueHolder);
    MR::getPrologueHolder()->registerPrologueObj(this);
}

void PrologueDirector::initAfterPlacement() {
    if (_D0) {
        MR::onSwitchA(this);
    }
}

void PrologueDirector::appear() {
    LiveActor::appear();
    setNerve(&PrologueDirectorNrvWait::sInstance);
    MR::forceCloseWipeFade();
    MR::forceOffImageEffect();

    if (_D0) {
        MR::offSwitchA(this);
    }
}

void PrologueDirector::kill() {
    LiveActor::kill();

    if (_D0) {
        MR::onSwitchA(this);
    }
}

void PrologueDirector::exeWait() {
    if (MR::tryStartDemoWithoutCinemaFrame(this, sPictureBookDemoName)) {
        MR::submitLevelSE();
        setNerve(&PrologueDirectorNrvPictureBook::sInstance);
        pauseOff();
        auto* player_base_mtx = MR::getPlayerBaseMtx();
        for (auto row = 0U; row < 3U; ++row) {
            for (auto column = 0U; column < 4U; ++column) {
                _A0[row][column] = player_base_mtx[row][column];
            }
        }
    }
}

void PrologueDirector::exePictureBook() {
    ActorCameraInfo cameraInfo = ActorCameraInfo(-1, 0);

    if (MR::isFirstStep(this)) {
        mPictureBook->appear();
        MR::openWipeFade(sPicBookStartWipeFrame);
        mScenery->appear();
        MR::startAnimCameraTargetSelf(mScenery, &cameraInfo, "DemoLetter", 0, 1.0f);
        MR::startStageBGM("STM_PROLOGUE_01", false);
    }

    bool isEndOrDead = mPictureBook->isEnd() || MR::isDead(mPictureBook);

    if (isEndOrDead) {
        MR::forceCloseWipeFade();
        mPictureBook->kill();
        MR::stopStageBGM(90);
#ifndef NDEBUG
        if (smgpc::game::story_sequence_executor().shouldRouteToHeavensDoorBunnyDemoAfterPictureBook()) {
            smgpc::game::story_sequence_executor().requestHeavensDoorBunnyDemoAfterPictureBook();
            MR::requestChangeStageInGameAfterLoadingGameData();
            kill();
            return;
        }
#endif
        setNerve(&PrologueDirectorNrvPeachLetterStart::sInstance);
    }
}

void PrologueDirector::exePeachLetterStart() {
    if (MR::isFirstStep(this)) {
        MR::openWipeFade(sPeachLetterStartWipeFrame);
    }

    if (MR::isStep(this, 90)) {
        setNerve(&PrologueDirectorNrvPeachLetter::sInstance);
    }
}

void PrologueDirector::exePeachLetter() {
    if (MR::isFirstStep(this)) {
        MR::startStageBGM("STM_PROLOGUE_02", false);
        MR::startSystemSE("SE_SY_LETTER_APPEAR", -1, -1);
        MR::startSystemSE("SE_SV_PEACH_OPENING_LETTER", -1, -1);
        mLetter->appear();
    }

    if (MR::isDead(mLetter)) {
        MR::stopSystemSE("SE_SV_PEACH_OPENING_LETTER", 0);
        setNerve(&PrologueDirectorNrvPeachLetterWait::sInstance);
    }
}

void PrologueDirector::exePeachLetterWait() {
    if (MR::isGreaterEqualStep(this, sPeachLetterWait)) {
        setNerve(&PrologueDirectorNrvPeachLetterEnd::sInstance);
    }
}

void PrologueDirector::exePeachLetterEnd() {
    if (MR::isFirstStep(this)) {
        MR::closeWipeFade(sPeachLetterEndWipeFrame);
    }

    if (MR::isWipeActive()) {
        return;
    }

    ActorCameraInfo cameraInfo = ActorCameraInfo(-1, 0);

    MR::endAnimCamera(mScenery, &cameraInfo, "DemoLetter", 0, true);
    mScenery->kill();
    setNerve(&PrologueDirectorNrvBindWait::sInstance);
}

void PrologueDirector::exeBindWait() {
    if (MR::isFirstStep(this)) {
        MR::endDemo(this, sPictureBookDemoName);
    }

    if (MR::tryStartDemoMarioPuppetable(this, sArriveDemoName)) {
        pauseOff();
        mMarioPosDummyModel->appear();
        MR::startBck(mMarioPosDummyModel, "DemoPeachCastleGate", nullptr);

        ActorCameraInfo cameraInfo = ActorCameraInfo(-1, 0);
        CameraTargetArg cameraTarget = CameraTargetArg(nullptr, mCameraTarget, nullptr, nullptr);

        MR::startAnimCameraTargetOther(mMarioPosDummyModel, &cameraInfo, "DemoPeachCastleGate", cameraTarget, 0, 1.0f);
        setNerve(&PrologueDirectorNrvArrive::sInstance);
    }
}

void PrologueDirector::exeArrive() {
    ActorCameraInfo cameraInfo = ActorCameraInfo(-1, 0);

    if (MR::isFirstStep(this)) {
        MR::permitLevelSE();

        MR::setPlayerBaseMtx(MR::getJointMtx(mMarioPosDummyModel, "MarioPosition"));
        MR::startBckPlayer("DemoPeachCastleGate", nullptr);
        MR::openWipeFade(sArriveStartWipeFrame);
        MR::setImageEffectControlAuto();
    }

    MR::setPlayerBaseMtx(MR::getJointMtx(mMarioPosDummyModel, "MarioPosition"));

    if (MR::isStep(this, sFallingStarStep)) {
        MR::startAtmosphereSE("SE_DM_ARRIVE_CASTLE_STAR", -1, -1);
    }

    if (MR::isStep(this, MR::getBckFrameMaxPlayer("DemoPeachCastleGate") - sArriveEndWipeFrame)) {
        MR::closeWipeFade(sArriveEndWipeFrame);
    }

    if (MR::isBckStoppedPlayer()) {
        MR::endAnimCamera(mMarioPosDummyModel, &cameraInfo, "DemoPeachCastleGate", 0, true);
        mMarioPosDummyModel->kill();
        setNerve(&PrologueDirectorNrvGameStart::sInstance);
    }
}

void PrologueDirector::exeGameStart() {
    if (MR::isFirstStep(this)) {
        MR::setPlayerBaseMtx(_A0);
        MR::forceCloseWipeFade();
    }

    if (MR::isStep(this, sGameStartFrame)) {
        MR::openWipeFade(sGameStartWipeFrame);
        MR::endDemo(this, sArriveDemoName);
        MR::initPlayerAfterOpeningDemo();
        kill();
    }
}

void PrologueDirector::createPictureBook() {
    mPictureBook = new ProloguePictureBook();
    mPictureBook->initWithoutIter();
    mPictureBook->kill();
}

void PrologueDirector::createLetter() {
    mLetter = new PrologueLetter("ピーチからの手紙");
    mLetter->initWithoutIter();
}

void PrologueDirector::createScenery() {
    mScenery = new ModelObj("背景書割", "DemoLetter", nullptr, MR::DrawBufferType_MapObjStrongLight, -2, -2, false);

    MR::invalidateClipping(mScenery);
    mScenery->initWithoutIter();
    MR::initLightCtrl(mScenery);
    mScenery->kill();

    ActorCameraInfo cameraInfo = ActorCameraInfo(-1, 0);

    MR::initAnimCamera(mScenery, &cameraInfo, "DemoLetter");
}

void PrologueDirector::createMarioPosDummyModel() {
    mMarioPosDummyModel = new ModelObj("マリオの経路", "DemoPeachCastleGate", nullptr, -2, -2, -2, false);
    mMarioPosDummyModel->initWithoutIter();

    MR::invalidateClipping(mMarioPosDummyModel);
    mMarioPosDummyModel->kill();
    mMarioPosDummyModel->mPosition.set(0.0f, 0.0f, 0.0f);

    ActorCameraInfo cameraInfo = ActorCameraInfo(-1, 0);

    MR::initAnimCamera(mMarioPosDummyModel, &cameraInfo, "DemoPeachCastleGate");
}

void PrologueDirector::createCameraTarget() {
    mCameraTarget = new CameraTargetMtx("カメラターゲットダミー");
    mCameraTarget->mMatrix.identity();
}

void PrologueDirector::control() {
}

void PrologueDirector::pauseOff() {
    MR::requestMovementOn(mPictureBook);
    mLetter->pauseOff();
    MR::requestMovementOn(mScenery);
    MR::requestMovementOn(mMarioPosDummyModel);
}

PrologueHolder::PrologueHolder(const char* pName) : NameObj(pName), mDirector(nullptr) {
}

void PrologueHolder::registerPrologueObj(PrologueDirector* pDirector) {
    mDirector = pDirector;
}

void PrologueHolder::start() {
    if (mDirector != nullptr) {
        mDirector->appear();
    }
}

namespace MR {
    PrologueHolder* getPrologueHolder() {
        return MR::getSceneObj< PrologueHolder >(SceneObj_PrologueHolder);
    }

    void startPrologue() {
        getPrologueHolder()->start();
    }
};  // namespace MR
