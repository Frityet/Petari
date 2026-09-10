#include "Game/Util/JointController.hpp"
#include "Game/Util/JointUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "JSystem/J3DGraphAnimator/J3DJoint.hpp"
#include "JSystem/J3DGraphAnimator/J3DModel.hpp"
#include "JSystem/J3DGraphBase/J3DSys.hpp"

JointController::JointController() {
    mModel = 0;
    mJoint = 0;
}

bool JointController::calcJointMatrix(TPos3f*, const JointControllerInfo&) {
    return 0;
}

bool JointController::calcJointMatrixAfterChild(TPos3f*, const JointControllerInfo&) {
    return 0;
}

void JointController::registerCallBack() {
    mJoint->mCallBack = JointController::staticCallBack;
    mJoint->mCallBackUserData = this;
}

void JointController::calcJointMatrixAndSetSystem(J3DJoint* pJoint) {
    MtxPtr anmMtx = mModel->getAnmMtx(pJoint->mJntNo);
    TPos3f jointMtx;
    PSMTXCopy(anmMtx, jointMtx);

    JointControllerInfo info = {this, pJoint};
    if (calcJointMatrix(&jointMtx, info)) {
        PSMTXCopy(jointMtx, anmMtx);
        PSMTXCopy(jointMtx, J3DSys::mCurrentMtx);
    }
}

void JointController::calcJointMatrixAfterChildAndSetSystem(J3DJoint* pJoint) {
    MtxPtr anmMtx = mModel->getAnmMtx(pJoint->mJntNo);
    TPos3f jointMtx;
    PSMTXCopy(anmMtx, jointMtx);

    JointControllerInfo info = {this, pJoint};
    if (calcJointMatrixAfterChild(&jointMtx, info)) {
        PSMTXCopy(jointMtx, anmMtx);
    }
}

int JointController::staticCallBack(J3DJoint* pJoint, int timing) {
    if (pJoint == nullptr) {
        return 0;
    }

    JointController* controller = static_cast< JointController* >(pJoint->mCallBackUserData);
    if (controller == nullptr) {
        return 0;
    }

    if (timing == 0) {
        controller->calcJointMatrixAndSetSystem(pJoint);
    }

    if (timing == 1) {
        controller->calcJointMatrixAfterChildAndSetSystem(pJoint);
        pJoint->mCallBack = nullptr;
        pJoint->mCallBackUserData = nullptr;
    }

    return 0;
}

namespace MR {
    void setJointControllerParam(JointController* pController, const LiveActor* pActor, const char* pJointName) {
        J3DJoint* joint = MR::getJoint(pActor, pJointName);
        pController->mModel = MR::getJ3DModel(pActor);
        pController->mJoint = joint;
    }

    void setJointControllerParam(JointController* pController, const LiveActor* pActor, u16 jointIndex) {
        J3DJoint* joint = MR::getJoint(pActor, jointIndex);
        pController->mModel = MR::getJ3DModel(pActor);
        pController->mJoint = joint;
    }
};  // namespace MR
