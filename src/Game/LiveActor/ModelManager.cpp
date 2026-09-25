#include "Game/LiveActor/ModelManager.hpp"
#include "Game/Animation/BckCtrl.hpp"
#include "Game/Animation/BpkPlayer.hpp"
#include "Game/Animation/BrkPlayer.hpp"
#include "Game/Animation/BtkPlayer.hpp"
#include "Game/Animation/BtpPlayer.hpp"
#include "Game/Animation/BvaPlayer.hpp"
#include "Game/Animation/XanimeCore.hpp"
#include "Game/Animation/XanimePlayer.hpp"
#include "Game/Animation/XanimeResource.hpp"
#include "Game/LiveActor/DisplayListMaker.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "Game/System/ResourceHolderManager.hpp"
#include "Game/Util/MutexHolder.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include <JSystem/JKernel/JKRHeap.hpp>
#include <aurora/allocation.hpp>
#include <JSystem/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
#include <cstdio>
#include <aurora/exception.hpp>
#include <exception>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
    class LoadMutexRecovery final {
    public:
        LoadMutexRecovery() : mThread(OSGetCurrentThread()), mExceptions(std::uncaught_exceptions()) {
            mCount = mutex().thread == mThread ? mutex().count : 0;
        }
        ~LoadMutexRecovery() {
            if (std::uncaught_exceptions() > mExceptions) {
                while (mutex().thread == mThread && mutex().count > mCount) {
                    OSUnlockMutex(&mutex());
                }
            }
        }
    private:
        static OSMutex& mutex() { return MR::MutexHolder<0>::sMutex; }
        OSThread* mThread;
        int mExceptions;
        int mCount;
    };
}

struct ModelManager::NativeState {
    JKRHeap::Handle domain;
    std::shared_ptr<const void> modelResources;
    std::shared_ptr<const void> animationResources;
    std::vector<std::shared_ptr<void>> dependencies;
    J3DModel* createdModel = nullptr;
    XanimePlayer* createdPlayer = nullptr;
    XanimeResourceTable* createdResourceTable = nullptr;
};

namespace MR {
    J3DModel* newJ3DModel(const ResourceHolder*, const char*, J3DMdlFlag);
    XanimePlayer* newXanimePlayer(const ResourceHolder*, const char*, const ResourceHolder*, J3DMdlFlag, XanimeResourceTable*);
    XanimeResourceTable* newXanimeResourceTable(ResourceHolder*);
};

ModelManager::ModelManager()
    : mBtkPlayer(nullptr), mBrkPlayer(nullptr), mBtpPlayer(nullptr), mBpkPlayer(nullptr), mBvaPlayer(nullptr), mXanimeResourceTable(nullptr),
      mXanimePlayer(nullptr), mModel(nullptr), mModelResourceHolder(nullptr), mDisplayListMaker(nullptr) {
}

ModelManager::~ModelManager() {
    const aurora::allocation::HostAllocationScope host;
    if (mNativeState == nullptr) {
        return;
    }
    // Authored animators can replace this pointer while queued packets still
    // borrow the original model. Restore it before retiring their graphs.
    mXanimePlayer = mNativeState->createdPlayer;
    mNativeState->dependencies.clear();
    delete mNativeState->createdPlayer;
    delete mNativeState->createdResourceTable;
    delete mNativeState->createdModel;
    // Original raw child arrays retire with the retained Game heap. The
    // resource holder owns shared material-animation storage.
    mNativeState.reset();
}

