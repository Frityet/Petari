#pragma once

#include "Game/LiveActor/PartsModel.hpp"
#include "Game/NPC/Rosetta.hpp"
#include "Game/System/NerveExecutor.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/NPCUtil.hpp"

class NameObjArchiveListCollector;

class RosettaDemoHeavensDoor1 : public NerveExecutor {
public:
    RosettaDemoHeavensDoor1(Rosetta* pRosetta, const JMapInfoIter& rIter);

    static void makeArchiveList(NameObjArchiveListCollector* pCollector, const JMapInfoIter& rIter);

    void preDemo();
    void pstDemo();
    void fadeOut();
    void fadeIn();
    void exeWait();
    void exeFade();
    void exeDemo();

    template < typename T >
    void changeNerve() {
        setNerve(&T::sInstance);
    }

    /* 0x08 */ Rosetta* mRosetta;
    /* 0x0C */ PartsModel* mLightDome;
    /* 0x10 */ PartsModel* mDomeHalo;
};

class RosettaDemoHeavensDoor2 : public NerveExecutor {
public:
    RosettaDemoHeavensDoor2(Rosetta* pRosetta, const JMapInfoIter& rIter);

    static void makeArchiveList(NameObjArchiveListCollector* pCollector, const JMapInfoIter& rIter);

    void exeWait();

    template < typename T >
    void changeNerve() {
        setNerve(&T::sInstance);
    }

    /* 0x08 */ DemoStarter mDemoStarter;
    /* 0x14 */ Rosetta* mRosetta;
};
