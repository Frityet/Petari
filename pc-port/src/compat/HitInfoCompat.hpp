#pragma once

#include "Game/Map/HitInfo.hpp"
#include "scene/StageCollisionService.hpp"

class TriangleFilterBase;

namespace smgpc::compat {

    // Preserve the source prism's geometry separately from an individual
    // sphere or line query's contact position.
    [[nodiscard]] Triangle make_collision_triangle(
        const scene::StageCollisionService& collision, std::uint32_t triangle_index);
    [[nodiscard]] scene::StageCollisionTriangleFilter make_collision_triangle_filter(
        const scene::StageCollisionService& collision, const TriangleFilterBase* filter);

}  // namespace smgpc::compat
