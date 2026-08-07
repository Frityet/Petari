#include "compat/GameActorSensorCompat.hpp"

#include "Game/LiveActor/HitSensor.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/MessageSensorHolder.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/Util/JointUtil.hpp"
#include "runtime/RuntimeContext.hpp"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

    enum class SensorBindingKind {
        Position,
        Matrix,
        Joint,
    };

    struct SensorBinding {
        HitSensor* sensor = nullptr;
        SensorBindingKind kind = SensorBindingKind::Position;
        const TVec3f* position = nullptr;
        MtxPtr matrix = nullptr;
        std::string joint_name{};
        TVec3f offset{};
    };

    using ActorSensorBindings = std::unordered_map<const LiveActor*, std::vector<SensorBinding>>;

    [[nodiscard]] ActorSensorBindings& actor_sensor_bindings() {
        static auto bindings = ActorSensorBindings{};
        return bindings;
    }

    [[nodiscard]] bool can_register_sensor(const LiveActor* actor, const char* name) {
        return actor != nullptr && name != nullptr && *name != '\0';
    }

    [[nodiscard]] TVec3f transform_point(MtxPtr matrix, const TVec3f& point) {
        return TVec3f{
            matrix[0][0] * point.x + matrix[0][1] * point.y + matrix[0][2] * point.z + matrix[0][3],
            matrix[1][0] * point.x + matrix[1][1] * point.y + matrix[1][2] * point.z + matrix[1][3],
            matrix[2][0] * point.x + matrix[2][1] * point.y + matrix[2][2] * point.z + matrix[2][3],
        };
    }

    [[nodiscard]] MtxPtr resolve_binding_matrix(LiveActor* actor, const SensorBinding& binding) {
        if (binding.kind == SensorBindingKind::Joint) {
            return MR::getJointMtx(actor, binding.joint_name.c_str());
        }
        return binding.matrix;
    }

    void update_binding(LiveActor* actor, SensorBinding& binding) {
        if (binding.sensor == nullptr) {
            return;
        }

        switch (binding.kind) {
        case SensorBindingKind::Position:
            if (binding.position == nullptr) {
                binding.sensor->invalidateBySystem();
                return;
            }
            binding.sensor->mPosition = *binding.position + binding.offset;
            break;
        case SensorBindingKind::Matrix:
        case SensorBindingKind::Joint:
            if (auto* matrix = resolve_binding_matrix(actor, binding); matrix != nullptr) {
                binding.sensor->mPosition = transform_point(matrix, binding.offset);
            } else {
                binding.sensor->invalidateBySystem();
                return;
            }
            break;
        }

        binding.sensor->validateBySystem();
    }

    void register_binding(LiveActor* actor, SensorBinding binding) {
        auto& stored = actor_sensor_bindings()[actor].emplace_back(std::move(binding));
        update_binding(actor, stored);
    }

    [[nodiscard]] HitSensor* add_position_sensor(LiveActor* actor, const char* name, u32 type, u16 group_size, f32 radius,
                                                 const TVec3f* position, const TVec3f& offset) {
        if (!can_register_sensor(actor, name) || position == nullptr) {
            return nullptr;
        }

        auto* sensor = actor->addHitSensor(name, type, group_size, radius, {});
        if (sensor == nullptr) {
            return nullptr;
        }
        register_binding(actor, SensorBinding{
                                    .sensor = sensor,
                                    .kind = SensorBindingKind::Position,
                                    .position = position,
                                    .offset = offset,
                                });
        return sensor;
    }

    [[nodiscard]] HitSensor* add_joint_sensor(LiveActor* actor, const char* name, const char* joint_name, u32 type,
                                              u16 group_size, f32 radius, const TVec3f& offset) {
        if (!can_register_sensor(actor, name) || joint_name == nullptr || *joint_name == '\0' ||
            MR::getJointMtx(actor, joint_name) == nullptr) {
            return nullptr;
        }

        auto* sensor = actor->addHitSensor(name, type, group_size, radius, {});
        if (sensor == nullptr) {
            return nullptr;
        }
        register_binding(actor, SensorBinding{
                                    .sensor = sensor,
                                    .kind = SensorBindingKind::Joint,
                                    .joint_name = joint_name,
                                    .offset = offset,
                                });
        return sensor;
    }

    [[nodiscard]] HitSensor* sensor_at(LiveActor* actor, int index) {
        if (actor == nullptr || index < 0) {
            return nullptr;
        }
        auto sensors = std::vector<HitSensor*>{};
        actor->collectHitSensors(sensors);
        const auto sensor_index = static_cast<std::size_t>(index);
        return sensor_index < sensors.size() ? sensors[sensor_index] : nullptr;
    }

    [[nodiscard]] bool sensor_type_in_open_range(const HitSensor* sensor, u32 start, u32 end) {
        return sensor != nullptr && sensor->mType > start && sensor->mType < end;
    }

    [[nodiscard]] bool send_message(u32 message, HitSensor* receiver, HitSensor* sender) {
        return receiver != nullptr && receiver->receiveMessage(message, sender);
    }

}  // namespace

