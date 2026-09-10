#include <aurora/exception.hpp>
#include <aurora/allocation.hpp>
#include "compat/HitInfoCompat.hpp"
#include "compat/CollisionPartsCompat.hpp"
#include "Game/Map/KCollision.hpp"
#include "Game/Util/MathUtil.hpp"

#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Map/CollisionParts.hpp"
#include "Game/Util/TriangleFilter.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>

namespace {
    struct AttributeCache {
        const smgpc::scene::StageCollisionService* service = nullptr;
        std::uint64_t generation = 0U;
        std::uint64_t revision = 0U;
        std::map<std::uint32_t, JMapInfo> sources{};
    };

    [[nodiscard]] AttributeCache& attribute_cache() {
        static thread_local auto cache = AttributeCache{};
        return cache;
    }

    [[nodiscard]] std::optional<smgpc::scene::StageCollisionSurface> triangle_surface(const Triangle& triangle) {
        if (triangle.mIdx == 0xFFFFFFFFU) {
            return std::nullopt;
        }
        auto* collision = triangle.mParts ? smgpc::compat::collision_service_for_parts(triangle.mParts) : smgpc::scene::StageCollisionService::active();
        if (!collision) return std::nullopt;
        return triangle.mParts ? collision->surface(triangle.mParts, triangle.mIdx) : collision->surface(triangle.mIdx);
    }
    [[nodiscard]] smgpc::scene::StageCollisionMatrices& triangle_matrices(const Triangle& triangle) {
        auto* collision = smgpc::scene::StageCollisionService::active();
        if (collision == nullptr) {
            aurora::throw_host_exception<std::logic_error>("Triangle transforms require an active collision owner.");
        }
        const auto surface = triangle_surface(triangle);
        if (!surface) aurora::throw_host_exception<std::logic_error>("Triangle transforms require their live collision part and prism.");
        return collision->matrices_for_triangle(surface->triangle_index);
    }
}  // namespace

namespace smgpc::compat {

    Triangle make_collision_triangle(const scene::StageCollisionService& collision,
                                     std::uint32_t triangle_index) {
        const auto surface = collision.surface(triangle_index);
        if (!surface.has_value()) {
            aurora::throw_host_exception<std::logic_error>("A collision hit requires its live source KCL prism.");
        }
        auto triangle = Triangle{};
        if (surface->parts) {
            triangle.fillData(surface->parts, surface->prism_index, surface->sensor);
            return triangle;
        }
        triangle.mParts = surface->parts;
        triangle.mIdx = surface->parts ? surface->prism_index : triangle_index;
        triangle.mSensor = surface->sensor;
        for (auto index = std::size_t{}; index < surface->vertices.size(); ++index) {
            triangle.mPos[index] = surface->vertices[index];
        }
        for (auto index = std::size_t{}; index < surface->normals.size(); ++index) {
            triangle.mNormals[index] = surface->normals[index];
        }
        return triangle;
    }

    scene::StageCollisionTriangleFilter make_collision_triangle_filter(
        const scene::StageCollisionService& collision, const TriangleFilterBase* filter) {
        if (filter == nullptr) {
            return {};
        }
        return [&collision, filter](std::uint32_t triangle_index) {
            const auto triangle = make_collision_triangle(collision, triangle_index);
            const aurora::allocation::ClientAllocationScope client_allocations;
            return !filter->isInvalidTriangle(&triangle);
        };
    }

}  // namespace smgpc::compat

Triangle::Triangle()
    : mParts(nullptr), mIdx(0xFFFFFFFFU), mSensor(nullptr), mNormals{}, mPos{} {
}

const char* Triangle::getHostName() const {
    const auto surface = triangle_surface(*this);
    // CollisionParts::getHostName reads the live sensor host's NameObj name.
    // A resource path is diagnostic metadata, not an actor identity.
    if (!surface.has_value() || surface->sensor == nullptr || surface->sensor->mHost == nullptr) {
        return nullptr;
    }
    return surface->sensor->mHost->mName;
}

s32 Triangle::getHostPlacementZoneID() const {
    const auto surface = triangle_surface(*this);
    if (!surface.has_value() || !surface->placement_zone_id.has_value()) {
        aurora::throw_host_exception<std::logic_error>("Triangle placement-zone lookup requires a live collision owner with placement provenance.");
    }
    return *surface->placement_zone_id;
}

bool Triangle::isHostMoved() const {
    return mParts != nullptr && triangle_surface(*this).has_value() && mParts->_D4 == 0;
}

