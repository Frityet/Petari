#include "DisabledObjectAudio.hpp"
#include "Game/AudioLib/AudAnmSoundObject.hpp"
#include "JSystem/JAudio2/JASSoundParams.hpp"
#include "JSystem/JKernel/JKRHeap.hpp"
#include <atomic>

namespace {
std::atomic_size_t requests = 0;
JAISoundHandle* decline() noexcept { return aurora::audio::DisabledObjectAudio::request(); }
}
namespace aurora::audio {
JAISoundHandle* DisabledObjectAudio::request() noexcept {
    requests.fetch_add(1, std::memory_order_relaxed);
    return nullptr;
}
std::size_t DisabledObjectAudio::declined_requests() noexcept {
    return requests.load(std::memory_order_relaxed);
}
}

// These classes own ordinary detached Aurora handles, never a second handle
// ABI or an invented SDK sound/track graph. No start entrypoint attaches them.
JAISound* JAISoundHandles::getSound(int) { return nullptr; }
JAISoundHandle* JAISoundHandles::getHandleSoundID(JAISoundID) { return nullptr; }
JAISoundHandle* JAISoundHandles::getHandleUserData(u32) { return nullptr; }
JAISoundHandle* JAISoundHandles::getFreeHandle() {
    for (int i = 0; i < mNumHandles; ++i)
        if (!mHandles[i].isSoundAttached()) return &mHandles[i];
    return nullptr;
}
void JAISoundHandles::setPos(const TVec3f&) {}

JAUSoundObject::JAUSoundObject() : JAUSoundObject(nullptr, 0, nullptr) {}
JAUSoundObject::JAUSoundObject(TVec3f* position, u8 count, JKRHeap* heap)
    : JAISoundHandles(new (heap, 0) JAISoundHandle[count], count), mIsAllocated(true), _10(0), mPos(position) {}
JAUSoundObject::~JAUSoundObject() { dispose(); delete[] mHandles; }
void JAUSoundObject::process() {}
void JAUSoundObject::dispose() { mIsAllocated = false; }
bool JAUSoundObject::stopOK(JAISoundHandle&) { return true; }
JAISoundHandle* JAUSoundObject::startSound(JAISoundID) { return decline(); }
JAISoundHandle* JAUSoundObject::startLevelSound(JAISoundID) { return decline(); }
JAISoundHandle* JAUSoundObject::startSoundIndex(JAISoundID, u8) { return decline(); }
JAISoundHandle* JAUSoundObject::startLevelSoundIndex(JAISoundID, u8) { return decline(); }
void JAUSoundObject::stopSound(JAISoundID, u32) {}
JAISoundHandle* JAUSoundObject::getLowPrioSound(JAISoundID) { return nullptr; }
JAISoundHandle* JAUSoundObject::getFreeHandleNotReserved() { return getFreeHandle(); }

JAUSoundAnimator::JAUSoundAnimator(JAISoundHandles* handles)
    : mHandles(handles), mSoundAnimation(nullptr), mIsReversed(false), mLoopSoundIndex(0), mLifeTime(0),
      mLoopStartSoundIndex(0), mLoopEndSoundIndex(0), mLoopStartFrame(0), mLoopEndFrame(0), mTime(0) {}
JAISound* JAUSoundAnimator::getSound(int) { return nullptr; }
JAISoundHandle* JAUSoundAnimator::getFreeHandle(const JAUSoundAnimationSound*) { return mHandles->getFreeHandle(); }
u32 JAUSoundAnimator::getSoundID(const JAUSoundAnimationSound* sound, const TVec3f&, f32) { return sound->getSoundID(); }
void JAUSoundAnimator::ageSounds_() {}
void JAUSoundAnimator::updateSoundLifeTime_(f32, f32) {}

