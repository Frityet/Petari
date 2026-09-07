#include "compat/HitInfoCompat.hpp"

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
        auto* collision = smgpc::scene::StageCollisionService::active();
        return collision != nullptr ? collision->surface(triangle.mIdx) : std::nullopt;
    }
}  // namespace

namespace smgpc::compat {

    Triangle make_collision_triangle(const scene::StageCollisionService& collision,
                                     std::uint32_t triangle_index) {
        const auto surface = collision.surface(triangle_index);
        if (!surface.has_value()) {
            throw std::logic_error("A collision hit requires its live source KCL prism.");
        }
        auto triangle = Triangle{};
        triangle.mIdx = triangle_index;
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
            return !filter->isInvalidTriangle(&triangle);
        };
    }

}  // namespace smgpc::compat

Triangle::Triangle()
    : mParts(nullptr), mIdx(0xFFFFFFFFU), mSensor(nullptr), mNormals{}, mPos{} {
}

Triangle& Triangle::operator=(const Triangle& other) = default;

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
        throw std::logic_error("Triangle placement-zone lookup requires a live collision owner with placement provenance.");
    }
    return *surface->placement_zone_id;
}

bool Triangle::isHostMoved() const {
    // Registered native KCL is static. Real CollisionParts retain the game's
    // movement counter instead of losing their moving-host behavior here.
    return mParts != nullptr && mParts->_D4 == 0;
}

void Triangle::calcForceMovePower(TVec3f* output, const TVec3f& position) const {
    if (mParts != nullptr) {
        mParts->calcForceMovePower(output, position);
        return;
    }
    if (!triangle_surface(*this).has_value()) {
        throw std::logic_error("Motion queries require a live collision triangle.");
    }
    output->zero();
}

bool Triangle::isValid() const {
    if (mIdx == 0xFFFFFFFFU) {
        return false;
    }
    return mParts != nullptr || triangle_surface(*this).has_value();
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
    return getNormal(index);
}

const TVec3f* Triangle::calcAndGetEdgeNormal(int index) {
    return getEdgeNormal(index);
}

const TVec3f* Triangle::calcAndGetPos(int index) {
    return getPos(index);
}

JMapInfoIter Triangle::getAttributes() const {
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
