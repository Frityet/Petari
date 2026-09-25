#include "Game/System/DrawBufferExecuter.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/ModelManager.hpp"
#include "Game/System/DrawBuffer.hpp"
#include "Game/Util/LightUtil.hpp"
#include <algorithm>
#include <aurora/exception.hpp>
#include <memory>
#include <stdexcept>

DrawBufferExecuter::DrawBufferExecuter(const char* pName, J3DModel* pModel, s32 a1)
    : mActors(nullptr), mMaxNumActors(0), mNumActors(0), mName(pName), mDrawBuffer(nullptr), mLightType(-1), mDrawBufferCount(0) {
    mDrawBuffer = new DrawBuffer(pModel);
}

DrawBufferExecuter::~DrawBufferExecuter() {
    if (mNativeModel) {
        GXDrawDone();
    }
    delete mDrawBuffer;
    delete[] mActors;
    mNativeModel.reset();
}

void DrawBufferExecuter::retainNativeModel(LiveActor* pActor) {
    if (pActor == nullptr || mNativeModel) {
        aurora::throw_host_exception< std::logic_error >("Draw prototype ownership can only be established once");
    }
    auto owner = pActor->retainNativeModel();
    if (!owner || owner.get() != pActor->mModelManager || owner->getJ3DModel() != mDrawBuffer->mModel) {
        aurora::throw_host_exception< std::invalid_argument >("Draw executer must retain its actual prototype ModelManager");
    }
    mNativeModel = std::move(owner);
}

void DrawBufferExecuter::allocateActorListBuffer() {
    if (mActors != nullptr) {
        aurora::throw_host_exception< std::logic_error >("Draw actor list is already allocated");
    }
    if (mDrawBufferCount > 0) {
        std::unique_ptr< LiveActor*[] > actors(new LiveActor*[mDrawBufferCount]());
        mDrawBuffer->init(mDrawBufferCount);
        mActors = actors.release();
        mMaxNumActors = mDrawBufferCount;
    }
}

void DrawBufferExecuter::add(LiveActor* pActor) {
    if (pActor == nullptr || mActors == nullptr || mNumActors >= mMaxNumActors) {
        aurora::throw_host_exception< std::length_error >("Draw actor list capacity exceeded or not allocated");
    }
    if (std::find(mActors, mActors + mNumActors, pActor) != mActors + mNumActors) {
        aurora::throw_host_exception< std::logic_error >("Actor is already active in its draw executer");
    }
    mDrawBuffer->add(pActor);
    mActors[mNumActors++] = pActor;
}

void DrawBufferExecuter::remove(LiveActor* pActor) {
    if (mActors == nullptr || mNumActors == 0) {
        aurora::throw_host_exception< std::logic_error >("Actor is not active in its draw executer");
    }
    LiveActor** it = std::find(mActors, mActors + mNumActors, pActor);
    if (it == mActors + mNumActors) {
        aurora::throw_host_exception< std::logic_error >("Actor is not active in its draw executer");
    }
    GXDrawDone();
    mDrawBuffer->remove(pActor);
    *it = mActors[--mNumActors];
    mActors[mNumActors] = nullptr;
}

void DrawBufferExecuter::findLightInfo(LiveActor* pActor) {
    MR::initActorLightInfoDrawBuffer(pActor, mDrawBuffer);
}

void DrawBufferExecuter::onExecuteLight(s32 lightType) {
    mLightType = lightType;
}
void DrawBufferExecuter::offExecuteLight() {
    mLightType = -1;
}

void DrawBufferExecuter::calcViewAndEntry() {
    for (s32 idx = 0; idx < mNumActors; idx++) {
        mActors[idx]->calcViewAndEntry();
    }
}

void DrawBufferExecuter::drawOpa() const {
    if (mLightType != -1) {
        MR::loadLight(mLightType);
    }
    mDrawBuffer->drawOpa();
}

void DrawBufferExecuter::drawXlu() const {
    if (mLightType != -1) {
        MR::loadLight(mLightType);
    }
    mDrawBuffer->drawXlu();
}
