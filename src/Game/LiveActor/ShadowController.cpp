#include "Game/LiveActor/ShadowController.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/ShadowDrawer.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Scene/SceneFunction.hpp"

ShadowControllerHolder::ShadowControllerHolder() : NameObj("影管理"), _C(), _18(), _24(false) {
    mFarClip = 4000.0f;
    _C.init(0x500);
    _18.init(0x400);
    MR::connectToScene(this, MR::MovementType_ShadowControllerHolder, -1, -1, -1);

    if (MR::isEqualStageName("AstroGalaxy") || MR::isEqualStageName("PeachCastleGardenGalaxy") || MR::isEqualStageName("PeachCastleFinalGalaxy")) {
        _24 = true;
    }
}

ShadowController::ShadowController(LiveActor* pActor, const char* pName)
    : mActor(pActor), mName(pName), mGroupName(""), mDrawer(nullptr), mProjectedSensor(nullptr), mCollisionPartsFilter(nullptr), _18(nullptr),
      _1C(nullptr), mDropPos(nullptr), mDropDir(nullptr), mProjPos(nullptr), mProjNorm(nullptr), _30(0.0f, 0.0f, 0.0f),
      _3C(0.0f, -1.0f, 0.0f), _48(0.0f, 0.0f, 0.0f), _54(0.0f, 1.0f, 0.0f), _60(1), _61(0), _62(0), _63(0), _64(0), _65(0),
      _66(0), _67(0), mStartOffset(50.0f), mDropLength(0.0f), _70(0), _71(1), _72(1) {
    MR::createSceneObj(SceneObj_ShadowControllerHolder);
    MR::addShadowController(this);
}

void ShadowControllerHolder::movement() {
    updateController();
}

void ShadowControllerHolder::initAfterPlacement() {
    ShadowController* pController;
    int size = _C.size();

    for (u32 i = 0; i < size; i++) {
        pController = _C[i];

        pController->updateDirection();
        pController->updateProjection();
    }
}

void ShadowControllerHolder::updateController() {
    int size = _18.size();

    for (u32 i = 0; i < size; i++) {
        if (_24) {
            _18[i]->updateFarClipping(mFarClip);
        }

        _18[i]->update();
    }

    _18.clear();
}

ShadowControllerList::ShadowControllerList(LiveActor* pActor, u32 listCount) : mShadowList(), mHost(pActor) {
    mShadowList.init(listCount);
}

void ShadowControllerList::addController(ShadowController* pController) {
    mShadowList.push_back(pController);
}

u32 ShadowControllerList::getControllerCount() const {
    return mShadowList.size();
}

ShadowController* ShadowControllerList::getController(u32 idx) const {
    return mShadowList[idx];
}

ShadowController* ShadowControllerList::getController(const char* pName) const {
    if (mShadowList.size() == 1) {
        return mShadowList[0];
    }

    for (u32 i = 0; i < mShadowList.size(); i++) {
        if (MR::isEqualString(pName, mShadowList[i]->mName)) {
            return mShadowList[i];
        }
    }

    return nullptr;
}

void ShadowControllerList::resetCalcCount() {
    for (u32 i = 0; i < mShadowList.size(); i++) {
        ShadowController* controller = mShadowList[i];
        controller->_65 = 0;
        controller->_66 = 0;
    }
}

void ShadowControllerList::requestCalc() {
    for (u32 i = 0; i < mShadowList.size(); i++) {
        mShadowList[i]->requestCalc();
    }
}

void ShadowController::requestCalc() {
    if (!_64) {
        _64 = 1;
        appendToHolder();
    }
}

void ShadowController::update() {
    if (isDraw()) {
        updateDirection();
        updateProjection();
    }

    _64 = 0;
}

void ShadowController::updateFarClipping(f32 clip) {
    TVec3f dropPos;
    getDropPos(&dropPos);
    f32 cameraDistZ = MR::calcCameraDistanceZ(dropPos);
    _67 = cameraDistZ >= clip;
}

void ShadowController::updateDirection() {
    if (isCalcGravity()) {
        TVec3f dropPos;
        getDropPos(&dropPos);

        TVec3f currentDir(_3C);

        if (!isCalcShadowGravity() || !MR::calcDropShadowVectorOrZero(mActor, dropPos, &_3C, nullptr, 0)) {
            if (!MR::calcGravityVectorOrZero(mActor, dropPos, &_3C, nullptr, 0)) {
                _3C.set< f32 >(currentDir);
            }
        }

        if (_61 == 2 || _61 == 5) {
            _66++;
        }
    }
}

