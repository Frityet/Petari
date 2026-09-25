#pragma once

#include <JSystem/JGeometry/TVec.hpp>
#include <stdint.h>
#if !defined(__MWERKS__)
#include <cstddef>
#include <type_traits>
#endif

class GravityInfo;
class JMapInfoIter;
class LiveActor;
class NameObj;
class PlanetGravity;

namespace MR {
    void registerGravity(PlanetGravity* pGravity);
    bool calcGravityVector(const LiveActor* pActor, TVec3f* pDest, GravityInfo* pInfo, uintptr_t host);
    bool calcGravityVector(const NameObj* pObj, const TVec3f& rPosition, TVec3f* pDest, GravityInfo* pInfo, uintptr_t host);
    bool calcDropShadowVector(const LiveActor* pActor, TVec3f* pDest, GravityInfo* pInfo, uintptr_t host);
    bool calcDropShadowVector(const NameObj* pObj, const TVec3f& rPosition, TVec3f* pDest, GravityInfo* pInfo, uintptr_t host);
    bool calcGravityAndDropShadowVector(const LiveActor* pActor, TVec3f* pDest, GravityInfo* pInfo, uintptr_t host);
    bool calcGravityAndMagnetVector(const NameObj* pObj, const TVec3f& rPosition, TVec3f* pDest, GravityInfo* pInfo, uintptr_t host);
    bool calcGravityVectorOrZero(const LiveActor* pActor, TVec3f* pDest, GravityInfo* pInfo, uintptr_t host);
    bool calcGravityVectorOrZero(const NameObj* pObj, const TVec3f& rPosition, TVec3f* pDest, GravityInfo* pInfo, uintptr_t host);
    bool calcDropShadowVectorOrZero(const NameObj* pObj, const TVec3f& rPosition, TVec3f* pDest, GravityInfo* pInfo, uintptr_t host);
    bool calcGravityAndDropShadowVectorOrZero(const LiveActor* pActor, TVec3f* pDest, GravityInfo* pInfo, uintptr_t host);
    bool calcAttractMarioLauncherOrZero(const LiveActor* pActor, TVec3f* pDest, GravityInfo* pInfo, uintptr_t host);
    bool isZeroGravity(const LiveActor* pActor);
    bool isLightGravity(const GravityInfo& rInfo);
    void settingGravityParamFromJMap(PlanetGravity* pGravity, const JMapInfoIter& rIter);
    void getJMapInfoGravityType(const JMapInfoIter& rIter, PlanetGravity* pGravity);
    void getJMapInfoGravityPower(const JMapInfoIter& rIter, PlanetGravity* pGravity);

#if !defined(__MWERKS__)
    // Original calls also spell the zero host identity as nullptr. Constrain
    // these overloads so integer zero still selects the native-width API.
    template < typename Host > requires std::is_same_v< Host, std::nullptr_t >
    inline bool calcGravityVector(const LiveActor* pActor, TVec3f* pDest, GravityInfo* pInfo, Host) {
        return calcGravityVector(pActor, pDest, pInfo, uintptr_t{0});
    }

    template < typename Host > requires std::is_same_v< Host, std::nullptr_t >
    inline bool calcGravityVector(const NameObj* pObj, const TVec3f& rPosition, TVec3f* pDest, GravityInfo* pInfo, Host) {
        return calcGravityVector(pObj, rPosition, pDest, pInfo, uintptr_t{0});
    }

    template < typename Host > requires std::is_same_v< Host, std::nullptr_t >
    inline bool calcGravityVectorOrZero(const LiveActor* pActor, TVec3f* pDest, GravityInfo* pInfo, Host) {
        return calcGravityVectorOrZero(pActor, pDest, pInfo, uintptr_t{0});
    }

    template < typename Host > requires std::is_same_v< Host, std::nullptr_t >
    inline bool calcGravityVectorOrZero(const NameObj* pObj, const TVec3f& rPosition, TVec3f* pDest, GravityInfo* pInfo, Host) {
        return calcGravityVectorOrZero(pObj, rPosition, pDest, pInfo, uintptr_t{0});
    }
#endif
};  // namespace MR
