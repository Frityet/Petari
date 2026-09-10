// Original speaker connection control; the optional mixer/stream owner is separate.
// Bodies come from the corresponding decomp Game/Speaker translation units.
#include "Game/Speaker/SpkSystem.hpp"
#include "Game/Speaker/SpkSpeakerCtrl.hpp"
#include <JSystem/JAudio2/JASCriticalSection.hpp>
#include <cstring>
#include <revolution/wpad.h>

SpeakerInfo sSpeakerInfo[WPAD_MAX_CONTROLLERS];

void SpkSystem::connect(s32 padChannel) {
    SpkSpeakerCtrl::connect(padChannel);
}

void SpkSystem::disconnect(s32 padChannel) {
    SpkSpeakerCtrl::disconnect(padChannel);
}

void SpkSystem::extensionProcess(s32 a1, s32 a2) {
    SpkSpeakerCtrl::extensionProcess(a1, a2);
}

void SpkSpeakerCtrl::connect(s32 padChannel) {
    JASCriticalSection crit;
    sSpeakerInfo[padChannel].mIsConnected = true;
    sSpeakerInfo[padChannel].mIsPlaying = false;
    sSpeakerInfo[padChannel].mState = SpeakerInfo::State_ENABLE;
    SpkSpeakerCtrl::initReconnect(padChannel);
    sSpeakerInfo[padChannel].mUsingTimeOut = -1;
    SpkSpeakerCtrl::setSpeakerOn(padChannel);
}

void SpkSpeakerCtrl::disconnect(s32 padChannel) {
    JASCriticalSection crit;
    sSpeakerInfo[padChannel].mIsConnected = false;
    sSpeakerInfo[padChannel].mIsPlaying = false;
    sSpeakerInfo[padChannel].mState = SpeakerInfo::State_ENABLE;
    SpkSpeakerCtrl::setSpeakerOff(padChannel);
    SpkSpeakerCtrl::initReconnect(padChannel);
    sSpeakerInfo[padChannel].mUsingTimeOut = -1;
}

void SpkSpeakerCtrl::setSpeakerOn(s32 padChannel) {
    JASCriticalSection crit;
    // TODO: WPAD command magic numbers
    s32 err = WPADControlSpeaker(padChannel, 1, SpkSpeakerCtrl::setSpeakerOnCallback);

    if (err == WPAD_ERR_BUSY) {
        sSpeakerInfo[padChannel].mState = SpeakerInfo::State_ON;
    } else {
        sSpeakerInfo[padChannel].mState = SpeakerInfo::State_ENABLE;
    }
}

void SpkSpeakerCtrl::setSpeakerOnCallback(s32 padChannel, s32 err) {
    JASCriticalSection crit;
    if (err == WPAD_ERR_NONE) {
        sSpeakerInfo[padChannel].mState = SpeakerInfo::State_ENABLE;
        SpkSpeakerCtrl::setSpeakerPlay(padChannel);
    } else if (err == WPAD_ERR_TRANSFER) {
        sSpeakerInfo[padChannel].mState = SpeakerInfo::State_ON;
    }
}

void SpkSpeakerCtrl::setSpeakerPlay(s32 padChannel) {
    JASCriticalSection crit;
    // TODO: WPAD command magic numbers
    s32 err = WPADControlSpeaker(padChannel, 4, SpkSpeakerCtrl::startPlayCallback);

    if (err == WPAD_ERR_BUSY) {
        sSpeakerInfo[padChannel].mState = SpeakerInfo::State_PLAY;
    } else {
        sSpeakerInfo[padChannel].mState = SpeakerInfo::State_ENABLE;
    }
}

void SpkSpeakerCtrl::startPlayCallback(s32 padChannel, s32 err) {
    JASCriticalSection crit;

    if (err == WPAD_ERR_NONE) {
        sSpeakerInfo[padChannel].mIsPlaying = true;
        sSpeakerInfo[padChannel].mIsUpdated = true;
        sSpeakerInfo[padChannel].mState = SpeakerInfo::State_ENABLE;
        sSpeakerInfo[padChannel].mUsingTimeOut = 8 * 60 * 60;
        memset(&sSpeakerInfo[padChannel].mWENCInfo, 0, sizeof(WENCInfo));
    } else if (err == WPAD_ERR_TRANSFER) {
        sSpeakerInfo[padChannel].mState = SpeakerInfo::State_PLAY;
    }
}

void SpkSpeakerCtrl::setSpeakerOff(s32 padChannel) {
    sSpeakerInfo[padChannel].mIsPlaying = false;
    sSpeakerInfo[padChannel].mState = SpeakerInfo::State_ENABLE;
    sSpeakerInfo[padChannel].mUsingTimeOut = -1;
    // TODO: WPAD command magic numbers
    WPADControlSpeaker(padChannel, 0, nullptr);
}

void SpkSpeakerCtrl::initReconnect(s32 padChannel) {
    sSpeakerInfo[padChannel].mReconnectState = SpeakerInfo::Reconnect_NONE;
    sSpeakerInfo[padChannel].mReconnectTime = -1;
}

void SpkSpeakerCtrl::extensionProcess(s32, s32) {
}
