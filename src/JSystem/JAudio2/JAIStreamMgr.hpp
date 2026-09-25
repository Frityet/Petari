#pragma once

#include "JSystem/JAudio2/JAIAudience.hpp"
#include "JSystem/JAudio2/JAIStream.hpp"
#include "JSystem/JAudio2/JASGlobal.hpp"
#include <aurora/j_audio_stream.hpp>
#include <functional>
#include <memory>

class JAIStreamAramMgr;
struct JAIStreamDataMgr;

class JAIStreamMgr : public JASGlobalInstance< JAIStreamMgr > {
public:
    JAIStreamMgr(bool setInstance);
    ~JAIStreamMgr();
    void bindNativeOutput(std::shared_ptr< aurora::audio::PcmAudioMixer >,
                          std::function< aurora::audio::JAudioStreamRecipe(JAISoundID) >);
    bool startSound(JAISoundID id, JAISoundHandle* handle, const TVec3f* posPtr);
    void freeDeadStream_();
    void calc();
    void stop();
    void stop(u32 fadeTime);
    void stopSoundID(JAISoundID id);
    void mixOut();
    JAIStream* newStream_();
    bool isActive() const {
        return mStreamList.getNumLinks() != 0;
    }

    JAIAudience* getAudience() {
        return mAudience;
    }

    void setAudience(JAIAudience* audience) {
        mAudience = audience;
    }
    JAISoundParamsMove* getParams() {
        return &mParams;
    }
    JAIStreamAramMgr* getStreamAramMgr() {
        return mStreamAramMgr;
    }
    JSUList< JAIStream >* getStreamList() {
        return &mStreamList;
    }

    void setStreamDataMgr(JAIStreamDataMgr* streamDataMgr) {
        streamDataMgr_ = streamDataMgr;
    }

    void setStreamAramMgr(JAIStreamAramMgr* streamDataMgr) {
        mStreamAramMgr = streamDataMgr;
    }

    void pause(bool paused) {
        mActivity.pause(paused);
    }

    void initParams() {
        mParams.init();
    }

    int getNumActiveStreams() const {
        return mStreamList.getNumLinks();
    }

private:
    /* 0x00 */ JAISoundActivity mActivity;
    /* 0x04 */ JAISoundParamsMove mParams;
    /* 0x54 */ JAIAudience* mAudience;
    /* 0x58 */ JSUList< JAIStream > mStreamList;
    /* 0x64 */ JAIStreamDataMgr* streamDataMgr_;
    /* 0x68 */ JAIStreamAramMgr* mStreamAramMgr;
    /* 0x6C */ JAISoundStrategyMgr< JAIStream >* soundStrategyMgr;
    std::shared_ptr< aurora::audio::PcmAudioMixer > mNativeMixer;
    std::function< aurora::audio::JAudioStreamRecipe(JAISoundID) > mNativeLoader;
};
