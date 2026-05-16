#include "compat/TitleBackground.hpp"

#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <string_view>

#include "Game/Screen/LayoutActor.hpp"
#include "compat/RuntimeAssetLoader.hpp"
#include "compat/SharedSkyBackground.hpp"
#include "layout/LayoutArchiveLoader.hpp"

namespace smgpc::game::compat {
    namespace {

        constexpr float TITLE_WIDTH = 836.0F;
        constexpr float TITLE_HEIGHT = 456.0F;

        [[nodiscard]] std::optional< float > get_float_from_environment(const char* name) {
            const char* value = std::getenv(name);
            if (value == nullptr || value[0] == '\0') {
                return std::nullopt;
            }

            char* end_pointer = nullptr;
            const float parsed_value = std::strtof(value, &end_pointer);
            if (end_pointer == value || *end_pointer != '\0' || !std::isfinite(parsed_value)) {
                return std::nullopt;
            }

            return parsed_value;
        }

        [[nodiscard]] float environment_float_or(const char* name, float fallback) {
            if (const auto value = get_float_from_environment(name)) {
                return *value;
            }

            return fallback;
        }

        [[nodiscard]] std::optional< bool > get_optional_bool_from_environment(const char* name) {
            const char* value = std::getenv(name);
            if (value == nullptr || value[0] == '\0') {
                return std::nullopt;
            }

            return std::string_view(value) != "0" && std::string_view(value) != "false" && std::string_view(value) != "False" &&
                   std::string_view(value) != "FALSE";
        }

        [[nodiscard]] bool should_draw_title_live_j3d_sky() {
            if (const auto value = get_optional_bool_from_environment("SMGPC_TITLE_LIVE_J3D_SKY")) {
                return *value;
            }

            return false;
        }

        [[nodiscard]] bool should_draw_title_world_j3d_sky() {
            if (const auto value = get_optional_bool_from_environment("SMGPC_TITLE_WORLD_J3D_SKY")) {
                return *value;
            }

            return false;
        }

