#pragma once

#include <cstddef>
#include <cstdint>

class LiveActor;

namespace smgpc::compat {

    [[nodiscard]] std::size_t actor_sensor_binding_count(const LiveActor* actor);

    // Debug-only naming is a host concern. Unknown retail values remain absent
    // instead of being labeled with a fabricated name.
    [[nodiscard]] const char* actor_message_name(std::uint32_t message);

}  // namespace smgpc::compat
