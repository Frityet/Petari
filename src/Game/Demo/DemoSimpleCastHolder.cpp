#include "Game/Demo/DemoSimpleCastHolder.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Screen/LayoutActor.hpp"
#include "Game/Util/ObjUtil.hpp"

DemoSimpleCastHolder::DemoSimpleCastHolder(s32 liveActorCount, s32 layoutActorCount, s32 nameObjCount) {
    mLiveActors.init(liveActorCount);
    mLayoutActors.init(layoutActorCount);
    mNameObjs.init(nameObjCount);
}

void DemoSimpleCastHolder::registerActor(LiveActor* pActor) {
    mLiveActors.push_back(pActor);
}

void DemoSimpleCastHolder::registerActor(LayoutActor* pActor) {
    mLayoutActors.push_back(pActor);
}

void DemoSimpleCastHolder::registerNameObj(NameObj* pNameObj) {
    mNameObjs.push_back(pNameObj);
}

void DemoSimpleCastHolder::movementOnAllCasts() {
    for (LiveActor** pActor = mLiveActors.begin(); pActor != mLiveActors.end(); pActor++) {
        MR::requestMovementOn(*pActor);
    }

    for (LayoutActor** pActor = mLayoutActors.begin(); pActor != mLayoutActors.end(); pActor++) {
        MR::requestMovementOn(*pActor);
    }

    for (NameObj** pNameObj = mNameObjs.begin(); pNameObj != mNameObjs.end(); pNameObj++) {
        MR::requestMovementOn(*pNameObj);
    }
}

void DemoSimpleCastHolder::releaseNativeReference(const NameObj* pObject) noexcept {
    const auto release = [pObject](auto& values) {
        s32 retained = 0;
        for (s32 i = 0; i < values.mCount; i++) {
            if (values[i] != pObject) {
                values[retained++] = values[i];
            }
        }
        for (s32 i = retained; i < values.mCount; i++) {
            values[i] = nullptr;
        }
        values.mCount = retained;
    };
    release(mLiveActors);
    release(mLayoutActors);
    release(mNameObjs);
}

std::size_t DemoSimpleCastHolder::nativeRegistrationCount(const NameObj* pObject) const noexcept {
    std::size_t count = 0;
    const auto add = [&](const auto& values) {
        for (const auto* value : values) {
            if (pObject == nullptr || value == pObject) {
                count++;
            }
        }
    };
    add(mLiveActors);
    add(mLayoutActors);
    add(mNameObjs);
    return count;
}