namespace smgpc::compat {

    void update_actor_sensor_bindings(LiveActor* actor) {
        const auto found = actor_sensor_bindings().find(actor);
        if (found == actor_sensor_bindings().end()) {
            return;
        }
        for (auto& binding : found->second) {
            update_binding(actor, binding);
        }
    }

    void release_actor_sensor_bindings(const LiveActor* actor) {
        actor_sensor_bindings().erase(actor);
    }

    std::size_t actor_sensor_binding_count(const LiveActor* actor) {
        const auto found = actor_sensor_bindings().find(actor);
        return found != actor_sensor_bindings().end() ? found->second.size() : 0U;
    }

    const char* actor_message_name(std::uint32_t message) {
        switch (message) {
        case ACTMES_PLAYER_PUNCH:
            return "ACTMES_PLAYER_PUNCH";
        case ACTMES_PLAYER_TRAMPLE:
            return "ACTMES_PLAYER_TRAMPLE";
        case ACTMES_PUSH:
            return "ACTMES_PUSH";
        case ACTMES_PUSH_FORCE:
            return "ACTMES_PUSH_FORCE";
        case ACTMES_INHALE_BLACK_HOLE:
            return "ACTMES_INHALE_BLACK_HOLE";
        case ACTMES_ITEM_GET:
            return "ACTMES_ITEM_GET";
        case ACTMES_ITEM_PULL:
            return "ACTMES_ITEM_PULL";
        case ACTMES_ITEM_SHOW:
            return "ACTMES_ITEM_SHOW";
        case ACTMES_ITEM_HIDE:
            return "ACTMES_ITEM_HIDE";
        case ACTMES_ITEM_START_MOVE:
            return "ACTMES_ITEM_START_MOVE";
        case ACTMES_ITEM_END_MOVE:
            return "ACTMES_ITEM_END_MOVE";
        case ACTMES_RUSH_BEGIN:
            return "ACTMES_RUSH_BEGIN";
        case ACTMES_AUTORUSH_BEGIN:
            return "ACTMES_AUTORUSH_BEGIN";
        case ACTMES_RUSH_CANCEL:
            return "ACTMES_RUSH_CANCEL";
        case ACTMES_UPDATE_BASEMTX:
            return "ACTMES_UPDATE_BASEMTX";
        default:
            return nullptr;
        }
    }

}  // namespace smgpc::compat

namespace MR {

    HitSensor* addHitSensor(LiveActor* actor, const char* name, u32 type, u16 group_size, f32 radius, const TVec3f& offset) {
        return can_register_sensor(actor, name) ? actor->addHitSensor(name, type, group_size, radius, offset) : nullptr;
    }

    HitSensor* addHitSensorBinder(LiveActor* actor, const char* name, u16 group_size, f32 radius, const TVec3f& offset) {
        return addHitSensor(actor, name, ATYPE_BINDER, group_size, radius, offset);
    }

    HitSensor* addHitSensorTransferableBinder(LiveActor* actor, const char* name, u16 group_size, f32 radius,
                                              const TVec3f& offset) {
        return addHitSensor(actor, name, ATYPE_TRANSFERABLE_BINDER, group_size, radius, offset);
    }

    HitSensor* addHitSensorPriorBinder(LiveActor* actor, const char* name, u16 group_size, f32 radius,
                                      const TVec3f& offset) {
        return addHitSensor(actor, name, ATYPE_PRIOR_BINDER, group_size, radius, offset);
    }

    HitSensor* addHitSensorRide(LiveActor* actor, const char* name, u16 group_size, f32 radius, const TVec3f& offset) {
        return addHitSensor(actor, name, ATYPE_RIDE, group_size, radius, offset);
    }

    HitSensor* addHitSensorMapObj(LiveActor* actor, const char* name, u16 group_size, f32 radius, const TVec3f& offset) {
        return addHitSensor(actor, name, ATYPE_MAP_OBJ, group_size, radius, offset);
    }

