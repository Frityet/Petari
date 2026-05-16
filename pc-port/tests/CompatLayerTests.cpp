#include "Game/Map/FileSelector.hpp"
#include "Game/Util/ActorSensorUtil.hpp"
#include "Game/compat/BcsvTable.hpp"
#include "Game/compat/BrfntFont.hpp"
#include "Game/compat/BrlanAnimation.hpp"
#include "Game/compat/BrlytLayout.hpp"
#include "Game/compat/CameraParam.hpp"
#include "Game/compat/CameraPose.hpp"
#include "Game/compat/FileSelectSkyRuntime.hpp"
#include "Game/compat/J3dAnimation.hpp"
#include "Game/compat/J3dMaterialRuntime.hpp"
#include "Game/compat/J3dModel.hpp"
#include "Game/compat/J3dTexture.hpp"
#include "Game/compat/JMathTrig.hpp"
#include "Game/compat/RarcArchive.hpp"
#include "Game/compat/TplTexture.hpp"
#include "Game/compat/Yaz0.hpp"
#include "capture/ScreenshotService.hpp"
#include "core/RenderTypes.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

    [[nodiscard]] std::uint32_t read_be32(std::span< const std::uint8_t > data, std::size_t offset) {
        if (offset + 4U > data.size()) {
            throw std::runtime_error("read_be32 out of range");
        }

        return (static_cast< std::uint32_t >(data[offset]) << 24U) | (static_cast< std::uint32_t >(data[offset + 1U]) << 16U) |
               (static_cast< std::uint32_t >(data[offset + 2U]) << 8U) | static_cast< std::uint32_t >(data[offset + 3U]);
    }

    [[nodiscard]] std::vector< std::uint8_t > read_file(const std::filesystem::path& path) {
        auto file = std::ifstream(path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("cannot open " + path.string());
        }

        file.seekg(0, std::ios::end);
        const auto size = file.tellg();
        if (size < 0) {
            throw std::runtime_error("cannot determine size for " + path.string());
        }

        auto bytes = std::vector< std::uint8_t >(static_cast< std::size_t >(size));
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast< char* >(bytes.data()), static_cast< std::streamsize >(bytes.size()));
        if (!file) {
            throw std::runtime_error("cannot read " + path.string());
        }

        return bytes;
    }

    [[nodiscard]] std::filesystem::path disc_files_root() {
        const auto cwd = std::filesystem::current_path();
        const std::filesystem::path candidates[]{
            cwd / "orig" / "RMGK01" / "files",
            cwd.parent_path() / "orig" / "RMGK01" / "files",
        };

        for (const auto& candidate : candidates) {
            std::error_code error{};
            const auto canonical = std::filesystem::weakly_canonical(candidate, error);
            if (!error && std::filesystem::is_directory(canonical, error)) {
                return canonical;
            }
        }

        throw std::runtime_error("could not locate orig/RMGK01/files from " + cwd.string());
    }

    void require(bool condition, std::string_view message) {
        if (!condition) {
            throw std::runtime_error(std::string(message));
        }
    }

    void require_near(float actual, float expected, float tolerance, std::string_view message) {
        if (std::abs(actual - expected) > tolerance) {
            throw std::runtime_error(std::string(message));
        }
    }

    void require_tev_stage(const smgpc::game::J3dTevStageSummary& stage, std::array< std::uint8_t, 4U > color_in, std::uint8_t k_color_sel,
                           std::array< std::uint8_t, 4U > alpha_in, std::uint8_t alpha_clamp, std::uint8_t k_alpha_sel, std::string_view message) {
        require(stage.color_in == color_in, message);
        require(stage.color_op == 0U && stage.color_bias == 0U && stage.color_scale == 0U && stage.color_clamp == 1U && stage.color_out == 0U,
                message);
        require(stage.k_color_sel == k_color_sel, message);
        require(stage.alpha_in == alpha_in, message);
        require(stage.alpha_op == 0U && stage.alpha_bias == 0U && stage.alpha_scale == 0U && stage.alpha_clamp == alpha_clamp &&
                    stage.alpha_out == 0U,
                message);
        require(stage.k_alpha_sel == k_alpha_sel, message);
    }

    void require_magic(std::span< const std::uint8_t > data, std::string_view magic) {
        require(data.size() >= magic.size(), "data too short for magic");
        for (std::size_t i = 0; i < magic.size(); ++i) {
            require(data[i] == static_cast< std::uint8_t >(magic[i]), "unexpected magic");
        }
    }

    [[nodiscard]] std::string lower_copy(std::string_view value) {
        auto lower = std::string(value);
        std::ranges::transform(lower, lower.begin(), [](unsigned char character) { return static_cast< char >(std::tolower(character)); });
        return lower;
    }

    [[nodiscard]] std::string base_name(std::string_view path) {
        const auto slash = path.find_last_of('/');
        if (slash == std::string_view::npos) {
            return std::string(path);
        }

        return std::string(path.substr(slash + 1U));
    }

    [[nodiscard]] const smgpc::game::RarcEntry* find_entry_by_basename(const smgpc::game::RarcArchive& archive, std::string_view name) {
        const auto requested = lower_copy(name);
        const auto it =
            std::ranges::find_if(archive.entries(), [&requested](const auto& entry) { return lower_copy(base_name(entry.path)) == requested; });

        return it == archive.entries().end() ? nullptr : &(*it);
    }

    void test_yaz0_decompression(const std::filesystem::path& title_logo_path) {
        const auto compressed = read_file(title_logo_path);
        require(smgpc::game::is_yaz0(compressed), "TitleLogo.arc should be Yaz0-compressed");

        const auto decompressed = smgpc::game::decompress_yaz0(compressed);
        require_magic(decompressed, "RARC");
        require(read_be32(decompressed, 0x04U) == decompressed.size(), "RARC header file size should match decompressed size");
    }

    void test_rarc_title_archives(const std::filesystem::path& root) {
        const auto title_logo = smgpc::game::RarcArchive::from_file(root / "KrKorean" / "LayoutData" / "TitleLogo.arc");
        const auto press_start = smgpc::game::RarcArchive::from_file(root / "KrKorean" / "LayoutData" / "PressStart.arc");

        require(title_logo.entries().size() == 18U, "TitleLogo.arc entry count changed");
        require(press_start.entries().size() == 5U, "PressStart.arc entry count changed");

        require(title_logo.contains("blyt/titlelogo.brlyt"), "TitleLogo.arc missing titlelogo.brlyt");
        require(title_logo.contains("anim/appear.brlan"), "TitleLogo.arc missing appear.brlan");
        require(title_logo.contains("anim/wait.brlan"), "TitleLogo.arc missing wait.brlan");
        require(title_logo.contains("anim/decide.brlan"), "TitleLogo.arc missing decide.brlan");
        require(title_logo.contains("timg/mytitlelogokor.tpl"), "TitleLogo.arc missing Korean title logo texture");

        require(press_start.contains("blyt/pressstart.brlyt"), "PressStart.arc missing pressstart.brlyt");
        require(press_start.contains("anim/appear.brlan"), "PressStart.arc missing appear.brlan");
        require(press_start.contains("anim/wait.brlan"), "PressStart.arc missing wait.brlan");
        require(press_start.contains("anim/end.brlan"), "PressStart.arc missing end.brlan");

        require_magic(title_logo.file_data("blyt/titlelogo.brlyt"), "RLYT");
        require_magic(title_logo.file_data("anim/appear.brlan"), "RLAN");
        require_magic(press_start.file_data("blyt/pressstart.brlyt"), "RLYT");
        require_magic(press_start.file_data("anim/appear.brlan"), "RLAN");
    }

    void test_tpl_title_texture_decode(const std::filesystem::path& root) {
        const auto title_logo = smgpc::game::RarcArchive::from_file(root / "KrKorean" / "LayoutData" / "TitleLogo.arc");
        const auto title_texture = smgpc::game::decode_tpl_texture(title_logo.file_data("timg/mytitlelogokor.tpl"));
        require(title_texture.width == 272U, "mytitlelogokor.tpl width changed");
        require(title_texture.height == 32U, "mytitlelogokor.tpl height changed");
        require(title_texture.format == smgpc::game::TplTextureFormat::I4, "mytitlelogokor.tpl format changed");
        require(title_texture.rgba.size() == static_cast< std::size_t >(title_texture.width) * title_texture.height * 4U,
                "decoded title texture size mismatch");

        auto visible_pixels = 0U;
        for (std::size_t offset = 3U; offset < title_texture.rgba.size(); offset += 4U) {
            if (title_texture.rgba[offset] != 0U) {
                ++visible_pixels;
            }
        }
        require(visible_pixels > 1000U, "decoded title texture should contain visible pixels");
    }

    void test_brlyt_title_picture_parse(const std::filesystem::path& root) {
        const auto title_logo = smgpc::game::RarcArchive::from_file(root / "KrKorean" / "LayoutData" / "TitleLogo.arc");
        const auto layout = smgpc::game::parse_brlyt_layout(title_logo.file_data("blyt/titlelogo.brlyt"));
        require_near(layout.width, 608.0F, 0.001F, "titlelogo.brlyt layout width changed");
        require_near(layout.height, 456.0F, 0.001F, "titlelogo.brlyt layout height changed");
        require(!layout.panes.empty(), "titlelogo.brlyt should expose pane hierarchy");
        require(!layout.texture_names.empty(), "titlelogo.brlyt should reference textures");
        require(!layout.materials.empty(), "titlelogo.brlyt should expose materials");
        require(!layout.pictures.empty(), "titlelogo.brlyt should contain picture panes");

        const auto title_root = std::ranges::find_if(layout.panes, [](const auto& pane) { return pane.name == "SMGTitleLogo"; });
        require(title_root != layout.panes.end(), "titlelogo.brlyt should contain SMGTitleLogo pane");
        require(title_root->scale_x == 0.0F && title_root->scale_y == 0.0F, "SMGTitleLogo base scale should remain animation-driven");

        const auto title_picture =
            std::ranges::find_if(layout.pictures, [](const auto& picture) { return picture.texture_name == "MyTitleLogoKOR.tpl"; });
        require(title_picture != layout.pictures.end(), "titlelogo.brlyt should reference MyTitleLogoKOR.tpl");
        require(title_picture->material_index == 3U, "title logo picture material index changed");
        require(title_picture->wrap_s == 0U && title_picture->wrap_t == 0U, "title logo picture should preserve clamp wrap modes from BRLYT TexMap");
        require(title_picture->min_filter == 0U && title_picture->mag_filter == 0U, "title logo picture should preserve BRLYT TexMap filter bits");
        require(title_picture->width > 0.0F, "title logo picture width should be positive");
        require(title_picture->height > 0.0F, "title logo picture height should be positive");
        require_near(title_picture->tex_coords[0U].u, 0.0F, 0.001F, "title picture top-left U should come from BRLYT tex coords");
        require_near(title_picture->tex_coords[0U].v, 0.0F, 0.001F, "title picture top-left V should come from BRLYT tex coords");
        require_near(title_picture->tex_coords[2U].u, 1.0F, 0.001F, "title picture bottom-right U should come from BRLYT tex coords");
        require_near(title_picture->tex_coords[2U].v, 1.0F, 0.001F, "title picture bottom-right V should come from BRLYT tex coords");
        require(title_picture->vertex_colors[0U][3U] == 255U, "title picture vertex alpha should come from BRLYT vertex colors");

        const auto galaxy_picture = std::ranges::find_if(layout.pictures, [](const auto& picture) { return picture.name == "PicLogoGalaxy"; });
        require(galaxy_picture != layout.pictures.end(), "titlelogo.brlyt should keep the multi-texture PicLogoGalaxy picture");
        require(galaxy_picture->material_index == 0U, "PicLogoGalaxy material index changed");

        const auto& galaxy_material = layout.materials.at(galaxy_picture->material_index);
        require(galaxy_material.name == "PicLogoGalaxy", "PicLogoGalaxy material name changed");
        require(galaxy_material.textures.size() == 2U, "PicLogoGalaxy should use the original two texture maps");
        require(galaxy_material.textures[0U].texture_name == "MyTitleSpaceKOR.tpl",
                "PicLogoGalaxy first texture should be the scrolling space texture");
        require(galaxy_material.textures[1U].texture_name == "MyTitleMaskKOR.tpl", "PicLogoGalaxy second texture should be the Wii mask texture");
        require(galaxy_material.textures[0U].wrap_s == 1U && galaxy_material.textures[0U].wrap_t == 0U,
                "PicLogoGalaxy space texture should preserve BRLYT wrap modes");
        require(galaxy_material.tex_srts.size() == 2U, "PicLogoGalaxy should expose both texture SRT slots");
        require(galaxy_material.tex_coord_gens.size() == 2U, "PicLogoGalaxy should expose both texture coord generators");
        require(galaxy_material.tex_coord_gens[0U].tex_mtx == 30U && galaxy_material.tex_coord_gens[1U].tex_mtx == 33U,
                "PicLogoGalaxy texture generators should preserve GX_TEXMTX0/GX_TEXMTX1");
        require(galaxy_material.tev_stages.size() == 2U, "PicLogoGalaxy should expose both original TEV stages");
        require(galaxy_material.alpha_compare.enabled, "PicLogoGalaxy should preserve alpha compare state");
        require(galaxy_material.blend_mode.enabled, "PicLogoGalaxy should preserve blend mode state");
    }

    void test_brlyt_press_start_text_parse(const std::filesystem::path& root) {
        const auto press_start = smgpc::game::RarcArchive::from_file(root / "KrKorean" / "LayoutData" / "PressStart.arc");
        const auto layout = smgpc::game::parse_brlyt_layout(press_start.file_data("blyt/pressstart.brlyt"));
        require_near(layout.width, 608.0F, 0.001F, "pressstart.brlyt layout width changed");
        require_near(layout.height, 456.0F, 0.001F, "pressstart.brlyt layout height changed");
        require(!layout.font_names.empty(), "pressstart.brlyt should reference font resources");
        require(!layout.panes.empty(), "pressstart.brlyt should expose pane hierarchy");
        require(!layout.text_boxes.empty(), "pressstart.brlyt should contain text boxes");

        const auto shadow = std::ranges::find_if(layout.text_boxes, [](const auto& text_box) { return text_box.name == "ShaStart"; });
        require(shadow != layout.text_boxes.end(), "pressstart.brlyt should contain ShaStart shadow text box");
        require(shadow->material_index == 0U, "ShaStart should use the original shadow material");
        require(shadow->color_mapping_max[0U] == 0U && shadow->color_mapping_max[1U] == 0U && shadow->color_mapping_max[2U] == 0U,
                "ShaStart material should map glyph color to black");
        require(shadow->color_mapping_max[3U] == 100U, "ShaStart material should preserve original shadow alpha mapping");

        const auto prompt = std::ranges::find_if(layout.text_boxes, [](const auto& text_box) { return text_box.name == "TxtStart"; });
        require(prompt != layout.text_boxes.end(), "pressstart.brlyt should contain TxtStart text box");
        require(prompt->material_index == 1U, "TxtStart should use the original foreground text material");
        require(prompt->color_mapping_max[0U] == 255U && prompt->color_mapping_max[1U] == 255U && prompt->color_mapping_max[2U] == 255U,
                "TxtStart material should map glyph color to white");
        require(prompt->font_name == "MessageFont26kor.brfnt", "TxtStart should use the original Korean message font");
        require(prompt->font_width > 0.0F && prompt->font_height > 0.0F, "TxtStart font size should be positive");

        const std::array< std::uint16_t, 11U > expected_text{
            0xff21U, 0xc640U, 0x0042U, 0xb97cU, 0x0020U, 0xb20cU, 0xb7ecU, 0x0020U, 0xc8fcU, 0xc138U, 0xc694U,
        };
        require(prompt->text.size() == expected_text.size(), "TxtStart text length changed");
        require(std::ranges::equal(prompt->text, expected_text), "TxtStart UTF-16BE text changed");
    }

    void test_brfnt_message_font_decode(const std::filesystem::path& root) {
        const auto font_archive = smgpc::game::RarcArchive::from_file(root / "KrKorean" / "LayoutData" / "Font.arc");
        const auto* font_entry = find_entry_by_basename(font_archive, "MessageFont26.brfnt");
        require(font_entry != nullptr, "Font.arc should contain MessageFont26.brfnt");

        const auto font = smgpc::game::parse_brfnt_font(font_archive.file_data(*font_entry));
        require(!font.sheets.empty(), "MessageFont26.brfnt should contain decoded glyph sheets");
        require(font.sheet_width > 0U && font.sheet_height > 0U, "MessageFont26.brfnt sheet dimensions should be positive");
        require(font.width > 0U && font.height > 0U, "MessageFont26.brfnt font dimensions should be positive");

        for (const auto code : std::array< std::uint16_t, 4U >{0xff21U, 0xc640U, 0x0042U, 0xb20cU}) {
            require(font.glyph_for(code).has_value(), "MessageFont26.brfnt should map prompt glyphs");
        }
        require(font.glyph_for(0xff21U)->x == font.glyph_for(0x0041U)->x, "fullwidth A should normalize to ASCII A in the BRFNT compatibility layer");
        require(font.glyph_for(0xff21U)->y == font.glyph_for(0x0041U)->y, "fullwidth A should normalize to ASCII A glyph row");

        require(!font.glyph_for_exact(0xff21U).has_value(), "MessageFont26.brfnt should not directly map fullwidth A");
        const auto icon_a = font.glyph_for_exact(0xe000U);
        const auto icon_b = font.glyph_for_exact(0xe00bU);
        require(icon_a.has_value() && icon_b.has_value(), "MessageFont26.brfnt should expose private-use A/B button icon glyphs");
        require(icon_a->sheet_index == 4U && icon_a->x == 190U && icon_a->y == 727U, "A button icon glyph location changed");
        require(icon_b->sheet_index == 4U && icon_b->x == 1U && icon_b->y == 793U, "B button icon glyph location changed");
    }

    void test_bcsv_file_select_camera_parse(const std::filesystem::path& root) {
        const auto file_select = smgpc::game::RarcArchive::from_file(root / "StageData" / "FileSelect.arc");
        const auto camera = smgpc::game::BcsvTable::from_bytes(file_select.file_data("camera/cameraparam.bcam"));

        require(camera.entry_count() == 8U, "FileSelect cameraparam entry count changed");
        require(camera.fields().size() == 32U, "FileSelect cameraparam field count changed");
        require(camera.entry_size() == 128U, "FileSelect cameraparam entry size changed");
        require(camera.field_index("camtype").has_value(), "FileSelect cameraparam should expose camtype field by JMap hash");

        require(camera.get_s32(5U, "version").has_value() && *camera.get_s32(5U, "version") == 196621, "FileSelect start camera version changed");
        require(camera.get_string(5U, "camtype").has_value() && *camera.get_string(5U, "camtype") == "CAM_TYPE_XZ_PARA",
                "FileSelect start camera type changed");
        require_near(*camera.get_float(5U, "angleA"), 1.57693F, 0.00001F, "FileSelect start camera angleA changed");
        require_near(*camera.get_float(5U, "angleB"), 0.473233F, 0.00001F, "FileSelect start camera angleB changed");
        require_near(*camera.get_float(5U, "dist"), 5000.0F, 0.001F, "FileSelect start camera distance changed");
        require_near(*camera.get_float(5U, "fovy"), 45.0F, 0.001F, "FileSelect start camera fovy changed");
        const auto start_world_offset = camera.get_vec3(5U, "woffset");
        require(start_world_offset.has_value(), "FileSelect start camera should expose woffset vector");
        require_near((*start_world_offset)[1U], 100.0F, 0.001F, "FileSelect start camera Y world offset changed");

        require(camera.get_string(6U, "camtype").has_value() && *camera.get_string(6U, "camtype") == "CAM_TYPE_FOLLOW",
                "FileSelect default camera type changed");
        require_near(*camera.get_float(6U, "angleA"), 0.174533F, 0.00001F, "FileSelect default camera angleA changed");
        require_near(*camera.get_float(6U, "angleB"), 0.349066F, 0.00001F, "FileSelect default camera angleB changed");
        require_near(*camera.get_float(6U, "loffset"), 100.0F, 0.001F, "FileSelect default camera local offset changed");
        const auto default_world_offset = camera.get_vec3(6U, "woffset");
        const auto default_axis = camera.get_vec3(6U, "axis");
        require(default_world_offset.has_value() && default_axis.has_value(), "FileSelect default camera should expose vector fields");
        require_near((*default_world_offset)[1U], 170.0F, 0.001F, "FileSelect default camera Y world offset changed");
        require_near((*default_axis)[0U], 1500.0F, 0.001F, "FileSelect default camera X axis changed");
        require_near((*default_axis)[1U], 1000.0F, 0.001F, "FileSelect default camera Y axis changed");

        require(camera.get_string(7U, "id").has_value() && *camera.get_string(7U, "id") == "s:03e7", "FileSelect fallback camera id changed");
    }

    void test_camera_param_file_select_chunk_load(const std::filesystem::path& root) {
        const auto file_select = smgpc::game::RarcArchive::from_file(root / "StageData" / "FileSelect.arc");
        const auto table = smgpc::game::BcsvTable::from_bytes(file_select.file_data("camera/cameraparam.bcam"));
        const auto chunks = smgpc::game::load_camera_param_chunks(table);

        require(chunks.size() == 8U, "FileSelect camera chunk count changed");

        const auto& start = chunks[5U];
        require(start.version == 196621U, "FileSelect start camera chunk version changed");
        require(start.camera_type == "CAM_TYPE_XZ_PARA", "FileSelect start camera chunk type changed");
        require_near(start.general.angle_a, 1.57693F, 0.00001F, "FileSelect start camera chunk angleA changed");
        require_near(start.general.angle_b, 0.473233F, 0.00001F, "FileSelect start camera chunk angleB changed");
        require_near(start.general.dist, 5000.0F, 0.001F, "FileSelect start camera chunk distance changed");
        require(start.general.num1 == 0, "FileSelect start camera chunk num1 changed");
        require_near(start.extra.fovy, 45.0F, 0.001F, "FileSelect start camera chunk fovy changed");
        require_near(start.extra.w_offset.y, 100.0F, 0.001F, "FileSelect start camera chunk world offset changed");
        require(start.extra.flags == 0U, "FileSelect start camera flags changed");
        require(!start.is_on_use_fovy(), "FileSelect start camera should preserve unset nofovy flag");
        require(start.game_thru == 0, "FileSelect start camera game thru flag changed");

        const auto& follow = chunks[6U];
        require(follow.camera_type == "CAM_TYPE_FOLLOW", "FileSelect default follow camera chunk type changed");
        require_near(follow.general.axis.x, 1500.0F, 0.001F, "FileSelect default follow camera axis X changed");
        require_near(follow.general.axis.y, 1000.0F, 0.001F, "FileSelect default follow camera axis Y changed");
        require_near(follow.general.dist, 0.15F, 0.001F, "FileSelect default follow camera dist changed");
        require(follow.general.num1 == 1, "FileSelect default follow camera num1 changed");
        require_near(follow.extra.l_offset, 100.0F, 0.001F, "FileSelect default follow camera local offset changed");
        require_near(follow.extra.w_offset.y, 170.0F, 0.001F, "FileSelect default follow camera world offset changed");

        const auto fallback = smgpc::game::find_camera_param_chunk(chunks, "s:03e7");
        require(fallback.has_value(), "FileSelect fallback camera should be findable by id");
        require(fallback->camera_type == "CAM_TYPE_FOLLOW", "FileSelect fallback camera chunk type changed");
        require_near(fallback->general.axis.x, 900.0F, 0.001F, "FileSelect fallback camera axis X changed");
        require_near(fallback->general.axis.y, 600.0F, 0.001F, "FileSelect fallback camera axis Y changed");
    }

    void test_file_select_title_camera_pose() {
        const auto pose = smgpc::game::file_select_title_camera_pose();
        require_near(pose.eye.x, 0.0F, 0.001F, "FileSelect title camera eye X changed");
        require_near(pose.eye.y, 15800.0F, 0.001F, "FileSelect title camera eye Y should include cFarTarget.Y + 15000 title offset");
        require_near(pose.eye.z, 15000.0F, 0.001F, "FileSelect title camera eye Z changed");
        require_near(pose.watch.x, 0.0F, 0.001F, "FileSelect title camera watch X changed");
        require_near(pose.watch.y, 15800.0F, 0.001F, "FileSelect title camera watch Y should include cFarTarget.Y + 15000 title offset");
        require_near(pose.watch.z, 0.0F, 0.001F, "FileSelect title camera watch Z changed");
        require_near(pose.up.y, 1.0F, 0.001F, "FileSelect title camera up vector changed");
        require_near(pose.fovy_degrees, 60.0F, 0.001F, "FileSelect title camera programmable FOV changed");
        require_near(pose.aspect_ratio, 608.0F / 456.0F, 0.001F, "FileSelect title camera should use original 4:3 CameraContext aspect");
        require_near(pose.near_clip, 100.0F, 0.001F, "FileSelect title camera should use original CameraContext near clip");
        require_near(pose.far_clip, 800000.0F, 0.001F, "FileSelect title camera should use original CameraContext far clip");

        const auto watch = smgpc::game::transform_world_to_camera(pose, pose.watch);
        require_near(watch.x, 0.0F, 0.001F, "FileSelect title watch point should land on camera center X");
        require_near(watch.y, 0.0F, 0.001F, "FileSelect title watch point should land on camera center Y");
        require_near(watch.z, 15000.0F, 0.001F, "FileSelect title watch point depth changed");

        const auto origin = smgpc::game::transform_world_to_camera(pose, {0.0F, 0.0F, 0.0F});
        require_near(origin.y, -15800.0F, 0.001F, "FileSelect title origin Y should match original raised title view");
        require_near(origin.z, 15000.0F, 0.001F, "FileSelect title origin depth changed");
    }

    void test_jmath_short_trig_compat() {
        require(smgpc::game::jmath_sincos_table_index_from_short(0xffffU) == 0x3fffU, "JMath short trig index should use high 14 bits");
        require(smgpc::game::jmath_fctiwz_to_u16(2607.9F) == 2607U, "JMath fctiwz helper should truncate positive values toward zero");
        require(smgpc::game::jmath_fctiwz_to_u16(-1.9F) == 0xffffU, "JMath fctiwz helper should preserve low 16 bits for negative values");
        require_near(smgpc::game::jmath_cos_short(0x0000U), 1.0F, 0.000001F, "JMath cosShort(0) changed");
        require_near(smgpc::game::jmath_sin_short(0x4000U), 1.0F, 0.000001F, "JMath sinShort(0x4000) changed");
        require_near(smgpc::game::jmath_cos_short(0x8000U), -1.0F, 0.000001F, "JMath cosShort(0x8000) changed");
    }

    void test_file_select_sky_runtime() {
        require_near(smgpc::game::file_select_sky_yaw(2300U), 2.3F, 0.000001F, "FileSelectSky yaw update rate changed");
        require_near(smgpc::game::file_select_sky_pitch(0U), 0.0F, 0.000001F, "FileSelectSky pitch frame 0 changed");
        require_near(smgpc::game::file_select_sky_pitch(100U), 0.000400535F, 0.000001F,
                     "FileSelectSky pitch frame 100 should use JMath cosShort conversion");
        require_near(smgpc::game::file_select_sky_pitch(500U), 0.010059165F, 0.000001F,
                     "FileSelectSky pitch frame 500 should use JMath cosShort conversion");
        require_near(smgpc::game::file_select_sky_pitch(1500U), 0.089677349F, 0.000001F,
                     "FileSelectSky pitch frame 1500 should use JMath cosShort conversion");
        require_near(smgpc::game::file_select_sky_pitch(2300U), 0.207162336F, 0.000001F,
                     "FileSelectSky pitch frame 2300 should use JMath cosShort conversion");
        require_near(smgpc::game::file_select_sky_pitch(3000U), 0.345056713F, 0.000001F,
                     "FileSelectSky pitch frame 3000 should use JMath cosShort conversion");

        const auto identity_scaled = smgpc::game::file_select_sky_actor_matrix(0U);
        require_near(identity_scaled.m[0U], 0.8F, 0.000001F, "FileSelectSky frame 0 matrix X scale changed");
        require_near(identity_scaled.m[5U], 0.8F, 0.000001F, "FileSelectSky frame 0 matrix Y scale changed");
        require_near(identity_scaled.m[10U], 0.8F, 0.000001F, "FileSelectSky frame 0 matrix Z scale changed");

        const auto matrix = smgpc::game::file_select_sky_actor_matrix(2300U);
        require_near(matrix.m[0U], -0.533020794F, 0.000001F, "FileSelectSky frame 2300 matrix[0] changed");
        require_near(matrix.m[2U], -0.596564233F, 0.000001F, "FileSelectSky frame 2300 matrix[2] changed");
        require_near(matrix.m[4U], 0.122554213F, 0.000001F, "FileSelectSky frame 2300 matrix[4] changed");
        require_near(matrix.m[5U], 0.782936871F, 0.000001F, "FileSelectSky frame 2300 matrix[5] changed");
        require_near(matrix.m[6U], -0.109500274F, 0.000001F, "FileSelectSky frame 2300 matrix[6] changed");
        require_near(matrix.m[8U], 0.583840132F, 0.000001F, "FileSelectSky frame 2300 matrix[8] changed");
        require_near(matrix.m[9U], -0.164346725F, 0.000001F, "FileSelectSky frame 2300 matrix[9] changed");
        require_near(matrix.m[10U], -0.521652043F, 0.000001F, "FileSelectSky frame 2300 matrix[10] changed");
    }

    void test_brlan_title_animation_parse(const std::filesystem::path& root) {
        const auto title_logo = smgpc::game::RarcArchive::from_file(root / "KrKorean" / "LayoutData" / "TitleLogo.arc");
        const auto appear = smgpc::game::parse_brlan_animation(title_logo.file_data("anim/appear.brlan"));
        require(appear.frame_size == 201U, "TitleLogo appear frame size changed");
        require(!appear.loop, "TitleLogo appear should not loop");
        require(!appear.contents.empty(), "TitleLogo appear should contain animation content");

        const auto first_frame = appear.pane_frame("SMGTitleLogo", 0.0F);
        require(first_frame.translate_y.has_value() && *first_frame.translate_y == -26.0F, "TitleLogo appear should animate initial root Y");
        require(first_frame.scale_x.has_value() && *first_frame.scale_x == 0.0F, "TitleLogo appear should animate initial root X scale");
        require(first_frame.scale_y.has_value() && *first_frame.scale_y == 0.0F, "TitleLogo appear should animate initial root Y scale");

        const auto visible_frame = appear.pane_frame("SMGTitleLogo", 4.0F);
        require(visible_frame.scale_x.has_value() && *visible_frame.scale_x == 1.0F, "TitleLogo appear should animate root X scale to 1");
        require(visible_frame.scale_y.has_value() && *visible_frame.scale_y == 1.0F, "TitleLogo appear should animate root Y scale to 1");

        const auto wait = smgpc::game::parse_brlan_animation(title_logo.file_data("anim/wait.brlan"));
        require(wait.frame_size == 10000U, "TitleLogo wait frame size changed");
        require(wait.loop, "TitleLogo wait should loop");
        const auto galaxy_texture_middle = wait.texture_frame("PicLogoGalaxy", 5000.0F);
        require(galaxy_texture_middle.translate_s.has_value(), "TitleLogo wait should animate PicLogoGalaxy texture S translation");
        require_near(*galaxy_texture_middle.translate_s, 0.5F, 0.001F, "TitleLogo wait should scroll PicLogoGalaxy texture S at half-frame");
    }

    void test_brlan_press_start_animation_parse(const std::filesystem::path& root) {
        const auto press_start = smgpc::game::RarcArchive::from_file(root / "KrKorean" / "LayoutData" / "PressStart.arc");
        const auto appear = smgpc::game::parse_brlan_animation(press_start.file_data("anim/appear.brlan"));
        require(appear.frame_size == 31U, "PressStart appear frame size changed");
        require(!appear.loop, "PressStart appear should not loop");
        const auto faded_in = appear.pane_frame("PressAB", 30.0F);
        require(faded_in.alpha.has_value() && *faded_in.alpha == 255.0F, "PressStart appear should fade prompt alpha to 255");

        const auto wait = smgpc::game::parse_brlan_animation(press_start.file_data("anim/wait.brlan"));
        require(wait.frame_size == 120U, "PressStart wait frame size changed");
        require(wait.loop, "PressStart wait should loop");
        const auto middle = wait.pane_frame("PressAB", 60.0F);
        require(middle.translate_y.has_value(), "PressStart wait should animate prompt Y");
        require_near(*middle.translate_y, 3.0F, 0.001F, "PressStart wait should bob prompt Y at mid-frame");
    }

    void test_j3d_comet_near_orbit_sky_textures(const std::filesystem::path& root) {
        const auto sky_archive = smgpc::game::RarcArchive::from_file(root / "ObjectData" / "CometNearOrbitSky.arc");
        const auto textures = smgpc::game::extract_j3d_textures(sky_archive.file_data("cometnearorbitsky.bdl"));
        require(textures.size() == 12U, "CometNearOrbitSky.bdl TEX1 texture count changed");

        const auto find_texture = [&textures](std::string_view name) -> const smgpc::game::J3dTexture* {
            const auto it = std::ranges::find_if(textures, [name](const auto& texture) { return texture.name == name; });
            return it == textures.end() ? nullptr : &*it;
        };

        const auto* orbit_universe = find_texture("OrbitUniverseL");
        require(orbit_universe != nullptr, "CometNearOrbitSky should contain OrbitUniverseL");
        require(orbit_universe->image.width == 1024U && orbit_universe->image.height == 512U, "OrbitUniverseL dimensions changed");
        require(orbit_universe->image.format == smgpc::game::TplTextureFormat::I4, "OrbitUniverseL should use GX I4");

        const auto* earth = find_texture("EarthKsMM");
        require(earth != nullptr, "CometNearOrbitSky should contain EarthKsMM");
        require(earth->image.width == 256U && earth->image.height == 256U, "EarthKsMM dimensions changed");

        const auto* galaxy = find_texture("Galaxy");
        require(galaxy != nullptr, "CometNearOrbitSky should contain Galaxy");
        require(galaxy->image.width == 64U && galaxy->image.height == 64U, "Galaxy dimensions changed");
        require(galaxy->image.format == smgpc::game::TplTextureFormat::CMPR, "Galaxy should exercise GX CMPR decoding");
        require(std::ranges::any_of(galaxy->image.rgba, [](std::uint8_t value) { return value != 0U; }),
                "CMPR decoded Galaxy texture should not be blank");
    }

    void test_j3d_comet_near_orbit_sky_model_probe(const std::filesystem::path& root) {
        const auto sky_archive = smgpc::game::RarcArchive::from_file(root / "ObjectData" / "CometNearOrbitSky.arc");
        const auto model = smgpc::game::inspect_j3d_model(sky_archive.file_data("cometnearorbitsky.bdl"));

        require(model.section_count == 9U, "CometNearOrbitSky.bdl section count changed");
        require(model.info.has_value(), "CometNearOrbitSky.bdl should expose INF1");
        require(model.vertices.has_value(), "CometNearOrbitSky.bdl should expose VTX1");
        require(model.joints.has_value(), "CometNearOrbitSky.bdl should expose JNT1");
        require(model.shapes.has_value(), "CometNearOrbitSky.bdl should expose SHP1");
        require(model.materials.has_value(), "CometNearOrbitSky.bdl should expose MAT3");
        require(model.textures.size() == 12U, "CometNearOrbitSky.bdl should expose TEX1 textures through model probe");

        require(model.info->packet_count == 9U, "CometNearOrbitSky packet count changed");
        require(model.info->vertex_count == 1029U, "CometNearOrbitSky vertex count changed");
        require(model.info->hierarchy.size() == 71U, "CometNearOrbitSky hierarchy size changed");
        require(model.vertices->formats.size() == 4U, "CometNearOrbitSky VTX1 format count changed");
        require(model.joints->joint_count == 8U, "CometNearOrbitSky JNT1 joint count changed");
        require(model.joints->joints.size() == 8U, "CometNearOrbitSky JNT1 joints should be decoded");
        require(model.shapes->shape_count == 9U, "CometNearOrbitSky shape count changed");
        require(model.materials->material_count == 9U, "CometNearOrbitSky material count changed");
        require(model.joints->joints[0U].name == "world_root", "CometNearOrbitSky root joint name changed");
        require(model.joints->joints[7U].name == "Obit", "CometNearOrbitSky orbit joint name changed");
        require_near(model.joints->joints[7U].radius, 793869.0F, 0.5F, "CometNearOrbitSky orbit joint radius changed");

        const auto find_material = [&model](std::string_view name) -> const smgpc::game::J3dMaterialSummary* {
            const auto it = std::ranges::find_if(model.materials->materials, [name](const auto& material) { return material.name == name; });
            return it == model.materials->materials.end() ? nullptr : &*it;
        };

        const auto* space = find_material("Space_Mat_v");
        require(space != nullptr, "CometNearOrbitSky should expose Space_Mat_v");
        require(space->textures.size() == 3U, "Space_Mat_v should bind the original three textures");
        require(space->textures[0U].texture_index == 5U, "Space_Mat_v first texture should be OrbitUniverseL");
        require(space->textures[1U].texture_index == 6U, "Space_Mat_v second texture should be Galaxy");
        require(space->textures[2U].texture_index == 7U, "Space_Mat_v third texture should be GalaxyRiverK");
        require(space->tex_coord_gens.size() == 3U, "Space_Mat_v should expose the original three texture coordinate generators");
        require(space->tex_coord_gens[0U].matrix == 30U && space->tex_coord_gens[1U].matrix == 33U && space->tex_coord_gens[2U].matrix == 36U,
                "Space_Mat_v texture coordinate generators should preserve GX texture matrix slots");
        require(space->tex_matrices.size() == 3U, "Space_Mat_v should expose the original three texture matrices");
        require_near(space->tex_matrices[0U].scale_s, 0.5F, 0.001F, "Space_Mat_v matrix 0 S scale changed");
        require_near(space->tex_matrices[1U].scale_t, 0.288086F, 0.001F, "Space_Mat_v matrix 1 T scale changed");
        require(space->tev_orders.size() == 3U, "Space_Mat_v should expose the original three TEV orders");
        require(space->tev_orders[0U].tex_coord == 1U && space->tev_orders[0U].tex_map == 1U,
                "Space_Mat_v stage 0 should sample Galaxy through texture coordinate slot 1");
        require(space->tev_orders[1U].tex_coord == 0U && space->tev_orders[1U].tex_map == 0U,
                "Space_Mat_v stage 1 should sample OrbitUniverseL through texture coordinate slot 0");
        require(space->tev_orders[2U].tex_coord == 2U && space->tev_orders[2U].tex_map == 2U,
                "Space_Mat_v stage 2 should sample GalaxyRiverK through texture coordinate slot 2");
        require(space->tev_stages.size() == 3U, "Space_Mat_v should expose the original three raw TEV stages");
        require_tev_stage(space->tev_stages[0U], {15U, 8U, 10U, 14U}, 12U, {7U, 4U, 5U, 7U}, 1U, 28U,
                          "Space_Mat_v TEV stage 0 semantic decode changed");
        require_tev_stage(space->tev_stages[1U], {15U, 10U, 8U, 0U}, 12U, {5U, 7U, 7U, 7U}, 0U, 28U,
                          "Space_Mat_v TEV stage 1 semantic decode changed");
        require(space->alpha_compare.enabled, "Space_Mat_v should preserve alpha compare state");
        require(space->blend.enabled && space->blend.type == 0U && space->blend.src_factor == 1U && space->blend.dst_factor == 0U,
                "Space_Mat_v should preserve original no-blend state");
        require(space->cull_mode == 0U, "Space_Mat_v should preserve original GX_CULL_NONE state");
        require(space->z_mode.enabled && space->z_mode.compare_enable == 1U && space->z_mode.function == 3U && space->z_mode.update_enable == 0U,
                "Space_Mat_v should preserve original test-only GX_LEQUAL Z mode");
        const auto space_passes = smgpc::game::j3d_material_texture_passes(*space);
        require(space_passes.size() == 3U, "Space_Mat_v should build three runtime texture passes from TEV order");
        require(space_passes[0U].texture_index == 6U && space_passes[0U].tex_coord_slot == 1U,
                "Space_Mat_v pass 0 should sample Galaxy through tex coord 1");
        require(space_passes[1U].texture_index == 5U && space_passes[1U].tex_coord_slot == 0U,
                "Space_Mat_v pass 1 should sample OrbitUniverseL through tex coord 0");
        require(space_passes[2U].texture_index == 7U && space_passes[2U].tex_coord_slot == 2U,
                "Space_Mat_v pass 2 should sample GalaxyRiverK through tex coord 2");
        require(space_passes[0U].tex_matrix.has_value() && space_passes[0U].tex_matrix->slot == 1U,
                "Space_Mat_v pass 0 should resolve GX_TEXMTX1-compatible slot 1");
        const auto representative_space_pass = smgpc::game::j3d_representative_texture_pass(*space);
        require(representative_space_pass.has_value(), "Space_Mat_v should expose a representative runtime texture pass");
        require(representative_space_pass->texture_index == 5U && representative_space_pass->tex_map_slot == 0U,
                "Space_Mat_v representative pass should use the original base starfield texture map");
        require(representative_space_pass->tex_matrix.has_value() && representative_space_pass->tex_matrix->slot == 0U,
                "Space_Mat_v representative pass should resolve its base texture matrix");
        const auto transformed_space_coord = smgpc::game::j3d_transform_tex_coord(smgpc::game::J3dMeshVertex{.u = 0.75F, .v = 0.25F},
                                                                                  &space->tex_coord_gens[0U], &space->tex_matrices[0U]);
        require_near(transformed_space_coord.u, 0.625F, 0.001F, "J3D texture matrix transform should apply centered S scale");
        require_near(transformed_space_coord.v, 0.25F, 0.001F, "J3D texture matrix transform should preserve V without SRT changes");

        const auto* earth_far = find_material("EarthFar_v");
        require(earth_far != nullptr, "CometNearOrbitSky should expose EarthFar_v");
        require(earth_far->textures.size() == 3U, "EarthFar_v should bind the original earth/cloud texture stack");
        require(earth_far->tex_coord_gens.size() == 3U, "EarthFar_v should expose the original three texture coordinate generators");
        require(earth_far->tex_coord_gens[0U].type == 0U && earth_far->tex_coord_gens[0U].source == 0U,
                "EarthFar_v base texture coordinate generator should be GX_TG_MTX3x4 from position");
        require(earth_far->tex_matrices.size() == 3U, "EarthFar_v should expose the original three texture matrices");
        require_near(earth_far->tex_matrices[1U].translate_s, 0.332031F, 0.001F, "EarthFar_v far texture S translation changed");
        require_near(earth_far->tex_matrices[2U].scale_s, 0.1F, 0.001F, "EarthFar_v cloud matrix S scale changed");
        require_near(earth_far->tex_matrices[2U].scale_t, 0.5F, 0.001F, "EarthFar_v cloud matrix T scale changed");
        const auto projected_earth_coord =
            smgpc::game::j3d_transform_tex_coord(smgpc::game::J3dMeshVertex{.x = 10.0F, .y = 20.0F, .z = 40.0F, .u = 0.75F, .v = 0.25F},
                                                 &earth_far->tex_coord_gens[0U], &earth_far->tex_matrices[0U]);
        require_near(projected_earth_coord.u, 0.49848F, 0.001F,
                     "J3D GX_TG_POS texture generation should apply projected texture matrix before Q divide");
        require_near(projected_earth_coord.v, 0.50253F, 0.001F,
                     "J3D GX_TG_POS texture generation should apply projected texture matrix before Q divide");
        const auto scaled_actor_matrix = smgpc::game::J3dMatrix3x4{
            .m =
                {
                    0.8F,
                    0.0F,
                    0.0F,
                    0.0F,
                    0.0F,
                    0.8F,
                    0.0F,
                    0.0F,
                    0.0F,
                    0.0F,
                    0.8F,
                    0.0F,
                },
        };
        const auto projected_scaled_earth_coord =
            smgpc::game::j3d_transform_tex_coord(smgpc::game::J3dMeshVertex{.x = 10.0F, .y = 20.0F, .z = 40.0F, .u = 0.75F, .v = 0.25F},
                                                 &earth_far->tex_coord_gens[0U], &earth_far->tex_matrices[0U], &scaled_actor_matrix);
        require_near(projected_scaled_earth_coord.u, 0.49879F, 0.001F,
                     "J3D projected texture generation should include the actor/model matrix passed to J3DTexMtx::calc");
        require_near(projected_scaled_earth_coord.v, 0.50202F, 0.001F,
                     "J3D projected texture generation should include the actor/model matrix passed to J3DTexMtx::calc");
        require(earth_far->tev_orders.size() == 3U, "EarthFar_v should expose the original three TEV orders");
        require(earth_far->tev_stages.size() == 3U, "EarthFar_v should expose the original three raw TEV stages");
        require_tev_stage(earth_far->tev_stages[0U], {15U, 10U, 8U, 15U}, 12U, {7U, 4U, 5U, 7U}, 1U, 28U,
                          "EarthFar_v TEV stage 0 semantic decode changed");
        require_tev_stage(earth_far->tev_stages[1U], {4U, 14U, 8U, 0U}, 13U, {5U, 7U, 7U, 7U}, 0U, 28U,
                          "EarthFar_v TEV stage 1 semantic decode changed");
        require_tev_stage(earth_far->tev_stages[2U], {15U, 10U, 8U, 0U}, 14U, {5U, 7U, 7U, 7U}, 0U, 28U,
                          "EarthFar_v TEV stage 2 semantic decode changed");
        require(earth_far->blend.enabled && earth_far->blend.type == 0U && earth_far->blend.src_factor == 1U && earth_far->blend.dst_factor == 0U,
                "EarthFar_v should preserve original no-blend state");
        require(earth_far->cull_mode == 0U, "EarthFar_v should preserve original GX_CULL_NONE state");
        require(earth_far->z_mode.enabled && earth_far->z_mode.compare_enable == 1U && earth_far->z_mode.function == 3U &&
                    earth_far->z_mode.update_enable == 1U,
                "EarthFar_v should preserve original GX_LEQUAL write-enabled Z mode");
        const auto earth_passes = smgpc::game::j3d_material_texture_passes(*earth_far);
        require(earth_passes.size() == 3U, "EarthFar_v should build three runtime texture passes from TEV order");
        require(earth_passes[0U].texture_index == 9U && earth_passes[1U].texture_index == 11U && earth_passes[2U].texture_index == 10U,
                "EarthFar_v runtime passes should preserve earth/far/cloud texture order");
        const auto representative_earth_pass = smgpc::game::j3d_representative_texture_pass(*earth_far);
        require(representative_earth_pass.has_value() && representative_earth_pass->texture_index == 9U,
                "EarthFar_v representative pass should use the original base earth texture map");

        const auto* sun = find_material("Sun_Mat_v");
        require(sun != nullptr, "CometNearOrbitSky should expose Sun_Mat_v");
        require(sun->textures.size() == 1U && sun->textures[0U].texture_index == 4U, "Sun_Mat_v should bind PlanetSun");
        require(sun->tev_stages.size() == 1U, "Sun_Mat_v should expose its original single TEV stage");
        require_tev_stage(sun->tev_stages[0U], {15U, 8U, 10U, 15U}, 12U, {7U, 4U, 5U, 7U}, 1U, 28U, "Sun_Mat_v TEV stage 0 semantic decode changed");
        require(sun->blend.enabled && sun->blend.type == 1U && sun->blend.src_factor == 4U && sun->blend.dst_factor == 1U,
                "Sun_Mat_v should preserve original additive blend state");

        const auto& space_shape = model.shapes->shapes.at(7U);
        require(space_shape.material_index == 7U, "CometNearOrbitSky shape 7 should use Space_Mat_v");
        require(space_shape.joint_index == 7U, "CometNearOrbitSky Space_Mat_v shape should be attached to Obit joint");
        require(space_shape.draw_order == 7U, "CometNearOrbitSky Space_Mat_v should keep INF1 draw order");
        require(space_shape.display_list_bytes == 3232U, "Space_Mat_v shape display list size changed");
        require(space_shape.parsed_display_list_bytes == space_shape.display_list_bytes, "Space_Mat_v shape display list should parse fully");
        require(space_shape.triangle_count == 480U, "Space_Mat_v triangle count changed");

        const auto& sky_shape = model.shapes->shapes.at(8U);
        require(sky_shape.material_index == 6U, "CometNearOrbitSky shape 8 should use Sky_Mat_v");
        require(sky_shape.draw_order == 6U, "CometNearOrbitSky Sky_Mat_v should draw before Space_Mat_v per INF1");

        const auto& sun_shape = model.shapes->shapes.at(6U);
        require(sun_shape.material_index == 8U, "CometNearOrbitSky shape 6 should use Sun_Mat_v");
        require(sun_shape.joint_index == 7U, "CometNearOrbitSky Sun_Mat_v shape should be attached to Obit joint");
        require(sun_shape.draw_order == 8U, "CometNearOrbitSky Sun_Mat_v should keep INF1 draw order");
        require(sun_shape.triangle_count == 16U, "Sun_Mat_v triangle count changed");
    }

    void test_j3d_comet_near_orbit_sky_animation_probe(const std::filesystem::path& root) {
        const auto sky_archive = smgpc::game::RarcArchive::from_file(root / "ObjectData" / "CometNearOrbitSky.arc");
        const auto bck = smgpc::game::inspect_j3d_animation(sky_archive.file_data("cometnearorbitsky.bck"));
        require(bck.type == "bck1", "CometNearOrbitSky BCK file type changed");
        require(bck.sections.size() == 1U && bck.sections[0U].tag == "ANK1", "CometNearOrbitSky BCK should contain one ANK1 section");
        require(bck.bck.has_value(), "CometNearOrbitSky BCK should expose ANK1 summary");
        require(bck.bck->frame_max == 3000, "CometNearOrbitSky BCK frame max changed");
        require(bck.bck->joint_count == 8U, "CometNearOrbitSky BCK joint count changed");
        require(bck.bck->rotation_fraction == 1U, "CometNearOrbitSky BCK rotation fraction changed");
        require(bck.bck->scale_count == 1U, "CometNearOrbitSky BCK scale value count changed");
        require(bck.bck->rotation_count == 16U, "CometNearOrbitSky BCK rotation value count changed");
        require(bck.bck->translation_count == 10U, "CometNearOrbitSky BCK translation value count changed");
        require(bck.bck->scale_values.size() == 1U, "CometNearOrbitSky BCK scale values should be decoded");
        require(bck.bck->rotation_values.size() == 16U, "CometNearOrbitSky BCK rotation values should be decoded");
        require(bck.bck->translation_values.size() == 10U, "CometNearOrbitSky BCK translation values should be decoded");

        const auto root_joint = smgpc::game::j3d_evaluate_bck_joint_transform(*bck.bck, 0U, 1500.0F);
        require(root_joint.has_value(), "CometNearOrbitSky BCK should evaluate root joint transform");
        require_near(root_joint->scale[0U], 1.0F, 0.001F, "CometNearOrbitSky BCK root X scale changed");
        require(root_joint->rotation[0U] == 0 && root_joint->rotation[1U] == 0 && root_joint->rotation[2U] == 0,
                "CometNearOrbitSky BCK root rotation should remain identity");

        const auto orbit_joint = smgpc::game::j3d_evaluate_bck_joint_transform(*bck.bck, 3U, 1500.0F);
        require(orbit_joint.has_value(), "CometNearOrbitSky BCK should evaluate animated orbit joint transform");
        require(orbit_joint->rotation[0U] == 32686 && orbit_joint->rotation[2U] == 32686,
                "CometNearOrbitSky BCK orbit joint half-frame rotation changed");
        require_near(orbit_joint->translation[0U], 518043.0F, 0.5F, "CometNearOrbitSky BCK orbit joint X translation changed");
        require(!smgpc::game::j3d_evaluate_bck_joint_transform(*bck.bck, 8U, 0.0F).has_value(),
                "CometNearOrbitSky BCK should reject out-of-range joint indices");

        const auto btk = smgpc::game::inspect_j3d_animation(sky_archive.file_data("cometnearorbitsky.btk"));
        require(btk.type == "btk1", "CometNearOrbitSky BTK file type changed");
        require(btk.sections.size() == 1U && btk.sections[0U].tag == "TTK1", "CometNearOrbitSky BTK should contain one TTK1 section");
        require(btk.btk.has_value(), "CometNearOrbitSky BTK should expose TTK1 summary");
        require(btk.btk->frame_max == 10000, "CometNearOrbitSky BTK frame max changed");
        require(btk.btk->track_count == 15U, "CometNearOrbitSky BTK track count changed");
        require(btk.btk->scale_count == 10U, "CometNearOrbitSky BTK scale value count changed");
        require(btk.btk->rotation_count == 3U, "CometNearOrbitSky BTK rotation value count changed");
        require(btk.btk->translation_count == 284U, "CometNearOrbitSky BTK translation value count changed");
        require(btk.btk->scale_values.size() == 10U, "CometNearOrbitSky BTK scale values should be decoded");
        require(btk.btk->rotation_values.size() == 3U, "CometNearOrbitSky BTK rotation values should be decoded");
        require(btk.btk->translation_values.size() == 284U, "CometNearOrbitSky BTK translation values should be decoded");
        require(btk.btk->materials.size() == 5U, "CometNearOrbitSky BTK material update count changed");
        require(btk.btk->materials[3U].material_name == "EarthFar_v", "CometNearOrbitSky BTK should animate EarthFar_v");
        require(btk.btk->materials[3U].tex_matrix_id == 2U, "CometNearOrbitSky BTK EarthFar_v texture matrix id changed");
        require_near(btk.btk->materials[3U].center[0U], 0.5F, 0.001F, "CometNearOrbitSky BTK SRT center X changed");

        const auto earth_start = smgpc::game::j3d_evaluate_btk_texture_srt(*btk.btk, "EarthFar_v", 2U, 0.0F);
        require(earth_start.has_value(), "CometNearOrbitSky BTK should evaluate EarthFar_v matrix 2 at frame 0");
        require_near(earth_start->scale_s, 0.1F, 0.001F, "CometNearOrbitSky BTK EarthFar_v initial S scale changed");
        require_near(earth_start->scale_t, 0.5F, 0.001F, "CometNearOrbitSky BTK EarthFar_v initial T scale changed");
        require_near(earth_start->translate_s, 0.0F, 0.001F, "CometNearOrbitSky BTK EarthFar_v initial S translation changed");
        require_near(earth_start->translate_t, 0.0F, 0.001F, "CometNearOrbitSky BTK EarthFar_v initial T translation changed");

        const auto earth_middle = smgpc::game::j3d_evaluate_btk_texture_srt(*btk.btk, "EarthFar_v", 2U, 5000.0F);
        require(earth_middle.has_value(), "CometNearOrbitSky BTK should evaluate EarthFar_v matrix 2 at half-frame");
        require_near(earth_middle->translate_s, 0.5F, 0.001F, "CometNearOrbitSky BTK EarthFar_v half-frame S translation changed");
        require_near(earth_middle->translate_t, 0.5F, 0.001F, "CometNearOrbitSky BTK EarthFar_v half-frame T translation changed");
        require(!smgpc::game::j3d_evaluate_btk_texture_srt(*btk.btk, "EarthFar_v", 0U, 5000.0F).has_value(),
                "CometNearOrbitSky BTK should only match the material's animated texture matrix id");
    }

    void test_png_screenshot_service() {
        const std::array< std::uint8_t, 16U > pixels{
            255U, 0U, 0U, 255U, 0U, 255U, 0U, 255U, 0U, 0U, 255U, 255U, 255U, 255U, 255U, 255U,
        };

        const auto output = std::filesystem::temp_directory_path() / "smg-pc-png-screenshot-service-test.png";
        const auto screenshot_service = smgpc::render::capture::create_png_screenshot_service();
        screenshot_service->write_png(output, smgpc::render::capture::ScreenshotImageView{
                                                  .width = 2U,
                                                  .height = 2U,
                                                  .pitch = 8U,
                                                  .pixels = std::span< const std::uint8_t >(pixels.data(), pixels.size()),
                                                  .format = smgpc::render::capture::PixelFormat::RGBA8,
                                                  .origin_bottom_left = false,
                                              });

        const auto png = read_file(output);
        require(png.size() > 64U, "PNG service output is too small");
        require(png[0] == 0x89U && png[1] == 0x50U && png[2] == 0x4eU && png[3] == 0x47U, "PNG service output missing signature");
        require(read_be32(png, 8U) == 13U, "PNG service IHDR length mismatch");
        require(png[12] == 'I' && png[13] == 'H' && png[14] == 'D' && png[15] == 'R', "PNG service output missing IHDR");
        require(read_be32(png, 16U) == 2U, "PNG service width mismatch");
        require(read_be32(png, 20U) == 2U, "PNG service height mismatch");
        require(png[24] == 8U, "PNG service bit depth mismatch");
        require(png[25] == 6U, "PNG service color type mismatch");
    }

    void test_j3d_single_texture_tev_composition() {
        auto material = smgpc::game::J3dMaterialSummary{};
        material.name = "synthetic-sky";
        material.material_colors[0U] = {149U, 195U, 165U, 255U};
        material.tev_k_colors[0U] = {0U, 28U, 43U, 255U};
        material.tev_stage_count = 1U;
        material.textures.push_back(smgpc::game::J3dMaterialTextureBinding{
            .slot = 0U,
            .texture_index = 0U,
        });
        material.tev_orders.push_back(smgpc::game::J3dTevOrderSummary{
            .stage = 0U,
            .tex_coord = 0U,
            .tex_map = 0U,
            .color_channel = 4U,
        });
        material.tev_stages.push_back(smgpc::game::J3dTevStageSummary{
            .stage = 0U,
            .color_in = {14U, 10U, 8U, 15U},
            .color_op = 0U,
            .color_bias = 0U,
            .color_scale = 1U,
            .color_clamp = 1U,
            .color_out = 0U,
            .k_color_sel = 12U,
            .alpha_in = {7U, 4U, 5U, 7U},
            .alpha_op = 0U,
            .alpha_bias = 0U,
            .alpha_scale = 0U,
            .alpha_clamp = 1U,
            .alpha_out = 0U,
            .k_alpha_sel = 28U,
        });

        const auto source = smgpc::game::DecodedTexture{
            .width = 2U,
            .height = 1U,
            .format = smgpc::game::TplTextureFormat::I8,
            .rgba = {0U, 0U, 0U, 0U, 255U, 255U, 255U, 255U},
        };
        const auto composed = smgpc::game::j3d_try_compose_material_texture(material, source, material.material_colors[0U], 0U);
        require(composed.has_value(), "single-texture J3D TEV material should compose");
        require(composed->raster_color_baked, "single-texture J3D TEV material should mark raster color as baked");
        require(composed->image.width == 2U && composed->image.height == 1U, "single-texture J3D TEV composition dimensions changed");
        require(composed->image.rgba[0U] == 0U && composed->image.rgba[1U] == 56U && composed->image.rgba[2U] == 86U &&
                    composed->image.rgba[3U] == 0U,
                "single-texture J3D TEV composition did not apply konst-to-raster color ramp at texel 0");
        require(composed->image.rgba[4U] == 255U && composed->image.rgba[5U] == 255U && composed->image.rgba[6U] == 255U &&
                    composed->image.rgba[7U] == 255U,
                "single-texture J3D TEV composition did not clamp raster color at texel 1");
    }

    void test_wii_logical_render_viewport() {
        require(smgpc::render::core::kWiiLogicalFramebufferWidth == 640U, "logical Wii framebuffer width should match Dolphin title captures");
        require(smgpc::render::core::kWiiLogicalFramebufferHeight == 456U, "logical Wii framebuffer height should match Dolphin title captures");
    }

    void test_file_selector_title_autorush_gate() {
        auto selector = FileSelector();
        require(selector.getSkyStep() == 0U, "FileSelector sky actor step should start at zero");
        require(!selector.isTitleStarted(), "FileSelector title should not start before AutoRushBegin");
        require(!selector.isTitleActive(), "FileSelector::createTitle should keep TitleSequenceProduct killed before AutoRushBegin");
        require(!selector.receiveOtherMsg(0U), "FileSelector should reject unrelated messages");

        selector.update();
        require(!selector.isTitleStarted(), "FileSelector WaitBind should not start the title by itself");
        require(selector.receiveOtherMsg(ACTMES_UPDATE_BASEMTX), "FileSelector should accept UpdateBaseMtx messages");
        require(selector.receiveOtherMsg(ACTMES_AUTORUSH_BEGIN), "FileSelector should accept AutoRushBegin while waiting for bind");
        require(!selector.isTitleStarted(), "FileSelector should defer title startup until the Title nerve first step");

        selector.update();
        require(selector.isTitleStarted(), "FileSelector should start TitleSequenceProduct on the first Title nerve step");
        require(selector.isTitleActive(), "FileSelector title should be active after AutoRushBegin");
        require(!selector.receiveOtherMsg(ACTMES_AUTORUSH_BEGIN), "FileSelector should not restart title outside WaitBind");
    }

}  // namespace

int main() try {
    const auto root = disc_files_root();
    test_yaz0_decompression(root / "KrKorean" / "LayoutData" / "TitleLogo.arc");
    test_rarc_title_archives(root);
    test_tpl_title_texture_decode(root);
    test_brlyt_title_picture_parse(root);
    test_brlyt_press_start_text_parse(root);
    test_brfnt_message_font_decode(root);
    test_bcsv_file_select_camera_parse(root);
    test_camera_param_file_select_chunk_load(root);
    test_file_select_title_camera_pose();
    test_jmath_short_trig_compat();
    test_file_select_sky_runtime();
    test_brlan_title_animation_parse(root);
    test_brlan_press_start_animation_parse(root);
    test_j3d_comet_near_orbit_sky_textures(root);
    test_j3d_comet_near_orbit_sky_model_probe(root);
    test_j3d_comet_near_orbit_sky_animation_probe(root);
    test_png_screenshot_service();
    test_j3d_single_texture_tev_composition();
    test_wii_logical_render_viewport();
    test_file_selector_title_autorush_gate();
    std::cout << "compat layer tests passed\n";
    return 0;
} catch (const std::exception& e) {
    std::cerr << "compat layer tests failed: " << e.what() << '\n';
    return 1;
}
