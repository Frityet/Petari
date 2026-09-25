#include "Game/Speaker/SpkSystem.hpp"
#include "Game/Speaker/SpkSpeakerCtrl.hpp"
#include <aurora/exception.hpp>
#include <stdexcept>

void SpkSystem::connect(s32 padChannel) {
    SpkSpeakerCtrl::connect(padChannel);
}

void SpkSystem::disconnect(s32 padChannel) {
    SpkSpeakerCtrl::disconnect(padChannel);
}

void SpkSystem::extensionProcess(s32 a1, s32 a2) {
    SpkSpeakerCtrl::extensionProcess(a1, a2);
}

void SpkSystem::reconnect(s32 padChannel) {
    if (padChannel < 0) {
        for (int i = 0; i < 4; i++) {
            SpkSpeakerCtrl::reconnect(i);
        }
    } else {
        SpkSpeakerCtrl::reconnect(padChannel);
    }
}

f32 SpkSystem::getDeviceVolume(s32 padChannel) {
    return SpkSpeakerCtrl::getDeviceVolume(padChannel);
}

void SpkSystem::startSound(s32, s32, SpkSoundHandle*) {
    aurora::throw_host_exception< std::logic_error >("Native speaker sound output is unavailable");
}
