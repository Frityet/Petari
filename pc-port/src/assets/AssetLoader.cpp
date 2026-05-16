#include "AssetLoader.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>

#include "Logger.hpp"
#include "layout/Binary.hpp"

namespace smgpc::assets {
    namespace {

        [[nodiscard]] std::string basename_without_extension(std::string_view path) {
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

        [[nodiscard]] bool has_extension(std::string_view path, std::string_view extension) {
            if (extension.empty()) {
                return true;
            }

            auto normalized_path = layout::binary::to_lower_ascii(std::string(path));
            auto normalized_extension = layout::binary::to_lower_ascii(std::string(extension));
            if (!normalized_extension.empty() && normalized_extension.front() != '.') {
                normalized_extension.insert(normalized_extension.begin(), '.');
            }
            return normalized_path.size() >= normalized_extension.size() &&
                   normalized_path.compare(normalized_path.size() - normalized_extension.size(), normalized_extension.size(), normalized_extension) == 0;
        }

    }  // namespace

    AssetLoader::AssetLoader(di::DependencyReference< IGameAssetService > asset_service, di::OptionalDependencyReference< logging::ILogger > logger)
        : _asset_service(std::move(asset_service)), _logger(logger) {
    }

    void AssetLoader::log_decode_failure(std::string_view asset_kind, std::string_view asset_path, const AssetError& error) const {
        if (_logger) {
            _logger->warning(__FILE__, __LINE__, logging::Category::ASSET, "Failed to decode {} {}: {}", asset_kind, asset_path, error.message);
        }
    }

    void AssetLoader::request_file(std::string_view file_path) const {
        _asset_service->request_load_file(file_path);
    }

    std::shared_ptr< const std::vector< std::byte > > AssetLoader::file(std::string_view file_path) const {
        auto bytes = _asset_service->receive_file(file_path);
        if (bytes == nullptr && _logger) {
            _logger->warning(__FILE__, __LINE__, logging::Category::ASSET, "Missing file {}", file_path);
        }
        return bytes;
    }

    bool AssetLoader::is_loaded_file(std::string_view file_path) const {
        return _asset_service->is_loaded_file(file_path);
    }

    void AssetLoader::request_archive(std::string_view archive_path) const {
        _asset_service->request_mount_archive(archive_path);
    }

    std::shared_ptr< const MountedArchiveData > AssetLoader::archive(std::string_view archive_path) const {
        return _asset_service->receive_archive(archive_path);
    }

    bool AssetLoader::is_mounted_archive(std::string_view archive_path) const {
        return _asset_service->is_mounted_archive(archive_path);
    }

    std::optional< ArchiveEntryView > AssetLoader::archive_entry(std::string_view archive_path, std::string_view entry_path) const {
        auto mounted = archive(archive_path);
        if (mounted == nullptr) {
            if (_logger) {
                _logger->warning(__FILE__, __LINE__, logging::Category::ASSET, "Missing archive {}", archive_path);
            }
            return std::nullopt;
        }

        auto bytes = mounted->archive.find_entry(std::string(entry_path));
        if (bytes.empty()) {
            if (_logger) {
                _logger->warning(__FILE__, __LINE__, logging::Category::ASSET, "Missing archive entry {} in {}", entry_path, archive_path);
            }
            return std::nullopt;
        }

        return ArchiveEntryView{
            .archive = std::move(mounted),
            .path = std::string(entry_path),
            .bytes = bytes,
        };
    }

    std::optional< ArchiveEntryView > AssetLoader::first_archive_entry(std::string_view archive_path, std::span< const std::string > entry_paths) const {
        auto mounted = archive(archive_path);
        if (mounted == nullptr) {
            if (_logger) {
                _logger->warning(__FILE__, __LINE__, logging::Category::ASSET, "Missing archive {}", archive_path);
            }
            return std::nullopt;
        }

        for (const auto& entry_path : entry_paths) {
            auto bytes = mounted->archive.find_entry(entry_path);
            if (!bytes.empty()) {
                return ArchiveEntryView{
                    .archive = mounted,
                    .path = entry_path,
                    .bytes = bytes,
                };
            }
        }

        if (_logger) {
            _logger->warning(__FILE__, __LINE__, logging::Category::ASSET, "Missing all candidate entries in {}", archive_path);
        }
        return std::nullopt;
    }

    std::vector< ArchiveEntryView > AssetLoader::archive_entries(std::string_view archive_path) const {
        auto mounted = archive(archive_path);
        if (mounted == nullptr) {
            if (_logger) {
                _logger->warning(__FILE__, __LINE__, logging::Category::ASSET, "Missing archive {}", archive_path);
            }
            return {};
        }

        std::vector< ArchiveEntryView > output{};
        output.reserve(mounted->archive.entries().size());
        for (const auto& entry : mounted->archive.entries()) {
            auto bytes = mounted->archive.find_entry(entry.path);
            if (bytes.empty()) {
                continue;
            }

            output.push_back(ArchiveEntryView{
                .archive = mounted,
                .path = entry.path,
                .bytes = bytes,
            });
        }
        return output;
    }