    HitSensor* addHitSensorMapObjPress(LiveActor* actor, const char* name, u16 group_size, f32 radius,
                                      const TVec3f& offset) {
        return addHitSensor(actor, name, ATYPE_MAP_OBJ_PRESS, group_size, radius, offset);
    }

    HitSensor* addHitSensorMapObjSimple(LiveActor* actor, const char* name, u16 group_size, f32 radius,
                                       const TVec3f& offset) {
        return addHitSensor(actor, name, ATYPE_MAP_OBJ_SIMPLE, group_size, radius, offset);
    }

    HitSensor* addHitSensorMapObjMoveCollision(LiveActor* actor, const char* name, u16 group_size, f32 radius,
                                              const TVec3f& offset) {
        return addHitSensor(actor, name, ATYPE_MAP_OBJ_MOVE_COLLISION, group_size, radius, offset);
    }

    HitSensor* addHitSensorEnemy(LiveActor* actor, const char* name, u16 group_size, f32 radius, const TVec3f& offset) {
        return addHitSensor(actor, name, ATYPE_ENEMY, group_size, radius, offset);
    }

    HitSensor* addHitSensorEnemySimple(LiveActor* actor, const char* name, u16 group_size, f32 radius,
                                      const TVec3f& offset) {
        return addHitSensor(actor, name, ATYPE_ENEMY_SIMPLE, group_size, radius, offset);
    }

    HitSensor* addHitSensorEnemyAttack(LiveActor* actor, const char* name, u16 group_size, f32 radius,
                                      const TVec3f& offset) {
        return addHitSensor(actor, name, ATYPE_ENEMY_ATTACK, group_size, radius, offset);
    }

    HitSensor* addHitSensorNpc(LiveActor* actor, const char* name, u16 group_size, f32 radius, const TVec3f& offset) {
        return addHitSensor(actor, name, ATYPE_NPC, group_size, radius, offset);
    }

    HitSensor* addHitSensorEye(LiveActor* actor, const char* name, u16 group_size, f32 radius, const TVec3f& offset) {
        return addHitSensor(actor, name, ATYPE_EYE, group_size, radius, offset);
    }

    HitSensor* addHitSensorPush(LiveActor* actor, const char* name, u16 group_size, f32 radius, const TVec3f& offset) {
        return addHitSensor(actor, name, ATYPE_PUSH, group_size, radius, offset);
    }

    HitSensor* addHitSensorPosBinder(LiveActor* actor, const char* name, u16 group_size, f32 radius,
                                    const TVec3f* position, const TVec3f& offset) {
        return add_position_sensor(actor, name, ATYPE_BINDER, group_size, radius, position, offset);
    }

    HitSensor* addHitSensorPosRide(LiveActor* actor, const char* name, u16 group_size, f32 radius,
                                  const TVec3f* position, const TVec3f& offset) {
        return add_position_sensor(actor, name, ATYPE_RIDE, group_size, radius, position, offset);
    }

    HitSensor* addHitSensorPosMapObj(LiveActor* actor, const char* name, u16 group_size, f32 radius,
                                    const TVec3f* position, const TVec3f& offset) {
        return add_position_sensor(actor, name, ATYPE_MAP_OBJ, group_size, radius, position, offset);
    }

    HitSensor* addHitSensorPosEye(LiveActor* actor, const char* name, u16 group_size, f32 radius,
                                 const TVec3f* position, const TVec3f& offset) {
        return add_position_sensor(actor, name, ATYPE_EYE, group_size, radius, position, offset);
    }

    HitSensor* addHitSensorMtx(LiveActor* actor, const char* name, u32 type, u16 group_size, f32 radius,
                               MtxPtr matrix, const TVec3f& offset) {
        if (!can_register_sensor(actor, name) || matrix == nullptr) {
            return nullptr;
        }

        auto* sensor = actor->addHitSensor(name, type, group_size, radius, {});
        if (sensor == nullptr) {
            return nullptr;
        }
        register_binding(actor, SensorBinding{
                                    .sensor = sensor,
                                    .kind = SensorBindingKind::Matrix,
                                    .matrix = matrix,
                                    .offset = offset,
                                });
        return sensor;
    }

    HitSensor* addHitSensorMtxRide(LiveActor* actor, const char* name, u16 group_size, f32 radius, MtxPtr matrix,
                                   const TVec3f& offset) {
        return addHitSensorMtx(actor, name, ATYPE_RIDE, group_size, radius, matrix, offset);
    }

