#include "TestSuites.hpp"
#include "TestSupport.hpp"

namespace smgpc::tests {
    namespace {
        constexpr auto kTestSuite = std::string_view{"resource/layout"};

        template <int Line>
        struct TestCase;

        $test("decompresses Yaz0 title archive into RARC bytes") {
            const auto root = disc_files_root();
            const auto title_logo_path = root / "KrKorean" / "LayoutData" / "TitleLogo.arc";
            const auto compressed = read_file(title_logo_path);
            require(smgpc::game::is_yaz0(compressed), "TitleLogo.arc should be Yaz0-compressed");

            const auto decompressed = smgpc::game::decompress_yaz0(compressed);
            require_magic(decompressed, "RARC");
            require(read_be32(decompressed, 0x04U) == decompressed.size(), "RARC header file size should match decompressed size");
        }

        $test("mounts original title RARC layout archives") {
            const auto root = disc_files_root();
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

        $test("decodes Korean title logo TPL texture") {
            const auto root = disc_files_root();
            const auto title_logo = smgpc::game::RarcArchive::from_file(root / "KrKorean" / "LayoutData" / "TitleLogo.arc");
            const auto title_texture = smgpc::game::decode_tpl_texture(title_logo.file_data("timg/mytitlelogokor.tpl"));
            require(title_texture.width == 272U, "mytitlelogokor.tpl width changed");
            require(title_texture.height == 32U, "mytitlelogokor.tpl height changed");
            require(title_texture.format == smgpc::game::TplTextureFormat::I4, "mytitlelogokor.tpl format changed");
            require(title_texture.rgba.size() == static_cast<std::size_t>(title_texture.width) * title_texture.height * 4U,
                    "decoded title texture size mismatch");

            auto visible_pixels = 0U;
            for (std::size_t offset = 3U; offset < title_texture.rgba.size(); offset += 4U) {
                if (title_texture.rgba[offset] != 0U) {
                    ++visible_pixels;
                }
            }
            require(visible_pixels > 1000U, "decoded title texture should contain visible pixels");
        }

        $test("uses NW4R default alpha blend for BRLYT materials without blend blocks") {
            auto material = smgpc::game::BrlytMaterial{};
            material.name = "DefaultBlendMaterial";
            require(!material.blend_mode.enabled, "test material should model an omitted BRLYT blend block");

            const auto state = smgpc::game::gx_state_from_brlyt_material(material);
            require(state.blend.enabled && state.blend.type == 1U && state.blend.src_factor == 4U && state.blend.dst_factor == 5U &&
                        state.blend.op == 15U,
                    "BRLYT GX state should match NW4R's default alpha blend when no blend block is present");
            require(state.blend.color_update && state.blend.alpha_update,
                    "BRLYT default GX blend state should keep color and alpha writes enabled");
        }

        $test("parses TitleLogo BRLYT picture panes and GX material state") {
            const auto root = disc_files_root();
            const auto title_logo = smgpc::game::RarcArchive::from_file(root / "KrKorean" / "LayoutData" / "TitleLogo.arc");
            const auto layout = smgpc::game::parse_brlyt_layout(title_logo.file_data("blyt/titlelogo.brlyt"));
            require_near(layout.width, 608.0F, 0.001F, "titlelogo.brlyt layout width changed");
            require_near(layout.height, 456.0F, 0.001F, "titlelogo.brlyt layout height changed");
            require(!layout.panes.empty(), "titlelogo.brlyt should expose pane hierarchy");
            require(!layout.texture_names.empty(), "titlelogo.brlyt should reference textures");
            require(!layout.materials.empty(), "titlelogo.brlyt should expose materials");
            require(!layout.pictures.empty(), "titlelogo.brlyt should contain picture panes");

            const auto title_root = std::ranges::find_if(layout.panes, [](const auto &pane) { return pane.name == "SMGTitleLogo"; });
            require(title_root != layout.panes.end(), "titlelogo.brlyt should contain SMGTitleLogo pane");
            require(title_root->scale_x == 0.0F && title_root->scale_y == 0.0F, "SMGTitleLogo base scale should remain animation-driven");

            const auto title_picture =
                std::ranges::find_if(layout.pictures, [](const auto &picture) { return picture.texture_name == "MyTitleLogoKOR.tpl"; });
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

            const auto galaxy_picture = std::ranges::find_if(layout.pictures, [](const auto &picture) { return picture.name == "PicLogoGalaxy"; });
            require(galaxy_picture != layout.pictures.end(), "titlelogo.brlyt should keep the multi-texture PicLogoGalaxy picture");
            require(galaxy_picture->material_index == 0U, "PicLogoGalaxy material index changed");

            const auto &galaxy_material = layout.materials.at(galaxy_picture->material_index);
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
            require(galaxy_material.gx_state.source == "BRLYT", "PicLogoGalaxy should populate the shared GX material state as a BRLYT material");
            require(galaxy_material.gx_state.textures.size() == galaxy_material.textures.size(),
                    "PicLogoGalaxy GX state should preserve BRLYT texture bindings");
            require(galaxy_material.gx_state.tex_coord_gens.size() == galaxy_material.tex_coord_gens.size(),
                    "PicLogoGalaxy GX state should preserve BRLYT texgen bindings");
            require(galaxy_material.gx_state.tev_stages.size() == galaxy_material.tev_stages.size(),
                    "PicLogoGalaxy GX state should preserve BRLYT TEV stages");
            require(galaxy_material.gx_state.alpha_compare.enabled && galaxy_material.gx_state.blend.enabled,
                    "PicLogoGalaxy GX state should carry alpha compare and blend state");
        }

        $test("draws TitleLogo SimpleLayout through GX TEV material batches") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            auto runtime = smgpc::game::RuntimeContext(logger, window);
            auto renderer = RecordingRenderer();
            auto layout = SimpleLayout("TitleLogoProbe", "TitleLogo", 2U, MR::DrawType_Layout);

            layout.startAnim("Appear", 0U);
            layout.setAnimFrameAndStop(4.0F, 0U);
            layout.startAnim("Wait", 1U);
            layout.setAnimFrameAndStop(5000.0F, 1U);
            layout.appear();
            layout.draw(renderer);

            require(renderer.texture_count > 0U, "TitleLogo SimpleLayout should upload original BRLYT textures");
            require(renderer.gx_material_batch_count > 0U, "TitleLogo SimpleLayout should draw picture panes through GX material batches");
            require(renderer.quad_count == 0U, "TitleLogo SimpleLayout should not use the old textured-quad picture path");
            require(renderer.saw_gx_material_two_stage_batch && renderer.saw_gx_material_texture_stage_one,
                    "TitleLogo SimpleLayout should submit PicLogoGalaxy as a two-stage BRLYT TEV batch");
            require(renderer.saw_gx_material_nonzero_initial_register,
                    "TitleLogo SimpleLayout should pass BRLYT TEV register colors into the GX material batch");
            require(renderer.last_two_stage_gx_material_blend.enabled && renderer.last_two_stage_gx_material_blend.type == 1U,
                    "TitleLogo SimpleLayout should pass PicLogoGalaxy raw BRLYT GX blend state into the GX material batch");
        }