    std::vector< ArchiveEntryView > AssetLoader::archive_entries_with_extension(std::string_view archive_path, std::string_view extension) const {
        auto entries = archive_entries(archive_path);
        entries.erase(std::remove_if(entries.begin(), entries.end(), [extension](const ArchiveEntryView& entry) {
            return !has_extension(entry.path, extension);
        }), entries.end());
        return entries;
    }

    std::optional< layout::LayoutDefinition > AssetLoader::brlyt_layout(std::string_view archive_path, std::string_view entry_path) const {
        return load_archive_entry_as< layout::LayoutDefinition >(archive_path, entry_path, "BRLYT layout",
                                                                 [](std::span< const std::byte > bytes) {
                                                                     return layout::parse_brlyt(bytes);
                                                                 });
    }

    std::optional< layout::LayoutDefinition > AssetLoader::first_brlyt_layout(std::string_view archive_path) const {
        const auto entries = archive_entries_with_extension(archive_path, ".brlyt");
        for (const auto& entry : entries) {
            auto decoded = decode_archive_entry_as< layout::LayoutDefinition >(entry, "BRLYT layout", [](std::span< const std::byte > bytes) {
                return layout::parse_brlyt(bytes);
            });
            if (decoded.has_value()) {
                return decoded;
            }
        }

        return std::nullopt;
    }

    std::optional< layout::BrlanAnimation > AssetLoader::brlan_animation(std::string_view archive_path, std::string_view entry_path,
                                                                         std::string_view animation_name) const {
        const auto resolved_name = animation_name.empty() ? basename_without_extension(entry_path) : std::string(animation_name);
        return load_archive_entry_as< layout::BrlanAnimation >(archive_path, entry_path, "BRLAN animation",
                                                               [&resolved_name](std::span< const std::byte > bytes) {
                                                                   return layout::parse_brlan(bytes, resolved_name);
                                                               });
    }

    std::optional< layout::BrfntFont > AssetLoader::brfnt_font(std::string_view archive_path, std::string_view entry_path,
                                                               std::string_view font_name) const {
        const auto resolved_name = font_name.empty() ? basename_without_extension(entry_path) : std::string(font_name);
        return load_archive_entry_as< layout::BrfntFont >(archive_path, entry_path, "BRFNT font",
                                                          [&resolved_name](std::span< const std::byte > bytes) {
                                                              return layout::parse_brfnt(bytes, resolved_name);
                                                          });
    }

    std::optional< layout::tpl::DecodedImage > AssetLoader::tpl_image(std::string_view archive_path, std::string_view entry_path) const {
        auto decoded = load_archive_entry_as< layout::tpl::DecodedImage >(archive_path, entry_path, "TPL image",
                                                                          [](std::span< const std::byte > bytes) {
                                                                              return layout::tpl::decode_tpl_first_image(bytes);
                                                                          });
        if (!decoded.has_value() || decoded->empty()) {
            return std::nullopt;
        }

        return decoded;
    }

    std::optional< layout::tpl::DecodedImage > AssetLoader::first_tpl_image(std::string_view archive_path, std::span< const std::string > entry_paths) const {
        auto decoded = load_first_archive_entry_as< layout::tpl::DecodedImage >(archive_path, entry_paths, "TPL image",
                                                                                [](std::span< const std::byte > bytes) {
                                                                                    return layout::tpl::decode_tpl_first_image(bytes);
                                                                                });
        if (!decoded.has_value() || decoded->empty()) {
            return std::nullopt;
        }

        return decoded;
    }

    std::optional< std::vector< LoadedBrlanAnimation > > AssetLoader::brlan_animations(std::string_view archive_path) const {
        const auto mounted = archive(archive_path);
        if (mounted == nullptr) {
            if (_logger) {
                _logger->warning(__FILE__, __LINE__, logging::Category::ASSET, "Missing archive {}", archive_path);
            }
            return std::nullopt;
        }

        std::vector< LoadedBrlanAnimation > output{};
        for (const auto& entry : archive_entries_with_extension(archive_path, ".brlan")) {
            const auto name = basename_without_extension(entry.path);
            auto animation = decode_archive_entry_as< layout::BrlanAnimation >(entry, "BRLAN animation", [&name](std::span< const std::byte > bytes) {
                return layout::parse_brlan(bytes, name);
            });
            if (!animation.has_value()) {
                continue;
            }

            output.push_back(LoadedBrlanAnimation{
                .entry_path = entry.path,
                .name = name,
                .animation = std::move(*animation),
            });
        }

        return output;
    }

