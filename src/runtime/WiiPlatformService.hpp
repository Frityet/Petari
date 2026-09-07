#pragma once

#include <cstdint>
#include <optional>

#include <revolution.h>

namespace smgpc::runtime {

    constexpr auto kWiiOsTimerClock = s64{60750000};

    class WiiPlatformService final {
    public:
        WiiPlatformService();

        void begin_frame(std::uint64_t frame_index);
        void set_time_override_ticks(std::optional<OSTime> ticks);

        [[nodiscard]] OSTime time_ticks() const;
        [[nodiscard]] u32 tick() const;
        [[nodiscard]] s64 ticks_to_seconds(OSTime ticks) const;
        [[nodiscard]] s64 ticks_to_milliseconds(OSTime ticks) const;
        [[nodiscard]] s64 ticks_to_microseconds(OSTime ticks) const;
        [[nodiscard]] OSTime seconds_to_ticks(s64 seconds) const;
        void ticks_to_calendar_time(OSTime ticks, OSCalendarTime &time) const;

    private:
        [[nodiscard]] OSTime host_time_ticks() const;

        std::optional<OSTime> _time_override_ticks;
        std::uint64_t _frame_index = 0U;
    };

}  // namespace smgpc::runtime
