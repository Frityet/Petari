#include "Game/Map/CollisionParts.hpp"
#include "Game/Camera/CameraPolygonCodeUtil.hpp"
#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/CollisionCategorizedKeeper.hpp"
#include "Game/Map/CollisionDirector.hpp"
#include "Game/Map/HitInfo.hpp"
#include "Game/Map/KCollision.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/System/ResourceHolder.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/MathUtil.hpp"
#include "Game/Util/MtxUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "Game/Util/TriangleFilter.hpp"
#include "resource/KCollisionResource.hpp"
#include "resource/RarcArchive.hpp"
#include <aurora/allocation.hpp>
#include <aurora/exception.hpp>
#include <cmath>
#include <cstdio>
#include <span>
#include <stdexcept>
#include <string>

struct CollisionParts::NativeResources {
    std::shared_ptr< const void > resource_owner;
    SceneObjHolder* scene_holder = nullptr;
    std::span< const std::uint8_t > kcl;
    std::span< const std::uint8_t > attributes;
    std::string resource_name;
    std::string source;
    std::unique_ptr< smgpc::resource::KCollisionResource > decoded;
    std::shared_ptr< smgpc::resource::GeneratedKCollisionResource > generated;
    s32 generated_prism_count = 0;
    bool geometry_published = true;
};

namespace {
    void validateMatrix(const TPos3f& matrix) {
        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 4; ++col)
                if (!std::isfinite(matrix.mMtx[row][col]))
                    aurora::throw_host_exception< std::invalid_argument >("Collision matrix contains a non-finite component.");
        Mtx inverse;
        if (!PSMTXInverse(matrix.toMtxPtr(), inverse))
            aurora::throw_host_exception< std::invalid_argument >("Collision matrix must be invertible.");
    }

    bool isFinite(const TVec3f& vector) {
        return std::isfinite(vector.x) && std::isfinite(vector.y) && std::isfinite(vector.z);
    }

    void validateGeneratedPrisms(KCollisionServer& server) {
        for (s32 i = 0; i < server.getTriangleNum(); ++i) {
            auto* prism = server.getPrismData(i);
            // Original KCollision deliberately ignores collapsed/nonpositive
            // faces. Do not reconstruct vertices with undefined divisors.
            if (prism->mHeight <= 0.0f)
                continue;
            const auto a = server.getPos(prism, 0);
            const auto b = server.getPos(prism, 1);
            const auto c = server.getPos(prism, 2);
            const auto ab = b - a;
            const auto ac = c - a;
            const auto bc = c - b;
            const auto area = ab.cross(ac).length();
            if (!isFinite(a) || !isFinite(b) || !isFinite(c) || !std::isfinite(area) || !(area > 1.0e-8f) || !(ab.length() > 1.0e-8f) ||
                !(ac.length() > 1.0e-8f) || !(bc.length() > 1.0e-8f))
                aurora::throw_host_exception< std::invalid_argument >("Generated collision contains a degenerate active prism.");
            const TVec3f* normals[] = {server.getFaceNormal(prism), server.getEdgeNormal1(prism), server.getEdgeNormal2(prism),
                                       server.getEdgeNormal3(prism)};
            for (auto* normal : normals)
                if (!isFinite(*normal) || !(normal->squared() > 1.0e-12f))
                    aurora::throw_host_exception< std::invalid_argument >("Generated collision contains an invalid active normal.");
        }
    }
}  // namespace

CollisionParts::~CollisionParts() {
    mNativeLifetime.reset();
    if (mNativeResources && _CC && MR::getSceneObjHolder() == mNativeResources->scene_holder &&
        mNativeResources->scene_holder->isExist(SceneObj_CollisionDirector))
        MR::invalidateCollisionParts(this);
    delete mServer->mapInfo;
    delete mServer;
}

std::string_view CollisionParts::nativeResourceName() const noexcept {
    return mNativeResources ? mNativeResources->resource_name : std::string_view{};
}
std::string_view CollisionParts::nativeResourceSource() const noexcept {
    return mNativeResources ? mNativeResources->source : std::string_view{};
}
std::size_t CollisionParts::nativeKclSize() const noexcept {
    return mNativeResources ? mNativeResources->kcl.size() : 0;
}
std::size_t CollisionParts::nativeAttributesSize() const noexcept {
    return mNativeResources ? mNativeResources->attributes.size() : 0;
}

