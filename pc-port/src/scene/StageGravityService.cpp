#include "scene/StageGravityService.hpp"

#include "Game/Util/JMapInfo.hpp"
#include "scene/StageCollisionService.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <string_view>

namespace smgpc::scene {
    namespace {
        StageGravityService* sActiveService = nullptr;

        [[nodiscard]] float dot(const TVec3f& lhs, const TVec3f& rhs) {
            return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
        }

        [[nodiscard]] float length_squared(const TVec3f& value) {
            return dot(value, value);
        }

        [[nodiscard]] bool normalize(TVec3f& value) {
            const auto square_length = length_squared(value);
            if (!(square_length > 1.0e-12F) || !std::isfinite(square_length)) {
                value.zero();
                return false;
            }
            value.scale(1.0F / std::sqrt(square_length));
            return true;
        }

        [[nodiscard]] TVec3f matrix_axis(const std::array<float, 12U>& matrix, std::size_t column) {
            auto result = TVec3f(matrix[column], matrix[4U + column], matrix[8U + column]);
            (void)normalize(result);
            return result;
        }

        [[nodiscard]] std::uint32_t gravity_type(const JMapInfoIter& iter) {
            const char* type = nullptr;
            if (!iter.getValue("Gravity_type", &type) || type == nullptr || std::strcmp(type, "Normal") == 0) {
                return 1U;
            }
            if (std::strcmp(type, "Shadow") == 0) {
                return 2U;
            }
            if (std::strcmp(type, "Magnet") == 0) {
                return 4U;
            }
            return 1U;
        }

        [[nodiscard]] bool is_gravity_name(std::string_view name) {
            return name.starts_with("Global") && name.ends_with("Gravity");
        }

    }  // namespace

    StageGravityService::~StageGravityService() {
        deactivate();
    }

    StageGravityLoadStats StageGravityService::load(std::span<const StagePlacementObject> placements) {
        clear();
        _stats.placement_count = placements.size();
        for (const auto& placement : placements) {
            auto kind = Kind::Point;
            if (placement.object_name == "GlobalPointGravity") {
                kind = Kind::Point;
            } else if (placement.object_name == "GlobalPlaneGravity") {
                kind = Kind::ParallelSphere;
            } else if (placement.object_name == "GlobalPlaneGravityInBox") {
                kind = Kind::ParallelBox;
            } else if (placement.object_name == "GlobalPlaneGravityInCylinder") {
                kind = Kind::ParallelCylinder;
            } else {
                if (is_gravity_name(placement.object_name)) {
                    ++_stats.unsupported_count;
                }
                continue;
            }

            const auto iter = JMapInfoIter(&placement.jmap_info, placement.jmap_entry_index);
            const auto matrix = stage_collision_matrix(placement);
            auto field = Field{
                .kind = kind,
                .position = TVec3f(placement.translation[0], placement.translation[1], placement.translation[2]),
                .side = matrix_axis(matrix, 0U),
                .up = matrix_axis(matrix, 1U),
                .front = matrix_axis(matrix, 2U),
                .extent = TVec3f(std::abs(placement.scale[0]) * 500.0F,
                                 std::abs(placement.scale[1]) * 500.0F,
                                 std::abs(placement.scale[2]) * 500.0F),
                .distant = kind == Kind::Point ? std::abs(placement.scale[0]) * 500.0F : 0.0F,
                .base_distance = placement.object_args[0] >= 0 ? static_cast<float>(placement.object_args[0]) : 2000.0F,
                .cylinder_radius = std::abs(placement.scale[0]) * 500.0F,
                .cylinder_height = std::abs(placement.scale[1]) * 500.0F,
                .distance_axis = placement.object_args[1] >= 0 && placement.object_args[1] <= 2
                                     ? placement.object_args[1]
                                     : -1,
                .gravity_type = gravity_type(iter),
            };
            (void)iter.getValue("Range", &field.range);
            (void)iter.getValue("Distant", &field.distant);
            (void)iter.getValue("Priority", &field.priority);
            auto inverse = std::int32_t{};
            if (iter.getValue("Inverse", &inverse)) {
                field.inverse = inverse != 0;
            }

            if (field.kind == Kind::ParallelBox) {
                field.position.add(field.up * field.extent.y);
            }
            _fields.push_back(field);
        }
        std::ranges::sort(_fields, [](const auto& lhs, const auto& rhs) {
            return lhs.priority > rhs.priority;
        });
        _stats.gravity_count = _fields.size();
        return _stats;
    }

