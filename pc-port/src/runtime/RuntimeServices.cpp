#include "RuntimeServices.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <system_error>
#include <utility>

#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Scene/SceneFunction.hpp"
#include "Game/System/WPadRumbleData.hpp"
#include "render/effects/JpcBillboard.hpp"
#include "resource/BmgMessageArchive.hpp"
#include "resource/TextEncoding.hpp"

namespace smgpc::runtime {
    namespace {

        constexpr auto CAMERA_SHAKE_FRAME_COUNT = std::uint32_t{25U};
        constexpr auto CAMERA_SHAKE_AMPLITUDES = std::array<float, 7U>{0.08F, 0.2F, 0.5F, 1.0F, 3.0F, 6.0F, 9.0F};

        [[nodiscard]] std::size_t camera_shake_index(CameraSystemService::ShakeRequestKind kind) {
            return static_cast<std::size_t>(kind);
        }

        [[nodiscard]] float camera_singly_vertical_offset(float amplitude, std::uint32_t step) {
            const auto remaining = static_cast<float>(CAMERA_SHAKE_FRAME_COUNT - step);
            const auto primary = std::sin((12.566371F * remaining) / static_cast<float>(CAMERA_SHAKE_FRAME_COUNT));
            const auto attenuation = std::sin((1.5707964F * remaining) / static_cast<float>(CAMERA_SHAKE_FRAME_COUNT));
            return amplitude * primary * attenuation;
        }

        [[nodiscard]] bool exists_regular_file(const std::filesystem::path &path) {
            std::error_code error{};
            return std::filesystem::is_regular_file(path, error);
        }

        constexpr auto SAVE_DATA_CONTAINER_NAME = std::string_view{"GameData.bin"};
        constexpr auto SAVE_DATA_VERSION = std::uint32_t{2U};
        constexpr auto SAVE_DATA_FILE_INFO_SIZE = std::size_t{16U};
        constexpr auto SAVE_DATA_FILE_NAME_SIZE = std::size_t{12U};
        constexpr auto SAVE_DATA_HEADER_SIZE = std::size_t{16U};
        constexpr auto SAVE_DATA_GAME_FILE_SIZE = std::size_t{0xF80U};
        constexpr auto SAVE_DATA_CONFIG_FILE_SIZE = std::size_t{0x60U};
        constexpr auto SAVE_DATA_SYSTEM_FILE_SIZE = std::size_t{0x80U};
        enum class SaveDataByteOrder {
            BigEndian,
            LittleEndian,
        };

        [[nodiscard]] std::uint16_t read_save_u16(std::span<const std::uint8_t> bytes, std::size_t offset, SaveDataByteOrder byte_order) {
            if (offset + sizeof(std::uint16_t) > bytes.size()) {
                return 0U;
            }

            if (byte_order == SaveDataByteOrder::BigEndian) {
                return static_cast<std::uint16_t>((static_cast<std::uint16_t>(bytes[offset]) << 8U) | bytes[offset + 1U]);
            }
            return static_cast<std::uint16_t>((static_cast<std::uint16_t>(bytes[offset + 1U]) << 8U) | bytes[offset]);
        }

        [[nodiscard]] std::uint32_t read_save_u32(std::span<const std::uint8_t> bytes, std::size_t offset, SaveDataByteOrder byte_order) {
            if (offset + sizeof(std::uint32_t) > bytes.size()) {
                return 0U;
            }

            if (byte_order == SaveDataByteOrder::BigEndian) {
                return (static_cast<std::uint32_t>(bytes[offset]) << 24U) | (static_cast<std::uint32_t>(bytes[offset + 1U]) << 16U) |
                       (static_cast<std::uint32_t>(bytes[offset + 2U]) << 8U) | bytes[offset + 3U];
            }
            return (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U) | (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U) |
                   (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U) | bytes[offset];
        }

        void write_save_u32(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint32_t value,
                            SaveDataByteOrder byte_order = SaveDataByteOrder::BigEndian) {
            if (offset + sizeof(value) > bytes.size()) {
                return;
            }

            if (byte_order == SaveDataByteOrder::BigEndian) {
                bytes[offset] = static_cast<std::uint8_t>(value >> 24U);
                bytes[offset + 1U] = static_cast<std::uint8_t>(value >> 16U);
                bytes[offset + 2U] = static_cast<std::uint8_t>(value >> 8U);
                bytes[offset + 3U] = static_cast<std::uint8_t>(value);
            } else {
                bytes[offset] = static_cast<std::uint8_t>(value);
                bytes[offset + 1U] = static_cast<std::uint8_t>(value >> 8U);
                bytes[offset + 2U] = static_cast<std::uint8_t>(value >> 16U);
                bytes[offset + 3U] = static_cast<std::uint8_t>(value >> 24U);
            }
        }

        [[nodiscard]] std::uint32_t save_check_sum(std::span<const std::uint8_t> bytes, SaveDataByteOrder byte_order) {
            auto sum = std::uint16_t{};
            auto inv_sum = std::uint16_t{};
            const auto word_count = bytes.size() / sizeof(std::uint16_t);
            for (auto index = std::size_t{}; index < word_count; ++index) {
                const auto word = read_save_u16(bytes, index * sizeof(std::uint16_t), byte_order);
                sum = static_cast<std::uint16_t>(sum + word);
                inv_sum = static_cast<std::uint16_t>(inv_sum + static_cast<std::uint16_t>(~word));
            }
            return (static_cast<std::uint32_t>(sum) << 16U) | inv_sum;
        }

        [[nodiscard]] std::uint32_t align_save_data_size(std::uint32_t size) {
            return (size + 0x1FU) & ~0x1FU;
        }

        [[nodiscard]] bool is_valid_save_data_container_shape(std::uint32_t version, std::uint32_t file_count,
                                                              std::uint32_t data_size, std::size_t byte_count) {
            return version == SAVE_DATA_VERSION && file_count > 0U && file_count < 24U &&
                   data_size >= SAVE_DATA_HEADER_SIZE + file_count * SAVE_DATA_FILE_INFO_SIZE &&
                   align_save_data_size(data_size) == byte_count;
        }

        [[nodiscard]] bool has_valid_save_data_checksum(std::span<const std::uint8_t> bytes,
                                                        SaveDataByteOrder byte_order) {
            if (bytes.size() < SAVE_DATA_HEADER_SIZE) {
                return false;
            }
            const auto version = read_save_u32(bytes, 4U, byte_order);
            const auto file_count = read_save_u32(bytes, 8U, byte_order);
            const auto data_size = read_save_u32(bytes, 12U, byte_order);
            if (!is_valid_save_data_container_shape(version, file_count, data_size, bytes.size())) {
                return false;
            }
            const auto expected = read_save_u32(bytes, 0U, byte_order);
            const auto actual = save_check_sum(bytes.subspan(sizeof(std::uint32_t), data_size - sizeof(std::uint32_t)),
                                               byte_order);
            return expected == actual;
        }

        [[nodiscard]] std::vector<std::uint8_t> convert_save_data_container_byte_order(std::span<const std::uint8_t> bytes,
                                                                                       SaveDataByteOrder source_byte_order,
                                                                                       SaveDataByteOrder destination_byte_order) {
            auto converted = std::vector<std::uint8_t>(bytes.begin(), bytes.end());
            const auto version = read_save_u32(bytes, 4U, source_byte_order);
            const auto file_count = read_save_u32(bytes, 8U, source_byte_order);
            const auto data_size = read_save_u32(bytes, 12U, source_byte_order);
            if (!has_valid_save_data_checksum(bytes, source_byte_order)) {
                throw std::invalid_argument("Save-data byte-order conversion requires a valid source container");
            }

            write_save_u32(converted, 4U, version, destination_byte_order);
            write_save_u32(converted, 8U, file_count, destination_byte_order);
            write_save_u32(converted, 12U, data_size, destination_byte_order);
            for (auto file_index = std::uint32_t{}; file_index < file_count; ++file_index) {
                const auto info_offset = SAVE_DATA_HEADER_SIZE + static_cast<std::size_t>(file_index) * SAVE_DATA_FILE_INFO_SIZE;
                write_save_u32(converted, info_offset + SAVE_DATA_FILE_NAME_SIZE,
                               read_save_u32(bytes, info_offset + SAVE_DATA_FILE_NAME_SIZE, source_byte_order), destination_byte_order);
            }

            write_save_u32(converted, 0U,
                           save_check_sum(std::span<const std::uint8_t>(converted).subspan(sizeof(std::uint32_t), data_size - sizeof(std::uint32_t)),
                                          destination_byte_order),
                           destination_byte_order);
            return converted;
        }

        [[nodiscard]] std::vector<std::uint8_t> retail_save_data_container_for_host(
            std::span<const std::uint8_t> retail_bytes) {
            if (!has_valid_save_data_checksum(retail_bytes, SaveDataByteOrder::BigEndian)) {
                throw std::invalid_argument("Persisted GameData.bin is not a valid retail big-endian container");
            }
            return convert_save_data_container_byte_order(retail_bytes, SaveDataByteOrder::BigEndian,
                                                          SaveDataByteOrder::LittleEndian);
        }

        [[nodiscard]] std::vector<std::uint8_t> host_save_data_container_for_retail(
            std::span<const std::uint8_t> host_bytes) {
            if (!has_valid_save_data_checksum(host_bytes, SaveDataByteOrder::LittleEndian)) {
                throw std::invalid_argument("Host save buffer is not a valid translated retail container");
            }
            return convert_save_data_container_byte_order(host_bytes, SaveDataByteOrder::LittleEndian,
                                                          SaveDataByteOrder::BigEndian);
        }

        [[nodiscard]] std::optional<std::size_t> save_data_file_size(std::string_view name) {
            if (name.starts_with("mario") || name.starts_with("luigi")) {
                return SAVE_DATA_GAME_FILE_SIZE;
            }
            if (name.starts_with("config")) {
                return SAVE_DATA_CONFIG_FILE_SIZE;
            }
            if (name == "sysconf") {
                return SAVE_DATA_SYSTEM_FILE_SIZE;
            }
            return std::nullopt;
        }

        struct StarPointerProjection {
            float x = 0.0F;
            float y = 0.0F;
            float radius = 0.0F;
        };

        [[nodiscard]] std::optional<StarPointerProjection> project_star_pointer_target(const StarPointerTargetState &target, const smgpc::camera::CameraPose &pose, bool check_z) {
            if (target.actor == nullptr) {
                return std::nullopt;
            }

            constexpr auto PI = 3.14159265358979323846F;
            const auto world = smgpc::camera::CameraParamVec3{
                .x = target.actor->mPosition.x + target.offset.x,
                .y = target.actor->mPosition.y + target.offset.y,
                .z = target.actor->mPosition.z + target.offset.z,
            };
            const auto camera = smgpc::camera::transform_world_to_camera(pose, world);
            if (check_z && camera.z <= pose.near_clip) {
                return std::nullopt;
            }

            const auto depth = std::abs(camera.z);
            if (depth <= 0.0001F) {
                return std::nullopt;
            }

            const auto fovy = pose.fovy_degrees * PI / 180.0F;
            const auto focal_y = 1.0F / std::tan(fovy * 0.5F);
            const auto focal_x = focal_y / pose.aspect_ratio;
            const auto half_width = static_cast<float>(render::core::kWiiLogicalFramebufferWidth) * 0.5F;
            const auto half_height = static_cast<float>(render::core::kWiiLogicalFramebufferHeight) * 0.5F;
            const auto ndc_x = (camera.x / camera.z) * focal_x + pose.projection_offset_x;
            const auto ndc_y = (camera.y / camera.z) * focal_y + pose.projection_offset_y;
            return StarPointerProjection{
                .x = (ndc_x * half_width) + half_width,
                .y = (ndc_y * half_height) + half_height,
                .radius = std::max((target.radius / depth) * focal_y * half_height, 1.0F),
            };
        }

        [[nodiscard]] std::filesystem::path weakly_canonical_or_normal(const std::filesystem::path &path) {
            std::error_code error{};
            auto canonical = std::filesystem::weakly_canonical(path, error);
            if (!error) {
                return canonical;
            }

            return path.lexically_normal();
        }

        [[nodiscard]] std::vector<std::uint8_t> read_binary_file(const std::filesystem::path &path) {
            auto file = std::ifstream(path, std::ios::binary);
            if (!file) {
                throw std::runtime_error("Cannot open save file " + path.string());
            }

            file.seekg(0, std::ios::end);
            const auto size = file.tellg();
            if (size < 0) {
                throw std::runtime_error("Cannot determine save file size " + path.string());
            }

            auto bytes = std::vector<std::uint8_t>(static_cast<std::size_t>(size));
            file.seekg(0, std::ios::beg);
            file.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            if (!file) {
                throw std::runtime_error("Cannot read save file " + path.string());
            }

            return bytes;
        }

        void write_binary_file(const std::filesystem::path &path, std::span<const std::uint8_t> bytes) {
            std::error_code error{};
            std::filesystem::create_directories(path.parent_path(), error);
            if (error) {
                throw std::runtime_error("Cannot create save directory " + path.parent_path().string());
            }

            auto file = std::ofstream(path, std::ios::binary | std::ios::trunc);
            if (!file) {
                throw std::runtime_error("Cannot open save file for writing " + path.string());
            }

            file.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            if (!file) {
                throw std::runtime_error("Cannot write save file " + path.string());
            }
        }

        [[nodiscard]] std::string texture_format_name(smgpc::resource::TplTextureFormat format) {
            switch (format) {
            case smgpc::resource::TplTextureFormat::I4:
                return "I4";
            case smgpc::resource::TplTextureFormat::I8:
                return "I8";
            case smgpc::resource::TplTextureFormat::IA4:
                return "IA4";
            case smgpc::resource::TplTextureFormat::IA8:
                return "IA8";
            case smgpc::resource::TplTextureFormat::RGB565:
                return "RGB565";
            case smgpc::resource::TplTextureFormat::RGB5A3:
                return "RGB5A3";
            case smgpc::resource::TplTextureFormat::RGBA8:
                return "RGBA8";
            case smgpc::resource::TplTextureFormat::C4:
                return "C4";
            case smgpc::resource::TplTextureFormat::C8:
                return "C8";
            case smgpc::resource::TplTextureFormat::C14X2:
                return "C14X2";
            case smgpc::resource::TplTextureFormat::CMPR:
                return "CMPR";
            }

            return "Unknown";
        }

        [[nodiscard]] bool effect_draw_type_accepts_order(s32 draw_type, std::string_view draw_order) {
            if (draw_type == MR::DrawType_EffectDraw3D) {
                return draw_order.empty() || draw_order == "3D";
            }
            if (draw_type == MR::DrawType_EffectDrawIndirect) {
                return draw_order == "Indirect";
            }
            if (draw_type == MR::DrawType_EffectDrawAfterIndirect) {
                return draw_order == "AfterIndirect";
            }
            if (draw_type == MR::DrawType_EffectDraw2D) {
                return draw_order.empty() || draw_order == "2D";
            }

            if (draw_type == MR::DrawType_EffectDrawFor2DModel) {
                return draw_order == "2DModel" || draw_order == "For2DModel";
            }
            if (draw_type == MR::DrawType_EffectDrawForBloomEffect) {
                return draw_order == "Bloom" || draw_order == "ForBloomEffect";
            }
            if (draw_type == MR::DrawType_EffectDrawAfterImageEffect) {
                return draw_order == "AfterImageEffect";
            }

            return false;
        }

        [[nodiscard]] bool effect_draw_type_uses_world_camera(s32 draw_type) {
            return draw_type == MR::DrawType_EffectDraw3D || draw_type == MR::DrawType_EffectDrawIndirect ||
                   draw_type == MR::DrawType_EffectDrawAfterIndirect || draw_type == MR::DrawType_EffectDrawForBloomEffect ||
                   draw_type == MR::DrawType_EffectDrawAfterImageEffect;
        }

        [[nodiscard]] std::optional<std::uint8_t> jpa_particle_shape_type(
            bool child, const smgpc::render::effects::JpcBaseShapeMetadata *base_shape,
            const smgpc::render::effects::JpcChildShapeMetadata *child_shape) {
            if (child) {
                if (child_shape != nullptr) {
                    return child_shape->shape_type;
                }
                return std::nullopt;
            }
            if (base_shape != nullptr) {
                return base_shape->shape_type;
            }
            return std::nullopt;
        }

