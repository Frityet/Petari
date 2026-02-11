#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

#include "AssetServices.hpp"
#include "layout/Brfnt.hpp"
#include "layout/Brlan.hpp"
#include "layout/Brlyt.hpp"
#include "layout/Tpl.hpp"

namespace smgpc::logging {
class ILogger;
}

namespace smgpc::game::title {

struct TitleLayoutResource {
    assets::layout::LayoutDefinition layout {};
    std::unordered_map<std::string, assets::layout::BrlanAnimation> animations_by_name {};
    std::unordered_map<std::string, assets::layout::tpl::DecodedImage> textures_by_name {};
};

struct TitleAssets {
    TitleLayoutResource press_start {};
    TitleLayoutResource title_logo {};
    std::unordered_map<std::string, assets::layout::BrfntFont> fonts_by_name {};
};

[[nodiscard]] assets::AssetResult<TitleAssets> load_title_assets(
    assets::IAssetManager &asset_manager,
    logging::ILogger &logger);

}  // namespace smgpc::game::title