void CollisionParts::initFromResource(ResourceHolder* resources, const char* name, HitSensor* sensor, const TPos3f& matrix, int scale_type,
                                      s32 category) {
    const aurora::allocation::HostAllocationScope host;
    auto* holder = MR::getSceneObjHolder();
    if (!resources || !name || !sensor || !sensor->mHost || !holder) {
        aurora::throw_host_exception< std::logic_error >("CollisionParts requires its actor, resources, scene and collision owners.");
    }
    if (category < 0 || category > 3)
        aurora::throw_host_exception< std::invalid_argument >("Collision category must be 0, 1, 2 or 3.");
    if (scale_type < MR::CollisionScaleType_AutoEqualScale || scale_type > MR::CollisionScaleType_Unk2) {
        aurora::throw_host_exception< std::invalid_argument >("CollisionParts scale policy is outside the original enum.");
    }
    const auto placement_zone_id = MR::getCurrentPlacementZoneId();
    if (placement_zone_id < 0 || placement_zone_id >= MR::getZoneNum() || MR::getZoneNum() > 32) {
        aurora::throw_host_exception< std::logic_error >("CollisionParts requires a valid original placement zone.");
    }
    const auto& archive = resources->nativeResourceSource();
    // Retail resource lookup uses two 0x80-byte filename buffers.
    char kcl_name[0x80];
    char attributes_name[0x80];
    std::snprintf(kcl_name, sizeof(kcl_name), "%s.kcl", name);
    std::snprintf(attributes_name, sizeof(attributes_name), "%s.pa", name);
    const auto* kcl_entry = archive.find_resource(kcl_name);
    const auto* attributes_entry = archive.find_resource(attributes_name);
    if (!kcl_entry)
        aurora::throw_host_exception< std::runtime_error >("Required CollisionParts KCL is unavailable: " + std::string(kcl_name));
    auto state = std::make_unique< NativeResources >();
    state->resource_owner = resources->retainNativeResources();
    state->scene_holder = holder;
    state->resource_name = name;
    state->source = resources->nativeResourcePath().generic_string() + ":/" + kcl_entry->path;
    state->kcl = archive.file_data(*kcl_entry);
    if (attributes_entry) {
        state->attributes = archive.file_data(*attributes_entry);
    }
    state->decoded = std::make_unique< smgpc::resource::KCollisionResource >(state->kcl, state->attributes);
    {
        const aurora::allocation::ClientAllocationScope client;
        auto* director = static_cast< CollisionDirector* >(MR::createSceneObj(SceneObj_CollisionDirector));
        if (director == nullptr) {
            aurora::throw_host_exception< std::logic_error >("CollisionParts requires its original CollisionDirector.");
        }
        auto* data = state->decoded->native_file();
        auto* attrs = state->decoded->attributes_data();
        switch (scale_type) {
        case MR::CollisionScaleType_AutoEqualScale:
            initWithAutoEqualScale(matrix, sensor, data, attrs, category, false);
            break;
        case MR::CollisionScaleType_NotUsingScale:
            initWithNotUsingScale(matrix, sensor, data, attrs, category, false);
            break;
        case MR::CollisionScaleType_Unk2:
            init(matrix, sensor, data, attrs, category, false);
            break;
        }
    }
    mNativeResources = std::move(state);
    validateNativeMatrices();
}

