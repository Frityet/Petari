#pragma once

#include "Game/Map/HitInfo.hpp"
#include "Game/Util/TriangleFilter.hpp"

class CollisionPartsFilterBase;
class CollisionParts;

class BinderParent {
public:
    explicit BinderParent(MtxPtr matrix)
        : mTriangleFilter(nullptr), mCollisionPartsFilter(nullptr), mCollisionParts(nullptr), _C(matrix) {
    }

    union {
        TriangleFilterBase* mTriangleFilter;
        int _0;
    };
    union {
        CollisionPartsFilterBase* mCollisionPartsFilter;
        int _4;
    };
    union {
        CollisionParts* mCollisionParts;
        u32 _8;
    };
    MtxPtr _C;
};

class Binder : public BinderParent {
public:
    Binder(MtxPtr, const TVec3f*, const TVec3f*, f32, f32, u32);
    ~Binder();

    void clear();
    void setCollisionPartsFilter(CollisionPartsFilterBase*);
    void setTriangleFilter(TriangleFilterBase*);
    const HitInfo* getPlane(int) const;
    u32 copyPlaneArrayAndSortingSensor(HitInfo**, u32);
    const TVec3f bind(const TVec3f&);
    static bool compSensor(const HitInfo*, const HitInfo*);
    void moveAlongHittedPlanes(TVec3f*, TVec3f*, TVec3f*, const TVec3f&, const TVec3f&, HitInfo*, u32, bool*);
    u32 findBindedPos(TVec3f*, TVec3f*, bool*, HitInfo*, u32, bool, bool);
    bool moveWithCollisionParts(TVec3f*, TVec3f*);
    u32 storeCurrentHitInfo(HitInfo*, u32, bool);
    void obtainMomentFixReaction(HitInfo*, u32, TVec3f*, u32);
    void storeContactPlane(HitInfo*, u32);
    void setExCollisionParts(CollisionParts*);

    bool isBindedGround() const {
        return 0.0F <= _C8;
    }

    bool isBindedWall() const {
        return 0.0F <= _158;
    }

    bool isBindedRoof() const {
        return 0.0F <= _1E8;
    }

    const TVec3f* _10;
    const TVec3f* _14;
    f32 mRadius;
    f32 _1C;
    const TVec3f* mOffsetVec;
    u32 _24;
    int mPlaneNum;
    HitInfo* mPlaneInfos;
    TVec3f mFixReactionVector;
    HitInfo mGroundInfo;
    f32 _C8;
    HitInfo mWallInfo;
    f32 _158;
    HitInfo mRoofInfo;
    f32 _1E8;

    struct {
        bool _0 : 1;
        bool _1 : 1;
        bool _2 : 1;
        bool _3 : 1;
        bool _4 : 1;
        bool _5 : 1;
    } _1EC;
};
