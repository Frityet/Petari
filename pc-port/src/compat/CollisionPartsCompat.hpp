#pragma once

#include <string_view>

#include <revolution/mtx.h>
#include <revolution/types.h>

class LiveActor;
class HitSensor;
class ResourceHolder;

namespace MR {

    ResourceHolder *createAndAddResourceHolder(const char *archive_name);
    void initCollisionPartsFromResourceHolder(LiveActor *actor, const char *resource_name,
                                              HitSensor *sensor, ResourceHolder *resource_holder,
                                              MtxPtr matrix);
    f32 getCollisionBoundingSphereRange(const LiveActor *actor);

}  // namespace MR

namespace smgpc::compat {

    [[nodiscard]] bool has_actor_collision_parts(const LiveActor *actor) noexcept;
    [[nodiscard]] std::string_view actor_collision_parts_source(const LiveActor *actor) noexcept;
    void release_actor_collision_parts(const LiveActor *actor) noexcept;

}  // namespace smgpc::compat
