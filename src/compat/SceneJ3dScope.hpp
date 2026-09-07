#pragma once
#include "compat/J3dCommandScope.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"
#include "JSystem/J3DGraphAnimator/J3DJoint.hpp"
#include "JSystem/J3DGraphAnimator/J3DMtxCalc.hpp"
#include <cstring>
#include <exception>
#include "Game/Util/MutexHolder.hpp"
namespace smgpc::compat {
// Original J3D routines share a process context; preserve the caller across
// complete native scene phases, including exceptions and nested callbacks.
class SceneJ3dScope final {
public:
    SceneJ3dScope() : _system(j3dSys), _buffer(J3DMtxCalc::mMtxBuffer),
        _joint(J3DMtxCalc::mJoint), _calculator(J3DJoint::mCurrentMtxCalc),
        _current_scale(J3DSys::mCurrentS), _parent_scale(J3DSys::mParentS),
        _thread(OSGetCurrentThread()), _exceptions(std::uncaught_exceptions()),
        _mutex_count(mutex().thread == _thread ? mutex().count : 0) {
        std::memcpy(_matrix, J3DSys::mCurrentMtx, sizeof(_matrix));
        std::memcpy(_tex_scale, J3DSys::sTexCoordScaleTable, sizeof(_tex_scale));
    }
    ~SceneJ3dScope() {
        j3dSys = _system;
        J3DMtxCalc::mMtxBuffer = _buffer;
        J3DMtxCalc::mJoint = _joint;
        J3DJoint::mCurrentMtxCalc = _calculator;
        J3DSys::mCurrentS = _current_scale;
        J3DSys::mParentS = _parent_scale;
        std::memcpy(J3DSys::mCurrentMtx, _matrix, sizeof(_matrix));
        std::memcpy(J3DSys::sTexCoordScaleTable, _tex_scale, sizeof(_tex_scale));
        if (std::uncaught_exceptions() > _exceptions)
            while (mutex().thread == _thread && mutex().count > _mutex_count) OSUnlockMutex(&mutex());
    }
    SceneJ3dScope(const SceneJ3dScope&) = delete;
    SceneJ3dScope& operator=(const SceneJ3dScope&) = delete;
private:
    J3dCommandScope _commands;
    J3DSys _system;
    J3DMtxBuffer* _buffer;
    J3DJoint* _joint;
    J3DMtxCalc* _calculator;
    Vec _current_scale, _parent_scale;
    Mtx _matrix;
    J3DTexCoordScaleInfo _tex_scale[8];
    static OSMutex& mutex() { return MR::MutexHolder<0>::sMutex; }
    OSThread* _thread;
    int _exceptions;
    int _mutex_count;
};
}