void CollisionParts::initFromGeneratedResource(std::shared_ptr< smgpc::resource::GeneratedKCollisionResource > resource, HitSensor* sensor,
                                               const TPos3f& matrix, s32 category) {
    const aurora::allocation::HostAllocationScope host;
    auto* holder = MR::getSceneObjHolder();
    if (!resource || !sensor || !sensor->mHost || !holder) {
        aurora::throw_host_exception< std::logic_error >("Generated CollisionParts requires its geometry, actor, scene and collision owners.");
    }
    if (category < 0 || category > 3)
        aurora::throw_host_exception< std::invalid_argument >("Collision category must be 0, 1, 2 or 3.");
    const auto zone = MR::getCurrentPlacementZoneId();
    if (zone < 0 || zone >= MR::getZoneNum() || MR::getZoneNum() > 32) {
        aurora::throw_host_exception< std::logic_error >("Generated CollisionParts requires a valid original placement zone.");
    }
    {
        const aurora::allocation::ClientAllocationScope client;
        auto* director = static_cast< CollisionDirector* >(MR::createSceneObj(SceneObj_CollisionDirector));
        if (!director) {
            aurora::throw_host_exception< std::logic_error >("Generated CollisionParts requires its original CollisionDirector.");
        }
    }
    auto state = std::make_unique< NativeResources >();
    state->scene_holder = holder;
    state->resource_name = sensor->mHost->mName;
    state->source = "generated:" + state->resource_name;
    state->generated = std::move(resource);
    {
        const aurora::allocation::ClientAllocationScope client;
        init(matrix, sensor, state->generated->native_file(), nullptr, category, true);
    }
    state->generated_prism_count = mServer->getTriangleNum();
    mNativeResources = std::move(state);
    publishNativeGeometry();
}

void CollisionParts::publishNativeGeometry() {
    auto* state = mNativeResources.get();
    if (!state || !state->generated)
        return;
    if (MR::getSceneObjHolder() != state->scene_holder)
        aurora::throw_host_exception< std::logic_error >("Generated collision update requires its live scene owner.");
    state->geometry_published = false;
    state->generated->validate();
    if (mServer->mFile != state->generated->native_file() || mServer->getTriangleNum() != state->generated_prism_count)
        aurora::throw_host_exception< std::logic_error >("Generated collision changed its owned file or prism count.");
    validateGeneratedPrisms(*mServer);
    validateNativeMatrices();
    state->geometry_published = true;
}

void CollisionParts::validateNativeMatrices() {
    if (!mNativeResources)
        return;
    if (MR::getSceneObjHolder() != mNativeResources->scene_holder)
        aurora::throw_host_exception< std::logic_error >("Collision matrix update requires its live scene owner.");
    validateMatrix(mBaseMatrix);
    validateMatrix(mPrevBaseMatrix);
}

void CollisionParts::requireNativeGeometryPublished() const {
    if (_CC && mNativeResources && !mNativeResources->geometry_published)
        aurora::throw_host_exception< std::logic_error >("Generated collision queries require successful geometry validation after mutation.");
}

[[maybe_unused]] static void FORCE_SCALE() {
    TVec3f vec;
    vec.scale(1.0f);
}

CollisionParts::CollisionParts()
    : _0(), mHitSensor(), _CC(), _CD(true), _CE(), _CF(), _D0(), _D4(), _D8(-1.0f), _DC(1.0f), mKeeperIndex(-1), mZone() {
    {
        const aurora::allocation::HostAllocationScope host;
        mNativeLifetime = std::make_shared< char >();
    }
    mServer = new KCollisionServer();

    mPrevBaseMatrix.identity();
    mBaseMatrix.identity();
    mMatrix.identity();
    PSMTXInverse(mBaseMatrix.toMtxPtr(), mInvBaseMatrix.toMtxPtr());
}

void CollisionParts::init(const TPos3f& a1, HitSensor* pHitSensor, const void* pKclData, const void* pMapInfo, s32 keeperIndex, bool a6) {
    mServer->init(const_cast< void* >(pKclData), pMapInfo);
    mHitSensor = pHitSensor;

    resetAllMtx(a1);

    TVec3f scale;
    mBaseMatrix.getScale(scale);

    mZone = MR::getCollisionDirector()->getCategoryKeeper(keeperIndex)->getZone(MR::getCurrentPlacementZoneId());

    MR::initCameraCodeCollection(pHitSensor->mHost->mName, mZone->mZoneID);
    mServer->calcFarthestVertexDistance();
    MR::termCameraCodeCollection();

    updateBoundingSphereRange(scale);
    mKeeperIndex = keeperIndex;
}

