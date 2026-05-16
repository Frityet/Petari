#pragma once

#include <array>
#include <optional>
#include <string_view>
#include <vector>

#include "compat/FileSelectSkyJ3d.hpp"
#include "layout/Tpl.hpp"

namespace smgpc::game::file_select_preview {

    struct SkyTextures {
        std::optional< compat::FileSelectSkyJ3d > j3d_sky{};
        std::optional< assets::layout::tpl::DecodedImage > model_snapshot{};
        std::optional< assets::layout::tpl::DecodedImage > star_field{};
        std::optional< assets::layout::tpl::DecodedImage > bottom_haze{};
        std::optional< assets::layout::tpl::DecodedImage > title_planet_surface{};
        std::optional< assets::layout::tpl::DecodedImage > nebula{};
        std::optional< assets::layout::tpl::DecodedImage > comet_halo{};
    };

    struct PointerTextures {
        std::optional< assets::layout::tpl::DecodedImage > hand{};
        std::optional< assets::layout::tpl::DecodedImage > hand_shadow{};
        std::optional< assets::layout::tpl::DecodedImage > star{};
    };

    struct FellowIconTextures {
        std::array< std::optional< assets::layout::tpl::DecodedImage >, 5 > fellows{};
        std::array< std::optional< assets::layout::tpl::DecodedImage >, 5 > fellow_models{};
        std::optional< assets::layout::tpl::DecodedImage > mii_placeholder{};
    };

    struct MiiSelectTextures {
        std::optional< assets::layout::tpl::DecodedImage > sys_bg{};
        std::optional< assets::layout::tpl::DecodedImage > page_window{};
    };

    [[nodiscard]] assets::layout::tpl::DecodedImage make_file_number_badge_texture();
    [[nodiscard]] assets::layout::tpl::DecodedImage make_prompt_pill_texture();
    [[nodiscard]] assets::layout::tpl::DecodedImage make_page_counter_pill_texture();
    [[nodiscard]] std::vector< assets::layout::tpl::DecodedImage > load_planet_textures();
    [[nodiscard]] SkyTextures load_sky_textures(bool includeLiveJ3dSky = true, bool liveJ3dSkyDefault = false,
                                                std::string_view liveJ3dEnvironment = "SMGPC_FILE_SELECT_LIVE_J3D_SKY");
    [[nodiscard]] MiiSelectTextures load_mii_select_textures();
    [[nodiscard]] PointerTextures load_pointer_textures();
    [[nodiscard]] FellowIconTextures load_fellow_icon_textures();

}  // namespace smgpc::game::file_select_preview