    HitSensor* addHitSensorMtxMapObj(LiveActor* actor, const char* name, u16 group_size, f32 radius, MtxPtr matrix,
                                     const TVec3f& offset) {
        return addHitSensorMtx(actor, name, ATYPE_MAP_OBJ, group_size, radius, matrix, offset);
    }

    HitSensor* addHitSensorMtxEnemy(LiveActor* actor, const char* name, u16 group_size, f32 radius, MtxPtr matrix,
                                    const TVec3f& offset) {
        return addHitSensorMtx(actor, name, ATYPE_ENEMY, group_size, radius, matrix, offset);
    }

    HitSensor* addHitSensorMtxEnemyAttack(LiveActor* actor, const char* name, u16 group_size, f32 radius, MtxPtr matrix,
                                          const TVec3f& offset) {
        return addHitSensorMtx(actor, name, ATYPE_ENEMY_ATTACK, group_size, radius, matrix, offset);
    }

    HitSensor* addHitSensorMtxNpc(LiveActor* actor, const char* name, u16 group_size, f32 radius, MtxPtr matrix,
                                  const TVec3f& offset) {
        return addHitSensorMtx(actor, name, ATYPE_NPC, group_size, radius, matrix, offset);
    }

    HitSensor* addHitSensorMtxAnimal(LiveActor* actor, const char* name, u16 group_size, f32 radius, MtxPtr matrix,
                                     const TVec3f& offset) {
        return addHitSensorMtx(actor, name, ATYPE_ANIMAL, group_size, radius, matrix, offset);
    }

    HitSensor* addHitSensorAtJoint(LiveActor* actor, const char* name, const char* joint_name, u32 type,
                                   u16 group_size, f32 radius, const TVec3f& offset) {
        return add_joint_sensor(actor, name, joint_name, type, group_size, radius, offset);
    }

    HitSensor* addHitSensorAtJointRide(LiveActor* actor, const char* name, const char* joint_name, u16 group_size,
                                       f32 radius, const TVec3f& offset) {
        return add_joint_sensor(actor, name, joint_name, ATYPE_RIDE, group_size, radius, offset);
    }

    HitSensor* addHitSensorAtJointMapObj(LiveActor* actor, const char* name, const char* joint_name, u16 group_size,
                                         f32 radius, const TVec3f& offset) {
        return add_joint_sensor(actor, name, joint_name, ATYPE_MAP_OBJ, group_size, radius, offset);
    }

    HitSensor* addHitSensorAtJointMapObjSimple(LiveActor* actor, const char* name, const char* joint_name,
                                               u16 group_size, f32 radius, const TVec3f& offset) {
        return add_joint_sensor(actor, name, joint_name, ATYPE_MAP_OBJ_SIMPLE, group_size, radius, offset);
    }

    HitSensor* addHitSensorAtJointEnemy(LiveActor* actor, const char* name, const char* joint_name, u16 group_size,
                                        f32 radius, const TVec3f& offset) {
        return add_joint_sensor(actor, name, joint_name, ATYPE_ENEMY, group_size, radius, offset);
    }

    HitSensor* addHitSensorAtJointEnemySimple(LiveActor* actor, const char* name, const char* joint_name,
                                              u16 group_size, f32 radius, const TVec3f& offset) {
        return add_joint_sensor(actor, name, joint_name, ATYPE_ENEMY_SIMPLE, group_size, radius, offset);
    }

    HitSensor* addHitSensorAtJointEnemyAttack(LiveActor* actor, const char* name, const char* joint_name,
                                              u16 group_size, f32 radius, const TVec3f& offset) {
        return add_joint_sensor(actor, name, joint_name, ATYPE_ENEMY_ATTACK, group_size, radius, offset);
    }

    HitSensor* addHitSensorAtJointNpc(LiveActor* actor, const char* name, const char* joint_name, u16 group_size,
                                      f32 radius, const TVec3f& offset) {
        return add_joint_sensor(actor, name, joint_name, ATYPE_NPC, group_size, radius, offset);
    }

    HitSensor* addHitSensorAtJointEye(LiveActor* actor, const char* name, const char* joint_name, u16 group_size,
                                      f32 radius, const TVec3f& offset) {
        return add_joint_sensor(actor, name, joint_name, ATYPE_EYE, group_size, radius, offset);
    }

    HitSensor* addBodyMessageSensor(LiveActor* actor, u32 type) {
        return addHitSensor(actor, "body", type, 0U, 0.0F, {});
    }