void CollisionParts::addToBelongZone() {
    s32 zoneID = mZone->mZoneID;

    MR::getCollisionDirector()->getCategoryKeeper(mKeeperIndex)->addToZone(this, zoneID);
}

void CollisionParts::removeFromBelongZone() {
    s32 zoneID = mZone->mZoneID;

    MR::getCollisionDirector()->getCategoryKeeper(mKeeperIndex)->removeFromZone(this, zoneID);
}

void CollisionParts::initWithAutoEqualScale(const TPos3f& a1, HitSensor* pHitSensor, const void* pKclData, const void* pMapInfo, s32 keeperIndex,
                                            bool a6) {
    _CF = true;
    _D0 = false;

    init(a1, pHitSensor, pKclData, pMapInfo, keeperIndex, a6);
}

void CollisionParts::initWithNotUsingScale(const TPos3f& a1, HitSensor* pHitSensor, const void* pKclData, const void* pMapInfo, s32 keeperIndex,
                                           bool a6) {
    _CF = false;
    _D0 = true;

    init(a1, pHitSensor, pKclData, pMapInfo, keeperIndex, a6);
}

void CollisionParts::resetAllMtx(const TPos3f& a1) {
    bool reset = false;

    if (_CD || _CE) {
        reset = true;
    }

    if (!reset) {
        return;
    }

    resetAllMtxPrivate(a1);
}

void CollisionParts::resetAllMtx() {
    bool reset = false;

    if (_CD || _CE) {
        reset = true;
    }

    if (reset) {
        TPos3f matrix(_0);
        makeEqualScale(reinterpret_cast< MtxPtr >(&matrix));

        resetAllMtxPrivate(matrix);
    }
}

void CollisionParts::forceResetAllMtxAndSetUpdateMtxOneTime() {
    TPos3f matrix(_0);
    makeEqualScale(reinterpret_cast< MtxPtr >(&matrix));
    resetAllMtxPrivate(matrix);

    _CE = true;
}

void CollisionParts::resetAllMtxPrivate(const TPos3f& a1) {
    mPrevBaseMatrix.setInline(a1);
    mBaseMatrix.setInline(a1);
    mMatrix.setInline(a1);
    PSMTXInverse(reinterpret_cast< MtxPtr >(&mBaseMatrix), reinterpret_cast< MtxPtr >(&mInvBaseMatrix));
    validateNativeMatrices();
}

void CollisionParts::setMtx(const TPos3f& matrix) {
    mMatrix.setInline(matrix);
}

void CollisionParts::setMtx() {
    mMatrix.setInline(_0);
}

void CollisionParts::updateMtx() {
    bool bVar1 = false;

    if (_CD || _CE) {
        bVar1 = true;
    }

    if (!bVar1) {
        if (MR::isSameMtx(reinterpret_cast< MtxPtr >(&mMatrix), reinterpret_cast< MtxPtr >(&mBaseMatrix))) {
            _D4++;
        }
    } else {
        if (MR::isSameMtx(reinterpret_cast< MtxPtr >(&mMatrix), reinterpret_cast< MtxPtr >(&mBaseMatrix))) {
            _D4++;
        } else {
            if (_CE) {
                _D4 = 1;
            } else {
                _D4 = 0;
            }

            f32 dVar4 = makeEqualScale(reinterpret_cast< MtxPtr >(&mMatrix));
            _E8 = dVar4;
            f32 var = dVar4 - _DC;
            _EC = dVar4;
            _F0 = dVar4;

            if (!MR::isNearZero(var)) {
                updateBoundingSphereRangePrivate(dVar4);
            }
        }

        _CE = false;

        if (_D4 < 2) {
            mPrevBaseMatrix.setInline(mBaseMatrix);
            mBaseMatrix.setInline(mMatrix);
            PSMTXInverse(reinterpret_cast< MtxPtr >(&mBaseMatrix), reinterpret_cast< MtxPtr >(&mInvBaseMatrix));
        }
    }
    validateNativeMatrices();
}

