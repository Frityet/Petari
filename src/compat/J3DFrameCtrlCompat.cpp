#include <JSystem/J3DGraphAnimator/J3DAnimation.hpp>

void J3DFrameCtrl::init(s16 endFrame) {
    mAttribute = EMode_LOOP;
    mState = 0;
    mStart = 0;
    mEnd = endFrame;
    mLoop = 0;
    mRate = 1.0f;
    mFrame = 0.0f;
}

BOOL J3DFrameCtrl::checkPass(f32 passFrame) {
    f32 nextFrame = mFrame + mRate;

    switch (mAttribute) {
    case EMode_NONE:
    case EMode_RESET:
        if (nextFrame < mStart) {
            nextFrame = mStart;
        }
        if (nextFrame >= mEnd) {
            nextFrame = mEnd - 0.001f;
        }
        if (mFrame <= nextFrame) {
            return mFrame <= passFrame && passFrame < nextFrame;
        }
        return nextFrame <= passFrame && passFrame < mFrame;

    case EMode_LOOP:
        if (mFrame < mStart) {
            while (nextFrame < mStart) {
                if (mLoop - mStart <= 0.0f) {
                    break;
                }
                nextFrame += mLoop - mStart;
            }
            return nextFrame <= passFrame && passFrame < mLoop;
        }
        if (mEnd <= mFrame) {
            while (nextFrame >= mEnd) {
                if (mEnd - mLoop <= 0.0f) {
                    break;
                }
                nextFrame -= mEnd - mLoop;
            }
            return mLoop <= passFrame && passFrame < nextFrame;
        }
        if (nextFrame < mStart) {
            while (nextFrame < mStart) {
                if (mLoop - mStart <= 0.0f) {
                    break;
                }
                nextFrame += mLoop - mStart;
            }
            return (mStart <= passFrame && passFrame < mFrame) || (nextFrame <= passFrame && passFrame < mLoop);
        }
        if (mEnd <= nextFrame) {
            while (nextFrame >= mEnd) {
                if (mEnd - mLoop <= 0.0f) {
                    break;
                }
                nextFrame -= mEnd - mLoop;
            }
            return (mFrame <= passFrame && passFrame < mEnd) || (mLoop <= passFrame && passFrame < nextFrame);
        }
        if (mFrame <= nextFrame) {
            return mFrame <= passFrame && passFrame < nextFrame;
        }
        return nextFrame <= passFrame && passFrame < mFrame;

    case EMode_REVERSE:
    case EMode_LOOP_REVERSE:
        if (nextFrame >= mEnd) {
            nextFrame = mEnd - 0.001f;
        }
        if (nextFrame < mStart) {
            nextFrame = mStart;
        }
        if (mFrame <= nextFrame) {
            return mFrame <= passFrame && passFrame < nextFrame;
        }
        return nextFrame <= passFrame && passFrame < mFrame;

    default:
        return FALSE;
    }
}

void J3DFrameCtrl::update() {
    mState = 0;
    mFrame += mRate;

    switch (mAttribute) {
    case EMode_NONE:
        if (mFrame < mStart) {
            mFrame = mStart;
            mRate = 0.0f;
            mState |= 1U;
        }
        if (mFrame >= mEnd) {
            mFrame = mEnd - 0.001f;
            mRate = 0.0f;
            mState |= 1U;
        }
        break;

    case EMode_RESET:
        if (mFrame < mStart) {
            mFrame = mStart;
            mRate = 0.0f;
            mState |= 1U;
        }
        if (mFrame >= mEnd) {
            mFrame = mStart;
            mRate = 0.0f;
            mState |= 1U;
        }
        break;

    case EMode_LOOP:
        while (mFrame < mStart) {
            mState |= 2U;
            if (mLoop - mStart <= 0.0f) {
                break;
            }
            mFrame += mLoop - mStart;
        }
        while (mFrame >= mEnd) {
            mState |= 2U;
            if (mEnd - mLoop <= 0.0f) {
                break;
            }
            mFrame -= mEnd - mLoop;
        }
        break;

    case EMode_REVERSE:
        if (mFrame >= mEnd) {
            mFrame = mEnd - (mFrame - mEnd);
            mRate = -mRate;
        }
        if (mFrame < mStart) {
            mFrame = mStart - (mFrame - mStart);
            mRate = 0.0f;
            mState |= 1U;
        }
        break;

    case EMode_LOOP_REVERSE:
        if (mFrame >= mEnd - 1.0f) {
            mFrame = (mEnd - 1.0f) - (mFrame - (mEnd - 1.0f));
            mRate = -mRate;
        }
        if (mFrame < mStart) {
            mFrame = mStart - (mFrame - mStart);
            mRate = -mRate;
            mState |= 2U;
        }
        break;

    default:
        break;
    }
}