void ShadowController::updateProjection() {
    if (isCalcCollision()) {
        Triangle triangle;
        TVec3f dropPos;
        TVec3f dropDir;

        getDropPos(&dropPos);
        getDropDir(&dropDir);

        TVec3f startOffset = dropDir * mStartOffset;
        dropPos -= startOffset;

        switch (_62) {
        case 0: {
            TVec3f dropOffset = dropDir * (mDropLength + mStartOffset);
            _63 = MR::getFirstPolyOnLineToMap(&_48, &triangle, dropPos, dropOffset, mCollisionPartsFilter, nullptr);
            break;
        }
        case 1: {
            TVec3f dropOffset = dropDir * (mDropLength + mStartOffset);
            _63 = MR::getFirstPolyOnLineToWaterSurface(&_48, &triangle, dropPos, dropOffset, mCollisionPartsFilter, nullptr);
            break;
        }
        }

        if (_63) {
            mProjectedSensor = triangle.mSensor;
            _54.set< f32 >(*triangle.getNormal(0));
        } else {
            mProjectedSensor = nullptr;
        }

        if (_60 == 2) {
            _65++;
        }
    }
}

ShadowDrawer* ShadowController::getShadowDrawer() {
    return mDrawer;
}

LiveActor* ShadowController::getHost() const {
    return mActor;
}

void ShadowController::setShadowDrawer(ShadowDrawer* pDrawer) {
    mDrawer = pDrawer;
    pDrawer->setShadowController(this);
}

void ShadowController::getDropPos(TVec3f* pOut) const {
    if (mDropPos) {
        pOut->set< f32 >(*mDropPos);
    } else {
        if (_1C) {
            PSMTXMultVec(_1C, (const Vec*)&_30, (Vec*)pOut);
        } else {
            pOut->set< f32 >(_30);
        }
    }
}

void ShadowController::getDropDir(TVec3f* pOut) const {
    if (mDropDir) {
        pOut->set< f32 >(*mDropDir);
    } else {
        pOut->set< f32 >(_3C);
    }
}

f32 ShadowController::getDropLength() const {
    return mDropLength;
}

void ShadowController::getProjectionPos(TVec3f* pOut) const {
    if (mProjPos) {
        pOut->set< f32 >(*mProjPos);
    } else {
        pOut->set< f32 >(_48);
    }
}

void ShadowController::getProjectionNormal(TVec3f* pOut) const {
    if (mProjNorm) {
        pOut->set< f32 >(*mProjNorm);
    } else {
        pOut->set< f32 >(_54);
    }
}

f32 ShadowController::getProjectionLength() const {
    if (!isProjected()) {
        return -1.0f;
    }

    TVec3f dropPos;
    getDropPos(&dropPos);

    TVec3f dropDir;
    getDropDir(&dropDir);

    TVec3f projOffset(_48);
    projOffset -= dropPos;

    if (dropDir.dot(projOffset) < 0.0f) {
        return 0.0f;
    }

    return PSVECDistance(&dropPos, &_48);
}

bool ShadowController::isProjected() const {
    return _63 != 0;
}

bool ShadowController::isDraw() const {
    if (_67) {
        return false;
    }

    if (!_71) {
        return false;
    }

    if (!_72) {
        return true;
    }

    return MR::isValidDraw(mActor);
}

bool ShadowController::isCalcCollision() const {
    if (!_60) {
        return false;
    }

    if (_60 == 2) {
        return _65 < 1;
    }

    return true;
}

bool ShadowController::isCalcGravity() const {
    if (!_61 || _61 == 3) {
        return false;
    }

    if (!_61) {
        return _66 < 1;
    }

    return true;
}

bool ShadowController::isCalcShadowGravity() const {
    return static_cast< u8 >(_61 + 0xFC) <= 1;
}

void ShadowController::setGroupName(const char* pName) {
    mGroupName = pName;
}

void ShadowController::setDropPosPtr(const TVec3f* pDropPos) {
    mDropPos = pDropPos;
    _1C = 0;
}