        [[nodiscard]] const char *jpa_packet_path_name(smgpc::render::effects::JpcParticlePacketPath path) {
            switch (path) {
            case smgpc::render::effects::JpcParticlePacketPath::ScreenSpace:
                return "JpcBillboard2D";
            case smgpc::render::effects::JpcParticlePacketPath::WorldBillboard:
                return "JpcBillboard3D";
            }
            throw std::logic_error("Unknown JPC particle packet path");
        }

        [[nodiscard]] float effect_billboard_size(const smgpc::render::effects::JpcTextureMetadata &texture, const smgpc::render::effects::ResolvedEffectResource &resource) {
            const auto source_size = static_cast<float>(std::max(texture.width, texture.height));
            const auto base_shape_scale = resource.resource != nullptr && resource.resource->base_shape.has_value() ?
                                              std::max(resource.resource->base_shape->base_size_x, resource.resource->base_shape->base_size_y) :
                                              1.0F;
            const auto scale = std::max(resource.auto_effect_scale_value * base_shape_scale, 0.125F);
            return std::clamp(source_size * scale, 8.0F, 192.0F);
        }

        [[nodiscard]] const smgpc::render::effects::JpcTextureMetadata *primary_effect_texture(const smgpc::render::effects::ResolvedEffectResource &resource) {
            if (resource.primary_texture_index.has_value()) {
                for (const auto &texture : resource.textures) {
                    if (texture.index == *resource.primary_texture_index) {
                        return &texture;
                    }
                }
            }

            return resource.textures.empty() ? nullptr : &resource.textures.front();
        }

        [[nodiscard]] const smgpc::render::effects::JpcTextureMetadata *child_effect_texture(const smgpc::render::effects::ResolvedEffectResource &resource) {
            if (resource.resource != nullptr && resource.resource->child_texture_index.has_value()) {
                for (const auto &texture : resource.textures) {
                    if (texture.index == *resource.resource->child_texture_index) {
                        return &texture;
                    }
                }
            }

            return primary_effect_texture(resource);
        }

        [[nodiscard]] std::array<float, 3U> effect_host_translation(const std::array<float, 12U> &matrix) {
            return {matrix[3U], matrix[7U], matrix[11U]};
        }

        [[nodiscard]] const char *effect_host_binding_source_name(EffectHostBindingSource source) {
            switch (source) {
            case EffectHostBindingSource::LiveActorBaseMatrix:
                return "LiveActorBaseMatrix";
            case EffectHostBindingSource::LayoutActorTransform:
                return "LayoutActorTransform";
            case EffectHostBindingSource::SimpleLayoutOrigin:
                return "SimpleLayoutOrigin";
            }

            return "Unknown";
        }

        [[nodiscard]] bool effect_host_matches(const ActiveEffectInstance &active, std::string_view host_name,
                                               const void *host_identity) {
            if (host_identity != nullptr) {
                return active.host_identity == host_identity;
            }
            return active.host_identity == nullptr && active.actor_name == host_name;
        }

        [[nodiscard]] render::DepthCompare jpa_compare_from_config(std::uint8_t compare) {
            switch (compare & 0x07U) {
            case 0U:
                return render::DepthCompare::Never;
            case 1U:
                return render::DepthCompare::Less;
            case 2U:
                return render::DepthCompare::LessEqual;
            case 3U:
                return render::DepthCompare::Equal;
            case 4U:
                return render::DepthCompare::NotEqual;
            case 5U:
                return render::DepthCompare::GreaterEqual;
            case 6U:
                return render::DepthCompare::Greater;
            case 7U:
                return render::DepthCompare::Always;
            default:
                return render::DepthCompare::Always;
            }
        }

        [[nodiscard]] std::uint8_t jpa_blend_factor_from_config(std::uint8_t factor) {
            constexpr auto BLEND_FACTORS = std::array<std::uint8_t, 10U>{0U, 1U, 2U, 3U, 2U, 3U, 4U, 5U, 6U, 7U};
            return factor < BLEND_FACTORS.size() ? BLEND_FACTORS[factor] : 0U;
        }

        [[nodiscard]] render::GxAlphaCompare2D jpa_alpha_compare(const smgpc::render::effects::JpcBaseShapeMetadata *base_shape) {
            if (base_shape == nullptr) {
                return {};
            }

            const auto comp0 = static_cast<std::uint8_t>(base_shape->alpha_compare_config & 0x07U);
            const auto op = static_cast<std::uint8_t>((base_shape->alpha_compare_config >> 3U) & 0x03U);
            const auto comp1 = static_cast<std::uint8_t>((base_shape->alpha_compare_config >> 5U) & 0x07U);
            return render::GxAlphaCompare2D{
                .comp0 = comp0,
                .ref0 = base_shape->alpha_ref0,
                .op = op,
                .comp1 = comp1,
                .ref1 = base_shape->alpha_ref1,
                .enabled = comp0 != 7U || comp1 != 7U || base_shape->alpha_ref0 != 0U || base_shape->alpha_ref1 != 0U || op != 0U,
            };
        }

        [[nodiscard]] render::GxBlendMode2D jpa_blend_mode(const smgpc::render::effects::JpcBaseShapeMetadata *base_shape) {
            if (base_shape == nullptr) {
                return render::GxBlendMode2D{.enabled = true};
            }

            const auto config = base_shape->blend_mode_config;
            const auto type = static_cast<std::uint8_t>(config & 0x03U);
            return render::GxBlendMode2D{
                .type = type,
                .src_factor = jpa_blend_factor_from_config(static_cast<std::uint8_t>((config >> 2U) & 0x0fU)),
                .dst_factor = jpa_blend_factor_from_config(static_cast<std::uint8_t>((config >> 6U) & 0x0fU)),
                .op = static_cast<std::uint8_t>((config >> 10U) & 0x0fU),
                .color_update = true,
                .alpha_update = false,
                .enabled = type != 0U,
            };
        }

        [[nodiscard]] bool jpa_depth_test(const smgpc::render::effects::JpcBaseShapeMetadata *base_shape) {
            return base_shape != nullptr && (base_shape->z_mode_config & 0x01U) != 0U;
        }

        [[nodiscard]] bool jpa_depth_write(const smgpc::render::effects::JpcBaseShapeMetadata *base_shape) {
            return base_shape != nullptr && (base_shape->z_mode_config & 0x10U) != 0U;
        }

        [[nodiscard]] render::DepthCompare jpa_depth_compare(const smgpc::render::effects::JpcBaseShapeMetadata *base_shape) {
            return jpa_compare_from_config(base_shape == nullptr ? 7U : static_cast<std::uint8_t>((base_shape->z_mode_config >> 1U) & 0x07U));
        }

        [[nodiscard]] render::GxTevStage2D jpa_tev_stage(const smgpc::render::effects::JpcBaseShapeMetadata *base_shape) {
            constexpr auto COLOR_ARGS = std::array<std::array<std::uint8_t, 4U>, 6U>{
                std::array<std::uint8_t, 4U>{15U, 8U, 12U, 15U},
                std::array<std::uint8_t, 4U>{15U, 2U, 8U, 15U},
                std::array<std::uint8_t, 4U>{2U, 12U, 8U, 15U},
                std::array<std::uint8_t, 4U>{4U, 2U, 8U, 15U},
                std::array<std::uint8_t, 4U>{15U, 8U, 2U, 4U},
                std::array<std::uint8_t, 4U>{15U, 15U, 15U, 2U},
            };
            constexpr auto ALPHA_ARGS = std::array<std::array<std::uint8_t, 4U>, 2U>{
                std::array<std::uint8_t, 4U>{7U, 4U, 1U, 7U},
                std::array<std::uint8_t, 4U>{7U, 7U, 7U, 1U},
            };
            const auto flags = base_shape == nullptr ? 0U : base_shape->flags;
            const auto color_index = static_cast<std::size_t>((flags >> 15U) & 0x07U);
            const auto alpha_index = static_cast<std::size_t>((flags >> 18U) & 0x01U);
            return render::GxTevStage2D{
                .texture_coord_stage = 0U,
                .texture_map_stage = 0U,
                .color_in = color_index < COLOR_ARGS.size() ? COLOR_ARGS[color_index] : COLOR_ARGS.front(),
                .alpha_in = ALPHA_ARGS[alpha_index],
            };
        }

        [[nodiscard]] render::GxTevRegisterColor2D jpa_register_color(std::array<std::uint8_t, 4U> color, float alpha_scale) {
            color[3U] = static_cast<std::uint8_t>(std::clamp(static_cast<float>(color[3U]) * alpha_scale, 0.0F, 255.0F));
            return render::GxTevRegisterColor2D{
                static_cast<std::int16_t>(color[0U]),
                static_cast<std::int16_t>(color[1U]),
                static_cast<std::int16_t>(color[2U]),
                static_cast<std::int16_t>(color[3U]),
            };
        }

        [[nodiscard]] std::array<render::GxTevRegisterColor2D, 4U> jpa_initial_tev_registers(const smgpc::render::effects::JpcBaseShapeMetadata *base_shape,
                                                                                             const smgpc::render::effects::JpcChildShapeMetadata *child_shape,
                                                                                             float alpha_scale) {
            auto registers = std::array<render::GxTevRegisterColor2D, 4U>{};
            const auto prm = child_shape == nullptr ? (base_shape == nullptr ? std::array<std::uint8_t, 4U>{255U, 255U, 255U, 255U} :
                                                                               base_shape->prm_color) :
                                                      child_shape->prm_color;
            const auto env = child_shape == nullptr ? (base_shape == nullptr ? std::array<std::uint8_t, 4U>{0U, 0U, 0U, 255U} :
                                                                               base_shape->env_color) :
                                                      child_shape->env_color;
            registers[0U] = jpa_register_color(prm, alpha_scale);
            registers[1U] = jpa_register_color(env, 1.0F);
            return registers;
        }

        template <std::size_t Size>
        [[nodiscard]] std::array<render::GxMaterialVertex2D, Size>
        jpa_material_vertices(const std::array<render::TexturedVertex2D, Size> &source) {
            auto vertices = std::array<render::GxMaterialVertex2D, Size>{};
            for (auto index = std::size_t{}; index < source.size(); ++index) {
                const auto &vertex = source[index];
                vertices[index] = render::GxMaterialVertex2D{
                    .x = vertex.x,
                    .y = vertex.y,
                    .z = vertex.z,
                    .tex_coords = {{{vertex.u, vertex.v, 1.0F}}},
                    .color = vertex.color,
                };
            }
            return vertices;
        }

        [[nodiscard]] std::uint32_t next_jpa_random_u(std::uint32_t &seed) {
            seed = seed * 0x0019660dU + 0x3c6ef35fU;
            return seed;
        }

        [[nodiscard]] float next_jpa_random_f(std::uint32_t &seed) {
            const auto bits = (next_jpa_random_u(seed) >> 9U) | 0x3f800000U;
            return std::bit_cast<float>(bits) - 1.0F;
        }

        [[nodiscard]] float next_jpa_random_zp(std::uint32_t &seed) {
            const auto value = next_jpa_random_f(seed);
            return value + value - 1.0F;
        }

        [[nodiscard]] float next_jpa_random_zh(std::uint32_t &seed) {
            return next_jpa_random_f(seed) - 0.5F;
        }

        [[nodiscard]] std::int16_t next_jpa_random_ss(std::uint32_t &seed) {
            return static_cast<std::int16_t>(next_jpa_random_u(seed) >> 16U);
        }

        struct JpcVec3 {
            float x = 0.0F;
            float y = 0.0F;
            float z = 0.0F;
        };

        [[nodiscard]] JpcVec3 jpa_add(JpcVec3 lhs, JpcVec3 rhs) {
            return {.x = lhs.x + rhs.x, .y = lhs.y + rhs.y, .z = lhs.z + rhs.z};
        }

        [[nodiscard]] JpcVec3 jpa_scale(JpcVec3 value, float scale) {
            return {.x = value.x * scale, .y = value.y * scale, .z = value.z * scale};
        }

        [[nodiscard]] float jpa_length(JpcVec3 value) {
            return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
        }

        [[nodiscard]] JpcVec3 jpa_set_length(JpcVec3 value, float length) {
            const auto source_length = jpa_length(value);
            if (source_length <= 0.000001F || std::abs(length) <= 0.000001F) {
                return {};
            }

            return jpa_scale(value, length / source_length);
        }

        [[nodiscard]] JpcVec3 jpa_normalized_or(JpcVec3 value, JpcVec3 fallback) {
            const auto source_length = jpa_length(value);
            if (source_length <= 0.000001F) {
                return fallback;
            }

            return jpa_scale(value, 1.0F / source_length);
        }

        [[nodiscard]] float jpa_sin(float angle) {
            constexpr auto kTwoPiOverS16 = 6.28318530717958647692F / 65536.0F;
            return std::sin(angle * kTwoPiOverS16);
        }

        [[nodiscard]] float jpa_cos(float angle) {
            constexpr auto kTwoPiOverS16 = 6.28318530717958647692F / 65536.0F;
            return std::cos(angle * kTwoPiOverS16);
        }

        struct JpcKeyedEmitterDynamics {
            float rate = 0.0F;
            float volume_size = 0.0F;
            float volume_min_radius = 0.0F;
            std::int16_t lifetime = 1;
            float away_from_center_speed = 0.0F;
            float away_from_axis_speed = 0.0F;
            float direction_speed = 0.0F;
            float spread = 0.0F;
            float scale_out = 1.0F;
        };

        struct JpcVolumeSample {
            JpcVec3 position{};
            JpcVec3 velocity_omni{};
            JpcVec3 velocity_axis{};
        };

        [[nodiscard]] float jpa_hermite_interpolation(float frame, const smgpc::render::effects::JpcKeyFrameMetadata &current,
                                                      const smgpc::render::effects::JpcKeyFrameMetadata &next) {
            const auto frame_delta = next.time - current.time;
            if (std::abs(frame_delta) <= 0.000001F) {
                return current.value;
            }

            const auto t = (frame - current.time) / frame_delta;
            const auto t2 = t * t;
            const auto t3 = t2 * t;
            const auto h00 = 2.0F * t3 - 3.0F * t2 + 1.0F;
            const auto h10 = t3 - 2.0F * t2 + t;
            const auto h01 = -2.0F * t3 + 3.0F * t2;
            const auto h11 = t3 - t2;
            return h00 * current.value + h10 * frame_delta * current.tangent_out + h01 * next.value + h11 * frame_delta * next.tangent_in;
        }

        [[nodiscard]] float jpa_key_animation_value(const smgpc::render::effects::JpcKeyBlockMetadata &key_block, float frame) {
            if (key_block.keys.empty()) {
                return 0.0F;
            }
            if (key_block.keys.size() == 1U) {
                return key_block.keys.front().value;
            }

            auto key_frame = frame;
            if (key_block.loop) {
                const auto period = static_cast<int>(key_block.keys.back().time) + 1;
                if (period > 0) {
                    const auto loop_count = static_cast<int>(key_frame) / period;
                    key_frame -= static_cast<float>(loop_count * period);
                }
            }

            if (key_frame < key_block.keys.front().time) {
                return key_block.keys.front().value;
            }
            if (key_block.keys.back().time <= key_frame) {
                return key_block.keys.back().value;
            }

            auto base_index = std::size_t{};
            auto remaining = key_block.keys.size();
            while (remaining > 1U) {
                const auto step = remaining / 2U;
                if (key_frame >= key_block.keys[base_index + step].time) {
                    base_index += step;
                    remaining -= step;
                } else {
                    remaining = step;
                }
            }

            return jpa_hermite_interpolation(key_frame, key_block.keys[base_index], key_block.keys[base_index + 1U]);
        }

        [[nodiscard]] std::int16_t jpa_keyed_lifetime(float value) {
            return static_cast<std::int16_t>(std::clamp(value, 1.0F, 32767.0F));
        }

