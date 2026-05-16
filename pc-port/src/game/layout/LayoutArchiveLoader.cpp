#include "LayoutArchiveLoader.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "Logger.hpp"
#include "layout/Binary.hpp"
#include "layout/Bmg.hpp"

namespace smgpc::game::layout {
    namespace {

        struct LocalizedPaneName {
            std::string_view base{};
            std::string_view suffix{};
        };

        [[nodiscard]] std::string_view pane_suffix_for_language(std::string_view language) {
            const auto normalized = assets::layout::binary::to_lower_ascii(std::string(language));
            if (normalized == "krkorean" || normalized == "korean" || normalized == "ko" || normalized == "krko") {
                return "KrKo";
            }
            if (normalized == "jpjapanese" || normalized == "japanese" || normalized == "ja" || normalized == "jpja") {
                return "JpJa";
            }
            if (normalized == "cnsimplified" || normalized == "cnsimplifiedchinese" || normalized == "simplifiedchinese" ||
                normalized == "schinese" || normalized == "zhcn" || normalized == "cnsi") {
                return "CnSi";
            }
            return {};
        }

        [[nodiscard]] std::optional< LocalizedPaneName > split_localized_pane_name(std::string_view pane_name) {
            static constexpr std::array< std::string_view, 9 > suffixes{
                "JpJa", "KrKo", "CnSi", "UsEn", "EuEn", "EuFr", "EuEs", "EuDe", "EuIt",
            };

            for (const auto suffix : suffixes) {
                if (pane_name.size() > suffix.size() && pane_name.ends_with(suffix)) {
                    return LocalizedPaneName{
                        .base = pane_name.substr(0U, pane_name.size() - suffix.size()),
                        .suffix = suffix,
                    };
                }
            }

            return std::nullopt;
        }

        [[nodiscard]] std::string layout_message_suffix_for_pane(std::string_view pane_name) {
            const auto localized = split_localized_pane_name(pane_name);
            const auto base_name = localized.has_value() ? localized->base : pane_name;
            if (base_name.size() <= 3U) {
                return {};
            }

            if (base_name.starts_with("Txt") || base_name.starts_with("Sha")) {
                return std::string(base_name.substr(3U));
            }

            return {};
        }

        [[nodiscard]] std::string basename_without_extension_local(std::string_view path) {
            auto name = std::string(path);
            const auto slash = name.find_last_of("/\\");
            if (slash != std::string::npos) {
                name = name.substr(slash + 1U);
            }

            const auto dot = name.find_last_of('.');
            if (dot != std::string::npos) {
                name = name.substr(0U, dot);
            }

            return name;
        }

        void apply_layout_messages(std::string_view archive_path, const assets::layout::BmgMessageMap& messages,
                                   assets::layout::LayoutDefinition* layout) {
            if (layout == nullptr) {
                return;
            }

            const auto archive_base = basename_without_extension_local(archive_path);
            if (archive_base.empty()) {
                return;
            }

            for (auto& pane : layout->panes) {
                if (pane.type != assets::layout::PaneType::Text) {
                    continue;
                }

                const auto suffix = layout_message_suffix_for_pane(pane.name);
                if (suffix.empty()) {
                    continue;
                }

                const auto found = messages.find("Layout_" + archive_base + suffix);
                if (found != messages.end()) {
                    pane.text = found->second;
                }
            }
        }

    }  // namespace

    LayoutArchiveLoader::LayoutArchiveLoader(di::DependencyReference< const assets::AssetLoader > asset_loader,
                                             di::OptionalDependencyReference< logging::ILogger > logger)
        : _asset_loader(std::move(asset_loader)), _logger(logger) {
    }

    assets::AssetResult< std::shared_ptr< LayoutArchiveData > > LayoutArchiveLoader::load(const LayoutArchiveLoadRequest& request) const {
        if (request.archive_path.empty()) {
            return make_error("LayoutArchiveLoader requires a non-empty archive path.");
        }

        if (_asset_loader->archive(request.archive_path) == nullptr) {
            return make_error("Failed to mount layout archive at path: " + request.archive_path);
        }

        auto resource = std::make_shared< LayoutArchiveData >();

        const auto load_layout_result = populate_layout_from_archive(request.archive_path, request.brlyt_path, resource.get());
        if (not load_layout_result) {
            return load_layout_result.failure();
        }
        apply_language_pane_visibility(_asset_loader->language(), &resource->layout);

        const auto messages = _asset_loader->bmg_messages("/MessageData/Message.arc");
        if (messages.has_value()) {
            apply_layout_messages(request.archive_path, *messages, &resource->layout);
        }

        const auto archive_font_result = populate_fonts_from_archive(request.archive_path, resource.get());
        if (not archive_font_result) {
            return archive_font_result.failure();
        }

        if (request.include_shared_fonts) {
            for (const auto& font_archive_path : request.shared_font_archives) {
                const auto shared_font_result = populate_fonts_from_archive(font_archive_path, resource.get());
                if (not shared_font_result && _logger) {
                    _logger->warning(__FILE__, __LINE__, logging::Category::GAME, "Skipping malformed shared font archive {}: {}", font_archive_path,
                                     shared_font_result.failure().message);
                }
            }
        }

        return resource;
    }

