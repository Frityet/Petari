#include "Game/LiveActor/DynamicJointCtrl.hpp"
#include "Game/Util.hpp"

// scheduling issues
JointCtrlRate::JointCtrlRate() {
    _8 = -1;
    _0 = 1.0f;
    _4 = 0;
    _C = -1;
}

void JointCtrlRate::update() {
    if (_8 >= 0) {
        if (--_4 < 0) {
            _8 = -1;
            _0 = 1.0f;
        } else {
            _0 = static_cast< f32 >(_8 - _4) / static_cast< f32 >(_8);
        }
    } else if (_C >= 0) {
        if (--_4 < 0) {
            _C = -1;
            _0 = 0.0f;
        } else {
            _0 = static_cast< f32 >(_4) / static_cast< f32 >(_C);
        }
    }
}

void JointCtrlRate::startCtrl(s32 val) {
    if (val < 0) {
        val = 0xA;
    }

    _8 = val;
    _C = -1;
    _4 = val;
    _0 = 0.0f;
}

void JointCtrlRate::endCtrl(s32 val) {
    if (val < 0) {
        val = 0xA;
    }

    _C = val;
    _8 = -1;
    _4 = val;
    _0 = 1.0f;
}

void DynamicJointCtrlKeeper::update() {
    for (s32 i = 0; i < _4; i++) {
        mControls[i]->update();
    }
}

void DynamicJointCtrlKeeper::setCallBackFunction() {
    for (s32 i = 0; i < _4; i++) {
        mControls[i]->setCallBackFunction();
    }
}

void DynamicJointCtrlKeeper::startCtrl(const char* pName, s32 a2) {
    findJointCtrl(pName)->mControlRate->startCtrl(a2);
}

void DynamicJointCtrlKeeper::endCtrl(const char* pName, s32 a2) {
    findJointCtrl(pName)->mControlRate->endCtrl(a2);
}

void DynamicJointCtrlKeeper::reset() {
    for (s32 i = 0; i < _4; i++) {
        mControls[i]->reset();
    }
}

DynamicJointCtrl* DynamicJointCtrlKeeper::findJointCtrl(const char* pName) {
    for (s32 i = 0; i < _4; i++) {
        if (MR::isEqualString(mControls[i]->mName, pName)) {
            return mControls[i];
        }
    }

    return mControls[0];
}
