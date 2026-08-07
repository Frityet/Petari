#include "Game/LiveActor/LodCtrl.hpp"

#include "Game/LiveActor/ActorLightCtrl.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/ModelObj.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/Util/CameraUtil.hpp"
#include "Game/Util/FileUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/PlayerUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "render/live_actor/LiveActorModel.hpp"

#include <cstdio>
#include <string>
#include <string_view>

namespace {
    constexpr bool cDefaultViewState = false;

    MtxPtr actorBaseMtx(const LiveActor *pActor) {
        if (pActor == nullptr) {
            return nullptr;
        }

        const auto &matrix = pActor->getBaseMatrix();
        return reinterpret_cast<MtxPtr>(const_cast<f32 *>(matrix.m.data()));
    }

    bool isViewStateEnabled(const bool *pState) {
        return pState != nullptr && *pState;
    }

    template <typename Function>
    void forEachLodActor(LodCtrl *pCtrl, Function function) {
        if (pCtrl == nullptr) {
            return;
        }

        if (pCtrl->mActor != nullptr) {
            function(pCtrl->mActor);
        }
        if (pCtrl->_10 != nullptr) {
            function(pCtrl->_10);
        }
        if (pCtrl->_14 != nullptr) {
            function(pCtrl->_14);
        }
    }

    void copyTransform(const LiveActor *pSource, LiveActor *pDestination) {
        if (pSource == nullptr || pDestination == nullptr) {
            return;
        }

        pDestination->mPosition = pSource->mPosition;
        pDestination->mRotation = pSource->mRotation;
        pDestination->mScale = pSource->mScale;
    }

    void copyLight(ActorLightCtrl *pDestination, const ActorLightCtrl *pSource) {
        if (pDestination != nullptr && pSource != nullptr) {
            pDestination->copy(pSource);
        }
    }

    void makeDeadIfAlive(LiveActor *pActor) {
        if (pActor != nullptr && !MR::isDead(pActor)) {
            pActor->makeActorDead();
        }
    }

    void syncMaterialAnimationTo(LiveActor *pDestination, const LiveActor *pSource) {
        if (pDestination == nullptr || pSource == nullptr) {
            return;
        }

        if (const auto name = pSource->currentBrkName(); !name.empty()) {
            pDestination->startBrk(std::string(name).c_str());
        }
        if (const auto name = pSource->currentBtkName(); !name.empty()) {
            pDestination->startBtk(std::string(name).c_str());
        }
    }

    void syncJointAnimationTo(LiveActor *pDestination, const LiveActor *pSource) {
        if (pDestination == nullptr || pSource == nullptr) {
            return;
        }

        if (const auto name = pSource->currentBckName(); !name.empty()) {
            pDestination->startBck(std::string(name).c_str(), nullptr);
        }
    }
}  // namespace

LodCtrl::LodCtrl(LiveActor *pActor, const JMapInfoIter &)
    : _0(2000.0F), _4(3000.0F), _8(pActor), mActor(pActor), _10(nullptr), _14(nullptr), _18(0), _19(0), _1A(1), _1B(0),
      _1C(&cDefaultViewState), _20(&cDefaultViewState), _24(&cDefaultViewState), _28(&cDefaultViewState), mViewGroupID(-1),
      mActorLightCtrl(pActor != nullptr ? pActor->mActorLightCtrl : nullptr) {
}

void LodCtrl::offSyncShadowHost() {
    _1A = 0;
}

void LodCtrl::appear() {
    if (mActor == nullptr) {
        return;
    }

    MR::showModel(mActor);
    mActorLightCtrl = mActor->mActorLightCtrl;
    makeDeadIfAlive(_10);
    makeDeadIfAlive(_14);
}

void LodCtrl::kill() {
    if (mActor != nullptr) {
        MR::showModel(mActor);
    }
    makeDeadIfAlive(_10);
    makeDeadIfAlive(_14);
    mActorLightCtrl = nullptr;
}