        $test("resolves disc root when launched from xmake target directory") {
            auto logger = NullLogger();
            auto window = TestWindowService();
            std::filesystem::path expected_root;
            {
                auto runtime = smgpc::game::RuntimeContext(logger, window);
                expected_root = runtime.dvd().root();
            }

            const auto nested_target_dir = std::filesystem::current_path() / "build" / "linux" / "x86_64" / "debug";
            std::filesystem::create_directories(nested_target_dir);

            {
                const auto scoped_path = ScopedCurrentPath(nested_target_dir);
                auto nested_logger = NullLogger();
                auto nested_window = TestWindowService();
                auto runtime = smgpc::game::RuntimeContext(nested_logger, nested_window);
                require(runtime.dvd().root() == expected_root, "RuntimeContext should resolve original disc files from xmake run targetdir");
                require(runtime.find_layout_archive("TitleLogo").has_value(), "RuntimeContext should find TitleLogo when xmake run starts in targetdir");
                require(runtime.find_object_archive("CometNearOrbitSky").has_value(),
                        "RuntimeContext should find CometNearOrbitSky when xmake run starts in targetdir");
            }
        }

        $test("parses PressStart BRLYT text boxes and glyph mapping") {
            const auto root = disc_files_root();
            const auto press_start = smgpc::game::RarcArchive::from_file(root / "KrKorean" / "LayoutData" / "PressStart.arc");
            const auto layout = smgpc::game::parse_brlyt_layout(press_start.file_data("blyt/pressstart.brlyt"));
            require_near(layout.width, 608.0F, 0.001F, "pressstart.brlyt layout width changed");
            require_near(layout.height, 456.0F, 0.001F, "pressstart.brlyt layout height changed");
            require(!layout.font_names.empty(), "pressstart.brlyt should reference font resources");
            require(!layout.panes.empty(), "pressstart.brlyt should expose pane hierarchy");
            require(!layout.text_boxes.empty(), "pressstart.brlyt should contain text boxes");

            const auto shadow = std::ranges::find_if(layout.text_boxes, [](const auto &text_box) { return text_box.name == "ShaStart"; });
            require(shadow != layout.text_boxes.end(), "pressstart.brlyt should contain ShaStart shadow text box");
            require(shadow->material_index == 0U, "ShaStart should use the original shadow material");
            require(shadow->color_mapping_max[0U] == 0U && shadow->color_mapping_max[1U] == 0U && shadow->color_mapping_max[2U] == 0U,
                    "ShaStart material should map glyph color to black");
            require(shadow->color_mapping_max[3U] == 100U, "ShaStart material should preserve original shadow alpha mapping");

            const auto prompt = std::ranges::find_if(layout.text_boxes, [](const auto &text_box) { return text_box.name == "TxtStart"; });
            require(prompt != layout.text_boxes.end(), "pressstart.brlyt should contain TxtStart text box");
            require(prompt->material_index == 1U, "TxtStart should use the original foreground text material");
            require(prompt->color_mapping_max[0U] == 255U && prompt->color_mapping_max[1U] == 255U && prompt->color_mapping_max[2U] == 255U,
                    "TxtStart material should map glyph color to white");
            require(prompt->font_name == "MessageFont26kor.brfnt", "TxtStart should use the original Korean message font");
            require(prompt->font_width > 0.0F && prompt->font_height > 0.0F, "TxtStart font size should be positive");

            const std::array<std::uint16_t, 11U> expected_text{
                0xff21U,
                0xc640U,
                0x0042U,
                0xb97cU,
                0x0020U,
                0xb20cU,
                0xb7ecU,
                0x0020U,
                0xc8fcU,
                0xc138U,
                0xc694U,
            };
            require(prompt->text.size() == expected_text.size(), "TxtStart text length changed");
            require(std::ranges::equal(prompt->text, expected_text), "TxtStart UTF-16BE text changed");
        }

