#include "render/core/FixedStepClock.hpp"

#include "JSystem/J3DGraphAnimator/J3DAnimation.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

    using smgpc::render::core::FixedStepClock;
    using namespace std::chrono_literals;

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void presentation_rate_does_not_change_animation_rate() {
        for (const auto presentations : std::array{18, 30, 60, 120, 144}) {
            FixedStepClock clock;
            J3DFrameCtrl animation(1000);
            animation.setRate(1.25F);
            auto previous = 0ns;
            auto ticks = std::uint64_t{};
            bool saw_multiple_ticks = false;
            bool saw_zero_ticks = false;
            for (auto frame = 1; frame <= presentations; ++frame) {
                // Integer presentation timestamps cover exactly one second,
                // including nonintegral nanosecond refresh periods.
                const auto now = std::chrono::nanoseconds{1'000'000'000LL * frame / presentations};
                const auto due = clock.advance(now - previous);
                previous = now;
                saw_multiple_ticks |= due > 1U;
                saw_zero_ticks |= due == 0U;
                ticks += due;
                for (auto tick = std::uint64_t{}; tick < due; ++tick) {
                    animation.update();
                }
            }
            require(ticks == 60U, "Each presentation schedule must execute 60 original updates per elapsed second");
            require(animation.getFrame() == 75.0F && animation.getRate() == 1.25F,
                    "Original J3DFrameCtrl must preserve the authored rate across presentation frequencies");
            require(presentations >= 60 || saw_multiple_ticks, "Slow presentation must retain all elapsed game ticks");
            require(presentations <= 60 || saw_zero_ticks, "Fast presentation must not create extra game updates");
        }
    }

    void fractional_ticks_do_not_drift() {
        FixedStepClock clock;
        require(clock.advance(16'666'666ns) == 0U, "A rounded-down 1/60 duration is not yet a complete tick");
        require(clock.advance(1ns) == 1U, "The following nanosecond must cross the first tick boundary");
        require(clock.advance(16'666'666ns) == 0U, "The fractional remainder must not round the second tick up");
        require(clock.advance(1ns) == 1U, "The second exact tick boundary must be retained");
        require(clock.advance(16'666'666ns) == 1U, "Exactly 50 milliseconds must produce three ticks");

        clock.reset();
        auto ticks = std::uint64_t{};
        for (auto sample = 0; sample < 10'000; ++sample) {
            ticks += clock.advance(100us);
        }
        require(ticks == 60U && clock.advance(0ns) == 0U,
                "Many small elapsed intervals must add to exactly one second without drift");
    }

    void stalled_presentations_retain_all_ticks() {
        FixedStepClock clock;
        require(clock.advance(5ms) == 0U, "Short elapsed intervals must remain pending");
        require(clock.advance(2s) == 120U, "A delayed presentation must not discard original game updates");
        require(clock.advance(45ms) == 3U, "The pre-stall fractional tick must survive the catch-up interval");
    }

    void pause_and_reset_have_distinct_time_ownership() {
        FixedStepClock clock;
        require(clock.advance(10ms) == 0U, "Initial partial tick must remain pending");
        for (auto paused_present = 0; paused_present < 120; ++paused_present) {
            require(clock.advance(0ns) == 0U, "A paused game clock must not advance simulation");
        }
        require(clock.advance(10ms) == 1U, "Resuming must preserve time elapsed before the pause");
        clock.reset();
        require(clock.advance(15ms) == 0U, "An explicit new epoch must discard the prior fractional tick");
        require(clock.advance(2ms) == 1U, "A new epoch must resume normal tick accounting");
    }

    void long_elapsed_intervals_do_not_overflow() {
        FixedStepClock clock;
        require(clock.advance(std::chrono::nanoseconds::max()) == 553'402'322'211ULL,
                "The full supported duration range must not overflow when multiplied by 60");
        clock.reset();
        require(clock.advance(24h) == 5'184'000U, "Long intervals must retain the exact integer tick count");
    }

    void negative_elapsed_time_is_rejected_without_mutation() {
        FixedStepClock clock;
        require(clock.advance(10ms) == 0U, "Initial partial tick must remain pending");
        bool rejected = false;
        try {
            static_cast<void>(clock.advance(-1ns));
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected, "A backwards elapsed interval must be rejected explicitly");
        require(clock.advance(10ms) == 1U, "A rejected interval must leave the accumulated time intact");
    }

}  // namespace

int main() {
    try {
        presentation_rate_does_not_change_animation_rate();
        fractional_ticks_do_not_drift();
        stalled_presentations_retain_all_ticks();
        pause_and_reset_have_distinct_time_ownership();
        long_elapsed_intervals_do_not_overflow();
        negative_elapsed_time_is_rejected_without_mutation();
        std::cout << "6/6 fixed-step clock and original animation-rate groups passed\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "Fixed-step clock regression failed: " << exception.what() << '\n';
        return 1;
    }
}