    std::optional< std::vector< LoadedTplImage > > AssetLoader::tpl_images(std::string_view archive_path) const {
        const auto mounted = archive(archive_path);
        if (mounted == nullptr) {
            if (_logger) {
                _logger->warning(__FILE__, __LINE__, logging::Category::ASSET, "Missing archive {}", archive_path);
            }
            return std::nullopt;
        }

        std::vector< LoadedTplImage > output{};
        for (const auto& entry : archive_entries_with_extension(archive_path, ".tpl")) {
            auto decoded = decode_archive_entry_as< layout::tpl::DecodedImage >(entry, "TPL image", [](std::span< const std::byte > bytes) {
                return layout::tpl::decode_tpl_first_image(bytes);
            });
            if (!decoded.has_value()) {
                continue;
            }

            output.push_back(LoadedTplImage{
                .entry_path = entry.path,
                .name = basename_without_extension(entry.path),
                .image = std::move(*decoded),
            });
        }

        return output;
    }

    std::optional< std::vector< LoadedBrfntFont > > AssetLoader::brfnt_fonts(std::string_view archive_path) const {
        const auto mounted = archive(archive_path);
        if (mounted == nullptr) {
            if (_logger) {
                _logger->warning(__FILE__, __LINE__, logging::Category::ASSET, "Missing archive {}", archive_path);
            }
            return std::nullopt;
        }

        std::vector< LoadedBrfntFont > output{};
        for (const auto& entry : archive_entries_with_extension(archive_path, ".brfnt")) {
            const auto name = basename_without_extension(entry.path);
            auto font = decode_archive_entry_as< layout::BrfntFont >(entry, "BRFNT font", [&name](std::span< const std::byte > bytes) {
                return layout::parse_brfnt(bytes, name);
            });
            if (!font.has_value()) {
                continue;
            }

            output.push_back(LoadedBrfntFont{
                .entry_path = entry.path,
                .name = font->name(),
                .font = std::move(*font),
            });
        }

        return output;
    }

    std::optional< layout::BmgMessageMap > AssetLoader::bmg_messages(std::string_view archive_path) const {
        const auto bmg_entry = archive_entry(archive_path, "message.bmg");
        const auto table_entry = archive_entry(archive_path, "messageid.tbl");
        if (!bmg_entry.has_value() || !table_entry.has_value()) {
            return std::nullopt;
        }

        auto messages = layout::parse_bmg_messages(bmg_entry->bytes, table_entry->bytes);
        if (!messages) {
            log_decode_failure("BMG message table", archive_path, messages.failure());
            return std::nullopt;
        }

        return *messages;
    }

    std::optional< std::vector< layout::J3dTexture > > AssetLoader::j3d_tex1_textures(std::string_view archive_path,
                                                                                      std::string_view model_entry_path) const {
        return load_archive_entry_as< std::vector< layout::J3dTexture > >(archive_path, model_entry_path, "J3D TEX1 textures",
                                                                          [](std::span< const std::byte > bytes) {
                                                                              return layout::parse_j3d_tex1_textures(bytes);
                                                                          });
    }

    std::optional< layout::tpl::DecodedImage > AssetLoader::j3d_thumbnail(std::string_view archive_path, std::string_view model_entry_path,
                                                                          const layout::J3dThumbnailOptions& options) const {
        auto thumbnail = load_archive_entry_as< layout::tpl::DecodedImage >(archive_path, model_entry_path, "J3D thumbnail",
                                                                            [&options](std::span< const std::byte > bytes) {
                                                                                return layout::render_j3d_thumbnail(bytes, options);
                                                                            });
        if (!thumbnail.has_value() || thumbnail->empty()) {
            return std::nullopt;
        }

        return thumbnail;
    }

    std::string AssetLoader::file_name_considering_language(std::string_view file_path) const {
        return _asset_service->path_resolver().make_file_name_considering_language(file_path);
    }

    bool AssetLoader::is_file_exist(std::string_view file_path, bool consider_language) const {
        return _asset_service->path_resolver().is_file_exist(file_path, consider_language);
    }

    std::optional< std::string > AssetLoader::object_archive_file_name(std::string_view file_name) const {
        return _asset_service->path_resolver().make_object_archive_file_name(file_name);
    }

    std::optional< std::string > AssetLoader::object_archive_file_name_from_prefix(std::string_view file_prefix, bool unused) const {
        return _asset_service->path_resolver().make_object_archive_file_name_from_prefix(file_prefix, unused);
    }

    std::optional< std::string > AssetLoader::layout_archive_file_name(std::string_view file_name) const {
        return _asset_service->path_resolver().make_layout_archive_file_name(file_name);
    }

    std::optional< std::string > AssetLoader::layout_archive_file_name_from_prefix(std::string_view file_prefix, bool fallback) const {
        return _asset_service->path_resolver().make_layout_archive_file_name_from_prefix(file_prefix, fallback);
    }

    std::string AssetLoader::normalize_path(std::string_view file_path) const {
        return _asset_service->path_resolver().normalize_path(file_path);
    }

    std::string AssetLoader::to_logical_path(std::string_view file_path) const {
        return _asset_service->path_resolver().to_logical_path(file_path);
    }

    const GameAssetPathResolverConfiguration& AssetLoader::configuration() const {
        return _asset_service->path_resolver().configuration();
    }

    std::string_view AssetLoader::language() const {
        return _asset_service->path_resolver().configuration().language;
    }

}  // namespace smgpc::assets
