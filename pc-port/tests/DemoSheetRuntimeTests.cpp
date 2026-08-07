#include "compat/DemoSheetRuntime.hpp"
#include "resource/BcsvTable.hpp"
#include "resource/RarcArchive.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <set>
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

    std::string lower_ascii(std::string_view value) {
        auto result = std::string(value);
        std::ranges::transform(result, result.begin(), [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
        return result;
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

    std::vector<std::uint8_t> make_empty_time_bcsv() {
        auto bytes = std::vector<std::uint8_t>(0x10U, 0U);
        write_be32(bytes, 0x00U, 0U);
        write_be32(bytes, 0x04U, 0U);
        write_be32(bytes, 0x08U, 0x10U);
        write_be32(bytes, 0x0cU, 0U);
        return bytes;
    }

    std::vector<std::uint8_t> make_missing_part_name_bcsv() {
        constexpr auto field_count = std::size_t{1U};
        constexpr auto data_offset = std::size_t{0x10U + field_count * 0x0cU};
        constexpr auto entry_size = std::size_t{4U};
        auto bytes = std::vector<std::uint8_t>(data_offset + entry_size, 0U);
        write_be32(bytes, 0x00U, 1U);
        write_be32(bytes, 0x04U, field_count);
        write_be32(bytes, 0x08U, data_offset);
        write_be32(bytes, 0x0cU, entry_size);
        write_bcsv_field(bytes, 0U, "TotalStep", 0U, smgpc::resource::BcsvFieldType::Int32);
        write_be32(bytes, data_offset, 30U);
        return bytes;
    }

    std::vector<std::uint8_t> make_single_string_field_bcsv(std::string_view field_name,
                                                            std::span<const std::uint8_t> value) {
        constexpr auto field_count = std::size_t{1U};
        constexpr auto data_offset = std::size_t{0x10U + field_count * 0x0cU};
        constexpr auto entry_size = std::size_t{4U};
        auto bytes = std::vector<std::uint8_t>(data_offset + entry_size + value.size() + 1U, 0U);
        write_be32(bytes, 0x00U, 1U);
        write_be32(bytes, 0x04U, field_count);
        write_be32(bytes, 0x08U, data_offset);
        write_be32(bytes, 0x0cU, entry_size);
        write_bcsv_field(bytes, 0U, field_name, 0U, smgpc::resource::BcsvFieldType::StringOffset);
        std::copy(value.begin(), value.end(), bytes.begin() + data_offset + entry_size);
        return bytes;
    }

    std::vector<std::uint8_t> make_camera_bad_optional_type_bcsv() {
        constexpr auto field_count = std::size_t{2U};
        constexpr auto data_offset = std::size_t{0x10U + field_count * 0x0cU};
        constexpr auto entry_size = std::size_t{8U};
        constexpr auto string_table = data_offset + entry_size;
        constexpr auto strings = std::array<std::uint8_t, 9U>{'p', 'a', 'r', 't', 0U, 'b', 'a', 'd', 0U};
        auto bytes = std::vector<std::uint8_t>(string_table + strings.size(), 0U);
        write_be32(bytes, 0x00U, 1U);
        write_be32(bytes, 0x04U, field_count);
        write_be32(bytes, 0x08U, data_offset);
        write_be32(bytes, 0x0cU, entry_size);
        write_bcsv_field(bytes, 0U, "PartName", 0U, smgpc::resource::BcsvFieldType::StringOffset);
        write_bcsv_field(bytes, 1U, "AnimCameraStartFrame", 4U, smgpc::resource::BcsvFieldType::StringOffset);
        write_be32(bytes, data_offset, 0U);
        write_be32(bytes, data_offset + 4U, 5U);
        std::copy(strings.begin(), strings.end(), bytes.begin() + string_table);
        return bytes;
    }

    std::vector<std::uint8_t> make_27_part_time_bcsv(std::optional<std::size_t> suspend_row = std::nullopt) {
        constexpr auto durations = std::array<std::int32_t, 27U>{
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            1,
            420,
            120,
            150,
            120,
            240,
            120,
            500,
            60,
            120,
            370,
            330,
            1,
        };
        constexpr auto field_count = std::size_t{3U};
        constexpr auto data_offset = std::size_t{0x10U + field_count * 0x0cU};
        constexpr auto entry_size = std::size_t{12U};

        auto part_names = std::array<std::string, 27U>{};
        for (auto row = std::size_t{}; row < part_names.size(); ++row) {
            part_names[row] = row < 15U ? "prefix-" + std::to_string(row) : "spin-" + std::to_string(row - 14U);
        }

        auto string_bytes = std::size_t{};
        for (const auto &part_name : part_names) {
            string_bytes += part_name.size() + 1U;
        }
        auto bytes = std::vector<std::uint8_t>(data_offset + durations.size() * entry_size + string_bytes, 0U);
        write_be32(bytes, 0x00U, static_cast<std::uint32_t>(durations.size()));
        write_be32(bytes, 0x04U, field_count);
        write_be32(bytes, 0x08U, data_offset);
        write_be32(bytes, 0x0cU, entry_size);
        write_bcsv_field(bytes, 0U, "SuspendFlag", 4U, smgpc::resource::BcsvFieldType::Int32);
        write_bcsv_field(bytes, 1U, "PartName", 8U, smgpc::resource::BcsvFieldType::StringOffset);
        write_bcsv_field(bytes, 2U, "TotalStep", 0U, smgpc::resource::BcsvFieldType::Int32);

        auto string_offset = std::size_t{};
        const auto string_table = data_offset + durations.size() * entry_size;
        for (auto row = std::size_t{}; row < durations.size(); ++row) {
            const auto entry = data_offset + row * entry_size;
            write_be32(bytes, entry, static_cast<std::uint32_t>(durations[row]));
            write_be32(bytes, entry + 4U, suspend_row == row ? 1U : 0U);
            write_be32(bytes, entry + 8U, static_cast<std::uint32_t>(string_offset));
            for (auto index = std::size_t{}; index < part_names[row].size(); ++index) {
                bytes[string_table + string_offset + index] = static_cast<std::uint8_t>(part_names[row][index]);
            }
            string_offset += part_names[row].size() + 1U;
        }
        return bytes;
    }

    smgpc::resource::RarcArchive make_single_file_rarc(std::string_view file_name,
                                                       const std::vector<std::uint8_t> &file_data) {
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

        for (auto index = std::size_t{}; index < file_name.size(); ++index) {
            bytes[string_table_offset + index] = static_cast<std::uint8_t>(file_name[index]);
        }
        std::copy(file_data.begin(), file_data.end(), bytes.begin() + file_data_offset);
        return smgpc::resource::RarcArchive::from_bytes(std::move(bytes));
    }

    smgpc::compat::DemoSheetRuntime make_timing_fixture() {
        const auto archive = make_single_file_rarc("dEmOfIxTuReTiMe.BcSv", make_27_part_time_bcsv());
        return smgpc::compat::DemoSheetRuntime::load(archive, "Fixture");
    }

    std::optional<std::filesystem::path> real_demo_sheet_path() {
        auto root = std::filesystem::current_path();
        for (auto depth = 0U; depth < 8U; ++depth) {
            const auto candidates = std::array{
                root / "container/orig/RMGK01/files/ObjectData/DemoSheet.arc",
                root / "pc-port/container/orig/RMGK01/files/ObjectData/DemoSheet.arc",
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

    void test_optional_real_tico_guide_schema_and_rows() {
        const auto archive_path = real_demo_sheet_path();
        if (!archive_path.has_value()) {
            std::cout << "[skip] extracted RMGK01 DemoSheet schema check\n";
            return;
        }
        const auto archive = smgpc::resource::RarcArchive::from_file(*archive_path);
        const auto runtime = smgpc::compat::DemoSheetRuntime::load(archive, "TicoGuideDemo");
        using smgpc::compat::DemoSheetTable;
        require(runtime.has_table(DemoSheetTable::Time) && runtime.has_table(DemoSheetTable::SubPart) &&
                    runtime.has_table(DemoSheetTable::Player) && runtime.has_table(DemoSheetTable::Camera) &&
                    runtime.has_table(DemoSheetTable::Action) && runtime.has_table(DemoSheetTable::Wipe) &&
                    runtime.has_table(DemoSheetTable::Sound),
                "the real Tico guide export must expose all seven tables");
        require(runtime.time_rows().size() == 27U, "the real Tico guide Time table must have 27 rows");
        require(runtime.sub_part_rows().empty(), "the real Tico guide SubPart table must be present and empty");
        require(runtime.player_rows().size() == 5U, "the real Tico guide Player table must have five rows");
        require(runtime.camera_rows().size() == 2U, "the real Tico guide Camera table must have two rows");
        require(runtime.action_rows().size() == 36U, "the real Tico guide Action table must have 36 rows");
        require(runtime.wipe_rows().size() == 3U, "the real Tico guide Wipe table must have three rows");
        require(runtime.sound_rows().empty(), "the real Tico guide Sound table must be present and empty");

        require(runtime.time_rows()[15U].part_name == "スピンゲット[デモ1]" &&
                    runtime.time_rows()[15U].total_step == 420 && !runtime.time_rows()[15U].suspend,
                "Time fields must be decoded by hash despite their descriptor offsets");
        require(runtime.action_rows()[12U].cast_id == -1 && runtime.action_rows()[12U].action_type == 2 &&
                    runtime.action_rows()[12U].position_name == "MarioDemoPos4",
                "Action fields must preserve signed wildcard IDs and dispatch arguments");
        require(runtime.player_rows()[3U].position_name == "MarioDemoPos4",
                "Player position data must be preserved");
        require(runtime.camera_rows()[0U].target_cast_id == -1 &&
                    runtime.camera_rows()[0U].animation_start_frame == -1,
                "Camera fields must preserve signed sentinel values");
        require(runtime.wipe_rows()[2U].wipe_type == 0 && runtime.wipe_rows()[2U].wipe_frame == -1,
                "Wipe fields must preserve source values");
    }

    void test_optional_real_archive_all_time_sheet_families() {
        const auto archive_path = real_demo_sheet_path();
        if (!archive_path.has_value()) {
            std::cout << "[skip] extracted RMGK01 archive-wide DemoSheet check\n";
            return;
        }

        const auto archive = smgpc::resource::RarcArchive::from_file(*archive_path);
        auto families = std::set<std::string>{};
        constexpr auto prefix = std::string_view{"demo"};
        constexpr auto suffix = std::string_view{"time.bcsv"};
        for (const auto &entry : archive.entries()) {
            const auto name = lower_ascii(entry.name);
            if (name.size() <= prefix.size() + suffix.size() || !name.starts_with(prefix) || !name.ends_with(suffix)) {
                continue;
            }
            families.insert(name.substr(prefix.size(), name.size() - prefix.size() - suffix.size()));
        }
        require(families.size() == 138U, "the real DemoSheet archive must contain 138 Time-sheet families");

        for (const auto &family : families) {
            try {
                const auto runtime = smgpc::compat::DemoSheetRuntime::load(archive, family);
                require(runtime.has_table(smgpc::compat::DemoSheetTable::Time),
                        "an enumerated Time family must retain its Time table");
            } catch (const std::exception &error) {
                throw std::runtime_error("cannot load real DemoSheet family '" + family + "': " + error.what());
            }
        }
    }

    void test_case_insensitive_lookup_and_exact_part_name() {
        const auto archive = make_single_file_rarc("dEmOfIxTuReTiMe.BcSv", make_27_part_time_bcsv());
        auto runtime = smgpc::compat::DemoSheetRuntime::load(archive, "fIxTuRe");
        require(runtime.time_rows().size() == 27U,
                "archive table lookup must be ASCII case-insensitive");
        require(runtime.start_at_part("spin-1 ") == smgpc::compat::DemoSheetStartResult::PartNotFound,
                "part lookup must not normalize or alias the requested name");
        require(!runtime.is_active(), "a failed exact part lookup must leave the time keeper inactive");
    }

    void test_cp932_time_part_and_source_defaults() {
        constexpr auto encoded_part = std::array<std::uint8_t, 19U>{
            0x83U,
            0x58U,
            0x83U,
            0x73U,
            0x83U,
            0x93U,
            0x83U,
            0x51U,
            0x83U,
            0x62U,
            0x83U,
            0x67U,
            0x5bU,
            0x83U,
            0x66U,
            0x83U,
            0x82U,
            0x31U,
            0x5dU,
        };
        const auto archive = make_single_file_rarc(
            "DemoEncodedTime.bcsv", make_single_string_field_bcsv("PartName", encoded_part));
        auto runtime = smgpc::compat::DemoSheetRuntime::load(archive, "Encoded");
        require(runtime.time_rows().size() == 1U &&
                    runtime.time_rows()[0U].part_name == "スピンゲット[デモ1]" &&
                    runtime.time_rows()[0U].total_step == 1 && !runtime.time_rows()[0U].suspend,
                "a PartName-only Time row must decode CP932 and retain source numeric defaults");
        require(runtime.start_at_part("スピンゲット[デモ1]") ==
                        smgpc::compat::DemoSheetStartResult::Started &&
                    runtime.advance() && runtime.is_demo_last_step() && !runtime.advance(),
                "the decoded Japanese part name must be an exact executable one-frame part");
    }

    void test_sparse_optional_columns_use_source_defaults() {
        constexpr auto part_name = std::array<std::uint8_t, 4U>{'p', 'a', 'r', 't'};

        const auto camera_archive = make_single_file_rarc(
            "DemoSparseCamera.bcsv", make_single_string_field_bcsv("PartName", part_name));
        const auto camera = smgpc::compat::DemoSheetRuntime::load(camera_archive, "Sparse");
        require(camera.camera_rows().size() == 1U && camera.camera_rows()[0U].part_name == "part" &&
                    camera.camera_rows()[0U].target_name.empty() && camera.camera_rows()[0U].target_cast_id == -1 &&
                    camera.camera_rows()[0U].animation_name.empty() && camera.camera_rows()[0U].animation_start_frame == -1 &&
                    camera.camera_rows()[0U].animation_end_frame == -1 && !camera.camera_rows()[0U].continuous,
                "a PartName-only Camera row must preserve every constructor default");

        const auto action_archive = make_single_file_rarc(
            "DemoSparseAction.bcsv", make_single_string_field_bcsv("PartName", part_name));
        const auto action = smgpc::compat::DemoSheetRuntime::load(action_archive, "Sparse");
        require(action.action_rows().size() == 1U && action.action_rows()[0U].cast_name.empty() &&
                    action.action_rows()[0U].cast_id == -1 && action.action_rows()[0U].action_type == 0 &&
                    action.action_rows()[0U].position_name.empty() && action.action_rows()[0U].animation_name.empty(),
                "a PartName-only Action row must preserve every constructor default");

        const auto wipe_archive = make_single_file_rarc(
            "DemoSparseWipe.bcsv", make_single_string_field_bcsv("PartName", part_name));
        const auto wipe = smgpc::compat::DemoSheetRuntime::load(wipe_archive, "Sparse");
        require(wipe.wipe_rows().size() == 1U && wipe.wipe_rows()[0U].wipe_name == "フェードワイプ" &&
                    wipe.wipe_rows()[0U].wipe_type == 0 && wipe.wipe_rows()[0U].wipe_frame == -1,
                "a PartName-only Wipe row must preserve the original fade-wipe defaults");

        const auto sound_archive = make_single_file_rarc(
            "DemoSparseSound.bcsv", make_single_string_field_bcsv("PartName", part_name));
        const auto sound = smgpc::compat::DemoSheetRuntime::load(sound_archive, "Sparse");
        require(sound.sound_rows().size() == 1U && sound.sound_rows()[0U].bgm.empty() &&
                    sound.sound_rows()[0U].system_sound.empty() && sound.sound_rows()[0U].return_bgm == 0 &&
                    sound.sound_rows()[0U].bgm_wipeout_frame == -1,
                "a PartName-only Sound row must preserve every constructor default");
    }

    void test_spin_subset_timekeeper_boundaries() {
        auto runtime = make_timing_fixture();
        require(runtime.time_rows().size() == 27U &&
                    runtime.start_at_part("spin-1") == smgpc::compat::DemoSheetStartResult::Started,
                "the spin subset must start at its exact row-15 part name");
        require(runtime.current_part_index() == 15U && runtime.current_part_step() == -1,
                "start-at-part must select row 15 before the first update");

        auto dispatch_frames = std::size_t{};
        while (runtime.advance()) {
            ++dispatch_frames;
            if (dispatch_frames == 1U) {
                require(runtime.current_part_index() == 15U && runtime.current_part_step() == 0 &&
                            runtime.is_part_first_step("spin-1") &&
                            runtime.current_part_total_step() == 420,
                        "the first dispatch tick must expose row 15 step zero");
            } else if (dispatch_frames == 420U) {
                require(runtime.current_part_index() == 15U && runtime.current_part_step() == 419 &&
                            runtime.is_part_last_step("spin-1"),
                        "a 420-step part must expose step 419 as its last dispatch tick");
            } else if (dispatch_frames == 421U) {
                require(runtime.current_part_index() == 16U && runtime.current_part_step() == 0 &&
                            runtime.is_part_first_step("spin-2"),
                        "the boundary update must select the next part at step zero");
            } else if (dispatch_frames == 2551U) {
                require(runtime.current_part_index() == 26U && runtime.current_part_step() == 0 &&
                            runtime.is_last_part() && runtime.is_demo_last_step(),
                        "the final one-step part must be dispatchable before the demo ends");
            }
        }
        require(dispatch_frames == 2551U,
                "starting at real row 15 must yield exactly 2551 dispatch frames");
        require(!runtime.is_active() && !runtime.current_part_index().has_value() &&
                    !runtime.current_part_step().has_value(),
                "the update after the last frame must cleanly end the time keeper");
    }

    void test_pause_matches_source_early_step_correction() {
        auto runtime = make_timing_fixture();
        require(runtime.start_at_part("spin-1") == smgpc::compat::DemoSheetStartResult::Started,
                "pause fixture must start");
        runtime.pause();
        require(runtime.is_paused() && !runtime.advance() && runtime.is_active() &&
                    runtime.current_part_step() == 0,
                "the first paused update must mirror DemoTimeKeeper's -1 to 0 correction without dispatch");
        require(!runtime.advance() && runtime.current_part_step() == 1,
                "the second paused update must mirror DemoTimeKeeper's 0 to 1 correction");
        require(!runtime.advance() && runtime.current_part_step() == 1,
                "later paused updates must freeze after the source early-step correction");
        runtime.resume();
        require(!runtime.is_paused() && runtime.advance() && runtime.current_part_step() == 2,
                "resuming must continue from the corrected source step");
    }

    void test_paused_final_step_is_not_last_part() {
        auto runtime = make_timing_fixture();
        require(runtime.start_at_part("spin-12") == smgpc::compat::DemoSheetStartResult::Started &&
                    runtime.advance() && runtime.is_part_last_step("spin-12") &&
                    runtime.is_last_part() && runtime.is_demo_last_step(),
                "the final one-frame part must initially expose its last-step state");
        runtime.pause();
        require(runtime.is_paused() && !runtime.is_last_part() && !runtime.is_demo_last_step() &&
                    runtime.is_part_last_step("spin-12"),
                "DemoTimeKeeper::isPartLast must be false while paused without hiding the current part step");
        require(!runtime.advance() && runtime.is_active() && runtime.current_part_step() == 1,
                "a paused final step must remain active during the source early-step correction");
        runtime.resume();
        require(!runtime.advance() && !runtime.is_active(),
                "resuming after the corrected final step must end before another dispatch");
    }

    void test_suspend_ends_before_following_part_dispatch() {
        const auto archive = make_single_file_rarc("DemoSuspendTime.bcsv", make_27_part_time_bcsv(0U));
        auto runtime = smgpc::compat::DemoSheetRuntime::load(archive, "Suspend");
        require(runtime.start() == smgpc::compat::DemoSheetStartResult::Started && runtime.advance() &&
                    runtime.current_part_index() == 0U && runtime.current_part_step() == 0 &&
                    runtime.is_part_last_step("prefix-0"),
                "a one-frame suspended part must expose its step-zero dispatch tick");
        require(!runtime.advance() && !runtime.is_active(),
                "the next update must end a suspended part before selecting the following row");
    }

    void test_missing_and_empty_time_reject_start() {
        const auto lock_archive = make_single_file_rarc("DemoSpinGetDemoLockFile.txt",
                                                        std::vector<std::uint8_t>{'a', 'u', 't', 'h', 'o', 'r'});
        auto missing = smgpc::compat::DemoSheetRuntime::load(lock_archive, "SpinGetDemo");
        require(!missing.has_table(smgpc::compat::DemoSheetTable::Time) && missing.time_rows().empty(),
                "the lock-only SpinGet export must not be treated as a Time table");
        require(missing.start() == smgpc::compat::DemoSheetStartResult::MissingTimeTable &&
                    !missing.is_active(),
                "a missing Time table must reject start without indexing rows");

        if (const auto archive_path = real_demo_sheet_path(); archive_path.has_value()) {
            const auto real_archive = smgpc::resource::RarcArchive::from_file(*archive_path);
            auto real_missing = smgpc::compat::DemoSheetRuntime::load(real_archive, "SpinGetDemo");
            require(real_missing.start() == smgpc::compat::DemoSheetStartResult::MissingTimeTable,
                    "the real lock-only SpinGet export must reject start as missing Time");
        }

        const auto empty_archive = make_single_file_rarc("dEmOeMpTyTiMe.BcSv", make_empty_time_bcsv());
        auto empty = smgpc::compat::DemoSheetRuntime::load(empty_archive, "Empty");
        require(empty.has_table(smgpc::compat::DemoSheetTable::Time) && empty.time_rows().empty(),
                "an empty Time table must remain distinguishable from a missing table");
        require(empty.start() == smgpc::compat::DemoSheetStartResult::EmptyTimeTable &&
                    !empty.is_active(),
                "a present zero-row Time table must reject start safely");
    }

    void test_malformed_schemas_are_contextual_errors() {
        const auto archive = make_single_file_rarc("DemoBrokenTime.bcsv", make_missing_part_name_bcsv());
        try {
            static_cast<void>(smgpc::compat::DemoSheetRuntime::load(archive, "Broken"));
        } catch (const smgpc::compat::DemoSheetParseError &error) {
            const auto message = std::string_view(error.what());
            require(message.find("DemoBrokenTime.bcsv") != std::string_view::npos &&
                        message.find("PartName") != std::string_view::npos,
                    "a malformed schema error must identify its table and missing field");
            const auto camera_archive = make_single_file_rarc(
                "DemoBadCamera.bcsv", make_camera_bad_optional_type_bcsv());
            try {
                static_cast<void>(smgpc::compat::DemoSheetRuntime::load(camera_archive, "Bad"));
            } catch (const smgpc::compat::DemoSheetParseError &camera_error) {
                const auto camera_message = std::string_view(camera_error.what());
                require(camera_message.find("DemoBadCamera.bcsv") != std::string_view::npos &&
                            camera_message.find("AnimCameraStartFrame") != std::string_view::npos,
                        "an optional-field type error must identify its table and field");
                return;
            }
            throw std::runtime_error("a present Camera frame field with string type must fail schema validation");
        }
        throw std::runtime_error("a nonempty Time table missing PartName must fail schema validation");
    }

}  // namespace

int main() {
    try {
        constexpr auto tests = std::array{
            std::pair{"optional real Tico guide schema and rows", &test_optional_real_tico_guide_schema_and_rows},
            std::pair{"optional real archive all Time families", &test_optional_real_archive_all_time_sheet_families},
            std::pair{"case-insensitive lookup and exact part name", &test_case_insensitive_lookup_and_exact_part_name},
            std::pair{"CP932 Time part and source defaults", &test_cp932_time_part_and_source_defaults},
            std::pair{"sparse optional columns use source defaults", &test_sparse_optional_columns_use_source_defaults},
            std::pair{"spin subset timekeeper boundaries", &test_spin_subset_timekeeper_boundaries},
            std::pair{"pause matches source early-step correction", &test_pause_matches_source_early_step_correction},
            std::pair{"paused final step is not last part", &test_paused_final_step_is_not_last_part},
            std::pair{"suspend ends before following dispatch", &test_suspend_ends_before_following_part_dispatch},
            std::pair{"missing and empty Time reject start", &test_missing_and_empty_time_reject_start},
            std::pair{"malformed schemas are contextual errors", &test_malformed_schemas_are_contextual_errors},
        };
        for (const auto &[name, test] : tests) {
            test();
            std::cout << "[ok] " << name << '\n';
        }
        std::cout << tests.size() << " DemoSheet runtime test(s) passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "[failed] " << error.what() << '\n';
        return 1;
    }
}