// Issues with assignments of scaleDiff
f32 CollisionParts::makeEqualScale(MtxPtr matrix) {
    TPos3f& mtx = *reinterpret_cast< TPos3f* >(matrix);

    TVec3f scale;
    mtx.getScale(scale);

    TVec3f scaleDiff;
    scaleDiff.x = scale.z - scale.x;
    scaleDiff.y = scale.y - scale.z;
    scaleDiff.z = scale.x - scale.y;

    if (MR::isNearZero(scaleDiff.x) && MR::isNearZero(scaleDiff.y) && MR::isNearZero(scaleDiff.z)) {
        return scale.x;
    }

    f32 uniformScale = 1.0f;
    TVec3f invScale;

    if (_D0) {
        invScale.set(uniformScale / scale.x, uniformScale / scale.y, uniformScale / scale.z);
        uniformScale = 1.0f;
    } else if (_CF) {
        uniformScale = (scale.x + scale.y + scale.z) / 3.0f;
        invScale.set(uniformScale / scale.x, uniformScale / scale.y, uniformScale / scale.z);
    }

    mtx.mMtx[0][0] *= invScale.x;
    mtx.mMtx[1][0] *= invScale.x;
    mtx.mMtx[2][0] *= invScale.x;

    mtx.mMtx[0][1] *= invScale.y;
    mtx.mMtx[1][1] *= invScale.y;
    mtx.mMtx[2][1] *= invScale.y;

    mtx.mMtx[0][2] *= invScale.z;
    mtx.mMtx[1][2] *= invScale.z;
    mtx.mMtx[2][2] *= invScale.z;

    return uniformScale;
}

void CollisionParts::updateBoundingSphereRange() {
    TPos3f matrix(_0);
    f32 scale = makeEqualScale(reinterpret_cast< MtxPtr >(&matrix));
    updateBoundingSphereRangePrivate(scale);
}

void CollisionParts::updateBoundingSphereRange(TVec3f a1) {
    f32 range = (a1.x + a1.y + a1.z) / 3.0f;
    updateBoundingSphereRangePrivate(range);
}

void CollisionParts::updateBoundingSphereRangePrivate(f32 scale) {
    _DC = scale;
    _D8 = scale * mServer->mMaxVertexDistance;
    publishNativeGeometry();
}

const char* CollisionParts::getHostName() const {
    if (mHitSensor == nullptr) {
        return nullptr;
    }

    LiveActor* actor = mHitSensor->mHost;

    if (actor == nullptr) {
        return nullptr;
    }

    return actor->mName;
}

s32 CollisionParts::getPlacementZoneID() const {
    return mZone->mZoneID;
}

// Instruction order
bool CollisionParts::checkStrikePoint(HitInfo* pHitInfo, const TVec3f& rPos) {
    requireNativeGeometryPublished();
    TVec3f localPos;
    mInvBaseMatrix.mult(rPos, localPos);
    TVec3f scale;
    mInvBaseMatrix.getScale(scale);
    f32 localScale = (scale.x + scale.y + scale.z) / 3.0f;
    Fxyz position;
    position.x = localPos.x;
    position.y = localPos.y;
    position.z = localPos.z;
    KC_PrismData* pPrism = nullptr;
    f32 distance;

    if (1.0f < localScale) {
        f32 radius = 20.0f * localScale;
        u8 feature;
        mServer->checkSphereWithThickness(&position, radius, localScale, 1, &pPrism, &distance, &feature, 2.0f * radius);
        if (pPrism == nullptr) {
            return false;
        }
        TVec3f offset(localPos);
        offset.sub(mServer->getPos(pPrism, 0));
        TVec3f normal(*mServer->getFaceNormal(pPrism));
        distance = -offset.x * normal.x - offset.y * normal.y - offset.z * normal.z;
    } else {
        pPrism = mServer->checkPoint(&position, localScale, &distance);
        if (pPrism == nullptr) {
            return false;
        }
    }

    if (pHitInfo != nullptr) {
        pHitInfo->mParentTriangle.fillData(this, mServer->toIndex(pPrism), mHitSensor);
        f32 worldDistance = distance / localScale;
        pHitInfo->_60 = worldDistance;
        TVec3f offset(*pHitInfo->mParentTriangle.getNormal(0));
        offset.scale(worldDistance);
        TVec3f hitPos(rPos);
        hitPos.add(offset);
        pHitInfo->mHitPos = hitPos;
    }
    return true;
}