void Triangle::calcForceMovePower(TVec3f* output, const TVec3f& position) const {
    if (mParts != nullptr) {
        if (!triangle_surface(*this)) aurora::throw_host_exception<std::logic_error>("Triangle motion requires its live collision part.");
        mParts->calcForceMovePower(output, position);
        return;
    }
    const auto& matrices = triangle_matrices(*this);
    // Original CollisionParts::calcForceMovePower advances the old world
    // point through inverse(previous) then current. Keep this distinct from
    // MR::calcVelocityMovingPoint, which starts at the current world point.
    TVec3f moved = position;
    TMtx34f inverse_previous;
    PSMTXInverse(matrices.previous.toMtxPtr(), inverse_previous.toMtxPtr());
    inverse_previous.mult(moved, moved);
    matrices.base.mult(moved, moved);
    moved.sub(position);
    *output = moved;
}

bool Triangle::isValid() const {
    if (mIdx == 0xFFFFFFFFU) {
        return false;
    }
    return triangle_surface(*this).has_value();
}

const TVec3f* Triangle::getNormal(int index) const {
    return index >= 0 && index < 4 ? &mNormals[index] : nullptr;
}

const TVec3f* Triangle::getFaceNormal() const {
    return &mNormals[0];
}

const TVec3f* Triangle::getEdgeNormal(int index) const {
    return index >= 0 && index < 3 ? &mNormals[index + 1] : nullptr;
}

const TVec3f* Triangle::getPos(int index) const {
    return index >= 0 && index < 3 ? &mPos[index] : nullptr;
}

const TVec3f* Triangle::calcAndGetNormal(int index) {
    if (mParts == nullptr) return getNormal(index);
    if (!triangle_surface(*this)) aurora::throw_host_exception<std::logic_error>("Triangle calculation requires its live collision part.");
    KCollisionServer* server = mParts->mServer;
    KC_PrismData* prism = server->getPrismData(mIdx);

    MtxPtr matrix = reinterpret_cast< MtxPtr >(&mParts->mBaseMatrix);

    switch (index) {
    case 0: {
        mNormals[0].set< f32 >(*server->getFaceNormal(prism));
        PSMTXMultVecSR(matrix, &mNormals[0], &mNormals[0]);
        MR::normalize(&mNormals[0]);

        return &mNormals[0];
    } break;
    case 1: {
        mNormals[1].set< f32 >(*server->getEdgeNormal1(prism));
        PSMTXMultVecSR(matrix, &mNormals[1], &mNormals[1]);
        MR::normalize(&mNormals[1]);

        return &mNormals[1];
    } break;
    case 2: {
        mNormals[2].set< f32 >(*server->getEdgeNormal2(prism));
        PSMTXMultVecSR(matrix, &mNormals[2], &mNormals[2]);
        MR::normalize(&mNormals[2]);

        return &mNormals[2];
    } break;
    case 3: {
        mNormals[3].set< f32 >(*server->getEdgeNormal3(prism));
        PSMTXMultVecSR(matrix, &mNormals[3], &mNormals[3]);
        MR::normalize(&mNormals[3]);

        return &mNormals[3];
    } break;
    }

    return &mNormals[index];
}

const TVec3f* Triangle::calcAndGetEdgeNormal(int index) {
    if (mParts == nullptr) return getEdgeNormal(index);
    if (!triangle_surface(*this)) aurora::throw_host_exception<std::logic_error>("Triangle calculation requires its live collision part.");
    KCollisionServer* server = mParts->mServer;
    KC_PrismData* prism = server->getPrismData(mIdx);

    MtxPtr matrix = reinterpret_cast< MtxPtr >(&mParts->mBaseMatrix);

    switch (index) {
    case 0: {
        mNormals[1].set< f32 >(*server->getEdgeNormal1(prism));
        PSMTXMultVecSR(matrix, &mNormals[1], &mNormals[1]);
        MR::normalize(&mNormals[1]);

        return &mNormals[1];
    } break;
    case 1: {
        mNormals[2].set< f32 >(*server->getEdgeNormal2(prism));
        PSMTXMultVecSR(matrix, &mNormals[2], &mNormals[2]);
        MR::normalize(&mNormals[2]);

        return &mNormals[2];
    } break;
    case 2: {
        mNormals[3].set< f32 >(*server->getEdgeNormal3(prism));
        PSMTXMultVecSR(matrix, &mNormals[3], &mNormals[3]);
        MR::normalize(&mNormals[3]);

        return &mNormals[3];
    } break;
    }

    return &mNormals[index + 1];
}

