#include "JasStreamPcmBackend.hpp"
#include <JSystem/JAudio2/JASAramStream.hpp>

JASAramStream::JASAramStream() {
    mUpdateChannel = nullptr;
    _0AC = false;
    _0AD = false;
    _0AE = 0;
    _0B0 = 0;
    _0B4 = 0;
    _0B8 = 0;
    _0BC = 0;
    _0C0 = false;
    _0C4 = 0;
    _0C8 = 0.0f;
    _108 = 0;
    _10C = 0;
    mBlock = 0;
    _114 = 0;
    _118 = 0;
    _12C = 0;
    _148 = 0;
    _14C = 0;
    mCallback = nullptr;
    mCallbackData = nullptr;
    _158 = 0;
    mChannelNum = 0;
    mBufCount = 0;
    _160 = 0;
    _164 = 0;
    mLoop = false;
    mLoopStart = 0;
    mLoopEnd = 0;
    mVolume = 1.0f;
    mPitch = 1.0f;
    for (int i = 0; i < 6; i++) {
        mChannels[i] = nullptr;
        _130[i] = 0;
        _13C[i] = 0;
        mChannelVolume[i] = 1.0f;
        mChannelPan[i] = 0.5f;
        mChannelFxMix[i] = 0.0f;
        mChannelDolby[i] = 0.0f;
    }
    for (int i = 0; i < 6; i++) {
        _1DC[i] = 0;
    }
}

void JASAramStream::init(uintptr_t param_0, u32 param_1, StreamCallback i_callback, void* i_callbackData) {
    _148 = param_0;
    _14C = param_1;
    _0C8 = 0.0f;
    _0AE = 0;
    _0AC = false;
    _0AD = false;
    _114 = 0;
    mChannelNum = 0;
    for (int i = 0; i < 6; i++) {
        mChannelVolume[i] = 1.0f;
        mChannelPan[i] = 0.5f;
        mChannelFxMix[i] = 0.0f;
        mChannelDolby[i] = 0.0f;
    }
    mVolume = 1.0f;
    mPitch = 1.0f;
    _1DC[0] = 0xffff;
    mCallback = i_callback;
    mCallbackData = i_callbackData;
    OSInitMessageQueue(&_000, _040, 0x10);
    OSInitMessageQueue(&_020, _080, 4);
    aurora::audio::JasStreamPcmBackend::active().initialize(*this);
}

bool JASAramStream::start() {
    if (!OSSendMessage(&_000, (OSMessage)0, OS_MESSAGE_NOBLOCK)) {
        return false;
    }
    return true;
}

bool JASAramStream::stop(u16 param_0) {
    if (!OSSendMessage(&_000, (OSMessage)(uintptr_t)(param_0 << 0x10 | 1), OS_MESSAGE_NOBLOCK)) {
        return false;
    }
    return true;
}

bool JASAramStream::pause(bool param_0) {
    OSMessage msg = param_0 ? (OSMessage)2 : (OSMessage)3;
    if (!OSSendMessage(&_000, msg, OS_MESSAGE_NOBLOCK)) {
        return false;
    }
    return true;
}

bool JASAramStream::prepare(s32 entry, int channels) {
    return aurora::audio::JasStreamPcmBackend::active().prepare(*this, entry, channels);
}

bool JASAramStream::cancel() {
    _114 = 1;
    return aurora::audio::JasStreamPcmBackend::active().cancel(*this);
}