u32 CollisionParts::checkStrikeBall(HitInfo* pHitInfo, u32 capacity, const TVec3f& rPos, f32 radius, bool movingReaction,
                                    const TriangleFilterBase* pFilter) {
    requireNativeGeometryPublished();
    KC_PrismData* prisms[64];
    f32 distances[64];
    u8 features[64];
    TVec3f localPos;
    mInvBaseMatrix.mult(rPos, localPos);

    TVec3f scale;
    mInvBaseMatrix.getScale(scale);
    f32 localScale = (scale.x + scale.y + scale.z) / 3.0f;
    f32 worldScale = 1.0f / localScale;
    radius *= localScale;
    TVec3f movePower(0, 0, 0);

    if (movingReaction && _D4 == 0) {
        TPos3f inversePrevious;
        PSMTXInverse(mPrevBaseMatrix.toMtxPtr(), inversePrevious.toMtxPtr());
        TVec3f previousPos;
        inversePrevious.mult(rPos, previousPos);
        TVec3f movement = localPos - previousPos;
        TVec3f worldMovement(movement);
        mBaseMatrix.mult33(worldMovement, worldMovement);
        s32 stepCount = static_cast< s32 >((1.0f / 35.0f) * movement.length()) + 1;
        TVec3f step(movement);

        if (stepCount > 1) {
            step.scale(1.0f / stepCount);
        }

        TVec3f offset(0.0f, 0.0f, 0.0f);

        for (s32 i = 0; i <= stepCount; i++) {
            movePower.set(-(movement - offset));
            mBaseMatrix.mult33(movePower, movePower);
            const TVec3f* pRejectNormal = &worldMovement;

            if (i == stepCount) {
                pRejectNormal = nullptr;
            }

            u32 count = checkStrikeBallCore(pHitInfo, capacity, previousPos + offset, movePower, radius, localScale, worldScale, prisms, distances,
                                            features, pFilter, pRejectNormal);

            if (count != 0) {
                return count;
            }

            offset.add(step);
        }

        return 0;
    }

    return checkStrikeBallCore(pHitInfo, capacity, localPos, TVec3f(0, 0, 0), radius, localScale, worldScale, prisms, distances, features, pFilter,
                               nullptr);
}

u32 CollisionParts::checkStrikeBallCore(HitInfo* pHitInfo, u32 capacity, const TVec3f& rLocalPos, const TVec3f& rMovePower, f32 radius,
                                        f32 localScale, f32 worldScale, KC_PrismData** pPrisms, f32* pDistances, u8* pFeatures,
                                        const TriangleFilterBase* pFilter, const TVec3f* pRejectNormal) {
    u32 count = mServer->checkSphere(reinterpret_cast< Fxyz* >(const_cast< TVec3f* >(&rLocalPos)), radius, localScale, capacity, pPrisms, pDistances,
                                     pFeatures);
    u32 acceptedCount = 0;

    for (u32 i = 0; i < count; i++) {
        HitInfo* pHit = &pHitInfo[acceptedCount];
        TVec3f position(rLocalPos);
        calcCollidePosition(&position, *pPrisms[i], pFeatures[i]);
        mBaseMatrix.mult(position, pHit->mHitPos);
        pHit->mParentTriangle.fillData(this, mServer->toIndex(pPrisms[i]), mHitSensor);

        if (pFilter != nullptr && pFilter->isInvalidTriangle(&pHit->mParentTriangle)) {
            continue;
        }

        if (pRejectNormal != nullptr && 0.0f < pRejectNormal->dot(*pHit->mParentTriangle.getFaceNormal())) {
            continue;
        }

        pHit->_60 = worldScale * pDistances[i];
        pHit->_88 = pFeatures[i];
        pHit->_7C.set(rMovePower);
        const TVec3f* pNormal = pHit->mParentTriangle.getFaceNormal();
        pHit->_7C.scale(pNormal->dot(pHit->_7C), *pNormal);
        acceptedCount++;
    }

    return acceptedCount;
}

