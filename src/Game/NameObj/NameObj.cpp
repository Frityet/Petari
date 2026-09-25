#include "Game/NameObj/NameObj.hpp"
#include "Game/NameObj/NameObjRegister.hpp"
#include "Game/Scene/SceneNameObjMovementController.hpp"
#include "Game/Util/SingletonHolder.hpp"
#include "runtime/RuntimeContext.hpp"
#include <aurora/allocation.hpp>
#include <aurora/exception.hpp>
#include <limits>
#include <stdexcept>

#define FLAG_MOVEMENT_OFF 1u
#define FLAG_SUSPEND 2u
#define FLAG_RESUME 4u

NameObj* NameObj::sNativeFirst = nullptr;
NameObj* NameObj::sNativeLast = nullptr;
u64 NameObj::sNativeNextGeneration = 1;
NameObj::NativeIteration* NameObj::sNativeIteration = nullptr;

// Retirement callbacks may delete the next object or start another retirement.
// Unlinking repairs every active stack cursor before any object storage is freed.
struct NameObj::NativeIteration {
    NativeIteration* mPrevious;
    NameObj* mNext;
    u64 mEndGeneration;

    NativeIteration()
        : mPrevious(sNativeIteration), mNext(sNativeFirst), mEndGeneration(sNativeNextGeneration) {
        sNativeIteration = this;
    }

    ~NativeIteration() {
        sNativeIteration = mPrevious;
    }

    NameObj* take() noexcept {
        NameObj* object = mNext;
        if (!object || object->mNativeGeneration >= mEndGeneration) {
            return nullptr;
        }
        mNext = object->mNativeNext;
        return object;
    }
};

NameObj::NameObj(const char* pName) : mName(pName), mFlag(), mExecutorIdx(-1) {
    if (sNativeNextGeneration == std::numeric_limits< u64 >::max()) {
        aurora::throw_host_exception< std::overflow_error >("NameObj native generation space is exhausted");
    }
    mNativeGeneration = sNativeNextGeneration++;
    mNativePrevious = sNativeLast;
    if (sNativeLast) {
        sNativeLast->mNativeNext = this;
    } else {
        sNativeFirst = this;
    }
    sNativeLast = this;
    try {
        if (auto* registry = SingletonHolder<NameObjRegister>::get(); registry && registry->mHolder) {
            registry->add(this);
        }
    } catch (...) {
        retireNativeLifetime();
        throw;
    }
}

NameObj::~NameObj() {
    if (auto* scheduler = smgpc::runtime::try_active_scene_scheduler()) {
        scheduler->disconnect_name_obj(*this);
    } else if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance()) {
        runtime->scheduler().disconnect_name_obj(*this);
    }
    detachNativeHolder();
    retireNativeLifetime();
}

void NameObj::detachNativeHolder() noexcept {
    if (mNativeHolder) {
        mNativeHolder->removeNativeObject(this);
    }
}

u64 NameObj::nativeGeneration(const NameObj* object) noexcept {
    for (auto* current = sNativeFirst; current; current = current->mNativeNext) {
        if (current == object) {
            return current->mNativeGeneration;
        }
    }
    return 0;
}

bool NameObj::isNativeOwnershipClaimed(const NameObj* object) noexcept {
    for (auto* current = sNativeFirst; current; current = current->mNativeNext) {
        if (current == object) {
            return current->mNativeOwner != nullptr;
        }
    }
    return false;
}

void NameObj::claimNativeOwnership(const void* owner) {
    if (!owner || !mNativeGeneration) {
        aurora::throw_host_exception< std::invalid_argument >("NameObj ownership requires a live object and owner");
    }
    if (mNativeOwner) {
        aurora::throw_host_exception< std::logic_error >("NameObj ownership is already claimed");
    }
    mNativeOwner = owner;
}

void NameObj::retireNativeLifetime() noexcept {
    if (!mNativeGeneration) {
        return;
    }
    for (auto* iteration = sNativeIteration; iteration; iteration = iteration->mPrevious) {
        if (iteration->mNext == this) {
            iteration->mNext = mNativeNext;
        }
    }
    if (mNativePrevious) {
        mNativePrevious->mNativeNext = mNativeNext;
    } else {
        sNativeFirst = mNativeNext;
    }
    if (mNativeNext) {
        mNativeNext->mNativePrevious = mNativePrevious;
    } else {
        sNativeLast = mNativePrevious;
    }
    mNativePrevious = nullptr;
    mNativeNext = nullptr;
    mNativeGeneration = 0;
    mNativeOwner = nullptr;

    NativeIteration iteration;
    while (NameObj* object = iteration.take()) {
        object->releaseNativeReference(this);
    }
}

