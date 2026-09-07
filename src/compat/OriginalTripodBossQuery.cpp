// Original Accesser identity and joint-matrix queries; no native boss state substitute.
#include "Game/Boss/TripodBossAccesser.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/ObjUtil.hpp"

namespace {
    static s32 cJMapBoneIDToBoneIndexTable[] = {0,  1, 2, 7,  8,  9, 14, 15, 16, 21, 3,  4,  0,  1,  5,  6,  2,  -1, -1, -1, 10,
                                                11, 7, 8, 12, 13, 9, -1, -1, -1, 17, 18, 14, 15, 19, 20, 16, -1, -1, -1, 21};
    static s32 cJMapBoneIDToBoneIndexTableSize = ARRAY_SIZE(cJMapBoneIDToBoneIndexTable);

    s32 convertBoneIDToIndex(s32 id) NO_INLINE {
        if (id < 0 || cJMapBoneIDToBoneIndexTableSize <= id) {
            return -1;
        }

        return cJMapBoneIDToBoneIndexTable[id];
    }
};  // namespace



TripodBossAccesser::TripodBossAccesser(const char* pName) : NameObj(pName), mBoss(), mPartsNum() {
}

TripodBoss* TripodBossAccesser::getTriPodBoss() const {
    return mBoss;
}

void TripodBossAccesser::setTriPodBoss(TripodBoss* pBoss) {
    mBoss = pBoss;
}

TripodBossAccesser* TripodBossAccesser::createSceneObj() {
    return static_cast< TripodBossAccesser* >(MR::createSceneObj(SceneObj_TripodBossAccesser));
}

void TripodBoss::getJointMatrix(TPos3f* pMtx, s32 a2) const {
    pMtx->set(*mBossBones[a2]._30);
}

namespace MR {

TripodBossAccesser* getTripodBossAccesser() {
        return MR::getSceneObj< TripodBossAccesser >(SceneObj_TripodBossAccesser);
    }

bool isCreatedTripodBoss() {
        if (!MR::isExistSceneObj(SceneObj_TripodBossAccesser)) {
            return false;
        }

        return getTripodBossAccesser()->getTriPodBoss() != nullptr;
    }

void getTripodBossJointMatrix(TPos3f* pMtx, s32 id) {
        getTripodBossAccesser()->getTriPodBoss()->getJointMatrix(pMtx, ::convertBoneIDToIndex(id));
    }

} // namespace MR
