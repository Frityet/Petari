#pragma once

#include <JSystem/JGeometry/TMatrix.hpp>

class LiveActor;
class JointController;
class JointControllerInfo;
class JointCtrlRate;

class TurnJointCtrl {
public:
    enum AXIS { X = 0, Y = 1, Z = 2 };

    struct Ctrl {
        bool mEnabled;
        JointController* mController;
        f32 mRate;
        AXIS mFrontAxis;
        AXIS mPitchAxis;
        AXIS mYawAxis;
    };

    TurnJointCtrl(LiveActor*);

    void init(f32, f32, f32);
    void addFace(const char*, f32, AXIS, AXIS, AXIS);
    void addWaist(const char*, f32, AXIS, AXIS, AXIS);
    void startCtrl(s32);
    void endCtrl(s32);
    void validate();
    void invalidate();
    void setStarePos(const TVec3f&);
    void update();
    void setCallBackFunction();
    bool updateJointMtxCallBackFace(TPos3f*, const JointControllerInfo&);
    bool updateJointMtxCallBackWaist(TPos3f*, const JointControllerInfo&);
    void getMtxDir(TVec3f*, const TPos3f*, AXIS);
    bool updateJointMtxCallBack(TPos3f*, const Ctrl&);

    /* 0x00 */ LiveActor* mHostActor;
    /* 0x04 */ Ctrl mFace;
    /* 0x1C */ Ctrl mWaist;
    /* 0x34 */ JointCtrlRate* mControlRate;
    /* 0x38 */ f32 mMaxYawDegree;
    /* 0x3C */ f32 mMaxPitchUpDegree;
    /* 0x40 */ f32 mMaxPitchDownDegree;
    /* 0x44 */ TVec3f mStarePos;
    /* 0x50 */ TVec3f _50;
    /* 0x5C */ f32 _5C;
    /* 0x60 */ f32 _60;
    /* 0x64 */ s32 _64;
    /* 0x68 */ bool mEnabled;
};
