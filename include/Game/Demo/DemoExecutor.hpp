#pragma once

#include "Game/Demo/DemoCastGroup.hpp"
#include "Game/Demo/DemoSubPartKeeper.hpp"

class DemoActionKeeper;
class DemoCameraKeeper;
class DemoPlayerKeeper;
class DemoSheetKeeperBase;
class DemoSoundKeeper;
class DemoTimeKeeper;
class DemoTalkAnimCtrl;
class DemoWipeKeeper;
class StageSwitchCtrl;
class TalkMessageCtrl;

class DemoExecutor : public DemoCastGroup {
public:
    DemoExecutor(const char*);

    virtual ~DemoExecutor();
    virtual void init(const JMapInfoIter&);
    virtual void registerDemoActor(LiveActor*, const JMapInfoIter&);
    virtual void movement();

    void start(NameObj*, const char*, s32);
    void startPart(NameObj*, const char*, const char*, s32);
    void startProperDemoSystem();
    void startDemoSystemPart(const char*, s32);
    bool tryStartProperDemoSystem();
    bool tryStartDemoSystemPart(const char*, s32);
    bool tryStartProperDemoSystemPart(const char*);

    void pause();
    void resume();
    void end();

    void addTalkAnimCtrl(DemoTalkAnimCtrl*);
    void addTalkMessageCtrl(LiveActor*, TalkMessageCtrl*);
    TalkMessageCtrl* findTalkMessageCtrl(const LiveActor*) const;
    void setTalkMessageCtrl(const LiveActor*, TalkMessageCtrl*);

    inline s32 getSubPartStep(const char* pName) {
        DemoSubPartInfo* subpart;
        DemoSubPartKeeper* subpartkeeper = mSubPartKeeper;
        for (int i = 0; i < subpartkeeper->mNumSubPartInfos; i++) {
            subpart = &subpartkeeper->mSubPartInfos[i];
            if (MR::isEqualString(pName, subpart->mMainPartName) && MR::isEqualSubString(subpart->mSubPartName, "会話アニメループ")) {
                return subpart->mMainPartStep;
            }
        }
        return 0;
    }

    const char* mSheetName;                                          // 0x14
    DemoTimeKeeper* mTimeKeeper;                                     // 0x18
    DemoSubPartKeeper* mSubPartKeeper;                               // 0x1C
    DemoPlayerKeeper* mPlayerKeeper;                                 // 0x20
    DemoCameraKeeper* mCameraKeeper;                                 // 0x24
    DemoActionKeeper* mActionKeeper;                                 // 0x28
    DemoWipeKeeper* mWipeKeeper;                                     // 0x2C
    DemoSoundKeeper* mSoundKeeper;                                   // 0x30
    MR::Vector< MR::FixedArray< DemoSheetKeeperBase*, 2 > > mSheetKeepers;  // 0x34
    StageSwitchCtrl* mStageSwitchCtrl;                               // 0x40
    NameObj* mDemoStarter;                                           // 0x44
    const char* mDemoName;                                           // 0x48
    s32 mStartType;                                                  // 0x4C
    bool _50;                                                        // 0x50
    u8 _51[3];                                                       // 0x51
    LiveActor* mInvalidateClippingActors[192];                       // 0x54
    s32 mNumInvalidateClippingActors;                                // 0x354
    DemoTalkAnimCtrl* mTalkAnimCtrls[8];  // 0x358 (inline array)
    s32 mNumTalkAnimCtrls;              // 0x378
    struct TalkMessageInfo {
        const LiveActor* mActor;       // 0x00
        TalkMessageCtrl* mMessageCtrl;  // 0x04
    };
    TalkMessageInfo mTalkMessageInfos[8];  // 0x37C
    s32 mNumTalkMessageInfos;              // 0x3BC
};
