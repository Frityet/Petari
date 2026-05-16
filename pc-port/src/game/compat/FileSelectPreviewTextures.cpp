#include "FileSelectPreviewTextures.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Game/Map/FileSelectIconID.hpp"
#include "compat/RuntimeAssetLoader.hpp"

namespace smgpc::game::file_select_preview {
    namespace {

        [[nodiscard]] std::string trim_ascii_space(std::string text) {
            const auto first = text.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) {
                return {};
            }

            const auto last = text.find_last_not_of(" \t\r\n");
            return text.substr(first, last - first + 1U);
        }

        [[nodiscard]] bool get_bool_from_environment(const char* name) {
            const char* value = std::getenv(name);
            if (value == nullptr || value[0] == '\0') {
                return false;
            }

            const auto text = trim_ascii_space(value);
            return text != "0" && text != "false" && text != "False" && text != "FALSE";
        }

        [[nodiscard]] std::optional< bool > get_optional_bool_from_environment(const char* name) {
            const char* value = std::getenv(name);
            if (value == nullptr || value[0] == '\0') {
                return std::nullopt;
            }

            const auto text = trim_ascii_space(value);
            return text != "0" && text != "false" && text != "False" && text != "FALSE";
        }

        [[nodiscard]] std::optional< float > get_float_from_environment(const char* name) {
            const char* value = std::getenv(name);
            if (value == nullptr || value[0] == '\0') {
                return std::nullopt;
            }

            char* end{};
            const float parsed = std::strtof(value, &end);
            if (end == value || !std::isfinite(parsed)) {
                return std::nullopt;
            }

            return parsed;
        }

        [[nodiscard]] float environment_float_or(const char* name, float fallback) {
            if (const auto value = get_float_from_environment(name)) {
                return *value;
            }

            return fallback;
        }

