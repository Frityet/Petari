#include "Game/AudioLib/AudAnmSoundObject.hpp"
#include "JSystem/JAudio2/JASSoundParams.hpp"
#include "resource/BasResource.hpp"

u32 JAUSoundAnimation::getStartSoundIndex(f32 time) const {
    int idx;
    for (idx = 0; idx < getNumSounds(); idx++) {
        if (getSound(idx)->mNoteOnTime >= time) {
            break;
        }
    }
    return idx;
}

u32 JAUSoundAnimation::getEndSoundIndex(f32 time) const {
    int idx;
    for (idx = 0; idx < getNumSounds(); idx++) {
        if (getSound(idx)->mNoteOnTime > time) {
            break;
        }
    }
    return idx;
}

void JAUSoundAnimator::removeAnimation() {
    mSoundAnimation = nullptr;
}

bool JAUSoundAnimator::playsSound(const JAUSoundAnimationSound* pAnimation, const TVec3f& rPos, f32 f1) {
    if (pAnimation->playsOnlyOnce()) {
        if (mTime != pAnimation->mPlayTime) {
            return false;
        }
    } else if (pAnimation->playsAtIntervals() && pAnimation->mPlayTime != mTime % pAnimation->mRepeatInterval) {
        return false;
    }

    if (mIsReversed) {
        if (pAnimation->playsOnlyForward()) {
            return false;
        }
    } else {
        if (pAnimation->playsOnlyReverse()) {
            return false;
        }
    }

    return true;
}

void JAUSoundAnimator::modifySoundParams(JASSoundParams* pParams, const JAUSoundAnimationSound* pAnimation, f32 time) {
    time = time < 0.0f ? -time - 1.0f : time - 1.0f;

    f32 volume = pAnimation->mBaseVolume;
    if (pAnimation->mVolumeDelta != 0) {
        volume += pAnimation->mVolumeDelta * 2.0f * time;
    }

    pParams->mVolume = volume * (1.0f / 127.0f);

    if (pAnimation->mPitchDelta != 0) {
        pParams->mPitch = pAnimation->mBasePitch + pAnimation->mPitchDelta * time * (1.0f / 32.0f);
    } else {
        pParams->mPitch = pAnimation->mBasePitch;
    }

    pParams->mPan = pAnimation->mBasePan * (1.0f / 127.0f);
}

void JAUSoundAnimator::startAnimation(const JAUSoundAnimation* pAnimation, bool reversed, f32 loopStartFrame, f32 loopEndFrame) {
    pAnimation = smgpc::resource::resolve_bas_animation(pAnimation);
    ageSounds_();
    mSoundAnimation = pAnimation;
    if (mSoundAnimation == nullptr) {
        return;
    }

    mIsReversed = reversed;
    mTime = 0;

    if (mIsReversed) {
        mLoopSoundIndex = mSoundAnimation->getNumSounds() - 1;
        mLifeTime = FLOAT_MAX;
    } else {
        mLoopSoundIndex = 0;
        mLifeTime = 0.0f;
    }

    setLoopFrame(loopStartFrame, loopEndFrame);
}

void AudAnmSoundObject::update(f32 time) {
    if (mPos == nullptr) {
        return;
    }

    updateAnimSound(time, *mPos, nullptr);
}

void AudAnmSoundObject::skip(f32 time) {
    if (mSoundAnimation == nullptr || mSoundAnimation->getNumSounds() == 0) {
        return;
    }

    f32 speed = time - mLifeTime;
    if (!mIsReversed) {
        if (speed < 0.0f) {
            speed += mLoopEndFrame - mLoopStartFrame;
            while (mLoopSoundIndex < mLoopEndSoundIndex) {
                if (mIsReversed) {
                    mLoopSoundIndex--;
                } else {
                    mLoopSoundIndex++;
                }
            }
            mLifeTime = time;
            mLoopSoundIndex = mLoopStartSoundIndex;
            if (mTime < 0xFFFF) {
                mTime++;
            }
        }
        updateSoundLifeTime_(time, speed);
        while (mLoopSoundIndex < mSoundAnimation->getNumSounds() && mSoundAnimation->getSound(mLoopSoundIndex)->isNotingOn(time, false)) {
            if (mIsReversed) {
                mLoopSoundIndex--;
            } else {
                mLoopSoundIndex++;
            }
        }
    } else {
        if (speed > 0.0f) {
            speed -= mLoopEndFrame - mLoopStartFrame;
            while (mLoopSoundIndex >= mLoopStartSoundIndex) {
                if (mIsReversed) {
                    mLoopSoundIndex--;
                } else {
                    mLoopSoundIndex++;
                }
            }
            mLifeTime = time;
            mLoopSoundIndex = mLoopEndSoundIndex - 1;
            if (mTime < 0xFFFF) {
                mTime++;
            }
        }
        updateSoundLifeTime_(time, speed);
        while (mLoopSoundIndex >= 0 && mSoundAnimation->getSound(mLoopSoundIndex)->isNotingOn(time, true)) {
            if (mIsReversed) {
                mLoopSoundIndex--;
            } else {
                mLoopSoundIndex++;
            }
        }
    }

    mLifeTime = time;
}

void AudAnmSoundObject::setStartPos(f32 time) {
    if (mSoundAnimation == nullptr || mSoundAnimation->getNumSounds() == 0) {
        return;
    }

    mLifeTime = time;
    mLoopSoundIndex = mSoundAnimation->getStartSoundIndex(time);
    if (mIsReversed && mLoopSoundIndex > 0) {
        mLoopSoundIndex--;
    }
}

void AudAnmSoundObject::modifySoundParams(JASSoundParams* pParams, const JAUSoundAnimationSound* pSound, f32 speed) {
    if (speed > 3.0f) {
        speed = 3.0f;
    }
    JAUSoundAnimator::modifySoundParams(pParams, pSound, speed);
}

void AudSoundObject::setTrans(TVec3f* pTrans) {
    mPos = pTrans;
}

void AudSoundObject::setMapCode(s32 code) {
    if (code < 0) {
        mMapCode = 0;
    } else {
        mMapCode = code;
    }
}

void AudSoundObject::setMapCodeExtra(s32 code) {
    if (code == 0) {
        code = 1;
    }

    if (code < 0) {
        mMapCodeExtra = 0;
    } else {
        mMapCodeExtra = code;
    }
}

s32 AudSoundObject::getMapCode() const {
    if (mMapCodeExtra > 0) {
        return mMapCodeExtra;
    } else {
        return mMapCode;
    }
}

void AudSoundObject::clearMapCode() {
    mMapCode = 0;
    mMapCodeExtra = 0;
}
