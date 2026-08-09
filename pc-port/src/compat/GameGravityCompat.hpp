#pragma once

#include <JSystem/JGeometry/TVec.hpp>
#include <revolution/types.h>

#include <cstddef>

class GravityInfo;
class LiveActor;
class NameObj;

namespace MR {
    bool calcGravityVector(const LiveActor* actor, TVec3f* destination, GravityInfo* info, u32 host);
    bool calcGravityVector(const NameObj* object, const TVec3f& position, TVec3f* destination,
                           GravityInfo* info, u32 host);
    bool calcGravityVectorOrZero(const LiveActor* actor, TVec3f* destination, GravityInfo* info, u32 host);
    bool calcGravityVectorOrZero(const NameObj* object, const TVec3f& position, TVec3f* destination,
                                 GravityInfo* info, u32 host);

    // Metrowerks accepts the decompiled null host arguments as integer zero.
    // Keep that source spelling valid without changing original Game calls.
    inline bool calcGravityVector(const LiveActor* actor, TVec3f* destination, GravityInfo* info,
                                  std::nullptr_t) {
        return calcGravityVector(actor, destination, info, 0U);
    }

    inline bool calcGravityVector(const LiveActor* actor, TVec3f* destination, GravityInfo* info,
                                  int host) {
        return calcGravityVector(actor, destination, info, static_cast<u32>(host));
    }

    inline bool calcGravityVector(const NameObj* object, const TVec3f& position, TVec3f* destination,
                                  GravityInfo* info, std::nullptr_t) {
        return calcGravityVector(object, position, destination, info, 0U);
    }

    inline bool calcGravityVector(const NameObj* object, const TVec3f& position, TVec3f* destination,
                                  GravityInfo* info, int host) {
        return calcGravityVector(object, position, destination, info, static_cast<u32>(host));
    }

    inline bool calcGravityVectorOrZero(const LiveActor* actor, TVec3f* destination, GravityInfo* info,
                                        std::nullptr_t) {
        return calcGravityVectorOrZero(actor, destination, info, 0U);
    }

    inline bool calcGravityVectorOrZero(const LiveActor* actor, TVec3f* destination, GravityInfo* info,
                                        int host) {
        return calcGravityVectorOrZero(actor, destination, info, static_cast<u32>(host));
    }

    inline bool calcGravityVectorOrZero(const NameObj* object, const TVec3f& position, TVec3f* destination,
                                        GravityInfo* info, std::nullptr_t) {
        return calcGravityVectorOrZero(object, position, destination, info, 0U);
    }

    inline bool calcGravityVectorOrZero(const NameObj* object, const TVec3f& position, TVec3f* destination,
                                        GravityInfo* info, int host) {
        return calcGravityVectorOrZero(object, position, destination, info, static_cast<u32>(host));
    }
}  // namespace MR