        [[nodiscard]] bool should_draw_title_extra_background(bool has_world_j3d_sky) {
            if (const auto value = get_optional_bool_from_environment("SMGPC_TITLE_EXTRA_BACKGROUND")) {
                return *value;
            }

            return !has_world_j3d_sky;
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

        [[nodiscard]] render::layout::TextureRef texture_ref(const assets::layout::tpl::DecodedImage& image, std::uint8_t wrap_s = 0U,
                                                             std::uint8_t wrap_t = 0U) {
            return render::layout::TextureRef{
                .id = static_cast< std::uint64_t >(reinterpret_cast< std::uintptr_t >(&image)),
                .rgba8 = image.rgba8.data(),
                .width = image.width,
                .height = image.height,
                .wrap_s = wrap_s,
                .wrap_t = wrap_t,
            };
        }

        void push_quad(render::layout::LayoutDrawList* pDrawList, float x0, float y0, float x1, float y1, std::uint32_t color_tl,
                       std::uint32_t color_tr, std::uint32_t color_bl, std::uint32_t color_br,
                       render::layout::BlendMode blend_mode = render::layout::BlendMode::Alpha) {
            pDrawList->push_quad(render::layout::QuadCommand{
                .x0 = x0,
                .y0 = y0,
                .x1 = x1,
                .y1 = y1,
                .coordinate_width = TITLE_WIDTH,
                .coordinate_height = TITLE_HEIGHT,
                .u0 = 0.0F,
                .v0 = 0.0F,
                .u1 = 1.0F,
                .v1 = 1.0F,
                .color_tl = color_tl,
                .color_tr = color_tr,
                .color_bl = color_bl,
                .color_br = color_br,
                .blend_mode = blend_mode,
            });
        }

        void push_textured_quad(render::layout::LayoutDrawList* pDrawList, const assets::layout::tpl::DecodedImage& image, float x0, float y0,
                                float x1, float y1, float u0, float v0, float u1, float v1, std::uint8_t alpha,
                                render::layout::BlendMode blend_mode = render::layout::BlendMode::Alpha, std::uint8_t wrap_s = 0U,
                                std::uint8_t wrap_t = 0U) {
            pDrawList->push_quad(render::layout::QuadCommand{
                .x0 = x0,
                .y0 = y0,
                .x1 = x1,
                .y1 = y1,
                .coordinate_width = TITLE_WIDTH,
                .coordinate_height = TITLE_HEIGHT,
                .u0 = u0,
                .v0 = v0,
                .u1 = u1,
                .v1 = v1,
                .color_tl = render::layout::pack_abgr(255U, 255U, 255U, alpha),
                .color_tr = render::layout::pack_abgr(255U, 255U, 255U, alpha),
                .color_bl = render::layout::pack_abgr(255U, 255U, 255U, alpha),
                .color_br = render::layout::pack_abgr(255U, 255U, 255U, alpha),
                .blend_mode = blend_mode,
                .texture = texture_ref(image, wrap_s, wrap_t),
            });
        }

        void push_masked_textured_quad(render::layout::LayoutDrawList* pDrawList, const assets::layout::tpl::DecodedImage& image,
                                       const assets::layout::tpl::DecodedImage& mask, float x0, float y0, float x1, float y1, float u0, float v0, float u1,
                                       float v1, std::uint8_t alpha) {
            pDrawList->push_quad(render::layout::QuadCommand{
                .x0 = x0,
                .y0 = y0,
                .x1 = x1,
                .y1 = y1,
                .coordinate_width = TITLE_WIDTH,
                .coordinate_height = TITLE_HEIGHT,
                .u0 = u0,
                .v0 = v0,
                .u1 = u1,
                .v1 = v1,
                .u0_secondary = 0.0F,
                .v0_secondary = 0.0F,
                .u1_secondary = 1.0F,
                .v1_secondary = 1.0F,
                .color_tl = render::layout::pack_abgr(255U, 255U, 255U, alpha),
                .color_tr = render::layout::pack_abgr(255U, 255U, 255U, alpha),
                .color_bl = render::layout::pack_abgr(255U, 255U, 255U, alpha),
                .color_br = render::layout::pack_abgr(255U, 255U, 255U, alpha),
                .blend_mode = render::layout::BlendMode::Alpha,
                .use_mask_texture = true,
                .invert_mask = true,
                .mask_uses_alpha = false,
                .texture = texture_ref(image, 1U, 1U),
                .mask_texture = texture_ref(mask),
            });
        }

        void push_title_horizon_glow(render::layout::LayoutDrawList* pDrawList, float alpha_scale) {
            if (alpha_scale <= 0.0F) {
                return;
            }

            const std::uint8_t upper_alpha = clamp_u8(118.0F * alpha_scale);
            const std::uint8_t lower_alpha = clamp_u8(156.0F * alpha_scale);
            const std::uint8_t fade_alpha = clamp_u8(72.0F * alpha_scale);

            push_quad(pDrawList, 0.0F, 238.0F, TITLE_WIDTH, 276.0F, render::layout::pack_abgr(80U, 214U, 230U, 0U),
                      render::layout::pack_abgr(80U, 214U, 230U, 0U), render::layout::pack_abgr(96U, 235U, 235U, upper_alpha),
                      render::layout::pack_abgr(96U, 235U, 235U, upper_alpha));
            push_quad(pDrawList, 0.0F, 276.0F, TITLE_WIDTH, 306.0F, render::layout::pack_abgr(76U, 238U, 225U, lower_alpha),
                      render::layout::pack_abgr(76U, 238U, 225U, lower_alpha), render::layout::pack_abgr(36U, 146U, 158U, fade_alpha),
                      render::layout::pack_abgr(36U, 146U, 158U, fade_alpha));
            push_quad(pDrawList, 0.0F, 306.0F, TITLE_WIDTH, 338.0F, render::layout::pack_abgr(34U, 140U, 152U, fade_alpha),
                      render::layout::pack_abgr(34U, 140U, 152U, fade_alpha), render::layout::pack_abgr(10U, 65U, 88U, 0U),
                      render::layout::pack_abgr(10U, 65U, 88U, 0U));
        }

        void push_title_nebula(render::layout::LayoutDrawList* pDrawList, const assets::layout::tpl::DecodedImage& image, float alpha_scale) {
            if (alpha_scale <= 0.0F) {
                return;
            }

            pDrawList->push_quad(render::layout::QuadCommand{
                .x0 = 254.0F,
                .y0 = -72.0F,
                .x1 = 916.0F,
                .y1 = 262.0F,
                .coordinate_width = TITLE_WIDTH,
                .coordinate_height = TITLE_HEIGHT,
                .u0 = 0.0F,
                .v0 = 0.0F,
                .u1 = 1.0F,
                .v1 = 1.0F,
                .color_tl = render::layout::pack_abgr(255U, 255U, 255U, clamp_u8(255.0F * alpha_scale)),
                .color_tr = render::layout::pack_abgr(255U, 255U, 255U, clamp_u8(255.0F * alpha_scale)),
                .color_bl = render::layout::pack_abgr(255U, 255U, 255U, clamp_u8(178.0F * alpha_scale)),
                .color_br = render::layout::pack_abgr(255U, 255U, 255U, clamp_u8(178.0F * alpha_scale)),
                .blend_mode = render::layout::BlendMode::Additive,
                .texture = texture_ref(image),
            });
        }

        void push_title_lower_shadow(render::layout::LayoutDrawList* pDrawList, float alpha_scale) {
            if (alpha_scale <= 0.0F) {
                return;
            }

            const std::uint8_t upper_alpha = clamp_u8(92.0F * alpha_scale);
            const std::uint8_t middle_alpha = clamp_u8(134.0F * alpha_scale);

            push_quad(pDrawList, 0.0F, 328.0F, TITLE_WIDTH, 386.0F, render::layout::pack_abgr(0U, 12U, 28U, 0U),
                      render::layout::pack_abgr(0U, 12U, 28U, 0U), render::layout::pack_abgr(0U, 10U, 24U, upper_alpha),
                      render::layout::pack_abgr(0U, 10U, 24U, upper_alpha));
            push_quad(pDrawList, 0.0F, 386.0F, TITLE_WIDTH, 424.0F, render::layout::pack_abgr(0U, 8U, 20U, middle_alpha),
                      render::layout::pack_abgr(0U, 8U, 20U, middle_alpha), render::layout::pack_abgr(8U, 24U, 40U, 0U),
                      render::layout::pack_abgr(8U, 24U, 40U, 0U));
        }

        void push_title_bottom_cloud(render::layout::LayoutDrawList* pDrawList, float alpha_scale) {
            if (alpha_scale <= 0.0F) {
                return;
            }

            const std::uint8_t top_alpha = clamp_u8(76.0F * alpha_scale);
            const std::uint8_t bottom_alpha = clamp_u8(176.0F * alpha_scale);

            push_quad(pDrawList, 0.0F, 404.0F, TITLE_WIDTH, TITLE_HEIGHT, render::layout::pack_abgr(86U, 108U, 134U, 0U),
                      render::layout::pack_abgr(86U, 108U, 134U, 0U), render::layout::pack_abgr(130U, 146U, 170U, bottom_alpha),
                      render::layout::pack_abgr(130U, 146U, 170U, bottom_alpha));
            push_quad(pDrawList, 0.0F, 420.0F, TITLE_WIDTH, TITLE_HEIGHT, render::layout::pack_abgr(72U, 96U, 122U, top_alpha),
                      render::layout::pack_abgr(72U, 96U, 122U, top_alpha), render::layout::pack_abgr(180U, 194U, 214U, clamp_u8(112.0F * alpha_scale)),
                      render::layout::pack_abgr(180U, 194U, 214U, clamp_u8(112.0F * alpha_scale)));
        }

    }  // namespace

