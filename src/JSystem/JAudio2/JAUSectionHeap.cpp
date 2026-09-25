#include "JSystem/JAudio2/JAUSectionHeap.hpp"
#include "JSystem/JAudio2/JASWaveArcLoader.hpp"
#include "JSystem/JAudio2/JASWaveInfo.hpp"

// Native wave-archive I/O and its DSP heap are not initialized. Requests must
// report unavailable instead of publishing a loaded bank or a completion.
bool JAUSection::loadWaveArc(u32, u32) { return false; }
bool JAUSection::loadWaveArc(u32) { return false; }
bool JAUSection::eraseWaveArc(u32, u32) { return false; }
bool JAUSection::eraseWaveArc(u32) { return false; }

bool JAUSectionHeap::isWaveLoaded(u32 param_0, u32 param_1) {
    JASWaveBank* waveBank = sectionHeapData_.waveBankTable.getWaveBank(param_0);

    if (waveBank != nullptr) {
        JASWaveArc* waveArc = waveBank->getWaveArc(param_1);

        if (waveArc != nullptr) {
            if (waveArc->mStatus == 2) {
                return true;
            }
        }
    }

    return false;
}