        $test("decodes message BRFNT sheets and glyphs") {
            const auto root = disc_files_root();
            const auto font_archive = smgpc::game::RarcArchive::from_file(root / "KrKorean" / "LayoutData" / "Font.arc");
            const auto *font_entry = find_entry_by_basename(font_archive, "MessageFont26.brfnt");
            require(font_entry != nullptr, "Font.arc should contain MessageFont26.brfnt");

            const auto font = smgpc::game::parse_brfnt_font(font_archive.file_data(*font_entry));
            require(!font.sheets.empty(), "MessageFont26.brfnt should contain decoded glyph sheets");
            require(font.sheet_width > 0U && font.sheet_height > 0U, "MessageFont26.brfnt sheet dimensions should be positive");
            require(font.width > 0U && font.height > 0U, "MessageFont26.brfnt font dimensions should be positive");

            for (const auto code : std::array<std::uint16_t, 4U>{0xff21U, 0xc640U, 0x0042U, 0xb20cU}) {
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

        $test("parses FileSelect BCSV camera table") {
            const auto root = disc_files_root();
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

        $test("loads FileSelect camera parameter chunks") {
            const auto root = disc_files_root();
            const auto file_select = smgpc::game::RarcArchive::from_file(root / "StageData" / "FileSelect.arc");
            const auto table = smgpc::game::BcsvTable::from_bytes(file_select.file_data("camera/cameraparam.bcam"));
            const auto chunks = smgpc::game::load_camera_param_chunks(table);

            require(chunks.size() == 8U, "FileSelect camera chunk count changed");

            const auto &start = chunks[5U];
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

            const auto &follow = chunks[6U];
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

        $test("matches FileSelect title camera pose math") {
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

        $test("matches JMath short trig conversion helpers") {
            require(smgpc::game::jmath_sincos_table_index_from_short(0xffffU) == 0x3fffU, "JMath short trig index should use high 14 bits");
            require(smgpc::game::jmath_fctiwz_to_u16(2607.9F) == 2607U, "JMath fctiwz helper should truncate positive values toward zero");
            require(smgpc::game::jmath_fctiwz_to_u16(-1.9F) == 0xffffU, "JMath fctiwz helper should preserve low 16 bits for negative values");
            require_near(smgpc::game::jmath_cos_short(0x0000U), 1.0F, 0.000001F, "JMath cosShort(0) changed");
            require_near(smgpc::game::jmath_sin_short(0x4000U), 1.0F, 0.000001F, "JMath sinShort(0x4000) changed");
            require_near(smgpc::game::jmath_cos_short(0x8000U), -1.0F, 0.000001F, "JMath cosShort(0x8000) changed");
        }

        $test("matches FileSelectSky runtime matrix animation math") {
            require_near(smgpc::game::file_select_sky_yaw(1900U), 1.9F, 0.000001F, "FileSelectSky yaw update rate changed");
            require_near(smgpc::game::file_select_sky_pitch(0U), 0.0F, 0.000001F, "FileSelectSky pitch frame 0 changed");
            require_near(smgpc::game::file_select_sky_pitch(100U), 0.000400535F, 0.000001F,
                         "FileSelectSky pitch frame 100 should use JMath cosShort conversion");
            require_near(smgpc::game::file_select_sky_pitch(500U), 0.010059165F, 0.000001F,
                         "FileSelectSky pitch frame 500 should use JMath cosShort conversion");
            require_near(smgpc::game::file_select_sky_pitch(1500U), 0.089677349F, 0.000001F,
                         "FileSelectSky pitch frame 1500 should use JMath cosShort conversion");
            require_near(smgpc::game::file_select_sky_pitch(1900U), 0.142750874F, 0.000001F,
                         "FileSelectSky pitch frame 1900 should use JMath cosShort conversion");
            require_near(smgpc::game::file_select_sky_pitch(3000U), 0.345056713F, 0.000001F,
                         "FileSelectSky pitch frame 3000 should use JMath cosShort conversion");

            const auto identity_scaled = smgpc::game::file_select_sky_actor_matrix(0U);
            require_near(identity_scaled.m[0U], 0.8F, 0.000001F, "FileSelectSky frame 0 matrix X scale changed");
            require_near(identity_scaled.m[5U], 0.8F, 0.000001F, "FileSelectSky frame 0 matrix Y scale changed");
            require_near(identity_scaled.m[10U], 0.8F, 0.000001F, "FileSelectSky frame 0 matrix Z scale changed");

            const auto matrix = smgpc::game::file_select_sky_actor_matrix(1900U);
            require_near(matrix.m[0U], -0.258631736F, 0.000001F, "FileSelectSky frame 1900 matrix[0] changed");
            require_near(matrix.m[2U], -0.757040024F, 0.000001F, "FileSelectSky frame 1900 matrix[2] changed");
            require_near(matrix.m[4U], 0.107539982F, 0.000001F, "FileSelectSky frame 1900 matrix[4] changed");
            require_near(matrix.m[5U], 0.791887224F, 0.000001F, "FileSelectSky frame 1900 matrix[5] changed");
            require_near(matrix.m[6U], -0.036739472F, 0.000001F, "FileSelectSky frame 1900 matrix[6] changed");
            require_near(matrix.m[8U], 0.749362886F, 0.000001F, "FileSelectSky frame 1900 matrix[8] changed");
            require_near(matrix.m[9U], -0.113642588F, 0.000001F, "FileSelectSky frame 1900 matrix[9] changed");
            require_near(matrix.m[10U], -0.256008953F, 0.000001F, "FileSelectSky frame 1900 matrix[10] changed");
        }

        $test("parses TitleLogo BRLAN animations") {
            const auto root = disc_files_root();
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

        $test("parses PressStart BRLAN animations") {
            const auto root = disc_files_root();
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

        $test("extracts CometNearOrbitSky J3D textures") {
            const auto root = disc_files_root();
            const auto sky_archive = smgpc::game::RarcArchive::from_file(root / "ObjectData" / "CometNearOrbitSky.arc");
            const auto textures = smgpc::game::extract_j3d_textures(sky_archive.file_data("cometnearorbitsky.bdl"));
            require(textures.size() == 12U, "CometNearOrbitSky.bdl TEX1 texture count changed");

            const auto find_texture = [&textures](std::string_view name) -> const smgpc::game::J3dTexture * {
                const auto it = std::ranges::find_if(textures, [name](const auto &texture) { return texture.name == name; });
                return it == textures.end() ? nullptr : &*it;
            };

            const auto *orbit_universe = find_texture("OrbitUniverseL");
            require(orbit_universe != nullptr, "CometNearOrbitSky should contain OrbitUniverseL");
            require(orbit_universe->image.width == 1024U && orbit_universe->image.height == 512U, "OrbitUniverseL dimensions changed");
            require(orbit_universe->image.format == smgpc::game::TplTextureFormat::I4, "OrbitUniverseL should use GX I4");

            const auto *earth = find_texture("EarthKsMM");
            require(earth != nullptr, "CometNearOrbitSky should contain EarthKsMM");
            require(earth->image.width == 256U && earth->image.height == 256U, "EarthKsMM dimensions changed");

            const auto *galaxy = find_texture("Galaxy");
            require(galaxy != nullptr, "CometNearOrbitSky should contain Galaxy");
            require(galaxy->image.width == 64U && galaxy->image.height == 64U, "Galaxy dimensions changed");
            require(galaxy->image.format == smgpc::game::TplTextureFormat::CMPR, "Galaxy should exercise GX CMPR decoding");
            require(std::ranges::any_of(galaxy->image.rgba, [](std::uint8_t value) { return value != 0U; }),
                    "CMPR decoded Galaxy texture should not be blank");
        }

    }  // namespace

    void run_resource_layout_tests() {
        run_registered_tests(kTestSuite);
    }

}  // namespace smgpc::tests
