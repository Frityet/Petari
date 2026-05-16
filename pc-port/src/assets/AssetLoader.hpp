#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "GameAssetService.hpp"
#include "ServiceProvider.hpp"
#include "layout/Bmg.hpp"
#include "layout/Brfnt.hpp"
#include "layout/Brlan.hpp"
#include "layout/Brlyt.hpp"
#include "layout/J3dTexture.hpp"
#include "layout/J3dThumbnail.hpp"
#include "layout/Tpl.hpp"

namespace smgpc::logging {
    class ILogger;
}

namespace smgpc::assets {

    struct ArchiveEntryView {
        std::shared_ptr< const MountedArchiveData > archive{};
        std::string path{};
        std::span< const std::byte > bytes{};
    };

    struct LoadedBrlanAnimation {
        std::string entry_path{};
        std::string name{};
        layout::BrlanAnimation animation{};
    };

    struct LoadedTplImage {
        std::string entry_path{};
        std::string name{};
        layout::tpl::DecodedImage image{};
    };

    struct LoadedBrfntFont {
        std::string entry_path{};
        std::string name{};
        layout::BrfntFont font{};
    };

    class AssetLoader {
    public:
        AssetLoader(di::DependencyReference< IGameAssetService > asset_service, di::OptionalDependencyReference< logging::ILogger > logger = nullptr);

        void request_file(std::string_view file_path) const;
        [[nodiscard]] std::shared_ptr< const std::vector< std::byte > > file(std::string_view file_path) const;
        [[nodiscard]] bool is_loaded_file(std::string_view file_path) const;
        void request_archive(std::string_view archive_path) const;
        [[nodiscard]] std::shared_ptr< const MountedArchiveData > archive(std::string_view archive_path) const;
        [[nodiscard]] bool is_mounted_archive(std::string_view archive_path) const;
        [[nodiscard]] std::optional< ArchiveEntryView > archive_entry(std::string_view archive_path, std::string_view entry_path) const;
        [[nodiscard]] std::optional< ArchiveEntryView > first_archive_entry(std::string_view archive_path, std::span< const std::string > entry_paths) const;
        [[nodiscard]] std::vector< ArchiveEntryView > archive_entries(std::string_view archive_path) const;
        [[nodiscard]] std::vector< ArchiveEntryView > archive_entries_with_extension(std::string_view archive_path, std::string_view extension) const;

        template < typename T, typename Decoder >
        [[nodiscard]] std::optional< T > load_file_as(std::string_view file_path, std::string_view asset_kind, Decoder decoder) const {
            const auto bytes = file(file_path);
            if (bytes == nullptr) {
                return std::nullopt;
            }

            const auto view = std::span< const std::byte >(bytes->data(), bytes->size());
            auto decoded = decoder(view);
            if (!decoded) {
                log_decode_failure(asset_kind, file_path, decoded.failure());
                return std::nullopt;
            }

            return std::move(*decoded);
        }

        template < typename T, typename Decoder >
        [[nodiscard]] std::optional< T > decode_archive_entry_as(const ArchiveEntryView& entry, std::string_view asset_kind, Decoder decoder) const {
            auto decoded = decoder(entry.bytes);
            if (!decoded) {
                log_decode_failure(asset_kind, entry.path, decoded.failure());
                return std::nullopt;
            }

            return std::move(*decoded);
        }

        template < typename T, typename Decoder >
        [[nodiscard]] std::optional< T > load_archive_entry_as(std::string_view archive_path, std::string_view entry_path, std::string_view asset_kind,
                                                               Decoder decoder) const {
            const auto entry = archive_entry(archive_path, entry_path);
            if (!entry.has_value()) {
                return std::nullopt;
            }

            auto decoded = decoder(entry->bytes);
            if (!decoded) {
                log_decode_failure(asset_kind, std::string(entry->path) + " in " + std::string(archive_path), decoded.failure());
                return std::nullopt;
            }

            return std::move(*decoded);
        }

        template < typename T, typename Decoder >
        [[nodiscard]] std::optional< T > load_first_archive_entry_as(std::string_view archive_path, std::span< const std::string > entry_paths,
                                                                     std::string_view asset_kind, Decoder decoder) const {
            const auto entry = first_archive_entry(archive_path, entry_paths);
            if (!entry.has_value()) {
                return std::nullopt;
            }

            auto decoded = decoder(entry->bytes);
            if (!decoded) {
                log_decode_failure(asset_kind, std::string(entry->path) + " in " + std::string(archive_path), decoded.failure());
                return std::nullopt;
            }

            return std::move(*decoded);
        }

        [[nodiscard]] std::optional< layout::LayoutDefinition > brlyt_layout(std::string_view archive_path, std::string_view entry_path) const;
        [[nodiscard]] std::optional< layout::LayoutDefinition > first_brlyt_layout(std::string_view archive_path) const;
        [[nodiscard]] std::optional< layout::BrlanAnimation > brlan_animation(std::string_view archive_path, std::string_view entry_path,
                                                                              std::string_view animation_name = {}) const;
        [[nodiscard]] std::optional< layout::BrfntFont > brfnt_font(std::string_view archive_path, std::string_view entry_path,
                                                                     std::string_view font_name = {}) const;
        [[nodiscard]] std::optional< layout::tpl::DecodedImage > tpl_image(std::string_view archive_path, std::string_view entry_path) const;
        [[nodiscard]] std::optional< layout::tpl::DecodedImage > first_tpl_image(std::string_view archive_path, std::span< const std::string > entry_paths) const;
        [[nodiscard]] std::optional< std::vector< LoadedBrlanAnimation > > brlan_animations(std::string_view archive_path) const;
        [[nodiscard]] std::optional< std::vector< LoadedTplImage > > tpl_images(std::string_view archive_path) const;
        [[nodiscard]] std::optional< std::vector< LoadedBrfntFont > > brfnt_fonts(std::string_view archive_path) const;
        [[nodiscard]] std::optional< layout::BmgMessageMap > bmg_messages(std::string_view archive_path) const;
        [[nodiscard]] std::optional< std::vector< layout::J3dTexture > > j3d_tex1_textures(std::string_view archive_path,
                                                                                           std::string_view model_entry_path) const;
        [[nodiscard]] std::optional< layout::tpl::DecodedImage > j3d_thumbnail(std::string_view archive_path, std::string_view model_entry_path,
                                                                               const layout::J3dThumbnailOptions& options) const;
        [[nodiscard]] std::string file_name_considering_language(std::string_view file_path) const;
        [[nodiscard]] bool is_file_exist(std::string_view file_path, bool consider_language) const;
        [[nodiscard]] std::optional< std::string > object_archive_file_name(std::string_view file_name) const;
        [[nodiscard]] std::optional< std::string > object_archive_file_name_from_prefix(std::string_view file_prefix, bool unused) const;
        [[nodiscard]] std::optional< std::string > layout_archive_file_name(std::string_view file_name) const;
        [[nodiscard]] std::optional< std::string > layout_archive_file_name_from_prefix(std::string_view file_prefix, bool fallback) const;
        [[nodiscard]] std::string normalize_path(std::string_view file_path) const;
        [[nodiscard]] std::string to_logical_path(std::string_view file_path) const;
        [[nodiscard]] const GameAssetPathResolverConfiguration& configuration() const;
        [[nodiscard]] std::string_view language() const;

    private:
        void log_decode_failure(std::string_view asset_kind, std::string_view asset_path, const AssetError& error) const;

        di::DependencyReference< IGameAssetService > _asset_service;
        di::OptionalDependencyReference< logging::ILogger > _logger;
    };

}  // namespace smgpc::assets
