#pragma once

namespace smgpc::logging { class ILogger; }
namespace smgpc::app {
struct BootstrapConfiguration;
int run_original_game(const BootstrapConfiguration&, logging::ILogger&);
}
