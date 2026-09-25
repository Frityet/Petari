#include "Game/Gravity.hpp"
#include "Game/Util/MathUtil.hpp"
#include <aurora/exception.hpp>
#include <stdexcept>

PlanetGravityManager::PlanetGravityManager(const char* pName) : NameObj(pName) {
    mNumGravities = 0;
}

PlanetGravityManager::~PlanetGravityManager() {
    while (mNumGravities > 0) {
        unregisterGravity(mGravities[mNumGravities - 1]);
    }
}

void PlanetGravityManager::init(const JMapInfoIter& rIter) {
}

bool PlanetGravityManager::calcTotalGravityVector(TVec3f* pGravity, GravityInfo* pInfo, const TVec3f& rPosition, u32 gravityType, uintptr_t host) const {
    TVec3f totalGravity;
    totalGravity.z = 0.0f;
    totalGravity.y = 0.0f;
    totalGravity.x = 0.0f;

    PlanetGravity** pGravities = (PlanetGravity**)&mGravities;
    bool hasCalculated = false;

    if (pInfo) {
        pInfo->init();
    }

    f32 largestScalar = -1.0f;
    s32 largestPriority = -1;

    while (pGravities != &mGravities[mNumGravities]) {
        // Is gravity valid/active?
        bool validGravity = false;

        if ((*pGravities)->mActivated && (*pGravities)->mValidFollower && (*pGravities)->mAppeared) {
            validGravity = true;
        }

        // Gravity is valid, matches specified gravity type(s) and does not have same host
        if (validGravity && ((gravityType & (*pGravities)->mGravityType) != 0) && (host != reinterpret_cast< uintptr_t >((*pGravities)->mHost))) {
            // Is priority lower -> end of list reached as it's sorted in descending order
            s32 priority = (*pGravities)->mPriority;

            if (priority < largestPriority) {
                break;
            }

            // Calculate gravity
            TVec3f gravityVec;

            if ((*pGravities)->calcGravity(&gravityVec, rPosition)) {
                bool storeInfo = false;
                f32 scalar = gravityVec.length();

                // If same priority, add gravity vector to total result
                if (priority == largestPriority) {
                    totalGravity += gravityVec;
                    hasCalculated = true;

                    if (largestScalar < scalar) {
                        storeInfo = true;
                    }
                }
                // If larger priority, total gravity gets overriden
                else if (priority > largestPriority) {
                    largestPriority = priority;
                    hasCalculated = true;
                    storeInfo = true;
                    totalGravity = gravityVec;
                }

                // Try to store gravity info if necessary
                if (pInfo && storeInfo) {
                    largestScalar = scalar;
                    pInfo->mGravityVector = gravityVec;
                    pInfo->mLargestPriority = largestPriority;
                    pInfo->mGravityInstance = *pGravities;
                }
            }
        }

        pGravities++;
    }

    // Store normalized vector as result if necessary
    if (pGravity) {
        MR::normalizeOrZero(&totalGravity);
        *pGravity = totalGravity;
    }

    return hasCalculated;
}

void PlanetGravityManager::registerGravity(PlanetGravity* pGravity) {
    if (pGravity->mNativeManager != nullptr) {
        aurora::throw_host_exception< std::logic_error >("PlanetGravity is already registered");
    }
    if (mNumGravities == 128) {
        aurora::throw_host_exception< std::length_error >("PlanetGravityManager capacity exceeded");
    }
    pGravity->mNativeManager = this;
    pGravity->mIsRegistered = true;
    int index = mNumGravities++;
    mGravities[index] = pGravity;

    sortGravities();
}

void PlanetGravityManager::unregisterGravity(PlanetGravity* pGravity) noexcept {
    for (s32 i = 0; i < mNumGravities; ++i) {
        if (mGravities[i] != pGravity) {
            continue;
        }
        for (s32 j = i + 1; j < mNumGravities; ++j) {
            mGravities[j - 1] = mGravities[j];
        }
        mGravities[--mNumGravities] = nullptr;
        pGravity->mNativeManager = nullptr;
        pGravity->mIsRegistered = false;
        return;
    }
}

inline void PlanetGravityManager::sortGravities() {
    for (s32 i = mNumGravities - 1; i >= 1; i--) {
        if (mGravities[i]->mPriority > mGravities[i - 1]->mPriority) {
            bubbleDown(i);
        } else {
            break;
        }
    }
}

inline void PlanetGravityManager::bubbleDown(int i) {
    PlanetGravity* temp = mGravities[i - 1];
    mGravities[i - 1] = mGravities[i];
    mGravities[i] = temp;
}