std::shared_ptr<ModelManager> ModelManager::createNative(JKRHeap::Handle domain,
                                                       const char* model, const char* animation, bool createDL) {
    const aurora::allocation::HostAllocationScope host;
    if (!domain) {
        aurora::throw_host_exception<std::invalid_argument>("A ModelManager requires its actual retained Game heap");
    }
    if (!SingletonHolder<ResourceHolderManager>::get()) {
        aurora::throw_host_exception<std::invalid_argument>("ModelManager requires the original ResourceHolderManager");
    }
    auto state = std::make_unique<NativeState>();
    state->domain = domain;
    ModelManager* original;
    {
        const aurora::allocation::ClientAllocationScope game({true, true});
        original = new ModelManager();
    }
    original->mNativeState = std::move(state);
    auto manager = std::shared_ptr<ModelManager>(original, [](ModelManager* value) {
        const aurora::allocation::HostAllocationScope host;
        // Keep the object allocation alive through operator delete. Holding
        // this locally also lets weak references outlive the retired heap.
        const auto domain = value->nativeAllocationHeap();
        delete value;
    });
    {
        // File/resource waits may switch heaps on the main thread; inherit the
        // caller's selected heap without holding a heap-selection mutex here.
        const aurora::allocation::ClientAllocationScope game({true, true});
        LoadMutexRecovery recovery;
        manager->init(model, animation, createDL);
    }
    return manager;
}

void ModelManager::retainNativeDependency(std::shared_ptr<void> dependency) {
    const aurora::allocation::HostAllocationScope host;
    if (!dependency || !mNativeState) {
        aurora::throw_host_exception<std::invalid_argument>("Model lifetime dependency requires a retained native model");
    }
    mNativeState->dependencies.push_back(std::move(dependency));
}

JKRHeap::Handle ModelManager::nativeAllocationHeap() const noexcept {
    return mNativeState ? mNativeState->domain : nullptr;
}

void ModelManager::update() {
    XanimePlayer* pXanimePlayer = mXanimePlayer;

    if (pXanimePlayer != nullptr) {
        pXanimePlayer->updateBeforeMovement();
        pXanimePlayer->updateAfterMovement();
    }

    if (mBtkPlayer != nullptr) {
        mBtkPlayer->update();
    }

    if (mBrkPlayer != nullptr) {
        mBrkPlayer->update();
    }

    if (mBtpPlayer != nullptr) {
        mBtpPlayer->update();
    }

    if (mBpkPlayer != nullptr) {
        mBpkPlayer->update();
    }

    if (mBvaPlayer != nullptr) {
        mBvaPlayer->update();
    }
}

void ModelManager::calcAnim() {
    calc();
    updateDL(true);
}

void ModelManager::calcView() {
    getJ3DModel()->viewCalc();
}

void ModelManager::entry() {
    getJ3DModel()->entry();
}

void ModelManager::newDifferedDLBuffer() {
    mDisplayListMaker->newDifferedDisplayList();
}

void ModelManager::updateDL(bool isDiffDL) {
    OSLockMutex(&MR::MutexHolder< 0 >::sMutex);

    bool isUpdateDL = mDisplayListMaker != nullptr && mDisplayListMaker->isValidDiff();

    if (isUpdateDL) {
        mDisplayListMaker->update();

        if (mBtkPlayer != nullptr) {
            mBtkPlayer->beginDiff();
        }

        if (mBrkPlayer != nullptr) {
            mBrkPlayer->beginDiff();
        }

        if (mBtpPlayer != nullptr) {
            mBtpPlayer->beginDiff();
        }

        if (mBpkPlayer != nullptr) {
            mBpkPlayer->beginDiff();
        }
    }

    getJ3DModel()->calcMaterial();

    if (isUpdateDL) {
        if (isDiffDL) {
            mDisplayListMaker->diff();
        }

        if (mBtkPlayer != nullptr) {
            mBtkPlayer->endDiff();
        }

        if (mBrkPlayer != nullptr) {
            mBrkPlayer->endDiff();
        }

        if (mBtpPlayer != nullptr) {
            mBtpPlayer->endDiff();
        }

        if (mBpkPlayer != nullptr) {
            mBpkPlayer->endDiff();
        }
    }

    OSUnlockMutex(&MR::MutexHolder< 0 >::sMutex);
}