        [[nodiscard]] std::uint8_t clamp_u8(float value) {
            if (value <= 0.0F) {
                return 0U;
            }
            if (value >= 255.0F) {
                return 255U;
            }
            return static_cast< std::uint8_t >(std::lround(value));
        }

    }  // namespace

    [[nodiscard]] assets::layout::tpl::DecodedImage make_planet_impostor_texture(const assets::layout::tpl::DecodedImage& source) {
        assets::layout::tpl::DecodedImage output{};
        output.width = source.width;
        output.height = source.height;
        output.rgba8.assign(static_cast< std::size_t >(output.width) * output.height * 4U, 0U);

        if (source.empty()) {
            return output;
        }

        const float light_x = -0.35F;
        const float light_y = -0.45F;
        const float light_z = 0.82F;
        for (std::uint16_t y = 0U; y < output.height; ++y) {
            for (std::uint16_t x = 0U; x < output.width; ++x) {
                const float nx = ((static_cast< float >(x) + 0.5F) / static_cast< float >(output.width)) * 2.0F - 1.0F;
                const float ny = ((static_cast< float >(y) + 0.5F) / static_cast< float >(output.height)) * 2.0F - 1.0F;
                const float radius_squared = nx * nx + ny * ny;
                const auto dst = (static_cast< std::size_t >(y) * output.width + x) * 4U;
                if (radius_squared > 1.0F) {
                    continue;
                }

                const float nz = std::sqrt(std::max(0.0F, 1.0F - radius_squared));
                const float light = std::max(0.0F, nx * light_x + ny * light_y + nz * light_z);
                const float edge = std::clamp((1.0F - radius_squared) * 5.0F, 0.0F, 1.0F);
                const float shade = (0.28F + 0.72F * light) * (0.62F + 0.38F * edge);

                const auto src = (static_cast< std::size_t >(y) * source.width + x) * 4U;
                output.rgba8[dst + 0U] = clamp_u8(static_cast< float >(source.rgba8[src + 0U]) * shade * 0.55F);
                output.rgba8[dst + 1U] = clamp_u8(static_cast< float >(source.rgba8[src + 1U]) * shade * 0.78F);
                output.rgba8[dst + 2U] = clamp_u8(static_cast< float >(source.rgba8[src + 2U]) * shade * 0.92F);
                output.rgba8[dst + 3U] = clamp_u8(255.0F * edge);
            }
        }

        return output;
    }

    [[nodiscard]] assets::layout::tpl::DecodedImage make_sky_star_texture(const assets::layout::tpl::DecodedImage& source) {
        assets::layout::tpl::DecodedImage output{};
        output.width = source.width;
        output.height = source.height;
        output.rgba8.assign(static_cast< std::size_t >(output.width) * output.height * 4U, 255U);

        if (source.empty()) {
            return output;
        }

        for (std::uint16_t y = 0U; y < output.height; ++y) {
            const float fy = static_cast< float >(y) / static_cast< float >(std::max< std::uint16_t >(1U, output.height - 1U));
            const float lower = std::clamp((fy - 0.48F) / 0.52F, 0.0F, 1.0F);
            const float upper = 1.0F - lower;
            const float base_r = upper * 3.0F + lower * 0.0F;
            const float base_g = upper * 66.0F + lower * 51.0F;
            const float base_b = upper * 97.0F + lower * 79.0F;

            for (std::uint16_t x = 0U; x < output.width; ++x) {
                const auto index = (static_cast< std::size_t >(y) * output.width + x) * 4U;
                const auto src = (static_cast< std::size_t >(y) * source.width + x) * 4U;
                const float star = static_cast< float >(std::max({
                                       source.rgba8[src + 0U],
                                       source.rgba8[src + 1U],
                                       source.rgba8[src + 2U],
                                       source.rgba8[src + 3U],
                                   })) /
                                   255.0F;
                const float glow = std::pow(star, 0.72F);
                const float pin = std::pow(star, 2.2F);

                output.rgba8[index + 0U] = clamp_u8(base_r - 4.0F + glow * 26.0F + pin * 180.0F);
                output.rgba8[index + 1U] = clamp_u8(base_g + 2.0F + glow * 66.0F + pin * 170.0F);
                output.rgba8[index + 2U] = clamp_u8(base_b + 10.0F + glow * 118.0F + pin * 155.0F);
                output.rgba8[index + 3U] = 255U;
            }
        }

        return output;
    }

    [[nodiscard]] assets::layout::tpl::DecodedImage make_title_planet_surface_texture(const assets::layout::tpl::DecodedImage& source) {
        assets::layout::tpl::DecodedImage output{};
        output.width = source.width;
        output.height = source.height;
        output.rgba8.assign(static_cast< std::size_t >(output.width) * output.height * 4U, 0U);

        if (source.empty()) {
            return output;
        }

        for (std::uint16_t y = 0U; y < output.height; ++y) {
            const float fy = static_cast< float >(y) / static_cast< float >(std::max< std::uint16_t >(1U, output.height - 1U));
            const float lower_fade = std::clamp((fy - 0.02F) / 0.18F, 0.0F, 1.0F);
            const float horizon_fade = 1.0F - std::clamp((fy - 0.76F) / 0.24F, 0.0F, 1.0F);
            const float vertical_alpha = lower_fade * (0.36F + 0.64F * horizon_fade);
            const float cold_shadow = std::clamp((fy - 0.34F) / 0.38F, 0.0F, 1.0F);

            for (std::uint16_t x = 0U; x < output.width; ++x) {
                const auto index = (static_cast< std::size_t >(y) * output.width + x) * 4U;
                const float r = static_cast< float >(source.rgba8[index + 0U]);
                const float g = static_cast< float >(source.rgba8[index + 1U]);
                const float b = static_cast< float >(source.rgba8[index + 2U]);
                const float value = std::max({r, g, b}) / 255.0F;
                const float cloud = std::pow(std::clamp(value, 0.0F, 1.0F), 0.72F);
                const float ridge = std::clamp((cloud - 0.22F) / 0.58F, 0.0F, 1.0F);
                const float nx = std::abs(((static_cast< float >(x) + 0.5F) / static_cast< float >(std::max< std::uint16_t >(1U, output.width))) * 2.0F - 1.0F);
                const float side_lift = 0.84F + 0.18F * nx;

                const float base_r = 3.0F + 70.0F * ridge + 28.0F * cold_shadow;
                const float base_g = 34.0F + 95.0F * ridge + 18.0F * cold_shadow;
                const float base_b = 58.0F + 104.0F * ridge + 18.0F * cold_shadow;

                output.rgba8[index + 0U] = clamp_u8(base_r * side_lift);
                output.rgba8[index + 1U] = clamp_u8(base_g * side_lift);
                output.rgba8[index + 2U] = clamp_u8(base_b * side_lift);
                output.rgba8[index + 3U] = clamp_u8(vertical_alpha * (58.0F + 116.0F * ridge));
            }
        }

        return output;
    }

    [[nodiscard]] assets::layout::tpl::DecodedImage make_sky_haze_texture(const assets::layout::tpl::DecodedImage& source) {
        assets::layout::tpl::DecodedImage output{};
        output.width = source.width;
        output.height = source.height;
        output.rgba8.assign(static_cast< std::size_t >(output.width) * output.height * 4U, 0U);

        if (source.empty()) {
            return output;
        }

        for (std::uint16_t y = 0U; y < output.height; ++y) {
            const float fy = static_cast< float >(y) / static_cast< float >(std::max< std::uint16_t >(1U, output.height - 1U));
            const float glow = fy * fy * (3.0F - 2.0F * fy);
            for (std::uint16_t x = 0U; x < output.width; ++x) {
                const auto index = (static_cast< std::size_t >(y) * output.width + x) * 4U;
                const float nx = std::abs(((static_cast< float >(x) + 0.5F) / static_cast< float >(std::max< std::uint16_t >(1U, output.width))) * 2.0F - 1.0F);
                const float edge_lift = 0.92F + 0.10F * std::sqrt(nx);
                const float shade = edge_lift * (0.86F + 0.14F * glow);

                output.rgba8[index + 0U] = clamp_u8((64.0F + 32.0F * glow) * shade);
                output.rgba8[index + 1U] = clamp_u8((172.0F + 10.0F * glow) * shade);
                output.rgba8[index + 2U] = clamp_u8((196.0F - 8.0F * glow) * shade);
                output.rgba8[index + 3U] = clamp_u8((0.54F + 0.16F * glow) * 255.0F);
            }
        }

        return output;
    }

    [[nodiscard]] assets::layout::tpl::DecodedImage make_sky_nebula_texture(const assets::layout::tpl::DecodedImage& source) {
        assets::layout::tpl::DecodedImage output{};
        output.width = source.width;
        output.height = source.height;
        output.rgba8.assign(static_cast< std::size_t >(output.width) * output.height * 4U, 0U);

        if (source.empty()) {
            return output;
        }

        for (std::uint16_t y = 0U; y < output.height; ++y) {
            for (std::uint16_t x = 0U; x < output.width; ++x) {
                const auto index = (static_cast< std::size_t >(y) * output.width + x) * 4U;
                const float r = static_cast< float >(source.rgba8[index + 0U]);
                const float g = static_cast< float >(source.rgba8[index + 1U]);
                const float b = static_cast< float >(source.rgba8[index + 2U]);
                const float value = std::max({r, g, b});
                const float alpha = std::clamp((value - 4.0F) / 56.0F, 0.0F, 1.0F);

                output.rgba8[index + 0U] = clamp_u8(r * 1.18F + value * 0.22F);
                output.rgba8[index + 1U] = clamp_u8(g * 0.92F + value * 0.08F);
                output.rgba8[index + 2U] = clamp_u8(b * 0.74F);
                output.rgba8[index + 3U] = clamp_u8(alpha * 150.0F);
            }
        }

        return output;
    }

    [[nodiscard]] assets::layout::tpl::DecodedImage make_sky_comet_halo_texture(const assets::layout::tpl::DecodedImage& source) {
        assets::layout::tpl::DecodedImage output{};
        output.width = source.width;
        output.height = source.height;
        output.rgba8.assign(static_cast< std::size_t >(output.width) * output.height * 4U, 0U);

        if (source.empty()) {
            return output;
        }

        for (std::uint16_t y = 0U; y < output.height; ++y) {
            const float fy = static_cast< float >(y) / static_cast< float >(std::max< std::uint16_t >(1U, output.height - 1U));
            for (std::uint16_t x = 0U; x < output.width; ++x) {
                const auto index = (static_cast< std::size_t >(y) * output.width + x) * 4U;
                const float value = static_cast< float >(std::max({
                                        source.rgba8[index + 0U],
                                        source.rgba8[index + 1U],
                                        source.rgba8[index + 2U],
                                        source.rgba8[index + 3U],
                                    })) /
                                    255.0F;
                const float alpha = std::clamp((value - 0.13F) / 0.72F, 0.0F, 1.0F);
                const float warm = 1.0F - std::clamp((fy - 0.12F) / 0.74F, 0.0F, 1.0F);

                output.rgba8[index + 0U] = clamp_u8(value * (138.0F + warm * 132.0F));
                output.rgba8[index + 1U] = clamp_u8(value * (44.0F + warm * 54.0F));
                output.rgba8[index + 2U] = clamp_u8(value * (116.0F + (1.0F - warm) * 80.0F));
                output.rgba8[index + 3U] = clamp_u8(alpha * 142.0F);
            }
        }

        return output;
    }

    assets::layout::tpl::DecodedImage make_file_number_badge_texture() {
        constexpr std::uint16_t size = 64U;

        assets::layout::tpl::DecodedImage output{};
        output.width = size;
        output.height = size;
        output.rgba8.assign(static_cast< std::size_t >(size) * size * 4U, 0U);

        for (std::uint16_t y = 0U; y < size; ++y) {
            for (std::uint16_t x = 0U; x < size; ++x) {
                const float nx = ((static_cast< float >(x) + 0.5F) / static_cast< float >(size)) * 2.0F - 1.0F;
                const float ny = ((static_cast< float >(y) + 0.5F) / static_cast< float >(size)) * 2.0F - 1.0F;
                const float radius = std::sqrt(nx * nx + ny * ny);
                if (radius > 1.0F) {
                    continue;
                }

                const float edge = std::clamp((1.0F - radius) * 18.0F, 0.0F, 1.0F);
                const float rim = std::clamp((radius - 0.70F) / 0.22F, 0.0F, 1.0F);
                const float light = std::clamp(0.78F - ny * 0.18F - nx * 0.08F, 0.0F, 1.0F);

                const float center_r = 48.0F * light;
                const float center_g = 36.0F * light;
                const float center_b = 27.0F * light;
                const float rim_r = 118.0F * light;
                const float rim_g = 91.0F * light;
                const float rim_b = 63.0F * light;

                const auto index = (static_cast< std::size_t >(y) * size + x) * 4U;
                output.rgba8[index + 0U] = clamp_u8(center_r * (1.0F - rim) + rim_r * rim);
                output.rgba8[index + 1U] = clamp_u8(center_g * (1.0F - rim) + rim_g * rim);
                output.rgba8[index + 2U] = clamp_u8(center_b * (1.0F - rim) + rim_b * rim);
                output.rgba8[index + 3U] = clamp_u8(255.0F * edge);
            }
        }

        return output;
    }

    assets::layout::tpl::DecodedImage make_prompt_pill_texture() {
        constexpr std::uint16_t width = 512U;
        constexpr std::uint16_t height = 48U;
        constexpr float radius = 22.0F;

        assets::layout::tpl::DecodedImage output{};
        output.width = width;
        output.height = height;
        output.rgba8.assign(static_cast< std::size_t >(width) * height * 4U, 0U);

        for (std::uint16_t y = 0U; y < height; ++y) {
            for (std::uint16_t x = 0U; x < width; ++x) {
                const float fx = static_cast< float >(x) + 0.5F;
                const float fy = static_cast< float >(y) + 0.5F;
                const float cx = std::clamp(fx, radius, static_cast< float >(width) - radius);
                const float cy = std::clamp(fy, radius, static_cast< float >(height) - radius);
                const float dx = fx - cx;
                const float dy = fy - cy;
                const float distance = std::sqrt(dx * dx + dy * dy);
                const float alpha = std::clamp((radius - distance) * 2.0F, 0.0F, 1.0F);
                if (alpha <= 0.0F) {
                    continue;
                }

                const float vertical = fy / static_cast< float >(height);
                const auto index = (static_cast< std::size_t >(y) * width + x) * 4U;
                output.rgba8[index + 0U] = clamp_u8(54.0F + vertical * 16.0F);
                output.rgba8[index + 1U] = clamp_u8(166.0F + vertical * 26.0F);
                output.rgba8[index + 2U] = clamp_u8(222.0F + vertical * 10.0F);
                output.rgba8[index + 3U] = clamp_u8(alpha * 232.0F);
            }
        }

        return output;
    }

    assets::layout::tpl::DecodedImage make_page_counter_pill_texture() {
        constexpr std::uint16_t width = 128U;
        constexpr std::uint16_t height = 40U;
        constexpr float radius = 18.0F;

        assets::layout::tpl::DecodedImage output{};
        output.width = width;
        output.height = height;
        output.rgba8.assign(static_cast< std::size_t >(width) * height * 4U, 0U);

        for (std::uint16_t y = 0U; y < height; ++y) {
            for (std::uint16_t x = 0U; x < width; ++x) {
                const float fx = static_cast< float >(x) + 0.5F;
                const float fy = static_cast< float >(y) + 0.5F;
                const float cx = std::clamp(fx, radius, static_cast< float >(width) - radius);
                const float cy = std::clamp(fy, radius, static_cast< float >(height) - radius);
                const float dx = fx - cx;
                const float dy = fy - cy;
                const float distance = std::sqrt(dx * dx + dy * dy);
                const float alpha = std::clamp((radius - distance) * 2.0F, 0.0F, 1.0F);
                if (alpha <= 0.0F) {
                    continue;
                }

                const float vertical = fy / static_cast< float >(height);
                const auto index = (static_cast< std::size_t >(y) * width + x) * 4U;
                output.rgba8[index + 0U] = clamp_u8(232.0F - vertical * 16.0F);
                output.rgba8[index + 1U] = clamp_u8(250.0F - vertical * 18.0F);
                output.rgba8[index + 2U] = clamp_u8(255.0F - vertical * 10.0F);
                output.rgba8[index + 3U] = clamp_u8(alpha * 245.0F);
            }
        }

        return output;
    }

    [[nodiscard]] assets::layout::tpl::DecodedImage make_opaque_texture(assets::layout::tpl::DecodedImage image) {
        for (std::size_t index = 0U; index + 3U < image.rgba8.size(); index += 4U) {
            image.rgba8[index + 3U] = 255U;
        }

        return image;
    }

    std::vector< assets::layout::tpl::DecodedImage > load_planet_textures() {
        const compat::RuntimeAssetLoaderScope asset_loader{};
        if (!asset_loader) {
            return {};
        }

        constexpr std::string_view archive_path = "/ObjectData/FileSelectDataPlanet.arc";
        constexpr std::string_view model_path = "fileselectdataplanet.bdl";

        constexpr std::size_t FRAME_COUNT = 32U;
        const float pitch_degrees = environment_float_or("SMGPC_FILE_SELECT_PLANET_PITCH", -8.0F);
        const float margin = environment_float_or("SMGPC_FILE_SELECT_PLANET_MARGIN", 0.88F);
        const float ambient_light = environment_float_or("SMGPC_FILE_SELECT_PLANET_AMBIENT", 0.80F);
        const float diffuse_light = environment_float_or("SMGPC_FILE_SELECT_PLANET_DIFFUSE", 0.0F);
        const float color_scale_r = environment_float_or("SMGPC_FILE_SELECT_PLANET_R", 0.64F);
        const float color_scale_g = environment_float_or("SMGPC_FILE_SELECT_PLANET_G", 1.04F);
        const float color_scale_b = environment_float_or("SMGPC_FILE_SELECT_PLANET_B", 1.44F);
        std::vector< assets::layout::tpl::DecodedImage > frames{};
        frames.reserve(FRAME_COUNT);
        for (std::size_t i = 0U; i < FRAME_COUNT; ++i) {
            const auto thumbnail = asset_loader->j3d_thumbnail(
                archive_path, model_path,
                assets::layout::J3dThumbnailOptions{.width = 192U,
                                                    .height = 192U,
                                                    .yaw_degrees = static_cast< float >(i) * (360.0F / static_cast< float >(FRAME_COUNT)),
                                                    .pitch_degrees = pitch_degrees,
                                                    .margin = margin,
                                                    .ambient_light = ambient_light,
                                                    .diffuse_light = diffuse_light,
                                                    .color_scale_r = color_scale_r,
                                                    .color_scale_g = color_scale_g,
                                                    .color_scale_b = color_scale_b});
            if (thumbnail.has_value() && !thumbnail->empty()) {
                frames.push_back(*thumbnail);
            }
        }
        if (!frames.empty()) {
            return frames;
        }

        const auto textures = asset_loader->j3d_tex1_textures(archive_path, model_path);
        if (!textures.has_value() || textures->empty() || textures->front().image.empty()) {
            return {};
        }

        frames.push_back(make_planet_impostor_texture(textures->front().image));
        return frames;
    }

    SkyTextures load_sky_textures(bool includeLiveJ3dSky, bool liveJ3dSkyDefault, std::string_view liveJ3dEnvironment) {
        const compat::RuntimeAssetLoaderScope asset_loader{};
        if (!asset_loader) {
            return {};
        }

        constexpr std::string_view archive_path = "/ObjectData/CometNearOrbitSky.arc";
        const auto bdl_entry = asset_loader->archive_entry(archive_path, "cometnearorbitsky.bdl");
        if (!bdl_entry.has_value()) {
            return {};
        }
        const auto textures = asset_loader->j3d_tex1_textures(archive_path, "cometnearorbitsky.bdl");
        if (!textures.has_value()) {
            return {};
        }

        SkyTextures output{};
        const auto live_sky_environment = get_optional_bool_from_environment(std::string(liveJ3dEnvironment).c_str());
        const bool load_live_sky = includeLiveJ3dSky && live_sky_environment.value_or(liveJ3dSkyDefault);
        if (load_live_sky) {
            const auto bck_entry = asset_loader->archive_entry(archive_path, "cometnearorbitsky.bck");
            const auto btk_entry = asset_loader->archive_entry(archive_path, "cometnearorbitsky.btk");
            const auto bck_bytes = bck_entry.has_value() ? bck_entry->bytes : std::span< const std::byte >{};
            const auto btk_bytes = btk_entry.has_value() ? btk_entry->bytes : std::span< const std::byte >{};
            auto sky = compat::FileSelectSkyJ3d::parse(bdl_entry->bytes, bck_bytes, btk_bytes);
            if (sky && !sky->empty()) {
                output.j3d_sky = std::move(*sky);
            }
        }
        if (get_bool_from_environment("SMGPC_FILE_SELECT_J3D_SKY")) {
            const auto model_snapshot = asset_loader->j3d_thumbnail(archive_path, "cometnearorbitsky.bdl",
                                                                   assets::layout::J3dThumbnailOptions{.width = 836U,
                                                                                                       .height = 456U,
                                                                                                       .yaw_degrees = 0.0F,
                                                                                                       .pitch_degrees = -8.0F,
                                                                                                       .margin = 1.0F,
                                                                                                       .ambient_light = 0.62F,
                                                                                                       .diffuse_light = 0.18F,
                                                                                                       .color_scale_r = 0.95F,
                                                                                                       .color_scale_g = 1.05F,
                                                                                                       .color_scale_b = 1.18F});
            if (model_snapshot.has_value() && !model_snapshot->empty()) {
                output.model_snapshot = *model_snapshot;
            }
        }
        for (const auto& texture : *textures) {
            if (texture.name == "OrbitUniverseL" && !texture.image.empty()) {
                output.star_field = make_sky_star_texture(texture.image);
            } else if (texture.name == "AstroPlanetWall" && !texture.image.empty()) {
                output.bottom_haze = make_sky_haze_texture(texture.image);
                output.title_planet_surface = make_title_planet_surface_texture(texture.image);
            } else if (texture.name == "GalaxyRiverK" && !texture.image.empty()) {
                output.nebula = make_sky_nebula_texture(texture.image);
            } else if (texture.name == "CometHalo" && !texture.image.empty()) {
                output.comet_halo = make_sky_comet_halo_texture(texture.image);
            }
        }

        return output;
    }

    MiiSelectTextures load_mii_select_textures() {
        const compat::RuntimeAssetLoaderScope asset_loader{};
        if (!asset_loader) {
            return {};
        }

        auto textures = MiiSelectTextures{
            .sys_bg = asset_loader->tpl_image("/LayoutData/MiiSelect.arc", "timg/mysysbg.tpl"),
            .page_window = asset_loader->tpl_image("/LayoutData/MiiSelect.arc", "timg/mymiipagewindow.tpl"),
        };
        if (textures.sys_bg.has_value()) {
            textures.sys_bg = make_opaque_texture(std::move(*textures.sys_bg));
        }

        return textures;
    }

    PointerTextures load_pointer_textures() {
        const compat::RuntimeAssetLoaderScope asset_loader{};
        if (!asset_loader) {
            return {};
        }

        return PointerTextures{
            .hand = asset_loader->tpl_image("/LayoutData/DPDPointer.arc", "timg/handpointerpoint.tpl"),
            .hand_shadow = asset_loader->tpl_image("/LayoutData/DPDPointer.arc", "timg/handpointerpointshadow.tpl"),
            .star = asset_loader->tpl_image("/LayoutData/DPDPointer.arc", "timg/handpointerstarp.tpl"),
        };
    }

    FellowIconTextures load_fellow_icon_textures() {
        const compat::RuntimeAssetLoaderScope asset_loader{};
        if (!asset_loader) {
            return {};
        }

        FellowIconTextures output{};
        const auto model_options = assets::layout::J3dThumbnailOptions{.width = 128U,
                                                                       .height = 128U,
                                                                       .yaw_degrees = environment_float_or("SMGPC_FILE_SELECT_FELLOW_MODEL_YAW", 0.0F),
                                                                       .pitch_degrees = environment_float_or("SMGPC_FILE_SELECT_FELLOW_MODEL_PITCH", -8.0F),
                                                                       .margin = environment_float_or("SMGPC_FILE_SELECT_FELLOW_MODEL_MARGIN", 0.92F),
                                                                       .ambient_light = environment_float_or("SMGPC_FILE_SELECT_FELLOW_MODEL_AMBIENT", 0.80F),
                                                                       .diffuse_light = environment_float_or("SMGPC_FILE_SELECT_FELLOW_MODEL_DIFFUSE", 0.20F),
                                                                       .color_scale_r = environment_float_or("SMGPC_FILE_SELECT_FELLOW_MODEL_R", 1.0F),
                                                                       .color_scale_g = environment_float_or("SMGPC_FILE_SELECT_FELLOW_MODEL_G", 1.0F),
                                                                       .color_scale_b = environment_float_or("SMGPC_FILE_SELECT_FELLOW_MODEL_B", 1.0F)};
        output.fellow_models[static_cast< std::size_t >(FileSelectIconID::Mario)] =
            asset_loader->j3d_thumbnail("/ObjectData/FileSelectDataMario.arc", "fileselectdatamario.bdl", model_options);
        output.fellow_models[static_cast< std::size_t >(FileSelectIconID::Luigi)] =
            asset_loader->j3d_thumbnail("/ObjectData/FileSelectDataLuigi.arc", "fileselectdataluigi.bdl", model_options);
        output.fellow_models[static_cast< std::size_t >(FileSelectIconID::Yoshi)] =
            asset_loader->j3d_thumbnail("/ObjectData/FileSelectDataYoshi.arc", "fileselectdatayoshi.bdl", model_options);
        output.fellow_models[static_cast< std::size_t >(FileSelectIconID::Kinopio)] =
            asset_loader->j3d_thumbnail("/ObjectData/FileSelectDataKinopio.arc", "fileselectdatakinopio.bdl", model_options);
        output.fellow_models[static_cast< std::size_t >(FileSelectIconID::Peach)] =
            asset_loader->j3d_thumbnail("/ObjectData/FileSelectDataPeach.arc", "fileselectdatapeach.bdl", model_options);
        output.fellows[static_cast< std::size_t >(FileSelectIconID::Mario)] =
            asset_loader->tpl_image("/LayoutData/MiiIcon.arc", "timg/mymiimario.tpl");
        output.fellows[static_cast< std::size_t >(FileSelectIconID::Luigi)] =
            asset_loader->tpl_image("/LayoutData/MiiIcon.arc", "timg/mymiiluigi.tpl");
        output.fellows[static_cast< std::size_t >(FileSelectIconID::Yoshi)] =
            asset_loader->tpl_image("/LayoutData/MiiIcon.arc", "timg/mymiiyoshi.tpl");
        output.fellows[static_cast< std::size_t >(FileSelectIconID::Kinopio)] =
            asset_loader->tpl_image("/LayoutData/MiiIcon.arc", "timg/mymiikinopio.tpl");
        output.fellows[static_cast< std::size_t >(FileSelectIconID::Peach)] =
            asset_loader->tpl_image("/LayoutData/MiiIcon.arc", "timg/mymiipeach.tpl");
        output.mii_placeholder = asset_loader->tpl_image("/LayoutData/MiiIcon.arc", "timg/mymiiquestion.tpl");
        return output;
    }

}  // namespace smgpc::game::file_select_preview
