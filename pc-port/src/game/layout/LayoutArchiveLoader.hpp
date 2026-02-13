#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "AssetServices.hpp"
#include "ServiceProvider.hpp"
#include "GameAssetService.hpp"
#include "layout/Brfnt.hpp"
#include "layout/Brlan.hpp"
#include "layout/Brlyt.hpp"
#include "layout/Tpl.hpp"

namespace smgpc::logging {
class ILogger;
}

namespace smgpc::game::layout {

struct LayoutArchiveData {
    assets::layout::LayoutDefinition layout {};
    std::unordered_map<std::string, assets::layout::BrlanAnimation> animations_by_name {};
    std::unordered_map<std::string, assets::layout::tpl::DecodedImage> textures_by_name {};
    std::unordered_map<std::string, assets::layout::BrfntFont> fonts_by_name {};
};

struct LayoutArchiveLoadRequest {
    std::string archive_path {};
    std::string brlyt_path {};
    bool include_shared_fonts {true};
    std::vector<std::string> shared_font_archives {
        "/LayoutData/Font.arc",
        "/LayoutData/MiiFont.arc",
    };
};

class LayoutArchiveLoader {
public:
    LayoutArchiveLoader(
        di::DependencyReference<assets::IGameAssetService> asset_service,
        logging::ILogger *logger);

    [[nodiscard]] assets::AssetResult<std::shared_ptr<LayoutArchiveData>> load(const LayoutArchiveLoadRequest &request) const;

private:
    [[nodiscard]] assets::AssetResult<void> populate_layout_from_archive(
        const assets::layout::RarcArchive &archive,
        const std::string &brlyt_path,
        LayoutArchiveData *output) const;

    [[nodiscard]] assets::AssetResult<void> populate_fonts_from_archive(
        const assets::layout::RarcArchive &archive,
        LayoutArchiveData *output) const;

    [[nodiscard]] static assets::AssetError make_error(std::string message);
    [[nodiscard]] static std::string basename_without_extension(std::string_view path);
    [[nodiscard]] static std::string normalize_name(std::string name);
    [[nodiscard]] static bool has_extension(std::string_view path, std::string_view extension);

    di::DependencyReference<assets::IGameAssetService> _asset_service;
    logging::ILogger *_logger {};
};

}  // namespace smgpc::game::layout
