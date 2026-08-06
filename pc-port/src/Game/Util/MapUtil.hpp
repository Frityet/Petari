#pragma once

#include <JSystem/JGeometry/TVec.hpp>

class HitSensor;
class LiveActor;
class Triangle;

namespace MR {
    bool isBindedGroundDamageFire(const LiveActor* pActor);
    bool getFirstPolyOnLineToMap(TVec3f* pPosition, Triangle* pTriangle, const TVec3f& rStart, const TVec3f& rOffset);
    bool getFirstPolyNormalOnLineToMap(TVec3f* pNormal, const TVec3f& rStart, const TVec3f& rOffset, TVec3f* pPosition,
                                       const HitSensor* pExceptSensor);
    bool isExistMapCollision(const TVec3f& rStart, const TVec3f& rOffset);
}  // namespace MR
