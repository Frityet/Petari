#include "CameraParam.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace smgpc::camera {
    namespace {

        [[nodiscard]] CameraParamVec3 to_camera_vec3(std::array<float, 3U> value) {
            return CameraParamVec3 {
                .x = value[0U],
                .y = value[1U],
                .z = value[2U],
            };
        }

        void read_float(const resource::BcsvTable &table, std::size_t row, std::string_view name, float &out) {
            if (const auto value = table.get_float(row, name); value.has_value()) {
                out = *value;
            }
        }

        void read_s32(const resource::BcsvTable &table, std::size_t row, std::string_view name, std::int32_t &out) {
            if (const auto value = table.get_s32(row, name); value.has_value()) {
                out = *value;
            }
        }

        void read_u32(const resource::BcsvTable &table, std::size_t row, std::string_view name, std::uint32_t &out) {
            if (const auto value = table.get_u32(row, name); value.has_value()) {
                out = *value;
            }
        }

        void read_vec3(const resource::BcsvTable &table, std::size_t row, std::string_view name, CameraParamVec3 &out) {
            if (const auto value = table.get_vec3(row, name); value.has_value()) {
                out = to_camera_vec3(*value);
            }
        }

        [[nodiscard]] std::string arranged_camera_type(std::uint32_t version, std::string camera_type) {
            if (version < 0x30004U && camera_type == "CAM_TYPE_DONKETSU_TEST") {
                return "CAM_TYPE_BOSS_DONKETSU";
            }
            if (version < 0x30006U) {
                if (camera_type == "CAM_TYPE_BEHIND_DEBUG") {
                    return "CAM_TYPE_SLIDER";
                }
                if (camera_type == "CAM_TYPE_INWARD_TOWER_TEST") {
                    return "CAM_TYPE_INWARD_TOWER";
                }
                if (camera_type == "CAM_TYPE_EYE_FIXED_THERE_TEST") {
                    return "CAM_TYPE_EYEPOS_FIX_THERE";
                }
            }
            if (version < 0x30009U && camera_type == "CAM_TYPE_ICECUBE_PLANET") {
                return "CAM_TYPE_CUBE_PLANET";
            }

            return camera_type;
        }

        void load_extra_params(const resource::BcsvTable &table, std::size_t row, CameraExtraParam &extra) {
            read_vec3(table, row, "woffset", extra.w_offset);
            read_float(table, row, "loffset", extra.l_offset);
            read_float(table, row, "loffsetv", extra.l_offset_v);
            read_float(table, row, "roll", extra.roll);
            read_float(table, row, "fovy", extra.fovy);
            read_s32(table, row, "camint", extra.cam_int);
            read_float(table, row, "upper", extra.upper);
            read_float(table, row, "lower", extra.lower);
            read_s32(table, row, "gndint", extra.gnd_int);
            read_float(table, row, "uplay", extra.u_play);
            read_float(table, row, "lplay", extra.l_play);
            read_s32(table, row, "pushdelay", extra.push_delay);
            read_s32(table, row, "pushdelaylow", extra.push_delay_low);
            read_s32(table, row, "udown", extra.u_down);
            read_s32(table, row, "vpanuse", extra.v_pan_use);
            read_vec3(table, row, "vpanaxis", extra.v_pan_axis);

            constexpr std::array<std::string_view, 6U> flag_names {
                "flag.noreset",
                "flag.nofovy",
                "flag.lofserpoff",
                "flag.antibluroff",
                "flag.collisionoff",
                "flag.subjectiveoff",
            };

            for (auto i = 0U; i < flag_names.size(); ++i) {
                if (const auto flag = table.get_s32(row, flag_names[i]); flag.has_value()) {
                    extra.flags |= static_cast<std::uint16_t>((*flag & 1) << i);
                }
            }
        }

        void load_general_params(const resource::BcsvTable &table, std::size_t row, std::string_view camera_type, CameraGeneralParam &general) {
            read_float(table, row, "dist", general.dist);
            read_vec3(table, row, "axis", general.axis);
            read_vec3(table, row, "wpoint", general.w_point);
            read_vec3(table, row, "up", general.up);
            if (const auto angle_a = table.get_float(row, "angleA"); angle_a.has_value()) {
                general.angle_a = *angle_a;
            } else if (camera_type == "CAM_TYPE_PLANET") {
                general.angle_a = 30.0F;
            }
            read_float(table, row, "angleB", general.angle_b);
            read_s32(table, row, "num1", general.num1);
            read_s32(table, row, "num2", general.num2);
            if (const auto string_param = table.get_string(row, "string"); string_param.has_value()) {
                general.string_param = *string_param;
            }
        }

    }  // namespace

    bool CameraParamChunk::is_on_no_reset() const {
        return (extra.flags & (1U << 0U)) != 0U;
    }

    bool CameraParamChunk::is_on_use_fovy() const {
        return (extra.flags & (1U << 1U)) != 0U;
    }

    bool CameraParamChunk::is_l_offset_erp_off() const {
        return (extra.flags & (1U << 2U)) != 0U;
    }

    bool CameraParamChunk::is_anti_blur_off() const {
        return (extra.flags & (1U << 3U)) != 0U;
    }

    bool CameraParamChunk::is_collision_off() const {
        return (extra.flags & (1U << 4U)) != 0U;
    }

    bool CameraParamChunk::is_subjective_camera_off() const {
        return (extra.flags & (1U << 5U)) != 0U;
    }

    std::vector<CameraParamChunk> load_camera_param_chunks(const resource::BcsvTable &table) {
        auto chunks = std::vector<CameraParamChunk>{};
        chunks.reserve(table.entry_count());

        for (auto row = 0U; row < table.entry_count(); ++row) {
            auto chunk = CameraParamChunk {};
            if (const auto version = table.get_u32(row, "version"); version.has_value()) {
                chunk.version = *version;
            }
            if (const auto id = table.get_string(row, "id"); id.has_value()) {
                chunk.id = *id;
            }
            if (const auto camera_type = table.get_string(row, "camtype"); camera_type.has_value()) {
                chunk.camera_type = arranged_camera_type(chunk.version, *camera_type);
            }

            load_extra_params(table, row, chunk.extra);
            load_general_params(table, row, chunk.camera_type, chunk.general);

            if (const auto thru = table.get_s32(row, "gflag.thru"); thru.has_value()) {
                chunk.game_thru = *thru;
            }
            read_s32(table, row, "gflag.enableEndErpFrame", chunk.game_enable_end_erp_frame);
            read_u32(table, row, "gflag.camendint", chunk.game_cam_end_int);
            read_s32(table, row, "eflag.enableErpFrame", chunk.event_enable_erp_frame);
            read_s32(table, row, "eflag.enableEndErpFrame", chunk.event_enable_end_erp_frame);
            read_u32(table, row, "camendint", chunk.event_cam_end_int);
            read_u32(table, row, "evfrm", chunk.event_frame);
            read_u32(table, row, "evpriority", chunk.event_priority);

            chunks.push_back(std::move(chunk));
        }

        return chunks;
    }

    std::optional<CameraParamChunk> find_camera_param_chunk(std::span<const CameraParamChunk> chunks, std::string_view id) {
        const auto it = std::ranges::find_if(chunks, [id](const auto &chunk) { return chunk.id == id; });
        if (it == chunks.end()) {
            return std::nullopt;
        }

        return *it;
    }

}  // namespace smgpc::camera
