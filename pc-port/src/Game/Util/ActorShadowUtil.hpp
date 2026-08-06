#pragma once

#include "JSystem/JGeometry/TVec.hpp"

class LiveActor;

namespace MR {
    void initShadowSurfaceCircle(LiveActor* pActor, f32 radius);
    void initShadowVolumeSphere(LiveActor* pActor, f32 radius);
    void initShadowVolumeCylinder(LiveActor* pActor, f32 radius);
    void setShadowDropPositionPtr(LiveActor* pActor, const char* pName, const TVec3f* pPosition);
    void setShadowDropLength(LiveActor* pActor, const char* pName, f32 length);
    void onCalcShadow(LiveActor* pActor, const char* pName);
    void offCalcShadow(LiveActor* pActor, const char* pName);
    void onCalcShadowOneTime(LiveActor* pActor, const char* pName);
    void onCalcShadowDropPrivateGravity(LiveActor* pActor, const char* pName);
    void onCalcShadowDropPrivateGravityOneTime(LiveActor* pActor, const char* pName);
    void invalidateShadow(LiveActor* pActor, const char* pName);
    void validateShadow(LiveActor* pActor, const char* pName);
    void setClippingRangeIncludeShadow(LiveActor* pActor, TVec3f* pCenter, f32 radius);
}  // namespace MR
