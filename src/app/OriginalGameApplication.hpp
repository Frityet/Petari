#pragma once

#include <cstdint>

class GameSystem;

namespace smgpc::logging { class ILogger; }
namespace smgpc::app {
struct BootstrapConfiguration;
#ifndef NDEBUG
// Explicit test/diagnostic callbacks run synchronously at the completed frame
// boundary, while the actual process still owns guest execution and allocation.
// The caller retains context until run_original_game returns after retirement.
struct OriginalGameDebugObserver {
    void* context = nullptr;
    void (*after_frame)(void*, GameSystem&, std::uint64_t) = nullptr;
};
#endif
int run_original_game(const BootstrapConfiguration&, logging::ILogger&
#ifndef NDEBUG
                      , OriginalGameDebugObserver = {}
#endif
);
}
