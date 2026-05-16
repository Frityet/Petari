#include "TestSuites.hpp"
#include "TestSupport.hpp"

#include <bit>

namespace smgpc::tests {
    namespace {
        constexpr auto kTestSuite = std::string_view{"j3d/gx"};

        template <int Line>
        struct TestCase;

        $test("decodes effective GX state from MDL3 display lists") {
            const auto append_bp = [](std::vector<std::uint8_t> &bytes, std::uint8_t address, std::uint32_t value) {
                bytes.push_back(0x61U);
                bytes.push_back(address);
                bytes.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xffU));
                bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
                bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
            };
            const auto append_xf = [](std::vector<std::uint8_t> &bytes, std::uint16_t address, std::initializer_list<std::uint32_t> values) {
                bytes.push_back(0x10U);
                const auto header = static_cast<std::uint32_t>(((values.size() - 1U) & 0x0fU) << 16U) | address;
                bytes.push_back(static_cast<std::uint8_t>((header >> 24U) & 0xffU));
                bytes.push_back(static_cast<std::uint8_t>((header >> 16U) & 0xffU));
                bytes.push_back(static_cast<std::uint8_t>((header >> 8U) & 0xffU));
                bytes.push_back(static_cast<std::uint8_t>(header & 0xffU));
                for (const auto value : values) {
                    bytes.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xffU));
                    bytes.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xffU));
                    bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
                    bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
                }
            };
            const auto f32 = [](float value) { return std::bit_cast<std::uint32_t>(value); };

            auto display_list = std::vector<std::uint8_t>{};
            append_bp(display_list, 0x00U, ((3U - 1U) << 10U) | (2U << 16U));
            append_xf(display_list, 0x1009U,
                      {
                          2U,
                          0x10203040U,
                          0x50607080U,
                          0x90a0b0c0U,
                          0xd0e0f011U,
                          1U | (1U << 1U) | (5U << 2U) | (1U << 6U) | (2U << 7U) | (3U << 9U) | (10U << 11U),
                          0x400U,
                          0U,
                          1U,
                      });
            append_xf(display_list, 0x0623U,
                      {
                          0x11223344U,
                          f32(1.0F),
                          f32(0.25F),
                          f32(0.0F),
                          f32(1.0F),
                          f32(0.0F),
                          f32(0.0F),
                          f32(0.0F),
                          f32(0.0F),
                          f32(100.0F),
                          f32(0.0F),
                          f32(0.0F),
                          f32(-1.0F),
                      });
            append_bp(display_list, 0x06U, 100U | ((0x7ecU & 0x7ffU) << 11U) | (1U << 22U));
            append_bp(display_list, 0x07U, 30U | (40U << 11U) | (2U << 22U));
            append_bp(display_list, 0x08U, (0x7fbU & 0x7ffU) | (60U << 11U) | (1U << 22U));
            append_bp(display_list, 0x10U,
                      1U | (2U << 2U) | (3U << 4U) | (2U << 7U) | (1U << 9U) | (2U << 11U) | (4U << 13U) | (5U << 16U) |
                          (1U << 19U) | (1U << 20U));
            append_bp(display_list, 0x25U, 1U | (2U << 4U) | (3U << 8U) | (4U << 12U));
            append_bp(display_list, 0x27U, 2U | (3U << 3U) | (4U << 6U) | (5U << 9U));
            append_bp(display_list, 0x28U, 2U | (3U << 3U) | (1U << 6U) | (4U << 7U));
            append_bp(display_list, 0x40U, 1U | (4U << 1U));
            append_bp(display_list, 0xc0U, (4U << 12U) | (14U << 8U) | (8U << 4U) | (1U << 19U) | (2U << 22U));
            append_bp(display_list, 0xc1U, (5U << 13U) | (6U << 10U) | (7U << 7U) | (1U << 19U) | (2U << 22U));
            append_bp(display_list, 0xe2U, 100U | (0x7fbU << 12U));
            append_bp(display_list, 0xe3U, 30U | (40U << 12U));
            append_bp(display_list, 0xe8U, 400U | (1U << 10U));
            append_bp(display_list, 0xe9U, 0x100U | (0x200U << 12U));
            append_bp(display_list, 0xeeU, 127U << 11U);
            append_bp(display_list, 0xefU, 123456U);
            append_bp(display_list, 0xf0U, 5U);
            append_bp(display_list, 0xf1U, (126U << 11U) | (1U << 20U) | (2U << 21U));
            append_bp(display_list, 0xf2U, (17U << 16U) | (34U << 8U) | 51U);
            append_bp(display_list, 0xf3U, (2U << 16U) | 10U | (1U << 22U) | (5U << 19U) | (20U << 8U));
            append_bp(display_list, 0xf6U, (14U << 4U) | (28U << 9U));

            auto state = smgpc::game::GXMaterialState{};
            smgpc::game::gx_apply_mdl3_display_list(state, display_list);

            require(state.color_channel_count == 2U, "MDL3 XF num-channel load should update effective color-channel count");
            require(state.color_channels[0U].ambient_color == smgpc::game::GXColorValue{0x10U, 0x20U, 0x30U, 0x40U} &&
                        state.color_channels[1U].ambient_color == smgpc::game::GXColorValue{0x50U, 0x60U, 0x70U, 0x80U},
                    "MDL3 XF ambient color loads should preserve RGBA bytes");
            require(state.color_channels[0U].material_color == smgpc::game::GXColorValue{0x90U, 0xa0U, 0xb0U, 0xc0U} &&
                        state.color_channels[1U].material_color == smgpc::game::GXColorValue{0xd0U, 0xe0U, 0xf0U, 0x11U},
                    "MDL3 XF material color loads should preserve RGBA bytes");
            require(state.color_channels[0U].color_control.material_source == 1U && state.color_channels[0U].color_control.lighting_enabled &&
                        state.color_channels[0U].color_control.light_mask == 0xa5U &&
                        state.color_channels[0U].color_control.ambient_source == 1U &&
                        state.color_channels[0U].color_control.diffuse_function == 2U &&
                        state.color_channels[0U].color_control.attenuation_function == 1U &&
                        state.color_channels[0U].color_control.attenuation_mode == 3U,
                    "MDL3 XF color channel control should decode Dolphin LitChannel bits");
            require(state.color_channels[0U].alpha_control.attenuation_function == 2U &&
                        state.color_channels[0U].alpha_control.attenuation_mode == 0U &&
                        state.color_channels[1U].alpha_control.material_source == 1U,
                    "MDL3 XF alpha channel controls should decode independently from color controls");
            require(state.lights[2U].loaded && state.lights[2U].color == smgpc::game::GXColorValue{0x11U, 0x22U, 0x33U, 0x44U},
                    "MDL3 XF light object loads should preserve light color bytes");
            require_near(state.lights[2U].cosine_attenuation[0U], 1.0F, 0.0001F,
                         "MDL3 XF light object loads should decode cosine attenuation floats");
            require_near(state.lights[2U].cosine_attenuation[1U], 0.25F, 0.0001F,
                         "MDL3 XF light object loads should decode cosine attenuation floats");
            require_near(state.lights[2U].distance_attenuation[0U], 1.0F, 0.0001F,
                         "MDL3 XF light object loads should decode distance attenuation floats");
            require_near(state.lights[2U].position[2U], 100.0F, 0.0001F, "MDL3 XF light object loads should decode position floats");
            require_near(state.lights[2U].direction[2U], -1.0F, 0.0001F, "MDL3 XF light object loads should decode direction floats");
            require(state.lights[2U].word_loaded[3U] && state.lights[2U].word_loaded[15U],
                    "MDL3 XF light object loads should retain raw word load evidence");
            require(state.tev_stage_count == 3U, "MDL3 gen-mode BP load should update effective TEV stage count");
            require(state.indirect.stage_count == 2U, "MDL3 gen-mode BP load should update effective indirect stage count");
            require(state.indirect.texture_matrices.size() == 1U && state.indirect.texture_matrices[0U].matrix == 0U &&
                        state.indirect.texture_matrices[0U].ma == 100 && state.indirect.texture_matrices[0U].mb == -20 &&
                        state.indirect.texture_matrices[0U].mc == 30 && state.indirect.texture_matrices[0U].md == 40 &&
                        state.indirect.texture_matrices[0U].me == -5 && state.indirect.texture_matrices[0U].mf == 60 &&
                        state.indirect.texture_matrices[0U].scale == 25U,
                    "MDL3 indirect matrix BP loads should preserve signed 2x3 matrix rows and scale bits");
            require(state.indirect.tev_stages.size() == 1U && state.indirect.tev_stages[0U].tev_stage == 0U &&
                        state.indirect.tev_stages[0U].ind_stage == 1U && state.indirect.tev_stages[0U].format == 2U &&
                        state.indirect.tev_stages[0U].bias == 3U && state.indirect.tev_stages[0U].bump_alpha == 2U &&
                        state.indirect.tev_stages[0U].matrix_index == 1U && state.indirect.tev_stages[0U].matrix_id == 2U &&
                        state.indirect.tev_stages[0U].wrap_s == 4U && state.indirect.tev_stages[0U].wrap_t == 5U &&
                        state.indirect.tev_stages[0U].use_original_lod && state.indirect.tev_stages[0U].add_previous &&
                        state.indirect.tev_stages[0U].active,
                    "MDL3 indirect TEV BP loads should preserve stage command semantics");
            require(state.indirect.texture_coord_scales.size() == 2U && state.indirect.texture_coord_scales[0U].stage == 0U &&
                        state.indirect.texture_coord_scales[0U].scale_s == 1U && state.indirect.texture_coord_scales[0U].scale_t == 2U &&
                        state.indirect.texture_coord_scales[1U].stage == 1U && state.indirect.texture_coord_scales[1U].scale_s == 3U &&
                        state.indirect.texture_coord_scales[1U].scale_t == 4U,
                    "MDL3 indirect texture scale BP loads should preserve paired stage scale exponents");
            require(state.indirect.texture_orders.size() == 4U && state.indirect.texture_orders[0U].stage == 0U &&
                        state.indirect.texture_orders[0U].tex_map == 2U && state.indirect.texture_orders[0U].tex_coord == 3U &&
                        state.indirect.texture_orders[1U].stage == 1U && state.indirect.texture_orders[1U].tex_map == 4U &&
                        state.indirect.texture_orders[1U].tex_coord == 5U,
                    "MDL3 indirect texture order BP load should preserve indirect texture map and coordinate selectors");
            require(state.tev_orders.size() >= 2U && state.tev_orders[0U].tex_map == 2U && state.tev_orders[0U].tex_coord == 3U &&
                        state.tev_orders[0U].color_channel == 4U,
                    "MDL3 TEV order BP load should update effective texture order state");
            require(state.z_mode.enabled && state.z_mode.compare_enable == 1U && state.z_mode.function == 4U && state.z_mode.update_enable == 0U,
                    "MDL3 z-mode BP load should update effective depth state");
            require(state.tev_stages.size() >= 1U && state.tev_stages[0U].color_in == std::array<std::uint8_t, 4U>{4U, 14U, 8U, 0U} &&
                        state.tev_stages[0U].alpha_in == std::array<std::uint8_t, 4U>{5U, 6U, 7U, 0U} &&
                        state.tev_stages[0U].color_out == 2U && state.tev_stages[0U].alpha_out == 2U &&
                        state.tev_stages[0U].k_color_sel == 14U && state.tev_stages[0U].k_alpha_sel == 28U,
                    "MDL3 TEV BP loads should update effective stage and konst selector state");
            require(state.tev_registers[1U] == smgpc::game::GXTevRegisterColor{100, 40, 30, -5},
                    "MDL3 TEV color BP loads should update signed effective TEV registers");
            require(state.alpha_compare.enabled && state.alpha_compare.comp0 == 2U && state.alpha_compare.ref0 == 10U &&
                        state.alpha_compare.op == 1U && state.alpha_compare.comp1 == 5U && state.alpha_compare.ref1 == 20U,
                    "MDL3 alpha-compare BP load should update effective pixel-engine state");
            require(state.fog.enabled && state.fog.type == 2U && state.fog.projection == 1U && state.fog.range_adjust_enabled &&
                        state.fog.range_center == 400U && state.fog.b_magnitude == 123456U && state.fog.b_shift == 5U &&
                        state.fog.color == std::array<std::uint8_t, 4U>{17U, 34U, 51U, 255U},
                    "MDL3 fog BP loads should preserve typed fog selector, projection, range, B, and color state");
            require_near(state.fog.a, 1.0F, 0.0001F, "MDL3 fog A parameter should decode from GX float bits");
            require_near(state.fog.c, 0.5F, 0.0001F, "MDL3 fog C parameter should decode from GX float bits");
            require_near(state.fog.range_k[0U], 1.0F, 0.0001F, "MDL3 fog range K low sample should decode using Dolphin scale");
            require_near(state.fog.range_k[1U], 2.0F, 0.0001F, "MDL3 fog range K high sample should decode using Dolphin scale");
        }

        $test("evaluates lit GX raster colors from typed channel and light state") {
            auto state = smgpc::game::GXMaterialState{};
            state.color_channel_count = 1U;
            state.color_channels[0U].material_color = {100U, 100U, 100U, 100U};
            state.color_channels[0U].ambient_color = {10U, 10U, 10U, 10U};
            state.color_channels[0U].color_control =
                smgpc::game::gx_color_channel_control_from_j3d(1U, 0U, 1U << 0U, 2U, 2U, 0U);
            state.color_channels[0U].alpha_control = smgpc::game::gx_color_channel_control_from_j3d(0U, 0U, 0U, 0U, 2U, 0U);
            state.lights[0U].loaded = true;
            state.lights[0U].color = {255U, 0U, 0U, 255U};
            state.lights[0U].position = {0.0F, 0.0F, 10.0F};
            state.lights[0U].direction = {0.0F, 0.0F, -1.0F};

            const auto lit = smgpc::game::gx_evaluate_lit_raster_color(state, 4U, {255U, 255U, 255U, 255U},
                                                                       {0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F});
            require(lit[0U] == 100U && lit[1U] == 3U && lit[2U] == 3U && lit[3U] == 100U,
                    "GX raster color lighting should apply Dolphin-style ambient/light accumulator before material modulation");

            state.lights[0U].loaded = false;
            const auto ambient_only = smgpc::game::gx_evaluate_lit_raster_color(state, 4U, {255U, 255U, 255U, 255U},
                                                                                {0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F});
            require(ambient_only[0U] == 3U && ambient_only[1U] == 3U && ambient_only[2U] == 3U && ambient_only[3U] == 100U,
                    "GX raster color lighting should fall back to ambient when selected light objects have not been loaded");
        }

        $test("probes CometNearOrbitSky J3D model sections and materials") {
            const auto root = disc_files_root();
            const auto sky_archive = smgpc::game::RarcArchive::from_file(root / "ObjectData" / "CometNearOrbitSky.arc");
            const auto model_data = sky_archive.file_data("cometnearorbitsky.bdl");
            const auto model = smgpc::game::inspect_j3d_model(model_data);

            require(model.section_count == 9U, "CometNearOrbitSky.bdl section count changed");
            require(model.info.has_value(), "CometNearOrbitSky.bdl should expose INF1");
            require(model.vertices.has_value(), "CometNearOrbitSky.bdl should expose VTX1");
            require(model.joints.has_value(), "CometNearOrbitSky.bdl should expose JNT1");
            require(model.envelopes.has_value(), "CometNearOrbitSky.bdl should expose EVP1 envelope metadata");
            require(model.draw_matrices.has_value(), "CometNearOrbitSky.bdl should expose DRW1 draw matrix metadata");
            require(model.shapes.has_value(), "CometNearOrbitSky.bdl should expose SHP1");
            require(model.materials.has_value(), "CometNearOrbitSky.bdl should expose MAT3");
            require(model.mdl3.has_value(), "CometNearOrbitSky.bdl should expose MDL3 material display-list metadata");
            require(model.textures.size() == 12U, "CometNearOrbitSky.bdl should expose TEX1 textures through model probe");

            require(model.info->packet_count == 9U, "CometNearOrbitSky packet count changed");
            require(model.info->vertex_count == 1029U, "CometNearOrbitSky vertex count changed");
            require(model.info->hierarchy.size() == 71U, "CometNearOrbitSky hierarchy size changed");
            require(model.vertices->formats.size() == 4U, "CometNearOrbitSky VTX1 format count changed");
            require(std::ranges::any_of(model.vertices->formats, [](const auto &format) { return format.attr == 10U; }),
                    "CometNearOrbitSky VTX1 should expose original normal attribute format");
            require(model.joints->joint_count == 8U, "CometNearOrbitSky JNT1 joint count changed");
            require(model.joints->joints.size() == 8U, "CometNearOrbitSky JNT1 joints should be decoded");
            require(model.shapes->shape_count == 9U, "CometNearOrbitSky shape count changed");
            require(model.materials->material_count == 9U, "CometNearOrbitSky material count changed");
            require(model.joints->joints[0U].name == "world_root", "CometNearOrbitSky root joint name changed");
            require(model.joints->joints[7U].name == "Obit", "CometNearOrbitSky orbit joint name changed");
            require(model.joints->parent_indices.size() == 8U, "CometNearOrbitSky joint hierarchy parent map should be decoded");
            require(model.joints->parent_indices[0U] == 0xffffU && model.joints->parent_indices[1U] == 0U && model.joints->parent_indices[2U] == 1U &&
                        model.joints->parent_indices[3U] == 2U && model.joints->parent_indices[7U] == 0U,
                    "CometNearOrbitSky joint hierarchy parent map changed");
            require(model.envelopes->matrix_count == 0U, "CometNearOrbitSky EVP1 should be present but have no weighted envelopes");
            require(model.draw_matrices->matrices.size() == model.draw_matrices->matrix_count,
                    "CometNearOrbitSky DRW1 draw matrix table should parse to the declared entry count");
            require_near(model.joints->joints[7U].radius, 793869.0F, 0.5F, "CometNearOrbitSky orbit joint radius changed");
            require(model.mdl3->material_count == 9U && model.mdl3->packets.size() == 9U, "CometNearOrbitSky MDL3 packet count changed");
            require(std::ranges::all_of(model.mdl3->packets, [](const auto &packet) { return packet.size > 0U && !packet.bytes.empty(); }),
                    "CometNearOrbitSky MDL3 packets should preserve raw GX display-list bytes");

            const auto find_material = [&model](std::string_view name) -> const smgpc::game::J3dMaterialSummary * {
                const auto it = std::ranges::find_if(model.materials->materials, [name](const auto &material) { return material.name == name; });
                return it == model.materials->materials.end() ? nullptr : &*it;
            };

            const auto *space = find_material("Space_Mat_v");
            require(space != nullptr, "CometNearOrbitSky should expose Space_Mat_v");
            require(space->gx_state.source == "J3D", "Space_Mat_v should populate shared GX state as a J3D material");
            require(space->gx_state.textures.size() == space->textures.size(), "Space_Mat_v GX state should preserve texture bindings");
            require(space->gx_state.tev_orders.size() >= space->tev_orders.size(), "Space_Mat_v GX state should preserve TEV orders");
            require(space->gx_state.tev_stages.size() >= space->tev_stages.size(), "Space_Mat_v GX state should preserve TEV stages");
            require(space->color_channel_count == 1U && space->gx_state.color_channel_count == 1U,
                    "Space_Mat_v should preserve the original one-channel color block through effective GX state");
            require(space->gx_state.color_channels[0U].material_color == space->material_colors[0U] &&
                        space->gx_state.color_channels[0U].ambient_color == space->ambient_colors[0U],
                    "Space_Mat_v GX state should preserve MAT3/XF material and ambient channel colors");
            require(space->gx_state.color_channels[0U].color_control.raw == space->color_channel_controls[0U].raw &&
                        space->gx_state.color_channels[0U].alpha_control.raw == space->alpha_channel_controls[0U].raw,
                    "Space_Mat_v GX state should preserve MAT3/XF color and alpha channel controls");
            require(std::ranges::any_of(space->gx_state.mdl3_register_loads, [](const auto &load) {
                        return load.space == smgpc::game::GXRegisterSpace::XF && load.address >= 0x1009U && load.address <= 0x1011U;
                    }),
                    "Space_Mat_v GX state should retain XF color-channel register load evidence");
            require(std::ranges::any_of(space->gx_state.mdl3_register_loads, [](const auto &load) {
                        return load.space == smgpc::game::GXRegisterSpace::BP && load.address >= 0xe0U && load.address <= 0xe7U;
                    }),
                    "Space_Mat_v GX state should retain and apply MDL3 TEV register BP loads");
            require(!space->gx_state.mdl3_display_list.empty(), "Space_Mat_v GX state should preserve its MDL3 material packet");
            require(space->gx_state.mdl3_stats.bp_load_count > 0U && space->gx_state.mdl3_stats.xf_load_count > 0U,
                    "Space_Mat_v GX state should decode BP/XF loads from its MDL3 material packet");
            require(space->gx_state.mdl3_stats.parsed_bytes == space->gx_state.mdl3_display_list.size(),
                    "Space_Mat_v GX state should parse the full MDL3 material packet");
            require(space->gx_state.mdl3_stats.unknown_opcode_count == 0U, "Space_Mat_v GX state should recognize all MDL3 material-packet opcodes");
            require(std::ranges::any_of(space->gx_state.mdl3_register_loads, [](const auto &load) {
                        return load.space == smgpc::game::GXRegisterSpace::BP && load.address == 0x00U;
                    }),
                    "Space_Mat_v GX state should retain MDL3 gen-mode BP loads");
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
            require(space->gx_state.cull_mode == 1U, "Space_Mat_v GX state should preserve MDL3 hardware back-face cull state");
            require(space->z_mode.enabled && space->z_mode.compare_enable == 1U && space->z_mode.function == 3U && space->z_mode.update_enable == 0U,
                    "Space_Mat_v should preserve original test-only GX_LEQUAL Z mode");
            require(!space->gx_state.fog.enabled && space->gx_state.fog.type == 0U &&
                        space->gx_state.fog.color == std::array<std::uint8_t, 4U>{255U, 255U, 255U, 255U},
                    "Space_Mat_v MDL3 fog registers should decode to original GX_FOG_NONE state instead of a raw-load marker");
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
            const auto composed_space = smgpc::game::j3d_try_compose_material_texture(*space, model.textures, space_passes, space->material_colors[0U]);
            require(composed_space.has_value(), "Space_Mat_v should compose from its original material texture passes");
            require(composed_space->raster_color_baked, "Space_Mat_v pass composition should bake raster color into the texture");
            require(composed_space->image.width == 1024U && composed_space->image.height == 512U,
                    "Space_Mat_v pass composition should use the largest source texture dimensions");
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

            const auto *core_rock = find_material("CoreRock");
            require(core_rock != nullptr, "CometNearOrbitSky should expose CoreRock");
            require(smgpc::game::j3d_material_texture_passes(*core_rock).empty(), "CoreRock should be an untextured material");
            require(core_rock->gx_state.color_channels[0U].alpha_control.lighting_enabled &&
                        core_rock->gx_state.color_channels[0U].alpha_control.light_mask == 4U,
                    "CoreRock should preserve its original alpha-channel light mask for GX raster lighting");
            require(std::ranges::none_of(core_rock->gx_state.lights, [](const auto &light) { return light.loaded; }),
                    "CoreRock material packet should expose that its selected light comes from scene GX state rather than MDL3 local light loads");
            const auto composed_core_rock = smgpc::game::j3d_try_compose_material_constant(*core_rock, core_rock->material_colors[0U]);
            require(composed_core_rock.has_value(), "CoreRock should compose to a constant material texture");
            require(composed_core_rock->raster_color_baked, "CoreRock constant material composition should bake raster color");
            require(composed_core_rock->image.width == 1U && composed_core_rock->image.height == 1U,
                    "CoreRock constant material composition should produce one RGBA texel");

            const auto *comet_halo = find_material("CometHalo_v");
            require(comet_halo != nullptr, "CometNearOrbitSky should expose CometHalo_v");
            require(comet_halo->gx_state.indirect.stage_count == 1U, "CometHalo_v MDL3 GX state should preserve declared indirect stage count");
            require(!comet_halo->gx_state.indirect.texture_orders.empty(), "CometHalo_v MDL3 GX state should preserve indirect texture order registers");
            require(std::ranges::any_of(comet_halo->gx_state.indirect.tev_stages, [](const auto &stage) {
                        return stage.active && stage.tev_stage == 0U && stage.ind_stage == 0U;
                    }),
                    "CometHalo_v MDL3 GX state should preserve its active indirect TEV command");
            require(std::ranges::any_of(comet_halo->gx_state.indirect.texture_orders, [](const auto &order) {
                        return order.stage == 0U && order.tex_map == 1U && order.tex_coord == 1U;
                    }),
                    "CometHalo_v indirect stage 0 should sample the original texture-map and texcoord slots");
            const auto comet_halo_passes = smgpc::game::j3d_material_texture_passes(*comet_halo);
            require(comet_halo_passes.size() == 1U && comet_halo_passes[0U].texture_index == 2U && comet_halo_passes[0U].stage == 0U,
                    "CometHalo_v regular TEV order should still expose one base texture pass");
            const auto indirect_source = smgpc::game::J3dMeshVertex{
                .u = 3.0F / 8.0F,
                .v = 5.0F / 8.0F,
            };
            const auto indirect_trace =
                smgpc::game::j3d_trace_indirect_texture_transform(*comet_halo, model.textures, indirect_source, comet_halo_passes[0U]);
            require(indirect_trace.has_value(), "CometHalo_v should expose a traceable active indirect texture transform");
            require(indirect_trace->tev_stage == 0U && indirect_trace->indirect_stage == 0U && indirect_trace->indirect_tex_map == 1U &&
                        indirect_trace->indirect_tex_coord == 1U,
                    "CometHalo_v indirect trace should preserve Dolphin/GX stage and indirect order selectors");
            const auto base_coord = smgpc::game::j3d_transform_tex_coord(
                indirect_source, comet_halo_passes[0U].tex_coord_gen.has_value() ? &*comet_halo_passes[0U].tex_coord_gen : nullptr,
                comet_halo_passes[0U].tex_matrix.has_value() ? &*comet_halo_passes[0U].tex_matrix : nullptr);
            require_near(indirect_trace->base_coord.u, base_coord.u, 0.00001F,
                         "CometHalo_v indirect trace should use the regular pass texture coordinate as its base S coordinate");
            require_near(indirect_trace->base_coord.v, base_coord.v, 0.00001F,
                         "CometHalo_v indirect trace should use the regular pass texture coordinate as its base T coordinate");
            const auto indirect_binding = std::ranges::find_if(comet_halo->textures, [&indirect_trace](const auto &binding) {
                return binding.slot == indirect_trace->indirect_tex_map;
            });
            require(indirect_binding != comet_halo->textures.end() && indirect_binding->texture_index < model.textures.size(),
                    "CometHalo_v indirect trace should reference a valid indirect texture binding");
            const auto &base_texture = model.textures[comet_halo_passes[0U].texture_index].image;
            const auto &indirect_texture = model.textures[indirect_binding->texture_index].image;
            const auto shift_indirect_value = [](std::int64_t value, int shift) {
                if (shift >= 0) {
                    return value / (std::int64_t{1} << std::min(shift, 30));
                }

                return value * (std::int64_t{1} << std::min(-shift, 30));
            };
            const auto expected_base_ind_s = static_cast<std::int64_t>(
                std::llround(indirect_trace->indirect_coord.u * static_cast<float>(indirect_texture.width) * 128.0F));
            const auto expected_base_ind_t = static_cast<std::int64_t>(
                std::llround(indirect_trace->indirect_coord.v * static_cast<float>(indirect_texture.height) * 128.0F));
            require(indirect_trace->base_indirect_s == expected_base_ind_s && indirect_trace->base_indirect_t == expected_base_ind_t,
                    "CometHalo_v indirect trace should preserve Dolphin's 1/128 indirect texture coordinate basis");
            const auto scale = std::ranges::find_if(comet_halo->gx_state.indirect.texture_coord_scales, [&indirect_trace](const auto &entry) {
                return entry.stage == indirect_trace->indirect_stage;
            });
            const auto scale_s = scale == comet_halo->gx_state.indirect.texture_coord_scales.end() ? 0 : scale->scale_s;
            const auto scale_t = scale == comet_halo->gx_state.indirect.texture_coord_scales.end() ? 0 : scale->scale_t;
            require(indirect_trace->scaled_indirect_s == shift_indirect_value(indirect_trace->base_indirect_s, scale_s) &&
                        indirect_trace->scaled_indirect_t == shift_indirect_value(indirect_trace->base_indirect_t, scale_t),
                    "CometHalo_v indirect trace should apply GXSetIndTexCoordScale exponents before sampling");
            constexpr auto format_shifts = std::array<std::uint8_t, 4U>{0U, 3U, 4U, 5U};
            const auto format_shift = format_shifts[std::min<std::size_t>(indirect_trace->format, format_shifts.size() - 1U)];
            const auto bias_value = indirect_trace->format == 0U ? -128 : 1;
            const auto expected_biased = std::array<std::int32_t, 3U>{
                static_cast<std::int32_t>((indirect_trace->sampled_indirect_color[3U] >> format_shift) +
                                          ((indirect_trace->bias & 0x1U) != 0U ? bias_value : 0)),
                static_cast<std::int32_t>((indirect_trace->sampled_indirect_color[2U] >> format_shift) +
                                          ((indirect_trace->bias & 0x2U) != 0U ? bias_value : 0)),
                static_cast<std::int32_t>((indirect_trace->sampled_indirect_color[1U] >> format_shift) +
                                          ((indirect_trace->bias & 0x4U) != 0U ? bias_value : 0)),
            };
            require(indirect_trace->biased_indirect_coord == expected_biased,
                    "CometHalo_v indirect trace should apply Dolphin's ALP/BLU/GRN format and bias decode");
            const auto matrix = std::ranges::find_if(comet_halo->gx_state.indirect.texture_matrices, [&indirect_trace](const auto &entry) {
                return indirect_trace->matrix_index != 0U && entry.matrix == indirect_trace->matrix_index - 1U;
            });
            auto expected_translation = std::array<std::int64_t, 2U>{0, 0};
            if (matrix != comet_halo->gx_state.indirect.texture_matrices.end()) {
                switch (indirect_trace->matrix_id) {
                case 0U:
                    expected_translation[0U] = (static_cast<std::int64_t>(matrix->ma) * indirect_trace->biased_indirect_coord[0U] +
                                                static_cast<std::int64_t>(matrix->mc) * indirect_trace->biased_indirect_coord[1U] +
                                                static_cast<std::int64_t>(matrix->me) * indirect_trace->biased_indirect_coord[2U]) /
                                               8;
                    expected_translation[1U] = (static_cast<std::int64_t>(matrix->mb) * indirect_trace->biased_indirect_coord[0U] +
                                                static_cast<std::int64_t>(matrix->md) * indirect_trace->biased_indirect_coord[1U] +
                                                static_cast<std::int64_t>(matrix->mf) * indirect_trace->biased_indirect_coord[2U]) /
                                               8;
                    break;
                case 1U:
                    expected_translation[0U] = (indirect_trace->base_s * indirect_trace->biased_indirect_coord[0U]) / 256;
                    expected_translation[1U] = (indirect_trace->base_t * indirect_trace->biased_indirect_coord[0U]) / 256;
                    break;
                case 2U:
                    expected_translation[0U] = (indirect_trace->base_s * indirect_trace->biased_indirect_coord[1U]) / 256;
                    expected_translation[1U] = (indirect_trace->base_t * indirect_trace->biased_indirect_coord[1U]) / 256;
                    break;
                default:
                    break;
                }
                const auto matrix_shift = 17 - static_cast<int>(matrix->scale);
                expected_translation[0U] = shift_indirect_value(expected_translation[0U], matrix_shift);
                expected_translation[1U] = shift_indirect_value(expected_translation[1U], matrix_shift);
            }
            require(indirect_trace->translation == expected_translation,
                    "CometHalo_v indirect trace should match Dolphin's indirect matrix translation math");
            const auto wrap_indirect_coordinate = [](std::int64_t coord, std::uint8_t wrap) {
                switch (wrap) {
                case 0U:
                    return coord;
                case 1U:
                    return coord & ((std::int64_t{256} << 7U) - 1);
                case 2U:
                    return coord & ((std::int64_t{128} << 7U) - 1);
                case 3U:
                    return coord & ((std::int64_t{64} << 7U) - 1);
                case 4U:
                    return coord & ((std::int64_t{32} << 7U) - 1);
                case 5U:
                    return coord & ((std::int64_t{16} << 7U) - 1);
                default:
                    return std::int64_t{0};
                }
            };
            auto expected_transformed_s = wrap_indirect_coordinate(indirect_trace->base_s, indirect_trace->wrap_s) + indirect_trace->translation[0U];
            auto expected_transformed_t = wrap_indirect_coordinate(indirect_trace->base_t, indirect_trace->wrap_t) + indirect_trace->translation[1U];
            if (indirect_trace->add_previous) {
                expected_transformed_s += indirect_trace->base_s;
                expected_transformed_t += indirect_trace->base_t;
            }
            require(indirect_trace->transformed_s == expected_transformed_s && indirect_trace->transformed_t == expected_transformed_t,
                    "CometHalo_v indirect trace should apply wrap and add-previous exactly like Dolphin software TEV");
            require_near(indirect_trace->transformed_coord.u,
                         static_cast<float>(expected_transformed_s) / (static_cast<float>(base_texture.width) * 128.0F), 0.00001F,
                         "CometHalo_v indirect trace should expose the transformed source S coordinate");
            require_near(indirect_trace->transformed_coord.v,
                         static_cast<float>(expected_transformed_t) / (static_cast<float>(base_texture.height) * 128.0F), 0.00001F,
                         "CometHalo_v indirect trace should expose the transformed source T coordinate");
            auto comet_halo_without_indirect = *comet_halo;
            comet_halo_without_indirect.gx_state.indirect.stage_count = 0U;
            comet_halo_without_indirect.gx_state.indirect.tev_stages.clear();
            auto indirect_changes_sample = false;
            for (auto y = 1U; y < 8U && !indirect_changes_sample; ++y) {
                for (auto x = 1U; x < 8U; ++x) {
                    const auto source = smgpc::game::J3dMeshVertex{
                        .u = static_cast<float>(x) / 8.0F,
                        .v = static_cast<float>(y) / 8.0F,
                    };
                    const auto with_indirect = smgpc::game::j3d_evaluate_material_color(*comet_halo, model.textures, comet_halo_passes, source,
                                                                                        comet_halo->material_colors[0U]);
                    const auto without_indirect =
                        smgpc::game::j3d_evaluate_material_color(comet_halo_without_indirect, model.textures, comet_halo_passes, source,
                                                                 comet_halo->material_colors[0U]);
                    require(with_indirect.has_value() && without_indirect.has_value(),
                            "CometHalo_v material evaluator should sample both regular and indirect textures");
                    if (*with_indirect != *without_indirect) {
                        indirect_changes_sample = true;
                        break;
                    }
                }
            }
            require(indirect_changes_sample, "CometHalo_v material evaluation should apply decoded indirect texture coordinates");

            const auto *earth_far = find_material("EarthFar_v");
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
            require(!smgpc::game::j3d_try_compose_material_texture(*earth_far, model.textures, earth_passes, earth_far->material_colors[0U]).has_value(),
                    "EarthFar_v should not compose because it uses position/projected texture generation");
            const auto representative_earth_pass = smgpc::game::j3d_representative_texture_pass(*earth_far);
            require(representative_earth_pass.has_value() && representative_earth_pass->texture_index == 9U,
                    "EarthFar_v representative pass should use the original base earth texture map");
            const auto evaluated_earth_color = smgpc::game::j3d_evaluate_material_color(
                *earth_far, model.textures, earth_passes, smgpc::game::J3dMeshVertex{.x = 10.0F, .y = 20.0F, .z = 40.0F, .u = 0.75F, .v = 0.25F},
                earth_far->material_colors[0U]);
            require(evaluated_earth_color.has_value(), "EarthFar_v projected/POS material should evaluate through the shared J3D runtime");
            require(std::ranges::any_of(*evaluated_earth_color, [](std::uint8_t channel) { return channel != 0U; }),
                    "EarthFar_v projected/POS material evaluation should produce a non-empty RGBA result");

            const auto *sun = find_material("Sun_Mat_v");
            require(sun != nullptr, "CometNearOrbitSky should expose Sun_Mat_v");
            require(sun->textures.size() == 1U && sun->textures[0U].texture_index == 4U, "Sun_Mat_v should bind PlanetSun");
            require(sun->tev_stages.size() == 1U, "Sun_Mat_v should expose its original single TEV stage");
            require_tev_stage(sun->tev_stages[0U], {15U, 8U, 10U, 15U}, 12U, {7U, 4U, 5U, 7U}, 1U, 28U, "Sun_Mat_v TEV stage 0 semantic decode changed");
            require(sun->blend.enabled && sun->blend.type == 1U && sun->blend.src_factor == 4U && sun->blend.dst_factor == 1U,
                    "Sun_Mat_v should preserve original additive blend state");

            const auto &space_shape = model.shapes->shapes.at(7U);
            require(space_shape.material_index == 7U, "CometNearOrbitSky shape 7 should use Space_Mat_v");
            require(space_shape.joint_index == 7U, "CometNearOrbitSky Space_Mat_v shape should be attached to Obit joint");
            require(space_shape.draw_order == 7U, "CometNearOrbitSky Space_Mat_v should keep INF1 draw order");
            require(space_shape.matrix_groups.size() == 1U && space_shape.matrix_groups[0U].use_matrix_index != 0xffffU,
                    "CometNearOrbitSky Space_Mat_v should preserve its SHP1 matrix group draw matrix index");
            require(space_shape.matrix_groups[0U].display_list_size == space_shape.display_list_bytes,
                    "CometNearOrbitSky Space_Mat_v matrix group should own the shape display packet");
            require(space_shape.display_list_bytes == 3232U, "Space_Mat_v shape display list size changed");
            require(space_shape.parsed_display_list_bytes == space_shape.display_list_bytes, "Space_Mat_v shape display list should parse fully");
            require(space_shape.triangle_count == 480U, "Space_Mat_v triangle count changed");
            const auto geometry = smgpc::game::extract_j3d_model_geometry(model_data);
            const auto &space_packet_mesh = geometry.shapes.at(7U).draw_packets.at(0U);
            require(space_packet_mesh.matrix_group.display_list_size == 3232U,
                    "Space_Mat_v runtime packet mesh should preserve the original SHP1 display-list size");
            require(!space_packet_mesh.vertices.empty(), "Space_Mat_v runtime packet mesh should expose source vertices");
            require(std::ranges::any_of(space_packet_mesh.vertices, [](const auto &vertex) {
                        const auto normal_length =
                            std::sqrt(vertex.normal[0U] * vertex.normal[0U] + vertex.normal[1U] * vertex.normal[1U] + vertex.normal[2U] * vertex.normal[2U]);
                        return normal_length > 0.5F && normal_length < 1.5F;
                    }),
                    "Space_Mat_v runtime packet mesh should preserve indexed VTX1 normal vectors");
            require(std::ranges::all_of(space_packet_mesh.vertices, [](const auto &vertex) {
                        return vertex.tex_coord_count >= 1U && vertex.u == vertex.tex_coords[0U][0U] && vertex.v == vertex.tex_coords[0U][1U];
                    }),
                    "Space_Mat_v runtime packet mesh should preserve TEX0 coordinates alongside legacy u/v aliases");

            const auto &sky_shape = model.shapes->shapes.at(8U);
            require(sky_shape.material_index == 6U, "CometNearOrbitSky shape 8 should use Sky_Mat_v");
            require(sky_shape.draw_order == 6U, "CometNearOrbitSky Sky_Mat_v should draw before Space_Mat_v per INF1");

            const auto &sun_shape = model.shapes->shapes.at(6U);
            require(sun_shape.material_index == 8U, "CometNearOrbitSky shape 6 should use Sun_Mat_v");
            require(sun_shape.joint_index == 7U, "CometNearOrbitSky Sun_Mat_v shape should be attached to Obit joint");
            require(sun_shape.draw_order == 8U, "CometNearOrbitSky Sun_Mat_v should keep INF1 draw order");
            require(sun_shape.triangle_count == 16U, "Sun_Mat_v triangle count changed");
        }

        $test("probes CometNearOrbitSky BCK and BTK animations") {
            const auto root = disc_files_root();
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

        $test("renders CometNearOrbitSky as original SHP1 draw packets") {
            const auto root = disc_files_root();
            const auto sky_archive = smgpc::game::RarcArchive::from_file(root / "ObjectData" / "CometNearOrbitSky.arc");
            require(sky_archive.contains("cometnearorbitsky.bdl"), "CometNearOrbitSky.arc should contain the original BDL");
            const auto sky_model = smgpc::game::inspect_j3d_model(sky_archive.file_data("cometnearorbitsky.bdl"));
            require(sky_model.materials.has_value(), "CometNearOrbitSky renderer test should expose source materials");
            const auto find_material_summary = [&sky_model](std::string_view name) -> const smgpc::game::J3dMaterialSummary * {
                const auto it = std::ranges::find_if(sky_model.materials->materials, [name](const auto &material) { return material.name == name; });
                return it == sky_model.materials->materials.end() ? nullptr : &*it;
            };
            const auto *space_summary = find_material_summary("Space_Mat_v");
            const auto *earth_far_summary = find_material_summary("EarthFar_v");
            const auto *core_rock_summary = find_material_summary("CoreRock");
            require(space_summary != nullptr && earth_far_summary != nullptr && core_rock_summary != nullptr,
                    "CometNearOrbitSky renderer test should resolve source material state");

            auto renderer = RecordingRenderer();
            auto model_renderer = smgpc::game::J3dModelRenderer();
            model_renderer.load(renderer, sky_archive.file_data("cometnearorbitsky.bdl"));
            require(model_renderer.is_loaded(), "J3dModelRenderer should load CometNearOrbitSky original geometry");
            require(model_renderer.mesh_count() == 9U,
                    "J3dModelRenderer should execute one CometNearOrbitSky render packet per original SHP1 matrix group");
            const auto packets = model_renderer.render_packets();
            require(packets.size() == 9U, "J3dModelRenderer should expose one state packet per CometNearOrbitSky SHP1 matrix group");
            for (auto packet_index = std::size_t{1U}; packet_index < packets.size(); ++packet_index) {
                const auto previous = std::tuple<std::uint16_t, std::uint16_t, std::uint8_t>{packets[packet_index - 1U].shape_draw_order,
                                                                                             packets[packet_index - 1U].matrix_group_index,
                                                                                             packets[packet_index - 1U].pass_order};
                const auto current = std::tuple<std::uint16_t, std::uint16_t, std::uint8_t>{
                    packets[packet_index].shape_draw_order, packets[packet_index].matrix_group_index, packets[packet_index].pass_order};
                require(previous <= current, "J3dModelRenderer packet evidence should preserve draw-order, matrix-group, then material-pass ordering");
            }
            const auto earth_far_packet = std::ranges::find_if(packets, [](const auto &packet) {
                return packet.material_name == "EarthFar_v";
            });
            require(earth_far_packet != packets.end() && earth_far_packet->packet_mode == smgpc::game::J3dRendererPacketMode::ShaderGxTev &&
                        !earth_far_packet->evaluate_material_per_vertex && earth_far_packet->material_pass_count == 3U &&
                        earth_far_packet->shader_texture_stage_count == 3U,
                    "J3dModelRenderer packet evidence should identify projected multi-TEV shader packets");
            const auto earth_night_packet = std::ranges::find_if(packets, [](const auto &packet) {
                return packet.material_name == "EarthNightMat_v";
            });
            require(earth_night_packet != packets.end() &&
                        earth_night_packet->packet_mode == smgpc::game::J3dRendererPacketMode::ShaderGxTev &&
                        earth_night_packet->shader_texture_stage_count == 2U,
                    "J3dModelRenderer packet evidence should identify projected two-stage TEV shader packets");
            const auto comet_halo_packet = std::ranges::find_if(packets, [](const auto &packet) {
                return packet.material_name == "CometHalo_v";
            });
            require(comet_halo_packet != packets.end() && comet_halo_packet->packet_mode == smgpc::game::J3dRendererPacketMode::ComposedMaterial &&
                        !comet_halo_packet->evaluate_material_per_vertex && comet_halo_packet->indirect_stage_count == 1U &&
                        comet_halo_packet->indirect_texture_order_count > 0U && comet_halo_packet->declared_tev_stage_count == 1U &&
                        comet_halo_packet->active_tev_stage_count == 1U,
                    "J3dModelRenderer packet evidence should route CometHalo_v active-indirect TEV through compat material evaluation");
            const auto comet_halo_batch_count_before_draw = renderer.triangle_batch_count;
            const auto comet_halo_vertices_before_draw = renderer.submitted_vertices;
            model_renderer.draw(renderer, smgpc::game::file_select_title_camera_pose(), smgpc::game::J3dMatrix3x4{}, 0U,
                                {.material_filter = "CometHalo_v"});
            require(renderer.triangle_batch_count > comet_halo_batch_count_before_draw &&
                        renderer.submitted_vertices > comet_halo_vertices_before_draw,
                    "J3dModelRenderer should submit visible CometHalo_v active-indirect geometry to the renderer");
            const auto sky_packet = std::ranges::find_if(packets, [](const auto &packet) {
                return packet.material_name == "Sky_Mat_v";
            });
            require(sky_packet != packets.end() && sky_packet->packet_mode == smgpc::game::J3dRendererPacketMode::ShaderGxTev &&
                        sky_packet->shader_texture_stage_count == 1U,
                    "J3dModelRenderer packet evidence should route single-texture TEV packets through the GX shader path");
            const auto core_rock_packet = std::ranges::find_if(packets, [](const auto &packet) {
                return packet.material_name == "CoreRock";
            });
            require(core_rock_packet != packets.end() && core_rock_packet->packet_mode == smgpc::game::J3dRendererPacketMode::CpuTevPerVertex &&
                        core_rock_packet->evaluate_material_per_vertex && core_rock_packet->loaded_light_mask == 0U &&
                        core_rock_packet->alpha_channel_controls[0U].lighting_enabled &&
                        core_rock_packet->alpha_channel_controls[0U].light_mask == 4U,
                    "J3dModelRenderer packet evidence should route untextured lit materials through per-vertex GX raster evaluation");
            const auto space_packet = std::ranges::find_if(packets, [](const auto &packet) {
                return packet.material_name == "Space_Mat_v";
            });
            require(space_packet != packets.end() && space_packet->shape_draw_order == 7U && space_packet->joint_index == 7U &&
                        space_packet->tev_stage_count > 0U && space_packet->mdl3_packet_bytes > 0U && space_packet->mdl3_bp_load_count > 0U &&
                        space_packet->mdl3_xf_load_count > 0U && space_packet->cull_mode == smgpc::render::CullMode::Back &&
                        space_packet->packet_mode == smgpc::game::J3dRendererPacketMode::ShaderGxTev && space_packet->shader_texture_stage_count == 3U &&
                        space_packet->gx_blend.enabled && space_packet->gx_blend.type == 0U && space_packet->gx_blend.src_factor == 1U &&
                        space_packet->gx_blend.dst_factor == 0U && space_packet->fog_type == space_summary->gx_state.fog.type &&
                        space_packet->fog_color == space_summary->gx_state.fog.color,
                    "J3dModelRenderer packet evidence should preserve GX state and shader-backed material context for Space_Mat_v");
            require(space_packet->color_channel_count == space_summary->gx_state.color_channel_count &&
                        space_packet->color_channel_material_colors[0U] == space_summary->gx_state.color_channels[0U].material_color &&
                        space_packet->color_channel_ambient_colors[0U] == space_summary->gx_state.color_channels[0U].ambient_color &&
                        space_packet->color_channel_controls[0U].raw == space_summary->gx_state.color_channels[0U].color_control.raw &&
                        space_packet->alpha_channel_controls[0U].raw == space_summary->gx_state.color_channels[0U].alpha_control.raw,
                    "J3dModelRenderer packet evidence should preserve effective GX color-channel state for Space_Mat_v");
            require(space_packet->matrix_group_index == 0U && space_packet->matrix_group_count == 1U && space_packet->use_matrix_index == 4U &&
                        space_packet->use_matrix_count == 1U && space_packet->first_matrix_table_index == 7U &&
                        space_packet->matrix_table_count == 1U && space_packet->display_list_offset == 0x3720U &&
                        space_packet->display_list_size == 3232U && space_packet->parsed_display_list_bytes == 3232U &&
                        space_packet->draw_packet_triangle_count == 480U && space_packet->source_triangle_count == 480U,
                    "J3dModelRenderer packet evidence should preserve Space_Mat_v SHP1 matrix-group draw packet metadata");
            const auto sky_bck = smgpc::game::inspect_j3d_animation(sky_archive.file_data("cometnearorbitsky.bck"));
            const auto sky_btk = smgpc::game::inspect_j3d_animation(sky_archive.file_data("cometnearorbitsky.btk"));
            require(sky_bck.bck.has_value() && sky_btk.btk.has_value(), "CometNearOrbitSky renderer test should resolve BCK and BTK state");
            model_renderer.set_bck_animation(*sky_bck.bck);
            model_renderer.set_btk_animation(*sky_btk.btk);
            const auto animated_packets = model_renderer.render_packets(3001U);
            const auto animated_space_packet = std::ranges::find_if(animated_packets, [](const auto &packet) {
                return packet.material_name == "Space_Mat_v";
            });
            require(animated_space_packet != animated_packets.end() && animated_space_packet->bck_active && animated_space_packet->btk_active &&
                        animated_space_packet->bck_frame_max == 3000 && animated_space_packet->bck_joint_count == 8U &&
                        animated_space_packet->btk_frame_max == 10000 && animated_space_packet->btk_material_count == 5U,
                    "J3dModelRenderer packet evidence should include active BCK and BTK metadata for runtime comparisons");
            require_near(animated_space_packet->bck_frame, 3001.0F, 0.001F,
                         "J3dModelRenderer packet evidence should preserve the submitted BCK frame");
            require_near(animated_space_packet->bck_normalized_frame, 1.0F, 0.001F,
                         "J3dModelRenderer packet evidence should expose looped BCK frame state");
            require_near(animated_space_packet->btk_normalized_frame, 3001.0F, 0.001F,
                         "J3dModelRenderer packet evidence should expose looped BTK frame state");
            const auto batch_count_before_draw = renderer.gx_material_batch_count;
            model_renderer.draw(renderer, smgpc::game::file_select_title_camera_pose(), smgpc::game::J3dMatrix3x4{}, 0U,
                                {.material_filter = "Space_Mat_v"});
            require(renderer.gx_material_batch_count > batch_count_before_draw,
                    "J3dModelRenderer should submit filtered Space_Mat_v geometry through the GX material shader path");
            require(renderer.last_gx_material_stage_count == 3U && renderer.last_gx_material_tev_stage_count == 3U,
                    "J3dModelRenderer should submit Space_Mat_v as a three-texture GX TEV material batch");
            require(renderer.last_gx_material_color_inputs[0U] == std::array<std::uint8_t, 4U>{15U, 8U, 10U, 14U} &&
                        renderer.last_gx_material_color_inputs[1U] == std::array<std::uint8_t, 4U>{15U, 10U, 8U, 0U} &&
                        renderer.last_gx_material_color_inputs[2U] == std::array<std::uint8_t, 4U>{15U, 10U, 8U, 0U} &&
                        renderer.last_gx_material_alpha_compare_enabled && renderer.last_gx_material_blend.enabled &&
                        renderer.last_gx_material_blend.type == 0U && renderer.last_gx_material_blend.src_factor == 1U &&
                        renderer.last_gx_material_blend.dst_factor == 0U,
                    "J3dModelRenderer should preserve Space_Mat_v raw TEV shader state");
            require(renderer.last_gx_material_alpha_compare.comp0 == space_summary->gx_state.alpha_compare.comp0 &&
                        renderer.last_gx_material_alpha_compare.ref0 == space_summary->gx_state.alpha_compare.ref0 &&
                        renderer.last_gx_material_alpha_compare.op == space_summary->gx_state.alpha_compare.op &&
                        renderer.last_gx_material_alpha_compare.comp1 == space_summary->gx_state.alpha_compare.comp1 &&
                        renderer.last_gx_material_alpha_compare.ref1 == space_summary->gx_state.alpha_compare.ref1 &&
                        renderer.last_gx_material_initial_tev_registers == space_summary->gx_state.tev_registers,
                    "J3dModelRenderer should submit Space_Mat_v alpha compare and initial TEV registers from effective GX state");
            require(renderer.last_gx_material_fog.enabled == space_summary->gx_state.fog.enabled &&
                        renderer.last_gx_material_fog.type == space_summary->gx_state.fog.type &&
                        renderer.last_gx_material_fog.projection == space_summary->gx_state.fog.projection &&
                        renderer.last_gx_material_fog.color == space_summary->gx_state.fog.color,
                    "J3dModelRenderer should submit Space_Mat_v typed GX fog state to the renderer batch");
            require(renderer.last_triangle_cull_mode == smgpc::render::CullMode::Back,
                    "J3dModelRenderer should submit the decoded MDL3 GX cull mode to the GX material renderer batch");
            const auto earth_batch_count_before_draw = renderer.gx_material_batch_count;
            model_renderer.draw(renderer, smgpc::game::file_select_title_camera_pose(), smgpc::game::J3dMatrix3x4{}, 0U,
                                {.material_filter = "EarthFar_v"});
            require(renderer.gx_material_batch_count > earth_batch_count_before_draw,
                    "J3dModelRenderer should submit projected EarthFar_v geometry through the GX material shader path");
            require(renderer.last_gx_material_stage_count == 3U && renderer.last_gx_material_tev_stage_count == 3U &&
                        renderer.last_gx_material_color_inputs[0U] == std::array<std::uint8_t, 4U>{15U, 10U, 8U, 15U} &&
                        renderer.last_gx_material_color_inputs[1U] == std::array<std::uint8_t, 4U>{4U, 14U, 8U, 0U} &&
                        renderer.last_gx_material_color_inputs[2U] == std::array<std::uint8_t, 4U>{15U, 10U, 8U, 0U},
                    "J3dModelRenderer should preserve EarthFar_v projected TEV shader state");
            require(renderer.last_gx_material_saw_projective_q && renderer.last_gx_material_saw_clip_w,
                    "J3dModelRenderer should submit projected texture q and clip w for EarthFar_v shader interpolation");
            require(renderer.last_gx_material_blend.enabled && renderer.last_gx_material_blend.type == 0U &&
                        renderer.last_gx_material_blend.src_factor == 1U && renderer.last_gx_material_blend.dst_factor == 0U,
                    "J3dModelRenderer should submit EarthFar_v raw GX no-blend state to the GX material renderer batch");
            require(renderer.last_gx_material_initial_tev_registers == earth_far_summary->gx_state.tev_registers &&
                        renderer.last_gx_material_depth_test == (earth_far_summary->gx_state.z_mode.compare_enable != 0U) &&
                        renderer.last_gx_material_depth_write == (earth_far_summary->gx_state.z_mode.update_enable != 0U) &&
                        renderer.last_gx_material_depth_compare == smgpc::render::DepthCompare::LessEqual,
                    "J3dModelRenderer should submit EarthFar_v initial TEV registers and depth state from effective GX state");
            require(renderer.last_gx_material_fog.enabled == earth_far_summary->gx_state.fog.enabled &&
                        renderer.last_gx_material_fog.type == earth_far_summary->gx_state.fog.type &&
                        renderer.last_gx_material_fog.color == earth_far_summary->gx_state.fog.color,
                    "J3dModelRenderer should submit EarthFar_v typed GX fog state to the renderer batch");
            const auto sun_batch_count_before_draw = renderer.gx_material_batch_count;
            model_renderer.draw(renderer, smgpc::game::file_select_title_camera_pose(), smgpc::game::J3dMatrix3x4{}, 0U,
                                {.material_filter = "Sun_Mat_v"});
            require(renderer.gx_material_batch_count > sun_batch_count_before_draw,
                    "J3dModelRenderer should submit filtered Sun_Mat_v geometry through the GX material shader path");
            require(renderer.last_gx_material_blend.enabled && renderer.last_gx_material_blend.type == 1U &&
                        renderer.last_gx_material_blend.src_factor == 4U && renderer.last_gx_material_blend.dst_factor == 1U,
                    "J3dModelRenderer should submit Sun_Mat_v raw GX additive blend state to the GX material renderer batch");
            require(renderer.texture_count > 0U, "J3dModelRenderer should upload CometNearOrbitSky textures");

            auto shader_only_renderer = RecordingRenderer();
            auto shader_only_options = smgpc::game::J3dModelRendererLoadOptions{};
            shader_only_options.use_cpu_tev = false;
            auto shader_only_model = smgpc::game::J3dModelRenderer();
            shader_only_model.load(shader_only_renderer, sky_archive.file_data("cometnearorbitsky.bdl"), shader_only_options);
            const auto shader_only_packets = shader_only_model.render_packets();
            const auto shader_only_space = std::ranges::find_if(shader_only_packets, [](const auto &packet) {
                return packet.material_name == "Space_Mat_v";
            });
            require(shader_only_space != shader_only_packets.end() &&
                        shader_only_space->packet_mode == smgpc::game::J3dRendererPacketMode::ShaderGxTev,
                    "J3dModelRenderer should keep shader-backed Space_Mat_v even when CPU TEV fallbacks are disabled");
            const auto shader_only_earth_far = std::ranges::find_if(shader_only_packets, [](const auto &packet) {
                return packet.material_name == "EarthFar_v";
            });
            require(shader_only_earth_far != shader_only_packets.end() &&
                        shader_only_earth_far->packet_mode == smgpc::game::J3dRendererPacketMode::ShaderGxTev,
                    "J3dModelRenderer should keep shader-backed EarthFar_v even when CPU TEV fallbacks are disabled");
        }

        $test("renders FileSelectDataPlanet original J3D model") {
            const auto root = disc_files_root();
            const auto planet_archive = smgpc::game::RarcArchive::from_file(root / "ObjectData" / "FileSelectDataPlanet.arc");
            require(planet_archive.contains("fileselectdataplanet.bdl"), "FileSelectDataPlanet.arc should contain the original BDL");
            const auto planet_model = smgpc::game::inspect_j3d_model(planet_archive.file_data("fileselectdataplanet.bdl"));
            require(planet_model.materials.has_value(), "FileSelectDataPlanet renderer test should expose source materials");
            const auto base_material = std::ranges::find_if(planet_model.materials->materials, [](const auto &material) {
                return material.name == "BaseMat_v";
            });
            require(base_material != planet_model.materials->materials.end(), "FileSelectDataPlanet should expose BaseMat_v");
            require(base_material->tev_stage_count == 3U && base_material->tev_orders.size() == 3U &&
                        base_material->tev_orders[0U].tex_map == 0xffU && base_material->tev_orders[1U].tex_map == 0U &&
                        base_material->tev_orders[2U].tex_map == 0xffU,
                    "BaseMat_v should preserve texture-disabled TEV stages around its real texture stage");

            auto renderer = RecordingRenderer();
            auto model_renderer = smgpc::game::J3dModelRenderer();
            model_renderer.load(renderer, planet_archive.file_data("fileselectdataplanet.bdl"));
            require(model_renderer.is_loaded(), "J3dModelRenderer should load FileSelectDataPlanet original geometry");
            require(model_renderer.mesh_count() > 0U, "J3dModelRenderer should expose renderable FileSelectDataPlanet meshes");
            require(renderer.texture_count > 0U, "J3dModelRenderer should upload FileSelectDataPlanet textures");
            const auto packets = model_renderer.render_packets();
            const auto base_packet = std::ranges::find_if(packets, [](const auto &packet) {
                return packet.material_name == "BaseMat_v";
            });
            require(base_packet != packets.end() && base_packet->packet_mode == smgpc::game::J3dRendererPacketMode::ShaderGxTev &&
                        base_packet->shader_texture_stage_count == 1U && base_packet->active_tev_stage_count == 3U,
                    "J3dModelRenderer should keep BaseMat_v texture-disabled TEV stages in the GX shader path");

            const auto base_gx_batches_before = renderer.gx_material_batch_count;
            model_renderer.draw(renderer, smgpc::game::file_select_far_camera_pose(),
                                smgpc::game::j3d_matrix_from_translation_scale({0.0F, 800.0F, 0.0F}, 30.0F), 0U,
                                {.material_filter = "BaseMat_v"});
            require(renderer.gx_material_batch_count > base_gx_batches_before && renderer.last_gx_material_stage_count == 1U &&
                        renderer.last_gx_material_tev_stage_count == 3U,
                    "J3dModelRenderer should submit BaseMat_v as one texture with all three TEV stages");

            const auto submitted_before = renderer.submitted_vertices;
            model_renderer.draw(renderer, smgpc::game::file_select_far_camera_pose(),
                                smgpc::game::j3d_matrix_from_translation_scale({0.0F, 800.0F, 0.0F}, 30.0F), 0U);
            require(renderer.gx_material_batch_count > 0U || renderer.triangle_batch_count > 0U,
                    "J3dModelRenderer should submit projected FileSelectDataPlanet triangles");
            require(renderer.submitted_vertices > submitted_before && renderer.submitted_indices > 0U,
                    "J3dModelRenderer should submit non-empty FileSelectDataPlanet batches");
        }

    }  // namespace

    void run_j3d_gx_tests() {
        run_registered_tests(kTestSuite);
    }

}  // namespace smgpc::tests