    void StageGravityService::clear() {
        _fields.clear();
        _stats = {};
    }

    bool StageGravityService::query(const TVec3f& position, TVec3f* gravity,
                                    std::uint32_t gravity_type_mask) const {
        auto total = TVec3f{};
        auto selected_priority = std::numeric_limits<std::int32_t>::min();
        auto found = false;
        for (const auto& field : _fields) {
            if ((field.gravity_type & gravity_type_mask) == 0U) {
                continue;
            }
            if (found && field.priority < selected_priority) {
                break;
            }
            auto vector = TVec3f{};
            if (!calculate(field, position, vector)) {
                continue;
            }
            if (!found || field.priority > selected_priority) {
                total = vector;
                selected_priority = field.priority;
                found = true;
            } else {
                total.add(vector);
            }
        }
        if (gravity != nullptr) {
            if (found && normalize(total)) {
                gravity->set(total);
            } else {
                gravity->zero();
            }
        }
        return found;
    }

    bool StageGravityService::calculate(const Field& field, const TVec3f& position, TVec3f& vector) const {
        auto direction = TVec3f{};
        auto distance = 0.0F;
        switch (field.kind) {
        case Kind::Point: {
            direction = field.position - position;
            distance = std::sqrt(length_squared(direction));
            if (field.range >= 0.0F && distance >= field.range + field.distant) {
                return false;
            }
            if (!normalize(direction)) {
                return false;
            }
            break;
        }
        case Kind::ParallelSphere: {
            if (field.range >= 0.0F && length_squared(field.position - position) >= field.range * field.range) {
                return false;
            }
            direction = -field.up;
            distance = field.base_distance;
            break;
        }
        case Kind::ParallelBox: {
            const auto from_center = position - field.position;
            const auto coordinates = TVec3f(dot(from_center, field.side), dot(from_center, field.up),
                                            dot(from_center, field.front));
            if (std::abs(coordinates.x) > field.extent.x || std::abs(coordinates.y) > field.extent.y ||
                std::abs(coordinates.z) > field.extent.z) {
                return false;
            }
            direction = -field.up;
            distance = field.base_distance;
            if (field.distance_axis == 0) {
                distance += std::abs(coordinates.x);
            } else if (field.distance_axis == 1) {
                distance += std::abs(coordinates.y);
            } else if (field.distance_axis == 2) {
                distance += std::abs(coordinates.z);
            }
            break;
        }
        case Kind::ParallelCylinder: {
            const auto from_base = position - field.position;
            const auto height = dot(from_base, field.up);
            if (height < 0.0F || height > field.cylinder_height) {
                return false;
            }
            const auto radial = from_base - field.up * height;
            const auto radius = std::sqrt(length_squared(radial));
            if (radius > field.cylinder_radius) {
                return false;
            }
            direction = -field.up;
            distance = field.base_distance + radius;
            break;
        }
        }

        distance = std::max(1.0F, distance - field.distant);
        direction.scale(4000000.0F / (distance * distance));
        if (field.inverse) {
            direction.negate();
        }
        vector.set(direction);
        return true;
    }

    const StageGravityLoadStats& StageGravityService::stats() const {
        return _stats;
    }

    bool StageGravityService::empty() const {
        return _fields.empty();
    }

    void StageGravityService::activate() {
        sActiveService = this;
    }

    void StageGravityService::deactivate() {
        if (sActiveService == this) {
            sActiveService = nullptr;
        }
    }

    StageGravityService* StageGravityService::active() {
        return sActiveService;
    }

}  // namespace smgpc::scene
