#include "Game/Util/GravityUtil.hpp"
#include "Game/Gravity/GravityInfo.hpp"
#include "Game/Gravity/PlanetGravity.hpp"
#include "Game/Gravity/PlanetGravityManager.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/JMapInfo.hpp"
#include <aurora/exception.hpp>
#include <stdexcept>
#include <cstring>

namespace {
    char cRange[] = "Range";
    char cDistant[] = "Distant";
    char cPriority[] = "Priority";
    char cGravityId[] = "Gravity_id";
    char cGravityType[] = "Gravity_type";
    char cNormal[] = "Normal";
    char cShadow[] = "Shadow";
    char cMagnet[] = "Magnet";
    char cPower[] = "Power";
    char cLight[] = "Light";
    char cHeavy[] = "Heavy";
    char cInverse[] = "Inverse";

    PlanetGravityManager* getGravityManager() {
        auto* holder = MR::getSceneObjHolder();
        if (holder == nullptr) {
            aurora::throw_host_exception< std::logic_error >("PlanetGravity is unavailable without a scene-owned SceneObjHolder");
        }
        auto* manager = static_cast< PlanetGravityManager* >(holder->getObj(SceneObj_PlanetGravityManager));
        if (manager == nullptr) {
            aurora::throw_host_exception< std::logic_error >("PlanetGravity is unavailable without the scene-owned PlanetGravityManager");
        }
        return manager;
    }

    const TVec3f& getGravityPosition(const LiveActor* pActor) {
        if (pActor == nullptr) {
            aurora::throw_host_exception< std::invalid_argument >("gravity queries require a real LiveActor");
        }
        return pActor->mPosition;
    }

    void getJMapInfoArgPlus(const JMapInfoIter& rIter, const char* pFieldName, f32* pDest) {
        f32 result;

        if (rIter.getValue(pFieldName, &result) && result >= 0.0f) {
            *pDest = result;
        }
    }

    void getJMapInfoArgPlus(const JMapInfoIter& rIter, const char* pFieldName, s32* pDest) {
        s32 result;

        if (rIter.getValue(pFieldName, &result) && result >= 0.0f) {
            *pDest = result;
        }
    }

    bool calcGravityVectorOrZero(const NameObj* pActor, const TVec3f& rPosition, u32 typeFlags, TVec3f* pDest, GravityInfo* pInfo,
                                 uintptr_t host) NO_INLINE {
        if (host == 0) {
            host = reinterpret_cast< uintptr_t >(pActor);
        }

        return getGravityManager()->calcTotalGravityVector(pDest, pInfo, rPosition, typeFlags, host);
    }
};  // namespace

namespace MR {
    void registerGravity(PlanetGravity* pGravity) {
        if (pGravity == nullptr) {
            aurora::throw_host_exception< std::invalid_argument >("cannot register a null PlanetGravity");
        }
        auto* manager = ::getGravityManager();
        if (pGravity->mIsRegistered) {
            aurora::throw_host_exception< std::logic_error >("PlanetGravity is already registered");
        }
        manager->registerGravity(pGravity);
    }

    bool calcGravityVector(const LiveActor* pActor, TVec3f* pDest, GravityInfo* pInfo, uintptr_t host) {
        u32 typeFlags = GRAVITY_TYPE_NORMAL;
        return ::calcGravityVectorOrZero(pActor, ::getGravityPosition(pActor), typeFlags, pDest, pInfo, host);
    }

    bool calcGravityVector(const NameObj* pActor, const TVec3f& rPosition, TVec3f* pDest, GravityInfo* pInfo, uintptr_t host) {
        u32 typeFlags = GRAVITY_TYPE_NORMAL;
        return ::calcGravityVectorOrZero(pActor, rPosition, typeFlags, pDest, pInfo, host);
    }

    bool calcDropShadowVector(const LiveActor* pActor, TVec3f* pDest, GravityInfo* pInfo, uintptr_t host) {
        u32 typeFlags = GRAVITY_TYPE_SHADOW;
        return ::calcGravityVectorOrZero(pActor, ::getGravityPosition(pActor), typeFlags, pDest, pInfo, host);
    }

    bool calcDropShadowVector(const NameObj* pActor, const TVec3f& rPosition, TVec3f* pDest, GravityInfo* pInfo, uintptr_t host) {
        u32 typeFlags = GRAVITY_TYPE_SHADOW;
        return ::calcGravityVectorOrZero(pActor, rPosition, typeFlags, pDest, pInfo, host);
    }

    bool calcGravityAndDropShadowVector(const LiveActor* pActor, TVec3f* pDest, GravityInfo* pInfo, uintptr_t host) {
        u32 typeFlags = GRAVITY_TYPE_NORMAL | GRAVITY_TYPE_SHADOW;
        return ::calcGravityVectorOrZero(pActor, ::getGravityPosition(pActor), typeFlags, pDest, pInfo, host);
    }

    bool calcGravityAndMagnetVector(const NameObj* pActor, const TVec3f& rPosition, TVec3f* pDest, GravityInfo* pInfo, uintptr_t host) {
        u32 typeFlags = GRAVITY_TYPE_NORMAL | GRAVITY_TYPE_MAGNET;
        return ::calcGravityVectorOrZero(pActor, rPosition, typeFlags, pDest, pInfo, host);
    }

