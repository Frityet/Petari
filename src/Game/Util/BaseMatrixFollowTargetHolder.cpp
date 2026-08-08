#include "Game/Util/BaseMatrixFollowTargetHolder.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/JMapLinkInfo.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/ObjUtil.hpp"

BaseMatrixFollower::BaseMatrixFollower(NameObj* pObj, const JMapInfoIter& rIter)
    : mLinkInfo(nullptr), mFollowerObj(pObj), mFollowTarget(nullptr), mFollowID(-1) {
    MR::getJMapInfoFollowID(rIter, &mFollowID);
    mLinkInfo = new JMapLinkInfo(rIter, false);
}

NameObj* BaseMatrixFollower::getFollowTargetActor() const {
    return mFollowTarget->mActor;
}

void BaseMatrixFollower::calcFollowMatrix(TPos3f* pOut) const {
    pOut->set(mFollowTarget->getHostBaseMtx());
    pOut->concat(*pOut, mFollowTarget->_0);
}

bool BaseMatrixFollower::isEnableFollow() const {
    BaseMatrixFollowTarget* targ = mFollowTarget;

    if (targ == nullptr) {
        return false;
    }

    return targ->mActor != nullptr;
}

bool BaseMatrixFollower::isValid() const {
    return mFollowTarget->isValid(mFollowID);
}

BaseMatrixFollowTarget::BaseMatrixFollowTarget(const JMapLinkInfo* pInfo) : _30(nullptr), mActor(nullptr), mLinkInfo(pInfo), mValidater(nullptr) {
    _0.identity();
}

void BaseMatrixFollowTarget::set(LiveActor* pActor, const TPos3f& a2, const TPos3f* a3, BaseMatrixFollowValidater* pValidator) {
    mValidater = pValidator;
    mActor = pActor;
    _0.set(a2);
    _0.invert(_0);

    if (a3 != nullptr) {
        _30 = a3;
    }
}

const MtxPtr BaseMatrixFollowTarget::getHostBaseMtx() const {
    // MtxPtr and TPos3f are both 3 x 4 matrices, so this is safe.
    return _30 != nullptr ? (const MtxPtr)_30 : mActor->getBaseMtx();
}

bool BaseMatrixFollowTarget::isValid(s32 followId) const {
    if (mValidater != nullptr) {
        return mValidater->isValid(followId);
    }

    return true;
}

BaseMatrixFollowTargetHolder::BaseMatrixFollowTargetHolder(const char* pName, s32 targetCount, s32 followerCount) : NameObj(pName) {
    mTargets.init(targetCount);
    mFollowers.init(followerCount);
    MR::connectToSceneMapObjMovement(this);
}

void BaseMatrixFollowTargetHolder::initAfterPlacement() {
    for (u32 i = 0; i < mFollowers.size(); i++) {
        BaseMatrixFollower* follower = mFollowers[i];
        BaseMatrixFollowTarget* target = findFollowTarget(follower);

        if (target->mActor != nullptr) {
            follower->setGravityFollowHost(target->mActor);
        }
    }
}

void BaseMatrixFollowTargetHolder::movement() {
    for (u32 i = 0; i < mFollowers.size(); i++) {
        if (mFollowers[i]->isEnableFollow()) {
            mFollowers[i]->update();
        }
    }
}

void BaseMatrixFollowTargetHolder::addFollower(BaseMatrixFollower* pFollower) {
    mFollowers.push_back(pFollower);

    BaseMatrixFollowTarget* target = findFollowTarget(pFollower);
    if (target == nullptr) {
        target = new BaseMatrixFollowTarget(pFollower->mLinkInfo);
        mTargets.push_back(target);
    }

    pFollower->mFollowTarget = target;
}

void BaseMatrixFollowTargetHolder::setFollowTargetInfo(LiveActor* pActor, const JMapInfoIter& rIter, const TPos3f* pBaseMtx,
                                                       BaseMatrixFollowValidater* pValidater) {
    JMapLinkInfo linkInfo(rIter, true);
    BaseMatrixFollowTarget* target = findFollowTarget(&linkInfo);

    if (target != nullptr) {
        TPos3f placementMtx;
        MR::getJMapInfoMatrixFromRT(rIter, &placementMtx);
        target->set(pActor, placementMtx, pBaseMtx, pValidater);
    }
}

BaseMatrixFollowTarget* BaseMatrixFollowTargetHolder::findFollowTarget(const JMapLinkInfo* pLinkInfo) {
    if (pLinkInfo == nullptr) {
        return nullptr;
    }

    const JMapLinkInfo* targetLinkInfo;

    for (u32 i = 0; i < mTargets.size(); i++) {
        targetLinkInfo = mTargets[i]->mLinkInfo;
        bool isSameLink;

        if (!targetLinkInfo->isValid() || !pLinkInfo->isValid()) {
            isSameLink = false;
        } else {
            isSameLink = targetLinkInfo->_0 == pLinkInfo->_0 && targetLinkInfo->_4 == pLinkInfo->_4 && targetLinkInfo->_8 == pLinkInfo->_8;
        }

        if (isSameLink) {
            return mTargets[i];
        }
    }

    return nullptr;
}

BaseMatrixFollowTarget* BaseMatrixFollowTargetHolder::findFollowTarget(const BaseMatrixFollower* pFollower) {
    if (pFollower == nullptr) {
        return nullptr;
    }

    return findFollowTarget(pFollower->mLinkInfo);
}

namespace MR {
    bool isValidFollowID(const JMapInfoIter& rIter) {
        if (!MR::isValidInfo(rIter)) {
            return false;
        }

        JMapLinkInfo linkInfo(rIter, false);
        return linkInfo.isValid();
    }

    void addBaseMatrixFollower(BaseMatrixFollower* pFollower) {
        MR::createSceneObj(SceneObj_BaseMatrixFollowTargetHolder);
        MR::getSceneObj< BaseMatrixFollowTargetHolder >(SceneObj_BaseMatrixFollowTargetHolder)->addFollower(pFollower);
    }

    void addBaseMatrixFollowTarget(LiveActor* pActor, const JMapInfoIter& rIter, const TPos3f* pBaseMtx, BaseMatrixFollowValidater* pValidater) {
        if (!MR::isValidInfo(rIter)) {
            return;
        }

        if (MR::isExistSceneObj(SceneObj_BaseMatrixFollowTargetHolder)) {
            MR::getSceneObj< BaseMatrixFollowTargetHolder >(SceneObj_BaseMatrixFollowTargetHolder)
                ->setFollowTargetInfo(pActor, rIter, pBaseMtx, pValidater);
        }
    }
};  // namespace MR

void BaseMatrixFollower::update() {
}

BaseMatrixFollowTargetHolder::~BaseMatrixFollowTargetHolder() {
}
