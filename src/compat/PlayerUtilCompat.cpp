#include "compat/PlayerUtilCompat.hpp"
#include "runtime/RuntimeContext.hpp"

#include <utility>

namespace smgpc::compat {
    namespace {
        thread_local smgpc::runtime::PlayerSystemService *sPlayerSystemOverride = nullptr;
    }

    ScopedPlayerSystemServiceOverride::ScopedPlayerSystemServiceOverride(
        smgpc::runtime::PlayerSystemService &service)
        : _previous(std::exchange(sPlayerSystemOverride, &service)) {
    }

    ScopedPlayerSystemServiceOverride::~ScopedPlayerSystemServiceOverride() {
        sPlayerSystemOverride = _previous;
    }

    [[nodiscard]] smgpc::runtime::PlayerSystemService *active_player_system_for_player_util() {
        if (sPlayerSystemOverride != nullptr) {
            return sPlayerSystemOverride;
        }
        auto *runtime = smgpc::runtime::RuntimeContext::try_instance();
        return runtime != nullptr ? &runtime->player_system() : nullptr;
    }
}  // namespace smgpc::compat