    HitSensor* addBodyMessageSensorReceiver(LiveActor* actor) {
        return addBodyMessageSensor(actor, ATYPE_RECEIVER);
    }

    HitSensor* addBodyMessageSensorMapObj(LiveActor* actor) {
        return addBodyMessageSensor(actor, ATYPE_MAP_OBJ);
    }

    HitSensor* addBodyMessageSensorMapObjPress(LiveActor* actor) {
        return addBodyMessageSensor(actor, ATYPE_MAP_OBJ_PRESS);
    }

    HitSensor* addBodyMessageSensorMapObjMoveCollision(LiveActor* actor) {
        return addBodyMessageSensor(actor, ATYPE_MAP_OBJ_MOVE_COLLISION);
    }

    HitSensor* addBodyMessageSensorEnemy(LiveActor* actor) {
        return addBodyMessageSensor(actor, ATYPE_ENEMY);
    }

    HitSensor* addMessageSensorReceiver(LiveActor* actor, const char* name) {
        return addHitSensor(actor, name, ATYPE_RECEIVER, 0U, 0.0F, {});
    }

    HitSensor* addMessageSensorMapObj(LiveActor* actor, const char* name) {
        return addHitSensor(actor, name, ATYPE_MAP_OBJ, 0U, 0.0F, {});
    }

    HitSensor* addMessageSensorMapObjMoveCollision(LiveActor* actor, const char* name) {
        return addHitSensor(actor, name, ATYPE_MAP_OBJ_MOVE_COLLISION, 0U, 0.0F, {});
    }

    HitSensor* addMessageSensorEnemy(LiveActor* actor, const char* name) {
        return addHitSensor(actor, name, ATYPE_ENEMY, 0U, 0.0F, {});
    }

    bool tryUpdateHitSensorsAll(LiveActor* actor) {
        if (actor == nullptr) {
            return false;
        }
        auto sensors = std::vector<HitSensor*>{};
        actor->collectHitSensors(sensors);
        if (sensors.empty()) {
            return false;
        }
        actor->updateHitSensors();
        return true;
    }

    void updateHitSensorsAll(LiveActor* actor) {
        if (actor != nullptr) {
            actor->updateHitSensors();
        }
    }

    bool isSensorType(const HitSensor* sensor, u32 type) {
        return sensor != nullptr && sensor->isType(type);
    }

    HitSensor* getSensorWithIndex(LiveActor* actor, int index) {
        return sensor_at(actor, index);
    }

    HitSensor* getSensor(LiveActor* actor, int index) {
        return sensor_at(actor, index);
    }

    LiveActor* getSensorHost(const HitSensor* sensor) {
        return sensor != nullptr ? sensor->mHost : nullptr;
    }

    void setSensorPos(HitSensor* sensor, const TVec3f& position) {
        if (sensor != nullptr) {
            sensor->mPosition = position;
        }
    }

    void setSensorRadius(LiveActor* actor, const char* name, f32 radius) {
        if (actor == nullptr || name == nullptr) {
            return;
        }
        if (auto* sensor = actor->getSensor(name); sensor != nullptr) {
            sensor->mRadius = radius;
        }
    }

    void validateHitSensors(LiveActor* actor) {
        if (actor != nullptr) {
            actor->validateHitSensors();
        }
    }

    void invalidateHitSensors(LiveActor* actor) {
        if (actor != nullptr) {
            actor->invalidateHitSensors();
        }
    }

    void validateHitSensor(LiveActor* actor, const char* name) {
        if (actor != nullptr && name != nullptr) {
            if (auto* sensor = actor->getSensor(name); sensor != nullptr) {
                sensor->validate();
            }
        }
    }

    void invalidateHitSensor(LiveActor* actor, const char* name) {
        if (actor != nullptr && name != nullptr) {
            if (auto* sensor = actor->getSensor(name); sensor != nullptr) {
                sensor->invalidate();
            }
        }
    }

    bool isValidHitSensor(LiveActor* actor, const char* name) {
        return actor != nullptr && name != nullptr && actor->getSensor(name) != nullptr &&
               actor->getSensor(name)->mValidByHost;
    }

    void clearHitSensors(LiveActor* actor) {
        if (actor == nullptr) {
            return;
        }
        auto sensors = std::vector<HitSensor*>{};
        actor->collectHitSensors(sensors);
        for (auto* sensor : sensors) {
            if (sensor != nullptr) {
                sensor->mSensorCount = 0U;
            }
        }
    }