const TVec3f* Triangle::calcAndGetPos(int index) {
    if (mParts == nullptr) return getPos(index);
    if (!triangle_surface(*this)) aurora::throw_host_exception<std::logic_error>("Triangle calculation requires its live collision part.");
    KCollisionServer* server = mParts->mServer;
    KC_PrismData* prism = server->getPrismData(mIdx);

    TVec3f* pos = &mPos[index];

    pos->set< f32 >(server->getPos(prism, index));

    mParts->mBaseMatrix.mult(*pos, *pos);

    return pos;
}

JMapInfoIter Triangle::getAttributes() const {
    const aurora::allocation::HostAllocationScope host;
    if (mParts != nullptr) {
        return triangle_surface(*this) ? mParts->mServer->getAttributes(mIdx) : JMapInfoIter{};
    }
    const auto surface = triangle_surface(*this);
    if (!surface.has_value() || surface->attributes.empty()) {
        return {};
    }

    auto* collision = smgpc::scene::StageCollisionService::active();
    auto& cache = attribute_cache();
    if (cache.service != collision || cache.generation != collision->generation() ||
        cache.revision != collision->revision()) {
        cache.service = collision;
        cache.generation = collision->generation();
        cache.revision = collision->revision();
        cache.sources.clear();
    }

    auto found = cache.sources.find(surface->source_index);
    if (found == cache.sources.end()) {
        found = cache.sources.emplace(surface->source_index, JMapInfo::from_bcsv(surface->attributes)).first;
    }
    return JMapInfoIter(&found->second, static_cast<s32>(surface->attribute));
}

HitInfo::HitInfo()
    : mParentTriangle(), _60(0.0F), mHitPos(), _70(), _7C(), _88(0U), _89{} {
}

bool HitInfo::isCollisionAtFace() const {
    return _88 == 1U;
}

bool HitInfo::isCollisionAtEdge() const {
    return _88 == 2U || _88 == 3U || _88 == 4U;
}

bool HitInfo::isCollisionAtCorner() const {
    return _88 == 5U || _88 == 6U || _88 == 7U;
}

// Original parts retain their own matrices; native KCL triangles borrow the
// actual source transforms from their collision owner.
TPos3f* Triangle::getBaseMtx() const {
    if (mParts != nullptr) {
        if (!triangle_surface(*this)) aurora::throw_host_exception<std::logic_error>("Triangle transforms require their live collision part.");
        return &mParts->mBaseMatrix;
    }
    return &triangle_matrices(*this).base;
}

TPos3f* Triangle::getBaseInvMtx() const {
    if (mParts != nullptr) {
        if (!triangle_surface(*this)) aurora::throw_host_exception<std::logic_error>("Triangle transforms require their live collision part.");
        return &mParts->mInvBaseMatrix;
    }
    return &triangle_matrices(*this).inverse;
}

TPos3f* Triangle::getPrevBaseMtx() const {
    if (mParts != nullptr) {
        if (!triangle_surface(*this)) aurora::throw_host_exception<std::logic_error>("Triangle transforms require their live collision part.");
        return &mParts->mPrevBaseMatrix;
    }
    return &triangle_matrices(*this).previous;
}

void Triangle::fillData(CollisionParts* pParts, u32 index, HitSensor* pSensor) {
    mParts = pParts;
    mIdx = index;
    mSensor = pSensor;

    KCollisionServer* server = pParts->mServer;
    MtxPtr matrix;
    KC_PrismData* prism = server->getPrismData(index);

    mNormals[0].set< f32 >(*server->getFaceNormal(prism));
    mNormals[1].set< f32 >(*server->getEdgeNormal1(prism));
    mNormals[2].set< f32 >(*server->getEdgeNormal2(prism));
    mNormals[3].set< f32 >(*server->getEdgeNormal3(prism));

    matrix = reinterpret_cast< MtxPtr >(&mParts->mBaseMatrix);

    PSMTXMultVecSR(matrix, &mNormals[0], &mNormals[0]);
    PSMTXMultVecSR(matrix, &mNormals[1], &mNormals[1]);
    PSMTXMultVecSR(matrix, &mNormals[2], &mNormals[2]);
    PSMTXMultVecSR(matrix, &mNormals[3], &mNormals[3]);

    MR::normalize(&mNormals[0]);
    MR::normalize(&mNormals[1]);
    MR::normalize(&mNormals[2]);
    MR::normalize(&mNormals[3]);

    mPos[0].set< f32 >(server->getPos(prism, 0));
    mPos[1].set< f32 >(server->getPos(prism, 1));
    mPos[2].set< f32 >(server->getPos(prism, 2));

    PSMTXMultVecSR(matrix, &mPos[0], &mPos[0]);
    PSMTXMultVecSR(matrix, &mPos[1], &mPos[1]);
    PSMTXMultVecSR(matrix, &mPos[2], &mPos[2]);
}
