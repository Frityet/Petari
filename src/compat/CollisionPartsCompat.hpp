#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <revolution/mtx.h>
#include <revolution/types.h>
#include <JSystem/JGeometry/TMatrix.hpp>

class LiveActor;
class HitSensor;
class ResourceHolder;
class CollisionParts;
class SceneObjHolder;

namespace MR {

    ResourceHolder *createAndAddResourceHolder(const char *archive_name);
    void initCollisionPartsFromResourceHolder(LiveActor *actor, const char *resource_name,
                                              HitSensor *sensor, ResourceHolder *resource_holder,
                                              MtxPtr matrix);
    f32 getCollisionBoundingSphereRange(const LiveActor *actor);

}  // namespace MR

namespace smgpc::scene { class StageCollisionService; }

namespace smgpc::compat {

    CollisionParts* create_collision_parts(ResourceHolder* resources, const char* name,
                                           HitSensor* sensor, const TPos3f& matrix,
                                           int scale_type, s32 category);
    scene::StageCollisionService* collision_service_for_parts(const CollisionParts* parts) noexcept;
    void publish_collision_parts(CollisionParts& parts);
    void publish_collision_parts_membership(CollisionParts& parts, bool enabled);

    struct ActorCollisionPartsResource {
        std::string resource_name;
        std::string kcl_source;
        std::string attributes_source;
        std::size_t kcl_size = 0U;
        std::size_t attributes_size = 0U;
        float bounding_radius = 0.0F;
    };

    [[nodiscard]] bool has_actor_collision_parts(const LiveActor *actor) noexcept;
    [[nodiscard]] std::size_t actor_collision_parts_count(const LiveActor *actor) noexcept;
    [[nodiscard]] std::string_view actor_collision_parts_source(const LiveActor *actor) noexcept;
    [[nodiscard]] std::vector<ActorCollisionPartsResource>
    actor_collision_parts_resources(const LiveActor *actor);
    void release_scene_collision_parts(const SceneObjHolder* holder) noexcept;
    void release_actor_collision_parts(const LiveActor *actor) noexcept;

}  // namespace smgpc::compat
