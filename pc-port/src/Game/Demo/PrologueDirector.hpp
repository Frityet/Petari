#pragma once

#include "Game/LiveActor/LiveActor.hpp"

class CameraTargetMtx;
class ModelObj;
class PrologueLetter;
class ProloguePictureBook;

class PrologueDirector : public LiveActor {
public:
    explicit PrologueDirector(const char *pName);

    void init(const JMapInfoIter &rIter) override;
    void initAfterPlacement() override;
    void appear() override;
    void kill() override;
    void control() override;

    void exeWait();
    void exePictureBook();
    void exePeachLetterStart();
    void exePeachLetter();
    void exePeachLetterWait();
    void exePeachLetterEnd();
    void exeBindWait();
    void exeArrive();
    void exeGameStart();
    void createPictureBook();
    void createLetter();
    void createScenery();
    void createMarioPosDummyModel();
    void createCameraTarget();
    void pauseOff();

private:
    /* 0x8C */ ProloguePictureBook *mPictureBook;
    /* 0x90 */ PrologueLetter *mLetter;
    /* 0x94 */ ModelObj *mScenery;
    /* 0x98 */ ModelObj *mMarioPosDummyModel;
    /* 0x9C */ CameraTargetMtx *mCameraTarget;
    /* 0xA0 */ Mtx _A0;
    /* 0xD0 */ bool _D0;
};

class PrologueHolder : public NameObj {
public:
    explicit PrologueHolder(const char *pName);

    void registerPrologueObj(PrologueDirector *pDirector);
    void start();

private:
    /* 0xC */ PrologueDirector *mDirector;
};

namespace MR {
    PrologueHolder *getPrologueHolder();
    void startPrologue();
}  // namespace MR
