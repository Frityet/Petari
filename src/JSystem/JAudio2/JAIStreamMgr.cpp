#include "JSystem/JAudio2/JAIStreamMgr.hpp"
#include "JSystem/JAudio2/JAISoundHandles.hpp"
#include "JSystem/JAudio2/JAISoundInfo.hpp"
#include "JSystem/JAudio2/JAIStreamDataMgr.hpp"
#include <aurora/allocation.hpp>
#include <aurora/exception.hpp>
#include <stdexcept>

JAIStreamMgr::JAIStreamMgr(bool setInstance) : JASGlobalInstance< JAIStreamMgr >(setInstance) {
    streamDataMgr_ = nullptr;
    mStreamAramMgr = nullptr;
    soundStrategyMgr = nullptr;
    mAudience = nullptr;
    mParams.init();
    mActivity.init();
}

JAIStreamMgr::~JAIStreamMgr() {
    while (auto* link = mStreamList.getFirst()) {
        auto* stream = link->getObject();
        mStreamList.remove(link);
        stream->die_JAIStream_();
        if (mNativeMixer) ::delete stream;
        else delete stream;
    }
}

void JAIStreamMgr::bindNativeOutput(std::shared_ptr< aurora::audio::PcmAudioMixer > mixer,
                                  std::function< aurora::audio::JAudioStreamRecipe(JAISoundID) > loader) {
    if (isActive() || !mixer || !loader)
        aurora::throw_host_exception< std::logic_error >("JAI stream output requires an idle manager, mixer and resource loader");
    mNativeMixer = std::move(mixer);
    mNativeLoader = std::move(loader);
}

bool JAIStreamMgr::startSound(JAISoundID id, JAISoundHandle* handle, const JGeometry::TVec3< f32 >* posPtr) {
    if (handle != nullptr && handle->isSoundAttached()) {
        (*handle)->stop();
    }

    s32 streamFileEntry = -1;
    if (!mNativeMixer && streamDataMgr_) streamFileEntry = streamDataMgr_->getStreamFileEntry(id);
    if (!mNativeMixer && streamFileEntry < 0) {
        return false;
    }

    JAIStream* stream = newStream_();
    JAISoundInfo* soundInfo = JASGlobalInstance< JAISoundInfo >::getInstance();

    int category = -1;
    if (soundInfo != nullptr) {
        category = soundInfo->getCategory(id);
    }

    if (stream == nullptr) {
        return false;
    }

    stream->JAIStreamMgr_startID_(id, streamFileEntry, posPtr, mAudience, category);
    if (mNativeMixer) {
        try {
            const aurora::allocation::HostAllocationScope host;
            stream->prepareNative(mNativeMixer, mNativeLoader(id));
        } catch (...) {
            mStreamList.remove(stream);
            stream->die_JAIStream_();
            ::delete stream;
            throw;
        }
    }
    if (soundInfo != nullptr) {
        soundInfo->getStreamInfo(id, stream);
    }

    if (handle != nullptr) {
        stream->attachHandle(handle);
    }

    return true;
}

void JAIStreamMgr::freeDeadStream_() {
    JSULink< JAIStream >* i = mStreamList.getFirst();
    while (i != nullptr) {
        JAIStream* stream = i->getObject();
        JSULink< JAIStream >* next = i->getNext();
        if (stream->isDead()) {
            mStreamList.remove(i);
            void* aramAddr = stream->JAIStreamMgr_getAramAddr_();
            if (aramAddr != nullptr) {
                bool result = mStreamAramMgr->deleteStreamAram(reinterpret_cast< uintptr_t >(aramAddr));
            }

            if (mNativeMixer) ::delete stream;
            else delete stream;
        }
        i = next;
    }
}

void JAIStreamMgr::calc() {
    JSULink< JAIStream >* i;
    mParams.calc();
    for (i = mStreamList.getFirst(); i != nullptr; i = i->getNext()) {
        i->getObject()->JAIStreamMgr_calc_();
    }
    freeDeadStream_();
}

void JAIStreamMgr::stop() {
    JSULink< JAIStream >* i;
    for (i = mStreamList.getFirst(); i != nullptr; i = i->getNext()) {
        i->getObject()->stop();
    }
}

void JAIStreamMgr::stop(u32 fadeTime) {
    JSULink< JAIStream >* i;
    for (i = mStreamList.getFirst(); i != nullptr; i = i->getNext()) {
        i->getObject()->stop(fadeTime);
    }
}

void JAIStreamMgr::stopSoundID(JAISoundID id) {
    JSULink< JAIStream >* i;
    for (i = mStreamList.getFirst(); i != nullptr; i = i->getNext()) {
        if ((u32)i->getObject()->getID() == (u32)id) {
            i->getObject()->stop();
        }
    }
}

void JAIStreamMgr::mixOut() {
    JSULink< JAIStream >* i;
    for (i = mStreamList.getFirst(); i != nullptr; i = i->getNext()) {
        i->getObject()->JAIStreamMgr_mixOut_(mParams.mParams, mActivity);
    }
}

JAIStream* JAIStreamMgr::newStream_() {
    if (!mNativeMixer && mStreamAramMgr == nullptr) {
        return nullptr;
    }

    JAIStream* stream = mNativeMixer ? ::new JAIStream(this, soundStrategyMgr) : new JAIStream(this, soundStrategyMgr);
    if (stream == nullptr) {
        return nullptr;
    }

    mStreamList.append(stream);
    return stream;
}