        [[nodiscard]] JpcKeyedEmitterDynamics jpa_keyed_emitter_dynamics(const smgpc::render::effects::JpcDynamicsBlockMetadata &dynamics,
                                                                         std::span<const smgpc::render::effects::JpcKeyBlockMetadata> key_blocks,
                                                                         float tick) {
            auto keyed = JpcKeyedEmitterDynamics{
                .rate = dynamics.rate,
                .volume_size = static_cast<float>(dynamics.volume_size),
                .volume_min_radius = dynamics.volume_min_radius,
                .lifetime = std::max<std::int16_t>(dynamics.lifetime, 1),
                .away_from_center_speed = dynamics.initial_velocity_omni,
                .away_from_axis_speed = dynamics.initial_velocity_axis,
                .direction_speed = dynamics.initial_velocity_direction,
                .spread = dynamics.spread,
            };

            for (auto key_index = key_blocks.size(); key_index > 0U; --key_index) {
                const auto &key_block = key_blocks[key_index - 1U];
                const auto value = jpa_key_animation_value(key_block, tick);
                switch (key_block.id) {
                case 0U:
                    keyed.rate = value;
                    break;
                case 1U:
                    keyed.volume_size = value;
                    break;
                case 3U:
                    keyed.volume_min_radius = value;
                    break;
                case 4U:
                    keyed.lifetime = jpa_keyed_lifetime(value);
                    break;
                case 6U:
                    keyed.away_from_center_speed = value;
                    break;
                case 7U:
                    keyed.away_from_axis_speed = value;
                    break;
                case 8U:
                    keyed.direction_speed = value;
                    break;
                case 9U:
                    keyed.spread = value;
                    break;
                case 10U:
                    keyed.scale_out = value;
                    break;
                default:
                    break;
                }
            }

            return keyed;
        }

        [[nodiscard]] std::uint16_t jpa_particle_lifetime(const smgpc::render::effects::JpcDynamicsBlockMetadata &dynamics, std::int16_t source_lifetime,
                                                          std::uint32_t &seed) {
            const auto clamped_lifetime = std::max<std::int16_t>(source_lifetime, 1);
            const auto lifetime = (1.0F - dynamics.lifetime_random * next_jpa_random_f(seed)) * static_cast<float>(clamped_lifetime);
            return static_cast<std::uint16_t>(std::max(1.0F, std::floor(lifetime)));
        }

        [[nodiscard]] JpcVolumeSample jpa_particle_volume_sample(const smgpc::render::effects::JpcDynamicsBlockMetadata &dynamics,
                                                                 const JpcKeyedEmitterDynamics &keyed_dynamics, std::uint32_t &seed,
                                                                 int emitted_index, int emit_count) {
            constexpr auto kVolumeCube = std::uint8_t{0U};
            constexpr auto kVolumeSphere = std::uint8_t{1U};
            constexpr auto kVolumeCylinder = std::uint8_t{2U};
            constexpr auto kVolumeTorus = std::uint8_t{3U};
            constexpr auto kVolumePoint = std::uint8_t{4U};
            constexpr auto kVolumeCircle = std::uint8_t{5U};
            constexpr auto kVolumeLine = std::uint8_t{6U};

            const auto volume_size = keyed_dynamics.volume_size;
            const auto volume_min_radius = std::clamp(keyed_dynamics.volume_min_radius, 0.0F, 1.0F);
            auto sample = JpcVolumeSample{};
            const auto fixed_step = [&] {
                if (!dynamics.fixed_interval || emit_count <= 1) {
                    return 0.0F;
                }
                return static_cast<float>(emitted_index) / static_cast<float>(emit_count - 1);
            };
            const auto radius_sample = [&] {
                auto value = next_jpa_random_f(seed);
                if (dynamics.fixed_density) {
                    value = 1.0F - value * value;
                }
                return volume_size * (volume_min_radius + value * (1.0F - volume_min_radius));
            };

            switch (dynamics.volume_type) {
            case kVolumeCube:
                sample.position = {
                    .x = next_jpa_random_zh(seed) * volume_size,
                    .y = next_jpa_random_zh(seed) * volume_size,
                    .z = next_jpa_random_zh(seed) * volume_size,
                };
                sample.velocity_omni = sample.position;
                sample.velocity_axis = {.x = sample.position.x, .y = 0.0F, .z = sample.position.z};
                break;
            case kVolumeSphere: {
                const auto phi = static_cast<float>(next_jpa_random_ss(seed)) * 0.5F;
                const auto theta = keyed_dynamics.spread == 0.0F ? dynamics.volume_sweep * static_cast<float>(next_jpa_random_ss(seed)) : dynamics.volume_sweep * static_cast<float>(next_jpa_random_ss(seed));
                auto radius_value = next_jpa_random_f(seed);
                if (dynamics.fixed_density) {
                    radius_value = 1.0F - radius_value * radius_value * radius_value;
                }
                const auto radius = volume_size * (volume_min_radius + radius_value * (1.0F - volume_min_radius));
                sample.position = {
                    .x = radius * jpa_cos(phi) * jpa_sin(theta),
                    .y = -radius * jpa_sin(phi),
                    .z = radius * jpa_cos(phi) * jpa_cos(theta),
                };
                sample.velocity_omni = sample.position;
                sample.velocity_axis = {.x = sample.position.x, .y = 0.0F, .z = sample.position.z};
                break;
            }
            case kVolumeCylinder: {
                const auto theta = dynamics.volume_sweep * static_cast<float>(next_jpa_random_ss(seed));
                const auto radius = radius_sample();
                sample.position = {
                    .x = radius * jpa_sin(theta),
                    .y = volume_size * next_jpa_random_zp(seed),
                    .z = radius * jpa_cos(theta),
                };
                sample.velocity_omni = sample.position;
                sample.velocity_axis = {.x = sample.position.x, .y = 0.0F, .z = sample.position.z};
                break;
            }
            case kVolumeTorus: {
                const auto theta = dynamics.volume_sweep * static_cast<float>(next_jpa_random_ss(seed));
                const auto phi = static_cast<float>(next_jpa_random_ss(seed));
                const auto radius = volume_size * volume_min_radius;
                sample.velocity_axis = {
                    .x = radius * jpa_sin(theta) * jpa_cos(phi),
                    .y = radius * jpa_sin(phi),
                    .z = radius * jpa_cos(theta) * jpa_cos(phi),
                };
                sample.position = {
                    .x = sample.velocity_axis.x + volume_size * jpa_sin(theta),
                    .y = sample.velocity_axis.y,
                    .z = sample.velocity_axis.z + volume_size * jpa_cos(theta),
                };
                sample.velocity_omni = sample.position;
                break;
            }
            case kVolumePoint:
                sample.position = {};
                sample.velocity_omni = {
                    .x = next_jpa_random_zh(seed),
                    .y = next_jpa_random_zh(seed),
                    .z = next_jpa_random_zh(seed),
                };
                sample.velocity_axis = {.x = sample.velocity_omni.x, .y = 0.0F, .z = sample.velocity_omni.z};
                break;
            case kVolumeCircle: {
                const auto theta = dynamics.fixed_interval && emit_count > 0 ? dynamics.volume_sweep * 65536.0F * static_cast<float>(emitted_index) / static_cast<float>(emit_count) : dynamics.volume_sweep * static_cast<float>(next_jpa_random_ss(seed));
                const auto radius = radius_sample();
                sample.position = {.x = radius * jpa_sin(theta), .y = 0.0F, .z = radius * jpa_cos(theta)};
                sample.velocity_omni = sample.position;
                sample.velocity_axis = {.x = sample.position.x, .y = 0.0F, .z = sample.position.z};
                break;
            }
            case kVolumeLine:
            default: {
                const auto step = fixed_step();
                const auto z = dynamics.fixed_interval && emit_count > 1 ? volume_size * (step - 0.5F) : volume_size * next_jpa_random_zh(seed);
                sample.position = {.x = 0.0F, .y = 0.0F, .z = z};
                sample.velocity_omni = {.x = 0.0F, .y = 0.0F, .z = z};
                sample.velocity_axis = sample.velocity_omni;
                break;
            }
            }

            return sample;
        }

        [[nodiscard]] JpcVec3 jpa_direction_velocity(const smgpc::render::effects::JpcDynamicsBlockMetadata &dynamics,
                                                     const JpcKeyedEmitterDynamics &keyed_dynamics, std::uint32_t &seed) {
            if (std::abs(keyed_dynamics.direction_speed) <= 0.000001F) {
                return {};
            }

            const auto base_direction = jpa_normalized_or(
                {.x = dynamics.emitter_direction.x, .y = dynamics.emitter_direction.y, .z = dynamics.emitter_direction.z},
                {.x = 0.0F, .y = 0.0F, .z = 1.0F});
            const auto angle_y = next_jpa_random_zp(seed) * 32768.0F * keyed_dynamics.spread;
            const auto angle_z = static_cast<float>(next_jpa_random_ss(seed));
            const auto side = JpcVec3{.x = jpa_sin(angle_y) * jpa_cos(angle_z), .y = jpa_sin(angle_y) * jpa_sin(angle_z), .z = 0.0F};
            const auto cone = jpa_normalized_or(jpa_add(base_direction, side), base_direction);
            return jpa_scale(cone, keyed_dynamics.direction_speed);
        }

        [[nodiscard]] JpcVec3 jpa_parent_velocity(const smgpc::render::effects::JpcDynamicsBlockMetadata &dynamics,
                                                  const JpcKeyedEmitterDynamics &keyed_dynamics, const JpcVolumeSample &volume_sample,
                                                  std::uint32_t &seed) {
            const auto vel_omni = jpa_set_length(volume_sample.velocity_omni, keyed_dynamics.away_from_center_speed);
            const auto vel_axis = jpa_set_length(volume_sample.velocity_axis, keyed_dynamics.away_from_axis_speed);
            const auto vel_dir = jpa_direction_velocity(dynamics, keyed_dynamics, seed);
            const auto vel_random = JpcVec3{
                .x = dynamics.initial_velocity_random * next_jpa_random_zh(seed),
                .y = dynamics.initial_velocity_random * next_jpa_random_zh(seed),
                .z = dynamics.initial_velocity_random * next_jpa_random_zh(seed),
            };
            const auto ratio = next_jpa_random_zp(seed) * dynamics.initial_velocity_ratio + 1.0F;
            return jpa_scale(jpa_add(jpa_add(vel_omni, vel_axis), jpa_add(vel_dir, vel_random)), ratio);
        }

        [[nodiscard]] JpcVec3 jpa_child_velocity(const JpcEffectParticleInstance &parent,
                                                 const smgpc::render::effects::JpcChildShapeMetadata &child_shape,
                                                 std::uint32_t &seed) {
            const auto base_speed = child_shape.base_velocity * (child_shape.base_velocity_random * next_jpa_random_zp(seed) + 1.0F);
            const auto random_velocity = jpa_set_length(
                {
                    .x = next_jpa_random_zp(seed),
                    .y = next_jpa_random_zp(seed),
                    .z = next_jpa_random_zp(seed),
                },
                base_speed);
            const auto inherited = JpcVec3{
                .x = parent.velocity_x * child_shape.velocity_inherit_rate,
                .y = parent.velocity_y * child_shape.velocity_inherit_rate,
                .z = parent.velocity_z * child_shape.velocity_inherit_rate,
            };
            return jpa_add(random_velocity, inherited);
        }

    }  // namespace

    DvdFileSystemService::DvdFileSystemService(std::filesystem::path root) : _root(std::move(root)) {
    }

    void DvdFileSystemService::begin_frame(std::uint64_t frame_index) {
        _frame_index = frame_index;
        complete_ready_async_reads();
    }

    const std::filesystem::path &DvdFileSystemService::root() const {
        return _root;
    }

    std::string DvdFileSystemService::normalize_disc_path_string(std::string_view disc_path) const {
        const auto normalized = normalize_disc_path(disc_path);
        const auto key = entry_key(normalized);
        return key.empty() ? std::string("/") : "/" + key;
    }

    std::filesystem::path DvdFileSystemService::resolve(std::string_view disc_path) const {
        const auto normalized = normalize_disc_path(disc_path);
        return std::filesystem::path(normalize_disc_path_string(normalized.generic_string()));
    }

    bool DvdFileSystemService::exists(std::string_view disc_path) const {
        return entry_metadata(disc_path).has_value();
    }

    s32 DvdFileSystemService::entry_num(std::string_view disc_path) const {
        const auto normalized = normalize_disc_path_string(disc_path);
        return DVDConvertPathToEntrynum(normalized.c_str());
    }

    std::optional<DvdEntryMetadata> DvdFileSystemService::entry_metadata(std::string_view disc_path) const {
        const auto normalized = normalize_disc_path_string(disc_path);
        const auto entry = DVDConvertPathToEntrynum(normalized.c_str());
        if (entry < 0) {
            return std::nullopt;
        }

        auto file_info = DVDFileInfo{};
        if (DVDOpen(normalized.c_str(), &file_info)) {
            const auto length = file_info.length;
            (void)DVDClose(&file_info);
            return DvdEntryMetadata{
                .entry_num = entry,
                .disc_path = normalized,
                .resolved_path = normalized,
                .is_directory = false,
                .length = length,
            };
        }

        auto dir = DVDDir{};
        if (DVDOpenDir(normalized.c_str(), &dir)) {
            (void)DVDCloseDir(&dir);
            return DvdEntryMetadata{
                .entry_num = entry,
                .disc_path = normalized,
                .resolved_path = normalized,
                .is_directory = true,
                .length = 0U,
            };
        }

        return std::nullopt;
    }

    std::optional<DvdEntryMetadata> DvdFileSystemService::entry_metadata(s32 entry_num) const {
        if (entry_num < 0) {
            return std::nullopt;
        }

        auto file_info = DVDFileInfo{};
        if (DVDFastOpen(entry_num, &file_info)) {
            const auto length = file_info.length;
            (void)DVDClose(&file_info);
            return DvdEntryMetadata{
                .entry_num = entry_num,
                .disc_path = std::to_string(entry_num),
                .resolved_path = std::to_string(entry_num),
                .is_directory = false,
                .length = length,
            };
        }

        auto dir = DVDDir{};
        if (DVDFastOpenDir(entry_num, &dir)) {
            (void)DVDCloseDir(&dir);
            return DvdEntryMetadata{
                .entry_num = entry_num,
                .disc_path = std::to_string(entry_num),
                .resolved_path = std::to_string(entry_num),
                .is_directory = true,
                .length = 0U,
            };
        }

        return std::nullopt;
    }

    std::vector<DvdDirectoryEntry> DvdFileSystemService::directory_entries(std::string_view disc_path) const {
        const auto normalized = normalize_disc_path_string(disc_path);
        auto dir = DVDDir{};
        if (!DVDOpenDir(normalized.c_str(), &dir)) {
            return {};
        }

        auto entries = std::vector<DvdDirectoryEntry>{};
        auto dir_entry = DVDDirEntry{};
        while (DVDReadDir(&dir, &dir_entry)) {
            entries.push_back(DvdDirectoryEntry{
                .entry_num = static_cast<s32>(dir_entry.entryNum),
                .disc_path = (std::filesystem::path(normalized) / (dir_entry.name != nullptr ? dir_entry.name : "")).generic_string(),
                .name = dir_entry.name != nullptr ? dir_entry.name : "",
                .is_directory = dir_entry.isDir != FALSE,
            });
        }
        (void)DVDCloseDir(&dir);

        return entries;
    }

    std::optional<std::filesystem::path> DvdFileSystemService::find_first(std::initializer_list<std::filesystem::path> candidates) const {
        for (const auto &candidate : candidates) {
            const auto path = normalize_disc_path_string(candidate.generic_string());
            if (exists(path)) {
                return std::filesystem::path(path);
            }
        }

        return std::nullopt;
    }

    std::optional<std::filesystem::path> DvdFileSystemService::find_layout_archive(std::string_view layout_name) const {
        const auto archive_name = std::string(layout_name) + ".arc";
        return find_first({
            std::filesystem::path("KrKorean") / "LayoutData" / archive_name,
            std::filesystem::path("LayoutData") / archive_name,
        });
    }

    std::optional<std::filesystem::path> DvdFileSystemService::find_object_archive(std::string_view object_name) const {
        const auto archive_name = std::string(object_name) + ".arc";
        return find_first({
            std::filesystem::path("ObjectData") / archive_name,
        });
    }

    std::vector<std::uint8_t> DvdFileSystemService::read_file(std::string_view disc_path) const {
        return read_file_range(disc_path, 0U, std::numeric_limits<std::size_t>::max(), 0);
    }