NameObj::NativeRegistrationMarker NameObj::markNativeRegistrations() noexcept {
    return {sNativeNextGeneration};
}

std::vector< NameObj* > NameObj::snapshotNativeObjects() {
    return snapshotNativeObjectsSince({1});
}

std::vector< NameObj* > NameObj::snapshotNativeObjectsSince(NativeRegistrationMarker marker) {
    if (!marker.mNextGeneration || marker.mNextGeneration > sNativeNextGeneration) {
        aurora::throw_host_exception< std::invalid_argument >("NameObj native registration marker is invalid");
    }
    const aurora::allocation::HostAllocationScope host;
    std::vector< NameObj* > objects;
    for (auto* object = sNativeFirst; object; object = object->mNativeNext) {
        if (object->mNativeGeneration >= marker.mNextGeneration) {
            objects.push_back(object);
        }
    }
    return objects;
}

NameObj* NameObj::newestNativeObjectSince(NativeRegistrationMarker marker, NativeRegistrationFilter filter,
                                        const void* context) noexcept {
    if (!marker.mNextGeneration || marker.mNextGeneration > sNativeNextGeneration) {
        return nullptr;
    }
    for (auto* object = sNativeLast; object && object->mNativeGeneration >= marker.mNextGeneration;
         object = object->mNativePrevious) {
        if (!filter || filter(object, context)) {
            return object;
        }
    }
    return nullptr;
}

bool NameObj::wasNativeRegisteredSince(const NameObj* object, NativeRegistrationMarker marker) noexcept {
    return marker.mNextGeneration && marker.mNextGeneration <= sNativeNextGeneration &&
           nativeGeneration(object) >= marker.mNextGeneration;
}

void NameObj::releaseNativeReference(const NameObj*) noexcept {
}

void NameObj::notifyNativeSensorRetirement(const HitSensor* sensor) noexcept {
    if (!sensor) {
        return;
    }
    NativeIteration iteration;
    while (NameObj* object = iteration.take()) {
        object->releaseNativeSensorReference(sensor);
    }
}

void NameObj::releaseNativeSensorReference(const HitSensor*) noexcept {
}

void NameObj::init(const JMapInfoIter& rIter) {
}

void NameObj::initAfterPlacement() {
}

void NameObj::movement() {
}

void NameObj::draw() const {
}

void NameObj::calcAnim() {
}

void NameObj::calcViewAndEntry() {
}

void NameObj::initWithoutIter() {
    init(JMapInfoIter());
}

void NameObj::setName(const char* pName) {
    mName = pName;
}

void NameObj::executeMovement() {
    if ((mFlag & FLAG_MOVEMENT_OFF) == FLAG_MOVEMENT_OFF) {
        return;
    }

    movement();
}

void NameObj::requestSuspend() {
    if ((getFlag() & FLAG_RESUME) == FLAG_RESUME) {
        mFlag &= ~FLAG_RESUME;
    }

    mFlag |= FLAG_SUSPEND;
}

void NameObj::requestResume() {
    if ((getFlag() & FLAG_SUSPEND) == FLAG_SUSPEND) {
        mFlag &= ~FLAG_SUSPEND;
    }

    mFlag |= FLAG_RESUME;
}

void NameObj::syncWithFlags() {
    if ((getFlag() & FLAG_SUSPEND) == FLAG_SUSPEND) {
        mFlag &= ~FLAG_SUSPEND;
        mFlag |= FLAG_MOVEMENT_OFF;
    }

    if ((getFlag() & FLAG_RESUME) == FLAG_RESUME) {
        mFlag &= ~FLAG_RESUME;
        mFlag &= ~FLAG_MOVEMENT_OFF;
    }
}

void NameObjFunction::requestMovementOn(NameObj* pObj) {
    pObj->requestResume();
    MR::notifyRequestNameObjMovementOnOff();
}

void NameObjFunction::requestMovementOff(NameObj* pObj) {
    pObj->requestSuspend();
    MR::notifyRequestNameObjMovementOnOff();
}