    bool isSensor(const HitSensor* sensor, const char* name) {
        auto* host = getSensorHost(sensor);
        return host != nullptr && name != nullptr && host->getSensor(name) == sensor;
    }

    bool isSensorPlayer(const HitSensor* sensor) {
        return isSensorType(sensor, ATYPE_PLAYER);
    }

    bool isSensorBinder(const HitSensor* sensor) {
        return isSensorType(sensor, ATYPE_BINDER);
    }

    bool isSensorRide(const HitSensor* sensor) {
        return sensor_type_in_open_range(sensor, ATYPE_RIDE_START, ATYPE_RIDE_END);
    }

    bool isSensorPlayerOrRide(const HitSensor* sensor) {
        return isSensorPlayer(sensor) || isSensorRide(sensor);
    }

    bool isSensorEnemy(const HitSensor* sensor) {
        return sensor_type_in_open_range(sensor, ATYPE_ENEMY_START, ATYPE_ENEMY_END);
    }

    bool isSensorEnemyAttack(const HitSensor* sensor) {
        return isSensorType(sensor, ATYPE_ENEMY_ATTACK);
    }

    bool isSensorNpc(const HitSensor* sensor) {
        return sensor_type_in_open_range(sensor, ATYPE_NPC_START, ATYPE_NPC_END);
    }

    bool isSensorMapObj(const HitSensor* sensor) {
        return sensor_type_in_open_range(sensor, ATYPE_MAPOBJ_START, ATYPE_MAPOBJ_END);
    }

    bool isSensorAutoRush(const HitSensor* sensor) {
        return sensor_type_in_open_range(sensor, ATYPE_AUTO_RUSH_OBJ_START, ATYPE_AUTO_RUSH_OBJ_END);
    }

    bool isSensorRush(const HitSensor* sensor) {
        return sensor_type_in_open_range(sensor, ATYPE_RUSH_OBJ_START, ATYPE_RUSH_OBJ_END);
    }

    bool isSensorPressObj(const HitSensor* sensor) {
        return sensor_type_in_open_range(sensor, ATYPE_PRESS_OBJ_START, ATYPE_PRESS_OBJ_END);
    }

    bool isSensorEye(const HitSensor* sensor) {
        return isSensorType(sensor, ATYPE_EYE);
    }

    bool isSensorPush(const HitSensor* sensor) {
        return isSensorType(sensor, ATYPE_PUSH);
    }

    bool isSensorItem(const HitSensor* sensor) {
        return isSensorType(sensor, ATYPE_COIN) || isSensorType(sensor, ATYPE_COIN_RED) ||
               isSensorType(sensor, ATYPE_KINOKO_ONEUP);
    }

    bool tryGetItem(HitSensor* sender, HitSensor* receiver) {
        return send_message(ACTMES_ITEM_GET, receiver, sender);
    }

    const TVec3f& getSensorPos(const HitSensor* sensor) {
        return sensor->mPosition;
    }

    void calcSensorDirection(TVec3f* direction, const HitSensor* first, const HitSensor* second) {
        if (direction != nullptr && first != nullptr && second != nullptr) {
            direction->set(second->mPosition - first->mPosition);
        }
    }

    void calcSensorDirectionNormalize(TVec3f* direction, const HitSensor* first, const HitSensor* second) {
        calcSensorDirection(direction, first, second);
        if (direction != nullptr) {
            direction->normalize();
        }
    }

    void calcSensorHorizonNormalize(TVec3f* horizon, const TVec3f& gravity, const HitSensor* first,
                                    const HitSensor* second) {
        if (horizon == nullptr || first == nullptr || second == nullptr) {
            return;
        }
        horizon->killElement(second->mPosition - first->mPosition, gravity);
        horizon->normalize();
    }

    HitSensor* getMessageSensor() {
        auto* scene_holder = getSceneObjHolder();
        if (scene_holder == nullptr) {
            return nullptr;
        }
        auto* holder = dynamic_cast<MessageSensorHolder*>(scene_holder->getObj(SceneObj_MessageSensorHolder));
        return holder != nullptr ? holder->getSensor("body") : nullptr;
    }

    bool sendArbitraryMsg(u32 message, HitSensor* receiver, HitSensor* sender) {
        return send_message(message, receiver, sender);
    }

    bool sendMsgPush(HitSensor* receiver, HitSensor* sender) {
        return send_message(ACTMES_PUSH, receiver, sender);
    }

    bool sendMsgPlayerTrample(HitSensor* receiver, HitSensor* sender) {
        return send_message(ACTMES_PLAYER_TRAMPLE, receiver, sender);
    }

