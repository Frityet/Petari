#include "Game/Map/HitInfo.hpp"
#include "Game/MapObj/StarPiece.hpp"
#include "Game/Util/GravityUtil.hpp"

class MultiEmitter;
class TriangleFilterBase;

namespace MR {
    void initDLMakerMatColor0(LiveActor*, const char*, const J3DGXColor*);
    void newDifferedDLBuffer(LiveActor*);
    void setBtkFrameAndStop(const LiveActor*, f32);
    f32 getBtkFrameMax(const LiveActor*);
    void setBinderRadius(LiveActor*, f32);
    void forceDeleteEffect(LiveActor*, const char*);
    s32* getStarPointerLastPointedPort(const LiveActor*);
    void emitEffectHit(LiveActor*, const TVec3f&, const char*);
    f32 calcNerveRate(const LiveActor*, s32);
    void setBindTriangleFilter(LiveActor*, TriangleFilterBase*);
    bool isBinded(const LiveActor*);
    const TVec3f* getBindedNormal(const LiveActor*);
    const TVec3f* getBindedHitPos(const LiveActor*);
    bool isSameDirection(const TVec3f&, const TVec3f&, f32);
    void calcParabolicFunctionParam(f32*, f32*, f32, f32);
    void addStarPiece(int);
    f32 calcPointRadius2D(const TVec3f&, f32);
    MultiEmitter* emitEffectWithScale(LiveActor*, const char*, f32, s32);
    void setEffectPrmColor(LiveActor*, const char*, u8, u8, u8);
    void setEffectColor(LiveActor*, const char*, u8, u8, u8, u8, u8, u8);
    bool isStarPointerPointing1Por2P(const LiveActor*, const char*, bool, bool);
    bool calcReflectionVector(TVec3f*, const TVec3f&, f32, f32);
    bool tryRumblePadMiddle(const void*, s32);
}  // namespace MR

#define getBaseMtx() reinterpret_cast<const f32 (*)[4]>(getBaseMatrix().m.data())
#define allocateDelegator(object, method) allocateDelegator(object, &StarPiece::isIgnoreTriOnThrow)
#define calcGravityVectorOrZero(actor, destination, info, host)                                                                  \
    calcGravityVectorOrZero(actor, destination, info, static_cast<u32>(host))
#include "Game/MapObj/StarPiece.cpp"
#undef calcGravityVectorOrZero
#undef allocateDelegator
#undef getBaseMtx