void LodCtrl::validate() {
    appear();
    _18 = 1;
}

void LodCtrl::invalidate() {
    kill();
    _18 = 0;
}

void LodCtrl::update() {
    if (MR::isDead(mActor) || !_18) {
        return;
    }

    if (_10 == nullptr && _14 == nullptr) {
        if (isViewStateEnabled(_28)) {
            hideAllModel();
        } else {
            showHighModel();
        }
        return;
    }

    const f32 distance = calcDistanceToCamera();
    if (isViewStateEnabled(_28)) {
        hideAllModel();
    } else if (isViewStateEnabled(_1C)) {
        showHighModel();
    } else if (_10 != nullptr && isViewStateEnabled(_20)) {
        showMiddleModel();
    } else if (_14 != nullptr && isViewStateEnabled(_24)) {
        showLowModel();
    } else if (distance < _0) {
        showHighModel();
    } else if (_10 == nullptr && distance < _4) {
        showHighModel();
    } else if (_10 != nullptr && distance < _4) {
        showMiddleModel();
    } else if (_14 != nullptr) {
        showLowModel();
    }

    if (_8 != nullptr && _8 != mActor) {
        copyTransform(mActor, _8);
    }
}

bool LodCtrl::isShowLowModel() const {
    return _14 != nullptr && _14 == _8;
}

void LodCtrl::setDistanceToMiddle(f32 distance) {
    _0 = distance;
}

f32 LodCtrl::calcDistanceToCamera() const {
    if (mActor == nullptr) {
        return 0.0F;
    }
    if (_1B) {
        return MR::calcCameraDistanceZ(mActor->mPosition);
    }
    return MR::calcDistanceToPlayer(mActor->mPosition);
}

void LodCtrl::setDistanceToLow(f32 distance) {
    _4 = distance;
}

void LodCtrl::setDistanceToMiddleAndLow(f32 middleDistance, f32 lowDistance) {
    _0 = middleDistance;
    _4 = lowDistance;
}

void LodCtrl::setClippingTypeSphereContainsModelBoundingBox(f32 radius) {
    forEachLodActor(this, [radius](LiveActor *pActor) { MR::setClippingTypeSphere(pActor, radius); });
}

void LodCtrl::setFarClipping(f32 distance) {
    forEachLodActor(this, [distance](LiveActor *pActor) { MR::setClippingFar(pActor, distance); });
}

void LodCtrl::invalidateClipping() {
    forEachLodActor(this, [](LiveActor *pActor) { MR::invalidateClipping(pActor); });
}

void LodCtrl::showHighModel() {
    if (mActor == nullptr) {
        _8 = nullptr;
        return;
    }

    if (MR::isHiddenModel(mActor)) {
        copyLight(mActor->mActorLightCtrl, mActorLightCtrl);
        mActorLightCtrl = mActor->mActorLightCtrl;
        MR::showModel(mActor);
    } else {
        makeDeadIfAlive(_10);
        makeDeadIfAlive(_14);
    }
    _8 = mActor;
}

void LodCtrl::showMiddleModel() {
    if (_10 == nullptr) {
        showHighModel();
        return;
    }

    if (MR::isDead(_10)) {
        copyLight(_10->mActorLightCtrl, mActorLightCtrl);
        mActorLightCtrl = _10->mActorLightCtrl;
        _10->makeActorAppeared();
        if (mActor != nullptr) {
            mActor->calcAnim();
        }
    } else {
        if (mActor != nullptr && !MR::isHiddenModel(mActor)) {
            MR::hideModel(mActor);
        }
        makeDeadIfAlive(_14);
    }
    _8 = _10;
}

void LodCtrl::showLowModel() {
    if (_14 == nullptr) {
        showHighModel();
        return;
    }

    if (MR::isDead(_14)) {
        copyLight(_14->mActorLightCtrl, mActorLightCtrl);
        mActorLightCtrl = _14->mActorLightCtrl;
        _14->makeActorAppeared();
        if (mActor != nullptr) {
            mActor->calcAnim();
        }
    } else {
        if (mActor != nullptr && !MR::isHiddenModel(mActor)) {
            MR::hideModel(mActor);
        }
        makeDeadIfAlive(_10);
    }
    _8 = _14;
}

