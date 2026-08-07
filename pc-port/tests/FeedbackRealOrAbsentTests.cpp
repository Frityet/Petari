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
        require(!service.try_request_pattern(&source, "最強", 1),
                "a retail pattern must fail honestly when its channel has no actuator");
        require(service.events().empty() && actuator.calls.empty(),
                "rejected rumble requests must not fabricate actuator calls or success events");

        require(service.try_request_pattern(&source, "最強", 0),
                "an exact retail pattern should start when a real actuator is available");
        require(!service.try_request_pattern(&source, "最強", 0),
                "the same source and active pattern must retain retail duplicate suppression");
        require(service.events().size() == 1U && service.events().front().pattern_name == "最強" &&
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

    void test_camera_shake_is_exact_projection_motion() {
        auto camera = smgpc::runtime::CameraSystemService{};
        camera.set_game_camera_pose(smgpc::camera::CameraPose{});
        require_throws<std::logic_error>([&] { camera.request_normal_shake(); },
                                         "camera shake without proven retail projection dimensions must be absent");
        require_throws<std::invalid_argument>([&] { camera.set_shake_projection_dimensions(0.0F, 456.0F); },
                                               "invalid projection dimensions must be rejected");

        camera.set_shake_projection_dimensions(608.0F, 456.0F);
        camera.begin_frame(20U);
        camera.request_normal_shake();
        camera.request_normal_shake();
        require(camera.shake_request_events().size() == 1U,
                "an already-running retail shake power must not restart or add a fake request");

        camera.begin_frame(21U);
        const auto pose = camera.effective_camera_pose();
        const auto remaining = 24.0F;
        const auto raw_offset = std::sin(12.566371F * remaining / 25.0F) *
                                std::sin(1.5707964F * remaining / 25.0F);
        const auto expected_y = raw_offset * 30.0F / 456.0F;
        require(pose.has_value() && std::abs(pose->projection_offset_x) < 0.000001F &&
                    std::abs(pose->projection_offset_y - expected_y) < 0.000001F,
                "normal shake must use the decompiled 25-frame sine and exact 30/EFB-height scaling");

        for (auto frame = std::uint64_t{22U}; frame <= 45U; ++frame) {
            camera.begin_frame(frame);
        }
        const auto ended = camera.effective_camera_pose();
        require(ended.has_value() && ended->projection_offset_x == 0.0F && ended->projection_offset_y == 0.0F,
                "the retail singly-vertical shake must end exactly after 25 updates");
    }

    void test_effect_deletion_requires_a_real_keeper() {
        auto effects = smgpc::runtime::EffectService{};
        auto host = 0;
        require_throws<std::logic_error>([&] { effects.delete_all("Host", &host); },
                                         "effect deletion without a keeper must be explicitly absent");
        require(effects.events().empty(), "an absent effect keeper must not produce a successful delete event");

        effects.register_keeper(smgpc::runtime::EffectKeeperHostKind::LiveActor, "Host", 1, "Host", false, &host);
        effects.delete_all("Host", &host);
        require(effects.events().size() == 1U && effects.events().front().keeper.has_value(),
                "delete-all should execute only against the real registered keeper");

        require_throws<std::invalid_argument>([] { MR::deleteEffectAll(static_cast<LiveActor*>(nullptr)); },
                                               "the required Game delete API must reject a null actor");
        auto actor = LiveActor{"NoRuntimeEffectHost"};
        require_throws<std::logic_error>([&] { MR::deleteEffectAll(&actor); },
                                         "the required Game delete API must reject an absent runtime");
    }

    void test_game_feedback_boundary_reports_absence() {
        auto source = 0;
        require(!MR::tryRumblePad(&source, "最強", WPAD_CHAN0) &&
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
            std::pair{"camera shake changes projection exactly", &test_camera_shake_is_exact_projection_motion},
            std::pair{"effect deletion requires keeper", &test_effect_deletion_requires_a_real_keeper},
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