AudSoundObjHashData::AudSoundObjHashData() { init(); }
void AudSoundObjHashData::init() { mName = nullptr; mHash = 0; mID = 0; }
AudSoundObject::AudSoundObject(TVec3f* position, u8 count, JKRHeap* heap)
    : JAUSoundObject(position, count, heap), JKRDisposer(), mSeVersion(0), mMapCode(0), mMapCodeExtra(0),
      mHashDatas(count ? new AudSoundObjHashData[count] : nullptr), mNumHandles(count), mNumSounds(0) {}
AudSoundObject::~AudSoundObject() { delete[] mHashDatas; }
void AudSoundObject::addToSoundObjHolder() {}
bool AudSoundObject::isEnableStartSound(JAISoundID) { return aurora::audio::DisabledObjectAudio::enabled(); }
JAISoundHandle* AudSoundObject::startSound(JAISoundID) { return decline(); }
JAISoundHandle* AudSoundObject::startLevelSound(JAISoundID) { return decline(); }
JAISoundHandle* AudSoundObject::startLevelSound(const char*) { return decline(); }
JAISoundHandle* AudSoundObject::startSoundParam(JAISoundID, s32, s32) { return decline(); }
JAISoundHandle* AudSoundObject::startLevelSoundParam(JAISoundID, s32, s32) { return decline(); }
JAISoundHandle* AudSoundObject::startLevelSoundParam(const char*, s32, s32) { return decline(); }
bool AudSoundObject::writePort(JAISoundHandle*, u32, u16) { return false; }
bool AudSoundObject::isLimitedSound(JAISoundID) { return true; }
bool AudSoundObject::modifyLimitedSound(JAISoundID) { return false; }
bool AudSoundObject::isPlayingID(JAISoundID) { return false; }
void AudSoundObject::releaseHandle(JAISoundID) {}
JAISoundID AudSoundObject::convertNameToLevelSEID(const char*) { return JAISoundID(u32(-1)); }
void AudSoundObject::limitVoiceOne(JAISoundID) {}
void AudSoundObject::stopCategorySound(u32, u32) {}
void AudSoundObject::setMapCodeToPort(JAISoundHandle*, JAISoundID) {}
void AudSoundObject::setCutoffToPort(JAISoundHandle*, JAISoundID) {}
JAISoundID AudSoundObject::convertSoundIdFromSeVersion(JAISoundID id) const { return id; }
void AudSoundObject::modifySe_Kawamura(JAISoundHandle*, s32) {}
bool AudSoundObject::modifyLimitedSound_Kawamura(JAISoundID) { return false; }
void AudSoundObject::modifySe_Takezawa(JAISoundHandle*, s32, s32) {}
bool AudSoundObject::modifyLimitedSound_Takezawa(JAISoundID) { return false; }
void AudSoundObject::modifySe_Gohara(JAISoundHandle*, s32, s32) {}

AudAnmSoundObject::AudAnmSoundObject(TVec3f* position, u8 count, JKRHeap* heap)
    : AudSoundObject(position, count, heap), JAUSoundAnimator(this) {}
bool AudAnmSoundObject::playsSound(const JAUSoundAnimationSound*, const TVec3f&, f32) { return false; }
u32 AudAnmSoundObject::getSoundID(const JAUSoundAnimationSound* sound, const TVec3f& pos, f32 speed) {
    return JAUSoundAnimator::getSoundID(sound, pos, speed);
}
JAISoundHandle* AudAnmSoundObject::getFreeHandle(const JAUSoundAnimationSound* sound) {
    return JAUSoundAnimator::getFreeHandle(sound);
}
void AudAnmSoundObject::updateAnimSound(f32 time, const TVec3f&, JAISoundStarter*) {
    // The original skip path maintains all forward/reverse/loop scheduler
    // indices without issuing sound requests or visiting absent sound graphs.
    skip(time);
}
void AudAnmSoundObject::startAnimSound(const TVec3f&, f32, JAISoundStarter*) {
    mLoopSoundIndex += mIsReversed ? -1 : 1;
    (void)decline();
}
bool AudAnmSoundObject::releaseHandleIfNecessary(JAISoundHandle*, u32) { return false; }
