#include "compat/RumbleCompat.hpp"

#include "runtime/RuntimeServices.hpp"

#include <revolution.h>

namespace smgpc::compat {
    namespace {
        class AuroraRumbleActuator final : public smgpc::runtime::RumbleActuator {
        public:
            [[nodiscard]] bool is_available(s32 channel) const noexcept override {
                return WPADSupportsRumble(channel) != FALSE;
            }

            void set_motor(s32 channel, bool enabled) noexcept override {
                WPADControlMotor(channel, enabled ? WPAD_MOTOR_RUMBLE : WPAD_MOTOR_STOP);
            }
        };
    }

    smgpc::runtime::RumbleActuator& aurora_rumble_actuator() {
        static auto actuator = AuroraRumbleActuator{};
        return actuator;
    }
}
