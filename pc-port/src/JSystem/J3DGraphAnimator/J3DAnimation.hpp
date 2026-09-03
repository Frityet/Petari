#pragma once

#include <revolution/types.h>

#include <cstddef>

class J3DModelData;

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

    [[nodiscard]] u8 andState(u8 state) const {
        return mState & state;
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

struct J3DAnmKeyTableBase {
    /* 0x00 */ u16 mMaxFrame;
    /* 0x02 */ u16 mOffset;
    /* 0x04 */ u16 mType;
};  // Size = 0x6

struct J3DAnmTransformKeyTable {
    J3DAnmKeyTableBase mScaleInfo;
    J3DAnmKeyTableBase mRotationInfo;
    J3DAnmKeyTableBase mTranslateInfo;
};  // Size = 0x12

struct J3DAnmTransformFullTable {
    /* 0x00 */ u16 mScaleMaxFrame;
    /* 0x02 */ u16 mScaleOffset;
    /* 0x04 */ u16 mRotationMaxFrame;
    /* 0x06 */ u16 mRotationOffset;
    /* 0x08 */ u16 mTranslateMaxFrame;
    /* 0x0A */ u16 mTranslateOffset;
};  // Size = 0xC

struct J3DTransformInfo;

class J3DAnmBase {
public:
    J3DAnmBase() {
        mAttribute = 0;
        field_0x5 = 0;
        mFrameMax = 0;
        mFrame = 0.0f;
    }

    J3DAnmBase(s16 frameMax) {
        mAttribute = 0;
        field_0x5 = 0;
        mFrameMax = frameMax;
        mFrame = 0.0f;
    }

    virtual ~J3DAnmBase() {
    }
    virtual s32 getKind() const = 0;

    u8 getAttribute() const {
        return mAttribute;
    }
    s16 getFrameMax() const {
        return mFrameMax;
    }
    f32 getFrame() const {
        return mFrame;
    }
    void setFrame(f32 frame) {
        mFrame = frame;
    }

    /* 0x4 */ u8 mAttribute;
    /* 0x5 */ u8 field_0x5;
    /* 0x6 */ s16 mFrameMax;
    /* 0x8 */ f32 mFrame;
};  // Size: 0xC

class J3DAnmTransform : public J3DAnmBase {
public:
    J3DAnmTransform(s16, f32*, s16*, f32*);

    virtual ~J3DAnmTransform() {
    }
    virtual s32 getKind() const {
        return 0;
    }
    virtual void getTransform(u16, J3DTransformInfo*) const = 0;

    /* 0x0C */ f32* mScaleData;
    /* 0x10 */ s16* mRotData;
    /* 0x14 */ f32* mTransData;
    /* 0x18 */ s16 field_0x18;
    /* 0x1A */ s16 field_0x1a;
    /* 0x1C */ u16 field_0x1c;
    /* 0x1E */ u16 field_0x1e;
};  // Size: 0x20

class J3DAnmTransformKey : public J3DAnmTransform {
public:
    J3DAnmTransformKey() : J3DAnmTransform(0, NULL, NULL, NULL) {
        mDecShift = 0;
        mAnmTable = 0;
    }

    void calcTransform(f32, u16, J3DTransformInfo*) const;

    virtual ~J3DAnmTransformKey() {
    }
    virtual s32 getKind() const {
        return 8;
    }
    virtual void getTransform(u16 jointNo, J3DTransformInfo* pTransform) const {
        calcTransform(mFrame, jointNo, pTransform);
    }

    /* 0x20 */ int mDecShift;
    /* 0x24 */ J3DAnmTransformKeyTable* mAnmTable;
};  // Size: 0x28

class J3DAnmTransformFull : public J3DAnmTransform {
public:
    J3DAnmTransformFull() : J3DAnmTransform(0, NULL, NULL, NULL) {
        mAnmTable = NULL;
    }

    virtual ~J3DAnmTransformFull();
    virtual s32 getKind() const {
        return 9;
    }
    virtual void getTransform(u16, J3DTransformInfo*) const;

    /* 0x20 */ J3DAnmTransformFullTable* mAnmTable;
};  // Size: 0x24

class J3DAnmTransformFullWithLerp : public J3DAnmTransformFull {
public:
    virtual ~J3DAnmTransformFullWithLerp() {
    }
    virtual s32 getKind() const {
        return 16;
    }
    virtual void getTransform(u16, J3DTransformInfo*) const;
};  // Size: 0x24
