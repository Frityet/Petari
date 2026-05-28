#include "runtime/WiiPlatformService.hpp"

#include "runtime/RuntimeContext.hpp"

#include <array>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <string>
#include <string_view>

namespace smgpc::runtime {
    namespace {

        constexpr auto UNIX_SECONDS_TO_WII_EPOCH = s64{946684800};
        constexpr auto SECONDS_PER_DAY = s64{86400};
        constexpr auto WII_CALENDAR_DAY_OFFSET = s64{0xB2575};

        [[nodiscard]] bool is_leap_year(s32 year) {
            return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
        }

        [[nodiscard]] s32 leap_days(s32 year) {
            if (year < 1) {
                return 0;
            }

            return (year + 3) / 4 - (year - 1) / 100 + (year - 1) / 400;
        }

        [[nodiscard]] std::optional<s64> read_s64_environment(std::string_view name) {
            const auto key = std::string(name);
            const auto *value = std::getenv(key.c_str());
            if (value == nullptr || value[0] == '\0') {
                return std::nullopt;
            }

            auto parsed = s64{};
            const auto text = std::string_view(value);
            const auto *begin = text.data();
            const auto *end = begin + text.size();
            const auto result = std::from_chars(begin, end, parsed);
            if (result.ec != std::errc{} || result.ptr != end) {
                return std::nullopt;
            }
            return parsed;
        }

        [[nodiscard]] std::optional<OSTime> read_time_override_environment() {
            if (const auto ticks = read_s64_environment("SMGPC_OS_TIME_TICKS")) {
                return *ticks;
            }
            if (const auto unix_seconds = read_s64_environment("SMGPC_OS_TIME_UNIX_SECONDS")) {
                return (*unix_seconds - UNIX_SECONDS_TO_WII_EPOCH) * kWiiOsTimerClock;
            }
            return std::nullopt;
        }

        void fill_calendar_date(s32 days, OSCalendarTime &time) {
            static constexpr auto year_days = std::array{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
            static constexpr auto leap_year_days = std::array{0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335};

            time.wday = (days + 6) % 7;

            auto year = days / 365;
            while (days < leap_days(year) + 365 * year) {
                --year;
            }

            days -= leap_days(year) + 365 * year;
            time.year = year;
            time.yday = days;

            const auto &month_days = is_leap_year(year) ? leap_year_days : year_days;
            auto month = 12;
            while (days < month_days[--month]) {
            }

            time.mon = month;
            time.mday = days - month_days[month] + 1;
        }

    }  // namespace

    WiiPlatformService::WiiPlatformService() : _time_override_ticks(read_time_override_environment()) {
    }

    void WiiPlatformService::begin_frame(std::uint64_t frame_index) {
        _frame_index = frame_index;
    }

    void WiiPlatformService::set_time_override_ticks(std::optional<OSTime> ticks) {
        _time_override_ticks = ticks;
    }

    OSTime WiiPlatformService::time_ticks() const {
        if (_time_override_ticks.has_value()) {
            return *_time_override_ticks + static_cast<OSTime>(_frame_index) * kWiiOsTimerClock / 60;
        }

        return host_time_ticks();
    }

    u32 WiiPlatformService::tick() const {
        return static_cast<u32>(time_ticks());
    }

    s64 WiiPlatformService::ticks_to_seconds(OSTime ticks) const {
        return ticks / kWiiOsTimerClock;
    }

    s64 WiiPlatformService::ticks_to_milliseconds(OSTime ticks) const {
        return ticks / (kWiiOsTimerClock / 1000);
    }

    s64 WiiPlatformService::ticks_to_microseconds(OSTime ticks) const {
        return (ticks * 8) / (kWiiOsTimerClock / 125000);
    }

    OSTime WiiPlatformService::seconds_to_ticks(s64 seconds) const {
        return seconds * kWiiOsTimerClock;
    }

    void WiiPlatformService::ticks_to_calendar_time(OSTime ticks, OSCalendarTime &time) const {
        auto ticks_after_second = ticks % kWiiOsTimerClock;
        if (ticks_after_second < 0) {
            ticks_after_second += kWiiOsTimerClock;
        }

        time.usec = static_cast<s32>(ticks_to_microseconds(ticks_after_second) % 1000);
        time.msec = static_cast<s32>(ticks_to_milliseconds(ticks_after_second) % 1000);

        ticks -= ticks_after_second;
        auto days = static_cast<s32>(ticks_to_seconds(ticks) / SECONDS_PER_DAY + WII_CALENDAR_DAY_OFFSET);
        auto seconds = static_cast<s32>(ticks_to_seconds(ticks) % SECONDS_PER_DAY);
        if (seconds < 0) {
            --days;
            seconds += static_cast<s32>(SECONDS_PER_DAY);
        }

        fill_calendar_date(days, time);
        time.hour = seconds / 60 / 60;
        time.min = (seconds / 60) % 60;
        time.sec = seconds % 60;
    }

    OSTime WiiPlatformService::host_time_ticks() const {
        const auto now = std::chrono::system_clock::now().time_since_epoch();
        const auto unix_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(now).count();
        const auto wii_microseconds = unix_microseconds - UNIX_SECONDS_TO_WII_EPOCH * s64{1000000};
        return (wii_microseconds * kWiiOsTimerClock) / s64{1000000};
    }

}  // namespace smgpc::runtime

OSTime OSGetTime() {
    if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance()) {
        return runtime->wii_platform().time_ticks();
    }

    return smgpc::runtime::WiiPlatformService().time_ticks();
}

u32 OSGetTick() {
    if (auto *runtime = smgpc::runtime::RuntimeContext::try_instance()) {
        return runtime->wii_platform().tick();
    }

    return smgpc::runtime::WiiPlatformService().tick();
}

s64 OSTicksToSeconds(OSTime ticks) {
    return smgpc::runtime::WiiPlatformService().ticks_to_seconds(ticks);
}

s64 OSTicksToMilliseconds(OSTime ticks) {
    return smgpc::runtime::WiiPlatformService().ticks_to_milliseconds(ticks);
}

s64 OSTicksToMicroseconds(OSTime ticks) {
    return smgpc::runtime::WiiPlatformService().ticks_to_microseconds(ticks);
}

OSTime OSSecondsToTicks(s64 seconds) {
    return smgpc::runtime::WiiPlatformService().seconds_to_ticks(seconds);
}

void OSTicksToCalendarTime(OSTime ticks, OSCalendarTime *pTime) {
    if (pTime == nullptr) {
        return;
    }

    smgpc::runtime::WiiPlatformService().ticks_to_calendar_time(ticks, *pTime);
}
