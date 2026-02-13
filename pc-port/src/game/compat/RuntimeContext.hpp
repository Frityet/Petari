#pragma once

#include "RendererService.hpp"
#include "ServiceProvider.hpp"

namespace smgpc::assets {
class IGameAssetService;
}

namespace smgpc::logging {
class ILogger;
}

namespace smgpc::game::compat {

struct RuntimeContext {
    di::OptionalDependencyReference<assets::IGameAssetService> asset_service {};
    di::OptionalDependencyReference<render::IRendererEngine> renderer_engine {};
    di::OptionalDependencyReference<render::IInputService> input_service {};
    di::OptionalDependencyReference<logging::ILogger> logger {};
    bool is_widescreen {true};
};

void set_runtime_context(RuntimeContext context);
[[nodiscard]] const RuntimeContext &runtime_context();

}  // namespace smgpc::game::compat
