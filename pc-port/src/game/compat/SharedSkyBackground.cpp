#include "compat/SharedSkyBackground.hpp"

#include <cmath>
#include <cstdlib>
#include <optional>

#include "layout/LayoutDrawList.hpp"

namespace smgpc::game::compat {
    namespace {

        constexpr float SKY_REFERENCE_WIDTH = 836.0F;
        constexpr float SKY_REFERENCE_HEIGHT = 456.0F;

        [[nodiscard]] std::optional< float > get_float_from_environment(const char *name) {
            const char *value = std::getenv(name);
            if (value == nullptr || value[0] == '\0') {
                return std::nullopt;
            }

            char *end_pointer = nullptr;
            const float parsed_value = std::strtof(value, &end_pointer);
            if (end_pointer == value || *end_pointer != '\0' || !std::isfinite(parsed_value)) {
                return std::nullopt;
            }

            return parsed_value;
        }

        [[nodiscard]] float environment_float_or(const char *name, float fallback) {
            if (const auto value = get_float_from_environment(name)) {
                return *value;
            }

            return fallback;
        }

        [[nodiscard]] float environment_float_or(const char *name, const char *legacy_name, float fallback) {
            if (const auto value = get_float_from_environment(name)) {
                return *value;
            }
            if (const auto value = get_float_from_environment(legacy_name)) {
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

        [[nodiscard]] render::layout::TextureRef texture_ref(const assets::layout::tpl::DecodedImage &image, std::uint8_t wrap_s = 0U,
                                                             std::uint8_t wrap_t = 0U) {
            return render::layout::TextureRef {
                .id = static_cast< std::uint64_t >(reinterpret_cast< std::uintptr_t >(&image)),
                .rgba8 = image.rgba8.data(),
                .width = image.width,
                .height = image.height,
                .wrap_s = wrap_s,
                .wrap_t = wrap_t,
            };
        }

        void push_reference_quad(render::layout::LayoutDrawList *pDrawList, render::layout::QuadCommand command) {
            command.coordinate_width = SKY_REFERENCE_WIDTH;
            command.coordinate_height = SKY_REFERENCE_HEIGHT;
            pDrawList->push_quad(command);
        }

    }  // namespace

    void append_shared_sky_background_draw_commands(render::layout::LayoutDrawList *pDrawList, const file_select_preview::SkyTextures &textures,
                                                    std::uint64_t frame, const SharedSkyBackgroundOptions &options) {
        if (pDrawList == nullptr) {
            return;
        }

        constexpr float ORIGINAL_SKY_YAW_RADIANS_PER_FRAME = 0.001F;
        constexpr float TWO_PI = 6.28318530717958647692F;
        const float scroll_u_per_frame = environment_float_or("SMGPC_SHARED_SKY_SCROLL_U", ORIGINAL_SKY_YAW_RADIANS_PER_FRAME / TWO_PI);
        const float scroll_v_per_frame = environment_float_or("SMGPC_SHARED_SKY_SCROLL_V", 0.0F);
        const float star_scroll_u = std::fmod(static_cast< float >(frame) * scroll_u_per_frame, 1.0F);
        const float star_scroll_v = std::fmod(static_cast< float >(frame) * scroll_v_per_frame, 1.0F);
        const float star_u_scale = environment_float_or("SMGPC_SHARED_SKY_STAR_U_SCALE", "SMGPC_FILE_SELECT_STAR_U_SCALE", 0.816F);
        const float star_v_scale = environment_float_or("SMGPC_SHARED_SKY_STAR_V_SCALE", "SMGPC_FILE_SELECT_STAR_V_SCALE", 0.891F);
        const float star_alpha_scale = environment_float_or("SMGPC_SHARED_SKY_STAR_ALPHA_SCALE", "SMGPC_FILE_SELECT_STAR_ALPHA_SCALE", 0.82F);
        const float bottom_haze_alpha_scale =
            environment_float_or("SMGPC_SHARED_SKY_BOTTOM_HAZE_ALPHA_SCALE", "SMGPC_FILE_SELECT_BOTTOM_HAZE_ALPHA_SCALE", 0.85F);

        push_reference_quad(pDrawList, render::layout::QuadCommand {
                                       .x0 = 0.0F,
                                       .y0 = 0.0F,
                                       .x1 = SKY_REFERENCE_WIDTH,
                                       .y1 = SKY_REFERENCE_HEIGHT,
                                       .u0 = 0.0F,
                                       .v0 = 0.0F,
                                       .u1 = 1.0F,
                                       .v1 = 1.0F,
                                       .color_tl = render::layout::pack_abgr(4U, 67U, 98U, 255U),
                                       .color_tr = render::layout::pack_abgr(4U, 67U, 98U, 255U),
                                       .color_bl = render::layout::pack_abgr(0U, 38U, 60U, 255U),
                                       .color_br = render::layout::pack_abgr(0U, 38U, 60U, 255U),
                                   });

        if (textures.star_field.has_value() && !textures.star_field->empty()) {
            push_reference_quad(pDrawList, render::layout::QuadCommand {
                                               .x0 = 0.0F,
                                               .y0 = 0.0F,
                                               .x1 = SKY_REFERENCE_WIDTH,
                                               .y1 = SKY_REFERENCE_HEIGHT,
                                               .u0 = star_scroll_u,
                                               .v0 = star_scroll_v,
                                               .u1 = star_scroll_u + star_u_scale,
                                               .v1 = star_scroll_v + star_v_scale,
                                               .color_tl = render::layout::pack_abgr(255U, 255U, 255U, clamp_u8(255.0F * star_alpha_scale)),
                                               .color_tr = render::layout::pack_abgr(255U, 255U, 255U, clamp_u8(255.0F * star_alpha_scale)),
                                               .color_bl = render::layout::pack_abgr(255U, 255U, 255U, clamp_u8(255.0F * star_alpha_scale)),
                                               .color_br = render::layout::pack_abgr(255U, 255U, 255U, clamp_u8(255.0F * star_alpha_scale)),
                                               .blend_mode = render::layout::BlendMode::Alpha,
                                               .texture = texture_ref(*textures.star_field, 1U, 1U),
                                           });
        }

        if (options.show_bottom_haze && textures.bottom_haze.has_value() && !textures.bottom_haze->empty()) {
            push_reference_quad(pDrawList, render::layout::QuadCommand {
                                               .x0 = 0.0F,
                                               .y0 = 416.0F,
                                               .x1 = SKY_REFERENCE_WIDTH,
                                               .y1 = SKY_REFERENCE_HEIGHT,
                                               .u0 = 0.0F,
                                               .v0 = 0.0F,
                                               .u1 = 1.0F,
                                               .v1 = 1.0F,
                                               .color_tl = render::layout::pack_abgr(255U, 255U, 255U, 0U),
                                               .color_tr = render::layout::pack_abgr(255U, 255U, 255U, 0U),
                                               .color_bl = render::layout::pack_abgr(255U, 255U, 255U, clamp_u8(255.0F * bottom_haze_alpha_scale)),
                                               .color_br = render::layout::pack_abgr(255U, 255U, 255U, clamp_u8(255.0F * bottom_haze_alpha_scale)),
                                               .blend_mode = render::layout::BlendMode::Alpha,
                                               .texture = texture_ref(*textures.bottom_haze),
                                           });
        } else if (options.show_bottom_haze) {
            push_reference_quad(pDrawList, render::layout::QuadCommand {
                                               .x0 = 0.0F,
                                               .y0 = 416.0F,
                                               .x1 = SKY_REFERENCE_WIDTH,
                                               .y1 = SKY_REFERENCE_HEIGHT,
                                               .u0 = 0.0F,
                                               .v0 = 0.0F,
                                               .u1 = 1.0F,
                                               .v1 = 1.0F,
                                               .color_tl = render::layout::pack_abgr(55U, 166U, 188U, 210U),
                                               .color_tr = render::layout::pack_abgr(55U, 166U, 188U, 210U),
                                               .color_bl = render::layout::pack_abgr(128U, 223U, 210U, 255U),
                                               .color_br = render::layout::pack_abgr(128U, 223U, 210U, 255U),
                                           });
        }
    }

}  // namespace smgpc::game::compat
