#include "Game/AudioLib/AudSoundObjHolder.hpp"

#include "Game/AudioLib/AudSoundObject.hpp"
#include <JSystem/JKernel/JKRHeap.hpp>
#include <aurora/exception.hpp>
#include <stdexcept>

AudSoundObjHolder::AudSoundObjHolder(JKRHeap* pHeap, s32 capacity) {
    if (capacity < 0) {
        aurora::throw_host_exception< std::invalid_argument >("Sound-object holder capacity must be nonnegative");
    }
    mCapacity = capacity;
    mSize = 0;
    mArray = new (pHeap, 0) AudSoundObject*[capacity]();
}

AudSoundObjHolder::~AudSoundObjHolder() {
    for (s32 i = 0; i < mSize; i++) {
        if (mArray[i]->mNativeHolder == this) {
            mArray[i]->mNativeHolder = nullptr;
        }
    }
    delete[] mArray;
}

void AudSoundObjHolder::update() {
    for (int i = 0; i < mSize; i++) {
        mArray[i]->clearMapCode();
    }
}

void AudSoundObjHolder::add(AudSoundObject* pSound) {
    if (!pSound || pSound->mNativeHolder == this || mSize >= mCapacity) {
        return;
    }
    if (pSound->mNativeHolder) {
        pSound->mNativeHolder->remove(pSound);
    }
    mArray[mSize] = pSound;
    mSize++;
    pSound->mNativeHolder = this;
};

void AudSoundObjHolder::remove(AudSoundObject* pSound) {
    int soundIndex = -1;

    // Find sound object index in mArray
    for (int i = mSize - 1; i >= 0; i--) {
        if (mArray[i] == pSound) {
            soundIndex = i;
            break;
        }
    }

    // Remove sound object if found
    if (soundIndex >= 0) {
        moveOver(soundIndex, mSize - 1);
        mSize--;
        mArray[mSize] = nullptr;
    }
    if (pSound && pSound->mNativeHolder == this) {
        pSound->mNativeHolder = nullptr;
    }
}

void AudSoundObjHolder::moveOver(s32 initialIndex, s32 finalIndex) {
    for (int i = initialIndex; i < finalIndex; i++) {
        mArray[i] = mArray[i + 1];
    }
}