void LodCtrl::hideAllModel() {
    if (mActor != nullptr && !MR::isHiddenModel(mActor)) {
        MR::hideModel(mActor);
    }
    makeDeadIfAlive(_10);
    makeDeadIfAlive(_14);
    _8 = nullptr;
}

void LodCtrl::setViewCtrlPtr(const bool *pHigh, const bool *pMiddle, const bool *pLow, const bool *pHidden) {
    _1C = pHigh != nullptr ? pHigh : &cDefaultViewState;
    _20 = pMiddle != nullptr ? pMiddle : &cDefaultViewState;
    _24 = pLow != nullptr ? pLow : &cDefaultViewState;
    _28 = pHidden != nullptr ? pHidden : &cDefaultViewState;
}

void LodCtrl::createLodModel(int drawBufferType, int movementType, int calcAnimType) {
    _10 = initLodModel(drawBufferType, movementType, calcAnimType, false);
    _14 = initLodModel(drawBufferType, movementType, calcAnimType, true);
    if (_10 != nullptr || _14 != nullptr) {
        appear();
        _18 = 1;
    } else {
        kill();
        _18 = 0;
    }
}

void LodCtrl::syncMaterialAnimation() {
    syncMaterialAnimationTo(_10, mActor);
    syncMaterialAnimationTo(_14, mActor);
}

void LodCtrl::syncJointAnimation() {
    syncJointAnimationTo(_10, mActor);
    syncJointAnimationTo(_14, mActor);
}

void LodCtrl::initLightCtrl() {
    if (_10 != nullptr) {
        MR::initLightCtrl(_10);
    }
    if (_14 != nullptr) {
        MR::initLightCtrl(_14);
    }
}

ModelObj *LodCtrl::initLodModel(int drawBufferType, int movementType, int calcAnimType, bool isLowModel) const {
    const auto *pModel = smgpc::compat::actor_model(mActor);
    if (mActor == nullptr || pModel == nullptr || pModel->model_arc_name().empty()) {
        return nullptr;
    }

    const auto resourceName = std::string(pModel->model_arc_name());
    const auto type = std::string_view(isLowModel ? "Low" : "Middle");
    const auto archivePath = "/ObjectData/" + resourceName + std::string(type) + ".arc";
    if (!MR::isFileExist(archivePath.c_str(), false)) {
        return nullptr;
    }

    const auto objectName = std::string(mActor->getName()) + "（" + std::string(type) + "）";
    const auto lodResourceName = resourceName + std::string(type);
    auto *pObject =
        new ModelObj(objectName.c_str(), lodResourceName.c_str(), actorBaseMtx(mActor), drawBufferType, movementType, calcAnimType, false);
    pObject->initWithoutIter();
    pObject->makeActorDead();
    MR::setClippingTypeSphere(pObject, 100.0F);
    copyTransform(mActor, pObject);
    return pObject;
}

bool LodCtrlFunction::isExistLodLowModel(const char *pName) {
    if (pName == nullptr) {
        return false;
    }

    char path[0x100];
    std::snprintf(path, sizeof(path), "/ObjectData/%sLow.arc", pName);
    return MR::isFileExist(path, false);
}

namespace MR {
    LodCtrl *createLodCtrlNPC(LiveActor *pActor, const JMapInfoIter &rIter) {
        auto *pLod = new LodCtrl(pActor, rIter);
        pLod->createLodModel(MR::DrawBufferType_NPC, MR::MovementType_NPC, -1);
        pLod->syncMaterialAnimation();
        pLod->syncJointAnimation();
        pLod->initLightCtrl();
        pLod->offSyncShadowHost();
        pLod->_1B = true;
        return pLod;
    }
}  // namespace MR
