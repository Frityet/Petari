#include "JSystem/JAudio2/JASAiCtrl.hpp"
#include "JSystem/JAudio2/JASChannel.hpp"
#include "JSystem/JAudio2/JASCmdStack.hpp"
#include "JSystem/JAudio2/JASDSPChannel.hpp"
#include "JSystem/JAudio2/JASDriverIF.hpp"
#include "JSystem/JAudio2/JASLfo.hpp"

// Original CPU subframe order. Native DMA scheduling and DSP execution live
// at the host boundary; there is no hardware-mailbox overrun heuristic here.

namespace JASDriver {
f32 sDacRate = 32028.5f;
u32 sSubFrames = 7;
u32 sSubFrameCounter;
MixCallback extMixCallback;
JASMixMode sMixMode = MIX_MODE_EXTRA;
void registerMixCallback(MixCallback callback, JASMixMode mode) { extMixCallback = callback; sMixMode = mode; }
f32 getDacRate() { return sDacRate; }
u32 getSubFrames() { return sSubFrames; }
u32 getDacSize() { return sSubFrames * 80 * 2; }
u32 getFrameSamples() { return sSubFrames * 80; }
u32 getSubFrameCounter() { return sSubFrameCounter; }
void updateDSP() {
    JASPortCmd::execAllCommand();
    DSPSyncCallback();
    JASChannel::receiveBankDisposeMsg();
    JASDSPChannel::updateAll();
    subframeCallback();
    JASLfo::updateFreeRun(32028.5f / getDacRate());
    ++sSubFrameCounter;
}
}
