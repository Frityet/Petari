#pragma once

#include <chrono>
#include <cstdint>
#include <stdexcept>

namespace smgpc::render::core {

    // Converts elapsed game-clock time to complete original game updates.
    // The caller executes every returned tick with the original 1/60 delta;
    // presentation frequency does not change animation or gameplay rates.
    class FixedStepClock final {
    public:
        static constexpr std::uint64_t ticks_per_second = 60U;

        [[nodiscard]] std::uint64_t advance(std::chrono::nanoseconds elapsed) {
            if (elapsed.count() < 0) {
                throw std::invalid_argument("Simulation elapsed time cannot be negative");
            }

            constexpr std::uint64_t nanoseconds_per_second = 1'000'000'000U;
            const auto nanoseconds = static_cast<std::uint64_t>(elapsed.count());
            const auto seconds = nanoseconds / nanoseconds_per_second;
            // Split whole seconds before scaling so even nanoseconds::max()
            // cannot overflow. Preserve sub-tick time without rounding 1/60
            // second down to an integer nanosecond duration.
            const auto fraction = (nanoseconds % nanoseconds_per_second) * ticks_per_second + _fraction;
            _fraction = fraction % nanoseconds_per_second;
            return seconds * ticks_per_second + fraction / nanoseconds_per_second;
        }

        // Scene changes can explicitly start a new elapsed-time epoch. Pausing
        // an existing epoch belongs to the caller's game clock and needs no
        // reset here; advance(0ns) retains the partial tick.
        void reset() noexcept {
            _fraction = 0U;
        }

    private:
        std::uint64_t _fraction = 0U;
    };

}  // namespace smgpc::render::core
