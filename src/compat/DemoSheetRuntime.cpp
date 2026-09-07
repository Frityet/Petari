#include "compat/DemoSheetRuntime.hpp"

#include "resource/BcsvTable.hpp"
#include "resource/RarcArchive.hpp"
#include "resource/TextEncoding.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace smgpc::compat {
    namespace {

        [[nodiscard]] constexpr std::size_t table_index(DemoSheetTable table) {
            return static_cast<std::size_t>(table);
        }

        [[nodiscard]] bool is_string_field(smgpc::resource::BcsvFieldType type) {
            using smgpc::resource::BcsvFieldType;
            return type == BcsvFieldType::InlineString || type == BcsvFieldType::StringOffset;
        }

        [[nodiscard]] bool is_integer_field(smgpc::resource::BcsvFieldType type) {
            using smgpc::resource::BcsvFieldType;
            return type == BcsvFieldType::Int32 || type == BcsvFieldType::UInt32 || type == BcsvFieldType::Int16 || type == BcsvFieldType::Int8;
        }

        void require_string_field(const smgpc::resource::BcsvTable &table, std::string_view name) {
            const auto index = table.field_index(name);
            if (!index.has_value()) {
                throw std::runtime_error("missing required field '" + std::string(name) + "'");
            }

            const auto type = table.fields()[*index].type;
            if (!is_string_field(type)) {
                throw std::runtime_error("field '" + std::string(name) + "' has incompatible type " + smgpc::resource::bcsv_field_type_name(type));
            }
        }

        void validate_optional_string_field(const smgpc::resource::BcsvTable &table, std::string_view name) {
            const auto index = table.field_index(name);
            if (index.has_value() && !is_string_field(table.fields()[*index].type)) {
                throw std::runtime_error("optional field '" + std::string(name) + "' has incompatible type " +
                                         smgpc::resource::bcsv_field_type_name(table.fields()[*index].type));
            }
        }

        void validate_optional_integer_field(const smgpc::resource::BcsvTable &table, std::string_view name) {
            const auto index = table.field_index(name);
            if (index.has_value() && !is_integer_field(table.fields()[*index].type)) {
                throw std::runtime_error("optional field '" + std::string(name) + "' has incompatible type " +
                                         smgpc::resource::bcsv_field_type_name(table.fields()[*index].type));
            }
        }

        [[nodiscard]] std::int32_t read_integer_or(const smgpc::resource::BcsvTable &table, std::size_t row, std::string_view name,
                                                   std::int32_t default_value) {
            return table.get_s32(row, name).value_or(default_value);
        }

        [[nodiscard]] std::string read_string(const smgpc::resource::BcsvTable &table, std::size_t row, std::string_view name) {
            const auto value = table.get_string(row, name);
            if (!value.has_value()) {
                throw std::runtime_error("cannot read string field '" + std::string(name) + "' at row " + std::to_string(row));
            }
            return smgpc::resource::decode_cp932(*value);
        }

        [[nodiscard]] std::string read_string_or(const smgpc::resource::BcsvTable &table, std::size_t row, std::string_view name,
                                                 std::string default_value) {
            const auto value = table.get_string(row, name);
            return value.has_value() ? smgpc::resource::decode_cp932(*value) : std::move(default_value);
        }

        void parse_time(const smgpc::resource::BcsvTable &table, std::vector<DemoTimeRow> &rows) {
            if (table.entry_count() == 0U) {
                return;
            }
            require_string_field(table, "PartName");
            validate_optional_integer_field(table, "TotalStep");
            validate_optional_integer_field(table, "SuspendFlag");
            rows.reserve(table.entry_count());
            for (auto row = std::size_t{}; row < table.entry_count(); ++row) {
                rows.push_back({
                    .part_name = read_string(table, row, "PartName"),
                    .total_step = read_integer_or(table, row, "TotalStep", 1),
                    .suspend = read_integer_or(table, row, "SuspendFlag", 0) != 0,
                });
            }
        }

        void parse_sub_part(const smgpc::resource::BcsvTable &table, std::vector<DemoSubPartRow> &rows) {
            if (table.entry_count() == 0U) {
                return;
            }
            require_string_field(table, "SubPartName");
            require_string_field(table, "MainPartName");
            validate_optional_integer_field(table, "SubPartTotalStep");
            validate_optional_integer_field(table, "MainPartStep");
            rows.reserve(table.entry_count());
            for (auto row = std::size_t{}; row < table.entry_count(); ++row) {
                rows.push_back({
                    .sub_part_name = read_string(table, row, "SubPartName"),
                    .sub_part_total_step = read_integer_or(table, row, "SubPartTotalStep", 1),
                    .main_part_name = read_string(table, row, "MainPartName"),
                    .main_part_step = read_integer_or(table, row, "MainPartStep", 1),
                });
            }
        }

        void parse_player(const smgpc::resource::BcsvTable &table, std::vector<DemoPlayerRow> &rows) {
            if (table.entry_count() == 0U) {
                return;
            }
            require_string_field(table, "PartName");
            require_string_field(table, "PosName");
            require_string_field(table, "BckName");
            rows.reserve(table.entry_count());
            for (auto row = std::size_t{}; row < table.entry_count(); ++row) {
                rows.push_back({
                    .part_name = read_string(table, row, "PartName"),
                    .position_name = read_string(table, row, "PosName"),
                    .bck_name = read_string(table, row, "BckName"),
                });
            }
        }

        void parse_camera(const smgpc::resource::BcsvTable &table, std::vector<DemoCameraRow> &rows) {
            if (table.entry_count() == 0U) {
                return;
            }
            require_string_field(table, "PartName");
            validate_optional_string_field(table, "CameraTargetName");
            validate_optional_integer_field(table, "CameraTargetCastID");
            validate_optional_string_field(table, "AnimCameraName");
            validate_optional_integer_field(table, "AnimCameraStartFrame");
            validate_optional_integer_field(table, "AnimCameraEndFrame");
            validate_optional_integer_field(table, "IsContinuous");
            rows.reserve(table.entry_count());
            for (auto row = std::size_t{}; row < table.entry_count(); ++row) {
                rows.push_back({
                    .part_name = read_string(table, row, "PartName"),
                    .target_name = read_string_or(table, row, "CameraTargetName", {}),
                    .target_cast_id = read_integer_or(table, row, "CameraTargetCastID", -1),
                    .animation_name = read_string_or(table, row, "AnimCameraName", {}),
                    .animation_start_frame = read_integer_or(table, row, "AnimCameraStartFrame", -1),
                    .animation_end_frame = read_integer_or(table, row, "AnimCameraEndFrame", -1),
                    .continuous = read_integer_or(table, row, "IsContinuous", -1) == 1,
                });
            }
        }

        void parse_action(const smgpc::resource::BcsvTable &table, std::vector<DemoActionRow> &rows) {
            if (table.entry_count() == 0U) {
                return;
            }
            require_string_field(table, "PartName");
            validate_optional_string_field(table, "CastName");
            validate_optional_integer_field(table, "CastID");
            validate_optional_integer_field(table, "ActionType");
            validate_optional_string_field(table, "PosName");
            validate_optional_string_field(table, "AnimName");
            rows.reserve(table.entry_count());
            for (auto row = std::size_t{}; row < table.entry_count(); ++row) {
                rows.push_back({
                    .part_name = read_string(table, row, "PartName"),
                    .cast_name = read_string_or(table, row, "CastName", {}),
                    .cast_id = read_integer_or(table, row, "CastID", -1),
                    .action_type = read_integer_or(table, row, "ActionType", 0),
                    .position_name = read_string_or(table, row, "PosName", {}),
                    .animation_name = read_string_or(table, row, "AnimName", {}),
                });
            }
        }

        void parse_wipe(const smgpc::resource::BcsvTable &table, std::vector<DemoWipeRow> &rows) {
            if (table.entry_count() == 0U) {
                return;
            }
            require_string_field(table, "PartName");
            validate_optional_string_field(table, "WipeName");
            validate_optional_integer_field(table, "WipeType");
            validate_optional_integer_field(table, "WipeFrame");
            rows.reserve(table.entry_count());
            for (auto row = std::size_t{}; row < table.entry_count(); ++row) {
                rows.push_back({
                    .part_name = read_string(table, row, "PartName"),
                    .wipe_name = read_string_or(table, row, "WipeName", "フェードワイプ"),
                    .wipe_type = read_integer_or(table, row, "WipeType", 0),
                    .wipe_frame = read_integer_or(table, row, "WipeFrame", -1),
                });
            }
        }

        void parse_sound(const smgpc::resource::BcsvTable &table, std::vector<DemoSoundRow> &rows) {
            if (table.entry_count() == 0U) {
                return;
            }
            require_string_field(table, "PartName");
            validate_optional_string_field(table, "Bgm");
            validate_optional_string_field(table, "SystemSe");
            validate_optional_integer_field(table, "ReturnBgm");
            validate_optional_integer_field(table, "BgmWipeoutFrame");
            rows.reserve(table.entry_count());
            for (auto row = std::size_t{}; row < table.entry_count(); ++row) {
                rows.push_back({
                    .part_name = read_string(table, row, "PartName"),
                    .bgm = read_string_or(table, row, "Bgm", {}),
                    .system_sound = read_string_or(table, row, "SystemSe", {}),
                    .return_bgm = read_integer_or(table, row, "ReturnBgm", 0),
                    .bgm_wipeout_frame = read_integer_or(table, row, "BgmWipeoutFrame", -1),
                });
            }
        }

    }  // namespace

    std::string_view demo_sheet_start_result_name(DemoSheetStartResult result) {
        switch (result) {
        case DemoSheetStartResult::Started:
            return "started";
        case DemoSheetStartResult::MissingTimeTable:
            return "missing-time-table";
        case DemoSheetStartResult::EmptyTimeTable:
            return "empty-time-table";
        case DemoSheetStartResult::PartNotFound:
            return "part-not-found";
        }
        return "unknown";
    }

    DemoSheetRuntime DemoSheetRuntime::load(const smgpc::resource::RarcArchive &archive, std::string_view time_sheet_name) {
        auto runtime = DemoSheetRuntime(std::string(time_sheet_name));
        if (time_sheet_name.empty()) {
            return runtime;
        }
        const auto load_table = [&](DemoSheetTable kind, std::string_view suffix, auto &&parse) {
            const auto file_name = "Demo" + runtime._time_sheet_name + std::string(suffix) + ".bcsv";
            const auto *entry = archive.find_by_basename(file_name);
            if (entry == nullptr) {
                return;
            }

            runtime._present_tables[table_index(kind)] = true;
            try {
                const auto table = smgpc::resource::BcsvTable::from_bytes(archive.file_data(*entry));
                parse(table);
            } catch (const std::exception &error) {
                throw DemoSheetParseError(file_name + ": " + error.what());
            }
        };

        load_table(DemoSheetTable::Time, "Time", [&](const auto &table) { parse_time(table, runtime._time_rows); });
        load_table(DemoSheetTable::SubPart, "SubPart", [&](const auto &table) { parse_sub_part(table, runtime._sub_part_rows); });
        load_table(DemoSheetTable::Player, "Player", [&](const auto &table) { parse_player(table, runtime._player_rows); });
        load_table(DemoSheetTable::Camera, "Camera", [&](const auto &table) { parse_camera(table, runtime._camera_rows); });
        load_table(DemoSheetTable::Action, "Action", [&](const auto &table) { parse_action(table, runtime._action_rows); });
        load_table(DemoSheetTable::Wipe, "Wipe", [&](const auto &table) { parse_wipe(table, runtime._wipe_rows); });
        load_table(DemoSheetTable::Sound, "Sound", [&](const auto &table) { parse_sound(table, runtime._sound_rows); });
        runtime._sub_part_remaining.assign(runtime._sub_part_rows.size(), -1);
        return runtime;
    }

    DemoSheetRuntime::DemoSheetRuntime(std::string time_sheet_name)
        : _time_sheet_name(std::move(time_sheet_name)) {
    }

    const std::string &DemoSheetRuntime::time_sheet_name() const {
        return _time_sheet_name;
    }

    bool DemoSheetRuntime::has_table(DemoSheetTable table) const {
        const auto index = table_index(table);
        return index < _present_tables.size() && _present_tables[index];
    }

    std::span<const DemoTimeRow> DemoSheetRuntime::time_rows() const {
        return _time_rows;
    }

    std::span<const DemoSubPartRow> DemoSheetRuntime::sub_part_rows() const {
        return _sub_part_rows;
    }

    std::span<const DemoPlayerRow> DemoSheetRuntime::player_rows() const {
        return _player_rows;
    }

    std::span<const DemoCameraRow> DemoSheetRuntime::camera_rows() const {
        return _camera_rows;
    }

    std::span<const DemoActionRow> DemoSheetRuntime::action_rows() const {
        return _action_rows;
    }

    std::span<const DemoWipeRow> DemoSheetRuntime::wipe_rows() const {
        return _wipe_rows;
    }

    std::span<const DemoSoundRow> DemoSheetRuntime::sound_rows() const {
        return _sound_rows;
    }

    DemoSheetStartResult DemoSheetRuntime::start() {
        return start_at_index(0U);
    }

    DemoSheetStartResult DemoSheetRuntime::start_at_part(std::string_view part_name) {
        stop();
        if (!has_table(DemoSheetTable::Time)) {
            return DemoSheetStartResult::MissingTimeTable;
        }
        if (_time_rows.empty()) {
            return DemoSheetStartResult::EmptyTimeTable;
        }

        const auto found = std::ranges::find(_time_rows, part_name, &DemoTimeRow::part_name);
        if (found == _time_rows.end()) {
            return DemoSheetStartResult::PartNotFound;
        }
        return start_at_index(static_cast<std::size_t>(std::distance(_time_rows.begin(), found)));
    }

    DemoSheetStartResult DemoSheetRuntime::start_at_index(std::size_t index) {
        stop();
        if (!has_table(DemoSheetTable::Time)) {
            return DemoSheetStartResult::MissingTimeTable;
        }
        if (_time_rows.empty()) {
            return DemoSheetStartResult::EmptyTimeTable;
        }
        if (index >= _time_rows.size()) {
            return DemoSheetStartResult::PartNotFound;
        }

        _part_index = index;
        _part_step = -1;
        _active = true;
        return DemoSheetStartResult::Started;
    }

    void DemoSheetRuntime::stop() {
        _part_index.reset();
        _part_step = -1;
        _active = false;
        std::ranges::fill(_sub_part_remaining, -1);
    }

    void DemoSheetRuntime::pause() {
        _paused = true;
    }

    void DemoSheetRuntime::resume() {
        _paused = false;
    }

    bool DemoSheetRuntime::advance() {
        if (!_active || !_part_index.has_value()) {
            return false;
        }

        // DemoTimeKeeper::update performs this early-step correction even
        // while paused. DemoExecutor still suppresses dispatch for the tick.
        if (_paused) {
            if (_part_step <= 0) {
                ++_part_step;
            }
            return false;
        }

        const auto *part = current_part();
        if (part == nullptr) {
            return false;
        }
        ++_part_step;
        if (part->total_step > _part_step) {
            update_sub_parts();
            return true;
        }

        if (part->suspend) {
            stop();
            return false;
        }

        ++*_part_index;
        if (*_part_index < _time_rows.size()) {
            _part_step = 0;
            update_sub_parts();
            return true;
        }

        // DemoTimeKeeper retains the final part pointer after its index moves
        // one-past-the-end. Its isDemoEnd comparison also requires the old
        // TotalStep to be >= the current step. Usually equality ends here. If
        // pause correction has already advanced a one-frame part past that
        // equality, the source remains active and keeps advancing both values.
        if (*_part_index == _time_rows.size() && part->total_step >= _part_step) {
            stop();
            return false;
        }

        update_sub_parts();
        return true;
    }

    void DemoSheetRuntime::update_sub_parts() {
        for (auto index = std::size_t{}; index < _sub_part_rows.size(); ++index) {
            auto &remaining = _sub_part_remaining[index];
            if (remaining > 0) {
                --remaining;
            }

            const auto &row = _sub_part_rows[index];
            if (is_part_active(row.main_part_name) &&
                part_step(row.main_part_name) == row.main_part_step) {
                remaining = row.sub_part_total_step;
            }
        }
    }

    bool DemoSheetRuntime::is_active() const {
        return _active;
    }

    bool DemoSheetRuntime::is_paused() const {
        return _paused;
    }

    bool DemoSheetRuntime::is_part_active(std::string_view part_name) const {
        const auto *part = current_part();
        if (part != nullptr && part->part_name == part_name) {
            return true;
        }
        const auto found = find_sub_part(part_name);
        return found.has_value() && _sub_part_remaining[*found] > 0;
    }

    bool DemoSheetRuntime::is_part_first_step(std::string_view part_name) const {
        return is_part_active(part_name) && part_step(part_name) == 0;
    }

    bool DemoSheetRuntime::is_part_last_step(std::string_view part_name) const {
        const auto step = part_step(part_name);
        const auto total = part_total_step(part_name);
        return is_part_active(part_name) && step.has_value() && total.has_value() &&
               *step == *total - 1;
    }

    bool DemoSheetRuntime::contains_part(std::string_view part_name) const {
        return std::ranges::any_of(_time_rows, [part_name](const auto &row) {
                   return row.part_name == part_name;
               }) ||
               find_sub_part(part_name).has_value();
    }

    std::optional<std::int32_t> DemoSheetRuntime::part_step(
        std::string_view part_name) const {
        const auto *part = current_part();
        if (part != nullptr && part->part_name == part_name) {
            return _part_step;
        }
        const auto found = find_sub_part(part_name);
        if (!found.has_value()) {
            return std::nullopt;
        }
        return _sub_part_rows[*found].sub_part_total_step -
               _sub_part_remaining[*found];
    }

    std::optional<std::int32_t> DemoSheetRuntime::part_total_step(
        std::string_view part_name) const {
        const auto *part = current_part();
        if (part != nullptr && part->part_name == part_name) {
            return part->total_step;
        }
        const auto found = find_sub_part(part_name);
        return found.has_value() ? std::optional(_sub_part_rows[*found].sub_part_total_step) : std::nullopt;
    }

    bool DemoSheetRuntime::is_last_part() const {
        return _active && !_paused && _part_index.has_value() && *_part_index + 1U == _time_rows.size();
    }

    bool DemoSheetRuntime::is_demo_last_step() const {
        const auto *part = current_part();
        return is_last_part() && part != nullptr && _part_step == part->total_step - 1;
    }

    bool DemoSheetRuntime::is_final_boundary_overshoot() const {
        return _active && _part_index.has_value() &&
               *_part_index >= _time_rows.size();
    }

    const DemoTimeRow *DemoSheetRuntime::current_part() const {
        if (!_active || !_part_index.has_value() || _time_rows.empty()) {
            return nullptr;
        }
        return *_part_index < _time_rows.size() ? &_time_rows[*_part_index] :
                                                  &_time_rows.back();
    }

    std::optional<std::size_t> DemoSheetRuntime::current_part_index() const {
        return _active ? _part_index : std::nullopt;
    }

    std::optional<std::int32_t> DemoSheetRuntime::current_part_step() const {
        return _active ? std::optional(_part_step) : std::nullopt;
    }

    std::optional<std::int32_t> DemoSheetRuntime::current_part_total_step() const {
        const auto *part = current_part();
        return part == nullptr ? std::nullopt : std::optional(part->total_step);
    }

    std::optional<std::size_t> DemoSheetRuntime::find_sub_part(
        std::string_view part_name) const {
        const auto found = std::ranges::find(_sub_part_rows, part_name,
                                             &DemoSubPartRow::sub_part_name);
        return found != _sub_part_rows.end() ? std::optional(static_cast<std::size_t>(
                                                   std::distance(_sub_part_rows.begin(), found))) :
                                               std::nullopt;
    }

}  // namespace smgpc::compat
