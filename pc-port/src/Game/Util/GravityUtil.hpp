#pragma once

#include <JSystem/JGeometry/TVec.hpp>
#include <revolution/types.h>

#include <cstddef>

class GravityInfo;
class LiveActor;
class NameObj;

namespace MR {
    bool calcGravityVector(const LiveActor* pActor, TVec3f* pDest, GravityInfo* pInfo, u32 host);
    bool calcGravityVector(const NameObj* pObj, const TVec3f& rPosition, TVec3f* pDest, GravityInfo* pInfo, u32 host);
    bool calcGravityVectorOrZero(const LiveActor* pActor, TVec3f* pDest, GravityInfo* pInfo, u32 host);
    bool calcGravityVectorOrZero(const NameObj* pObj, const TVec3f& rPosition, TVec3f* pDest, GravityInfo* pInfo, u32 host);

    // The original sources use `nullptr` as the zero host value. Keep those
    // call sites source-close under a modern C++ compiler while retaining the
    // original u32 entry points above.
    inline bool calcGravityVector(const LiveActor* pActor, TVec3f* pDest, GravityInfo* pInfo, std::nullptr_t) {
        return calcGravityVector(pActor, pDest, pInfo, 0U);
    }

    inline bool calcGravityVector(const NameObj* pObj, const TVec3f& rPosition, TVec3f* pDest, GravityInfo* pInfo, std::nullptr_t) {
        return calcGravityVector(pObj, rPosition, pDest, pInfo, 0U);
    }

    inline bool calcGravityVectorOrZero(const LiveActor* pActor, TVec3f* pDest, GravityInfo* pInfo, std::nullptr_t) {
        return calcGravityVectorOrZero(pActor, pDest, pInfo, 0U);
    }

    inline bool calcGravityVectorOrZero(const NameObj* pObj, const TVec3f& rPosition, TVec3f* pDest, GravityInfo* pInfo, std::nullptr_t) {
        return calcGravityVectorOrZero(pObj, rPosition, pDest, pInfo, 0U);
    }

    void calcGravityOrZero(LiveActor* pActor);
    void calcGravityOrZero(LiveActor* pActor, const TVec3f& rPosition);
}  // namespace MR
