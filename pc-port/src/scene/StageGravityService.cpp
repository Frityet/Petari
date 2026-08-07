#include "scene/StageGravityService.hpp"

#include "Game/Gravity/GravityInfo.hpp"
#include "Game/Gravity/PlanetGravity.hpp"
#include "Game/Util/GravityUtil.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Util/MathUtil.hpp"
#include "scene/StageCollisionService.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace smgpc::scene {
    namespace {
        StageGravityService* sActiveService = nullptr;

        enum class PlacementKind : std::uint8_t {
            Point,
            ParallelSphere,
            ParallelBox,
            ParallelCylinder,
        };

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
            if (!normalize(result)) {
                throw std::runtime_error("gravity placement rotation produced a degenerate basis");
            }
            return result;
        }

        [[nodiscard]] bool is_gravity_name(std::string_view name) {
            return name.starts_with("Global") && name.find("Gravity") != std::string_view::npos;
        }

        [[nodiscard]] bool is_supported_gravity_name(std::string_view name) {
            return name == "GlobalPointGravity" || name == "GlobalPlaneGravity" ||
                   name == "GlobalPlaneGravityInBox" || name == "GlobalPlaneGravityInCylinder";
        }

        [[nodiscard]] bool requires_dynamic_gravity_actor(const StagePlacementObject& placement) {
            return placement.switch_a_id >= 0 || placement.switch_b_id >= 0 ||
                   placement.switch_sleep_id >= 0 || placement.follow_id >= 0;
        }

        [[nodiscard]] PlacementKind placement_kind(std::string_view name) {
            if (name == "GlobalPointGravity") {
                return PlacementKind::Point;
            }
            if (name == "GlobalPlaneGravity") {
                return PlacementKind::ParallelSphere;
            }
            if (name == "GlobalPlaneGravityInBox") {
                return PlacementKind::ParallelBox;
            }
            if (name == "GlobalPlaneGravityInCylinder") {
                return PlacementKind::ParallelCylinder;
            }
            throw std::invalid_argument("unsupported gravity placement: " + std::string(name));
        }

        class PlacementGravity final : public PlanetGravity {
        public:
            explicit PlacementGravity(const StagePlacementObject& placement)
                : _kind(placement_kind(placement.object_name)),
                  _position(placement.translation[0], placement.translation[1], placement.translation[2]) {
                auto rotation_placement = placement;
                rotation_placement.scale = {1.0F, 1.0F, 1.0F};
                const auto rotation_matrix = stage_collision_matrix(rotation_placement);
                _side = matrix_axis(rotation_matrix, 0U);
                _up = matrix_axis(rotation_matrix, 1U);
                _front = matrix_axis(rotation_matrix, 2U);

                switch (_kind) {
                case PlacementKind::Point:
                    mDistant = placement.scale[0] * 500.0F;
                    break;
                case PlacementKind::ParallelSphere:
                    break;
                case PlacementKind::ParallelBox:
                    _box_side = _side * (placement.scale[0] * 500.0F);
                    _box_up = _up * (placement.scale[1] * 500.0F);
                    _box_front = _front * (placement.scale[2] * 500.0F);
                    _box_extent_squared.set(length_squared(_box_side), length_squared(_box_up),
                                            length_squared(_box_front));
                    _position.add(_up * (placement.scale[1] * 500.0F));
                    set_parallel_args(placement);
                    break;
                case PlacementKind::ParallelCylinder:
                    _cylinder_radius = placement.scale[0] * 500.0F;
                    _cylinder_height = placement.scale[1] * 500.0F;
                    set_parallel_args(placement);
                    break;
                }

                const auto iter = JMapInfoIter(&placement.jmap_info, placement.jmap_entry_index);
                MR::settingGravityParamFromJMap(this, iter);
            }

            bool calcOwnGravityVector(TVec3f* destination, f32* scalar,
                                      const TVec3f& position) const override {
                auto direction = TVec3f{};
                auto distance = 0.0F;

                switch (_kind) {
                case PlacementKind::Point:
                    direction = _position - position;
                    distance = std::sqrt(length_squared(direction));
                    if (!isInRangeDistance(distance)) {
                        return false;
                    }
                    if (distance < 0.001F) {
                        direction.zero();
                    } else {
                        direction.scale(1.0F / distance);
                    }
                    break;
                case PlacementKind::ParallelSphere:
                    if (mRange >= 0.0F && length_squared(_position - position) >= mRange * mRange) {
                        return false;
                    }
                    direction = -_up;
                    distance = _base_distance;
                    break;
                case PlacementKind::ParallelBox: {
                    const auto from_center = position - _position;
                    const auto dot_x = dot(from_center, _box_side);
                    const auto dot_y = dot(from_center, _box_up);
                    const auto dot_z = dot(from_center, _box_front);
                    if (dot_x < -_box_extent_squared.x || dot_x > _box_extent_squared.x ||
                        dot_y < -_box_extent_squared.y || dot_y > _box_extent_squared.y ||
                        dot_z < -_box_extent_squared.z || dot_z > _box_extent_squared.z) {
                        return false;
                    }
                    direction = -_up;
                    distance = _base_distance;
                    if (_distance_axis == 0) {
                        distance += std::abs(dot_x) / std::sqrt(_box_extent_squared.x);
                    } else if (_distance_axis == 1) {
                        distance += std::abs(dot_y) / std::sqrt(_box_extent_squared.y);
                    } else if (_distance_axis == 2) {
                        distance += std::abs(dot_z) / std::sqrt(_box_extent_squared.z);
                    }
                    break;
                }
                case PlacementKind::ParallelCylinder: {
                    const auto from_base = position - _position;
                    const auto height = dot(from_base, _up);
                    if (height < 0.0F || height > _cylinder_height) {
                        return false;
                    }
                    const auto radial = from_base - _up * height;
                    const auto radius = std::sqrt(length_squared(radial));
                    if (radius > _cylinder_radius) {
                        return false;
                    }
                    direction = -_up;
                    distance = _base_distance + radius;
                    break;
                }
                }

                if (destination != nullptr) {
                    destination->set(direction);
                }
                if (scalar != nullptr) {
                    *scalar = distance;
                }
                return true;
            }

        private:
            void set_parallel_args(const StagePlacementObject& placement) {
                if (placement.object_args[0] >= 0) {
                    _base_distance = static_cast<float>(placement.object_args[0]);
                }
                if (placement.object_args[1] >= 0 && placement.object_args[1] <= 2) {
                    _distance_axis = placement.object_args[1];
                }
            }

            PlacementKind _kind;
            TVec3f _position{};
            TVec3f _side{1.0F, 0.0F, 0.0F};
            TVec3f _up{0.0F, 1.0F, 0.0F};
            TVec3f _front{0.0F, 0.0F, 1.0F};
            TVec3f _box_side{};
            TVec3f _box_up{};
            TVec3f _box_front{};
            TVec3f _box_extent_squared{};
            float _base_distance = 2000.0F;
            float _cylinder_radius = 500.0F;
            float _cylinder_height = 1000.0F;
            std::int32_t _distance_axis = -1;
        };

    }  // namespace

    StageGravityService::StageGravityService() = default;

    StageGravityService::~StageGravityService() {
        deactivate();
    }

    StageGravityLoadStats StageGravityService::load(std::span<const StagePlacementObject> placements) {
        clear();
        _stats.placement_count = placements.size();

        auto first_unsupported = std::string{};
        for (const auto& placement : placements) {
            if (!is_gravity_name(placement.object_name)) {
                continue;
            }
            if (is_supported_gravity_name(placement.object_name) &&
                !requires_dynamic_gravity_actor(placement)) {
                ++_stats.gravity_count;
            } else {
                ++_stats.unsupported_count;
                if (first_unsupported.empty()) {
                    first_unsupported = placement.object_name;
                    if (requires_dynamic_gravity_actor(placement)) {
                        first_unsupported += " (switch/follower lifecycle)";
                    }
                }
            }
        }
        if (!first_unsupported.empty()) {
            throw std::runtime_error("unsupported PlanetGravity placement requires its real implementation: " +
                                     first_unsupported);
        }

        _placement_gravities.reserve(_stats.gravity_count);
        for (const auto& placement : placements) {
            if (!is_supported_gravity_name(placement.object_name)) {
                continue;
            }
            auto gravity = std::make_unique<PlacementGravity>(placement);
            register_gravity(gravity.get());
            _placement_gravities.push_back(std::move(gravity));
        }
        return _stats;
    }

    void StageGravityService::clear() {
        for (auto* gravity : _gravities) {
            gravity->mIsRegistered = false;
        }
        _gravities.clear();
        _placement_gravities.clear();
        _stats = {};
    }

    void StageGravityService::register_gravity(PlanetGravity* gravity) {
        if (gravity == nullptr) {
            throw std::invalid_argument("cannot register a null PlanetGravity");
        }
        if (gravity->mIsRegistered || std::ranges::find(_gravities, gravity) != _gravities.end()) {
            throw std::logic_error("PlanetGravity is already registered");
        }
        if (_gravities.size() >= 128U) {
            throw std::length_error("PlanetGravityManager capacity of 128 was exceeded");
        }

        gravity->mIsRegistered = true;
        _gravities.push_back(gravity);
        std::ranges::stable_sort(_gravities, [](const auto* lhs, const auto* rhs) {
            return lhs->mPriority > rhs->mPriority;
        });
    }

    bool StageGravityService::query(const TVec3f& position, TVec3f* gravity,
                                    std::uint32_t gravity_type_mask, GravityInfo* info,
                                    std::uint32_t host) const {
        auto total = TVec3f{};
        auto has_calculated = false;
        auto largest_scalar = -1.0F;
        auto largest_priority = std::int32_t{-1};

        if (info != nullptr) {
            info->init();
        }

        for (auto* instance : _gravities) {
            const auto valid = instance->mActivated && instance->mValidFollower && instance->mAppeared;
            const auto instance_host = static_cast<std::uint32_t>(
                reinterpret_cast<std::uintptr_t>(instance->mHost));
            if (!valid || (gravity_type_mask & instance->mGravityType) == 0U || host == instance_host) {
                continue;
            }
            if (instance->mPriority < largest_priority) {
                break;
            }

            auto vector = TVec3f{};
            if (!instance->calcGravity(&vector, position)) {
                continue;
            }

            auto store_info = false;
            const auto scalar = vector.length();
            if (instance->mPriority == largest_priority) {
                total.add(vector);
                has_calculated = true;
                if (largest_scalar < scalar) {
                    store_info = true;
                }
            } else {
                largest_priority = instance->mPriority;
                total = vector;
                has_calculated = true;
                store_info = true;
            }

            if (info != nullptr && store_info) {
                largest_scalar = scalar;
                info->mGravityVector = vector;
                info->mLargestPriority = largest_priority;
                info->mGravityInstance = instance;
            }
        }

        if (gravity != nullptr) {
            MR::normalizeOrZero(&total);
            gravity->set(total);
        }
        return has_calculated;
    }

    const StageGravityLoadStats& StageGravityService::stats() const {
        return _stats;
    }

    bool StageGravityService::empty() const {
        return _gravities.empty();
    }

    void StageGravityService::activate() {
        if (sActiveService != nullptr && sActiveService != this) {
            throw std::logic_error("another scene already owns the active PlanetGravity service");
        }
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
