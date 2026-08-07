#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace smgpc::resource {
    class RarcArchive;
}

namespace smgpc::compat {

    enum class DemoSheetTable : std::uint8_t {
        Time,
        SubPart,
        Player,
        Camera,
        Action,
        Wipe,
        Sound,
        Count,
    };

    struct DemoTimeRow {
        std::string part_name;
        std::int32_t total_step = 1;
        bool suspend = false;
    };

    struct DemoSubPartRow {
        std::string sub_part_name;
        std::int32_t sub_part_total_step = 1;
        std::string main_part_name;
        std::int32_t main_part_step = 1;
    };

    struct DemoPlayerRow {
        std::string part_name;
        std::string position_name;
        std::string bck_name;
    };

    struct DemoCameraRow {
        std::string part_name;
        std::string target_name;
        std::int32_t target_cast_id = -1;
        std::string animation_name;
        std::int32_t animation_start_frame = -1;
        std::int32_t animation_end_frame = -1;
        bool continuous = false;
    };

    struct DemoActionRow {
        std::string part_name;
        std::string cast_name;
        std::int32_t cast_id = -1;
        std::int32_t action_type = 0;
        std::string position_name;
        std::string animation_name;
    };

    struct DemoWipeRow {
        std::string part_name;
        std::string wipe_name = "フェードワイプ";
        std::int32_t wipe_type = 0;
        std::int32_t wipe_frame = -1;
    };

    struct DemoSoundRow {
        std::string part_name;
        std::string bgm;
        std::string system_sound;
        std::int32_t return_bgm = 0;
        std::int32_t bgm_wipeout_frame = -1;
    };

    class DemoSheetParseError final : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };

    enum class DemoSheetStartResult : std::uint8_t {
        Started,
        MissingTimeTable,
        EmptyTimeTable,
        PartNotFound,
    };

    [[nodiscard]] std::string_view demo_sheet_start_result_name(DemoSheetStartResult result);

    // Parsed DemoSheet data and its source-faithful main-part time keeper. Actor,
    // player, camera, wipe, and sound dispatch intentionally remain outside this
    // core so their Game-side keepers can consume the parsed rows later.
    class DemoSheetRuntime final {
    public:
        [[nodiscard]] static DemoSheetRuntime load(const smgpc::resource::RarcArchive &archive,
                                                   std::string_view time_sheet_name);

        [[nodiscard]] const std::string &time_sheet_name() const;
        [[nodiscard]] bool has_table(DemoSheetTable table) const;

        [[nodiscard]] std::span<const DemoTimeRow> time_rows() const;
        [[nodiscard]] std::span<const DemoSubPartRow> sub_part_rows() const;
        [[nodiscard]] std::span<const DemoPlayerRow> player_rows() const;
        [[nodiscard]] std::span<const DemoCameraRow> camera_rows() const;
        [[nodiscard]] std::span<const DemoActionRow> action_rows() const;
        [[nodiscard]] std::span<const DemoWipeRow> wipe_rows() const;
        [[nodiscard]] std::span<const DemoSoundRow> sound_rows() const;

        [[nodiscard]] DemoSheetStartResult start();
        [[nodiscard]] DemoSheetStartResult start_at_part(std::string_view part_name);
        void stop();
        void pause();
        void resume();

        // Advances to the next dispatchable frame. True means the current-part
        // queries describe a frame that callers should dispatch this tick.
        [[nodiscard]] bool advance();

        [[nodiscard]] bool is_active() const;
        [[nodiscard]] bool is_paused() const;
        [[nodiscard]] bool is_part_active(std::string_view part_name) const;
        [[nodiscard]] bool is_part_first_step(std::string_view part_name) const;
        [[nodiscard]] bool is_part_last_step(std::string_view part_name) const;
        [[nodiscard]] bool is_last_part() const;
        [[nodiscard]] bool is_demo_last_step() const;
        [[nodiscard]] const DemoTimeRow *current_part() const;
        [[nodiscard]] std::optional<std::size_t> current_part_index() const;
        [[nodiscard]] std::optional<std::int32_t> current_part_step() const;
        [[nodiscard]] std::optional<std::int32_t> current_part_total_step() const;

    private:
        explicit DemoSheetRuntime(std::string time_sheet_name);

        [[nodiscard]] DemoSheetStartResult start_at_index(std::size_t index);

        std::string _time_sheet_name;
        std::array<bool, static_cast<std::size_t>(DemoSheetTable::Count)> _present_tables{};
        std::vector<DemoTimeRow> _time_rows;
        std::vector<DemoSubPartRow> _sub_part_rows;
        std::vector<DemoPlayerRow> _player_rows;
        std::vector<DemoCameraRow> _camera_rows;
        std::vector<DemoActionRow> _action_rows;
        std::vector<DemoWipeRow> _wipe_rows;
        std::vector<DemoSoundRow> _sound_rows;
        std::optional<std::size_t> _part_index;
        std::int32_t _part_step = -1;
        bool _active = false;
        bool _paused = false;
    };

}  // namespace smgpc::compat