    TitleBackground::TitleBackground()
        : _skyTextures(file_select_preview::load_sky_textures(true, false, "SMGPC_TITLE_WORLD_J3D_SKY")) {
        const RuntimeAssetLoaderScope asset_loader{};
        if (!asset_loader) {
            return;
        }

        constexpr std::string_view TITLE_LOGO_ARCHIVE = "/LayoutData/TitleLogo.arc";
        _titleSpace = asset_loader->tpl_image(TITLE_LOGO_ARCHIVE, "timg/mytitlespacekor.tpl");
        _titleMask = asset_loader->tpl_image(TITLE_LOGO_ARCHIVE, "timg/mytitlemaskkor.tpl");
    }

	    void TitleBackground::appendDrawCommands(render::layout::LayoutDrawList* pDrawList, std::uint64_t frame) const {
	        if (pDrawList == nullptr) {
	            return;
	        }

        const bool has_world_j3d_sky = should_draw_title_world_j3d_sky() && _skyTextures.j3d_sky.has_value();
        if (!has_world_j3d_sky) {
	        append_shared_sky_background_draw_commands(pDrawList, _skyTextures, frame);
        }
        if (should_draw_title_live_j3d_sky() && _skyTextures.j3d_sky.has_value()) {
            _skyTextures.j3d_sky->appendDrawCommands(pDrawList, static_cast< float >(frame), 0U, FileSelectSkyCameraMode::Title);
        }
	        if (!should_draw_title_extra_background(has_world_j3d_sky)) {
	            return;
	        }

	        if (_skyTextures.nebula.has_value() && !_skyTextures.nebula->empty()) {
	            push_title_nebula(pDrawList, *_skyTextures.nebula, environment_float_or("SMGPC_TITLE_NEBULA_ALPHA_SCALE", 0.55F));
	        }

        push_title_horizon_glow(pDrawList, environment_float_or("SMGPC_TITLE_HORIZON_GLOW_ALPHA_SCALE", 2.20F));
        if (_skyTextures.title_planet_surface.has_value() && !_skyTextures.title_planet_surface->empty()) {
            push_textured_quad(pDrawList, *_skyTextures.title_planet_surface, 0.0F, 286.0F, TITLE_WIDTH, TITLE_HEIGHT, 0.0F,
                               environment_float_or("SMGPC_TITLE_PLANET_SURFACE_V0", 0.0F), 1.0F,
                               environment_float_or("SMGPC_TITLE_PLANET_SURFACE_V1", 1.0F),
                               clamp_u8(255.0F * environment_float_or("SMGPC_TITLE_PLANET_SURFACE_ALPHA_SCALE", 1.0F)));
        }
        push_title_lower_shadow(pDrawList, environment_float_or("SMGPC_TITLE_BOTTOM_SHADOW_ALPHA_SCALE", 1.0F));

	        push_title_bottom_cloud(pDrawList, environment_float_or("SMGPC_TITLE_BOTTOM_CLOUD_ALPHA_SCALE", 0.0F));
	    }

