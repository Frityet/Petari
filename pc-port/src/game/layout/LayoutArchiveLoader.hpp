#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "AssetLoader.hpp"
#include "AssetServices.hpp"
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
        di::DependencyReference<const assets::AssetLoader> asset_loader,
        di::OptionalDependencyReference<logging::ILogger> logger = nullptr);

    [[nodiscard]] assets::AssetResult<std::shared_ptr<LayoutArchiveData>> load(const LayoutArchiveLoadRequest &request) const;
    [[nodiscard]] static std::vector<std::string> make_font_name_lookup_keys(std::string_view font_name);
    static void apply_language_pane_visibility(std::string_view language, assets::layout::LayoutDefinition *layout);

private:
    [[nodiscard]] assets::AssetResult<void> populate_layout_from_archive(
        const std::string &archive_path,
        const std::string &brlyt_path,
        LayoutArchiveData *output) const;

    [[nodiscard]] assets::AssetResult<void> populate_fonts_from_archive(
        const std::string &archive_path,
        LayoutArchiveData *output) const;

    [[nodiscard]] static assets::AssetError make_error(std::string message);
    [[nodiscard]] static std::string basename_without_extension(std::string_view path);
    [[nodiscard]] static std::string normalize_name(std::string name);

    di::DependencyReference<const assets::AssetLoader> _asset_loader;
    di::OptionalDependencyReference<logging::ILogger> _logger;
};

}  // namespace smgpc::game::layout
