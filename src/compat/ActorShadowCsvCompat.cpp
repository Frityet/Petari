#include "compat/ActorShadowCsvCompat.hpp"

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/JointUtil.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/RarcArchive.hpp"
#include "resource/TextEncoding.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace smgpc::compat {
    namespace {
        struct ShadowTypeDefinition {
            std::string_view name;
            ActorShadowControllerKind kind;
        };

        constexpr auto kShadowTypes = std::array{
            ShadowTypeDefinition{"SurfaceCircle", ActorShadowControllerKind::SurfaceCircle},
            ShadowTypeDefinition{"SurfaceOval", ActorShadowControllerKind::SurfaceOval},
            ShadowTypeDefinition{"SurfaceBox", ActorShadowControllerKind::SurfaceBox},
            ShadowTypeDefinition{"VolumeSphere", ActorShadowControllerKind::VolumeSphere},
            ShadowTypeDefinition{"VolumeOval", ActorShadowControllerKind::VolumeOval},
            ShadowTypeDefinition{"VolumeOvalPole", ActorShadowControllerKind::VolumeOvalPole},
            ShadowTypeDefinition{"VolumeCylinder", ActorShadowControllerKind::VolumeCylinder},
            ShadowTypeDefinition{"VolumeBox", ActorShadowControllerKind::VolumeBox},
            ShadowTypeDefinition{"VolumeFlatModel", ActorShadowControllerKind::VolumeFlatModel},
            ShadowTypeDefinition{"VolumeLine", ActorShadowControllerKind::VolumeLine},
        };

        [[nodiscard]] std::string resource_name(std::string_view definition_name) {
            return std::string(definition_name) + ".bcsv";
        }

        [[nodiscard]] std::optional< std::string > decoded_string(const smgpc::resource::BcsvTable& table, std::size_t row, std::string_view field) {
            const auto value = table.get_string(row, field);
            return value.has_value() ? std::optional< std::string >{smgpc::resource::decode_cp932(*value)} : std::nullopt;
        }

        [[nodiscard]] std::optional< ActorShadowControllerKind > shadow_kind(const smgpc::resource::BcsvTable& table, std::size_t row) {
            const auto type = table.get_string(row, "Type");
            if (!type.has_value()) {
                return std::nullopt;
            }
            for (const auto& definition : kShadowTypes) {
                if (*type == definition.name) {
                    return definition.kind;
                }
            }
            return std::nullopt;
        }

        [[nodiscard]] float float_value(const smgpc::resource::BcsvTable& table, std::size_t row, std::string_view field, float default_value) {
            return table.get_float(row, field).value_or(default_value);
        }

        [[nodiscard]] std::int32_t integer_value(const smgpc::resource::BcsvTable& table, std::size_t row, std::string_view field,
                                                 std::int32_t default_value) {
            return table.get_s32(row, field).value_or(default_value);
        }

        [[nodiscard]] TVec3f drop_offset(const smgpc::resource::BcsvTable& table, std::size_t row) {
            const auto x = table.get_float(row, "DropOffsetX");
            const auto y = table.get_float(row, "DropOffsetY");
            const auto z = table.get_float(row, "DropOffsetZ");
            if (!x.has_value() || !y.has_value() || !z.has_value()) {
                return {};
            }
            return TVec3f{*x, *y, *z};
        }

        [[nodiscard]] TVec3f shadow_size(const smgpc::resource::BcsvTable& table, std::size_t row) {
            auto result = TVec3f{100.0F, 100.0F, 100.0F};
            const auto x = table.get_float(row, "SizeX");
            if (!x.has_value()) {
                return result;
            }
            result.x = *x;
            const auto y = table.get_float(row, "SizeY");
            if (!y.has_value()) {
                return result;
            }
            result.y = *y;
            if (const auto z = table.get_float(row, "SizeZ"); z.has_value()) {
                result.z = *z;
            }
            return result;
        }

        void bind_drop_position(ActorShadowControllerRuntimeState& controller, LiveActor& actor, std::string_view raw_joint_name) {
            controller.drop_position = nullptr;
            controller.drop_position_matrix = nullptr;
            if (raw_joint_name.empty() || raw_joint_name == "::ACTOR_TRANS") {
                controller.position_binding = ActorShadowPositionBinding::ActorTranslation;
                controller.drop_position = &actor.mPosition;
                return;
            }
            if (raw_joint_name == "::BASE_MATRIX") {
                controller.position_binding = ActorShadowPositionBinding::BaseMatrix;
                controller.drop_position_matrix = actor.getBaseMtx();
                return;
            }
            if (raw_joint_name == "::FIX_POSITION") {
                controller.position_binding = ActorShadowPositionBinding::FixedPosition;
                controller.fixed_drop_position.set(actor.mPosition);
                return;
            }
            if (raw_joint_name == "::OTHER_TRANS") {
                controller.position_binding = ActorShadowPositionBinding::OtherTranslation;
                controller.drop_position = &actor.mPosition;
                return;
            }
            if (raw_joint_name == "::OTHER_MATRIX") {
                controller.position_binding = ActorShadowPositionBinding::OtherMatrix;
                controller.drop_position_matrix = actor.getBaseMtx();
                return;
            }
            controller.position_binding = ActorShadowPositionBinding::JointMatrix;
            controller.drop_position_matrix = MR::getJointMtx(&actor, controller.joint_name_raw.c_str());
        }

        void apply_collision_mode(ActorShadowControllerRuntimeState& controller, std::int32_t value) {
            switch (value) {
            case 0:
                controller.calculation_mode = ActorShadowCalculationMode::Disabled;
                break;
            case 1:
                controller.calculation_mode = ActorShadowCalculationMode::Continuous;
                break;
            case 2:
                controller.calculation_mode = ActorShadowCalculationMode::OneTime;
                break;
            default:
                break;
            }
        }

        void apply_gravity_mode(ActorShadowControllerRuntimeState& controller, LiveActor& actor, std::int32_t value) {
            switch (value) {
            case 0:
                controller.gravity_mode = ActorShadowGravityMode::HostDirection;
                break;
            case 1:
                controller.gravity_mode = ActorShadowGravityMode::HostContinuous;
                controller.drop_direction = nullptr;
                controller.fixed_drop_direction.set(0.0F, 1.0F, 0.0F);
                break;
            case 2:
                controller.gravity_mode = ActorShadowGravityMode::HostOneTime;
                controller.drop_direction = nullptr;
                controller.fixed_drop_direction.set(0.0F, 1.0F, 0.0F);
                break;
            case 3:
                controller.gravity_mode = ActorShadowGravityMode::PrivateDisabled;
                break;
            case 4:
                controller.gravity_mode = ActorShadowGravityMode::PrivateContinuous;
                controller.drop_direction = nullptr;
                controller.fixed_drop_direction.set(0.0F, 1.0F, 0.0F);
                break;
            case 5:
                controller.gravity_mode = ActorShadowGravityMode::PrivateOneTime;
                controller.drop_direction = nullptr;
                controller.fixed_drop_direction.set(0.0F, 1.0F, 0.0F);
                break;
            default:
                break;
            }
            if (controller.drop_direction != nullptr) {
                controller.drop_direction = &actor.mGravity;
            }
        }

        [[nodiscard]] std::optional< std::size_t > resolve_line_endpoint(const ActorShadowRuntimeState& state,
                                                                         const std::optional< std::string >& raw_name) {
            if (state.controllers.size() == 1U) {
                return 0U;
            }
            if (!raw_name.has_value()) {
                return std::nullopt;
            }
            for (auto index = std::size_t{}; index < state.controllers.size(); ++index) {
                if (state.controllers[index].name_raw == *raw_name) {
                    return index;
                }
            }
            return std::nullopt;
        }

        [[nodiscard]] ActorShadowRuntimeState parse_shadow_table(LiveActor& actor, const smgpc::resource::BcsvTable& table) {
            auto result = ActorShadowRuntimeState{
                .valid = false,
                .calculation_enabled = false,
                .private_gravity = false,
                .capacity = table.entry_count(),
                .controllers = {},
            };
            result.controllers.reserve(result.capacity);

            for (auto row = std::size_t{}; row < static_cast< std::size_t >(table.entry_count()); ++row) {
                const auto kind = shadow_kind(table, row);
                if (!kind.has_value()) {
                    continue;
                }

                const auto raw_name = table.get_string(row, "Name").value_or("");
                auto controller = make_actor_shadow_controller_runtime_state(&actor, smgpc::resource::decode_cp932(raw_name), *kind, 100.0F);
                controller.name_raw = raw_name;
                controller.group_name = decoded_string(table, row, "GroupName").value_or("");
                controller.joint_name_raw = table.get_string(row, "Joint").value_or("");
                controller.joint_name = smgpc::resource::decode_cp932(controller.joint_name_raw);
                controller.drop_offset = drop_offset(table, row);
                controller.drop_length = float_value(table, row, "DropLength", 1000.0F);
                controller.drop_start_offset = float_value(table, row, "DropStart", 0.0F);
                controller.follow_host_scale = integer_value(table, row, "FollowScale", 1) != 0;
                controller.visible_sync_host = integer_value(table, row, "SyncShow", 1) != 0;

                // ShadowController construction starts collision calculation
                // enabled. CSV values 0..2 replace that mode; other authored
                // values retain the constructor state.
                controller.calculation_mode = ActorShadowCalculationMode::Continuous;
                apply_collision_mode(controller, integer_value(table, row, "Collision", 0));
                apply_gravity_mode(controller, actor, integer_value(table, row, "Gravity", 0));
                bind_drop_position(controller, actor, controller.joint_name_raw);

                switch (*kind) {
                case ActorShadowControllerKind::SurfaceCircle:
                    controller.radius = float_value(table, row, "Radius", 100.0F);
                    break;
                case ActorShadowControllerKind::SurfaceOval:
                case ActorShadowControllerKind::SurfaceBox:
                    controller.size = shadow_size(table, row);
                    break;
                case ActorShadowControllerKind::VolumeSphere:
                case ActorShadowControllerKind::VolumeOval:
                case ActorShadowControllerKind::VolumeOvalPole:
                case ActorShadowControllerKind::VolumeCylinder:
                case ActorShadowControllerKind::VolumeBox:
                case ActorShadowControllerKind::VolumeFlatModel:
                case ActorShadowControllerKind::VolumeLine:
                    controller.volume_start_offset = float_value(table, row, "VolumeStart", 100.0F);
                    controller.volume_end_offset = float_value(table, row, "VolumeEnd", 100.0F);
                    controller.volume_cut_drop_length = integer_value(table, row, "VolumeCut", 0) != 0;
                    break;
                }
                switch (*kind) {
                case ActorShadowControllerKind::VolumeSphere:
                case ActorShadowControllerKind::VolumeCylinder:
                    controller.radius = float_value(table, row, "Radius", 100.0F);
                    break;
                case ActorShadowControllerKind::VolumeOval:
                case ActorShadowControllerKind::VolumeOvalPole:
                case ActorShadowControllerKind::VolumeBox:
                    controller.size = shadow_size(table, row);
                    break;
                case ActorShadowControllerKind::VolumeFlatModel:
                    controller.model_name = decoded_string(table, row, "Model");
                    break;
                case ActorShadowControllerKind::VolumeLine:
                    controller.line_start_name_raw = table.get_string(row, "LineStart");
                    controller.line_end_name_raw = table.get_string(row, "LineEnd");
                    if (controller.line_start_name_raw.has_value()) {
                        controller.line_start_name = smgpc::resource::decode_cp932(*controller.line_start_name_raw);
                    }
                    if (controller.line_end_name_raw.has_value()) {
                        controller.line_end_name = smgpc::resource::decode_cp932(*controller.line_end_name_raw);
                    }
                    controller.line_start_radius = float_value(table, row, "LineStartRadius", 100.0F);
                    controller.line_end_radius = float_value(table, row, "LineEndRadius", 100.0F);
                    break;
                case ActorShadowControllerKind::SurfaceCircle:
                case ActorShadowControllerKind::SurfaceOval:
                case ActorShadowControllerKind::SurfaceBox:
                    break;
                }

                result.controllers.push_back(std::move(controller));
                if (*kind == ActorShadowControllerKind::VolumeLine) {
                    auto& line = result.controllers.back();
                    line.line_start_controller_index = resolve_line_endpoint(result, line.line_start_name_raw);
                    line.line_end_controller_index = resolve_line_endpoint(result, line.line_end_name_raw);
                }
            }

            result.valid = !result.controllers.empty();
            for (const auto& controller : result.controllers) {
                result.calculation_enabled |= controller.calculation_mode != ActorShadowCalculationMode::Disabled;
                result.private_gravity |= controller.gravity_mode == ActorShadowGravityMode::PrivateContinuous ||
                                          controller.gravity_mode == ActorShadowGravityMode::PrivateOneTime;
            }
            return result;
        }

        [[nodiscard]] ActorShadowRuntimeState empty_missing_csv_state() {
            auto result = ActorShadowRuntimeState{
                .valid = false,
                .calculation_enabled = false,
                .private_gravity = false,
                .capacity = 1U,
                .controllers = {},
            };
            result.controllers.reserve(1U);
            return result;
        }
    }  // namespace

    void initialize_actor_shadow_from_archive(LiveActor* actor, const smgpc::resource::RarcArchive& archive, std::string_view definition_name) {
        if (actor == nullptr) {
            throw std::invalid_argument("Shadow CSV initialization requires a LiveActor.");
        }
        const auto requested = resource_name(definition_name);
        const auto* entry = archive.find_resource(requested);
        auto candidate = entry == nullptr ? empty_missing_csv_state() :
                                            parse_shadow_table(*actor, smgpc::resource::BcsvTable::from_bytes(archive.file_data(*entry)));
        replace_actor_shadow_runtime_state(actor, std::move(candidate));
    }

    void initialize_actor_shadow_from_model_archive(LiveActor* actor, std::string_view definition_name) {
        if (actor == nullptr) {
            throw std::invalid_argument("Shadow CSV initialization requires a LiveActor.");
        }
        const auto data = actor_model_resource_data_if_present(actor, resource_name(definition_name));
        auto candidate = data.has_value() ? parse_shadow_table(*actor, smgpc::resource::BcsvTable::from_bytes(*data)) : empty_missing_csv_state();
        replace_actor_shadow_runtime_state(actor, std::move(candidate));
    }
}  // namespace smgpc::compat
