#include "Game/LiveActor/PartsModel.hpp"

#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/LiveActorUtil.hpp"

namespace {
    [[nodiscard]] smgpc::game::J3dMatrix3x4 matrix_from_mtx(MtxPtr matrix, const TVec3f& scale) {
        if (matrix == nullptr) {
            return {};
        }

        return smgpc::game::J3dMatrix3x4{{
            matrix[0][0] * scale.x,
            matrix[0][1] * scale.y,
            matrix[0][2] * scale.z,
            matrix[0][3],
            matrix[1][0] * scale.x,
            matrix[1][1] * scale.y,
            matrix[1][2] * scale.z,
            matrix[1][3],
            matrix[2][0] * scale.x,
            matrix[2][1] * scale.y,
            matrix[2][2] * scale.z,
            matrix[2][3],
        }};
    }
}  // namespace

PartsModel::PartsModel(LiveActor* pHost, const char* pName, const char* pModelName, MtxPtr pMtx, int drawBufferType, bool useLight)
    : LiveActor(pName), mHost(pHost), mMtx(pMtx) {
    initModelManagerWithAnm(pModelName, nullptr, useLight);
    if (drawBufferType < 0) {
        drawBufferType = MR::DrawBufferType_MapObj;
    }
    MR::connectToScene(this, MR::MovementType_MapObj, MR::CalcAnimType_MapObj, drawBufferType, -1);
    makeActorAppeared();
}

void PartsModel::movement() {
    if (mHost == nullptr || MR::isDead(mHost)) {
        makeActorDead();
        return;
    }

    LiveActor::movement();
}

void PartsModel::calcAndSetBaseMtx() {
    if (mMtx != nullptr) {
        MR::setBaseTRMtx(this, matrix_from_mtx(mMtx, mScale));
        return;
    }

    LiveActor::calcAndSetBaseMtx();
}