void ModelManager::startBck(const char* pBckName, const char* pActorAnimName) {
    mXanimePlayer->changeAnimationBck(pBckName);
    mXanimePlayer->changeInterpoleFrame(0);
    changeBckSetting(pBckName, pActorAnimName, nullptr);
}

void ModelManager::startBckWithInterpole(const char* pBckName, s32 interpole) {
    mXanimePlayer->changeAnimationBck(pBckName);
    mXanimePlayer->changeInterpoleFrame(0);
    mXanimePlayer->changeInterpoleFrame(interpole);
}

void ModelManager::startBtk(const char* pBtkName) {
    stopBtk();
    mBtkPlayer->start(pBtkName);
    mDisplayListMaker->onCurFlagBtk(mBtkPlayer->mAnmRes);
}

void ModelManager::startBrk(const char* pBrkName) {
    stopBrk();
    mBrkPlayer->start(pBrkName);
    mDisplayListMaker->onCurFlagBrk(mBrkPlayer->mAnmRes);
}

void ModelManager::startBtp(const char* pBtpName) {
    stopBtp();
    mBtpPlayer->start(pBtpName);
    mDisplayListMaker->onCurFlagBtp(mBtpPlayer->mAnmRes);
}

void ModelManager::startBpk(const char* pBpkName) {
    stopBpk();
    mBpkPlayer->start(pBpkName);
    mDisplayListMaker->onCurFlagBpk(mBpkPlayer->mAnmRes);
}

void ModelManager::startBva(const char* pBvaName) {
    stopBva();
    mBvaPlayer->start(pBvaName);
}

void ModelManager::stopBtk() {
    if (mBtkPlayer->mAnmRes != nullptr) {
        mDisplayListMaker->offCurFlagBtk(mBtkPlayer->mAnmRes);
        mBtkPlayer->stop();
    }
}

void ModelManager::stopBrk() {
    if (mBrkPlayer->mAnmRes != nullptr) {
        mDisplayListMaker->offCurFlagBrk(mBrkPlayer->mAnmRes);
        mBrkPlayer->stop();
    }
}

void ModelManager::stopBtp() {
    if (mBtpPlayer->mAnmRes != nullptr) {
        mDisplayListMaker->offCurFlagBtp(mBtpPlayer->mAnmRes);
        mBtpPlayer->stop();
    }
}

void ModelManager::stopBpk() {
    if (mBpkPlayer->mAnmRes != nullptr) {
        mDisplayListMaker->offCurFlagBpk(mBpkPlayer->mAnmRes);
        mBpkPlayer->stop();
    }
}

void ModelManager::stopBva() {
    if (mBvaPlayer->mAnmRes != nullptr) {
        mBvaPlayer->stop();
    }
}

J3DFrameCtrl* ModelManager::getBckCtrl() const {
    return mXanimePlayer->_20;
}

J3DFrameCtrl* ModelManager::getBtkCtrl() const {
    return &mBtkPlayer->mFrameCtrl;
}

J3DFrameCtrl* ModelManager::getBrkCtrl() const {
    return &mBrkPlayer->mFrameCtrl;
}

J3DFrameCtrl* ModelManager::getBtpCtrl() const {
    return &mBtpPlayer->mFrameCtrl;
}

J3DFrameCtrl* ModelManager::getBpkCtrl() const {
    return &mBpkPlayer->mFrameCtrl;
}

J3DFrameCtrl* ModelManager::getBvaCtrl() const {
    return &mBvaPlayer->mFrameCtrl;
}

bool ModelManager::isBckStopped() const {
    XanimeFrameCtrl* pCtrl = &mXanimePlayer->_24[mXanimePlayer->_54];
    return pCtrl->mState & 1;
}

bool ModelManager::isBtkStopped() const {
    if (mBtkPlayer != nullptr) {
        return mBtkPlayer->isStop();
    }

    return false;
}