    bool sendMsgPlayerPunch(HitSensor* receiver, HitSensor* sender) {
        return send_message(ACTMES_PLAYER_PUNCH, receiver, sender);
    }

    bool sendMsgJump(HitSensor* receiver, HitSensor* sender) {
        return send_message(ACTMES_JUMP, receiver, sender);
    }

    bool sendMsgTouchJump(HitSensor* receiver, HitSensor* sender) {
        return send_message(ACTMES_TOUCHJUMP, receiver, sender);
    }

    bool sendMsgTaken(HitSensor* receiver, HitSensor* sender) {
        return send_message(ACTMES_TAKEN, receiver, sender);
    }

    bool sendMsgKick(HitSensor* receiver, HitSensor* sender) {
        return send_message(ACTMES_KICK, receiver, sender);
    }

    bool sendMsgAwayJump(HitSensor* receiver, HitSensor* sender) {
        return send_message(ACTMES_AWAYJUMP, receiver, sender);
    }

    bool sendSimpleMsgToActor(u32 message, LiveActor* actor) {
        auto* sensor = getMessageSensor();
        return actor != nullptr && sensor != nullptr && actor->receiveMessage(message, sensor, sensor);
    }

    bool sendMsgStartDemo(LiveActor* actor) {
        return sendSimpleMsgToActor(ACTMES_START_DEMO, actor);
    }

    void sendMsgToAllLiveActor(u32 message, LiveActor* actor) {
        if (getMessageSensor() == nullptr) {
            return;
        }
        if (auto* runtime = smgpc::runtime::RuntimeContext::try_instance(); runtime != nullptr) {
            runtime->scheduler().send_message_to_live_actors(message, actor);
        }
    }

    bool receiveItemShowMsg(u32 message, HitSensor*, HitSensor* receiver) {
        auto* host = getSensorHost(receiver);
        if (message != ACTMES_ITEM_SHOW || host == nullptr || !host->isDead()) {
            return false;
        }
        host->makeActorAppeared();
        return true;
    }

    bool receiveItemHideMsg(u32 message, HitSensor*, HitSensor* receiver) {
        auto* host = getSensorHost(receiver);
        if (message != ACTMES_ITEM_HIDE || host == nullptr || host->isDead()) {
            return false;
        }
        host->makeActorDead();
        return true;
    }

    bool isMsgPlayerHitAll(u32 message) {
        return message == ACTMES_PLAYER_PUNCH || message == ACTMES_PLAYER_UPPER_PUNCH ||
               message == ACTMES_JET_TURTLE_ATTACK || message == ACTMES_FIREBALL_ATTACK ||
               message == ACTMES_FREEZE_ATTACK || message == ACTMES_INVINCIBLE_ATTACK;
    }