    std::vector<std::uint8_t> DvdFileSystemService::read_file_range(std::string_view disc_path, std::size_t offset, std::size_t length,
                                                                    s32 priority) const {
        const auto entry = entry_metadata(disc_path);
        if (!entry.has_value() || entry->is_directory) {
            throw std::runtime_error("Cannot open DVD file " + std::string(disc_path));
        }
        if (offset > entry->length) {
            throw std::runtime_error("DVD read offset is outside file " + std::string(disc_path));
        }

        const auto read_size = std::min(length, entry->length - offset);
        auto bytes = std::vector<std::uint8_t>(read_size);
        auto file_info = DVDFileInfo{};
        if (!DVDOpen(entry->disc_path.c_str(), &file_info)) {
            throw std::runtime_error("Cannot open DVD file " + entry->disc_path);
        }
        const auto result = DVDReadPrio(&file_info, bytes.data(), static_cast<s32>(bytes.size()), static_cast<s32>(offset), priority);
        (void)DVDClose(&file_info);
        if (result < 0) {
            throw std::runtime_error("Cannot read DVD file " + entry->disc_path);
        }
        bytes.resize(static_cast<std::size_t>(result));

        _file_read_trace.push_back(DvdFileReadTrace{
            .requested_path = std::string(disc_path),
            .disc_path = entry->disc_path,
            .resolved_path = entry->resolved_path,
            .entry_num = entry->entry_num,
            .byte_count = bytes.size(),
            .offset = offset,
            .priority = priority,
        });

        return bytes;
    }

    std::uint64_t DvdFileSystemService::submit_async_read(std::string_view disc_path, DVDFileInfo *file_info, void *destination,
                                                          std::size_t length, std::size_t offset, s32 priority, DVDCallback callback,
                                                          std::uint64_t delay_frames) {
        const auto entry = entry_metadata(disc_path);
        if (!entry.has_value() || entry->is_directory) {
            throw std::runtime_error("Cannot queue DVD read for " + std::string(disc_path));
        }

        auto request = DvdAsyncReadRequest{};
        request.id = _next_async_read_id++;
        request.disc_path = entry->disc_path;
        request.entry_num = entry->entry_num;
        request.file_info = file_info;
        request.destination = destination;
        request.length = length;
        request.offset = offset;
        request.priority = priority;
        request.submitted_frame = _frame_index;
        request.completion_frame = _frame_index + delay_frames;
        request.callback = callback;
        _async_read_trace.push_back(request);
        complete_ready_async_reads();
        return request.id;
    }

    smgpc::resource::RarcArchive &DvdFileSystemService::archive(std::string_view disc_path) {
        return archive_for_path_with_request(std::filesystem::path(normalize_disc_path_string(disc_path)), disc_path);
    }

    smgpc::resource::RarcArchive &DvdFileSystemService::archive_for_path(const std::filesystem::path &path) {
        return archive_for_path_with_request(path, path.generic_string());
    }

    smgpc::resource::RarcArchive &DvdFileSystemService::archive_for_path_with_request(const std::filesystem::path &path, std::string_view requested_path) {
        const auto key = archive_cache_key_for_path(path);
        if (auto it = _archives.find(key); it != _archives.end()) {
            _archive_load_trace.push_back(DvdArchiveLoadTrace{
                .requested_path = std::string(requested_path),
                .resolved_path = key,
                .cache_hit = true,
                .load_count = archive_load_count_for_path(path),
                .cached_archive_count = _archives.size(),
                .resource_count = it->second->entries().size(),
            });
            return *it->second;
        }

        auto archive = std::make_unique<smgpc::resource::RarcArchive>(smgpc::resource::RarcArchive::from_bytes(read_file(key)));
        auto [it, inserted] = _archives.emplace(key, std::move(archive));
        if (inserted) {
            ++_archive_load_counts[key];
        }
        _archive_load_trace.push_back(DvdArchiveLoadTrace{
            .requested_path = std::string(requested_path),
            .resolved_path = key,
            .cache_hit = false,
            .load_count = archive_load_count_for_path(path),
            .cached_archive_count = _archives.size(),
            .resource_count = it->second->entries().size(),
        });

        return *it->second;
    }

    std::size_t DvdFileSystemService::archive_load_count(std::string_view disc_path) const {
        return archive_load_count_for_path(resolve(disc_path));
    }

    std::size_t DvdFileSystemService::archive_load_count_for_path(const std::filesystem::path &path) const {
        const auto key = archive_cache_key_for_path(path);
        if (auto it = _archive_load_counts.find(key); it != _archive_load_counts.end()) {
            return it->second;
        }

        return 0U;
    }

    std::size_t DvdFileSystemService::cached_archive_count() const {
        return _archives.size();
    }

    std::span<const DvdFileReadTrace> DvdFileSystemService::file_read_trace() const {
        return _file_read_trace;
    }

    std::span<const DvdAsyncReadRequest> DvdFileSystemService::async_read_trace() const {
        return _async_read_trace;
    }

    std::span<const DvdArchiveLoadTrace> DvdFileSystemService::archive_load_trace() const {
        return _archive_load_trace;
    }

    void DvdFileSystemService::clear_trace() {
        _file_read_trace.clear();
        _async_read_trace.clear();
        _archive_load_trace.clear();
    }

    std::filesystem::path DvdFileSystemService::normalize_disc_path(std::string_view disc_path) const {
        auto text = std::string(disc_path);
        std::ranges::replace(text, '\\', '/');

        while (!text.empty() && text.front() == '/') {
            text.erase(text.begin());
        }
        while (text.starts_with("./")) {
            text.erase(0U, 2U);
        }
        if (text.starts_with("files/")) {
            text.erase(0U, 6U);
        }

        auto parts = std::vector<std::filesystem::path>{};
        for (const auto &component : std::filesystem::path(text)) {
            const auto part = component.generic_string();
            if (part.empty() || part == ".") {
                continue;
            }
            if (part == "..") {
                if (parts.empty()) {
                    throw std::runtime_error("DVD path escapes disc root: " + std::string(disc_path));
                }
                parts.pop_back();
                continue;
            }

            parts.push_back(component);
        }

        auto normalized = std::filesystem::path();
        for (const auto &part : parts) {
            normalized /= part;
        }

        return normalized;
    }

    void DvdFileSystemService::complete_ready_async_reads() {
        for (auto &request : _async_read_trace) {
            if (request.completed || request.completion_frame > _frame_index) {
                continue;
            }

            auto result = s32{-1};
            try {
                const auto bytes = read_file_range(request.disc_path, request.offset, request.length, request.priority);
                if (!bytes.empty() && request.destination != nullptr) {
                    std::memcpy(request.destination, bytes.data(), bytes.size());
                }
                result = static_cast<s32>(bytes.size());
                if (request.file_info != nullptr) {
                    request.file_info->cb.state = DVD_STATE_END;
                    request.file_info->cb.transferredSize += static_cast<u32>(bytes.size());
                }
            } catch (const std::exception &) {
                result = -1;
                if (request.file_info != nullptr) {
                    request.file_info->cb.state = DVD_STATE_FATAL_ERROR;
                }
            }

            request.result = result;
            request.completed = true;
            if (request.callback != nullptr) {
                request.callback(result, request.file_info);
            }
        }
    }

    void DvdFileSystemService::ensure_entry_table() const {
        if (_entry_table_initialized) {
            return;
        }

        _entry_table.clear();
        _entry_num_by_disc_path.clear();

        const auto add_entry = [this](std::filesystem::path disc_path, const std::filesystem::path &resolved_path,
                                      bool is_directory) -> s32 {
            const auto entry_num = static_cast<s32>(_entry_table.size());
            auto length = std::uintmax_t{};
            if (!is_directory) {
                std::error_code error{};
                length = std::filesystem::file_size(resolved_path, error);
                if (error) {
                    length = 0U;
                }
            }

            const auto key = entry_key(disc_path);
            _entry_num_by_disc_path[key] = entry_num;
            _entry_table.push_back(DvdEntryMetadata{
                .entry_num = entry_num,
                .disc_path = key.empty() ? std::string("/") : "/" + key,
                .resolved_path = weakly_canonical_or_normal(resolved_path).generic_string(),
                .is_directory = is_directory,
                .length = static_cast<std::size_t>(std::min<std::uintmax_t>(length, std::numeric_limits<std::size_t>::max())),
            });
            return entry_num;
        };

        add_entry({}, _root, true);

        const auto visit_directory = [&](const auto &self, const std::filesystem::path &relative_path) -> void {
            const auto absolute_path = relative_path.empty() ? _root : _root / relative_path;
            auto children = std::vector<std::filesystem::directory_entry>{};
            std::error_code error{};
            auto iter = std::filesystem::directory_iterator(absolute_path, std::filesystem::directory_options::skip_permission_denied, error);
            if (error) {
                return;
            }
            for (const auto &child : iter) {
                std::error_code status_error{};
                if (child.is_directory(status_error) || child.is_regular_file(status_error)) {
                    children.push_back(child);
                }
            }
            std::ranges::sort(children, [](const auto &lhs, const auto &rhs) {
                return lhs.path().filename().generic_string() < rhs.path().filename().generic_string();
            });

            for (const auto &child : children) {
                std::error_code status_error{};
                const auto is_directory = child.is_directory(status_error);
                const auto child_relative_path = relative_path / child.path().filename();
                add_entry(child_relative_path, child.path(), is_directory);
                if (is_directory) {
                    self(self, child_relative_path);
                }
            }
        };
        visit_directory(visit_directory, {});
        _entry_table_initialized = true;
    }

    std::string DvdFileSystemService::entry_key(const std::filesystem::path &disc_path) {
        auto key = disc_path.lexically_normal().generic_string();
        while (!key.empty() && key.front() == '/') {
            key.erase(key.begin());
        }
        if (key == ".") {
            key.clear();
        }
        return key;
    }

    std::string DvdFileSystemService::archive_cache_key_for_path(const std::filesystem::path &path) const {
        return normalize_disc_path_string(path.generic_string());
    }

    std::string DvdFileSystemService::archive_cache_key(std::string_view disc_path) const {
        return archive_cache_key_for_path(resolve(disc_path));
    }

    void AudioEventService::begin_frame(std::uint64_t frame_index) {
        _frame_index = frame_index;
    }

    void AudioEventService::start_stage_bgm(std::string_view name) {
        _stage_bgm_requested = true;
        _stage_bgm_start_frame = _frame_index;
        _stage_bgm_name = name;
        push_event(AudioEventKind::StageBgmStart, name);
    }

    void AudioEventService::unlock_stage_bgm() {
        _stage_bgm_unlocked = true;
        push_event(AudioEventKind::StageBgmUnlock, {});
    }

    void AudioEventService::stop_stage_bgm(s32 fade_frames) {
        const auto stopped_name = _stage_bgm_name;
        _stage_bgm_requested = false;
        _stage_bgm_name.clear();
        push_event(AudioEventKind::StageBgmStop, stopped_name, fade_frames);
    }

    void AudioEventService::set_stage_bgm_state(s32 state, u32 change_frames) {
        _stage_bgm_state = state;
        _stage_bgm_state_change_frames = change_frames;
        push_event(AudioEventKind::StageBgmStateChange, _stage_bgm_name, 0, state, change_frames);
    }

    void AudioEventService::start_system_sound(std::string_view name) {
        push_event(AudioEventKind::SystemSoundStart, name);
    }

    void AudioEventService::stop_system_sound(std::string_view name, u32 delay_frames) {
        push_event(AudioEventKind::SystemSoundStop, name, 0, 0, 0U, delay_frames);
    }

    void AudioEventService::start_system_level_sound(std::string_view name) {
        push_event(AudioEventKind::SystemLevelSoundStart, name);
    }

    void AudioEventService::submit_level_sound() {
        push_event(AudioEventKind::LevelSoundSubmit, {});
    }

    void AudioEventService::permit_level_sound() {
        push_event(AudioEventKind::LevelSoundPermit, {});
    }

    void AudioEventService::start_atmosphere_sound(std::string_view name) {
        push_event(AudioEventKind::AtmosphereSoundStart, name);
    }

    void AudioEventService::start_system_me(std::string_view name) {
        push_event(AudioEventKind::SystemMEStart, name);
    }

    void AudioEventService::start_controller_speaker_sound(std::string_view name) {
        push_event(AudioEventKind::ControllerSpeakerSoundStart, name);
    }

    bool AudioEventService::is_stage_bgm_prepared() const {
        return _stage_bgm_requested && _frame_index > _stage_bgm_start_frame;
    }

    bool AudioEventService::is_stage_bgm_unlocked() const {
        return _stage_bgm_unlocked;
    }

    std::string_view AudioEventService::current_stage_bgm_name() const {
        return _stage_bgm_name;
    }

    s32 AudioEventService::stage_bgm_state() const {
        return _stage_bgm_state;
    }

    u32 AudioEventService::stage_bgm_state_change_frames() const {
        return _stage_bgm_state_change_frames;
    }

    std::span<const AudioEvent> AudioEventService::events() const {
        return _events;
    }

    void AudioEventService::push_event(AudioEventKind kind, std::string_view name, s32 fade_frames, s32 state, u32 change_frames,
                                       u32 delay_frames) {
        _events.push_back(AudioEvent{
            .kind = kind,
            .name = std::string(name),
            .fade_frames = fade_frames,
            .state = state,
            .change_frames = change_frames,
            .delay_frames = delay_frames,
            .frame_index = _frame_index,
        });
    }

    void EffectService::load_resources(const smgpc::resource::RarcArchive &archive) {
        _resource_library = smgpc::render::effects::EffectResourceLibrary::from_archive(archive);
    }

    void EffectService::begin_frame(std::uint64_t frame_index) {
        _frame_index = frame_index;
        advance_effects_to_frame(frame_index);
#ifndef NDEBUG
        _draw_packets.clear();
#endif
    }

    void EffectService::register_keeper(EffectKeeperHostKind host_kind, std::string_view host_name, s32 requested_capacity,
                                        std::string_view resource_group_name, bool sort_enabled, const void *host_identity) {
        if (host_name.empty()) {
            return;
        }

        auto keeper = EffectKeeperRegistration{
            .host_kind = host_kind,
            .host_identity = host_identity,
            .host_name = std::string(host_name),
            .resource_group_name = std::string(resource_group_name),
            .requested_capacity = requested_capacity,
            .sort_enabled = sort_enabled,
            .frame_index = _frame_index,
        };
        if (host_identity != nullptr) {
            _registered_keeper_instances[host_identity] = keeper;
        } else {
            _registered_keepers[std::string(host_name)] = keeper;
        }
    }

    void EffectService::unregister_keeper(std::string_view host_name, const void *host_identity) {
        release_host_state(host_name, host_identity);
    }

    void EffectService::release_host_state(std::string_view host_name, const void *host_identity) noexcept {
        std::erase_if(_active_effects, [host_name, host_identity](const auto &active) {
            return effect_host_matches(active, host_name, host_identity);
        });
        if (host_identity != nullptr) {
            _registered_keeper_instances.erase(host_identity);
            _host_binding_instances.erase(host_identity);
            return;
        }
        if (const auto keeper = _registered_keepers.find(host_name); keeper != _registered_keepers.end()) {
            _registered_keepers.erase(keeper);
        }
        if (const auto binding = _host_bindings.find(host_name); binding != _host_bindings.end()) {
            _host_bindings.erase(binding);
        }
    }

    void EffectService::bind_host_transform(EffectKeeperHostKind host_kind, std::string_view host_name, EffectHostBindingSource source,
                                            const std::array<float, 12U> &matrix, bool host_dead, const void *host_identity) {
        if (host_name.empty()) {
            return;
        }

        auto binding = EffectHostBinding{
            .host_kind = host_kind,
            .source = source,
            .host_identity = host_identity,
            .host_name = std::string(host_name),
            .matrix = matrix,
            .translation = effect_host_translation(matrix),
            .host_dead = host_dead,
            .frame_index = _frame_index,
        };
        if (host_identity != nullptr) {
            _host_binding_instances[host_identity] = binding;
        } else {
            _host_bindings[std::string(host_name)] = binding;
        }
        for (auto &active : _active_effects) {
            if (effect_host_matches(active, host_name, host_identity)) {
                active.host_binding = binding;
            }
        }
    }

    void EffectService::unbind_host_transform(std::string_view host_name, const void *host_identity) {
        if (host_identity != nullptr) {
            _host_binding_instances.erase(host_identity);
        } else if (const auto it = _host_bindings.find(host_name); it != _host_bindings.end()) {
            _host_bindings.erase(it);
        }
        for (auto &active : _active_effects) {
            if (effect_host_matches(active, host_name, host_identity)) {
                active.host_binding = std::nullopt;
            }
        }
    }

