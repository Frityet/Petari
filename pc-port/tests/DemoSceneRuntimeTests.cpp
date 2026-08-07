#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "compat/PlayerUtilCompat.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void write_be16(std::vector<std::uint8_t> &bytes, std::size_t offset,
                    std::uint16_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value);
    }

    void write_be24(std::vector<std::uint8_t> &bytes, std::size_t offset,
                    std::uint32_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 16U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 2U] = static_cast<std::uint8_t>(value);
    }

    void write_be32(std::vector<std::uint8_t> &bytes, std::size_t offset,
                    std::uint32_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 24U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value >> 16U);
        bytes[offset + 2U] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 3U] = static_cast<std::uint8_t>(value);
    }

    void write_be_float(std::vector<std::uint8_t> &bytes, std::size_t offset,
                        float value) {
        write_be32(bytes, offset, std::bit_cast<std::uint32_t>(value));
    }

    void write_bcsv_field(std::vector<std::uint8_t> &bytes, std::size_t index,
                          std::string_view name, std::uint16_t offset,
                          smgpc::resource::BcsvFieldType type) {
        const auto descriptor = 0x10U + index * 0x0cU;
        write_be32(bytes, descriptor, smgpc::resource::jmap_hash(name));
        write_be32(bytes, descriptor + 0x04U, 0xffffffffU);
        write_be16(bytes, descriptor + 0x08U, offset);
        bytes[descriptor + 0x0aU] = 0U;
        bytes[descriptor + 0x0bU] = static_cast<std::uint8_t>(type);
    }

    [[nodiscard]] std::size_t align_up(std::size_t value, std::size_t alignment) {
        return (value + alignment - 1U) & ~(alignment - 1U);
    }

    [[nodiscard]] std::string raw_bytes(std::initializer_list<std::uint8_t> values) {
        auto result = std::string{};
        result.reserve(values.size());
        for (const auto value : values) {
            result.push_back(static_cast<char>(value));
        }
        return result;
    }

    struct ArchiveFile {
        std::string name;
        std::vector<std::uint8_t> data;
    };

    [[nodiscard]] smgpc::resource::RarcArchive make_rarc(
        std::span<const ArchiveFile> files) {
        constexpr auto header_size = std::size_t{0x20U};
        constexpr auto info_offset = std::size_t{0x20U};
        constexpr auto directory_offset = std::size_t{0x40U};
        constexpr auto file_entry_offset = std::size_t{0x50U};

        auto string_bytes = std::size_t{};
        auto data_bytes = std::size_t{};
        for (const auto &file : files) {
            string_bytes += file.name.size() + 1U;
            data_bytes += file.data.size();
        }
        const auto string_table_offset = align_up(file_entry_offset + files.size() * 0x14U, 0x20U);
        const auto file_data_offset = align_up(string_table_offset + string_bytes, 0x20U);
        auto bytes = std::vector<std::uint8_t>(file_data_offset + data_bytes, 0U);

        write_be32(bytes, 0x00U, 0x52415243U);
        write_be32(bytes, 0x04U, static_cast<std::uint32_t>(bytes.size()));
        write_be32(bytes, 0x08U, static_cast<std::uint32_t>(header_size));
        write_be32(bytes, 0x0cU, static_cast<std::uint32_t>(file_data_offset - header_size));
        write_be32(bytes, 0x10U, static_cast<std::uint32_t>(data_bytes));

        write_be32(bytes, info_offset + 0x00U, 1U);
        write_be32(bytes, info_offset + 0x04U,
                   static_cast<std::uint32_t>(directory_offset - info_offset));
        write_be32(bytes, info_offset + 0x08U, static_cast<std::uint32_t>(files.size()));
        write_be32(bytes, info_offset + 0x0cU,
                   static_cast<std::uint32_t>(file_entry_offset - info_offset));
        write_be32(bytes, info_offset + 0x10U, static_cast<std::uint32_t>(string_bytes));
        write_be32(bytes, info_offset + 0x14U,
                   static_cast<std::uint32_t>(string_table_offset - info_offset));

        write_be16(bytes, directory_offset + 0x0aU,
                   static_cast<std::uint16_t>(files.size()));
        write_be32(bytes, directory_offset + 0x0cU, 0U);

        auto string_offset = std::size_t{};
        auto data_offset = std::size_t{};
        for (auto index = std::size_t{}; index < files.size(); ++index) {
            const auto &file = files[index];
            const auto entry = file_entry_offset + index * 0x14U;
            write_be16(bytes, entry + 0x00U, static_cast<std::uint16_t>(index));
            write_be16(bytes, entry + 0x02U,
                       smgpc::resource::RarcArchive::hash_name(file.name));
            bytes[entry + 0x04U] = 1U;
            write_be24(bytes, entry + 0x05U, static_cast<std::uint32_t>(string_offset));
            write_be32(bytes, entry + 0x08U, static_cast<std::uint32_t>(data_offset));
            write_be32(bytes, entry + 0x0cU, static_cast<std::uint32_t>(file.data.size()));
            std::copy(file.name.begin(), file.name.end(),
                      bytes.begin() + static_cast<std::ptrdiff_t>(string_table_offset + string_offset));
            std::copy(file.data.begin(), file.data.end(),
                      bytes.begin() + static_cast<std::ptrdiff_t>(file_data_offset + data_offset));
            string_offset += file.name.size() + 1U;
            data_offset += file.data.size();
        }
        return smgpc::resource::RarcArchive::from_bytes(std::move(bytes));
    }

    [[nodiscard]] std::vector<std::uint8_t> make_empty_bcsv() {
        auto bytes = std::vector<std::uint8_t>(0x10U, 0U);
        write_be32(bytes, 0x08U, 0x10U);
        return bytes;
    }

    struct TimeRow {
        std::string part_name;
        std::int32_t total_step = 1;
        bool suspend = false;
    };

    [[nodiscard]] std::vector<std::uint8_t> make_time_bcsv(
        std::span<const TimeRow> rows) {
        constexpr auto field_count = std::size_t{3U};
        constexpr auto entry_size = std::size_t{12U};
        constexpr auto data_offset = std::size_t{0x10U + field_count * 0x0cU};

        auto strings = std::vector<std::uint8_t>{};
        auto string_offsets = std::vector<std::uint32_t>{};
        for (const auto &row : rows) {
            string_offsets.push_back(static_cast<std::uint32_t>(strings.size()));
            strings.insert(strings.end(), row.part_name.begin(), row.part_name.end());
            strings.push_back(0U);
        }

        auto bytes = std::vector<std::uint8_t>(
            data_offset + rows.size() * entry_size + strings.size(), 0U);
        write_be32(bytes, 0x00U, static_cast<std::uint32_t>(rows.size()));
        write_be32(bytes, 0x04U, field_count);
        write_be32(bytes, 0x08U, data_offset);
        write_be32(bytes, 0x0cU, entry_size);
        write_bcsv_field(bytes, 0U, "PartName", 0U,
                         smgpc::resource::BcsvFieldType::StringOffset);
        write_bcsv_field(bytes, 1U, "TotalStep", 4U,
                         smgpc::resource::BcsvFieldType::Int32);
        write_bcsv_field(bytes, 2U, "SuspendFlag", 8U,
                         smgpc::resource::BcsvFieldType::Int32);
        for (auto index = std::size_t{}; index < rows.size(); ++index) {
            const auto entry = data_offset + index * entry_size;
            write_be32(bytes, entry, string_offsets[index]);
            write_be32(bytes, entry + 4U,
                       static_cast<std::uint32_t>(rows[index].total_step));
            write_be32(bytes, entry + 8U, rows[index].suspend ? 1U : 0U);
        }
        std::copy(strings.begin(), strings.end(),
                  bytes.begin() + static_cast<std::ptrdiff_t>(
                                      data_offset + rows.size() * entry_size));
        return bytes;
    }

    [[nodiscard]] std::vector<std::uint8_t> make_time_bcsv(
        std::string_view part_name) {
        const auto rows = std::array{TimeRow{.part_name = std::string(part_name)}};
        return make_time_bcsv(rows);
    }

    struct SubPartRow {
        std::string sub_part_name;
        std::int32_t total_step = 1;
        std::string main_part_name;
        std::int32_t main_part_step = 1;
    };

    [[nodiscard]] std::vector<std::uint8_t> make_sub_part_bcsv(
        std::span<const SubPartRow> rows) {
        constexpr auto field_count = std::size_t{4U};
        constexpr auto entry_size = std::size_t{16U};
        constexpr auto data_offset = std::size_t{0x10U + field_count * 0x0cU};

        auto strings = std::vector<std::uint8_t>{};
        const auto add_string = [&strings](std::string_view value) {
            const auto offset = static_cast<std::uint32_t>(strings.size());
            strings.insert(strings.end(), value.begin(), value.end());
            strings.push_back(0U);
            return offset;
        };
        auto offsets = std::vector<std::pair<std::uint32_t, std::uint32_t>>{};
        for (const auto &row : rows) {
            offsets.emplace_back(add_string(row.sub_part_name),
                                 add_string(row.main_part_name));
        }

        auto bytes = std::vector<std::uint8_t>(
            data_offset + rows.size() * entry_size + strings.size(), 0U);
        write_be32(bytes, 0x00U, static_cast<std::uint32_t>(rows.size()));
        write_be32(bytes, 0x04U, field_count);
        write_be32(bytes, 0x08U, data_offset);
        write_be32(bytes, 0x0cU, entry_size);
        write_bcsv_field(bytes, 0U, "SubPartName", 0U,
                         smgpc::resource::BcsvFieldType::StringOffset);
        write_bcsv_field(bytes, 1U, "SubPartTotalStep", 4U,
                         smgpc::resource::BcsvFieldType::Int32);
        write_bcsv_field(bytes, 2U, "MainPartName", 8U,
                         smgpc::resource::BcsvFieldType::StringOffset);
        write_bcsv_field(bytes, 3U, "MainPartStep", 12U,
                         smgpc::resource::BcsvFieldType::Int32);
        for (auto index = std::size_t{}; index < rows.size(); ++index) {
            const auto entry = data_offset + index * entry_size;
            write_be32(bytes, entry, offsets[index].first);
            write_be32(bytes, entry + 4U,
                       static_cast<std::uint32_t>(rows[index].total_step));
            write_be32(bytes, entry + 8U, offsets[index].second);
            write_be32(bytes, entry + 12U,
                       static_cast<std::uint32_t>(rows[index].main_part_step));
        }
        std::copy(strings.begin(), strings.end(),
                  bytes.begin() + static_cast<std::ptrdiff_t>(
                                      data_offset + rows.size() * entry_size));
        return bytes;
    }

    [[nodiscard]] std::vector<std::uint8_t> make_player_bcsv(
        std::string_view part_name, std::string_view position_name,
        std::string_view bck_name) {
        constexpr auto field_count = std::size_t{3U};
        constexpr auto entry_size = std::size_t{12U};
        constexpr auto data_offset = std::size_t{0x10U + field_count * 0x0cU};
        auto strings = std::vector<std::uint8_t>{};
        const auto add_string = [&strings](std::string_view value) {
            const auto offset = static_cast<std::uint32_t>(strings.size());
            strings.insert(strings.end(), value.begin(), value.end());
            strings.push_back(0U);
            return offset;
        };
        const auto part_offset = add_string(part_name);
        const auto position_offset = add_string(position_name);
        const auto bck_offset = add_string(bck_name);
        auto bytes = std::vector<std::uint8_t>(
            data_offset + entry_size + strings.size(), 0U);
        write_be32(bytes, 0x00U, 1U);
        write_be32(bytes, 0x04U, field_count);
        write_be32(bytes, 0x08U, data_offset);
        write_be32(bytes, 0x0cU, entry_size);
        write_bcsv_field(bytes, 0U, "PartName", 0U,
                         smgpc::resource::BcsvFieldType::StringOffset);
        write_bcsv_field(bytes, 1U, "PosName", 4U,
                         smgpc::resource::BcsvFieldType::StringOffset);
        write_bcsv_field(bytes, 2U, "BckName", 8U,
                         smgpc::resource::BcsvFieldType::StringOffset);
        write_be32(bytes, data_offset, part_offset);
        write_be32(bytes, data_offset + 4U, position_offset);
        write_be32(bytes, data_offset + 8U, bck_offset);
        std::copy(strings.begin(), strings.end(),
                  bytes.begin() + static_cast<std::ptrdiff_t>(
                                      data_offset + entry_size));
        return bytes;
    }

    struct ActionRow {
        std::string part_name;
        std::string cast_name;
        std::int32_t cast_id = -1;
        std::int32_t action_type = 0;
        std::string position_name;
        std::string animation_name;
    };

    [[nodiscard]] JMapInfo make_general_pos_info(
        std::string_view name, const std::array<float, 3U> &position,
        const std::array<float, 3U> &rotation) {
        constexpr auto field_count = std::size_t{7U};
        constexpr auto entry_size = std::size_t{28U};
        constexpr auto data_offset = std::size_t{0x10U + field_count * 0x0cU};
        auto bytes = std::vector<std::uint8_t>(data_offset + entry_size + name.size() + 1U, 0U);
        write_be32(bytes, 0x00U, 1U);
        write_be32(bytes, 0x04U, field_count);
        write_be32(bytes, 0x08U, data_offset);
        write_be32(bytes, 0x0cU, entry_size);
        write_bcsv_field(bytes, 0U, "PosName", 0U,
                         smgpc::resource::BcsvFieldType::StringOffset);
        constexpr auto components = std::array<std::string_view, 6U>{
            "pos_x",
            "pos_y",
            "pos_z",
            "dir_x",
            "dir_y",
            "dir_z",
        };
        for (auto index = std::size_t{}; index < components.size(); ++index) {
            write_bcsv_field(bytes, index + 1U, components[index],
                             static_cast<std::uint16_t>((index + 1U) * 4U),
                             smgpc::resource::BcsvFieldType::Float);
        }
        write_be32(bytes, data_offset, 0U);
        for (auto index = std::size_t{}; index < 3U; ++index) {
            write_be_float(bytes, data_offset + (index + 1U) * 4U, position[index]);
            write_be_float(bytes, data_offset + (index + 4U) * 4U, rotation[index]);
        }
        std::copy(name.begin(), name.end(),
                  bytes.begin() + static_cast<std::ptrdiff_t>(data_offset + entry_size));
        return JMapInfo::from_bcsv(bytes);
    }

    [[nodiscard]] JMapInfo make_zone_list_info(
        std::span<const std::string_view> zone_names) {
        constexpr auto field_count = std::size_t{1U};
        constexpr auto entry_size = std::size_t{4U};
        constexpr auto data_offset = std::size_t{0x10U + field_count * 0x0cU};

        auto strings = std::vector<std::uint8_t>{};
        auto string_offsets = std::vector<std::uint32_t>{};
        for (const auto zone_name : zone_names) {
            string_offsets.push_back(static_cast<std::uint32_t>(strings.size()));
            strings.insert(strings.end(), zone_name.begin(), zone_name.end());
            strings.push_back(0U);
        }

        auto bytes = std::vector<std::uint8_t>(
            data_offset + zone_names.size() * entry_size + strings.size(), 0U);
        write_be32(bytes, 0x00U, static_cast<std::uint32_t>(zone_names.size()));
        write_be32(bytes, 0x04U, field_count);
        write_be32(bytes, 0x08U, data_offset);
        write_be32(bytes, 0x0cU, entry_size);
        write_bcsv_field(bytes, 0U, "ZoneName", 0U,
                         smgpc::resource::BcsvFieldType::StringOffset);
        for (auto index = std::size_t{}; index < zone_names.size(); ++index) {
            write_be32(bytes, data_offset + index * entry_size,
                       string_offsets[index]);
        }
        std::copy(strings.begin(), strings.end(),
                  bytes.begin() + static_cast<std::ptrdiff_t>(
                                      data_offset + zone_names.size() * entry_size));
        return JMapInfo::from_bcsv(bytes);
    }

    [[nodiscard]] std::vector<std::uint8_t> make_action_bcsv(
        std::span<const ActionRow> rows) {
        constexpr auto field_count = std::size_t{6U};
        constexpr auto entry_size = std::size_t{24U};
        constexpr auto data_offset = std::size_t{0x10U + field_count * 0x0cU};

        auto strings = std::vector<std::uint8_t>{};
        const auto add_string = [&strings](std::string_view value) {
            const auto offset = static_cast<std::uint32_t>(strings.size());
            strings.insert(strings.end(), value.begin(), value.end());
            strings.push_back(0U);
            return offset;
        };
        struct StringOffsets {
            std::uint32_t part = 0U;
            std::uint32_t cast = 0U;
            std::uint32_t position = 0U;
            std::uint32_t animation = 0U;
        };
        auto offsets = std::vector<StringOffsets>{};
        for (const auto &row : rows) {
            offsets.push_back(StringOffsets{
                .part = add_string(row.part_name),
                .cast = add_string(row.cast_name),
                .position = add_string(row.position_name),
                .animation = add_string(row.animation_name),
            });
        }

        auto bytes = std::vector<std::uint8_t>(data_offset + rows.size() * entry_size + strings.size(), 0U);
        write_be32(bytes, 0x00U, static_cast<std::uint32_t>(rows.size()));
        write_be32(bytes, 0x04U, field_count);
        write_be32(bytes, 0x08U, data_offset);
        write_be32(bytes, 0x0cU, entry_size);
        write_bcsv_field(bytes, 0U, "CastID", 8U, smgpc::resource::BcsvFieldType::Int32);
        write_bcsv_field(bytes, 1U, "PartName", 0U,
                         smgpc::resource::BcsvFieldType::StringOffset);
        write_bcsv_field(bytes, 2U, "ActionType", 12U,
                         smgpc::resource::BcsvFieldType::Int32);
        write_bcsv_field(bytes, 3U, "CastName", 4U,
                         smgpc::resource::BcsvFieldType::StringOffset);
        write_bcsv_field(bytes, 4U, "PosName", 16U,
                         smgpc::resource::BcsvFieldType::StringOffset);
        write_bcsv_field(bytes, 5U, "AnimName", 20U,
                         smgpc::resource::BcsvFieldType::StringOffset);
        for (auto index = std::size_t{}; index < rows.size(); ++index) {
            const auto entry = data_offset + index * entry_size;
            write_be32(bytes, entry, offsets[index].part);
            write_be32(bytes, entry + 4U, offsets[index].cast);
            write_be32(bytes, entry + 8U, static_cast<std::uint32_t>(rows[index].cast_id));
            write_be32(bytes, entry + 12U, static_cast<std::uint32_t>(rows[index].action_type));
            write_be32(bytes, entry + 16U, offsets[index].position);
            write_be32(bytes, entry + 20U, offsets[index].animation);
        }
        std::copy(strings.begin(), strings.end(),
                  bytes.begin() + static_cast<std::ptrdiff_t>(data_offset + rows.size() * entry_size));
        return bytes;
    }

    struct WipeRow {
        std::string part_name;
        std::string wipe_name;
        std::int32_t wipe_type = 0;
        std::int32_t wipe_frame = -1;
    };

    [[nodiscard]] std::vector<std::uint8_t> make_wipe_bcsv(
        std::span<const WipeRow> rows) {
        constexpr auto field_count = std::size_t{4U};
        constexpr auto entry_size = std::size_t{16U};
        constexpr auto data_offset = std::size_t{0x10U + field_count * 0x0cU};
        auto strings = std::vector<std::uint8_t>{};
        const auto add_string = [&strings](std::string_view value) {
            const auto offset = static_cast<std::uint32_t>(strings.size());
            strings.insert(strings.end(), value.begin(), value.end());
            strings.push_back(0U);
            return offset;
        };
        auto offsets = std::vector<std::pair<std::uint32_t, std::uint32_t>>{};
        for (const auto &row : rows) {
            offsets.emplace_back(add_string(row.part_name), add_string(row.wipe_name));
        }
        auto bytes = std::vector<std::uint8_t>(
            data_offset + rows.size() * entry_size + strings.size(), 0U);
        write_be32(bytes, 0x00U, static_cast<std::uint32_t>(rows.size()));
        write_be32(bytes, 0x04U, field_count);
        write_be32(bytes, 0x08U, data_offset);
        write_be32(bytes, 0x0cU, entry_size);
        write_bcsv_field(bytes, 0U, "PartName", 0U,
                         smgpc::resource::BcsvFieldType::StringOffset);
        write_bcsv_field(bytes, 1U, "WipeName", 4U,
                         smgpc::resource::BcsvFieldType::StringOffset);
        write_bcsv_field(bytes, 2U, "WipeType", 8U,
                         smgpc::resource::BcsvFieldType::Int32);
        write_bcsv_field(bytes, 3U, "WipeFrame", 12U,
                         smgpc::resource::BcsvFieldType::Int32);
        for (auto index = std::size_t{}; index < rows.size(); ++index) {
            const auto entry = data_offset + index * entry_size;
            write_be32(bytes, entry, offsets[index].first);
            write_be32(bytes, entry + 4U, offsets[index].second);
            write_be32(bytes, entry + 8U,
                       static_cast<std::uint32_t>(rows[index].wipe_type));
            write_be32(bytes, entry + 12U,
                       static_cast<std::uint32_t>(rows[index].wipe_frame));
        }
        std::copy(strings.begin(), strings.end(),
                  bytes.begin() + static_cast<std::ptrdiff_t>(
                                      data_offset + rows.size() * entry_size));
        return bytes;
    }

    struct DefinitionRow {
        std::string object_name = "DemoGroup";
        std::string demo_name;
        std::optional<std::string> sheet_name;
        std::int32_t zone_id = 0;
        std::int32_t link_id = -1;
        std::array<std::int32_t, 5U> switches{-1, -1, -1, -1, -1};
        std::string table_path = "jmp/placement/layera/DemoObjInfo";
    };

    [[nodiscard]] smgpc::scene::StagePlacementObject make_definition_placement(
        const DefinitionRow &row) {
        const auto field_count = row.sheet_name.has_value() ? 9U : 8U;
        const auto entry_size = field_count * 4U;
        const auto data_offset = 0x10U + field_count * 0x0cU;
        auto strings = std::vector<std::uint8_t>{};
        const auto add_string = [&strings](std::string_view value) {
            const auto offset = static_cast<std::uint32_t>(strings.size());
            strings.insert(strings.end(), value.begin(), value.end());
            strings.push_back(0U);
            return offset;
        };
        const auto object_offset = add_string(row.object_name);
        const auto demo_offset = add_string(row.demo_name);
        const auto sheet_offset = row.sheet_name.has_value() ? add_string(*row.sheet_name) : 0U;
        auto bytes = std::vector<std::uint8_t>(data_offset + entry_size + strings.size(), 0U);
        write_be32(bytes, 0x00U, 1U);
        write_be32(bytes, 0x04U, field_count);
        write_be32(bytes, 0x08U, data_offset);
        write_be32(bytes, 0x0cU, entry_size);

        auto field = std::size_t{};
        write_bcsv_field(bytes, field++, "DemoName", 4U,
                         smgpc::resource::BcsvFieldType::StringOffset);
        write_bcsv_field(bytes, field++, "name", 0U,
                         smgpc::resource::BcsvFieldType::StringOffset);
        auto integer_offset = std::uint16_t{8U};
        if (row.sheet_name.has_value()) {
            write_bcsv_field(bytes, field++, "TimeSheetName", integer_offset,
                             smgpc::resource::BcsvFieldType::StringOffset);
            integer_offset += 4U;
        }
        write_bcsv_field(bytes, field++, "l_id", integer_offset,
                         smgpc::resource::BcsvFieldType::Int32);
        const auto link_offset = integer_offset;
        integer_offset += 4U;
        constexpr auto switch_names = std::array<std::string_view, 5U>{
            "SW_APPEAR",
            "SW_DEAD",
            "SW_A",
            "SW_B",
            "SW_SLEEP",
        };
        for (const auto name : switch_names) {
            write_bcsv_field(bytes, field++, name, integer_offset,
                             smgpc::resource::BcsvFieldType::Int32);
            integer_offset += 4U;
        }

        write_be32(bytes, data_offset, object_offset);
        write_be32(bytes, data_offset + 4U, demo_offset);
        if (row.sheet_name.has_value()) {
            write_be32(bytes, data_offset + 8U, sheet_offset);
        }
        write_be32(bytes, data_offset + link_offset, static_cast<std::uint32_t>(row.link_id));
        for (auto index = std::size_t{}; index < row.switches.size(); ++index) {
            write_be32(bytes, data_offset + link_offset + 4U + index * 4U,
                       static_cast<std::uint32_t>(row.switches[index]));
        }
        std::copy(strings.begin(), strings.end(),
                  bytes.begin() + static_cast<std::ptrdiff_t>(data_offset + entry_size));

        auto info = JMapInfo::from_bcsv(bytes);
        info.setPlacedZoneId(row.zone_id);
        return smgpc::scene::StagePlacementObject{
            .object_name = row.object_name,
            .table_path = row.table_path,
            .l_id = row.link_id,
            .zone_id = row.zone_id,
            .jmap_info = std::move(info),
            .jmap_entry_index = 0,
        };
    }

    [[nodiscard]] JMapInfo make_actor_info(std::optional<std::int32_t> group_id,
                                           std::int32_t cast_id, std::int32_t zone_id) {
        const auto field_count = group_id.has_value() ? 2U : 1U;
        const auto entry_size = field_count * 4U;
        const auto data_offset = 0x10U + field_count * 0x0cU;
        auto bytes = std::vector<std::uint8_t>(data_offset + entry_size, 0U);
        write_be32(bytes, 0x00U, 1U);
        write_be32(bytes, 0x04U, field_count);
        write_be32(bytes, 0x08U, data_offset);
        write_be32(bytes, 0x0cU, entry_size);
        if (group_id.has_value()) {
            write_bcsv_field(bytes, 0U, "DemoGroupId", 0U,
                             smgpc::resource::BcsvFieldType::Int32);
            write_bcsv_field(bytes, 1U, "CastId", 4U,
                             smgpc::resource::BcsvFieldType::Int32);
            write_be32(bytes, data_offset, static_cast<std::uint32_t>(*group_id));
            write_be32(bytes, data_offset + 4U, static_cast<std::uint32_t>(cast_id));
        } else {
            write_bcsv_field(bytes, 0U, "CastId", 0U,
                             smgpc::resource::BcsvFieldType::Int32);
            write_be32(bytes, data_offset, static_cast<std::uint32_t>(cast_id));
        }
        auto info = JMapInfo::from_bcsv(bytes);
        info.setPlacedZoneId(zone_id);
        return info;
    }

    [[nodiscard]] smgpc::resource::RarcArchive make_sheet_fixture() {
        const auto alpha_actions = std::array{
            ActionRow{.part_name = "shared", .cast_name = "Actor", .cast_id = -1, .action_type = 2},
            ActionRow{.part_name = "shared", .cast_name = "Actor", .cast_id = -1, .action_type = 3},
            ActionRow{.part_name = "specific", .cast_name = "Actor", .cast_id = 7, .action_type = 2},
            ActionRow{.part_name = "wrong-cast", .cast_name = "Actor", .cast_id = 8, .action_type = 2},
            ActionRow{.part_name = "other", .cast_name = "Other", .cast_id = -1, .action_type = 2},
        };
        const auto beta_actions = std::array{
            ActionRow{.part_name = "beta", .cast_name = "Actor", .cast_id = -1, .action_type = 2},
        };
        const auto files = std::array{
            ArchiveFile{.name = "DemoAlphaTime.bcsv", .data = make_time_bcsv("shared")},
            ArchiveFile{.name = "DemoAlphaAction.bcsv", .data = make_action_bcsv(alpha_actions)},
            ArchiveFile{.name = "DemoBetaTime.bcsv", .data = make_time_bcsv("beta")},
            ArchiveFile{.name = "DemoBetaAction.bcsv", .data = make_action_bcsv(beta_actions)},
            ArchiveFile{.name = "DemoEmptyTime.bcsv", .data = make_empty_bcsv()},
        };
        return make_rarc(files);
    }

    [[nodiscard]] smgpc::resource::RarcArchive make_clock_sheet_fixture() {
        const auto clock_time = std::array{
            TimeRow{.part_name = "intro", .total_step = 3},
            TimeRow{.part_name = "outro", .total_step = 2},
        };
        const auto clock_sub_parts = std::array{
            SubPartRow{.sub_part_name = "pulse", .total_step = 2, .main_part_name = "intro", .main_part_step = 1},
            SubPartRow{.sub_part_name = "chain", .total_step = 1, .main_part_name = "pulse", .main_part_step = 1},
            SubPartRow{.sub_part_name = "boundary", .total_step = 3, .main_part_name = "intro", .main_part_step = 2},
            SubPartRow{.sub_part_name = "dupe", .total_step = 2, .main_part_name = "intro", .main_part_step = 0},
            SubPartRow{.sub_part_name = "dupe", .total_step = 5, .main_part_name = "intro", .main_part_step = 0},
            SubPartRow{.sub_part_name = "intro", .total_step = 1, .main_part_name = "intro", .main_part_step = 0},
        };
        const auto other_time = std::array{
            TimeRow{.part_name = "other", .total_step = 2},
        };
        const auto suspend_time = std::array{
            TimeRow{.part_name = "hold", .total_step = 2, .suspend = true},
            TimeRow{.part_name = "unreachable", .total_step = 1},
        };
        const auto runaway_time = std::array{
            TimeRow{.part_name = "one", .total_step = 1},
        };
        const auto files = std::array{
            ArchiveFile{.name = "DemoClockTime.bcsv",
                        .data = make_time_bcsv(clock_time)},
            ArchiveFile{.name = "DemoClockSubPart.bcsv",
                        .data = make_sub_part_bcsv(clock_sub_parts)},
            ArchiveFile{.name = "DemoClockPlayer.bcsv",
                        .data = make_player_bcsv("intro", "ClockStart", "Wait")},
            ArchiveFile{.name = "DemoOtherTime.bcsv",
                        .data = make_time_bcsv(other_time)},
            ArchiveFile{.name = "DemoSuspendTime.bcsv",
                        .data = make_time_bcsv(suspend_time)},
            ArchiveFile{.name = "DemoRunawayTime.bcsv",
                        .data = make_time_bcsv(runaway_time)},
            ArchiveFile{.name = "DemoEmptyClockTime.bcsv", .data = make_empty_bcsv()},
        };
        return make_rarc(files);
    }

    [[nodiscard]] smgpc::resource::RarcArchive make_dispatch_sheet_fixture() {
        const auto time = std::array{
            TimeRow{.part_name = "first", .total_step = 2},
            TimeRow{.part_name = "one", .total_step = 1},
        };
        const auto actions = std::array{
            ActionRow{.part_name = "first", .cast_name = "Actor", .action_type = 0},
            ActionRow{.part_name = "first", .cast_name = "Actor", .action_type = 2},
            ActionRow{.part_name = "first", .cast_name = "Actor", .action_type = 3},
            ActionRow{.part_name = "first", .cast_name = "Actor", .action_type = 7},
            ActionRow{.part_name = "first", .cast_name = "Actor", .action_type = 99, .position_name = "Anchor", .animation_name = "Wave"},
            ActionRow{.part_name = "one", .cast_name = "Actor", .action_type = 1},
        };
        const auto wipes = std::array{
            WipeRow{.part_name = "first", .wipe_name = "CustomWipe", .wipe_type = 0, .wipe_frame = -1},
        };
        const auto files = std::array{
            ArchiveFile{.name = "DemoDispatchTime.bcsv", .data = make_time_bcsv(time)},
            ArchiveFile{.name = "DemoDispatchAction.bcsv", .data = make_action_bcsv(actions)},
            ArchiveFile{.name = "DemoDispatchWipe.bcsv", .data = make_wipe_bcsv(wipes)},
        };
        return make_rarc(files);
    }

    [[nodiscard]] smgpc::resource::RarcArchive make_required_talk_sheet_fixture() {
        const auto time = std::array{
            TimeRow{.part_name = "talk", .total_step = 2},
        };
        const auto actions = std::array{
            ActionRow{.part_name = "talk", .cast_name = "Actor", .action_type = 8},
        };
        const auto files = std::array{
            ArchiveFile{.name = "DemoRequiredTalkTime.bcsv", .data = make_time_bcsv(time)},
            ArchiveFile{.name = "DemoRequiredTalkAction.bcsv", .data = make_action_bcsv(actions)},
        };
        return make_rarc(files);
    }

    [[nodiscard]] std::vector<smgpc::scene::StagePlacementObject> make_definition_fixture() {
        auto placements = std::vector<smgpc::scene::StagePlacementObject>{};
        placements.push_back(make_definition_placement(DefinitionRow{
            .demo_name = "Alpha",
            .sheet_name = "Alpha",
            .zone_id = 1,
            .link_id = 3,
            .switches = {10, 11, 12, 13, 14},
        }));
        placements.push_back(make_definition_placement(DefinitionRow{
            .demo_name = "Beta",
            .sheet_name = "Beta",
            .zone_id = 2,
            .link_id = 3,
        }));
        placements.push_back(make_definition_placement(DefinitionRow{
            .demo_name = "Duplicate",
            .sheet_name = "Alpha",
            .zone_id = 4,
            .link_id = 7,
        }));
        placements.push_back(make_definition_placement(DefinitionRow{
            .demo_name = "Duplicate",
            .sheet_name = "Beta",
            .zone_id = 5,
            .link_id = 8,
        }));
        placements.push_back(make_definition_placement(DefinitionRow{
            .demo_name = raw_bytes({0x83U, 0x60U, 0x83U, 0x52U}),
            .sheet_name = "Missing",
            .zone_id = 6,
            .link_id = 1,
        }));
        placements.push_back(make_definition_placement(DefinitionRow{
            .demo_name = "EmptyDemo",
            .sheet_name = "Empty",
            .zone_id = 7,
            .link_id = 1,
        }));
        placements.push_back(make_definition_placement(DefinitionRow{
            .demo_name = "NoSheet",
            .sheet_name = std::nullopt,
            .zone_id = 8,
            .link_id = 1,
        }));
        placements.push_back(make_definition_placement(DefinitionRow{
            .object_name = "DemoSubGroup",
            .demo_name = "Alpha",
            .sheet_name = "Ignored",
            .zone_id = 9,
            .link_id = 4,
        }));
        placements.push_back(make_definition_placement(DefinitionRow{
            .object_name = "DemoSubGroup",
            .demo_name = "Orphan",
            .sheet_name = "Ignored",
            .zone_id = 10,
            .link_id = 5,
        }));
        placements.push_back(make_definition_placement(DefinitionRow{
            .object_name = "DemoGroup",
            .demo_name = "WrongTable",
            .sheet_name = "Alpha",
            .zone_id = 11,
            .link_id = 6,
            .table_path = "jmp/placement/common/objinfo",
        }));
        return placements;
    }

    [[nodiscard]] std::vector<smgpc::scene::StagePlacementObject>
    make_clock_definition_fixture() {
        auto placements = std::vector<smgpc::scene::StagePlacementObject>{};
        placements.push_back(make_definition_placement(DefinitionRow{
            .demo_name = "Clock",
            .sheet_name = "Clock",
            .zone_id = 20,
            .link_id = 30,
        }));
        placements.push_back(make_definition_placement(DefinitionRow{
            .demo_name = "Other",
            .sheet_name = "Other",
            .zone_id = 21,
            .link_id = 31,
        }));
        placements.push_back(make_definition_placement(DefinitionRow{
            .demo_name = "Suspend",
            .sheet_name = "Suspend",
            .zone_id = 22,
            .link_id = 32,
        }));
        placements.push_back(make_definition_placement(DefinitionRow{
            .demo_name = "Runaway",
            .sheet_name = "Runaway",
            .zone_id = 23,
            .link_id = 33,
        }));
        placements.push_back(make_definition_placement(DefinitionRow{
            .demo_name = "MissingClock",
            .sheet_name = "MissingClock",
            .zone_id = 24,
            .link_id = 34,
        }));
        placements.push_back(make_definition_placement(DefinitionRow{
            .demo_name = "EmptyClock",
            .sheet_name = "EmptyClock",
            .zone_id = 25,
            .link_id = 35,
        }));
        placements.push_back(make_definition_placement(DefinitionRow{
            .object_name = "DemoSubGroup",
            .demo_name = "ClockSubGroupOnly",
            .sheet_name = "Ignored",
            .zone_id = 26,
            .link_id = 36,
        }));
        return placements;
    }

    [[nodiscard]] std::vector<smgpc::scene::StagePlacementObject>
    make_dispatch_definition_fixture() {
        return {make_definition_placement(DefinitionRow{
            .demo_name = "Dispatch",
            .sheet_name = "Dispatch",
            .zone_id = 40,
            .link_id = 50,
        })};
    }

    [[nodiscard]] std::vector<smgpc::scene::StagePlacementObject>
    make_required_talk_definition_fixture() {
        return {make_definition_placement(DefinitionRow{
            .demo_name = "RequiredTalk",
            .sheet_name = "RequiredTalk",
            .zone_id = 41,
            .link_id = 51,
        })};
    }

    class TestNerve final : public Nerve {
    public:
        void execute(Spine *) const override {
        }
    };

    struct Counter {
        void increment() {
            ++value;
        }
        int value = 0;
    };

    struct ActionObserver {
        void observe() {
            ++calls;
            saw_appeared = actor != nullptr && !actor->isDead();
        }

        LiveActor *actor = nullptr;
        int calls = 0;
        bool saw_appeared = false;
    };

    void test_general_pos_table_order_and_zone_transform() {
        auto root_layer_b = smgpc::scene::StagePlacementTable{
            .stage_name = "Stage",
            .zone_name = "Stage",
            .category = "generalpos",
            .layer_name = "layerb",
            .table_name = "generalposinfo",
            .table_path = "jmp/generalpos/layerb/generalposinfo",
            .jmap_info = make_general_pos_info("Second", {4.0F, 5.0F, 6.0F},
                                               {7.0F, 8.0F, 9.0F}),
            .zone_id = 0,
            .layer_id = 1,
            .archive_entry_order = 1U,
        };
        auto root_layer_a = smgpc::scene::StagePlacementTable{
            .stage_name = "Stage",
            .zone_name = "Stage",
            .category = "generalpos",
            .layer_name = "layera",
            .table_name = "generalposinfo",
            .table_path = "jmp/generalpos/layera/generalposinfo",
            .jmap_info = make_general_pos_info("First", {1.0F, 2.0F, 3.0F},
                                               {10.0F, 20.0F, 30.0F}),
            .zone_id = 0,
            .layer_id = 0,
            .archive_entry_order = 2U,
        };
        auto child = smgpc::scene::StagePlacementTable{
            .stage_name = "Child",
            .zone_name = "Child",
            .category = "generalpos",
            .layer_name = "layera",
            .table_name = "generalposinfo",
            .table_path = "jmp/generalpos/layera/generalposinfo",
            .jmap_info = make_general_pos_info("ChildPoint", {1.0F, 2.0F, 3.0F},
                                               {0.0F, 0.0F, 0.0F}),
            .zone_id = 7,
            .layer_id = 0,
            .archive_entry_order = 0U,
            .zone_transform = smgpc::scene::StageZoneTransform::from_translation_rotation(
                {10.0F, 20.0F, 30.0F}, {0.0F, 0.0F, 90.0F}),
        };
        const auto tables = std::array{root_layer_b, root_layer_a, child};
        const auto positions = smgpc::scene::select_stage_general_positions(tables);
        const auto near = [](float lhs, float rhs) {
            return std::abs(lhs - rhs) < 0.001F;
        };
        require(positions.size() == 3U && positions[0].name == "First" &&
                    positions[1].name == "Second" && positions[2].name == "ChildPoint",
                "GeneralPos traversal must preserve root-before-child and retail layer order");
        require(near(positions[2].world_position[0U], 8.0F) &&
                    near(positions[2].world_position[1U], 21.0F) &&
                    near(positions[2].world_position[2U], 33.0F) &&
                    near(positions[2].world_rotation[0U], 0.0F) &&
                    near(positions[2].world_rotation[1U], 0.0F) &&
                    near(positions[2].world_rotation[2U], 90.0F),
                "child-zone GeneralPos data must use the real composed stage transform");
    }

    void test_zone_ids_are_only_read_from_zone_list() {
        constexpr auto zone_names = std::array<std::string_view, 3U>{
            "RootGalaxy",
            "KnownChild",
            "OtherChild",
        };
        const auto zone_list = make_zone_list_info(zone_names);

        require(smgpc::scene::find_stage_zone_id(zone_list, "RootGalaxy") ==
                        std::optional<s32>{0} &&
                    smgpc::scene::find_stage_zone_id(zone_list, "knownchild") ==
                        std::optional<s32>{1} &&
                    smgpc::scene::find_stage_zone_id(zone_list, "OTHERCHILD") ==
                        std::optional<s32>{2},
                "zone IDs must be the case-insensitive retail ZoneList row indices");
        require(!smgpc::scene::find_stage_zone_id(zone_list, "InventedChild").has_value() &&
                    !smgpc::scene::find_stage_zone_id(zone_list, "").has_value(),
                "an absent or empty zone name must not receive a synthetic zone ID");

        constexpr auto misplaced_root_names =
            std::array<std::string_view, 2U>{"ActualRoot", "RequestedRoot"};
        const auto misplaced_root = make_zone_list_info(misplaced_root_names);
        require(smgpc::scene::find_stage_zone_id(misplaced_root, "RequestedRoot") ==
                    std::optional<s32>{1},
                "zone lookup must preserve the authoritative row index instead of remapping a requested root to zero");
    }

    void test_definition_ingestion_and_dormant_sheets() {
        const auto archive = make_sheet_fixture();
        const auto placements = make_definition_fixture();
        const auto runtime = smgpc::compat::DemoSceneRuntime(archive, placements);

        require(runtime.definitions().size() == 7U,
                "only primary DemoGroup rows from demoobjinfo should own sheets");
        require(runtime.subgroups().size() == 2U,
                "DemoSubGroup rows should be retained separately without sheets");
        const auto &alpha = runtime.definitions()[0U];
        require(alpha.zone_id == 1 && alpha.group_link_id == 3 && alpha.demo_name == "Alpha" &&
                    alpha.time_sheet_name == "Alpha",
                "the first definition should preserve stable placement identity and names");
        require(alpha.switches.appear == 10 && alpha.switches.dead == 11 &&
                    alpha.switches.a == 12 && alpha.switches.b == 13 && alpha.switches.sleep == 14,
                "all five stage-switch identifiers should survive ingestion exactly");
        require(alpha.sheet.has_table(smgpc::compat::DemoSheetTable::Time) &&
                    alpha.sheet.has_table(smgpc::compat::DemoSheetTable::Action),
                "a populated definition should parse its own sheet family");

        const auto *missing = runtime.definition(4U);
        const auto *empty = runtime.definition(5U);
        const auto *no_sheet = runtime.definition(6U);
        require(missing != nullptr && missing->demo_name == "チコ" &&
                    !missing->sheet.has_table(smgpc::compat::DemoSheetTable::Time),
                "CP932 demo names should be owned as UTF-8 while missing Time remains dormant");
        require(empty != nullptr && empty->sheet.has_table(smgpc::compat::DemoSheetTable::Time) &&
                    empty->sheet.time_rows().empty(),
                "an empty Time table should remain distinct from a missing Time table");
        require(no_sheet != nullptr && no_sheet->time_sheet_name.empty() &&
                    !no_sheet->sheet.has_table(smgpc::compat::DemoSheetTable::Time),
                "a missing TimeSheetName field should retain a dormant, unaliased definition");
        require(runtime.find_definition("Duplicate") == std::optional<std::size_t>(2U),
                "duplicate exact names should retain stable first-match lookup");
        require(!runtime.find_definition("Missing").has_value() &&
                    !runtime.find_definition(raw_bytes({0x83U, 0x60U, 0x83U, 0x52U})).has_value(),
                "localized names must not gain TimeSheetName or raw-CP932 aliases");
    }

    void test_zone_scoping_cast_id_and_duplicate_choice() {
        const auto archive = make_sheet_fixture();
        const auto placements = make_definition_fixture();
        auto runtime = smgpc::compat::DemoSceneRuntime(archive, placements);

        auto zone_one_info = make_actor_info(3, -1, 1);
        auto zone_two_info = make_actor_info(3, 7, 2);
        auto first = LiveActor("Actor");
        auto second = LiveActor("Actor");
        require(MR::tryRegisterDemoCast(&first, JMapInfoIter(&zone_one_info, 0)) &&
                    runtime.has_cast(&first, "Alpha") && !runtime.has_cast(&first, "Beta") &&
                    MR::isDemoCast(&first, nullptr) && MR::isDemoCast(&first, "Alpha"),
                "automatic registration should match group IDs inside the actor's placed zone");
        require(runtime.cast_id(&first, 0U) == std::optional<std::int32_t>(-1),
                "CastId -1 should remain valid wildcard metadata");
        require(MR::tryRegisterDemoCast(&second, JMapInfoIter(&zone_two_info, 0)) &&
                    runtime.has_cast(&second, "Beta") && !runtime.has_cast(&second, "Alpha"),
                "the same group ID in another zone should select another definition");

        auto duplicate = LiveActor("Actor");
        auto duplicate_named_info = make_actor_info(99, 4, 99);
        auto duplicate_exact_info = make_actor_info(8, 9, 5);
        require(MR::tryRegisterDemoCast(&duplicate, "Duplicate",
                                        JMapInfoIter(&duplicate_named_info, 0)),
                "explicit registration should find the first exact localized name without zone filtering");
        require(runtime.cast_id(&duplicate, 2U) == std::optional<std::int32_t>(4) &&
                    !runtime.cast_id(&duplicate, 3U).has_value(),
                "named duplicate lookup should choose the first ordered definition");
        require(MR::tryRegisterDemoCast(&duplicate, JMapInfoIter(&duplicate_exact_info, 0)) &&
                    runtime.cast_id(&duplicate, 3U) == std::optional<std::int32_t>(9) &&
                    runtime.membership_count(&duplicate) == 2U,
                "automatic exact-index lookup should add the matching duplicate without erasing the first");

        auto invalid_named = LiveActor("Actor");
        require(MR::tryRegisterDemoCast(&invalid_named, "Beta", JMapInfoIter{}) &&
                    runtime.cast_id(&invalid_named, 1U) == std::optional<std::int32_t>(-1),
                "the original named overload should permit an invalid iterator and use CastId -1");

        auto missing_group_info = make_actor_info(std::nullopt, 1, 1);
        auto negative_group_info = make_actor_info(-1, 1, 1);
        auto missing = LiveActor("Actor");
        require(!MR::tryRegisterDemoCast(&missing, JMapInfoIter(&missing_group_info, 0)) &&
                    !MR::tryRegisterDemoCast(&missing, JMapInfoIter(&negative_group_info, 0)),
                "missing and -1 DemoGroupId metadata should both reject automatic registration");
    }

    void test_duplicate_registered_name_resolution_and_pause_scan() {
        const auto archive = make_sheet_fixture();
        const auto placements = make_definition_fixture();
        auto runtime = smgpc::compat::DemoSceneRuntime(archive, placements);

        auto second_info = make_actor_info(8, -1, 5);
        auto second_only = LiveActor("Actor");
        require(MR::tryRegisterDemoCast(&second_only,
                                        JMapInfoIter(&second_info, 0)) &&
                    MR::tryStartDemoRegistered(&second_only, "shared") &&
                    MR::getDemoPartStep("shared") == -1 &&
                    !MR::isDemoActiveRegistered(&second_only),
                "registered start must select actor membership first, then preserve the source's first exact-name executor resolution");

        MR::pauseTimeKeepDemo(&second_only);
        require(!runtime.definition(2U)->sheet.is_paused() &&
                    runtime.definition(3U)->sheet.is_paused(),
                "actor pause must scan the first registered executor whose duplicate name compares active, even when its pointer is inactive");
        runtime.movement();
        require(MR::getDemoPartStep("shared") == 0,
                "pausing the inactive same-name executor must not freeze the active first duplicate");
        MR::resumeTimeKeepDemo(&second_only);
        MR::endDemo(&second_only, "Duplicate");
    }

    void test_explicit_multi_membership_and_row_callbacks() {
        const auto archive = make_sheet_fixture();
        const auto placements = make_definition_fixture();
        auto runtime = smgpc::compat::DemoSceneRuntime(archive, placements);
        auto alpha_info = make_actor_info(3, 7, 1);
        auto beta_info = make_actor_info(3, -1, 2);
        auto actor = LiveActor("Actor");
        require(MR::tryRegisterDemoCast(&actor, JMapInfoIter(&alpha_info, 0)) &&
                    MR::tryRegisterDemoCast(&actor, "Beta", JMapInfoIter(&beta_info, 0)) &&
                    runtime.membership_count(&actor) == 2U,
                "explicit registration should add executor membership without erasing prior membership");
        require(runtime.cast_name(&actor, 0U) == "Actor",
                "cast targeting should retain the actor's exact runtime name");

        auto counter = Counter{};
        const auto functor = MR::Functor(&counter, &Counter::increment);
        const auto nerve = TestNerve{};
        require(MR::tryRegisterDemoActionFunctor(&actor, functor, "shared") &&
                    MR::tryRegisterDemoActionNerve(&actor, &nerve, "shared"),
                "try callbacks should succeed when the first executor has matching type capability");
        require(runtime.functor_count(&actor, "Alpha") == 2U &&
                    runtime.nerve_count(&actor, "Alpha") == 2U &&
                    runtime.action_count(&actor, "Alpha") == 2U,
                "functor and nerve pointers should coexist in each matching per-row slot");

        require(MR::tryRegisterDemoActionFunctor(&actor, functor, "not-a-part") &&
                    runtime.functor_count(&actor, "Alpha") == 2U,
                "try capability should remain true even when an exact part selector stores no row");
        require(MR::tryRegisterDemoActionFunctor(&actor, functor, nullptr) &&
                    runtime.functor_count(&actor, "Alpha") == 3U,
                "a null part selector should bind every targeted row, unlike an empty string");
        require(MR::tryRegisterDemoActionFunctorDirect(&actor, functor, "Beta", "beta") &&
                    runtime.functor_count(&actor, "Beta") == 1U &&
                    runtime.functor_count(&actor, "Alpha") == 3U,
                "direct callbacks should target one exact executor membership");

        auto repeated = LiveActor("Actor");
        auto alpha_cast_eight_info = make_actor_info(3, 8, 1);
        require(MR::tryRegisterDemoCast(&repeated, JMapInfoIter(&alpha_info, 0)) &&
                    MR::tryRegisterDemoCast(&repeated,
                                            JMapInfoIter(&alpha_cast_eight_info, 0)) &&
                    runtime.membership_count(&repeated) == 1U &&
                    MR::tryRegisterDemoActionFunctor(&repeated, functor, "specific") &&
                    MR::tryRegisterDemoActionFunctor(&repeated, functor, "wrong-cast") &&
                    runtime.functor_count(&repeated, "Alpha") == 2U,
                "repeat registration should retain the union of rows matched by each CastId");

        auto wrong_name = LiveActor("OtherName");
        require(MR::tryRegisterDemoCast(&wrong_name, JMapInfoIter(&alpha_info, 0)) &&
                    !MR::tryRegisterDemoActionFunctor(&wrong_name, functor, nullptr),
                "callback capability should require exact CastName targeting");
        require(smgpc::compat::registered_demo_membership_count(&actor) == 2U &&
                    smgpc::compat::registered_demo_action_count(&actor) == 4U,
                "the actor registry should aggregate source-faithful per-executor state");

        smgpc::compat::release_demo_runtime_state(&actor);
        require(runtime.membership_count(&actor) == 0U && runtime.action_count(&actor) == 0U,
                "actor teardown should remove every membership and callback clone safely");
    }

    void test_subgroup_forwarding_and_named_behavior() {
        const auto archive = make_sheet_fixture();
        const auto placements = make_definition_fixture();
        auto runtime = smgpc::compat::DemoSceneRuntime(archive, placements);

        auto subgroup_info = make_actor_info(4, 5, 9);
        auto forwarded = LiveActor("Actor");
        require(MR::tryRegisterDemoCast(&forwarded, JMapInfoIter(&subgroup_info, 0)) &&
                    runtime.subgroup_membership_count(&forwarded) == 1U &&
                    runtime.has_cast(&forwarded, "Alpha") &&
                    runtime.cast_id(&forwarded, 0U) == std::optional<std::int32_t>(5),
                "automatic subgroup registration should forward into its exact-name primary executor");

        auto orphan_info = make_actor_info(5, 2, 10);
        auto orphan = LiveActor("Actor");
        require(!MR::tryRegisterDemoCast(&orphan, JMapInfoIter(&orphan_info, 0)) &&
                    runtime.subgroup_membership_count(&orphan) == 1U &&
                    runtime.membership_count(&orphan) == 0U,
                "an orphan subgroup should retain its base membership but report forwarding failure");

        auto explicit_orphan = LiveActor("Actor");
        require(MR::tryRegisterDemoCast(&explicit_orphan, "Orphan", JMapInfoIter{}) &&
                    runtime.subgroup_membership_count(&explicit_orphan) == 1U &&
                    runtime.membership_count(&explicit_orphan) == 0U,
                "explicit subgroup registration should join only the subgroup without forwarding");
        auto explicit_shared = LiveActor("Actor");
        require(MR::tryRegisterDemoCast(&explicit_shared, "Alpha", JMapInfoIter{}) &&
                    runtime.membership_count(&explicit_shared) == 1U &&
                    runtime.subgroup_membership_count(&explicit_shared) == 0U,
                "named lookup should scan primary definitions before same-name subgroups");
    }

    void test_timekeeper_queries_subparts_and_natural_end() {
        const auto archive = make_clock_sheet_fixture();
        const auto placements = make_clock_definition_fixture();
        auto runtime = smgpc::compat::DemoSceneRuntime(archive, placements);
        auto actor_info = make_actor_info(30, -1, 20);
        auto actor = LiveActor("ClockActor");
        require(MR::tryRegisterDemoCast(&actor, JMapInfoIter(&actor_info, 0)),
                "the clock actor must register with its zone-scoped primary definition");

        require(MR::isDemoExist("Clock") && MR::isDemoExist("MissingClock") &&
                    MR::isDemoExist("EmptyClock") &&
                    !MR::isDemoExist("ClockSubGroupOnly") && !MR::isDemoExist(nullptr),
                "isDemoExist must report exact primary definitions even when Time is missing or empty");
        require(MR::isDemoPartExist(&actor, "intro") &&
                    MR::isDemoPartExist(&actor, "pulse") &&
                    !MR::isDemoPartExist(&actor, "unknown") &&
                    !MR::isDemoPartExist(nullptr, "intro"),
                "part existence must use the actor's first primary membership and include SubPart rows");
        require(MR::getDemoPartStep("unknown") == -1 &&
                    MR::getDemoPartTotalStep("unknown") == 0 &&
                    MR::calcDemoPartStepRate("unknown") == 0.0f,
                "safe compatibility sentinels must normalize invalid direct queries");

        require(MR::tryStartTimeKeepDemo(&actor, "Clock", nullptr) &&
                    MR::isDemoActive("Clock") && MR::isTimeKeepDemoActive() &&
                    MR::isDemoActiveRegistered(&actor) &&
                    MR::isDemoPartActive("intro") &&
                    MR::isDemoPartStep("intro", -1) &&
                    !MR::isDemoPartFirstStep("intro") &&
                    MR::isDemoPartLessEqualStep("intro", -1) &&
                    MR::isDemoPartGreaterStep("intro", -2) &&
                    MR::getDemoPartStep("intro") == -1 &&
                    MR::getDemoPartTotalStep("intro") == 3 &&
                    !MR::isDemoPartActive("pulse") &&
                    MR::getDemoPartStep("pulse") == 3 &&
                    MR::getDemoPartTotalStep("pulse") == 2 &&
                    MR::calcDemoPartStepRate("intro") == -1.0f / 3.0f &&
                    std::string_view(MR::getCurrentDemoPartNameMain("Clock")) == "intro",
                "start must expose the main part at step -1 and the source's total+1 sentinel for a dormant known SubPart");

        runtime.movement();
        require(MR::isDemoPartFirstStep("intro") &&
                    MR::getDemoPartTotalStep("intro") == 3 &&
                    MR::isDemoPartFirstStep("dupe") &&
                    MR::getDemoPartTotalStep("dupe") == 2,
                "step zero must dispatch SubPart triggers in BCSV order while main-name lookup keeps precedence");
        runtime.movement();
        require(MR::isDemoPartStep("intro", 1) &&
                    MR::isDemoPartFirstStep("pulse") &&
                    MR::isDemoPartLastStep("dupe"),
                "a two-tick SubPart must expose steps zero and one from its exact main trigger");
        runtime.movement();
        require(MR::isDemoPartLastStep("intro") &&
                    MR::isDemoPartLastStep("pulse") &&
                    MR::isDemoPartFirstStep("chain") &&
                    MR::isDemoPartFirstStep("boundary") &&
                    !MR::isDemoPartActive("dupe") && !MR::isDemoLastStep(),
                "ordered SubParts may trigger from an earlier SubPart, and first duplicate lookup must shadow later rows");
        runtime.movement();
        require(MR::isDemoPartFirstStep("outro") &&
                    MR::isDemoPartStep("boundary", 1) &&
                    std::string_view(MR::getCurrentDemoPartNameMain("Clock")) == "outro",
                "SubPart activity must span a main-part boundary without resetting");
        runtime.movement();
        require(MR::isDemoPartLastStep("outro") &&
                    MR::isDemoPartLastStep("boundary") && MR::isDemoLastStep(),
                "the unpaused physical final row must expose its last dispatch step");
        runtime.movement();
        require(!MR::isTimeKeepDemoActive() && !MR::isDemoActive() &&
                    MR::getCurrentDemoPartNameMain("Clock") == nullptr,
                "the boundary update must naturally end and clear shared director state before dispatch");
    }

    void test_timekeeper_pause_resume_and_preserved_pause_flag() {
        const auto archive = make_clock_sheet_fixture();
        const auto placements = make_clock_definition_fixture();
        auto runtime = smgpc::compat::DemoSceneRuntime(archive, placements);
        auto actor_info = make_actor_info(30, -1, 20);
        auto actor = LiveActor("ClockActor");
        auto wrong_actor = LiveActor("WrongActor");
        require(MR::tryRegisterDemoCast(&actor, JMapInfoIter(&actor_info, 0)) &&
                    MR::tryStartTimeKeepDemo(&actor, "Clock", nullptr),
                "the pause fixture must register and start");

        MR::pauseTimeKeepDemo(&actor);
        runtime.movement();
        require(MR::isDemoPartFirstStep("intro") &&
                    !MR::isDemoPartActive("dupe"),
                "the first paused movement must correct -1 to zero without SubPart dispatch");
        runtime.movement();
        runtime.movement();
        require(MR::isDemoPartStep("intro", 1) &&
                    !MR::isDemoPartActive("pulse"),
                "paused movement must correct zero to one, then freeze without keeper dispatch");
        MR::resumeTimeKeepDemo(&wrong_actor);
        runtime.movement();
        require(MR::isDemoPartStep("intro", 1),
                "pause/resume must ignore actors outside the active executor cast");
        MR::resumeTimeKeepDemo(&actor);
        runtime.movement();
        require(MR::isDemoPartStep("intro", 2) &&
                    !MR::isDemoPartActive("pulse") &&
                    MR::isDemoPartFirstStep("boundary"),
                "resume must continue at step two, permanently missing a paused step-one trigger");

        MR::pauseTimeKeepDemo(&actor);
        runtime.movement();
        require(MR::isDemoPartStep("intro", 2),
                "a pause after the early correction window must freeze the current step");
        MR::endDemo(&actor, "Clock");
        require(!MR::isDemoActive() && runtime.definition(0U)->sheet.is_paused(),
                "DemoTimeKeeper::end must preserve its pause flag exactly");

        require(MR::tryStartTimeKeepDemo(&actor, "Clock", nullptr),
                "an ended clock must be restartable");
        runtime.movement();
        require(MR::isDemoPartFirstStep("intro") &&
                    !MR::isDemoPartActive("dupe"),
                "a restarted clock must remain paused until its registered actor resumes it");
        MR::resumeTimeKeepDemo(&actor);
        MR::endDemo(&actor, "Clock");
    }

    void test_registered_start_suspend_and_safe_rejections() {
        const auto archive = make_clock_sheet_fixture();
        const auto placements = make_clock_definition_fixture();
        auto runtime = smgpc::compat::DemoSceneRuntime(archive, placements);
        auto clock_info = make_actor_info(30, -1, 20);
        auto actor = LiveActor("ClockActor");
        auto wrong_actor = LiveActor("WrongActor");
        auto player = smgpc::runtime::PlayerSystemService{};
        const auto player_context =
            smgpc::compat::ScopedPlayerSystemServiceOverride{player};
        require(MR::tryRegisterDemoCast(&actor, JMapInfoIter(&clock_info, 0)) &&
                    MR::tryRegisterDemoCast(&actor, "Other", JMapInfoIter{}),
                "the registered-start actor must retain ordered multi-membership");
        require(MR::tryStartDemoRegistered(&actor, "outro") &&
                    MR::isDemoActiveRegistered(&actor) &&
                    MR::getDemoPartStep("outro") == -1 &&
                    !player.is_control_enabled(),
                "registered start must choose the actor's first primary executor, exact part, and puppetable path for Player rows");
        require(!MR::tryStartTimeKeepDemo(&actor, "Other", nullptr) &&
                    MR::isDemoActive("Clock"),
                "try-start must reject while the shared DemoDirector is already active");
        MR::endDemo(&wrong_actor, "WrongName");
        require(player.is_control_enabled() && !MR::isDemoActive() &&
                    !MR::isTimeKeepDemoActive(),
                "explicit end must ignore its informational owner/name and release auto-puppetable control like DemoDirector");

        require(MR::tryStartTimeKeepDemo(&actor, "Other", nullptr) &&
                    !MR::isDemoActiveRegistered(&actor),
                "active-registered must compare against the actor's first membership, not any membership");
        MR::startTimeKeepDemo(&actor, "Clock", "outro");
        require(MR::isDemoActive("Clock") && MR::getDemoPartStep("outro") == -1,
                "the explicit void start must replace an active demo without the try-start guard");
        MR::endDemo(&actor, "Clock");
        require(!MR::tryStartTimeKeepDemo(&actor, "MissingClock", nullptr) &&
                    !MR::tryStartTimeKeepDemo(&actor, "EmptyClock", nullptr) &&
                    !MR::tryStartTimeKeepDemo(&actor, "Unknown", nullptr) &&
                    !MR::isDemoActive(),
                "missing, empty, and unknown Time requests must fail safely without claiming director activity");

        require(MR::tryStartTimeKeepDemoMarioPuppetable(&actor, "Suspend", nullptr),
                "the LiveActor puppetable overload must use the same generalized clock");
        runtime.movement();
        runtime.movement();
        require(MR::isDemoPartLastStep("hold") && !MR::isDemoLastStep(),
                "a suspended non-final part must expose its last step without becoming the physical final row");
        runtime.movement();
        require(!MR::isDemoActive() && !MR::isTimeKeepDemoActive() &&
                    !MR::isDemoPartActive("unreachable"),
                "a suspend boundary must end before the following part dispatches");
    }

    void test_one_frame_final_pause_boundary_overshoot() {
        const auto archive = make_clock_sheet_fixture();
        const auto placements = make_clock_definition_fixture();
        auto runtime = smgpc::compat::DemoSceneRuntime(archive, placements);
        auto actor_info = make_actor_info(33, -1, 23);
        auto actor = LiveActor("RunawayActor");
        require(MR::tryRegisterDemoCast(&actor, JMapInfoIter(&actor_info, 0)) &&
                    MR::tryStartTimeKeepDemo(&actor, "Runaway", nullptr),
                "the one-frame edge fixture must start");
        runtime.movement();
        require(MR::isDemoLastStep() && MR::isDemoPartLastStep("one"),
                "an unpaused one-frame physical final part must expose step zero as last");
        MR::pauseTimeKeepDemo(&actor);
        require(!MR::isDemoLastStep() && MR::isDemoPartLastStep("one"),
                "physical-final detection must suppress paused state without changing the part-step predicate");
        runtime.movement();
        require(MR::getDemoPartStep("one") == 1 && MR::isTimeKeepDemoActive(),
                "paused correction must advance the final one-frame part to step one");
        MR::resumeTimeKeepDemo(&actor);
        runtime.movement();
        require(MR::isTimeKeepDemoActive() && MR::isDemoActive("Runaway") &&
                    MR::getDemoPartStep("one") == 2 &&
                    std::string_view(MR::getCurrentDemoPartNameMain("Runaway")) == "one" &&
                    !MR::isDemoLastStep(),
                "the exact source >= comparison must retain the final pointer after one-past-end overshoot");
        runtime.movement();
        require(MR::getDemoPartStep("one") == 3,
                "the retained-pointer source edge must continue running until explicit end");
        MR::endDemo(&actor, "Runaway");
        require(!MR::isDemoActive() && !MR::isTimeKeepDemoActive(),
                "explicit end must recover safely from the source boundary edge");
    }

    void test_action_dispatch_order_pause_and_callback_invariant() {
        const auto archive = make_dispatch_sheet_fixture();
        const auto placements = make_dispatch_definition_fixture();
        const auto general_positions = std::array{
            smgpc::scene::StageGeneralPos{
                .name = "Anchor",
                .world_position = {12.0F, 34.0F, 56.0F},
                .world_rotation = {7.0F, 8.0F, 9.0F},
            },
        };
        {
            auto runtime = smgpc::compat::DemoSceneRuntime(archive, placements,
                                                           general_positions);
            auto actor_info = make_actor_info(50, -1, 40);
            auto actor = LiveActor("Actor");
            const auto initial_nerve = TestNerve{};
            const auto action_nerve = TestNerve{};
            actor.initNerve(&initial_nerve);
            auto observer = ActionObserver{.actor = &actor};
            const auto functor = MR::Functor(&observer, &ActionObserver::observe);
            require(MR::tryRegisterDemoCast(&actor, JMapInfoIter(&actor_info, 0)) &&
                        MR::tryRegisterDemoActionFunctor(&actor, functor, "first") &&
                        MR::tryRegisterDemoActionNerve(&actor, &action_nerve, "first") &&
                        MR::tryStartTimeKeepDemo(&actor, "Dispatch", nullptr),
                    "the action-dispatch fixture must register callbacks and start");

            runtime.movement();
            actor.updateNerve();
            require(observer.calls == 1 && observer.saw_appeared &&
                        actor.isNerve(&action_nerve) && actor.mFlag.mIsHiddenModel &&
                        actor.currentBckName() == "Wave" && actor.mPosition.x == 12.0F &&
                        actor.mPosition.y == 34.0F && actor.mPosition.z == 56.0F &&
                        actor.mRotation.x == 7.0F && actor.mRotation.y == 8.0F &&
                        actor.mRotation.z == 9.0F,
                    "Action rows must run in BCSV order, then apply animation and real GeneralPos data after each row operation");
            runtime.movement();
            require(observer.calls == 1 && !actor.isDead(),
                    "a multi-frame Action row must not replay its first-step operation on the last step");
            runtime.movement();
            require(actor.isDead(),
                    "a one-frame Action part must execute its first branch even though it is also the last step");
            runtime.movement();
            require(!MR::isDemoActive(), "the dispatch fixture must end at its retail boundary");
        }

        {
            auto runtime = smgpc::compat::DemoSceneRuntime(archive, placements,
                                                           general_positions);
            auto actor_info = make_actor_info(50, -1, 40);
            auto actor = LiveActor("Actor");
            const auto initial_nerve = TestNerve{};
            const auto action_nerve = TestNerve{};
            actor.initNerve(&initial_nerve);
            auto observer = ActionObserver{.actor = &actor};
            const auto functor = MR::Functor(&observer, &ActionObserver::observe);
            require(MR::tryRegisterDemoCast(&actor, JMapInfoIter(&actor_info, 0)) &&
                        MR::tryRegisterDemoActionFunctor(&actor, functor, "first") &&
                        MR::tryRegisterDemoActionNerve(&actor, &action_nerve, "first") &&
                        MR::tryStartTimeKeepDemo(&actor, "Dispatch", nullptr),
                    "the paused action fixture must start");
            MR::pauseTimeKeepDemo(&actor);
            runtime.movement();
            require(observer.calls == 0 && actor.isDead(),
                    "a paused early-step correction must not dispatch Action rows");
            MR::resumeTimeKeepDemo(&actor);
            runtime.movement();
            require(observer.calls == 0 && actor.isDead(),
                    "resuming after a skipped first step must not invent or replay the missed Action operation");
            MR::endDemo(&actor, "Dispatch");
        }

        {
            auto runtime = smgpc::compat::DemoSceneRuntime(archive, placements);
            auto actor_info = make_actor_info(50, -1, 40);
            auto actor = LiveActor("Actor");
            require(MR::tryRegisterDemoCast(&actor, JMapInfoIter(&actor_info, 0)) &&
                        MR::tryStartTimeKeepDemo(&actor, "Dispatch", nullptr),
                    "the missing-callback fixture must start");
            auto threw = false;
            try {
                runtime.movement();
            } catch (const std::runtime_error &error) {
                threw = std::string_view(error.what()).find("registered functor") !=
                        std::string_view::npos;
            }
            require(threw,
                    "a targeted type-2 row without its source-required callback must fail explicitly");
        }

        {
            auto runtime = smgpc::compat::DemoSceneRuntime(archive, placements);
            auto actor_info = make_actor_info(50, -1, 40);
            auto actor = LiveActor("Actor");
            const auto initial_nerve = TestNerve{};
            const auto action_nerve = TestNerve{};
            actor.initNerve(&initial_nerve);
            auto observer = ActionObserver{.actor = &actor};
            const auto functor = MR::Functor(&observer, &ActionObserver::observe);
            require(MR::tryRegisterDemoCast(&actor, JMapInfoIter(&actor_info, 0)) &&
                        MR::tryRegisterDemoActionFunctor(&actor, functor, "first") &&
                        MR::tryRegisterDemoActionNerve(&actor, &action_nerve, "first") &&
                        MR::tryStartTimeKeepDemo(&actor, "Dispatch", nullptr),
                    "the missing-GeneralPos fixture must start");
            auto threw = false;
            try {
                runtime.movement();
            } catch (const std::runtime_error &error) {
                threw = std::string_view(error.what()).find("absent from the active scene GeneralPos data") !=
                        std::string_view::npos;
            }
            require(threw,
                    "a PosName row without real active-scene data must fail explicitly");
        }

        {
            const auto talk_archive = make_required_talk_sheet_fixture();
            const auto talk_placements = make_required_talk_definition_fixture();
            auto runtime = smgpc::compat::DemoSceneRuntime(talk_archive, talk_placements);
            auto actor_info = make_actor_info(51, -1, 41);
            auto actor = LiveActor("Actor");
            require(MR::tryRegisterDemoCast(&actor, JMapInfoIter(&actor_info, 0)) &&
                        MR::tryStartTimeKeepDemo(&actor, "RequiredTalk", nullptr),
                    "the missing-talk-controller fixture must start");
            auto threw = false;
            try {
                runtime.movement();
            } catch (const std::runtime_error &error) {
                threw = std::string_view(error.what()).find("registered talk controller") !=
                        std::string_view::npos;
            }
            require(threw,
                    "a talk Action row without its source-required controller must fail explicitly");
        }
    }

    void test_wipe_row_dispatch_uses_arbitrary_names_and_raw_frames() {
        auto wipe = smgpc::runtime::WipeService{};
        wipe.begin_frame(17U);
        smgpc::compat::dispatch_demo_wipe_row(
            smgpc::compat::DemoWipeRow{.wipe_name = "ArbitraryWipe", .wipe_type = 0, .wipe_frame = -1},
            wipe);
        smgpc::compat::dispatch_demo_wipe_row(
            smgpc::compat::DemoWipeRow{.wipe_name = "ArbitraryWipe", .wipe_type = 1, .wipe_frame = 12},
            wipe);
        smgpc::compat::dispatch_demo_wipe_row(
            smgpc::compat::DemoWipeRow{.wipe_name = "OtherWipe", .wipe_type = 2}, wipe);
        smgpc::compat::dispatch_demo_wipe_row(
            smgpc::compat::DemoWipeRow{.wipe_name = "OtherWipe", .wipe_type = 3}, wipe);
        smgpc::compat::dispatch_demo_wipe_row(
            smgpc::compat::DemoWipeRow{.wipe_name = "IgnoredWipe", .wipe_type = 99}, wipe);

        const auto events = wipe.events();
        require(events.size() == 4U &&
                    events[0].kind == smgpc::runtime::WipeEventKind::Open &&
                    events[0].name == "ArbitraryWipe" && events[0].frame_count == -1 &&
                    events[0].frame_index == 17U &&
                    events[1].kind == smgpc::runtime::WipeEventKind::Close &&
                    events[1].frame_count == 12 &&
                    events[2].kind == smgpc::runtime::WipeEventKind::ForceOpen &&
                    events[3].kind == smgpc::runtime::WipeEventKind::ForceClose,
                "Wipe types 0-3 must preserve arbitrary names and the raw -1 frame sentinel; unknown types are no-op");
    }

    void test_no_registry_and_scene_teardown() {
        auto info = make_actor_info(3, 1, 1);
        auto actor = LiveActor("Actor");
        auto player = smgpc::runtime::PlayerSystemService{};
        const auto player_context =
            smgpc::compat::ScopedPlayerSystemServiceOverride{player};
        require(smgpc::compat::active_demo_scene_runtime() == nullptr &&
                    !MR::tryRegisterDemoCast(&actor, JMapInfoIter(&info, 0)),
                "cast registration without an active scene registry should fail without an orphan fallback");
        require(!MR::tryStartTimeKeepDemo(&actor, "TryWithoutRegistry", nullptr) &&
                    !MR::isDemoActive(),
                "a try time-keep start without a scene registry must reject safely");
        MR::startTimeKeepDemo(&actor, "VoidWithoutRegistry", nullptr);
        require(MR::isDemoActive("VoidWithoutRegistry") &&
                    !MR::isTimeKeepDemoActive(),
                "the void time-keep API must retain its programmable fallback when no scene registry exists");
        MR::endDemo(&actor, "VoidWithoutRegistry");
        MR::startTimeKeepDemoMarioPuppetable(
            &actor, "PuppetVoidWithoutRegistry", nullptr);
        require(MR::isDemoActive("PuppetVoidWithoutRegistry") &&
                    !MR::isTimeKeepDemoActive() &&
                    !player.is_control_enabled(),
                "the void Mario API must retain its no-registry global and puppetable fallback");
        MR::endDemo(&actor, "PuppetVoidWithoutRegistry");
        require(player.is_control_enabled(),
                "ending the no-registry puppetable fallback must restore control");
        require(MR::tryStartDemoWithoutCinemaFrame(&actor, "Programmable") && MR::isDemoActive(),
                "programmable demos should remain usable without a time-keep scene executor");
        MR::endDemo(&actor, "Programmable");

        const auto archive = make_sheet_fixture();
        const auto placements = make_definition_fixture();
        {
            auto runtime = smgpc::compat::DemoSceneRuntime(archive, placements);
            require(MR::tryRegisterDemoCast(&actor, JMapInfoIter(&info, 0)) &&
                        smgpc::compat::has_registered_demo_cast(&actor),
                    "the scoped scene should own its actor membership");
        }
        require(smgpc::compat::active_demo_scene_runtime() == nullptr &&
                    !smgpc::compat::has_registered_demo_cast(&actor) &&
                    !MR::tryRegisterDemoCast(&actor, JMapInfoIter(&info, 0)),
                "scene teardown should clear the active pointer and all owned memberships");
    }

    void test_no_definition_skips_demo_sheet_archive() {
        auto dvd = smgpc::runtime::DvdFileSystemService(
            std::filesystem::path("/tmp/smg-pc-demo-scene-no-definitions"));
        const auto irrelevant = std::array{
            make_definition_placement(DefinitionRow{
                .object_name = "Other",
                .demo_name = "Other",
                .sheet_name = "Other",
                .zone_id = 0,
                .link_id = 0,
            }),
            make_definition_placement(DefinitionRow{
                .object_name = "DemoSubGroup",
                .demo_name = "OnlySubGroup",
                .sheet_name = "Unused",
                .zone_id = 0,
                .link_id = 1,
            }),
        };
        const auto runtime = smgpc::compat::DemoSceneRuntime(dvd, irrelevant);
        require(runtime.definitions().empty() && runtime.subgroups().size() == 1U &&
                    dvd.archive_load_count("/ObjectData/DemoSheet.arc") == 0U,
                "a scene without primary definitions should not touch DemoSheet.arc");
    }

    [[nodiscard]] std::optional<std::filesystem::path> real_disc_path() {
        auto root = std::filesystem::current_path();
        for (auto depth = 0U; depth < 8U; ++depth) {
            const auto candidates = std::array{
                root / "RMGK01.iso",
                root / "pc-port/RMGK01.iso",
            };
            for (const auto &candidate : candidates) {
                if (std::filesystem::is_regular_file(candidate)) {
                    return candidate;
                }
            }
            if (!root.has_parent_path() || root.parent_path() == root) {
                break;
            }
            root = root.parent_path();
        }
        return std::nullopt;
    }

    void test_optional_real_gateway_definitions() {
        const auto disc_path = real_disc_path();
        if (!disc_path.has_value()) {
            std::cout << "[skip] RMGK01 scene-demo definition check\n";
            return;
        }
        aurora_dvd_close();
        require(aurora_dvd_open(disc_path->c_str()), "the optional RMGK01 image should open");
        struct DiscCloseGuard {
            ~DiscCloseGuard() {
                aurora_dvd_close();
            }
        } close_guard;
        DVDInit();
        auto dvd = smgpc::runtime::DvdFileSystemService("/");
        {
            const auto tables = smgpc::scene::resolve_stage_placement_tables(
                dvd, "HeavensDoorGalaxy", 1);
            const auto placements = smgpc::scene::resolve_stage_placement_objects(dvd, tables);
            const auto positions = smgpc::scene::select_stage_general_positions(tables);
            const auto runtime = smgpc::compat::DemoSceneRuntime(dvd, placements, positions);
            if (runtime.definitions().size() != 3U) {
                throw std::runtime_error(
                    "real scenario 1 should retain all three active primary demo definitions; got " +
                    std::to_string(runtime.definitions().size()) + " from " +
                    std::to_string(placements.size()) + " placements");
            }
            const auto first = runtime.find_definition(5, 0);
            require(first.has_value() && runtime.definition(*first)->time_sheet_name == "TicoGuideDemo" &&
                        runtime.definition(*first)->sheet.action_rows().size() == 36U,
                    "the real zone-scoped guide definition should bind its exact complete sheet");
            const auto dormant = runtime.find_definition(5, 1);
            require(dormant.has_value() &&
                        !runtime.definition(*dormant)->sheet.has_table(
                            smgpc::compat::DemoSheetTable::Time),
                    "the real missing Time family should remain dormant without an alias");
            const auto has_general_pos = [&](std::string_view name) {
                return std::ranges::any_of(positions, [&](const auto &position) {
                    return position.name == name;
                });
            };
            for (const auto &definition : runtime.definitions()) {
                for (const auto &row : definition.sheet.action_rows()) {
                    if (!row.position_name.empty() && !has_general_pos(row.position_name)) {
                        throw std::runtime_error(
                            "Gateway Action PosName is absent from GeneralPos: demo='" +
                            definition.demo_name + "' part='" + row.part_name + "' position='" +
                            row.position_name + "'");
                    }
                }
                for (const auto &row : definition.sheet.player_rows()) {
                    if (!row.position_name.empty() && !has_general_pos(row.position_name)) {
                        throw std::runtime_error(
                            "Gateway Player PosName is absent from GeneralPos: demo='" +
                            definition.demo_name + "' part='" + row.part_name + "' position='" +
                            row.position_name + "'");
                    }
                }
            }
            std::cout << "[info] Gateway scenario 1 GeneralPos rows: " << positions.size() << '\n';
        }
        const auto placements = smgpc::scene::resolve_stage_placement_objects(
            dvd, "HeavensDoorGalaxy", 2);
        const auto runtime = smgpc::compat::DemoSceneRuntime(dvd, placements);
        const auto read_star = runtime.find_definition(5, 2);
        require(runtime.definitions().size() == 1U && read_star.has_value() &&
                    runtime.definition(*read_star)->time_sheet_name == "ReadStar",
                "real scenario 2 should retain only its exact zone-5 link-2 definition");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };

}  // namespace