    bool isMsgPlayerSpinAttack(u32 message) { return message == ACTMES_PLAYER_PUNCH; }
    bool isMsgPlayerTrample(u32 message) { return message == ACTMES_PLAYER_TRAMPLE; }
    bool isMsgPlayerHipDrop(u32 message) { return message == ACTMES_PLAYER_HIP_DROP; }
    bool isMsgPlayerHipDropFloor(u32 message) { return message == ACTMES_PLAYER_HIP_DROP_FLOOR; }
    bool isMsgPlayerUpperPunch(u32 message) { return message == ACTMES_PLAYER_UPPER_PUNCH; }
    bool isMsgPlayerKick(u32 message) { return message == ACTMES_KICK; }
    bool isMsgJetTurtleAttack(u32 message) { return message == ACTMES_JET_TURTLE_ATTACK; }
    bool isMsgFireBallAttack(u32 message) { return message == ACTMES_FIREBALL_ATTACK; }
    bool isMsgSearchlightAttack(u32 message) { return message == ACTMES_SEARCHLIGHT_ATTACK; }
    bool isMsgFreezeAttack(u32 message) { return message == ACTMES_FREEZE_ATTACK; }
    bool isMsgInvincibleAttack(u32 message) { return message == ACTMES_INVINCIBLE_ATTACK; }
    bool isMsgInvalidHit(u32 message) { return message == ACTMES_INVALID_HIT; }
    bool isMsgAutoRushBegin(u32 message) { return message == ACTMES_AUTORUSH_BEGIN; }
    bool isMsgRushBegin(u32 message) { return message == ACTMES_RUSH_BEGIN; }
    bool isMsgUpdateBaseMtx(u32 message) { return message == ACTMES_UPDATE_BASEMTX; }
    bool isMsgRushCancel(u32 message) { return message == ACTMES_RUSH_CANCEL; }
    bool isMsgIsRushTakeOver(u32 message) { return message == ACTMES_IS_RUSH_TAKEOVER; }
    bool isMsgFloorTouch(u32 message) { return message == ACTMES_FLOOR_TOUCH; }
    bool isMsgWallTouch(u32 message) { return message == ACTMES_WALL_TOUCH; }
    bool isMsgCeilTouch(u32 message) { return message == ACTMES_CEIL_TOUCH; }
    bool isMsgItemGet(u32 message) { return message == ACTMES_ITEM_GET; }
    bool isMsgItemPull(u32 message) { return message == ACTMES_ITEM_PULL; }
    bool isMsgItemShow(u32 message) { return message == ACTMES_ITEM_SHOW; }
    bool isMsgItemHide(u32 message) { return message == ACTMES_ITEM_HIDE; }
    bool isMsgItemStartMove(u32 message) { return message == ACTMES_ITEM_START_MOVE; }
    bool isMsgItemEndMove(u32 message) { return message == ACTMES_ITEM_END_MOVE; }
    bool isMsgInhaleBlackHole(u32 message) { return message == ACTMES_INHALE_BLACK_HOLE; }
    bool isMsgEnemyAttack(u32 message) { return message == ACTMES_ENEMY_ATTACK; }
    bool isMsgEnemyAttackFire(u32 message) { return message == ACTMES_ENEMY_ATTACK_FIRERUN; }
    bool isMsgEnemyAttackFireStrong(u32 message) { return message == ACTMES_ENEMY_ATTACK_FIRERUN_STRONG; }
    bool isMsgEnemyAttackElectric(u32 message) { return message == ACTMES_ENEMY_ATTACK_ELECTRIC; }
    bool isMsgExplosionAttack(u32 message) { return message == ACTMES_ENEMY_ATTACK_EXPLOSION; }
    bool isMsgToEnemyAttackBlow(u32 message) { return message == ACTMES_TO_ENEMY_ATTACK_BLOW; }
    bool isMsgToEnemyAttackTrample(u32 message) { return message == ACTMES_TO_ENEMY_ATTACK_TRAMPLE; }
    bool isMsgToEnemyAttackShockWave(u32 message) { return message == ACTMES_TO_ENEMY_ATTACK_SHOCK_WAVE; }
    bool isMsgSpinStormRange(u32 message) { return message == ACTMES_SPIN_STORM_RANGE; }
    bool isMsgTutorialStart(u32 message) { return message == ACTMES_TUTORIAL_START; }
    bool isMsgTutorialNext(u32 message) { return message == ACTMES_TUTORIAL_NEXT; }
    bool isMsgTutorialPrev(u32 message) { return message == ACTMES_TUTORIAL_PREV; }
    bool isMsgTutorialPass(u32 message) { return message == ACTMES_TUTORIAL_PASS; }
    bool isMsgTutorialOmit(u32 message) { return message == ACTMES_TUTORIAL_OMIT; }
    bool isMsgRaceReady(u32 message) { return message == ACTMES_RACE_READY; }
    bool isMsgRaceStart(u32 message) { return message == ACTMES_RACE_START; }
    bool isMsgRaceReset(u32 message) { return message == ACTMES_RACE_RESET; }
    bool isMsgLockOnStarPieceShoot(u32 message) { return message == ACTMES_IS_LOCKON_STAR_PIECE_SHOOT; }
    bool isMsgBallDashWall(u32 message) { return message == ACTMES_BALL_DASH_WALL; }
    bool isMsgBallDashGround(u32 message) { return message == ACTMES_BALL_DASH_GROUND; }
    bool isMsgStartPowerStarGet(u32 message) { return message == ACTMES_START_POWER_STAR_GET; }
    bool isMsgTouchPlantItem(u32 message) { return message == ACTMES_PLANT_GROUP_EMIT_ITEM; }
    bool isMsgHitmarkEmit(u32 message) { return message == ACTMES_HITMARK_EMIT; }
    bool isMsgStarPieceAttack(u32 message) { return message == ACTMES_STAR_PIECE_ATTACK; }
    bool isMsgStarPieceReflect(u32 message) { return message == ACTMES_IS_STAR_PIECE_REFLECT; }
    bool isMsgStarPieceGift(u32 message) {
        return message >= ACTMES_STAR_PIECE_GIFT && message < ACTMES_STAR_PIECE_GIFT_MAX;
    }

}  // namespace MR