    void EffectService::emit(std::string_view actor_name, std::string_view effect_name, const void *host_identity) {
        const auto keeper = registered_keeper(actor_name, host_identity);
        const auto binding = host_binding(actor_name, host_identity);
        auto resolved = resolve(actor_name, effect_name, host_identity);
        const auto found = std::ranges::find_if(_active_effects, [actor_name, effect_name, host_identity](const auto &active) {
            return effect_host_matches(active, actor_name, host_identity) && active.effect_name == effect_name;
        });
        if (found == _active_effects.end()) {
            auto emitters = create_emitters(resolved);
            auto &active = _active_effects.emplace_back(ActiveEffectInstance{
                .host_identity = host_identity,
                .actor_name = std::string(actor_name),
                .effect_name = std::string(effect_name),
                .start_frame_index = _frame_index,
                .keeper = keeper,
                .host_binding = binding,
                .resolved_resources = resolved,
                .emitters = std::move(emitters),
            });
            for (auto emitter_index = std::size_t{}; emitter_index < active.emitters.size() && emitter_index < active.resolved_resources.size();
                 ++emitter_index) {
                advance_emitter_to_frame(active.emitters[emitter_index], active.resolved_resources[emitter_index], _frame_index);
            }
        } else {
            found->keeper = keeper;
            found->host_binding = binding;
            found->resolved_resources = resolved;
            found->emitters = create_emitters(found->resolved_resources);
            for (auto emitter_index = std::size_t{}; emitter_index < found->emitters.size() && emitter_index < found->resolved_resources.size();
                 ++emitter_index) {
                advance_emitter_to_frame(found->emitters[emitter_index], found->resolved_resources[emitter_index], _frame_index);
            }
        }

        _events.push_back(EffectEvent{
            .kind = EffectEventKind::Emit,
            .host_identity = host_identity,
            .actor_name = std::string(actor_name),
            .effect_name = std::string(effect_name),
            .frame_index = _frame_index,
            .keeper = keeper,
            .resolved_resources = std::move(resolved),
        });
    }

    void EffectService::delete_effect(std::string_view actor_name, std::string_view effect_name, const void *host_identity) {
        const auto keeper = registered_keeper(actor_name, host_identity);
        if (!keeper.has_value()) {
            throw std::logic_error("Effect deletion requires a registered effect keeper.");
        }
        std::erase_if(_active_effects, [actor_name, effect_name, host_identity](const auto &active) {
            return effect_host_matches(active, actor_name, host_identity) && active.effect_name == effect_name;
        });

        _events.push_back(EffectEvent{
            .kind = EffectEventKind::Delete,
            .host_identity = host_identity,
            .actor_name = std::string(actor_name),
            .effect_name = std::string(effect_name),
            .frame_index = _frame_index,
            .keeper = keeper,
            .resolved_resources = resolve(actor_name, effect_name, host_identity),
        });
    }

    void EffectService::delete_all(std::string_view actor_name, const void *host_identity) {
        const auto keeper = registered_keeper(actor_name, host_identity);
        if (!keeper.has_value()) {
            throw std::logic_error("Effect deletion requires a registered effect keeper.");
        }
        std::erase_if(_active_effects, [actor_name, host_identity](const auto &active) {
            return effect_host_matches(active, actor_name, host_identity);
        });
        _events.push_back(EffectEvent{
            .kind = EffectEventKind::DeleteAll,
            .host_identity = host_identity,
            .actor_name = std::string(actor_name),
            .effect_name = {},
            .frame_index = _frame_index,
            .keeper = keeper,
            .resolved_resources = {},
        });
    }

    void EffectService::draw(s32 draw_type, const smgpc::camera::CameraPose *camera_pose) {
        auto &renderer = render::current_aurora_renderer();
        const auto world_draw = effect_draw_type_uses_world_camera(draw_type) && camera_pose != nullptr;
        for (const auto &active : _active_effects) {
            for (auto resource_index = std::size_t{}; resource_index < active.resolved_resources.size(); ++resource_index) {
                const auto &resource = active.resolved_resources[resource_index];
                if (!effect_draw_type_accepts_order(draw_type, resource.auto_effect_draw_order)) {
                    continue;
                }
                if (resource_index >= active.emitters.size()) {
                    continue;
                }
                const auto &emitter = active.emitters[resource_index];
                if (emitter.particles.empty()) {
                    continue;
                }

                const auto host_translation = active.host_binding.has_value() ? active.host_binding->translation : std::array<float, 3U>{};
                const auto *primary_texture = primary_effect_texture(resource);
                const auto *child_texture = child_effect_texture(resource);
                if (primary_texture == nullptr || child_texture == nullptr) {
                    continue;
                }

                const auto base_size = effect_billboard_size(*primary_texture, resource);
                const auto *base_shape = resource.resource != nullptr && resource.resource->base_shape.has_value() ? &*resource.resource->base_shape : nullptr;
                const auto *child_shape =
                    resource.resource != nullptr && resource.resource->child_shape.has_value() ? &*resource.resource->child_shape : nullptr;
                for (const auto &particle : emitter.particles) {
                    const auto shape_type = jpa_particle_shape_type(particle.child, base_shape, child_shape);
                    if (!shape_type.has_value()) {
                        continue;
                    }
                    const auto packet_path =
                        smgpc::render::effects::jpc_particle_packet_path(world_draw, *shape_type);
                    if (!packet_path.has_value()) {
                        continue;
                    }

                    const auto &texture = particle.child ? *child_texture : *primary_texture;
                    auto texture_handle = texture_handle_for(texture);
                    if (!texture_handle.is_valid()) {
                        continue;
                    }

                    const auto half_size_x = base_size * particle.scale_x * 0.5F;
                    const auto half_size_y = base_size * particle.scale_y * 0.5F;
                    const auto local_center = smgpc::camera::CameraParamVec3{
                        .x = particle.x,
                        .y = particle.y,
                        .z = particle.z,
                    };
                    auto center = local_center;
                    if (active.host_binding.has_value()) {
                        center = smgpc::render::effects::jpc_transform_particle_center(active.host_binding->matrix, local_center);
                    }
                    const auto x = center.x;
                    const auto y = center.y;
                    const auto z = center.z;
                    const auto alpha = static_cast<std::uint8_t>(std::clamp(particle.alpha, 0.0F, 1.0F) * 255.0F);
                    const auto color = std::array<std::uint8_t, 4U>{255U, 255U, 255U, alpha};
                    const auto vertex_count = std::uint32_t{4U};
                    const auto index_count = std::uint32_t{6U};
                    auto color_channel_count = std::uint32_t{1U};
                    const auto primitive_type = std::string_view{"triangles"};
                    auto alpha_compare = jpa_alpha_compare(base_shape);
                    auto blend = jpa_blend_mode(base_shape);
                    if (*packet_path == smgpc::render::effects::JpcParticlePacketPath::WorldBillboard) {
                        const auto textured_vertices = smgpc::render::effects::jpc_billboard_world_vertices(
                            *camera_pose,
                            smgpc::render::effects::JpcBillboardGeometry{
                                .center = {.x = x, .y = y, .z = z},
                                .half_size_x = half_size_x,
                                .half_size_y = half_size_y,
                                .rotation_radians = 0.0F,
                                .color = color,
                            });
                        const auto vertices = jpa_material_vertices(textured_vertices);
                        const auto indices = std::array<std::uint16_t, 6U>{0U, 1U, 2U, 0U, 2U, 3U};
                        const auto texture_stages = std::array<render::GxTextureStage2D, 1U>{
                            render::GxTextureStage2D{
                                .texture = texture_handle,
                                .wrap_u = texture.wrap_s,
                                .wrap_v = texture.wrap_t,
                                .min_filter = texture.min_filter,
                                .mag_filter = texture.mag_filter,
                                .texgen_source = GX_TG_TEX0,
                            },
                        };
                        const auto tev_stages = std::array<render::GxTevStage2D, 1U>{jpa_tev_stage(base_shape)};
                        renderer.submit_gx_material_triangles_3d(
                            render::GxMaterialTriangleBatch2D{
                                .vertices = std::span<const render::GxMaterialVertex2D>(vertices.data(), vertices.size()),
                                .indices = std::span<const std::uint16_t>(indices.data(), indices.size()),
                                .primitive_topology = render::PrimitiveTopology::Triangles,
                                .texture_stages = std::span<const render::GxTextureStage2D>(texture_stages.data(), texture_stages.size()),
                                .tev_stages = std::span<const render::GxTevStage2D>(tev_stages.data(), tev_stages.size()),
                                .initial_tev_registers =
                                    jpa_initial_tev_registers(base_shape, particle.child ? child_shape : nullptr, particle.alpha),
                                .alpha_compare = alpha_compare,
                                .blend = blend,
                                .depth_test = jpa_depth_test(base_shape),
                                .depth_write = jpa_depth_write(base_shape),
                                .depth_compare = jpa_depth_compare(base_shape),
                            },
                            *camera_pose);
                        color_channel_count = 0U;
                    } else {
                        renderer.submit_textured_quad(texture_handle,
                                                      render::TexturedQuad2D{
                                                          .vertices =
                                                              {
                                                                  render::TexturedVertex2D{
                                                                      .x = x - half_size_x,
                                                                      .y = y - half_size_y,
                                                                      .z = z,
                                                                      .u = 0.0F,
                                                                      .v = 1.0F,
                                                                      .color = color,
                                                                  },
                                                                  render::TexturedVertex2D{
                                                                      .x = x + half_size_x,
                                                                      .y = y - half_size_y,
                                                                      .z = z,
                                                                      .u = 1.0F,
                                                                      .v = 1.0F,
                                                                      .color = color,
                                                                  },
                                                                  render::TexturedVertex2D{
                                                                      .x = x + half_size_x,
                                                                      .y = y + half_size_y,
                                                                      .z = z,
                                                                      .u = 1.0F,
                                                                      .v = 0.0F,
                                                                      .color = color,
                                                                  },
                                                                  render::TexturedVertex2D{
                                                                      .x = x - half_size_x,
                                                                      .y = y + half_size_y,
                                                                      .z = z,
                                                                      .u = 0.0F,
                                                                      .v = 0.0F,
                                                                      .color = color,
                                                                  },
                                                              },
                                                          .wrap_u = texture.wrap_s,
                                                          .wrap_v = texture.wrap_t,
                                                          .min_filter = texture.min_filter,
                                                          .mag_filter = texture.mag_filter,
                                                          .blend = true,
                                                      });
                    }
#ifndef NDEBUG
                    _draw_packets.push_back(EffectDrawPacketTrace{
                        .actor_name = active.actor_name,
                        .effect_name = active.effect_name,
                        .particle_name = resource.particle_name,
                        .user_index = resource.user_index,
                        .draw_order = resource.auto_effect_draw_order,
                        .frame_index = _frame_index,
                        .draw_type = draw_type,
                        .packet_mode = jpa_packet_path_name(*packet_path),
                        .shape_type = *shape_type,
                        .world_space = *packet_path != smgpc::render::effects::JpcParticlePacketPath::ScreenSpace,
                        .primitive_type = std::string(primitive_type),
                        .vertex_count = vertex_count,
                        .index_count = index_count,
                        .color_channel_count = color_channel_count,
                        .particle_id = particle.id,
                        .particle_age = particle.age,
                        .particle_lifetime = particle.lifetime,
                        .host_binding_found = active.host_binding.has_value(),
                        .host_binding_source = active.host_binding.has_value() ? effect_host_binding_source_name(active.host_binding->source) : "",
                        .host_translation = host_translation,
                        .particle_x = x,
                        .particle_y = y,
                        .particle_z = z,
                        .particle_scale_x = particle.scale_x,
                        .particle_scale_y = particle.scale_y,
                        .particle_alpha = particle.alpha,
                        .live_particle_count = static_cast<std::uint32_t>(emitter.particles.size()),
                        .child_particle = particle.child,
                        .alpha_compare_enabled = alpha_compare.enabled,
                        .blend_enabled = blend.enabled,
                        .texture =
                            EffectTextureBindingTrace{
                                .slot = static_cast<std::uint8_t>(particle.child ? 1U : 0U),
                                .texture_index = texture.index,
                                .name = texture.name,
                                .width = texture.width,
                                .height = texture.height,
                                .format_raw = static_cast<std::uint32_t>(texture.format),
                                .format_name = texture_format_name(texture.format),
                            },
                    });
#endif
                }
            }
        }
    }

    std::span<const EffectEvent> EffectService::events() const {
        return _events;
    }

    std::span<const ActiveEffectInstance> EffectService::active_effect_instances() const {
        return _active_effects;
    }

    std::vector<EffectKeeperRegistration> EffectService::registered_keepers() const {
        auto out = std::vector<EffectKeeperRegistration>{};
        out.reserve(_registered_keepers.size() + _registered_keeper_instances.size());
        for (const auto &[_, keeper] : _registered_keepers) {
            out.push_back(keeper);
        }
        for (const auto &[_, keeper] : _registered_keeper_instances) {
            out.push_back(keeper);
        }
        return out;
    }

    std::optional<EffectKeeperRegistration> EffectService::registered_keeper(std::string_view host_name,
                                                                             const void *host_identity) const {
        if (host_identity != nullptr) {
            if (const auto it = _registered_keeper_instances.find(host_identity); it != _registered_keeper_instances.end()) {
                return it->second;
            }
            return std::nullopt;
        }
        if (auto it = _registered_keepers.find(host_name); it != _registered_keepers.end()) {
            return it->second;
        }

        return std::nullopt;
    }

    std::optional<EffectHostBinding> EffectService::host_binding(std::string_view host_name,
                                                                 const void *host_identity) const {
        if (host_identity != nullptr) {
            if (const auto it = _host_binding_instances.find(host_identity); it != _host_binding_instances.end()) {
                return it->second;
            }
            return std::nullopt;
        }
        if (auto it = _host_bindings.find(host_name); it != _host_bindings.end()) {
            return it->second;
        }

        return std::nullopt;
    }

    std::vector<std::string> EffectService::active_effects(std::string_view actor_name, const void *host_identity) const {
        auto out = std::vector<std::string>{};
        for (const auto &active : _active_effects) {
            if (effect_host_matches(active, actor_name, host_identity)) {
                out.push_back(active.effect_name);
            }
        }

        return out;
    }

    const smgpc::render::effects::EffectResourceLibrary *EffectService::resource_library() const {
        return _resource_library.has_value() ? &*_resource_library : nullptr;
    }

#ifndef NDEBUG
    std::span<const EffectDrawPacketTrace> EffectService::draw_packets() const {
        return _draw_packets;
    }
#endif

    std::vector<smgpc::render::effects::ResolvedEffectResource>
    EffectService::resolve(std::string_view actor_name, std::string_view effect_name, const void *host_identity) const {
        if (!_resource_library.has_value() || effect_name.empty()) {
            return {};
        }

        if (const auto keeper = registered_keeper(actor_name, host_identity); keeper.has_value() && !keeper->resource_group_name.empty()) {
            auto resources = _resource_library->resolve_auto_effect(keeper->resource_group_name, effect_name);
            if (!resources.empty()) {
                return resources;
            }
        }

        return _resource_library->resolve_effect_request(effect_name);
    }

    std::vector<JpcEffectEmitterInstance> EffectService::create_emitters(std::span<const smgpc::render::effects::ResolvedEffectResource> resources) {
        auto emitters = std::vector<JpcEffectEmitterInstance>{};
        emitters.reserve(resources.size());
        for (const auto &resource : resources) {
            const auto seed = next_jpa_random_u(_emitter_random_seed);
            emitters.push_back(JpcEffectEmitterInstance{
                .user_index = resource.user_index,
                .particle_name = resource.particle_name,
                .start_frame_index = _frame_index,
                .next_update_frame_index = _frame_index,
                .random_seed = seed,
                .particles = {},
            });
        }
        return emitters;
    }

    void EffectService::advance_effects_to_frame(std::uint64_t frame_index) {
        for (auto &active : _active_effects) {
            for (auto emitter_index = std::size_t{}; emitter_index < active.emitters.size() && emitter_index < active.resolved_resources.size();
                 ++emitter_index) {
                advance_emitter_to_frame(active.emitters[emitter_index], active.resolved_resources[emitter_index], frame_index);
            }
        }
    }