int main() {
    const auto tests = std::array{
        TestCase{"definition ingestion and dormant sheets", test_definition_ingestion_and_dormant_sheets},
        TestCase{"zone scoping, CastId, and duplicate choice", test_zone_scoping_cast_id_and_duplicate_choice},
        TestCase{"duplicate registered name resolution and pause scan", test_duplicate_registered_name_resolution_and_pause_scan},
        TestCase{"multi-membership and row callbacks", test_explicit_multi_membership_and_row_callbacks},
        TestCase{"subgroup forwarding and named behavior", test_subgroup_forwarding_and_named_behavior},
        TestCase{"timekeeper queries, SubParts, and natural end", test_timekeeper_queries_subparts_and_natural_end},
        TestCase{"timekeeper pause, resume, and preserved pause", test_timekeeper_pause_resume_and_preserved_pause_flag},
        TestCase{"registered start, suspend, and safe rejections", test_registered_start_suspend_and_safe_rejections},
        TestCase{"one-frame final paused boundary overshoot", test_one_frame_final_pause_boundary_overshoot},
        TestCase{"ZoneList-only zone IDs", test_zone_ids_are_only_read_from_zone_list},
        TestCase{"GeneralPos table order and zone transform", test_general_pos_table_order_and_zone_transform},
        TestCase{"Action dispatch order, pause, and callback invariant", test_action_dispatch_order_pause_and_callback_invariant},
        TestCase{"Wipe row arbitrary names and raw frames", test_wipe_row_dispatch_uses_arbitrary_names_and_raw_frames},
        TestCase{"no registry and scene teardown", test_no_registry_and_scene_teardown},
        TestCase{"no definition skips DemoSheet archive", test_no_definition_skips_demo_sheet_archive},
        TestCase{"optional real scene definitions", test_optional_real_gateway_definitions},
    };

    auto passed = std::size_t{};
    for (const auto &test : tests) {
        try {
            test.run();
            ++passed;
            std::cout << "[pass] " << test.name << '\n';
        } catch (const std::exception &error) {
            std::cerr << "[fail] " << test.name << ": " << error.what() << '\n';
        }
    }
    std::cout << passed << '/' << tests.size() << " tests passed\n";
    return passed == tests.size() ? 0 : 1;
}