u32 CollisionParts::checkStrikeBallWithThickness(HitInfo* pHitInfo, u32 capacity, const TVec3f& rPos, f32 radius, f32 thickness,
                                                 const TriangleFilterBase* pFilter) {
    requireNativeGeometryPublished();
    KC_PrismData* prisms[64];
    f32 distances[64];
    u8 features[64];
    TVec3f localPos;
    mInvBaseMatrix.mult(rPos, localPos);
    TVec3f scale;
    mInvBaseMatrix.getScale(scale);
    f32 localScale = (scale.x + scale.y + scale.z) / 3.0f;
    Fxyz position;
    position.x = localPos.x;
    position.y = localPos.y;
    position.z = localPos.z;
    u32 count = mServer->checkSphereWithThickness(&position, radius * localScale, localScale, capacity, prisms, distances, features, thickness);
    f32 worldScale = 1.0f / localScale;
    u32 acceptedCount = 0;

    for (u32 i = 0; i < count; i++) {
        HitInfo* pHit = &pHitInfo[acceptedCount];
        TVec3f hitPos(localPos);
        calcCollidePosition(&hitPos, *prisms[i], features[i]);
        mBaseMatrix.mult(hitPos, pHit->mHitPos);
        pHit->mParentTriangle.fillData(this, mServer->toIndex(prisms[i]), mHitSensor);

        if (pFilter != nullptr && pFilter->isInvalidTriangle(&pHit->mParentTriangle)) {
            continue;
        }

        pHit->_60 = worldScale * distances[i];
        pHit->_88 = features[i];
        acceptedCount++;
    }

    return acceptedCount;
}

void CollisionParts::calcCollidePosition(TVec3f* pPos, const KC_PrismData& rPrism, u8 feature) {
    TVec3f offset;
    TVec3f edgeNormal;

    switch (feature) {
    case 1:
        projectToPlane(pPos, *pPos, mServer->getPos(&rPrism, 0), *mServer->getNormal(rPrism.mNormalIndex));
        break;
    case 2:
        projectToPlane(pPos, *pPos, mServer->getPos(&rPrism, 0), *mServer->getNormal(rPrism.mNormalIndex));
        edgeNormal.set(*mServer->getNormal(rPrism.mEdgeIndices[0]));
        offset.set(*pPos);
        offset.sub(mServer->getPos(&rPrism, 0));
        pPos->add(-edgeNormal * offset.dot(edgeNormal));
        break;
    case 3:
        projectToPlane(pPos, *pPos, mServer->getPos(&rPrism, 0), *mServer->getNormal(rPrism.mNormalIndex));
        edgeNormal.set(*mServer->getNormal(rPrism.mEdgeIndices[1]));
        offset.set(*pPos);
        offset.sub(mServer->getPos(&rPrism, 0));
        pPos->add(-edgeNormal * offset.dot(edgeNormal));
        break;
    case 4:
        projectToPlane(pPos, *pPos, mServer->getPos(&rPrism, 0), *mServer->getNormal(rPrism.mNormalIndex));
        edgeNormal.set(*mServer->getNormal(rPrism.mEdgeIndices[2]));
        offset.set(*pPos);
        offset.sub(mServer->getPos(&rPrism, 1));
        pPos->add(-edgeNormal * offset.dot(edgeNormal));
        break;
    case 5:
        pPos->set(mServer->getPos(&rPrism, 0));
        break;
    case 6:
        pPos->set(mServer->getPos(&rPrism, 1));
        break;
    case 7:
        pPos->set(mServer->getPos(&rPrism, 2));
        break;
    }
}

void CollisionParts::projectToPlane(TVec3f* pProjected, const TVec3f& rPos, const TVec3f& rOrigin, const TVec3f& rNormal) {
    TVec3f projected = rPos;

    f32 distance = (rPos - rOrigin).dot(rNormal);

    projected.add(-rNormal * distance);
    pProjected->set(projected);
}