    bool calcGravityVectorOrZero(const LiveActor* pActor, TVec3f* pDest, GravityInfo* pInfo, uintptr_t host) {
        u32 typeFlags = GRAVITY_TYPE_NORMAL;
        return ::calcGravityVectorOrZero(pActor, ::getGravityPosition(pActor), typeFlags, pDest, pInfo, host);
    }

    bool calcGravityVectorOrZero(const NameObj* pActor, const TVec3f& rPosition, TVec3f* pDest, GravityInfo* pInfo, uintptr_t host) {
        u32 typeFlags = GRAVITY_TYPE_NORMAL;
        return ::calcGravityVectorOrZero(pActor, rPosition, typeFlags, pDest, pInfo, host);
    }

    bool calcDropShadowVectorOrZero(const NameObj* pActor, const TVec3f& rPosition, TVec3f* pDest, GravityInfo* pInfo, uintptr_t host) {
        u32 typeFlags = GRAVITY_TYPE_SHADOW;
        return ::calcGravityVectorOrZero(pActor, rPosition, typeFlags, pDest, pInfo, host);
    }

    bool calcGravityAndDropShadowVectorOrZero(const LiveActor* pActor, TVec3f* pDest, GravityInfo* pInfo, uintptr_t host) {
        u32 typeFlags = GRAVITY_TYPE_NORMAL | GRAVITY_TYPE_SHADOW;
        return ::calcGravityVectorOrZero(pActor, ::getGravityPosition(pActor), typeFlags, pDest, pInfo, host);
    }

    bool calcAttractMarioLauncherOrZero(const LiveActor* pActor, TVec3f* pDest, GravityInfo* pInfo, uintptr_t host) {
        u32 typeFlags = GRAVITY_TYPE_MARIO_LAUNCHER;
        return ::calcGravityVectorOrZero(pActor, ::getGravityPosition(pActor), typeFlags, pDest, pInfo, host);
    }

    bool isZeroGravity(const LiveActor* pActor) {
        TVec3f dummyGravity;
        return !::calcGravityVectorOrZero(pActor, ::getGravityPosition(pActor), GRAVITY_TYPE_NORMAL, &dummyGravity, nullptr, 0);
    }

    bool isLightGravity(const GravityInfo& rInfo) {
        PlanetGravity* pGravity = rInfo.mGravityInstance;

        if (pGravity == nullptr) {
            return false;
        }

        return pGravity->mGravityPower == GRAVITY_POWER_LIGHT;
    }

    void settingGravityParamFromJMap(PlanetGravity* pGravity, const JMapInfoIter& rIter) {
        if (pGravity == nullptr) {
            aurora::throw_host_exception< std::invalid_argument >("gravity JMap parameters require a real PlanetGravity");
        }

        f32 range = pGravity->mRange;
        ::getJMapInfoArgPlus(rIter, ::cRange, &range);
        pGravity->mRange = range;

        f32 distant = pGravity->getDistant();
        ::getJMapInfoArgPlus(rIter, ::cDistant, &distant);
        pGravity->mDistant = distant;

        s32 priority = pGravity->mPriority;
        ::getJMapInfoArgPlus(rIter, ::cPriority, &priority);
        pGravity->setPriority(priority);

        s32 id = pGravity->mGravityId;
        ::getJMapInfoArgPlus(rIter, ::cGravityId, &id);
        pGravity->mGravityId = id;

        getJMapInfoGravityType(rIter, pGravity);
        getJMapInfoGravityPower(rIter, pGravity);

        s32 inverse = pGravity->mIsInverse != false;
        ::getJMapInfoArgPlus(rIter, ::cInverse, &inverse);
        pGravity->mIsInverse = inverse;
    }

    void getJMapInfoGravityType(const JMapInfoIter& rIter, PlanetGravity* pGravity) {
        if (pGravity == nullptr) {
            aurora::throw_host_exception< std::invalid_argument >("gravity type parsing requires a real PlanetGravity");
        }

        const char* pType = nullptr;

        if (rIter.getValue(::cGravityType, &pType) && pType != nullptr) {
            if (strcmp(pType, ::cNormal) == 0) {
                pGravity->mGravityType = GRAVITY_TYPE_NORMAL;
            } else if (strcmp(pType, ::cShadow) == 0) {
                pGravity->mGravityType = GRAVITY_TYPE_SHADOW;
            } else if (strcmp(pType, ::cMagnet) == 0) {
                pGravity->mGravityType = GRAVITY_TYPE_MAGNET;
            }
        }
    }

    void getJMapInfoGravityPower(const JMapInfoIter& rIter, PlanetGravity* pGravity) {
        if (pGravity == nullptr) {
            aurora::throw_host_exception< std::invalid_argument >("gravity power parsing requires a real PlanetGravity");
        }

        const char* pPower = nullptr;

        if (rIter.getValue(::cPower, &pPower) && pPower != nullptr) {
            if (strcmp(pPower, ::cLight) == 0) {
                pGravity->mGravityPower = GRAVITY_POWER_LIGHT;
            } else if (strcmp(pPower, ::cNormal) == 0) {
                pGravity->mGravityPower = GRAVITY_POWER_NORMAL;
            } else if (strcmp(pPower, ::cHeavy) == 0) {
                pGravity->mGravityPower = GRAVITY_POWER_HEAVY;
            }
        }
    }
};  // namespace MR
