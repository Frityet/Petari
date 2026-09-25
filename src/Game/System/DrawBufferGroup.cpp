#include "Game/System/DrawBufferGroup.hpp"
#include "Game/System/DrawBufferExecuter.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/LightUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ModelUtil.hpp"
#include "Game/Util/StringUtil.hpp"
#include <JSystem/J3DGraphBase/J3DSys.hpp>
#include <algorithm>
#include <aurora/exception.hpp>
#include <limits>
#include <memory>
#include <stdexcept>

DrawBufferGroup::DrawBufferGroup() : mExecutors(), mActiveExecutors(), mDrawCameraType(0), mLightType(-1), mLightLoadType(-1), mActorListsAllocated(false) {
}

DrawBufferGroup::~DrawBufferGroup() {
    for (s32 idx = 0; idx < mExecutors.size(); idx++) {
        delete mExecutors[idx];
    }
}

void DrawBufferGroup::init(s32 count) {
    if (count < 0 || mExecutors.mArray.mArr != nullptr || mActiveExecutors.mArray.mArr != nullptr) {
        aurora::throw_host_exception< std::logic_error >("Invalid draw group initialization");
    }
    std::unique_ptr< DrawBufferExecuter*[] > executers(new DrawBufferExecuter*[count]());
    std::unique_ptr< DrawBufferExecuter*[] > active(new DrawBufferExecuter*[count]());
    mExecutors.mArray.mArr = executers.release();
    mExecutors.mArray.mMaxSize = count;
    mActiveExecutors.mArray.mArr = active.release();
    mActiveExecutors.mArray.mMaxSize = count;
}

DrawBufferExecuter* DrawBufferGroup::requireNativeExecuter(s32 index) const {
    if (index < 0 || index >= mExecutors.size()) {
        aurora::throw_host_exception< std::out_of_range >("Draw executer index is outside its original group");
    }
    return mExecutors[index];
}

bool DrawBufferGroup::isExecutorEmpty(int index) const {
    return requireNativeExecuter(index)->mNumActors == 0;
}

s32 DrawBufferGroup::registerDrawBuffer(LiveActor* pActor) {
    if (pActor == nullptr || mExecutors.mArray.mArr == nullptr || mActorListsAllocated) {
        aurora::throw_host_exception< std::logic_error >("Original model registration must precede actor-list allocation");
    }
    if (pActor->mModelManager == nullptr) {
        aurora::throw_host_exception< std::invalid_argument >("Draw registration requires an actor model");
    }
    const char* pModelName = MR::getModelResName(pActor);
    s32 idx = findExecuterIndex(pModelName);

    if (idx < 0) {
        if (mExecutors.size() >= mExecutors.capacity()) {
            aurora::throw_host_exception< std::length_error >("Original draw executer category capacity exceeded");
        }
        auto exec = std::make_unique< DrawBufferExecuter >(pModelName, MR::getJ3DModel(pActor), 0x10);
        exec->retainNativeModel(pActor);

        if (mLightLoadType == -1) {
            exec->onExecuteLight(mLightType);
        }
        idx = mExecutors.size();
        mExecutors.push_back(exec.release());
    }

    if (mExecutors[idx]->mDrawBufferCount == std::numeric_limits< s32 >::max()) {
        aurora::throw_host_exception< std::length_error >("Original draw actor count overflow");
    }
    mExecutors[idx]->mDrawBufferCount++;
    return idx;
}

void DrawBufferGroup::allocateActorListBuffer() {
    if (mActorListsAllocated) {
        aurora::throw_host_exception< std::logic_error >("Draw group actor lists are already allocated");
    }
    mActorListsAllocated = true;
    for (s32 idx = 0; idx < mExecutors.size(); idx++) {
        mExecutors[idx]->allocateActorListBuffer();
    }
}

void DrawBufferGroup::active(LiveActor* pActor, s32 index) {
    DrawBufferExecuter* exec = requireNativeExecuter(index);
    bool isEmpty = exec->mNumActors == 0;
    if (!mActorListsAllocated || (isEmpty && mActiveExecutors.size() >= mActiveExecutors.capacity())) {
        aurora::throw_host_exception< std::length_error >("Original active draw executer capacity exceeded or not allocated");
    }
    exec->add(pActor);
    if (isEmpty) {
        mActiveExecutors.push_back(exec);
    }
}

void DrawBufferGroup::deactive(LiveActor* pActor, s32 index) {
    DrawBufferExecuter* exec = requireNativeExecuter(index);
    s32 activeIndex = -1;
    for (s32 idx = 0; idx < mActiveExecutors.size(); idx++) {
        if (mActiveExecutors[idx] == exec) {
            activeIndex = idx;
            break;
        }
    }
    if (activeIndex < 0) {
        aurora::throw_host_exception< std::logic_error >("Draw executer is not active in its original group");
    }
    exec->remove(pActor);
    if (exec->mNumActors == 0) {
        mActiveExecutors[activeIndex] = mActiveExecutors[mActiveExecutors.size() - 1];
        mActiveExecutors.mCount--;
    }
}

void DrawBufferGroup::findLightInfo(LiveActor* pActor, s32 index) {
    DrawBufferExecuter* exec = requireNativeExecuter(index);
    MR::initActorLightInfoLightType(pActor, mLightType);
    exec->findLightInfo(pActor);

    if (mLightLoadType != -1) {
        for (u32 i = 0; i < mExecutors.size(); i++) {
            mExecutors[i]->onExecuteLight(mLightType);
        }

        mLightLoadType = -1;
    }

    exec->offExecuteLight();
}

void DrawBufferGroup::entry() {
    if (mActiveExecutors.size() != 0) {
        std::for_each(mActiveExecutors.begin(), mActiveExecutors.end(), std::mem_func(&DrawBufferExecuter::calcViewAndEntry));
    }
}

void DrawBufferGroup::drawOpa() const {
    if (mActiveExecutors.size() != 0) {
        j3dSys.mDrawMode = 3;
        if (mLightLoadType != -1) {
            MR::loadLight(mLightLoadType);
        }
        std::for_each(mActiveExecutors.begin(), mActiveExecutors.end(), std::mem_func(&DrawBufferExecuter::drawOpa));
    }
}

void DrawBufferGroup::drawXlu() const {
    if (mActiveExecutors.size() != 0) {
        j3dSys.mDrawMode = 4;
        if (mLightLoadType != -1) {
            MR::loadLight(mLightLoadType);
        }
        std::for_each(mActiveExecutors.begin(), mActiveExecutors.end(), std::mem_func(&DrawBufferExecuter::drawXlu));
    }
}

void DrawBufferGroup::setDrawCameraType(s32 type) {
    if (type < 0 || type >= 3) {
        aurora::throw_host_exception< std::out_of_range >("Original draw camera category is invalid");
    }
    mDrawCameraType = type;
}

void DrawBufferGroup::setLightType(s32 type) {
    mLightType = type;
    mLightLoadType = type;
}

s32 DrawBufferGroup::findExecuterIndex(const char* pName) const {
    for (u32 i = 0; i < mExecutors.size(); i++) {
        if (MR::isEqualString(mExecutors[i]->mName, pName)) {
            return i;
        }
    }

    return -1;
}
