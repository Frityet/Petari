#include "compat/HitInfoCompat.hpp"

#include "Game/Util/TriangleFilter.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>

namespace {
    struct AttributeCache {
        const smgpc::scene::StageCollisionService* service = nullptr;
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
    return surface.has_value() && !surface->source_name.empty() ? surface->source_name.data() : nullptr;
}

s32 Triangle::getHostPlacementZoneID() const {
    return -1;
}

bool Triangle::isHostMoved() const {
    return false;
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
    if (cache.service != collision || cache.revision != collision->revision()) {
        cache.service = collision;
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

HitInfo& HitInfo::operator=(const HitInfo& other) = default;

bool HitInfo::isCollisionAtFace() const {
    return _88 == 1U;
}

bool HitInfo::isCollisionAtEdge() const {
    return _88 == 2U || _88 == 3U || _88 == 4U;
}

bool HitInfo::isCollisionAtCorner() const {
    return _88 == 5U || _88 == 6U || _88 == 7U;
}