    void EffectService::advance_emitter_to_frame(JpcEffectEmitterInstance &emitter, const smgpc::render::effects::ResolvedEffectResource &resource,
                                                 std::uint64_t frame_index) {
        constexpr auto MAX_PARTICLES_PER_EMITTER = std::size_t{4096U};
        const auto *metadata = resource.resource;
        if (metadata == nullptr || !metadata->dynamics.has_value()) {
            if (emitter.particles.empty() && emitter.next_update_frame_index <= frame_index) {
                emitter.particles.push_back(JpcEffectParticleInstance{
                    .id = emitter.next_particle_id++,
                    .lifetime = 1U,
                    .x = resource.auto_effect_offset_x,
                    .y = -resource.auto_effect_offset_y,
                    .z = resource.auto_effect_offset_z * 0.001F,
                    .alpha = std::clamp(resource.auto_effect_rate_value, 0.0F, 1.0F),
                });
                emitter.next_update_frame_index = frame_index + 1U;
            }
            return;
        }

        const auto &dynamics = *metadata->dynamics;
        while (emitter.next_update_frame_index <= frame_index) {
            const auto local_frame = emitter.next_update_frame_index - emitter.start_frame_index;
            for (auto &particle : emitter.particles) {
                if (particle.age < particle.lifetime) {
                    if (particle.child && metadata->child_shape.has_value() && particle.age != 0U) {
                        particle.velocity_y -= metadata->child_shape->gravity;
                    }
                    const auto air_resistance = std::clamp(dynamics.air_resistance, 0.0F, 1.0F);
                    particle.velocity_x *= air_resistance;
                    particle.velocity_y *= air_resistance;
                    particle.velocity_z *= air_resistance;
                    particle.x += particle.velocity_x * particle.momentum;
                    particle.y += particle.velocity_y * particle.momentum;
                    particle.z += particle.velocity_z * particle.momentum;
                    ++particle.age;
                }
            }
            std::erase_if(emitter.particles, [](const auto &particle) {
                return particle.age >= particle.lifetime;
            });

            if (local_frame >= static_cast<std::uint64_t>(std::max<std::int16_t>(dynamics.start_frame, 0))) {
                const auto emitter_age = local_frame - static_cast<std::uint64_t>(std::max<std::int16_t>(dynamics.start_frame, 0));
                const auto keyed_dynamics = jpa_keyed_emitter_dynamics(dynamics, metadata->key_blocks, static_cast<float>(emitter_age));
                const auto stop_emit = dynamics.max_frame > 0 && emitter_age >= static_cast<std::uint64_t>(dynamics.max_frame);
                auto emit_count = 0;
                if (emitter.rate_step_emit && !stop_emit) {
                    if (dynamics.fixed_interval) {
                        emit_count = std::max<std::uint16_t>(dynamics.div_number, 1U);
                    } else {
                        const auto effective_rate = std::max(0.0F, keyed_dynamics.rate * resource.auto_effect_rate_value);
                        const auto new_particle_count = effective_rate * (dynamics.rate_random * next_jpa_random_zp(emitter.random_seed) + 1.0F);
                        emitter.fractional_emit_count += new_particle_count;
                        emit_count = static_cast<int>(emitter.fractional_emit_count);
                        emitter.fractional_emit_count -= static_cast<float>(emit_count);
                        if (emitter.first_emit && new_particle_count > 0.0F && new_particle_count < 1.0F) {
                            emit_count = 1;
                        }
                    }
                }

                for (auto emitted = 0; emitted < emit_count && emitter.particles.size() < MAX_PARTICLES_PER_EMITTER; ++emitted) {
                    const auto lifetime = jpa_particle_lifetime(dynamics, keyed_dynamics.lifetime, emitter.random_seed);
                    const auto volume_sample = jpa_particle_volume_sample(dynamics, keyed_dynamics, emitter.random_seed, emitted, emit_count);
                    const auto velocity = jpa_parent_velocity(dynamics, keyed_dynamics, volume_sample, emitter.random_seed);
                    const auto momentum = 1.0F - dynamics.moment * next_jpa_random_f(emitter.random_seed);
                    emitter.particles.push_back(JpcEffectParticleInstance{
                        .id = emitter.next_particle_id++,
                        .age = 0U,
                        .lifetime = lifetime,
                        .x = resource.auto_effect_offset_x + dynamics.emitter_translation.x + volume_sample.position.x,
                        .y = -(resource.auto_effect_offset_y + dynamics.emitter_translation.y + volume_sample.position.y),
                        .z = (resource.auto_effect_offset_z + dynamics.emitter_translation.z + volume_sample.position.z) * 0.001F,
                        .velocity_x = velocity.x,
                        .velocity_y = -velocity.y,
                        .velocity_z = velocity.z * 0.001F,
                        .momentum = momentum,
                        .alpha = std::clamp(resource.auto_effect_rate_value, 0.0F, 1.0F),
                    });
                }

                if (metadata->child_shape.has_value()) {
                    const auto &child_shape = *metadata->child_shape;
                    const auto child_rate = std::max<std::int16_t>(child_shape.rate, 0);
                    const auto child_lifetime = static_cast<std::uint16_t>(std::max<std::int16_t>(child_shape.lifetime, 1));
                    const auto child_step = static_cast<std::uint16_t>(child_shape.step) + 1U;
                    const auto parent_count = emitter.particles.size();
                    for (auto particle_index = std::size_t{}; particle_index < parent_count; ++particle_index) {
                        const auto parent = emitter.particles[particle_index];
                        if (parent.child) {
                            continue;
                        }
                        const auto child_start_age = static_cast<int>(
                            std::floor(static_cast<float>(std::max<std::uint16_t>(parent.lifetime, 1U) - 1U) * child_shape.timing));
                        const auto child_time = static_cast<int>(parent.age) - child_start_age;
                        if (child_time < 0 || child_time % child_step != 0) {
                            continue;
                        }

                        for (auto child_index = std::int16_t{}; child_index < child_rate && emitter.particles.size() < MAX_PARTICLES_PER_EMITTER;
                             ++child_index) {
                            const auto scale_inherit = child_shape.scale_inherited ? child_shape.inherit_scale : 1.0F;
                            const auto alpha_inherit = child_shape.alpha_inherited ? child_shape.inherit_alpha : 1.0F;
                            auto child_position = JpcVec3{.x = parent.x, .y = parent.y, .z = parent.z};
                            if (child_shape.position_random != 0.0F) {
                                child_position = jpa_add(child_position,
                                                         jpa_set_length(
                                                             {
                                                                 .x = next_jpa_random_zh(emitter.random_seed),
                                                                 .y = -next_jpa_random_zh(emitter.random_seed),
                                                                 .z = next_jpa_random_zh(emitter.random_seed) * 0.001F,
                                                             },
                                                             child_shape.position_random * next_jpa_random_f(emitter.random_seed)));
                            }
                            const auto child_velocity = jpa_child_velocity(parent, child_shape, emitter.random_seed);
                            emitter.particles.push_back(JpcEffectParticleInstance{
                                .id = emitter.next_particle_id++,
                                .age = 0U,
                                .lifetime = child_lifetime,
                                .child = true,
                                .x = child_position.x,
                                .y = child_position.y,
                                .z = child_position.z,
                                .velocity_x = child_velocity.x,
                                .velocity_y = -child_velocity.y,
                                .velocity_z = child_velocity.z * 0.001F,
                                .momentum = parent.momentum,
                                .scale_x = std::max(0.01F, child_shape.scale_x * scale_inherit),
                                .scale_y = std::max(0.01F, child_shape.scale_y * scale_inherit),
                                .alpha = std::clamp(parent.alpha * alpha_inherit, 0.0F, 1.0F),
                            });
                        }
                    }
                }

                const auto rate_step_period = static_cast<std::uint16_t>(dynamics.rate_step) + 1U;
                ++emitter.rate_step_timer;
                if (emitter.rate_step_timer >= rate_step_period) {
                    emitter.rate_step_timer = static_cast<std::uint16_t>(emitter.rate_step_timer - rate_step_period);
                    emitter.rate_step_emit = true;
                } else {
                    emitter.rate_step_emit = false;
                }
                emitter.first_emit = false;
            }

            ++emitter.next_update_frame_index;
        }
    }

    render::TextureHandle EffectService::texture_handle_for(const smgpc::render::effects::JpcTextureMetadata &texture) {
        if (const auto it = _texture_handles.find(texture.index); it != _texture_handles.end() && it->second.is_valid()) {
            return it->second;
        }

        if (texture.image.rgba.empty() || texture.image.width == 0U || texture.image.height == 0U) {
            return {};
        }

        auto &renderer = render::current_aurora_renderer();
        auto handle = renderer.create_rgba8_texture(texture.image.width, texture.image.height,
                                                    std::span<const std::uint8_t>(texture.image.rgba.data(), texture.image.rgba.size()));
        if (handle.is_valid()) {
            _texture_handles[texture.index] = handle;
        }
        return handle;
    }

    void WipeService::begin_frame(std::uint64_t frame_index) {
        _frame_index = frame_index;
        if ((_state != WipeState::Opening && _state != WipeState::Closing) || _remaining_frames <= 0) {
            return;
        }

        --_remaining_frames;
        if (_remaining_frames <= 0) {
            _state = _state == WipeState::Opening ? WipeState::Open : WipeState::Closed;
        }
    }

    void WipeService::open(std::string_view name, s32 frame_count) {
        start_transition(WipeEventKind::Open, WipeState::Opening, name, frame_count);
    }

    void WipeService::close(std::string_view name, s32 frame_count) {
        start_transition(WipeEventKind::Close, WipeState::Closing, name, frame_count);
    }

    void WipeService::force_open(std::string_view name) {
        _current_name = name;
        _state = WipeState::Open;
        _remaining_frames = 0;
        _duration_frames = 0;
        push_event(WipeEventKind::ForceOpen, name, 0);
    }

    void WipeService::force_close(std::string_view name) {
        _current_name = name;
        _state = WipeState::Closed;
        _remaining_frames = 0;
        _duration_frames = 0;
        push_event(WipeEventKind::ForceClose, name, 0);
    }

    bool WipeService::is_active() const {
        return _state == WipeState::Opening || _state == WipeState::Closing;
    }

    bool WipeService::is_blank() const {
        return _state == WipeState::Closed;
    }

    bool WipeService::is_open() const {
        return _state == WipeState::Open;
    }

    WipeState WipeService::state() const {
        return _state;
    }

    std::string_view WipeService::current_name() const {
        return _current_name;
    }

    s32 WipeService::remaining_frames() const {
        return _remaining_frames;
    }

    s32 WipeService::duration_frames() const {
        return _duration_frames;
    }

    std::span<const WipeEvent> WipeService::events() const {
        return _events;
    }

    void WipeService::start_transition(WipeEventKind kind, WipeState state, std::string_view name, s32 frame_count) {
        _current_name = name;
        _duration_frames = normalized_frame_count(frame_count);
        _remaining_frames = _duration_frames;
        _state = _remaining_frames <= 0 ? (state == WipeState::Opening ? WipeState::Open : WipeState::Closed) : state;
        push_event(kind, name, frame_count);
    }

    void WipeService::push_event(WipeEventKind kind, std::string_view name, s32 frame_count) {
        _events.push_back(WipeEvent{
            .kind = kind,
            .name = std::string(name),
            .frame_count = frame_count,
            .frame_index = _frame_index,
        });
    }

    s32 WipeService::normalized_frame_count(s32 frame_count) {
        return frame_count < 0 ? 30 : frame_count;
    }

    void ImageEffectService::begin_frame(std::uint64_t frame_index) {
        _frame_index = frame_index;
    }

    void ImageEffectService::force_off() {
        _forced_off = true;
        _control_auto = false;
        push_event(ImageEffectControlKind::ForceOff);
    }

    void ImageEffectService::set_control_auto() {
        _forced_off = false;
        _control_auto = true;
        push_event(ImageEffectControlKind::ControlAuto);
    }

    bool ImageEffectService::is_forced_off() const {
        return _forced_off;
    }

    bool ImageEffectService::is_control_auto() const {
        return _control_auto;
    }

    std::span<const ImageEffectControlEvent> ImageEffectService::events() const {
        return _events;
    }

    void ImageEffectService::push_event(ImageEffectControlKind kind) {
        _events.push_back(ImageEffectControlEvent{
            .kind = kind,
            .frame_index = _frame_index,
        });
    }

    void StarPointerService::begin_frame(std::uint64_t frame_index) {
        _frame_index = frame_index;
    }

    void StarPointerService::register_target(const LiveActor &actor, float radius, const smgpc::camera::CameraParamVec3 &offset) {
        _targets[&actor] = StarPointerTargetState{
            .actor = &actor,
            .radius = radius,
            .offset = offset,
        };
    }

    void StarPointerService::unregister_target(const LiveActor &actor) {
        _targets.erase(&actor);
    }

    void StarPointerService::set_target_radius(const LiveActor &actor, float radius) {
        if (auto iter = _targets.find(&actor); iter != _targets.end()) {
            iter->second.radius = radius;
        }
    }

    void StarPointerService::start_mode(StarPointerMode mode) {
        if (_mode == mode) {
            return;
        }

        _mode = mode;
        _mode_events.push_back(StarPointerModeEvent{
            .mode = mode,
            .frame_index = _frame_index,
        });
    }

    void StarPointerService::set_guidance_active(bool active) {
        _guidance_active = active;
    }

    void StarPointerService::request_guidance(StarPointerGuidanceRequest request) {
        if (request == StarPointerGuidanceRequest::None) {
            return;
        }

        if (std::find(_guidance_requests.begin(), _guidance_requests.end(), request) == _guidance_requests.end()) {
            _guidance_requests.push_back(request);
        }
    }

    StarPointerMode StarPointerService::mode() const {
        return _mode;
    }

    bool StarPointerService::has_target(const LiveActor &actor) const {
        return _targets.contains(&actor);
    }

    bool StarPointerService::is_pointing(const LiveActor &actor, const WpadService &wpad, const std::optional<smgpc::camera::CameraPose> &camera_pose, bool check_z) {
        const auto iter = _targets.find(&actor);
        if (iter == _targets.end()) {
            return false;
        }

        auto &target = iter->second;
        auto pointer = wpad.pointer(WPAD_CHAN0);
        auto projection = std::optional<StarPointerProjection>{};
        auto pointing = false;

        if (!actor.isDead() && camera_pose.has_value() && pointer.valid) {
            projection = project_star_pointer_target(target, *camera_pose, check_z);
            if (projection.has_value()) {
                const auto dx = pointer.x - projection->x;
                const auto dy = pointer.y - projection->y;
                pointing = (dx * dx) + (dy * dy) <= projection->radius * projection->radius;
            }
        }

#ifndef NDEBUG
        record_target_pointing_sample(target, pointing, pointer, projection.has_value(), projection.has_value() ? projection->x : 0.0F,
                                      projection.has_value() ? projection->y : 0.0F, projection.has_value() ? projection->radius : 0.0F,
                                      check_z, wpad.is_button_triggered(WPAD_CHAN0, WPAD_BUTTON_A));
#endif
        return pointing;
    }

    bool StarPointerService::is_guidance_active() const {
        return _guidance_active;
    }

    bool StarPointerService::is_guidance_requested(StarPointerGuidanceRequest request) const {
        return std::find(_guidance_requests.begin(), _guidance_requests.end(), request) != _guidance_requests.end();
    }

    std::span<const StarPointerGuidanceRequest> StarPointerService::guidance_requests() const {
        return _guidance_requests;
    }

    std::span<const StarPointerModeEvent> StarPointerService::mode_events() const {
        return _mode_events;
    }

#ifndef NDEBUG
    std::span<const StarPointerTargetEvent> StarPointerService::target_events() const {
        return _target_events;
    }

    void StarPointerService::record_target_pointing_sample(StarPointerTargetState &target, bool pointing, const WpadPointerState &pointer,
                                                           bool has_projection, float target_x, float target_y, float projected_radius,
                                                           bool check_z, bool select_triggered) {
        const auto push_event = [&](StarPointerTargetEventKind kind) {
            _target_events.push_back(StarPointerTargetEvent{
                .kind = kind,
                .actor_name = target.actor != nullptr ? target.actor->getName() : "",
                .frame_index = _frame_index,
                .channel = WPAD_CHAN0,
                .pointer_x = pointer.x,
                .pointer_y = pointer.y,
                .target_x = has_projection ? target_x : 0.0F,
                .target_y = has_projection ? target_y : 0.0F,
                .projected_radius = has_projection ? projected_radius : 0.0F,
                .check_z = check_z,
            });
        };

        if (pointing != target.was_pointing) {
            push_event(pointing ? StarPointerTargetEventKind::Enter : StarPointerTargetEventKind::Leave);
            target.was_pointing = pointing;
        }

        if (pointing && select_triggered && target.last_select_frame_index != _frame_index) {
            push_event(StarPointerTargetEventKind::Select);
            target.last_select_frame_index = _frame_index;
        }
    }
#endif

