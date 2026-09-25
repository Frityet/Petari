#include "Game/System/DrawBufferHolder.hpp"
#include "Game/System/DrawBufferGroup.hpp"
#include "Game/Util/MemoryUtil.hpp"
#include <algorithm>
#include <aurora/exception.hpp>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
    // static const char* sDrawTypeCameraName = ;
    // static const char* sPermitMultiCategoryArciveName = ;

    bool isExcludedCheckMultiRegistCategory(s32);
};  // namespace

DrawBufferHolder::DrawBufferHolder() : mTableInitialized(false) {
}

DrawBufferHolder::~DrawBufferHolder() = default;

void DrawBufferHolder::initTable(const DrawBufferInitialTable* pInitialTable, s32 numGroups) {
    if (pInitialTable == nullptr || numGroups <= 0 || mBufferGroups.mArr != nullptr) {
        aurora::throw_host_exception< std::invalid_argument >("Invalid original draw buffer initial table");
    }
    std::unique_ptr< DrawBufferGroup[] > groups(new DrawBufferGroup[numGroups]);
    std::vector< bool > initialized(numGroups, false);
    s32 cameraTypeCounts[3] = {};
    s32 idx = 0;
    for (; idx <= numGroups; idx++) {
        const DrawBufferInitialTable& entry = pInitialTable[idx];
        if (entry.mDrawBufferType == std::numeric_limits< u32 >::max()) {
            break;
        }
        if (idx == numGroups || entry.mDrawBufferType >= static_cast< u32 >(numGroups) || entry.mDrawCameraType >= 3 ||
            entry.mCapacity < 0 || initialized[entry.mDrawBufferType]) {
            aurora::throw_host_exception< std::out_of_range >("Original draw buffer table has an invalid or repeated category");
        }
        initialized[entry.mDrawBufferType] = true;
        DrawBufferGroup& group = groups[entry.mDrawBufferType];
        group.init(entry.mCapacity);
        group.setDrawCameraType(entry.mDrawCameraType);
        group.setLightType(entry.mLightType);
        cameraTypeCounts[entry.mDrawCameraType]++;
    }
    std::unique_ptr< DrawBufferGroup*[] > executeLists[3];
    for (s32 camera = 0; camera < 3; camera++) {
        if (cameraTypeCounts[camera] > 0) {
            executeLists[camera].reset(new DrawBufferGroup*[cameraTypeCounts[camera]]());
        }
    }
    mBufferGroups.mArr = groups.release();
    mBufferGroups.mMaxSize = numGroups;
    for (s32 camera = 0; camera < 3; camera++) {
        mExecuteLists[camera].mArray.mArr = executeLists[camera].release();
        mExecuteLists[camera].mArray.mMaxSize = cameraTypeCounts[camera];
    }
    mTableInitialized = true;
}

DrawBufferGroup* DrawBufferHolder::getDrawBufferGroup(s32 drawBufferType) {
    return const_cast< DrawBufferGroup* >(static_cast< const DrawBufferHolder* >(this)->getDrawBufferGroup(drawBufferType));
}

const DrawBufferGroup* DrawBufferHolder::getDrawBufferGroup(s32 drawBufferType) const {
    if (drawBufferType < 0 || drawBufferType >= mBufferGroups.size() || mBufferGroups[drawBufferType].mExecutors.mArray.mArr == nullptr) {
        aurora::throw_host_exception< std::out_of_range >("Draw buffer category is outside the original initial table");
    }
    return &mBufferGroups[drawBufferType];
}

ExecutorList& DrawBufferHolder::getExecuteList(s32 drawBufferType) {
    s32 camera = getDrawBufferGroup(drawBufferType)->mDrawCameraType;
    if (camera < 0 || camera >= 3) {
        aurora::throw_host_exception< std::out_of_range >("Original draw camera category is invalid");
    }
    return mExecuteLists[camera];
}

bool DrawBufferHolder::isBufferGroupEmpty(s32 drawBufferType) const {
    return getDrawBufferGroup(drawBufferType)->mActiveExecutors.size() == 0;
}

