#pragma once

#include "render/core/FixedStepClock.hpp"
#include "render/core/RenderTypes.hpp"

#include <aurora/time.hpp>

namespace smgpc::app {

    // Keep original per-frame game work independent of GPU presentation. Each
    // session begins with one initialization tick; subsequent ticks follow
    // Aurora's clock, including its pause and time-scale behavior.
    class SimulationClock final {
    public:
        explicit SimulationClock(std::uint64_t initial_frame = 0U)
            : _last_time(aurora::time::game_clock::now()), _frame_index(initial_frame) {
        }

        [[nodiscard]] bool poll() {
            const auto now = aurora::time::game_clock::now();
            _pending_ticks += _clock.advance(now - _last_time);
            _last_time = now;
            return _pending_ticks != 0U;
        }

        // Unconsumed ticks stay queued when the caller needs to present an
        // exact screenshot, trace, transition, or final frame.
        [[nodiscard]] bool advance_frame(render::core::FrameContext& context) {
            if (_pending_ticks == 0U) {
                return false;
            }
            --_pending_ticks;
            ++_frame_index;
            constexpr auto step_seconds = 1.0 / render::core::FixedStepClock::ticks_per_second;
            context.frame_index = _frame_index;
            context.frame_time_seconds = static_cast<double>(_frame_index - 1U) * step_seconds;
            context.frame_delta_seconds = step_seconds;
            return true;
        }

    private:
        render::core::FixedStepClock _clock;
        aurora::time::game_clock::time_point _last_time;
        std::uint64_t _frame_index = 0U;
        std::uint64_t _pending_ticks = 1U;
    };

}  // namespace smgpc::app
