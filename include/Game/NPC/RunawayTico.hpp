#pragma once

#include "Game/NameObj/NameObj.hpp"
#include "Game/NPC/Tico.hpp"

class NameObjArchiveListCollector;
class ActorCameraInfo;
class JMapInfoIter;

class RunawayTico : public Tico {
public:
    RunawayTico(const char*);

    virtual ~RunawayTico();
    virtual void init(const JMapInfoIter&);
    virtual void initAfterPlacement();

    static void makeArchiveList(NameObjArchiveListCollector*, const JMapInfoIter&);

    void appearBushComment(const TVec3f&);
    void appearHoleComment(const TVec3f&);
    void appearPipeComment(const TVec3f&);
    void appearMamaComment(const TVec3f&);
    void setPosAfterCaught(const TVec3f&);
    void setPosAllCaught();
    bool isStartRunaway() const;
    void startRunaway();
    void setDemoTrans();
    void exeGuide0();
    void exeGuide1();
    void exeWhiteOut();
    void exeWhiteIn();
    void exeAppear();
    void exeTalk();

    ActorCameraInfo* mCameraInfo;  // 0x190
    s32 mMode;                     // 0x194
    s32 mDemoCastID;               // 0x198
    bool mIsStartRunaway;          // 0x19C
    bool _19D;
};
