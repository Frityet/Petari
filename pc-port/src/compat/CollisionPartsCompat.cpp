#include "compat/CollisionPartsCompat.hpp"

#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/ActorMovementUtil.hpp"
#include "Game/Util/LiveActorUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "Game/Util/SceneUtil.hpp"
#include "compat/ResourceHolderCompat.hpp"
#include "resource/RarcArchive.hpp"
#include "scene/StageCollisionService.hpp"

#include <array>
#include <cmath>
#include <memory>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace {

    struct ActorCollisionPartsState {
        ResourceHolder *resource_holder = nullptr;
        HitSensor *sensor = nullptr;
        std::span<const std::uint8_t> kcl{};
        std::span<const std::uint8_t> attributes{};
        std::array<float, 12U> matrix{};
        float bounding_radius = 0.0F;
        std::string resource_name;
        std::string source;
        std::string attributes_source;
        std::shared_ptr<smgpc::scene::StageCollisionRegistrationState> registration;
    };

    auto &actor_collision_parts() {
        static auto states =
            std::unordered_map<const LiveActor *, std::vector<ActorCollisionPartsState>>{};
        return states;
    }

    [[nodiscard]] std::array<float, 12U> copy_matrix(MtxPtr matrix) {
        if (matrix == nullptr) {
            throw std::invalid_argument("CollisionParts requires a real placement matrix.");
        }
        return {
            matrix[0][0],
            matrix[0][1],
            matrix[0][2],
            matrix[0][3],
            matrix[1][0],
            matrix[1][1],
            matrix[1][2],
            matrix[1][3],
            matrix[2][0],
            matrix[2][1],
            matrix[2][2],
            matrix[2][3],
        };
    }

    [[nodiscard]] float average_matrix_scale(const std::array<float, 12U> &matrix) {
        const auto column_length = [&](std::size_t column) {
            return std::sqrt(matrix[column] * matrix[column] +
                             matrix[4U + column] * matrix[4U + column] +
                             matrix[8U + column] * matrix[8U + column]);
        };
        return (column_length(0U) + column_length(1U) + column_length(2U)) / 3.0F;
    }

    [[nodiscard]] ActorCollisionPartsState &require_actor_collision_parts(const LiveActor *actor) {
        const auto found = actor_collision_parts().find(actor);
        if (found == actor_collision_parts().end()) {
            throw std::logic_error("LiveActor has no registered CollisionParts.");
        }
        if (found->second.empty()) {
            throw std::logic_error("LiveActor has an empty CollisionParts registration list.");
        }
        return found->second.front();
    }

}  // namespace

namespace smgpc::compat {

    bool has_actor_collision_parts(const LiveActor *actor) noexcept {
        const auto found = actor_collision_parts().find(actor);
        return actor != nullptr && found != actor_collision_parts().end() &&
               !found->second.empty();
    }

    std::size_t actor_collision_parts_count(const LiveActor *actor) noexcept {
        const auto found = actor_collision_parts().find(actor);
        return actor != nullptr && found != actor_collision_parts().end() ?
                   found->second.size() :
                   0U;
    }

    std::string_view actor_collision_parts_source(const LiveActor *actor) noexcept {
        const auto found = actor_collision_parts().find(actor);
        return found != actor_collision_parts().end() && !found->second.empty() ?
                   std::string_view(found->second.front().source) :
                   std::string_view{};
    }

    std::vector<ActorCollisionPartsResource>
    actor_collision_parts_resources(const LiveActor *actor) {
        auto resources = std::vector<ActorCollisionPartsResource>{};
        const auto found = actor_collision_parts().find(actor);
        if (actor == nullptr || found == actor_collision_parts().end()) {
            return resources;
        }

        resources.reserve(found->second.size());
        for (const auto &state : found->second) {
            resources.push_back(ActorCollisionPartsResource{
                .resource_name = state.resource_name,
                .kcl_source = state.source,
                .attributes_source = state.attributes_source,
                .kcl_size = state.kcl.size(),
                .attributes_size = state.attributes.size(),
                .bounding_radius = state.bounding_radius,
            });
        }
        return resources;
    }

