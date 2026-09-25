#include "resource/TextEncoding.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include "Game/Util/EffectUtil.hpp"
#include "Game/Util/ObjUtil.hpp"
#include "runtime/RuntimeServices.hpp"

#include <array>
#include <cmath>
#include <exception>
#include <iostream>
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

    template <typename Exception, typename Fn>
    void require_throws(Fn&& fn, std::string_view message) {
        auto rejected = false;
        try {
            fn();
        } catch (const Exception&) {
            rejected = true;
        }
        require(rejected, message);
    }

    class RecordingRumbleActuator final : public smgpc::runtime::RumbleActuator {
    public:
        struct Call {
            s32 channel = 0;
            bool enabled = false;
        };

        [[nodiscard]] bool is_available(s32 channel) const noexcept override {
            return channel >= 0 && channel < static_cast<s32>(available.size()) && available[static_cast<std::size_t>(channel)];
        }

        void set_motor(s32 channel, bool enabled) noexcept override {
            calls.push_back(Call{.channel = channel, .enabled = enabled});
        }

        std::array<bool, WPAD_MAX_CONTROLLERS> available = {};
        std::vector<Call> calls;
    };

    void test_rumble_uses_exact_named_pattern_and_real_actuator() {
        auto actuator = RecordingRumbleActuator{};
        actuator.available[0U] = true;
        auto service = smgpc::runtime::RumbleService{&actuator};
        service.begin_frame(7U);

        auto source = 1;
        require(!service.try_request_pattern(&source, "not-a-retail-pattern", 0),
                "an unknown pattern must remain absent instead of becoming a strong rumble");
        require(!service.try_request_pattern(&source, smgpc::resource::encode_cp932("最強"), 1),
                "a retail pattern must fail honestly when its channel has no actuator");
        require(service.events().empty() && actuator.calls.empty(),
                "rejected rumble requests must not fabricate actuator calls or success events");

        require(service.try_request_pattern(&source, smgpc::resource::encode_cp932("最強"), 0),
                "an exact retail pattern should start when a real actuator is available");
        require(!service.try_request_pattern(&source, smgpc::resource::encode_cp932("最強"), 0),
                "the same source and active pattern must retain retail duplicate suppression");
        require(service.events().size() == 1U && service.events().front().pattern_name == smgpc::resource::encode_cp932("最強") &&
                    service.events().front().frame_index == 7U,
                "only an accepted physical-rumble request should be traced");
        require(actuator.calls.size() == 1U && actuator.calls.front().enabled,
                "the first sample of the exact strong pattern must drive the physical motor");

        for (auto frame = std::uint64_t{8U}; frame <= 40U; ++frame) {
            service.begin_frame(frame);
        }
        require(!actuator.calls.empty() && !actuator.calls.back().enabled,
                "the exact finite pattern must turn the physical motor back off");
    }

    void test_game_feedback_boundary_reports_absence() {
        auto source = 0;
        require(!MR::tryRumblePad(&source, smgpc::resource::encode_cp932("最強").c_str(), WPAD_CHAN0) &&
                    !MR::tryRumblePad(&source, "unknown", WPAD_CHAN0),
                "Game try-rumble APIs must return false without an active host runtime");
        require_throws<std::logic_error>([] { MR::shakeCameraNormal(); },
                                         "the required Game camera API must reject an absent host runtime");
    }
}

int main() {
    try {
        const auto tests = std::array{
            std::pair{"exact rumble pattern drives real actuator", &test_rumble_uses_exact_named_pattern_and_real_actuator},
            std::pair{"Game feedback boundary reports absence", &test_game_feedback_boundary_reports_absence},
        };
        for (const auto& [name, test] : tests) {
            test();
            std::cout << "[ok] " << name << '\n';
        }
        std::cout << tests.size() << " feedback real-or-absent test(s) passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[failed] " << error.what() << '\n';
        return 1;
    }
}
