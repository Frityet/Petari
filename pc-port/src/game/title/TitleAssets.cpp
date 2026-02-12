#include "TitleAssets.hpp"

#include <array>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "layout/Binary.hpp"
#include "Logger.hpp"
#include "PackedAsset.hpp"
#include "layout/RarcArchive.hpp"
#include "layout/Yaz0.hpp"

namespace smgpc::game::title {
namespace {

[[nodiscard]] assets::AssetError make_error(std::string message) {
    return assets::AssetError {
        .code = assets::AssetErrorCode::InvalidFormat,
        .message = std::move(message),
    };
}

[[nodiscard]] std::string to_lower_ascii(std::string text) {
    return assets::layout::binary::to_lower_ascii(std::move(text));
}

[[nodiscard]] std::string basename_without_extension(std::string_view path) {
    auto text = std::string(path);
    const auto slash = text.find_last_of("/\\");
    if (slash != std::string::npos) {
        text = text.substr(slash + 1U);
    }

    const auto dot = text.find_last_of('.');
    if (dot != std::string::npos) {
        text = text.substr(0U, dot);
    }

    return text;
}

[[nodiscard]] const assets::layout::BrfntFont *resolve_pane_font(const TitleLayoutResource &resource, const assets::layout::PaneDefinition &pane, const std::unordered_map<std::string, assets::layout::BrfntFont> &fonts_by_name) {
    if (pane.font_index < 0 || static_cast<std::size_t>(pane.font_index) >= resource.layout.font_names.size()) {
        return nullptr;
    }

    const auto font_name_key = to_lower_ascii(basename_without_extension(resource.layout.font_names[static_cast<std::size_t>(pane.font_index)]));
    const auto found_font = fonts_by_name.find(font_name_key);
    if (found_font == fonts_by_name.end()) {
        return nullptr;
    }

    return &found_font->second;
}

[[nodiscard]] std::pair<char16_t, char16_t> select_button_symbols(const assets::layout::BrfntFont *font) {
    if (font == nullptr) {
        return {u'A', u'B'};
    }

    constexpr std::array<std::pair<std::uint16_t, std::uint16_t>, 4> SYMBOL_CANDIDATES {{
        {0xE000U, 0xE001U},
        {0x24B6U, 0x24B7U},
        {0x2460U, 0x2461U},
        {static_cast<std::uint16_t>(u'A'), static_cast<std::uint16_t>(u'B')},
    }};

    for (const auto [a_codepoint, b_codepoint] : SYMBOL_CANDIDATES) {
        if (font->has_codepoint(a_codepoint) && font->has_codepoint(b_codepoint)) {
            return {static_cast<char16_t>(a_codepoint), static_cast<char16_t>(b_codepoint)};
        }
    }

    return {u'A', u'B'};
}

[[nodiscard]] std::u16string make_korean_press_start_text(const assets::layout::BrfntFont *font) {
    const auto [a_button, b_button] = select_button_symbols(font);
    std::u16string text {};
    text.reserve(32U);
    text.push_back(a_button);
    text.append(u" \uc640 ");
    text.push_back(b_button);
    text.append(u" \ub97c \ub20c\ub7ec \uc8fc\uc138\uc694");
    return text;
}

[[nodiscard]] bool is_korean_title_variant(const TitleAssets &title_assets) {
    for (const auto &[texture_name, _] : title_assets.title_logo.textures_by_name) {
        if (texture_name.find("kor") != std::string::npos) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool is_press_start_text_pane(const assets::layout::PaneDefinition &pane) {
    if (pane.type != assets::layout::PaneType::Text) {
        return false;
    }

    const auto pane_name = to_lower_ascii(pane.name);
    if (pane_name == "txtstart" || pane_name == "shastart") {
        return true;
    }

    if (pane.text.find(u"\u3092\u304a\u3057") != std::u16string::npos) {  // "をおし"
        return true;
    }

    return false;
}

void localize_press_start_for_korean(TitleAssets *title_assets, logging::ILogger &logger) {
    if (title_assets == nullptr) {
        return;
    }

    std::size_t localized_pane_count = 0U;
    for (auto &pane : title_assets->press_start.layout.panes) {
        if (not is_press_start_text_pane(pane)) {
            continue;
        }

        const auto *font = resolve_pane_font(title_assets->press_start, pane, title_assets->fonts_by_name);
        pane.text = make_korean_press_start_text(font);
        ++localized_pane_count;
    }

    if (localized_pane_count > 0U) {
        logger.info(__FILE__, __LINE__, logging::Category::GAME, "Localized PressStart text panes for Korean: {}", localized_pane_count);
    }
}

[[nodiscard]] assets::AssetResult<assets::layout::RarcArchive> load_archive(
    assets::IAssetManager &asset_manager,
    const assets::AssetId &id,
    logging::ILogger &logger) {
    const auto cached = asset_manager.load_cached_asset(id);
    if (not cached) {
        return cached.failure();
    }

    auto unpacked = assets::unpack_packed_asset(*cached);
    if (not unpacked) {
        logger.error(__FILE__, __LINE__, logging::Category::GAME, "Failed to unpack {}: {}", id.logical_path, unpacked.failure().message);
        return unpacked.failure();
    }

    std::vector<std::byte> archive_bytes = std::move(*unpacked);
    if (assets::layout::is_yaz0(archive_bytes)) {
        auto decoded = assets::layout::decode_yaz0(archive_bytes);
        if (not decoded) {
            logger.error(__FILE__, __LINE__, logging::Category::GAME, "Failed to decode Yaz0 in {}: {}", id.logical_path, decoded.failure().message);
            return decoded.failure();
        }
        archive_bytes = std::move(*decoded);
    }

    auto archive = assets::layout::RarcArchive::parse(std::move(archive_bytes));
    if (not archive) {
        logger.error(__FILE__, __LINE__, logging::Category::GAME, "Failed to parse RARC in {}: {}", id.logical_path, archive.failure().message);
        return archive.failure();
    }

    return archive;
}

[[nodiscard]] assets::AssetResult<void> populate_layout_resource(
    const assets::layout::RarcArchive &archive,
    std::string_view brlyt_path,
    TitleLayoutResource *resource,
    logging::ILogger &logger) {
    if (resource == nullptr) {
        return make_error("Layout resource output pointer is null.");
    }

    const auto brlyt_bytes = archive.find_entry(brlyt_path);
    if (brlyt_bytes.empty()) {
        return make_error("Required BRLYT entry was not found in archive.");
    }

    auto parsed_layout = assets::layout::parse_brlyt(brlyt_bytes);
    if (not parsed_layout) {
        return parsed_layout.failure();
    }
    resource->layout = std::move(*parsed_layout);

    for (const auto &entry : archive.entries()) {
        const auto normalized_path = to_lower_ascii(entry.path);
        const auto bytes = archive.find_entry(entry.path);
        if (bytes.empty()) {
            continue;
        }

        if (normalized_path.ends_with(".brlan")) {
            auto animation = assets::layout::parse_brlan(bytes, basename_without_extension(normalized_path));
            if (not animation) {
                logger.warning(__FILE__, __LINE__, logging::Category::GAME, "Skipping BRLAN {}: {}", entry.path, animation.failure().message);
                continue;
            }

            const auto key = to_lower_ascii(animation->name);
            resource->animations_by_name[key] = std::move(*animation);
            continue;
        }

        if (normalized_path.ends_with(".tpl")) {
            auto decoded_image = assets::layout::tpl::decode_tpl_first_image(bytes);
            if (not decoded_image) {
                logger.warning(__FILE__, __LINE__, logging::Category::GAME, "Skipping TPL {}: {}", entry.path, decoded_image.failure().message);
                continue;
            }

            const auto texture_key = to_lower_ascii(basename_without_extension(entry.path));
            resource->textures_by_name[texture_key] = std::move(*decoded_image);
            continue;
        }
    }

    return {};
}

[[nodiscard]] assets::AssetResult<void> populate_font_resource(
    const assets::layout::RarcArchive &archive,
    std::unordered_map<std::string, assets::layout::BrfntFont> *fonts,
    logging::ILogger &logger) {
    if (fonts == nullptr) {
        return make_error("Font output pointer is null.");
    }

    for (const auto &entry : archive.entries()) {
        const auto normalized_path = to_lower_ascii(entry.path);
        if (not normalized_path.ends_with(".brfnt")) {
            continue;
        }

        const auto bytes = archive.find_entry(entry.path);
        if (bytes.empty()) {
            continue;
        }

        auto font = assets::layout::parse_brfnt(bytes, basename_without_extension(entry.path));
        if (not font) {
            logger.warning(__FILE__, __LINE__, logging::Category::GAME, "Skipping BRFNT {}: {}", entry.path, font.failure().message);
            continue;
        }

        fonts->emplace(to_lower_ascii(font->name()), std::move(*font));
    }

    if (fonts->empty()) {
        return make_error("No BRFNT files were loaded from Font.arc.");
    }

    return {};
}

}  // namespace

assets::AssetResult<TitleAssets> load_title_assets(
    assets::IAssetManager &asset_manager,
    logging::ILogger &logger) {
    const assets::AssetId press_start_id {.logical_path = "LayoutData/PressStart.arc"};
    const assets::AssetId title_logo_id {.logical_path = "LayoutData/TitleLogo.arc"};
    const assets::AssetId font_id {.logical_path = "LayoutData/Font.arc"};

    auto press_start_archive = load_archive(asset_manager, press_start_id, logger);
    if (not press_start_archive) {
        return press_start_archive.failure();
    }

    auto title_logo_archive = load_archive(asset_manager, title_logo_id, logger);
    if (not title_logo_archive) {
        return title_logo_archive.failure();
    }

    auto font_archive = load_archive(asset_manager, font_id, logger);
    if (not font_archive) {
        return font_archive.failure();
    }

    TitleAssets title_assets {};

    const auto press_layout_result = populate_layout_resource(*press_start_archive, "blyt/pressstart.brlyt", &title_assets.press_start, logger);
    if (not press_layout_result) {
        return press_layout_result.failure();
    }

    const auto logo_layout_result = populate_layout_resource(*title_logo_archive, "blyt/titlelogo.brlyt", &title_assets.title_logo, logger);
    if (not logo_layout_result) {
        return logo_layout_result.failure();
    }

    const auto font_result = populate_font_resource(*font_archive, &title_assets.fonts_by_name, logger);
    if (not font_result) {
        return font_result.failure();
    }

    if (is_korean_title_variant(title_assets)) {
        localize_press_start_for_korean(&title_assets, logger);
    }

    if (title_assets.press_start.animations_by_name.empty() || title_assets.title_logo.animations_by_name.empty()) {
        return make_error("Required title BRLAN animations were not loaded.");
    }

    logger.info(
        __FILE__,
        __LINE__,
        logging::Category::GAME,
        "Loaded title assets: press_start_animations={}, title_logo_animations={}, press_start_textures={}, title_logo_textures={}, fonts={}",
        title_assets.press_start.animations_by_name.size(),
        title_assets.title_logo.animations_by_name.size(),
        title_assets.press_start.textures_by_name.size(),
        title_assets.title_logo.textures_by_name.size(),
        title_assets.fonts_by_name.size());

    const char *debug_materials = std::getenv("SMGPC_DEBUG_TITLE_MATERIALS");
    if (debug_materials != nullptr && debug_materials[0] != '\0' && debug_materials[0] != '0') {
        for (std::size_t i = 0; i < title_assets.title_logo.layout.materials.size(); ++i) {
            const auto &material = title_assets.title_logo.layout.materials[i];
            std::string_view texture_name = "<none>";
            if (material.texture_index >= 0 &&
                static_cast<std::size_t>(material.texture_index) < title_assets.title_logo.layout.texture_names.size()) {
                texture_name = title_assets.title_logo.layout.texture_names[static_cast<std::size_t>(material.texture_index)];
            }
            logger.info(
                __FILE__,
                __LINE__,
                logging::Category::GAME,
                "TitleLogo material[{}]: name={} tex={} tev={} color={},{},{},{}",
                i,
                material.name,
                texture_name,
                material.tev_stage_count,
                material.mat_color[0],
                material.mat_color[1],
                material.mat_color[2],
                material.mat_color[3]);
        }

        for (std::size_t i = 0; i < title_assets.title_logo.layout.panes.size(); ++i) {
            const auto &pane = title_assets.title_logo.layout.panes[i];
            if (pane.type != assets::layout::PaneType::Picture) {
                continue;
            }
            std::string_view material_name = "<none>";
            if (pane.material_index >= 0 &&
                static_cast<std::size_t>(pane.material_index) < title_assets.title_logo.layout.materials.size()) {
                material_name = title_assets.title_logo.layout.materials[static_cast<std::size_t>(pane.material_index)].name;
            }

            logger.info(
                __FILE__,
                __LINE__,
                logging::Category::GAME,
                "TitleLogo picture pane[{}]: name={} material={} ({}) size={}x{}",
                i,
                pane.name,
                pane.material_index,
                material_name,
                pane.size.x,
                pane.size.y);
        }
    }

    return title_assets;
}

}  // namespace smgpc::game::title
