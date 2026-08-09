#pragma once

#include <revolution/types.h>

class JAISoundID;

class AudBgmSetting {
public:
    struct BgmSettingInfo {
        s32 mMuteStateIndex;
        s32 mExtraChordIndex;
    };

    struct MultiBgmInfo {
        u32 mSeqId;
        u32 mStreamId;
        f32 mBeatMul;
        u32 mIntroBeats;
        u32 mLoopBeats;
        u32 mLoopStartSamples;
        u32 mLoopEndSamples;
    };

    static const u8* getMuteState(JAISoundID, s32);
    static u16 getExtraChordNum(JAISoundID, s32);
    static u32 getSeqIdForMultiBgm(u32);
    static u32 getStreamIdForMultiBgm(u32);
    static f32 getBeatMulForMultiBgm(u32);
    static u32 getIntroBeatsForMultiBgm(u32);
    static u32 getLoopBeatsForMultiBgm(u32);
    static u32 getLoopStartSamplesForMultiBgm(u32);
    static u32 getLoopEndSamplesForMultiBgm(u32);

private:
    static const BgmSettingInfo cBgmSettingInfo[];
    static const u8 cMuteStateTable[][8][10];
    static const u16 cExtraChordNum[][8];
    static const MultiBgmInfo cMultiBgmSet[];
};