    void CameraSystemService::begin_frame(std::uint64_t frame_index) {
        _frame_index = frame_index;
        _shake_offset_x = 0.0F;
        _shake_offset_y = 0.0F;
        for (auto index = std::size_t{}; index < _vertical_shake_steps.size(); ++index) {
            auto& step = _vertical_shake_steps[index];
            if (!step.has_value()) {
                continue;
            }

            ++*step;
            if (*step >= CAMERA_SHAKE_FRAME_COUNT) {
                step.reset();
                continue;
            }
            _shake_offset_y += camera_singly_vertical_offset(CAMERA_SHAKE_AMPLITUDES[index], *step);
        }
    }

    void CameraSystemService::set_shake_projection_dimensions(float screen_width, float efb_height) {
        if (!std::isfinite(screen_width) || !std::isfinite(efb_height) || screen_width <= 0.0F || efb_height <= 0.0F) {
            throw std::invalid_argument("Camera shake projection dimensions must be finite and positive.");
        }
        _shake_screen_width = screen_width;
        _shake_efb_height = efb_height;
    }

    void CameraSystemService::clear_shake_projection_dimensions() noexcept {
        _shake_screen_width.reset();
        _shake_efb_height.reset();
    }

    void CameraSystemService::reset_camera_man() {
        ++_reset_camera_man_count;
    }

    void CameraSystemService::request_very_weak_shake() {
        request_shake(ShakeRequestKind::VeryWeak);
    }

    void CameraSystemService::request_weak_shake() {
        request_shake(ShakeRequestKind::Weak);
    }

    void CameraSystemService::request_normal_weak_shake() {
        request_shake(ShakeRequestKind::NormalWeak);
    }

    void CameraSystemService::request_normal_shake() {
        request_shake(ShakeRequestKind::Normal);
    }

    void CameraSystemService::request_normal_strong_shake() {
        request_shake(ShakeRequestKind::NormalStrong);
    }

    void CameraSystemService::request_strong_shake() {
        request_shake(ShakeRequestKind::Strong);
    }

    void CameraSystemService::request_very_strong_shake() {
        request_shake(ShakeRequestKind::VeryStrong);
    }

    void CameraSystemService::pause_on_camera_director() {
        ++_camera_director_pause_count;
    }

    void CameraSystemService::pause_off_camera_director() {
        if (_camera_director_pause_count > 0U) {
            --_camera_director_pause_count;
        }
    }

    void CameraSystemService::declare_event_camera_programmable(std::string_view name) {
        if (name.empty()) {
            return;
        }

        auto &event = _programmable_camera_events[std::string(name)];
        event.declared = true;
        ++_programmable_camera_declare_count;
    }

    void CameraSystemService::start_global_event_camera_no_target(std::string_view name) {
        if (name.empty()) {
            return;
        }

        if (auto *event = find_programmable_event(_active_programmable_camera_name)) {
            event->active = false;
        }

        auto &event = _programmable_camera_events[std::string(name)];
        event.declared = true;
        event.active = true;
        _active_programmable_camera_name = std::string(name);
        ++_programmable_camera_start_count;
    }

    void CameraSystemService::end_global_event_camera(std::string_view name) {
        if (name.empty()) {
            return;
        }

        if (auto *event = find_programmable_event(name)) {
            event->active = false;
        }
        if (_active_programmable_camera_name == name) {
            _active_programmable_camera_name.clear();
        }
        ++_programmable_camera_end_count;
    }

    void CameraSystemService::set_game_camera_pose(const smgpc::camera::CameraPose &pose) {
        _game_camera_pose = pose;
    }

    void CameraSystemService::clear_game_camera_pose() {
        _game_camera_pose.reset();
    }

    std::optional<smgpc::camera::CameraPose> CameraSystemService::set_programmable_camera_param(std::string_view name, const smgpc::camera::CameraParamVec3 &watch,
                                                                                                const smgpc::camera::CameraParamVec3 &eye, const smgpc::camera::CameraParamVec3 &up,
                                                                                                bool do_zero_w_offset) {
        (void)do_zero_w_offset;
        if (name.empty()) {
            return std::nullopt;
        }

        auto &event = _programmable_camera_events[std::string(name)];
        event.declared = true;
        event.pose.eye = eye;
        event.pose.watch = watch;
        event.pose.up = up;
        event.has_pose = true;
        ++_programmable_camera_param_count;

        return active_programmable_camera_pose_for(name);
    }

    std::optional<smgpc::camera::CameraPose> CameraSystemService::set_programmable_camera_fovy(std::string_view name, float fovy_degrees) {
        if (name.empty()) {
            return std::nullopt;
        }

        auto &event = _programmable_camera_events[std::string(name)];
        event.declared = true;
        event.pose.fovy_degrees = fovy_degrees;
        ++_programmable_camera_fovy_count;

        return active_programmable_camera_pose_for(name);
    }

    std::uint32_t CameraSystemService::reset_camera_man_count() const {
        return _reset_camera_man_count;
    }

    std::uint32_t CameraSystemService::camera_director_pause_count() const {
        return _camera_director_pause_count;
    }

    bool CameraSystemService::is_camera_director_paused() const {
        return _camera_director_pause_count > 0U;
    }

    std::optional<smgpc::camera::CameraPose> CameraSystemService::game_camera_pose() const {
        return _game_camera_pose;
    }

    std::optional<smgpc::camera::CameraPose> CameraSystemService::active_programmable_camera_pose() const {
        return active_programmable_camera_pose_for(_active_programmable_camera_name);
    }

    std::optional<smgpc::camera::CameraPose> CameraSystemService::effective_camera_pose() const {
        if (const auto programmable = active_programmable_camera_pose()) {
            return apply_shake(*programmable);
        }
        return _game_camera_pose.has_value() ? std::optional{apply_shake(*_game_camera_pose)} : std::nullopt;
    }

    smgpc::camera::CameraPose CameraSystemService::apply_shake(const smgpc::camera::CameraPose& pose) const {
        auto shaken = pose;
        if (_shake_offset_x == 0.0F && _shake_offset_y == 0.0F) {
            return shaken;
        }
        if (!_shake_screen_width.has_value() || !_shake_efb_height.has_value()) {
            throw std::logic_error("Camera shake projection dimensions are unavailable.");
        }
        shaken.projection_offset_x += _shake_offset_x * 30.0F / *_shake_screen_width;
        shaken.projection_offset_y += _shake_offset_y * 30.0F / *_shake_efb_height;
        return shaken;
    }

    std::optional<std::string_view> CameraSystemService::active_programmable_camera_name() const {
        if (_active_programmable_camera_name.empty()) {
            return std::nullopt;
        }

        return std::string_view(_active_programmable_camera_name);
    }

    std::uint32_t CameraSystemService::programmable_camera_declare_count() const {
        return _programmable_camera_declare_count;
    }

    std::uint32_t CameraSystemService::programmable_camera_start_count() const {
        return _programmable_camera_start_count;
    }

    std::uint32_t CameraSystemService::programmable_camera_end_count() const {
        return _programmable_camera_end_count;
    }

    std::uint32_t CameraSystemService::programmable_camera_param_count() const {
        return _programmable_camera_param_count;
    }

    std::uint32_t CameraSystemService::programmable_camera_fovy_count() const {
        return _programmable_camera_fovy_count;
    }

    std::span<const CameraSystemService::ShakeRequestEvent> CameraSystemService::shake_request_events() const {
        return _shake_request_events;
    }

    void CameraSystemService::request_shake(ShakeRequestKind kind) {
        if (!_shake_screen_width.has_value() || !_shake_efb_height.has_value()) {
            throw std::logic_error("Camera shake requires an exact retail projection size.");
        }
        auto& step = _vertical_shake_steps[camera_shake_index(kind)];
        if (step.has_value()) {
            return;
        }

        step = 0U;
        push_shake_event(kind);
    }

    void CameraSystemService::push_shake_event(ShakeRequestKind kind) {
        _shake_request_events.push_back(ShakeRequestEvent{
            .kind = kind,
            .frame_index = _frame_index,
        });
    }

    CameraSystemService::ProgrammableCameraEventState *CameraSystemService::find_programmable_event(std::string_view name) {
        const auto it = _programmable_camera_events.find(std::string(name));
        return it == _programmable_camera_events.end() ? nullptr : &it->second;
    }

    const CameraSystemService::ProgrammableCameraEventState *CameraSystemService::find_programmable_event(std::string_view name) const {
        const auto it = _programmable_camera_events.find(std::string(name));
        return it == _programmable_camera_events.end() ? nullptr : &it->second;
    }

    std::optional<smgpc::camera::CameraPose> CameraSystemService::active_programmable_camera_pose_for(std::string_view name) const {
        if (name.empty() || _active_programmable_camera_name != name) {
            return std::nullopt;
        }

        const auto *event = find_programmable_event(name);
        if (event == nullptr || !event->active || !event->has_pose) {
            return std::nullopt;
        }

        return event->pose;
    }

    void PlayerSystemService::reset_stage_state() {
        _attached_actor = nullptr;
        _player_hidden = false;
        _has_base_matrix = false;
        _has_forced_base_matrix = false;
        _on_ground = false;
        _swing_permitted = false;
        // Control ownership can span scene boundaries (notably puppetable
        // demos), so stage-local actor teardown must not release it.
        _reset_condition_requested = false;
        ++_base_matrix_revision;
        _base_matrix = {};
        _position = {};
        _velocity = {};
        _gravity = {};
    }

    void PlayerSystemService::clear_stage_state() {
        reset_stage_state();
    }

    void PlayerSystemService::attach_actor(LiveActor &actor) {
        _attached_actor = &actor;
        copy_actor_state();
        actor.mFlag.mIsHiddenModel = _player_hidden;
    }

    void PlayerSystemService::detach_actor(const LiveActor *actor) {
        if (actor == nullptr || _attached_actor == actor) {
            _attached_actor = nullptr;
        }
    }

    void PlayerSystemService::synchronize_attached_actor() {
        if (_attached_actor == nullptr) {
            return;
        }

        _attached_actor->calcAndSetBaseMtx();
        copy_actor_state();
        _attached_actor->mFlag.mIsHiddenModel = _player_hidden;
    }

    void PlayerSystemService::show_player() {
        _player_hidden = false;
        if (_attached_actor != nullptr) {
            _attached_actor->mFlag.mIsHiddenModel = false;
        }
    }

    void PlayerSystemService::hide_player() {
        _player_hidden = true;
        if (_attached_actor != nullptr) {
            _attached_actor->mFlag.mIsHiddenModel = true;
        }
    }

    void PlayerSystemService::set_base_matrix(MtxPtr matrix) {
        ++_base_matrix_revision;
        _has_base_matrix = matrix != nullptr;
        _has_forced_base_matrix = matrix != nullptr;
        if (matrix == nullptr) {
            _base_matrix = {};
            _position = {};
            return;
        }

        auto index = std::size_t{};
        for (auto row = 0U; row < 3U; ++row) {
            for (auto column = 0U; column < 4U; ++column) {
                _base_matrix[index++] = matrix[row][column];
            }
        }
        _position = {_base_matrix[3U], _base_matrix[7U], _base_matrix[11U]};

        if (_attached_actor != nullptr) {
            _attached_actor->setBaseMatrix(smgpc::render::J3dMatrix3x4{_base_matrix});
            _attached_actor->mPosition.set(_position[0U], _position[1U], _position[2U]);
        }
    }

    void PlayerSystemService::set_swing_permission(bool permitted) {
        _swing_permitted = permitted;
    }

    void PlayerSystemService::disable_control() {
        _control_enabled = false;
    }

    void PlayerSystemService::enable_control(bool reset_condition) {
        _control_enabled = true;
        _reset_condition_requested = _reset_condition_requested || reset_condition;
    }

    void PlayerSystemService::finish_opening_demo() {
        _control_enabled = true;
        _reset_condition_requested = false;
        _has_forced_base_matrix = false;
        ++_base_matrix_revision;

        if (_attached_actor == nullptr) {
            return;
        }

        _attached_actor->mVelocity.zero();
        _attached_actor->mBindedGround = false;
        _attached_actor->mBindedWall = false;
        _attached_actor->mBindedRoof = false;
        _attached_actor->mFlag.mIsNoBind = false;
        copy_actor_state();
    }

    bool PlayerSystemService::is_player_hidden() const {
        return _player_hidden;
    }

    bool PlayerSystemService::has_base_matrix() const {
        return _has_base_matrix;
    }

    bool PlayerSystemService::has_forced_base_matrix() const {
        return _has_forced_base_matrix;
    }

    std::span<const f32, 12U> PlayerSystemService::base_matrix() const {
        return _base_matrix;
    }

    std::span<const f32, 3U> PlayerSystemService::position() const {
        return _position;
    }

    std::span<const f32, 3U> PlayerSystemService::velocity() const {
        return _velocity;
    }

    std::span<const f32, 3U> PlayerSystemService::gravity() const {
        return _gravity;
    }

    bool PlayerSystemService::is_on_ground() const {
        return _on_ground;
    }

    bool PlayerSystemService::is_swing_permitted() const {
        return _swing_permitted;
    }

    bool PlayerSystemService::is_control_enabled() const {
        return _control_enabled;
    }

    std::uint64_t PlayerSystemService::base_matrix_revision() const {
        return _base_matrix_revision;
    }

    LiveActor *PlayerSystemService::attached_actor() const {
        return _attached_actor;
    }

    bool PlayerSystemService::consume_reset_condition_request() {
        return std::exchange(_reset_condition_requested, false);
    }

    void PlayerSystemService::copy_actor_state() {
        if (_attached_actor == nullptr) {
            return;
        }

        _has_base_matrix = true;
        _base_matrix = _attached_actor->getBaseMatrix().m;
        _position = {_attached_actor->mPosition.x, _attached_actor->mPosition.y, _attached_actor->mPosition.z};
        _velocity = {_attached_actor->mVelocity.x, _attached_actor->mVelocity.y, _attached_actor->mVelocity.z};
        _gravity = {_attached_actor->mGravity.x, _attached_actor->mGravity.y, _attached_actor->mGravity.z};
        _on_ground = _attached_actor->mBindedGround;
    }

    void GameLayoutService::deactivate_default_game_layout() {
        _default_game_layout_active = false;
    }

    void GameLayoutService::activate_game_scene_draw_3d() {
        _game_scene_draw_3d_active = true;
    }

    void GameLayoutService::deactivate_game_scene_draw_3d() {
        _game_scene_draw_3d_active = false;
    }

    bool GameLayoutService::is_default_game_layout_active() const {
        return _default_game_layout_active;
    }

    bool GameLayoutService::is_game_scene_draw_3d_active() const {
        return _game_scene_draw_3d_active;
    }

    RumbleService::RumbleService(RumbleActuator* actuator) : _actuator(actuator) {
    }

    RumbleService::~RumbleService() {
        stop_all();
    }

    void RumbleService::attach_actuator(RumbleActuator& actuator) {
        if (_actuator == &actuator) {
            return;
        }

        stop_all();
        _actuator = &actuator;
    }

    void RumbleService::begin_frame(std::uint64_t frame_index) {
        _frame_index = frame_index;

        for (auto channel = std::size_t{}; channel < _active_patterns.size(); ++channel) {
            auto& patterns = _active_patterns[channel];
            const auto channel_index = static_cast<s32>(channel);
            if (_actuator == nullptr || !_actuator->is_available(channel_index)) {
                set_motor(channel_index, false);
                patterns.clear();
                continue;
            }

            auto enabled = false;
            std::erase_if(patterns, [&enabled](ActivePattern& active) {
                if (active.pattern == nullptr || active.next_frame >= static_cast<std::size_t>(active.pattern->mFrame)) {
                    return true;
                }

                enabled = enabled || active.pattern->mPattern[active.next_frame] == WPAD_MOTOR_RUMBLE;
                ++active.next_frame;
                return false;
            });
            set_motor(channel_index, enabled);
        }
    }

