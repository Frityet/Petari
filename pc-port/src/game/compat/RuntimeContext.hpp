#pragma once

namespace smgpc::assets {
class IGameAssetService;
}

namespace smgpc::logging {
class ILogger;
}

namespace smgpc::render {
class IRendererService;
}

namespace smgpc::game::compat {

struct RuntimeContext {
    assets::IGameAssetService *asset_service {};
    render::IRendererService *renderer_service {};
    logging::ILogger *logger {};
    bool is_widescreen {true};
};

void set_runtime_context(RuntimeContext context);
[[nodiscard]] const RuntimeContext &runtime_context();

}  // namespace smgpc::game::compat