    void release_actor_collision_parts(const LiveActor *actor) noexcept {
        const auto found = actor_collision_parts().find(actor);
        if (found == actor_collision_parts().end()) {
            return;
        }
        for (const auto &state : found->second) {
            if (state.registration != nullptr) {
                state.registration->release_owner();
            }
        }
        actor_collision_parts().erase(found);
    }

}  // namespace smgpc::compat

namespace MR {

    ResourceHolder *createAndAddResourceHolder(const char *archive_name) {
        if (archive_name == nullptr) {
            throw std::invalid_argument("ResourceHolder requires an exact archive name.");
        }
        auto *service = smgpc::compat::ResourceHolderService::active();
        if (service == nullptr) {
            throw std::logic_error("ResourceHolder requires an active runtime owner.");
        }
        return service->create_and_add(archive_name);
    }

    void initCollisionPartsFromResourceHolder(LiveActor *actor, const char *resource_name,
                                              HitSensor *sensor, ResourceHolder *resource_holder,
                                              MtxPtr matrix) {
        if (actor == nullptr || resource_name == nullptr || sensor == nullptr ||
            resource_holder == nullptr) {
            throw std::invalid_argument(
                "CollisionParts requires an actor, resource name, sensor, and ResourceHolder.");
        }
        auto *collision = smgpc::scene::StageCollisionService::active();
        if (collision == nullptr) {
            throw std::logic_error("CollisionParts requires an active stage collision owner.");
        }

        auto actor_matrix = TPos3f{};
        if (matrix == nullptr) {
            MR::makeMtxTRS(actor_matrix.toMtxPtr(), actor);
            matrix = actor_matrix.toMtxPtr();
        }
        const auto host_matrix = copy_matrix(matrix);

        const auto kcl_name = std::string(resource_name) + ".kcl";
        const auto attributes_name = std::string(resource_name) + ".pa";
        const auto *kcl_entry = resource_holder->archive().find_resource(kcl_name);
        if (kcl_entry == nullptr) {
            throw std::runtime_error("Required CollisionParts KCL is unavailable: " + kcl_name);
        }
        const auto *attributes_entry = resource_holder->archive().find_resource(attributes_name);
        const auto kcl = resource_holder->archive().file_data(*kcl_entry);
        const auto attributes = attributes_entry != nullptr ?
                                    resource_holder->archive().file_data(*attributes_entry) :
                                    std::span<const std::uint8_t>{};
        const auto source = resource_holder->resolved_path().generic_string() + ":/" +
                            kcl_entry->path;
        const auto attributes_source =
            attributes_entry != nullptr ?
                resource_holder->resolved_path().generic_string() + ":/" +
                    attributes_entry->path :
                std::string{};

        if (const auto found = actor_collision_parts().find(actor);
            found != actor_collision_parts().end()) {
            if (std::ranges::any_of(found->second, [&](const auto &state) {
                    return state.resource_holder == resource_holder && state.source == source;
                })) {
                return;
            }
        }

        // CollisionParts::init selects its CollisionZone from the placement
        // checker at creation. Retain that identity when the scope ends.
        const auto placement_zone_id = MR::getCurrentPlacementZoneId();
        if (placement_zone_id < 0) {
            throw std::logic_error("CollisionParts requires an active placement-zone ownership scope.");
        }
        auto registration = std::make_shared<smgpc::scene::StageCollisionRegistrationState>(
            &actor->mFlag.mIsDead);
        const auto result = collision->register_kcl(kcl, host_matrix, source, registration,
                                                    attributes, sensor, placement_zone_id);
        if (!result.accepted) {
            registration->release_owner();
            throw std::runtime_error("Required CollisionParts KCL is malformed: " + source);
        }

        actor_collision_parts()[actor].push_back(ActorCollisionPartsState{
            .resource_holder = resource_holder,
            .sensor = sensor,
            .kcl = kcl,
            .attributes = attributes,
            .matrix = host_matrix,
            .bounding_radius = result.local_bounding_radius * average_matrix_scale(host_matrix),
            .resource_name = resource_name,
            .source = source,
            .attributes_source = attributes_source,
            .registration = std::move(registration),
        });
    }

    f32 getCollisionBoundingSphereRange(const LiveActor *actor) {
        if (actor == nullptr) {
            throw std::invalid_argument("CollisionParts bounding range requires a LiveActor.");
        }
        return require_actor_collision_parts(actor).bounding_radius;
    }

}  // namespace MR