bool ModelManager::isBrkStopped() const {
    if (mBrkPlayer != nullptr) {
        return mBrkPlayer->isStop();
    }

    return false;
}

bool ModelManager::isBtpStopped() const {
    if (mBtpPlayer != nullptr) {
        return mBtpPlayer->isStop();
    }

    return false;
}

bool ModelManager::isBpkStopped() const {
    if (mBpkPlayer != nullptr) {
        return mBpkPlayer->isStop();
    }

    return false;
}

bool ModelManager::isBvaStopped() const {
    if (mBvaPlayer != nullptr) {
        return mBvaPlayer->isStop();
    }

    return false;
}

bool ModelManager::isBtkPlaying(const char* pBtkName) const {
    return mBtkPlayer->isPlaying(pBtkName);
}

bool ModelManager::isBrkPlaying(const char* pBrkName) const {
    return mBrkPlayer->isPlaying(pBrkName);
}

bool ModelManager::isBpkPlaying(const char* pBpkName) const {
    return mBpkPlayer->isPlaying(pBpkName);
}

bool ModelManager::isBtpPlaying(const char* pBtpName) const {
    return mBtpPlayer->isPlaying(pBtpName);
}

bool ModelManager::isBvaPlaying(const char* pBvaName) const {
    return mBvaPlayer->isPlaying(pBvaName);
}

void ModelManager::initJointTransform() {
    mXanimePlayer->mCore->enableJointTransform(getJ3DModelData());
}

XjointTransform* ModelManager::getJointTransform(const char* pJointName) {
    s32 idx = getJ3DModel()->mModelData->mJointTree.mJointName->getIndex(pJointName);
    XjointTransform* pTransformList = mXanimePlayer->mCore->mTransformList;

    if (pTransformList == nullptr) {
        return nullptr;
    }

    return &pTransformList[idx];
}

ResourceHolder* ModelManager::getResourceHolder() const {
    if (mXanimeResourceTable == nullptr) {
        return mModelResourceHolder;
    }

    return mXanimeResourceTable->mResourceHolder;
}

ResourceHolder* ModelManager::getModelResourceHolder() const {
    return mModelResourceHolder;
}

J3DModel* ModelManager::getJ3DModel() const {
    if (mXanimePlayer == nullptr) {
        return mModel;
    }

    return mXanimePlayer->mModel;
}

J3DModelData* ModelManager::getJ3DModelData() const {
    return getJ3DModel()->mModelData;
}

const char* ModelManager::getPlayingBckName() const {
    if (mXanimePlayer != nullptr) {
        return mXanimePlayer->getCurrentBckName();
    }

    return nullptr;
}

void ModelManager::initModelAndAnimation(ResourceHolder* pModelResource, const char* pModelName, ResourceHolder* pAnimResource, J3DMdlFlag flags) {
    if (mNativeState != nullptr) {
        mNativeState->modelResources = pModelResource->retainNativeResources();
        mNativeState->animationResources = (pAnimResource->mMotionResTable->mCount == 0 ? pModelResource : pAnimResource)->retainNativeResources();
    }
    mModelResourceHolder = pModelResource;

    if (pAnimResource->mMotionResTable->mCount == 0) {
        mModel = MR::newJ3DModel(pModelResource, pModelName, flags);
    }
    else {
        mXanimeResourceTable = MR::newXanimeResourceTable(pAnimResource);
        if (mNativeState != nullptr) {
            mNativeState->createdResourceTable = mXanimeResourceTable;
        }
        mXanimePlayer = MR::newXanimePlayer(pModelResource, pModelName, pAnimResource, flags, mXanimeResourceTable);
    }
    if (mNativeState != nullptr) {
        mNativeState->createdModel = getJ3DModel();
        mNativeState->createdPlayer = mXanimePlayer;
    }
}

