#include "GameAssetPathResolver.hpp"

#include <algorithm>
#include <filesystem>
#include <string>

namespace smgpc::assets {
namespace {

[[nodiscard]] std::string strip_leading_slash(std::string path) {
    while (not path.empty() && path.front() == '/') {
        path.erase(path.begin());
    }
    return path;
}

[[nodiscard]] std::string trim_absolute_prefix(std::string path) {
    while (path.starts_with("./")) {
        path.erase(0U, 2U);
    }
    return strip_leading_slash(std::move(path));
}

[[nodiscard]] std::string add_path_component(std::string_view directory, std::string_view file_name) {
    std::string output(directory);
    if (not output.ends_with('/')) {
        output.push_back('/');
    }

    std::string normalized_name(file_name);
    while (not normalized_name.empty() && normalized_name.front() == '/') {
        normalized_name.erase(normalized_name.begin());
    }
    output.append(normalized_name);
    return output;
}

[[nodiscard]] bool has_extension(std::string_view text, std::string_view extension) {
    if (text.size() < extension.size()) {
        return false;
    }
    return text.substr(text.size() - extension.size()) == extension;
}

}  // namespace

GameAssetPathResolver::GameAssetPathResolver(GameAssetPathResolverConfiguration configuration)
    : _configuration(std::move(configuration)) {
}

std::string GameAssetPathResolver::make_file_name_considering_language(std::string_view file_path) const {
    const auto normalized = normalize_path(file_path);
    const auto localized = make_language_prefixed_path(normalized);
    if (localized != normalized && exists_absolute(localized)) {
        return localized;
    }

    return normalized;
}

std::optional<std::string> GameAssetPathResolver::make_object_archive_file_name(std::string_view file_name) const {
    const auto normalized_name = strip_leading_slash(normalize_slashes(file_name));

    const auto object_path = normalize_path(add_path_component("/ObjectData", normalized_name));
    if (is_file_exist(object_path, true)) {
        return object_path;
    }

    const auto map_parts_path = normalize_path(add_path_component("/MapPartsData", normalized_name));
    if (is_file_exist(map_parts_path, false)) {
        return map_parts_path;
    }

    const auto fallback_path = normalize_path(normalized_name);
    if (is_file_exist(fallback_path, true)) {
        return fallback_path;
    }

    return std::nullopt;
}

std::optional<std::string> GameAssetPathResolver::make_object_archive_file_name_from_prefix(std::string_view file_prefix, bool unused) const {
    (void)unused;
    std::string file_name(file_prefix);
    file_name.append(".arc");
    return make_object_archive_file_name(file_name);
}

std::optional<std::string> GameAssetPathResolver::make_layout_archive_file_name(std::string_view file_name) const {
    const auto normalized_name = strip_leading_slash(normalize_slashes(file_name));

    const auto region_path = normalize_path(add_path_component("/Region/LayoutData", normalized_name));
    if (is_file_exist(region_path, false)) {
        return region_path;
    }

    const auto layout_path = normalize_path(add_path_component("/LayoutData", normalized_name));
    if (is_file_exist(layout_path, true)) {
        return layout_path;
    }

    const auto fallback_path = normalize_path(normalized_name);
    if (is_file_exist(fallback_path, false)) {
        return fallback_path;
    }

    return std::nullopt;
}

std::optional<std::string> GameAssetPathResolver::make_layout_archive_file_name_from_prefix(std::string_view file_prefix, bool fallback) const {
    std::string prefix(file_prefix);
    if (has_extension(prefix, ".arc")) {
        prefix.resize(prefix.size() - 4U);
    }

    const auto base_name = prefix + ".arc";
    const auto aspect_name = prefix + (_configuration.is_widescreen ? "16x9.arc" : "4x3.arc");
    const auto replace_name = prefix + "Replace.arc";

    const auto base_path = make_layout_archive_file_name(base_name);
    const auto aspect_path = make_layout_archive_file_name(aspect_name);
    const auto replace_path = make_layout_archive_file_name(replace_name);

    if (not base_path.has_value() && not aspect_path.has_value() && not replace_path.has_value() && not fallback) {
        return std::nullopt;
    }

    if (aspect_path.has_value()) {
        return aspect_path;
    }
    if (replace_path.has_value()) {
        return replace_path;
    }
    if (base_path.has_value()) {
        return base_path;
    }

    return normalize_path(add_path_component("/LayoutData", base_name));
}

bool GameAssetPathResolver::is_file_exist(std::string_view file_path, bool consider_language) const {
    const auto normalized = normalize_path(file_path);

    if (consider_language) {
        const auto localized = make_language_prefixed_path(normalized);
        if (localized != normalized && exists_absolute(localized)) {
            return true;
        }
    }

    return exists_absolute(normalized);
}

std::string GameAssetPathResolver::normalize_path(std::string_view file_path) const {
    std::string normalized = normalize_slashes(file_path);
    if (normalized.empty()) {
        return "/";
    }

    std::filesystem::path path(normalized);
    path = path.lexically_normal();

    std::string text = path.generic_string();
    if (text.empty() || text == ".") {
        text = "/";
    }

    if (not text.starts_with('/')) {
        text.insert(text.begin(), '/');
    }

    return text;
}

std::string GameAssetPathResolver::to_logical_path(std::string_view file_path) const {
    return trim_absolute_prefix(normalize_path(file_path));
}

const GameAssetPathResolverConfiguration &GameAssetPathResolver::configuration() const {
    return _configuration;
}

bool GameAssetPathResolver::exists_absolute(std::string_view absolute_path) const {
    const auto logical = trim_absolute_prefix(normalize_path(absolute_path));
    if (logical.empty()) {
        return false;
    }

    return std::filesystem::exists(disc_files_root() / logical);
}

std::string GameAssetPathResolver::make_language_prefixed_path(std::string_view absolute_path) const {
    const auto normalized = normalize_path(absolute_path);
    if (_configuration.language.empty()) {
        return normalized;
    }

    const auto language_prefix = "/" + _configuration.language + "/";
    if (normalized == "/" + _configuration.language || normalized.starts_with(language_prefix)) {
        return normalized;
    }

    return "/" + _configuration.language + normalized;
}

std::filesystem::path GameAssetPathResolver::disc_files_root() const {
    return _configuration.game_root / "orig" / _configuration.version / "files";
}

std::string GameAssetPathResolver::normalize_slashes(std::string_view path) {
    std::string output(path);
    std::replace(output.begin(), output.end(), '\\', '/');
    return output;
}

}  // namespace smgpc::assets