    assets::AssetResult< void > LayoutArchiveLoader::populate_layout_from_archive(const std::string& archive_path, const std::string& brlyt_path,
                                                                                  LayoutArchiveData* output) const {
        if (output == nullptr) {
            return make_error("LayoutArchiveLoader output pointer cannot be null.");
        }

        std::string selected_brlyt_path = normalize_name(brlyt_path);
        if (selected_brlyt_path.empty()) {
            const auto brlyt_entries = _asset_loader->archive_entries_with_extension(archive_path, ".brlyt");
            if (!brlyt_entries.empty()) {
                selected_brlyt_path = normalize_name(brlyt_entries.front().path);
            }
        }

        if (selected_brlyt_path.empty()) {
            return make_error("Layout archive did not contain any BRLYT file.");
        }

        auto parsed_layout = _asset_loader->brlyt_layout(archive_path, selected_brlyt_path);
        if (not parsed_layout.has_value()) {
            return make_error("Requested BRLYT was not found or failed to parse in archive: " + selected_brlyt_path);
        }
        output->layout = std::move(*parsed_layout);

        const auto animations = _asset_loader->brlan_animations(archive_path);
        if (animations.has_value()) {
            for (auto& animation : *animations) {
                output->animations_by_name[normalize_name(animation.animation.name)] = std::move(animation.animation);
            }
        }

        const auto textures = _asset_loader->tpl_images(archive_path);
        if (textures.has_value()) {
            for (auto& texture : *textures) {
                output->textures_by_name[normalize_name(texture.name)] = std::move(texture.image);
            }
        }

        return {};
    }

    assets::AssetResult< void > LayoutArchiveLoader::populate_fonts_from_archive(const std::string& archive_path,
                                                                                 LayoutArchiveData* output) const {
        if (output == nullptr) {
            return make_error("LayoutArchiveLoader font output pointer cannot be null.");
        }

        auto fonts = _asset_loader->brfnt_fonts(archive_path);
        if (!fonts.has_value()) {
            return make_error("Failed to mount font archive at path: " + archive_path);
        }

        for (auto& font : *fonts) {
            output->fonts_by_name.emplace(normalize_name(font.font.name()), std::move(font.font));
        }

        return {};
    }

    assets::AssetError LayoutArchiveLoader::make_error(std::string message) {
        return assets::AssetError{
            .code = assets::AssetErrorCode::InvalidFormat,
            .message = std::move(message),
        };
    }

    std::vector< std::string > LayoutArchiveLoader::make_font_name_lookup_keys(std::string_view font_name) {
        std::vector< std::string > keys{};

        const auto primary = normalize_name(basename_without_extension(font_name));
        if (primary.empty()) {
            return keys;
        }

        const auto add_key = [&keys](std::string key) {
            if (key.empty()) {
                return;
            }
            for (const auto& existing : keys) {
                if (existing == key) {
                    return;
                }
            }
            keys.push_back(std::move(key));
        };

        add_key(primary);

        for (std::string_view suffix : {"kor", "sch", "tch", "chi", "cn"}) {
            if (primary.size() > suffix.size() && primary.ends_with(suffix)) {
                add_key(primary.substr(0U, primary.size() - suffix.size()));
            }
        }

        return keys;
    }

    void LayoutArchiveLoader::apply_language_pane_visibility(std::string_view language, assets::layout::LayoutDefinition* layout) {
        if (layout == nullptr) {
            return;
        }

        const auto selected_suffix = pane_suffix_for_language(language);
        std::unordered_map< std::string, bool > has_selected_variant_by_base{};

        for (const auto& pane : layout->panes) {
            const auto localized = split_localized_pane_name(pane.name);
            if (not localized.has_value()) {
                continue;
            }

            auto& has_selected_variant = has_selected_variant_by_base[std::string(localized->base)];
            has_selected_variant = has_selected_variant || (!selected_suffix.empty() && localized->suffix == selected_suffix);
        }

        for (auto& pane : layout->panes) {
            const auto localized = split_localized_pane_name(pane.name);
            if (localized.has_value()) {
                pane.visible = pane.visible && !selected_suffix.empty() && localized->suffix == selected_suffix;
                continue;
            }

            const auto selected_variant_found = has_selected_variant_by_base.find(pane.name);
            if (selected_variant_found != has_selected_variant_by_base.end() && selected_variant_found->second) {
                pane.visible = false;
            }
        }
    }

    std::string LayoutArchiveLoader::basename_without_extension(std::string_view path) {
        auto name = std::string(path);
        const auto slash = name.find_last_of("/\\");
        if (slash != std::string::npos) {
            name = name.substr(slash + 1U);
        }

        const auto dot = name.find_last_of('.');
        if (dot != std::string::npos) {
            name = name.substr(0U, dot);
        }

        return name;
    }

    std::string LayoutArchiveLoader::normalize_name(std::string name) {
        return assets::layout::binary::to_lower_ascii(std::move(name));
    }

}  // namespace smgpc::game::layout
