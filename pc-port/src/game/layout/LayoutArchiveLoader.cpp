#include "LayoutArchiveLoader.hpp"

#include <cstddef>
#include <string>
#include <utility>

#include "Logger.hpp"
#include "layout/Binary.hpp"

namespace smgpc::game::layout {

LayoutArchiveLoader::LayoutArchiveLoader(
    di::DependencyReference<assets::IGameAssetService> asset_service,
    logging::ILogger *logger)
    : _asset_service(std::move(asset_service)), _logger(logger) {
}

assets::AssetResult<std::shared_ptr<LayoutArchiveData>> LayoutArchiveLoader::load(const LayoutArchiveLoadRequest &request) const {
    if (request.archive_path.empty()) {
        return make_error("LayoutArchiveLoader requires a non-empty archive path.");
    }

    const auto mounted = _asset_service->receive_archive(request.archive_path);
    if (mounted == nullptr) {
        return make_error("Failed to mount layout archive at path: " + request.archive_path);
    }

    auto resource = std::make_shared<LayoutArchiveData>();

    const auto load_layout_result = populate_layout_from_archive(mounted->archive, request.brlyt_path, resource.get());
    if (not load_layout_result) {
        return load_layout_result.failure();
    }

    const auto archive_font_result = populate_fonts_from_archive(mounted->archive, resource.get());
    if (not archive_font_result) {
        return archive_font_result.failure();
    }

    if (request.include_shared_fonts) {
        for (const auto &font_archive_path : request.shared_font_archives) {
            const auto font_archive = _asset_service->receive_archive(font_archive_path);
            if (font_archive == nullptr) {
                if (_logger != nullptr) {
                    _logger->warning(
                        __FILE__,
                        __LINE__,
                        logging::Category::GAME,
                        "Skipping missing shared font archive {}",
                        font_archive_path);
                }
                continue;
            }

            const auto shared_font_result = populate_fonts_from_archive(font_archive->archive, resource.get());
            if (not shared_font_result && _logger != nullptr) {
                _logger->warning(
                    __FILE__,
                    __LINE__,
                    logging::Category::GAME,
                    "Skipping malformed shared font archive {}: {}",
                    font_archive_path,
                    shared_font_result.failure().message);
            }
        }
    }

    return resource;
}

assets::AssetResult<void> LayoutArchiveLoader::populate_layout_from_archive(
    const assets::layout::RarcArchive &archive,
    const std::string &brlyt_path,
    LayoutArchiveData *output) const {
    if (output == nullptr) {
        return make_error("LayoutArchiveLoader output pointer cannot be null.");
    }

    std::string selected_brlyt_path = normalize_name(brlyt_path);
    if (selected_brlyt_path.empty()) {
        for (const auto &entry : archive.entries()) {
            if (has_extension(entry.path, ".brlyt")) {
                selected_brlyt_path = normalize_name(entry.path);
                break;
            }
        }
    }

    if (selected_brlyt_path.empty()) {
        return make_error("Layout archive did not contain any BRLYT file.");
    }

    const auto brlyt_bytes = archive.find_entry(selected_brlyt_path);
    if (brlyt_bytes.empty()) {
        return make_error("Requested BRLYT was not found in archive: " + selected_brlyt_path);
    }

    auto parsed_layout = assets::layout::parse_brlyt(brlyt_bytes);
    if (not parsed_layout) {
        return parsed_layout.failure();
    }
    output->layout = std::move(*parsed_layout);

    for (const auto &entry : archive.entries()) {
        const auto bytes = archive.find_entry(entry.path);
        if (bytes.empty()) {
            continue;
        }

        if (has_extension(entry.path, ".brlan")) {
            auto animation = assets::layout::parse_brlan(bytes, basename_without_extension(entry.path));
            if (not animation) {
                if (_logger != nullptr) {
                    _logger->warning(
                        __FILE__,
                        __LINE__,
                        logging::Category::GAME,
                        "Skipping BRLAN {}: {}",
                        entry.path,
                        animation.failure().message);
                }
                continue;
            }

            output->animations_by_name[normalize_name(animation->name)] = std::move(*animation);
            continue;
        }

        if (has_extension(entry.path, ".tpl")) {
            auto decoded = assets::layout::tpl::decode_tpl_first_image(bytes);
            if (not decoded) {
                if (_logger != nullptr) {
                    _logger->warning(
                        __FILE__,
                        __LINE__,
                        logging::Category::GAME,
                        "Skipping TPL {}: {}",
                        entry.path,
                        decoded.failure().message);
                }
                continue;
            }

            output->textures_by_name[normalize_name(basename_without_extension(entry.path))] = std::move(*decoded);
            continue;
        }
    }

    return {};
}

assets::AssetResult<void> LayoutArchiveLoader::populate_fonts_from_archive(
    const assets::layout::RarcArchive &archive,
    LayoutArchiveData *output) const {
    if (output == nullptr) {
        return make_error("LayoutArchiveLoader font output pointer cannot be null.");
    }

    for (const auto &entry : archive.entries()) {
        if (not has_extension(entry.path, ".brfnt")) {
            continue;
        }

        const auto bytes = archive.find_entry(entry.path);
        if (bytes.empty()) {
            continue;
        }

        auto font = assets::layout::parse_brfnt(bytes, basename_without_extension(entry.path));
        if (not font) {
            if (_logger != nullptr) {
                _logger->warning(
                    __FILE__,
                    __LINE__,
                    logging::Category::GAME,
                    "Skipping BRFNT {}: {}",
                    entry.path,
                    font.failure().message);
            }
            continue;
        }

        output->fonts_by_name.emplace(normalize_name(font->name()), std::move(*font));
    }

    return {};
}

assets::AssetError LayoutArchiveLoader::make_error(std::string message) {
    return assets::AssetError {
        .code = assets::AssetErrorCode::InvalidFormat,
        .message = std::move(message),
    };
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

bool LayoutArchiveLoader::has_extension(std::string_view path, std::string_view extension) {
    return normalize_name(std::string(path)).ends_with(normalize_name(std::string(extension)));
}

}  // namespace smgpc::game::layout
