#pragma once

#include "JSystem/JGeometry.hpp"
#include <memory>
#include <revolution.h>
#include <string_view>

class CollisionZone;
class HitInfo;
class HitSensor;
class KC_PrismData;
class KCollisionServer;
class Triangle;
class TriangleFilterBase;
class ResourceHolder;
namespace smgpc::resource {
    class GeneratedKCollisionResource;
}

class CollisionParts {
public:
    CollisionParts();
    ~CollisionParts();
    CollisionParts(const CollisionParts&) = delete;
    CollisionParts& operator=(const CollisionParts&) = delete;

    void initFromResource(ResourceHolder*, const char*, HitSensor*, const TPos3f&, int, s32);
    void initFromGeneratedResource(std::shared_ptr< smgpc::resource::GeneratedKCollisionResource >, HitSensor*, const TPos3f&, s32);
    void requireNativeGeometryPublished() const;
    std::weak_ptr< const void > nativeLifetime() const noexcept {
        return mNativeLifetime;
    }
    std::string_view nativeResourceName() const noexcept;
    std::string_view nativeResourceSource() const noexcept;
    std::size_t nativeKclSize() const noexcept;
    std::size_t nativeAttributesSize() const noexcept;

    TVec3f getTrans();
    void init(const TPos3f&, HitSensor*, const void*, const void*, s32, bool);
    void addToBelongZone();
    void removeFromBelongZone();
    void initWithAutoEqualScale(const TPos3f&, HitSensor*, const void*, const void*, s32, bool);
    void initWithNotUsingScale(const TPos3f&, HitSensor*, const void*, const void*, s32, bool);
    void resetAllMtx(const TPos3f&);
    void resetAllMtx();
    void forceResetAllMtxAndSetUpdateMtxOneTime();
    void resetAllMtxPrivate(const TPos3f&);
    void setMtx(const TPos3f&);
    void setMtx();
    void updateMtx();
    f32 makeEqualScale(MtxPtr);
    void updateBoundingSphereRange();
    void updateBoundingSphereRange(TVec3f);
    void updateBoundingSphereRangePrivate(f32);
    const char* getHostName() const;
    s32 getPlacementZoneID() const;
    bool checkStrikePoint(HitInfo*, const TVec3f&);
    u32 checkStrikeBall(HitInfo*, u32, const TVec3f&, f32, bool, const TriangleFilterBase*);
    u32 checkStrikeBallCore(HitInfo*, u32, const TVec3f&, const TVec3f&, f32, f32, f32, KC_PrismData**, f32*, u8*, const TriangleFilterBase*,
                            const TVec3f*);
    u32 checkStrikeBallWithThickness(HitInfo*, u32, const TVec3f&, f32, f32, const TriangleFilterBase*);
    void calcCollidePosition(TVec3f*, const KC_PrismData&, u8);
    void projectToPlane(TVec3f*, const TVec3f&, const TVec3f&, const TVec3f&);
    u32 checkStrikeLine(HitInfo*, u32, const TVec3f&, const TVec3f&, const TriangleFilterBase*);
    u32 createAreaPolygonList(Triangle*, u32, const TVec3f&, const TVec3f&);
    u32 createAreaPolygonListArray(Triangle*, u32, TVec3f*, u32);
    void calcForceMovePower(TVec3f*, const TVec3f&) const;

    TPos3f* _0;
    TPos3f mMatrix;             // 0x4
    TPos3f mBaseMatrix;         // 0x34
    TPos3f mInvBaseMatrix;      // 0x64
    TPos3f mPrevBaseMatrix;     // 0x94
    KCollisionServer* mServer;  // 0xC4
    HitSensor* mHitSensor;      // 0xC8
    bool _CC;
    bool _CD;
    bool _CE;
    bool _CF;
    bool _D0;
    u8 _D1[3];
    s32 _D4;
    f32 _D8;
    f32 _DC;
    s32 mKeeperIndex;      // 0xE0
    CollisionZone* mZone;  // 0xE4
    f32 _E8;
    f32 _EC;
    f32 _F0;

private:
    void validateNativeMatrices();
    void publishNativeGeometry();
    struct NativeResources;
    std::unique_ptr< NativeResources > mNativeResources;
    std::shared_ptr< const void > mNativeLifetime;
};
