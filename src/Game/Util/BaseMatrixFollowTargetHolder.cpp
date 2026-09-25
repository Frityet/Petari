#include "Game/Util/BaseMatrixFollowTargetHolder.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/JMapLinkInfo.hpp"
#include "Game/Util/JMapUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include <aurora/exception.hpp>
#include <stdexcept>

BaseMatrixFollower::BaseMatrixFollower(NameObj* pObj, const JMapInfoIter& rIter)
    : mLinkInfo(nullptr), mFollowerObj(pObj), mFollowTarget(nullptr), mFollowID(-1) {
    MR::getJMapInfoFollowID(rIter, &mFollowID);
    mLinkInfo = new JMapLinkInfo(rIter, false);
}

BaseMatrixFollower::~BaseMatrixFollower() {
    delete mLinkInfo;
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

BaseMatrixFollowTarget::BaseMatrixFollowTarget(const JMapLinkInfo* pInfo)
    : _30(nullptr), mActor(nullptr), mLinkInfo(nullptr), mValidater(nullptr),
      mNativeLinkInfo(pInfo != nullptr ? new JMapLinkInfo(*pInfo) : nullptr) {
    // Several followers may share this target; its link cannot borrow the
    // first follower's allocation after that follower has retired.
    mLinkInfo = mNativeLinkInfo.get();
    _0.identity();
}

BaseMatrixFollowTarget::~BaseMatrixFollowTarget() = default;

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
    std::unique_ptr< BaseMatrixFollower > follower(pFollower);
    if (mFollowers.size() == mFollowers.capacity()) {
        aurora::throw_host_exception< std::length_error >("BaseMatrix follower capacity exceeded");
    }

    BaseMatrixFollowTarget* target = findFollowTarget(pFollower);
    if (target == nullptr) {
        if (mTargets.size() == mTargets.capacity()) {
            aurora::throw_host_exception< std::length_error >("BaseMatrix follow target capacity exceeded");
        }
        auto newTarget = std::make_unique< BaseMatrixFollowTarget >(pFollower->mLinkInfo);
        target = newTarget.get();
        mTargets.push_back(newTarget.release());
    }

    pFollower->mFollowTarget = target;
    mFollowers.push_back(follower.release());
}

void BaseMatrixFollowTargetHolder::releaseNativeReference(const NameObj* pObject) noexcept {
    for (s32 i = 0; i < mFollowers.size();) {
        BaseMatrixFollower* follower = mFollowers[i];
        if (follower->mFollowerObj == pObject) {
            mFollowers.erase(mFollowers.begin() + i);
            delete follower;
        } else {
            ++i;
        }
    }
    for (s32 i = 0; i < mTargets.size(); ++i) {
        BaseMatrixFollowTarget* target = mTargets[i];
        if (target->mActor != pObject) {
            continue;
        }
        target->mActor = nullptr;
        target->_30 = nullptr;
        target->mValidater = nullptr;
        for (s32 j = 0; j < mFollowers.size(); ++j) {
            if (mFollowers[j]->mFollowTarget == target) {
                mFollowers[j]->setGravityFollowHost(nullptr);
            }
        }
    }
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
        std::unique_ptr< BaseMatrixFollower > follower(pFollower);
        MR::createSceneObj(SceneObj_BaseMatrixFollowTargetHolder);
        MR::getSceneObj< BaseMatrixFollowTargetHolder >(SceneObj_BaseMatrixFollowTargetHolder)->addFollower(follower.release());
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
    for (s32 i = mFollowers.size(); i > 0; --i) {
        delete mFollowers[i - 1];
    }
    mFollowers.clear();
    for (s32 i = mTargets.size(); i > 0; --i) {
        delete mTargets[i - 1];
    }
    mTargets.clear();
}
