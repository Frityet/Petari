#pragma once

#include <revolution/types.h>

class J3DFrameCtrl {
public:
    enum Attribute_e {
        /*  -1 */ EMode_NULL = -1,
        /* 0x0 */ EMode_NONE,
        /* 0x1 */ EMode_RESET,
        /* 0x2 */ EMode_LOOP,
        /* 0x3 */ EMode_REVERSE,
        /* 0x4 */ EMode_LOOP_REVERSE,
    };

    explicit J3DFrameCtrl(s16 endFrame) {
        init(endFrame);
    }

    J3DFrameCtrl() {
        init(0);
    }

    void init(s16 endFrame);
    void init(int endFrame) {
        init(static_cast< s16 >(endFrame));
    }
    BOOL checkPass(f32 passFrame);
    void update();
    virtual ~J3DFrameCtrl() {
    }

    [[nodiscard]] u8 getAttribute() const {
        return mAttribute;
    }
    void setAttribute(u8 attribute) {
        mAttribute = attribute;
    }
    [[nodiscard]] u8 getState() const {
        return mState;
    }
    [[nodiscard]] bool checkState(u8 state) const {
        return (mState & state) != 0;
    }
    [[nodiscard]] s16 getStart() const {
        return mStart;
    }
    void setStart(s16 start) {
        mStart = start;
        mFrame = start;
    }
    [[nodiscard]] s16 getEnd() const {
        return mEnd;
    }
    void setEnd(s16 end) {
        mEnd = end;
    }
    [[nodiscard]] s16 getLoop() const {
        return mLoop;
    }
    void setLoop(s16 loop) {
        mLoop = loop;
    }
    [[nodiscard]] f32 getRate() const {
        return mRate;
    }
    void setRate(f32 rate) {
        mRate = rate;
    }
    [[nodiscard]] f32 getFrame() const {
        return mFrame;
    }
    void setFrame(f32 frame) {
        mFrame = frame;
    }

    /* 0x04 */ u8 mAttribute;
    /* 0x05 */ u8 mState;
    /* 0x06 */ s16 mStart;
    /* 0x08 */ s16 mEnd;
    /* 0x0A */ s16 mLoop;
    /* 0x0C */ f32 mRate;
    /* 0x10 */ f32 mFrame;
};