    void TitleBackground::appendJ3dDrawCommands(render::core::RenderCommandBuffer* pCommands, std::uint64_t frame, std::uint16_t framebufferWidth,
                                                std::uint16_t framebufferHeight) const {
        if (pCommands == nullptr || !should_draw_title_world_j3d_sky() || !_skyTextures.j3d_sky.has_value()) {
            return;
        }

        _skyTextures.j3d_sky->appendJ3dDrawCommands(pCommands, static_cast< float >(frame), framebufferWidth, framebufferHeight, 0U,
                                                    FileSelectSkyCameraMode::Title);
    }

    void TitleBackground::appendLogoOverlayDrawCommands(render::layout::LayoutDrawList* pDrawList, const LayoutActor* pLogoLayout, std::uint64_t frame) const {
        if (pDrawList == nullptr || !_titleSpace.has_value() || !_titleMask.has_value() || _titleSpace->empty() || _titleMask->empty()) {
            return;
        }

        float x0 = 183.0F;
        float y0 = 157.0F;
        float x1 = 637.0F;
        float y1 = 253.0F;
        if (pLogoLayout != nullptr && !pLogoLayout->isDead()) {
            float pane_x0{};
            float pane_y0{};
            float pane_x1{};
            float pane_y1{};
            const auto* resource = pLogoLayout->getResource();
            if (resource != nullptr && resource->layout.size.x > 0.0F && resource->layout.size.y > 0.0F &&
                pLogoLayout->getPaneBounds("PicLogoGalaxy", &pane_x0, &pane_y0, &pane_x1, &pane_y1)) {
                const float scale_x = TITLE_WIDTH / resource->layout.size.x;
                const float scale_y = TITLE_HEIGHT / resource->layout.size.y;
                x0 = pane_x0 * scale_x;
                y0 = pane_y0 * scale_y;
                x1 = pane_x1 * scale_x;
                y1 = pane_y1 * scale_y;
            }
        }

        // PC compatibility overlay for TitleLogo.arc PicLogoGalaxy. The original sequence starts Wait after Appear stops; only Wait owns the RLTS scroll.
        const float wait_frame_max = pLogoLayout != nullptr ? pLogoLayout->getAnimFrameMax("Wait") : 0.0F;
        const bool is_wait_animation = wait_frame_max > 0.0F && std::fabs((pLogoLayout != nullptr ? pLogoLayout->getAnimFrameMax(0U) : 0.0F) - wait_frame_max) < 0.001F;
        if (!is_wait_animation) {
            return;
        }

        const float wait_frame = pLogoLayout != nullptr ? pLogoLayout->getAnimFrame(0U) : 0.0F;
        const float galaxy_scroll_u = std::fmod(wait_frame * environment_float_or("SMGPC_TITLE_LOGO_GALAXY_SCROLL_U", 0.0001F), 1.0F);
        const float galaxy_scroll_v = std::fmod(wait_frame * environment_float_or("SMGPC_TITLE_LOGO_GALAXY_SCROLL_V", 0.0F), 1.0F);
        push_masked_textured_quad(pDrawList, *_titleSpace, *_titleMask, x0, y0, x1, y1, galaxy_scroll_u, galaxy_scroll_v, galaxy_scroll_u + 1.0F,
                                  galaxy_scroll_v + 1.0F, 255U);
    }

}  // namespace smgpc::game::compat
