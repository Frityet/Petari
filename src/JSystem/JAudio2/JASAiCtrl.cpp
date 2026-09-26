#include "JSystem/JAudio2/JASAiCtrl.hpp"
#include "JSystem/JAudio2/JASCalc.hpp"
#include "JSystem/JAudio2/JASChannel.hpp"
#include "JSystem/JAudio2/JASCmdStack.hpp"
#include "JSystem/JAudio2/JASDSPChannel.hpp"
#include "JSystem/JAudio2/JASDriverIF.hpp"
#include "JSystem/JAudio2/JASLfo.hpp"
#include "JSystem/JAudio2/JASProbe.hpp"

// Original CPU subframe order. Native DMA scheduling and DSP execution live
// at the host boundary; there is no hardware-mailbox overrun heuristic here.

namespace JASDriver {
    f32 sDacRate = 32028.5f;
    u32 sSubFrames = 7;
    u32 sSubFrameCounter;
    MixCallback extMixCallback;
    JASMixMode sMixMode = MIX_MODE_EXTRA;
    void registerMixCallback(MixCallback callback, JASMixMode mode) {
        extMixCallback = callback;
        sMixMode = mode;
    }
    f32 getDacRate() {
        return sDacRate;
    }
    u32 getSubFrames() {
        return sSubFrames;
    }
    u32 getDacSize() {
        return sSubFrames * 80 * 2;
    }
    u32 getFrameSamples() {
        return sSubFrames * 80;
    }
    u32 getSubFrameCounter() {
        return sSubFrameCounter;
    }
    void updateDSP() {
        JASPortCmd::execAllCommand();
        DSPSyncCallback();
        JASChannel::receiveBankDisposeMsg();
        JASDSPChannel::updateAll();
        subframeCallback();
        JASLfo::updateFreeRun(32028.5f / getDacRate());
        ++sSubFrameCounter;
    }
}  // namespace JASDriver

void JASDriver::mixMonoTrack(s16 *buffer, u32 param_1, MixCallback callback) {
    JASProbe::start(5, "MONO-MIX");
    s16 *r26 = callback(param_1);
    if (r26 == nullptr) {
        return;
    }
    JASProbe::stop(5);
    s16 *pTrack = buffer;
    s16 *r28 = r26;
    for (u32 i = param_1; i != 0; i--) {
        s32 src = pTrack[0];
        src += r28[0];
        pTrack[0] = JASCalc::clamp<s16, s32>(src);
        pTrack++;
        src = pTrack[0];
        src += r28[0];
        pTrack[0] = JASCalc::clamp<s16, s32>(src);
        pTrack++;
        r28++;
    }
}

void JASDriver::mixMonoTrackWide(s16 *buffer, u32 param_1, MixCallback callback) {
    JASProbe::start(5, "MONO(W)-MIX");
    s16 *r26 = callback(param_1);
    if (!r26) {
        return;
    }
    JASProbe::stop(5);
    s16 *pTrack = buffer;
    s16 *r28 = r26;
    for (u32 i = param_1; i != 0; i--) {
        s32 src = pTrack[0];
        src += r28[0];
        pTrack[0] = JASCalc::clamp<s16, s32>(src);
        pTrack++;
        src = pTrack[0];
        src -= r28[0];
        pTrack[0] = JASCalc::clamp<s16, s32>(src);
        pTrack++;
        r28++;
    }
}

void JASDriver::mixExtraTrack(s16 *buffer, u32 param_1, MixCallback callback) {
    JASProbe::start(5, "DSPMIX");
    s16 *r27 = callback(param_1);
    if (!r27) {
        return;
    }
    JASProbe::stop(5);
    JASProbe::start(6, "MIXING");
    s16 *pTrack = buffer;
    s16 *r29 = r27;
    s16 *r28 = r27 + getFrameSamples();
    for (u32 i = param_1; i != 0; i--) {
        s32 r25 = pTrack[0] + r28[0];
        pTrack[0] = JASCalc::clamp<s16, s32>(r25);
        pTrack++;
        r25 = pTrack[0] + r29[0];
        pTrack[0] = JASCalc::clamp<s16, s32>(r25);
        pTrack++;
        r28++;
        r29++;
    }
    JASProbe::stop(6);
}

void JASDriver::mixInterleaveTrack(s16 *buffer, u32 param_1, MixCallback callback) {
    s16 *r31 = callback(param_1);
    if (!r31) {
        return;
    }
    s16 *pTrack = buffer;
    s16 *r30 = r31;
    for (u32 i = param_1 * 2; i != 0; i--) {
        s32 r26 = pTrack[0] + r30[0];
        pTrack[0] = JASCalc::clamp<s16, s32>(r26);
        pTrack++;
        r30++;
    }
}

const JASDriver::MixFunc JASDriver::sMixFuncs[4] = {
    mixMonoTrack,
    mixMonoTrackWide,
    mixExtraTrack,
    mixInterleaveTrack,
};