    bool RumbleService::try_request_pattern(const void* source, std::string_view pattern_name, s32 channel) {
        if (pattern_name.empty() || channel < 0 || channel >= static_cast<s32>(_active_patterns.size()) ||
            _actuator == nullptr || !_actuator->is_available(channel)) {
            return false;
        }

        const auto* pattern = static_cast<const RumblePattern*>(nullptr);
        for (auto index = u16{}; index < RumbleData::getTableSize(); ++index) {
            const auto* candidate = RumbleData::getData(index);
            if (candidate != nullptr && candidate->mName != nullptr && pattern_name == candidate->mName) {
                pattern = candidate;
                break;
            }
        }
        if (pattern == nullptr || pattern->mFrame <= 0) {
            return false;
        }

        auto& patterns = _active_patterns[static_cast<std::size_t>(channel)];
        if (std::ranges::any_of(patterns, [source, pattern](const ActivePattern& active) {
                return active.source == source && active.pattern == pattern;
            }) ||
            patterns.size() >= 8U) {
            return false;
        }

        patterns.push_back(ActivePattern{
            .source = source,
            .pattern = pattern,
            .next_frame = 1U,
        });

        auto enabled = false;
        for (const auto& active : patterns) {
            const auto current_frame = active.next_frame == 0U ? 0U : active.next_frame - 1U;
            enabled = enabled || (current_frame < static_cast<std::size_t>(active.pattern->mFrame) &&
                                  active.pattern->mPattern[current_frame] == WPAD_MOTOR_RUMBLE);
        }
        set_motor(channel, enabled);
        _events.push_back(RumbleRequestEvent{
            .kind = RumbleRequestKind::Named,
            .pattern_name = std::string(pattern_name),
            .channel = channel,
            .frame_index = _frame_index,
        });
        return true;
    }

    void RumbleService::stop_all() noexcept {
        for (auto channel = std::size_t{}; channel < _active_patterns.size(); ++channel) {
            set_motor(static_cast<s32>(channel), false);
            _active_patterns[channel].clear();
        }
    }

    std::span<const RumbleRequestEvent> RumbleService::events() const {
        return _events;
    }

    void RumbleService::set_motor(s32 channel, bool enabled) noexcept {
        if (channel < 0 || channel >= static_cast<s32>(_motor_enabled.size())) {
            return;
        }

        auto& current = _motor_enabled[static_cast<std::size_t>(channel)];
        if (current == enabled) {
            return;
        }
        if (_actuator != nullptr) {
            _actuator->set_motor(channel, enabled);
        }
        current = enabled;
    }

    void SequenceRequestService::begin_frame(std::uint64_t frame_index) {
        _frame_index = frame_index;
    }

    void SequenceRequestService::request_change_stage_in_game_after_loading_game_data() {
        if (_change_stage_in_game_after_loading_game_data_requested) {
            return;
        }

        _change_stage_in_game_after_loading_game_data_requested = true;
        _events.push_back(SequenceRequestEvent{
            .kind = SequenceRequestKind::ChangeStageInGameAfterLoadingGameData,
            .frame_index = _frame_index,
        });
    }

    bool SequenceRequestService::consume_change_stage_in_game_after_loading_game_data_request() {
        if (!_change_stage_in_game_after_loading_game_data_requested) {
            return false;
        }

        _change_stage_in_game_after_loading_game_data_requested = false;
        return true;
    }

    bool SequenceRequestService::is_change_stage_in_game_after_loading_game_data_requested() const {
        return _change_stage_in_game_after_loading_game_data_requested;
    }

    std::span<const SequenceRequestEvent> SequenceRequestService::events() const {
        return _events;
    }

    SaveDataService::SaveDataService() = default;

    void SaveDataService::write_file(std::string_view name, std::span<const std::uint8_t> bytes) {
        if (!_host_directory.has_value()) {
            throw std::logic_error("Save persistence is unavailable without a configured host directory");
        }
        const auto file_name = NandFileSystemService::file_name(_nand.normalize_path(name));
        if (file_name != SAVE_DATA_CONTAINER_NAME && save_data_file_size(file_name).has_value()) {
            throw std::invalid_argument("Retail save members may only be persisted inside GameData.bin");
        }
        if (file_name == SAVE_DATA_CONTAINER_NAME && !decode_game_data_container(bytes).has_value()) {
            throw std::invalid_argument("GameData.bin is not a valid retail big-endian container");
        }

        const auto key = std::string(name);
        _files[key] = std::vector<std::uint8_t>(bytes.begin(), bytes.end());
        if (file_name == SAVE_DATA_CONTAINER_NAME) {
            _has_valid_game_data_container = true;
        }
        write_host_file(name, bytes);
    }

    std::optional<std::vector<std::uint8_t>> SaveDataService::read_file(std::string_view name) const {
        if (auto it = _files.find(std::string(name)); it != _files.end()) {
            return it->second;
        }

        return std::nullopt;
    }

    void SaveDataService::write_nand_file(std::string_view name, std::span<const std::uint8_t> bytes) {
        if (!_host_directory.has_value()) {
            throw std::logic_error("NAND save persistence is unavailable without a configured host directory");
        }
        const auto file_name = NandFileSystemService::file_name(_nand.normalize_path(name));
        const auto wii_bytes =
            file_name == SAVE_DATA_CONTAINER_NAME ? host_save_data_container_for_retail(bytes) : std::vector<std::uint8_t>{};
        const auto payload = file_name == SAVE_DATA_CONTAINER_NAME ? std::span<const std::uint8_t>(wii_bytes.data(), wii_bytes.size()) : bytes;
        if (file_name == SAVE_DATA_CONTAINER_NAME && !decode_game_data_container(payload).has_value()) {
            throw std::invalid_argument("Translated GameData.bin does not match the retail container layout");
        }
        _nand.write_file(name, payload);
        if (file_name == SAVE_DATA_CONTAINER_NAME) {
            write_file(file_name, payload);
            return;
        }

        write_file(file_name, bytes);
    }

    std::optional<std::vector<std::uint8_t>> SaveDataService::read_nand_file(std::string_view name) const {
        const auto file_name = NandFileSystemService::file_name(_nand.normalize_path(name));
        auto bytes = _nand.read_file(name);
        if (!bytes.has_value()) {
            bytes = read_file(file_name);
            if (!bytes.has_value()) {
                return std::nullopt;
            }
        }

        if (file_name == SAVE_DATA_CONTAINER_NAME) {
            if (!decode_game_data_container(*bytes).has_value()) {
                throw std::runtime_error("Persisted GameData.bin is malformed or uses a non-retail byte order");
            }
            return retail_save_data_container_for_host(*bytes);
        }
        return bytes;
    }

    NandFileSystemService &SaveDataService::nand() {
        return _nand;
    }

    const NandFileSystemService &SaveDataService::nand() const {
        return _nand;
    }

    bool SaveDataService::exists(std::string_view name) const {
        return _files.contains(std::string(name)) || _nand.exists(name);
    }

    bool SaveDataService::erase(std::string_view name) {
        if (!_host_directory.has_value()) {
            throw std::logic_error("Save persistence is unavailable without a configured host directory");
        }
        const auto erased = _files.erase(std::string(name)) != 0U;
        const auto nand_erased = _nand.erase(name);
        erase_host_file(name);
        if (NandFileSystemService::file_name(_nand.normalize_path(name)) == SAVE_DATA_CONTAINER_NAME) {
            _has_valid_game_data_container = false;
        }
        return erased || nand_erased;
    }

    std::size_t SaveDataService::file_count() const {
        return _files.size();
    }

    void SaveDataService::set_host_directory(std::filesystem::path directory) {
        _host_directory = weakly_canonical_or_normal(std::move(directory));
        load_host_files();
    }

    const std::optional<std::filesystem::path> &SaveDataService::host_directory() const {
        return _host_directory;
    }

    void SaveDataService::load_host_files() {
        if (!_host_directory.has_value()) {
            return;
        }

        std::error_code error{};
        std::filesystem::create_directories(*_host_directory, error);
        if (error) {
            throw std::runtime_error("Cannot create save directory " + _host_directory->string());
        }

        _files.clear();
        _nand.clear();
        _has_valid_game_data_container = false;
        for (const auto &entry : std::filesystem::recursive_directory_iterator(*_host_directory, error)) {
            if (error) {
                throw std::runtime_error("Cannot scan save directory " + _host_directory->string());
            }
            if (!entry.is_regular_file(error)) {
                continue;
            }

            const auto relative = std::filesystem::relative(entry.path(), *_host_directory, error);
            if (error || relative.empty()) {
                continue;
            }
            const auto relative_name = relative.generic_string();
            const auto file_name = NandFileSystemService::file_name(_nand.normalize_path(relative_name));
            if (file_name != SAVE_DATA_CONTAINER_NAME && save_data_file_size(file_name).has_value()) {
                continue;
            }
            auto bytes = read_binary_file(entry.path());
            _nand.write_file(relative_name, bytes);
            _files[relative_name] = std::move(bytes);
        }

        if (const auto container = read_file(SAVE_DATA_CONTAINER_NAME)) {
            _has_valid_game_data_container = decode_game_data_container(*container).has_value();
        }
    }

    void SaveDataService::flush_host_files() {
        if (!_host_directory.has_value()) {
            throw std::logic_error("Save persistence is unavailable without a configured host directory");
        }

        for (const auto &[name, bytes] : _files) {
            write_host_file(name, bytes);
        }
    }

    bool SaveDataService::has_valid_game_data_container() const {
        return _has_valid_game_data_container;
    }

    std::filesystem::path SaveDataService::host_file_path(std::string_view name) const {
        if (!_host_directory.has_value()) {
            return {};
        }

        auto relative = std::filesystem::path(std::string(name)).lexically_normal();
        if (relative.empty() || relative.is_absolute()) {
            throw std::runtime_error("Invalid save file name " + std::string(name));
        }

        for (const auto &part : relative) {
            if (part == "..") {
                throw std::runtime_error("Invalid save file name " + std::string(name));
            }
        }

        return *_host_directory / relative;
    }

    void SaveDataService::write_host_file(std::string_view name, std::span<const std::uint8_t> bytes) const {
        if (!_host_directory.has_value()) {
            throw std::logic_error("Save persistence is unavailable without a configured host directory");
        }

        write_binary_file(host_file_path(name), bytes);
    }

    void SaveDataService::erase_host_file(std::string_view name) const {
        if (!_host_directory.has_value()) {
            throw std::logic_error("Save persistence is unavailable without a configured host directory");
        }

        std::error_code error{};
        std::filesystem::remove(host_file_path(name), error);
    }

    std::optional<std::map<std::string, std::vector<std::uint8_t>>> SaveDataService::decode_game_data_container(std::span<const std::uint8_t> bytes) const {
        if (bytes.size() < SAVE_DATA_HEADER_SIZE + SAVE_DATA_FILE_INFO_SIZE) {
            return std::nullopt;
        }

        constexpr auto byte_order = SaveDataByteOrder::BigEndian;
        const auto expected_check_sum = read_save_u32(bytes, 0U, byte_order);
        const auto version = read_save_u32(bytes, 4U, byte_order);
        const auto file_count = read_save_u32(bytes, 8U, byte_order);
        const auto data_size = read_save_u32(bytes, 12U, byte_order);
        if (version != SAVE_DATA_VERSION || file_count == 0U || file_count >= 24U ||
            data_size < SAVE_DATA_HEADER_SIZE + file_count * SAVE_DATA_FILE_INFO_SIZE) {
            return std::nullopt;
        }

        const auto aligned_size = align_save_data_size(data_size);
        if (aligned_size > bytes.size()) {
            return std::nullopt;
        }

        const auto actual_check_sum =
            save_check_sum(bytes.subspan(sizeof(std::uint32_t), data_size - sizeof(std::uint32_t)), byte_order);
        if (expected_check_sum != actual_check_sum) {
            return std::nullopt;
        }

        auto decoded = std::map<std::string, std::vector<std::uint8_t>>{};
        for (auto file_index = std::uint32_t{}; file_index < file_count; ++file_index) {
            const auto info_offset = SAVE_DATA_HEADER_SIZE + static_cast<std::size_t>(file_index) * SAVE_DATA_FILE_INFO_SIZE;
            auto name_size = std::size_t{};
            while (name_size < SAVE_DATA_FILE_NAME_SIZE && bytes[info_offset + name_size] != 0U) {
                ++name_size;
            }

            const auto name = std::string(reinterpret_cast<const char*>(bytes.data() + info_offset), name_size);
            const auto file_size = save_data_file_size(name);
            const auto data_offset = read_save_u32(bytes, info_offset + SAVE_DATA_FILE_NAME_SIZE, byte_order);
            if (name.empty() || !file_size.has_value() || data_offset > data_size || *file_size > data_size - data_offset ||
                decoded.contains(name)) {
                return std::nullopt;
            }

            decoded[name] =
                std::vector<std::uint8_t>(bytes.begin() + data_offset, bytes.begin() + data_offset + *file_size);
        }
        return decoded;
    }

    void MessageService::set_message(std::string_view tag, std::string_view text) {
        set_message(tag, smgpc::resource::utf16_from_utf8_lossy(text));
    }

    void MessageService::set_message(std::string_view tag, std::u16string_view text) {
        _messages[std::string(tag)] = MessageText{
            .raw_utf16 = std::u16string(text),
            .utf16 = std::u16string(text),
            .utf8 = smgpc::resource::utf8_from_utf16_lossy(text),
            .info = {},
            .control_tags = {},
        };
    }

    std::size_t MessageService::load_message_archive(const smgpc::resource::RarcArchive &archive) {
        const auto messages = smgpc::resource::BmgMessageArchive::from_message_archive(archive);
        for (const auto &message : messages.messages()) {
            _messages[message.id] = MessageText{
                .raw_utf16 = message.raw_text,
                .utf16 = message.display_text,
                .utf8 = smgpc::resource::utf8_from_utf16_lossy(message.display_text),
                .info = message.info,
                .control_tags = message.control_tags,
            };
        }

        return messages.message_count();
    }

    std::size_t MessageService::message_count() const {
        return _messages.size();
    }

    const std::string *MessageService::message(std::string_view tag) const {
        if (auto it = _messages.find(std::string(tag)); it != _messages.end()) {
            return &it->second.utf8;
        }

        return nullptr;
    }

    const std::u16string *MessageService::message_utf16(std::string_view tag) const {
        if (auto it = _messages.find(std::string(tag)); it != _messages.end()) {
            return &it->second.utf16;
        }

        return nullptr;
    }

    const std::u16string *MessageService::message_raw_utf16(std::string_view tag) const {
        if (auto it = _messages.find(std::string(tag)); it != _messages.end()) {
            return &it->second.raw_utf16;
        }

        return nullptr;
    }

    const smgpc::resource::BmgMessageInfo *MessageService::message_info(std::string_view tag) const {
        if (auto it = _messages.find(std::string(tag)); it != _messages.end()) {
            return &it->second.info;
        }

        return nullptr;
    }

    const std::vector<smgpc::resource::BmgControlTag> *MessageService::message_control_tags(std::string_view tag) const {
        if (auto it = _messages.find(std::string(tag)); it != _messages.end()) {
            return &it->second.control_tags;
        }

        return nullptr;
    }

    std::u16string MessageService::format_message_utf16(std::string_view tag, std::span<const smgpc::resource::BmgFormatArg> args) const {
        const auto *raw_text = message_raw_utf16(tag);
        if (raw_text == nullptr) {
            return {};
        }

        return smgpc::resource::format_bmg_text(*raw_text, args);
    }

    void SceneLightService::clear() {
        _lights = {};
    }

    void SceneLightService::clear_light(std::size_t index) {
        if (index >= _lights.size()) {
            return;
        }

        _lights[index] = smgpc::render::GXLightState{};
    }

    void SceneLightService::set_light(std::size_t index, const smgpc::render::GXLightState &light) {
        if (index >= _lights.size()) {
            return;
        }

        _lights[index] = light;
        _lights[index].loaded = true;
    }

    const smgpc::render::GXLightState *SceneLightService::light(std::size_t index) const {
        if (index >= _lights.size() || !_lights[index].loaded) {
            return nullptr;
        }

        return &_lights[index];
    }

    std::span<const smgpc::render::GXLightState> SceneLightService::lights() const {
        return _lights;
    }

    std::uint8_t SceneLightService::loaded_mask() const {
        auto mask = std::uint8_t{};
        for (auto index = 0zu; index < _lights.size(); ++index) {
            if (_lights[index].loaded) {
                mask |= static_cast<std::uint8_t>(1U << index);
            }
        }
        return mask;
    }

}  // namespace smgpc::runtime
