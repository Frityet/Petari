#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace smgpc::assets {

struct GameAssetPathResolverConfiguration {
    std::filesystem::path game_root {};
    std::string version {};
    std::string language {};
    bool is_widescreen {true};
};

class GameAssetPathResolver {
public:
    explicit GameAssetPathResolver(GameAssetPathResolverConfiguration configuration);

    [[nodiscard]] std::string make_file_name_considering_language(std::string_view file_path) const;
    [[nodiscard]] std::optional<std::string> make_object_archive_file_name(std::string_view file_name) const;
    [[nodiscard]] std::optional<std::string> make_object_archive_file_name_from_prefix(std::string_view file_prefix, bool unused) const;
    [[nodiscard]] std::optional<std::string> make_layout_archive_file_name(std::string_view file_name) const;
    [[nodiscard]] std::optional<std::string> make_layout_archive_file_name_from_prefix(std::string_view file_prefix, bool fallback) const;

    [[nodiscard]] bool is_file_exist(std::string_view file_path, bool consider_language) const;
    [[nodiscard]] std::string normalize_path(std::string_view file_path) const;
    [[nodiscard]] std::string to_logical_path(std::string_view file_path) const;
    [[nodiscard]] const GameAssetPathResolverConfiguration &configuration() const;

private:
    [[nodiscard]] bool exists_absolute(std::string_view absolute_path) const;
    [[nodiscard]] std::string make_language_prefixed_path(std::string_view absolute_path) const;
    [[nodiscard]] std::filesystem::path disc_files_root() const;

    static std::string normalize_slashes(std::string_view path);

    GameAssetPathResolverConfiguration _configuration {};
};

}  // namespace smgpc::assets