void DrawBufferHolder::allocateActorListBuffer() {
    if (!mTableInitialized) {
        aurora::throw_host_exception< std::logic_error >("Original draw actor lists require an initialized, unallocated table");
    }
    mTableInitialized = false;
    for (s32 idx = 0; idx < mBufferGroups.size(); idx++) {
        mBufferGroups[idx].allocateActorListBuffer();
    }
}

s32 DrawBufferHolder::registerDrawBuffer(LiveActor* pActor, s32 drawBufferType) {
    DrawBufferGroup* group = getDrawBufferGroup(drawBufferType);
    if (!mTableInitialized) {
        aurora::throw_host_exception< std::logic_error >("Original model registration must precede actor-list allocation");
    }
    return group->registerDrawBuffer(pActor);
}

void DrawBufferHolder::active(LiveActor* pActor, s32 drawBufferType, s32 executorIndex) {
    DrawBufferGroup* group = getDrawBufferGroup(drawBufferType);
    bool isEmpty = group->mActiveExecutors.size() == 0;
    ExecutorList& executeList = getExecuteList(drawBufferType);
    if (isEmpty && executeList.size() >= executeList.capacity()) {
        aurora::throw_host_exception< std::length_error >("Original active draw category capacity exceeded");
    }
    group->active(pActor, executorIndex);
    if (isEmpty) {
        executeList.push_back(group);
    }
}

void DrawBufferHolder::deactive(LiveActor* pActor, s32 drawBufferType, s32 executorIndex) {
    DrawBufferGroup* group = getDrawBufferGroup(drawBufferType);
    ExecutorList& executeList = getExecuteList(drawBufferType);
    s32 activeIndex = -1;
    for (s32 idx = 0; idx < executeList.size(); idx++) {
        if (executeList[idx] == group) {
            activeIndex = idx;
            break;
        }
    }
    if (activeIndex < 0) {
        aurora::throw_host_exception< std::logic_error >("Draw category is not active in its original camera list");
    }
    group->deactive(pActor, executorIndex);
    if (group->mActiveExecutors.size() == 0) {
        executeList[activeIndex] = executeList[executeList.size() - 1];
        executeList.mCount--;
    }
}

void DrawBufferHolder::findLightInfo(LiveActor* pActor, s32 drawBufferType, s32 executorIndex) {
    getDrawBufferGroup(drawBufferType)->findLightInfo(pActor, executorIndex);
}

void DrawBufferHolder::entry(s32 drawBufferType) {
    if (drawBufferType < 0 || drawBufferType >= 3) {
        aurora::throw_host_exception< std::out_of_range >("Original draw camera category is invalid");
    }
    for (s32 idx = 0; idx < mExecuteLists[drawBufferType].size(); idx++) {
        mExecuteLists[drawBufferType][idx]->entry();
    }
}

void DrawBufferHolder::drawOpa(s32 drawBufferType) const {
    if (isBufferGroupEmpty(drawBufferType)) {
        return;
    }

    getDrawBufferGroup(drawBufferType)->drawOpa();
}

void DrawBufferHolder::drawXlu(s32 drawBufferType) const {
    if (isBufferGroupEmpty(drawBufferType)) {
        return;
    }

    getDrawBufferGroup(drawBufferType)->drawXlu();
}

void DrawBufferHolder::dummy(s32 drawBufferType) {
    // TODO: This SHOULD NOT be here, this is only here because for_each and Vector<>.end are emitted in this file for DrawBufferGroups,
    // indicating some stripped function uses them. (Check Debug symbols for candidates)
    if (drawBufferType < 0 || drawBufferType >= 3) {
        aurora::throw_host_exception< std::out_of_range >("Original draw camera category is invalid");
    }
    for (s32 idx = 0; idx < mExecuteLists[drawBufferType].size(); idx++) {
        mExecuteLists[drawBufferType][idx]->entry();
    }
}