u32 CollisionParts::checkStrikeLine(HitInfo* pInfos, u32 maxCount, const TVec3f& rStart, const TVec3f& rOffset, const TriangleFilterBase* pFilter) {
    requireNativeGeometryPublished();
    f32 length = PSVECMag(&rOffset);
    TVec3f localStart;
    TVec3f localOffset;
    mInvBaseMatrix.mult(rStart, localStart);
    mInvBaseMatrix.mult(rStart + rOffset, localOffset);
    localOffset = localOffset - localStart;

    f32 fractions[64];
    KC_PrismData* prisms[64];
    // Retail checkArrow leaves all-hit flags unwritten; define those stack bytes on PC.
    u8 flags[64] = {};
    u32 foundCount = 0;
    mServer->checkArrow(localStart, localOffset, fractions, flags, &foundCount, prisms, maxCount);

    u32 hitCount = 0;
    for (u32 i = 0; i < foundCount; i++) {
        HitInfo* pInfo = &pInfos[hitCount];
        TVec3f hitPos = localStart + localOffset * fractions[i];
        mBaseMatrix.mult(hitPos, hitPos);
        pInfo->mParentTriangle.fillData(this, mServer->toIndex(prisms[i]), mHitSensor);
        if (pFilter != nullptr && pFilter->isInvalidTriangle(&pInfo->mParentTriangle)) {
            continue;
        }

        pInfo->_60 = length * fractions[i];
        pInfo->mHitPos = hitPos;
        pInfo->_88 = flags[i];
        hitCount++;
    }
    return hitCount;
}

void CollisionParts::calcForceMovePower(TVec3f* a1, const TVec3f& a2) const {
    TVec3f tStack88 = a2;
    TMtx34f auStack76;
    PSMTXInverse((MtxPtr)&mPrevBaseMatrix, reinterpret_cast< MtxPtr >(&auStack76));

    auStack76.mult(tStack88, tStack88);
    mBaseMatrix.mult(tStack88, tStack88);

    tStack88.sub(a2);
    *a1 = tStack88;
}

u32 CollisionParts::createAreaPolygonList(Triangle* pTriangles, u32 capacity, const TVec3f& rStart, const TVec3f& rEnd) {
    requireNativeGeometryPublished();
    KC_PrismData* prisms[512];
    TPos3f rotation;
    PSMTXCopy(mInvBaseMatrix.toMtxPtr(), rotation.toMtxPtr());
    rotation.zeroTrans();
    TVec3f minimum;
    TVec3f maximum;
    mInvBaseMatrix.mult(rStart, minimum);
    mInvBaseMatrix.mult(rEnd, maximum);
    u32 count = mServer->checkArea3D(reinterpret_cast< Fxyz* >(&minimum), reinterpret_cast< Fxyz* >(&maximum), prisms, capacity);
    if (count == 0) {
        return 0;
    }
    for (u32 i = 0; i < count; i++) {
        pTriangles[i].fillData(this, mServer->toIndex(prisms[i]), mHitSensor);
    }
    return count;
}

u32 CollisionParts::createAreaPolygonListArray(Triangle* pTriangles, u32 capacity, TVec3f* pPoints, u32 pointCount) {
    requireNativeGeometryPublished();
    KC_PrismData* prisms[512];
    TVec3f localPoints[32];
    TPos3f rotation;
    PSMTXCopy(mInvBaseMatrix.toMtxPtr(), rotation.toMtxPtr());
    rotation.zeroTrans();
    for (u32 i = 0; i < pointCount; i++) {
        mInvBaseMatrix.mult(pPoints[i], localPoints[i]);
    }
    TVec3f minimum;
    TVec3f maximum;
    MR::createBoundingBox(localPoints, pointCount, &minimum, &maximum);
    u32 count = mServer->checkArea3D(reinterpret_cast< Fxyz* >(&minimum), reinterpret_cast< Fxyz* >(&maximum), prisms, capacity);
    if (count == 0) {
        return 0;
    }
    for (u32 i = 0; i < count; i++) {
        pTriangles[i].fillData(this, mServer->toIndex(prisms[i]), mHitSensor);
    }
    return count;
}