void ModelManager::initMaterialAnm() {
    ResourceHolder* pResourceHolder = getResourceHolder();
    J3DModelData* pModelData = getJ3DModelData();

    if (pResourceHolder->mMaterialBuf == nullptr) {
        pResourceHolder->newMaterialAnmBuffer(pModelData);
    }

    if (pResourceHolder->mBtkResTable->mCount != 0) {
        mBtkPlayer = new BtkPlayer(pResourceHolder, pModelData);
    }

    if (pResourceHolder->mBrkResTable->mCount != 0) {
        mBrkPlayer = new BrkPlayer(pResourceHolder, pModelData);
    }

    if (pResourceHolder->mBtpResTable->mCount != 0) {
        mBtpPlayer = new BtpPlayer(pResourceHolder, pModelData);
    }

    if (pResourceHolder->mBpkResTable->mCount != 0) {
        mBpkPlayer = new BpkPlayer(pResourceHolder, pModelData);
    }
}

void ModelManager::initVisibilityAnm() {
    ResourceHolder* pResourceHolder = getResourceHolder();

    if (pResourceHolder->mBvaResTable->mCount != 0) {
        mBvaPlayer = new BvaPlayer(pResourceHolder->mBvaResTable, getJ3DModel());
    }
}

void ModelManager::calc() {
    if (mXanimePlayer != nullptr) {
        mXanimePlayer->calcAnm(0);
    }

    if (mBvaPlayer != nullptr) {
        mBvaPlayer->calc();
    }

    OSLockMutex(&MR::MutexHolder< 0 >::sMutex);
    getJ3DModel()->calc();
    OSUnlockMutex(&MR::MutexHolder< 0 >::sMutex);

    if (mXanimePlayer != nullptr) {
        mXanimePlayer->clearAnm(0);
    }
}

void ModelManager::changeBckSetting(const char* pBckName, const char* pActorAnimName, XanimePlayer* pXanimePlayer) {
    if (pXanimePlayer == nullptr) {
        pXanimePlayer = mXanimePlayer;
    }

    BckCtrl* pBckCtrl = getResourceHolder()->mBckCtrl;

    if (pBckCtrl != nullptr) {
        if (pActorAnimName == nullptr) {
            pActorAnimName = pBckName;
        }

        pBckCtrl->changeBckSetting(pActorAnimName, pXanimePlayer);
    }
}

void ModelManager::init(const char* pModelName, const char* pAnimName, bool createDL) {
    char modelArchiveName[0x40];
    snprintf(modelArchiveName, sizeof(modelArchiveName), "%s.arc", pModelName);
    ResourceHolder* pModelResource = MR::createAndAddResourceHolder(modelArchiveName);

    ResourceHolder* pAnimResource;
    if (pAnimName == nullptr) {
        pAnimResource = pModelResource;
    }
    else {
        char animArchiveName[0x40];
        snprintf(animArchiveName, sizeof(animArchiveName), "%s.arc", pAnimName);
        pAnimResource = MR::createAndAddResourceHolder(animArchiveName);
    }

    bool hasMaterialAnm = pAnimResource->isExistMaterialAnm();
    bool hasDiffMaterial = DisplayListMaker::isExistDiffMaterial(static_cast< J3DModelData* >(pModelResource->mModelResTable->getRes(pModelName)));

    J3DMdlFlag flags = J3DMdlFlag_UseSharedDL;
    if (createDL || hasMaterialAnm || hasDiffMaterial) {
        flags = static_cast< J3DMdlFlag >(flags | J3DMdlFlag_DifferedDLBuffer);
    }

    initModelAndAnimation(pModelResource, pModelName, pAnimResource, flags);
    initVisibilityAnm();

    if (hasMaterialAnm) {
        initMaterialAnm();
    }

    if (flags & J3DMdlFlag_DifferedDLBuffer) {
        mDisplayListMaker = new DisplayListMaker(getJ3DModel(), getResourceHolder());

        if (!createDL) {
            mDisplayListMaker->newDifferedDisplayList();
        }
    }
}
