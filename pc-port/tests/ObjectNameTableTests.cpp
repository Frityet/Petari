#include "resource/BcsvTable.hpp"
#include "resource/RarcArchive.hpp"
#include "runtime/RuntimeServices.hpp"
#include "scene/nameobj/ObjectNameTable.hpp"

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
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

    template <typename Function>
    void require_throws(Function &&function, std::string_view message) {
        try {
            function();
        } catch (const std::exception &) {
            return;
        }
        throw std::runtime_error(std::string(message));
    }

    void write_be16(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint16_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value);
    }

    void write_be32(std::vector<std::uint8_t> &bytes, std::size_t offset, std::uint32_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value >> 24U);
        bytes[offset + 1U] = static_cast<std::uint8_t>(value >> 16U);
        bytes[offset + 2U] = static_cast<std::uint8_t>(value >> 8U);
        bytes[offset + 3U] = static_cast<std::uint8_t>(value);
    }

    void write_bcsv_field(std::vector<std::uint8_t> &bytes, std::size_t index, std::string_view name,
                          std::uint16_t offset, smgpc::resource::BcsvFieldType type) {
        const auto descriptor = 0x10U + index * 0x0cU;
        write_be32(bytes, descriptor, smgpc::resource::jmap_hash(name));
        write_be32(bytes, descriptor + 0x04U, 0xffffffffU);
        write_be16(bytes, descriptor + 0x08U, offset);
        bytes[descriptor + 0x0aU] = 0U;
        bytes[descriptor + 0x0bU] = static_cast<std::uint8_t>(type);
    }

    [[nodiscard]] std::string raw_bytes(std::initializer_list<std::uint8_t> values) {
        auto result = std::string{};
        result.reserve(values.size());
        for (const auto value : values) {
            result.push_back(static_cast<char>(value));
        }
        return result;
    }

    struct NameRow {
        std::string english;
        std::string japanese_cp932;
    };

    [[nodiscard]] std::vector<std::uint8_t> make_name_table_bcsv(
        std::span<const NameRow> rows, bool include_japanese = true,
        smgpc::resource::BcsvFieldType japanese_type = smgpc::resource::BcsvFieldType::StringOffset) {
        const auto field_count = include_japanese ? 2U : 1U;
        const auto entry_size = include_japanese ? 8U : 4U;
        const auto data_offset = 0x10U + field_count * 0x0cU;

        auto string_table = std::vector<std::uint8_t>{};
        const auto add_string = [&string_table](std::string_view value) {
            const auto offset = static_cast<std::uint32_t>(string_table.size());
            string_table.insert(string_table.end(), value.begin(), value.end());
            string_table.push_back(0U);
            return offset;
        };

        auto offsets = std::vector<std::pair<std::uint32_t, std::uint32_t>>{};
        offsets.reserve(rows.size());
        for (const auto &row : rows) {
            offsets.emplace_back(add_string(row.english), add_string(row.japanese_cp932));
        }

        auto bytes = std::vector<std::uint8_t>(data_offset + rows.size() * entry_size + string_table.size(), 0U);
        write_be32(bytes, 0x00U, static_cast<std::uint32_t>(rows.size()));
        write_be32(bytes, 0x04U, field_count);
        write_be32(bytes, 0x08U, data_offset);
        write_be32(bytes, 0x0cU, entry_size);
        if (include_japanese) {
            // Reverse the source column order to prove lookup is hash-based.
            write_bcsv_field(bytes, 0U, "jp_name", 4U, japanese_type);
            write_bcsv_field(bytes, 1U, "en_name", 0U, smgpc::resource::BcsvFieldType::StringOffset);
        } else {
            write_bcsv_field(bytes, 0U, "en_name", 0U, smgpc::resource::BcsvFieldType::StringOffset);
        }
        for (auto row = std::size_t{}; row < rows.size(); ++row) {
            const auto entry = data_offset + row * entry_size;
            write_be32(bytes, entry, offsets[row].first);
            if (include_japanese) {
                write_be32(bytes, entry + 4U, offsets[row].second);
            }
        }
        std::copy(string_table.begin(), string_table.end(), bytes.begin() + data_offset + rows.size() * entry_size);
        return bytes;
    }

    [[nodiscard]] smgpc::resource::RarcArchive make_single_file_rarc(
        std::string_view file_name, const std::vector<std::uint8_t> &file_data) {
        constexpr auto header_size = std::size_t{0x20U};
        constexpr auto info_offset = std::size_t{0x20U};
        constexpr auto directory_offset = std::size_t{0x40U};
        constexpr auto file_entry_offset = std::size_t{0x50U};
        constexpr auto string_table_offset = std::size_t{0x64U};
        constexpr auto file_data_offset = std::size_t{0x80U};
        require(string_table_offset + file_name.size() + 1U <= file_data_offset,
                "test RARC file name must fit before its data section");

        auto bytes = std::vector<std::uint8_t>(file_data_offset + file_data.size(), 0U);
        write_be32(bytes, 0x00U, 0x52415243U);
        write_be32(bytes, 0x04U, static_cast<std::uint32_t>(bytes.size()));
        write_be32(bytes, 0x08U, header_size);
        write_be32(bytes, 0x0cU, file_data_offset - header_size);
        write_be32(bytes, 0x10U, static_cast<std::uint32_t>(file_data.size()));

        write_be32(bytes, info_offset + 0x00U, 1U);
        write_be32(bytes, info_offset + 0x04U, directory_offset - info_offset);
        write_be32(bytes, info_offset + 0x08U, 1U);
        write_be32(bytes, info_offset + 0x0cU, file_entry_offset - info_offset);
        write_be32(bytes, info_offset + 0x10U, static_cast<std::uint32_t>(file_name.size() + 1U));
        write_be32(bytes, info_offset + 0x14U, string_table_offset - info_offset);

        write_be16(bytes, directory_offset + 0x0aU, 1U);
        write_be32(bytes, directory_offset + 0x0cU, 0U);

        write_be16(bytes, file_entry_offset + 0x00U, 0U);
        write_be16(bytes, file_entry_offset + 0x02U, smgpc::resource::RarcArchive::hash_name(file_name));
        bytes[file_entry_offset + 0x04U] = 1U;
        write_be32(bytes, file_entry_offset + 0x08U, 0U);
        write_be32(bytes, file_entry_offset + 0x0cU, static_cast<std::uint32_t>(file_data.size()));

        std::copy(file_name.begin(), file_name.end(), bytes.begin() + string_table_offset);
        std::copy(file_data.begin(), file_data.end(), bytes.begin() + file_data_offset);
        return smgpc::resource::RarcArchive::from_bytes(std::move(bytes));
    }

    void test_synthetic_lookup_and_first_row_semantics() {
        const auto rows = std::vector<NameRow>{
            {.english = "FixtureActor", .japanese_cp932 = raw_bytes({0x83U, 0x60U, 0x83U, 0x52U})},
            {.english = "FixtureActor", .japanese_cp932 = raw_bytes({0x83U, 0x8dU, 0x83U, 0x5bU, 0x83U, 0x62U, 0x83U, 0x5eU})},
            {.english = "AsciiActor", .japanese_cp932 = "Runtime Actor"},
            {.english = "EmptyActor", .japanese_cp932 = ""},
        };
        const auto archive = make_single_file_rarc("oBjNaMeTaBlE.TbL", make_name_table_bcsv(rows));
        const auto table = smgpc::scene::nameobj::ObjectNameTable(archive);

        require(table.size() == 3U, "duplicate English keys should retain only their first row");
        const auto *fixture_name = table.lookup("FixtureActor");
        require(fixture_name != nullptr && *fixture_name == "チコ",
                "the first CP932 Japanese value should decode to UTF-8");
        require(table.lookup("FixtureActor") == fixture_name,
                "lookup should return a stable pointer owned by the table");
        require(table.lookup_or_self("AsciiActor") == "Runtime Actor",
                "lookup-or-self should return the mapped runtime name");
        require(table.lookup("EmptyActor") != nullptr && table.lookup("EmptyActor")->empty(),
                "an explicitly empty Japanese name should remain a present mapping");
        require(table.lookup("UnknownActor") == nullptr,
                "unknown object names should use the null lookup result");
        require(table.lookup_or_self("UnknownActor") == "UnknownActor",
                "unknown object names should fall back to their English identifier");
    }

    void test_schema_and_archive_validation() {
        const auto rows = std::vector<NameRow>{{.english = "FixtureActor", .japanese_cp932 = "Fixture"}};
        require_throws(
            [&] {
                const auto archive = make_single_file_rarc("ObjNameTable.tbl",
                                                           make_name_table_bcsv(rows, false));
                (void)smgpc::scene::nameobj::ObjectNameTable(archive);
            },
            "a table without jp_name should be rejected");
        require_throws(
            [&] {
                const auto archive = make_single_file_rarc(
                    "ObjNameTable.tbl",
                    make_name_table_bcsv(rows, true, smgpc::resource::BcsvFieldType::Int32));
                (void)smgpc::scene::nameobj::ObjectNameTable(archive);
            },
            "a non-string jp_name field should be rejected");
        require_throws(
            [&] {
                const auto archive = make_single_file_rarc("Other.tbl", make_name_table_bcsv(rows));
                (void)smgpc::scene::nameobj::ObjectNameTable(archive);
            },
            "an archive without ObjNameTable.tbl should be rejected");
    }

    void validate_real_table(const std::filesystem::path &path, std::string_view label) {
        const auto archive = smgpc::resource::RarcArchive::from_file(path);
        const auto table = smgpc::scene::nameobj::ObjectNameTable(archive);
        require(table.size() == 1691U, std::string(label) + " should contain all 1691 first-row mappings");
        require(table.lookup_or_self("DemoRabbit") == "デモウサギ",
                std::string(label) + " should map the Gateway demo rabbit name");
        require(table.lookup_or_self("Rosetta") == "ロゼッタ",
                std::string(label) + " should map the Gateway Rosetta name");
        require(table.lookup_or_self("RunawayTico") == "逃げチコ",
                std::string(label) + " should map the Gateway runaway Tico name");
        require(table.lookup_or_self("Tico") == "チコ",
                std::string(label) + " should map the Gateway Tico name");
        require(table.lookup_or_self("TicoBaby") == "ベビチコ",
                std::string(label) + " should map the Gateway baby Tico name");
        require(table.lookup_or_self("HeavensDoorAppearStepA") ==
                    "ヘブンズドアミステリアス惑星階段（デモ中）",
                std::string(label) + " should map the Gateway demo-state staircase name");
        require(table.lookup_or_self("HeavensDoorAppearStepAAfter") ==
                    "ヘブンズドアミステリアス惑星階段（デモ後）",
                std::string(label) + " should map the Gateway post-demo staircase name");
        for (const auto absent : {"LightDome", "DomeHalo", "TicoDemoGetPower"}) {
            require(table.lookup(absent) == nullptr && table.lookup_or_self(absent) == absent,
                    std::string(label) + " should preserve English fallback for absent " + absent);
        }
    }

    void test_optional_extracted_tables() {
        struct Candidate {
            const char *environment;
            std::filesystem::path conventional_path;
            std::string_view label;
        };
        const auto candidates = std::vector<Candidate>{
            {.environment = "SMGPC_RMGK01_OBJ_NAME_TABLE",
             .conventional_path = "container/orig/RMGK01/files/StageData/ObjNameTable.arc",
             .label = "RMGK01"},
            {.environment = "SMGPC_RMGK02_OBJ_NAME_TABLE",
             .conventional_path = "../orig/RMGK02/files/StageData/ObjNameTable.arc",
             .label = "RMGK02"},
        };

        for (const auto &candidate : candidates) {
            auto path = candidate.conventional_path;
            if (const auto *configured = std::getenv(candidate.environment);
                configured != nullptr && configured[0] != '\0') {
                path = configured;
            }
            if (!std::filesystem::is_regular_file(path)) {
                std::cout << "[skip] " << candidate.label << " extracted ObjNameTable (set "
                          << candidate.environment << ")\n";
                continue;
            }
            validate_real_table(path, candidate.label);
            std::cout << "[ok] " << candidate.label << " extracted ObjNameTable\n";
        }
    }

    void test_optional_dvd_service_load() {
        const auto *disc_path = std::getenv("SMGPC_REAL_DISC");
        if (disc_path == nullptr || disc_path[0] == '\0') {
            std::cout << "[skip] DVD service ObjNameTable load (set SMGPC_REAL_DISC)\n";
            return;
        }

        aurora_dvd_close();
        require(aurora_dvd_open(disc_path), "SMGPC_REAL_DISC should point to a readable SMG image");
        struct DiscCloseGuard {
            ~DiscCloseGuard() {
                aurora_dvd_close();
            }
        } close_guard;
        DVDInit();

        auto dvd = smgpc::runtime::DvdFileSystemService{"/"};
        const auto table = smgpc::scene::nameobj::ObjectNameTable(dvd);
        require(table.size() == 1691U && table.lookup_or_self("Rosetta") == "ロゼッタ",
                "the DVD-backed table should load and decode through DvdFileSystemService");
        require(dvd.archive_load_count("/StageData/ObjNameTable.arc") == 1U,
                "the DVD service should load ObjNameTable.arc once");
    }

    struct TestCase {
        std::string_view name;
        void (*run)();
    };

}  // namespace

int main() {
    const auto tests = std::vector<TestCase>{
        {"synthetic lookup and first-row semantics", test_synthetic_lookup_and_first_row_semantics},
        {"schema and archive validation", test_schema_and_archive_validation},
        {"optional extracted tables", test_optional_extracted_tables},
        {"optional DVD service load", test_optional_dvd_service_load},
    };

    auto passed = std::size_t{};
    for (const auto &test : tests) {
        try {
            test.run();
            ++passed;
            std::cout << "[ok] " << test.name << '\n';
        } catch (const std::exception &exception) {
            std::cerr << "[fail] " << test.name << ": " << exception.what() << '\n';
        }
    }

    std::cout << passed << '/' << tests.size() << " tests passed\n";
    return passed == tests.size() ? 0 : 1;
}
