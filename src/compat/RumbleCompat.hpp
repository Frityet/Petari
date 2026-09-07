#pragma once

namespace smgpc::runtime {
    class RumbleActuator;
}

namespace smgpc::compat {
    [[nodiscard]] smgpc::runtime::RumbleActuator& aurora_rumble_actuator();
}
