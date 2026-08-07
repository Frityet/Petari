#pragma once

#include "Game/Demo/DemoCastGroup.hpp"
#include "Game/Util/Array.hpp"

class DemoActionKeeper;
class DemoCameraKeeper;
class DemoExecutor;
class DemoPlayerKeeper;
class DemoSoundKeeper;
class DemoSubPartInfo;
class DemoSubPartKeeper;
class DemoTalkAnimCtrl;
class DemoTimeKeeper;
class DemoWipeKeeper;
class StageSwitchCtrl;
class TalkMessageCtrl;

class DemoSheetKeeperBase {
public:
    virtual const char* getName() const = 0;
    virtual const char* getTypeString() const = 0;
    virtual void initCast(LiveActor*, const JMapInfoIter&);
    virtual void start();
    virtual void end();
    virtual void update();

    /* 0x04 */ DemoExecutor* mExecutor;
};

template < class T >
class DemoSheetKeeperInfoHolder {
public:
    virtual void executeType(const T*);

    /* 0x00 */ MR::Vector< MR::AssignableArray< T > > mInfo;
};

class DemoExecutor : public DemoCastGroup {
public:
    /// @brief Creates a new `DemoExecutor`
    DemoExecutor(const char* pName);
    virtual ~DemoExecutor();

    virtual void init(const JMapInfoIter& rIter);
    virtual void movement();

    virtual void registerDemoActor(LiveActor*, const JMapInfoIter&);

    void start(NameObj*, const char*, s32);
    void startPart(NameObj*, const char*, const char*, s32);
    void startProperDemoSystem();
    void startDemoSystemPart(const char*, s32);
    bool tryStartProperDemoSystem();
    bool tryStartDemoSystemPart(const char*, s32);
    bool tryStartProperDemoSystemPart(const char*);
    void pause();
    void resume();
    void addTalkAnimCtrl(DemoTalkAnimCtrl*);
    void addTalkMessageCtrl(LiveActor*, TalkMessageCtrl*);
    TalkMessageCtrl* findTalkMessageCtrl(const LiveActor*) const;
    void setTalkMessageCtrl(const LiveActor*, TalkMessageCtrl*);
    void end();

    /* 0x014 */ const char* mSheetName;
    /* 0x018 */ DemoTimeKeeper* mTimeKeeper;
    /* 0x01C */ DemoSubPartKeeper* mSubPartKeeper;
    /* 0x020 */ DemoPlayerKeeper* mPlayerKeeper;
    /* 0x024 */ DemoCameraKeeper* mCameraKeeper;
    /* 0x028 */ DemoActionKeeper* mActionKeeper;
    /* 0x02C */ DemoWipeKeeper* mWipeKeeper;
    /* 0x030 */ DemoSoundKeeper* mSoundKeeper;
    /* 0x034 */ MR::Vector< MR::FixedArray< DemoSheetKeeperBase*, 2 > > mSheetKeepers;
    /* 0x040 */ StageSwitchCtrl* mStageSwitchCtrl;
    /* 0x044 */ NameObj* mDemoStarter;
    /* 0x048 */ const char* mDemoName;
    /* 0x04C */ s32 mStartType;
    /* 0x050 */ bool _50;
    /* 0x054 */ LiveActor* mInvalidateClippingActors[192];
    /* 0x354 */ s32 mNumInvalidateClippingActors;
    /* 0x358 */ DemoTalkAnimCtrl* mTalkAnimCtrls[8];
    /* 0x378 */ s32 mNumTalkAnimCtrls;
    struct TalkMessageInfo {
        /* 0x00 */ const LiveActor* mActor;
        /* 0x04 */ TalkMessageCtrl* mMessageCtrl;
    };

    /* 0x37C */ TalkMessageInfo mTalkMessageInfos[8];
    /* 0x3BC */ s32 mNumTalkMessageInfos;
};
