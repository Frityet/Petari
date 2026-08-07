#include "Game/LiveActor/LiveActor.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/Util/DemoUtil.hpp"
#include "Game/Util/Functor.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "compat/ActorRuntimeRegistry.hpp"
#include "compat/DemoSceneRuntime.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/StagePlacementResolver.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <algorithm>
#include <array>
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

    [[nodiscard]] std::vector<std::uint8_t> make_time_bcsv(std::string_view part_name) {
        constexpr auto field_count = std::size_t{2U};
        constexpr auto entry_size = std::size_t{8U};
        constexpr auto data_offset = std::size_t{0x10U + field_count * 0x0cU};
        auto bytes = std::vector<std::uint8_t>(data_offset + entry_size + part_name.size() + 1U, 0U);
        write_be32(bytes, 0x00U, 1U);
        write_be32(bytes, 0x04U, field_count);
        write_be32(bytes, 0x08U, data_offset);
        write_be32(bytes, 0x0cU, entry_size);
        write_bcsv_field(bytes, 0U, "PartName", 0U,
                         smgpc::resource::BcsvFieldType::StringOffset);
        write_bcsv_field(bytes, 1U, "TotalStep", 4U,
                         smgpc::resource::BcsvFieldType::Int32);
        write_be32(bytes, data_offset, 0U);
        write_be32(bytes, data_offset + 4U, 1U);
        std::copy(part_name.begin(), part_name.end(),
                  bytes.begin() + static_cast<std::ptrdiff_t>(data_offset + entry_size));
        return bytes;
    }

    struct ActionRow {
        std::string part_name;
        std::string cast_name;
        std::int32_t cast_id = -1;
        std::int32_t action_type = 0;
    };

    [[nodiscard]] std::vector<std::uint8_t> make_action_bcsv(
        std::span<const ActionRow> rows) {
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
            offsets.emplace_back(add_string(row.part_name), add_string(row.cast_name));
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
        for (auto index = std::size_t{}; index < rows.size(); ++index) {
            const auto entry = data_offset + index * entry_size;
            write_be32(bytes, entry, offsets[index].first);
            write_be32(bytes, entry + 4U, offsets[index].second);
            write_be32(bytes, entry + 8U, static_cast<std::uint32_t>(rows[index].cast_id));
            write_be32(bytes, entry + 12U, static_cast<std::uint32_t>(rows[index].action_type));
        }
        std::copy(strings.begin(), strings.end(),
                  bytes.begin() + static_cast<std::ptrdiff_t>(data_offset + rows.size() * entry_size));
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

    void test_no_registry_and_scene_teardown() {
        auto info = make_actor_info(3, 1, 1);
        auto actor = LiveActor("Actor");
        require(smgpc::compat::active_demo_scene_runtime() == nullptr &&
                    !MR::tryRegisterDemoCast(&actor, JMapInfoIter(&info, 0)),
                "cast registration without an active scene registry should fail without an orphan fallback");
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
            const auto placements = smgpc::scene::resolve_stage_placement_objects(
                dvd, "HeavensDoorGalaxy", 1);
            const auto runtime = smgpc::compat::DemoSceneRuntime(dvd, placements);
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
        TestCase{"multi-membership and row callbacks", test_explicit_multi_membership_and_row_callbacks},
        TestCase{"subgroup forwarding and named behavior", test_subgroup_forwarding_and_named_behavior},
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