void ShadowController::setDropPosMtxPtr(MtxPtr pMtx, const TVec3f& rPos) {
    _18 = pMtx;
    mDropPos = nullptr;
    _1C = pMtx;
    _30.set< f32 >(rPos);
}

void ShadowController::setDropPosFix(const TVec3f& rPos) {
    _30.set< f32 >(rPos);
    mDropPos = 0;
    _1C = 0;
}

void ShadowController::setDropDirPtr(const TVec3f* pDropDir) {
    mDropDir = pDropDir;
    _61 = 0;
}

void ShadowController::setProjectionPtr(const TVec3f* pProjPos, const TVec3f* pProjNorm) {
    mProjPos = const_cast< TVec3f* >(pProjPos);
    mProjNorm = const_cast< TVec3f* >(pProjNorm);
    _60 = 0;
    _63 = 1;
    mProjectedSensor = nullptr;
}

void ShadowController::setDropDirFix(const TVec3f& a1) {
    _3C.set< f32 >(a1);
    mDropDir = 0;
    _61 = 0;
}

void ShadowController::setDropLength(f32 len) {
    mDropLength = len;
}

void ShadowController::setDropStartOffset(f32 offs) {
    mStartOffset = offs;
}

void ShadowController::setDropTypeNormal() {
    _62 = 0;
}

void ShadowController::setDropTypeSurface() {
    _62 = 1;
}

void ShadowController::setProjectionFix(const TVec3f& a1, const TVec3f& a2, bool a3) {
    _48.set< f32 >(a1);
    _54.set< f32 >(a2);
    _63 = a3;
    mProjectedSensor = 0;
}

void ShadowController::onCalcCollision() {
    _60 = 1;
}

void ShadowController::offCalcCollision() {
    _60 = 0;
}

void ShadowController::onCalcCollisionOneTime() {
    _60 = 2;
    _65 = 0;
}

void ShadowController::onCalcDropGravity() {
    TVec3f vec(0.0f, 1.0f, 0.0f);
    _3C.set< f32 >(vec);
    mDropDir = 0;
    _61 = 1;
}

void ShadowController::onCalcDropGravityOneTime() {
    TVec3f vec(0.0f, 1.0f, 0.0f);
    _3C.set< f32 >(vec);
    mDropDir = 0;
    _61 = 2;
    _66 = 0;
}

void ShadowController::offCalcDropGravity() {
    _61 = 0;
}

void ShadowController::onCalcDropPrivateGravity() {
    TVec3f vec(0.0f, 1.0f, 0.0f);
    _3C.set< f32 >(vec);
    mDropDir = 0;
    _61 = 4;
}

void ShadowController::onCalcDropPrivateGravityOneTime() {
    TVec3f vec(0.0f, 1.0f, 0.0f);
    _3C.set< f32 >(vec);
    mDropDir = 0;
    _61 = 5;
    _66 = 0;
}

void ShadowController::offCalcDropPrivateGravity() {
    _61 = 3;
}

void ShadowController::setCollisionPartsFilter(CollisionPartsFilterBase* pBase) {
    mCollisionPartsFilter = pBase;
}

void ShadowController::onFollowHostScale() {
    _70 = 1;
}

void ShadowController::offFollowHostScale() {
    _70 = 0;
}

bool ShadowController::isFollowHostScale() const {
    return _70;
}

void ShadowController::onVisibleSyncHost() {
    _72 = 1;
}

void ShadowController::offVisibleSyncHost() {
    _72 = 0;
}

void ShadowController::validate() {
    _71 = 1;
}

void ShadowController::invalidate() {
    _71 = 0;
}

namespace MR {
    void addShadowController(ShadowController* pController) {
        MR::getSceneObj< ShadowControllerHolder >(SceneObj_ShadowControllerHolder)->_C.push_back(pController);
    }

    void requestCalcActorShadowAppear(LiveActor* pActor) {
        if (!MR::isInitializeStatePlacementSomething()) {
            if (pActor->mShadowControllerList) {
                pActor->mShadowControllerList->resetCalcCount();
            }

            if (pActor->mShadowControllerList) {
                pActor->mShadowControllerList->requestCalc();
            }
        }
    }

    void requestCalcActorShadow(LiveActor* pActor) {
        if (pActor->mShadowControllerList) {
            pActor->mShadowControllerList->requestCalc();
        }
    }
};  // namespace MR

ShadowControllerHolder::~ShadowControllerHolder() {}
