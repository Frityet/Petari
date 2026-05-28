#include "DebugPaths.hpp"
#include "DebugText.hpp"
#include "MarkdownWriter.hpp"
#include "render/J3dModel.hpp"
#include "resource/RarcArchive.hpp"

#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

    void write_sections(std::ofstream &out, const smgpc::render::J3dModelSummary &model) {
        out << "## Sections\n\n";
        out << "| tag | offset | size |\n";
        out << "| --- | ---: | ---: |\n";
        for (const auto &section : model.sections) {
            out << "| `" << section.tag << "` | 0x" << std::hex << section.offset << std::dec << " | " << section.size << " |\n";
        }
        out << '\n';
    }

    void write_info(std::ofstream &out, const smgpc::render::J3dModelSummary &model) {
        if (!model.info.has_value()) {
            return;
        }

        out << "## INF1\n\n";
        out << "- flags: 0x" << std::hex << model.info->flags << std::dec << '\n';
        out << "- packets: " << model.info->packet_count << '\n';
        out << "- vertices: " << model.info->vertex_count << '\n';
        out << "- hierarchy entries: " << model.info->hierarchy.size() << "\n\n";
        out << "| index | type | value |\n";
        out << "| ---: | --- | ---: |\n";
        for (auto i = std::size_t{}; i < model.info->hierarchy.size(); ++i) {
            const auto &entry = model.info->hierarchy[i];
            out << "| " << i << " | " << smgpc::render::j3d_hierarchy_type_name(entry.type) << " | " << entry.value << " |\n";
        }
        out << '\n';
    }

    void write_vertices(std::ofstream &out, const smgpc::render::J3dModelSummary &model) {
        if (!model.vertices.has_value()) {
            return;
        }

        out << "## VTX1\n\n";
        out << "### Attribute Formats\n\n";
        out << "| attr | component count | component type | fraction |\n";
        out << "| --- | ---: | ---: | ---: |\n";
        for (const auto &format : model.vertices->formats) {
            out << "| " << smgpc::render::j3d_vertex_attr_name(format.attr) << " | " << format.component_count << " | " << format.component_type
                << " | " << static_cast<int>(format.fraction) << " |\n";
        }

        out << "\n### Arrays\n\n";
        out << "| attr | offset | stride | inferred count |\n";
        out << "| --- | ---: | ---: | ---: |\n";
        for (const auto &array : model.vertices->arrays) {
            out << "| " << smgpc::render::j3d_vertex_attr_name(array.attr) << " | 0x" << std::hex << array.offset << std::dec << " | " << array.stride
                << " | " << array.inferred_count << " |\n";
        }
        out << '\n';
    }

    void write_joints(std::ofstream &out, const smgpc::render::J3dModelSummary &model) {
        if (!model.joints.has_value()) {
            return;
        }

        out << "## JNT1\n\n";
        out << "- joints: " << model.joints->joint_count << "\n\n";
        out << "| index | name | kind | scale compensate | scale | rotation | translation | radius | bounds |\n";
        out << "| ---: | --- | ---: | ---: | --- | --- | --- | ---: | --- |\n";
        for (const auto &joint : model.joints->joints) {
            out << "| " << joint.index << " | `" << joint.name << "` | " << joint.kind << " | " << static_cast<int>(joint.scale_compensate) << " | "
                << joint.scale[0U] << "," << joint.scale[1U] << "," << joint.scale[2U] << " | " << joint.rotation[0U] << "," << joint.rotation[1U]
                << "," << joint.rotation[2U] << " | " << joint.translation[0U] << "," << joint.translation[1U] << "," << joint.translation[2U]
                << " | " << joint.radius << " | [" << joint.min[0U] << ", " << joint.min[1U] << ", " << joint.min[2U] << "] -> [" << joint.max[0U]
                << ", " << joint.max[1U] << ", " << joint.max[2U] << "] |\n";
        }
        out << '\n';
    }

    void write_matrix_state(std::ofstream &out, const smgpc::render::J3dModelSummary &model) {
        if (model.envelopes.has_value()) {
            out << "## EVP1\n\n";
            out << "- weighted envelopes: " << model.envelopes->matrix_count << '\n';
            out << "- inverse bind matrices: " << model.envelopes->inverse_bind_matrices.size() << "\n\n";
            if (!model.envelopes->matrices.empty()) {
                out << "| envelope | joints | weights |\n";
                out << "| ---: | --- | --- |\n";
                for (auto i = std::size_t{}; i < model.envelopes->matrices.size(); ++i) {
                    const auto &envelope = model.envelopes->matrices[i];
                    out << "| " << i << " | ";
                    for (auto joint = std::size_t{}; joint < envelope.joint_indices.size(); ++joint) {
                        if (joint != 0U) {
                            out << ',';
                        }
                        out << envelope.joint_indices[joint];
                    }
                    out << " | ";
                    for (auto weight = std::size_t{}; weight < envelope.weights.size(); ++weight) {
                        if (weight != 0U) {
                            out << ',';
                        }
                        out << envelope.weights[weight];
                    }
                    out << " |\n";
                }
                out << '\n';
            }
        }

        if (model.draw_matrices.has_value()) {
            out << "## DRW1\n\n";
            out << "- draw matrices: " << model.draw_matrices->matrix_count << "\n\n";
            out << "| index | kind | target |\n";
            out << "| ---: | --- | ---: |\n";
            for (auto i = std::size_t{}; i < model.draw_matrices->matrices.size(); ++i) {
                const auto &matrix = model.draw_matrices->matrices[i];
                out << "| " << i << " | " << (matrix.weighted ? "envelope" : "joint") << " | " << matrix.index << " |\n";
            }
            out << '\n';
        }

        if (model.mdl3.has_value()) {
            out << "## MDL3\n\n";
            out << "- material packets: " << model.mdl3->material_count << "\n\n";
            out << "| material | packet offset | packet bytes |\n";
            out << "| ---: | ---: | ---: |\n";
            for (auto i = std::size_t{}; i < model.mdl3->packets.size(); ++i) {
                const auto &packet = model.mdl3->packets[i];
                out << "| " << i << " | 0x" << std::hex << packet.offset << std::dec << " | " << packet.size << " |\n";
            }
            out << '\n';
        }
    }

    void write_materials(std::ofstream &out, const smgpc::render::J3dModelSummary &model) {
        if (!model.materials.has_value()) {
            return;
        }

        out << "## MAT3\n\n";
        out << "- materials: " << model.materials->material_count << "\n\n";
        out << "| index | name | material id | mode | cull | z mode | z comp loc | mat color 0 | tev k color 0 | texgens | tex coord gens | tex "
               "matrices | tev stages | tev "
               "orders | tev stage ops | alpha compare | blend | textures |\n";
        out << "| ---: | --- | ---: | ---: | ---: | --- | ---: | --- | --- | ---: | --- | --- | ---: | --- | --- | --- | --- | --- |\n";
        for (const auto &material : model.materials->materials) {
            auto textures = std::ostringstream{};
            for (auto i = std::size_t{}; i < material.textures.size(); ++i) {
                if (i != 0U) {
                    textures << ", ";
                }
                textures << "slot " << static_cast<int>(material.textures[i].slot) << " -> tex " << material.textures[i].texture_index;
                if (material.textures[i].texture_index < model.textures.size()) {
                    textures << " (" << model.textures[material.textures[i].texture_index].name << ')';
                }
            }

            auto tex_coord_gens = std::ostringstream{};
            for (auto i = std::size_t{}; i < material.tex_coord_gens.size(); ++i) {
                if (i != 0U) {
                    tex_coord_gens << ", ";
                }
                const auto &gen = material.tex_coord_gens[i];
                tex_coord_gens << "slot " << static_cast<int>(gen.slot) << " type " << static_cast<int>(gen.type) << " src "
                               << static_cast<int>(gen.source) << " mtx " << static_cast<int>(gen.matrix);
            }

            auto tex_matrices = std::ostringstream{};
            for (auto i = std::size_t{}; i < material.tex_matrices.size(); ++i) {
                if (i != 0U) {
                    tex_matrices << ", ";
                }
                const auto &matrix = material.tex_matrices[i];
                tex_matrices << "slot " << static_cast<int>(matrix.slot) << " proj " << static_cast<int>(matrix.projection) << " info "
                             << static_cast<int>(matrix.info) << " srt(" << matrix.scale_s << ',' << matrix.scale_t << ',' << matrix.rotation << ','
                             << matrix.translate_s << ',' << matrix.translate_t << ") eff0(" << matrix.effect_matrix[0U] << ','
                             << matrix.effect_matrix[1U] << ',' << matrix.effect_matrix[2U] << ',' << matrix.effect_matrix[3U] << ") eff1("
                             << matrix.effect_matrix[4U] << ',' << matrix.effect_matrix[5U] << ',' << matrix.effect_matrix[6U] << ','
                             << matrix.effect_matrix[7U] << ") eff2(" << matrix.effect_matrix[8U] << ',' << matrix.effect_matrix[9U] << ','
                             << matrix.effect_matrix[10U] << ',' << matrix.effect_matrix[11U] << ") eff3(" << matrix.effect_matrix[12U] << ','
                             << matrix.effect_matrix[13U] << ',' << matrix.effect_matrix[14U] << ',' << matrix.effect_matrix[15U] << ')';
            }

            auto tev_orders = std::ostringstream{};
            for (auto i = std::size_t{}; i < material.tev_orders.size(); ++i) {
                if (i != 0U) {
                    tev_orders << ", ";
                }
                const auto &order = material.tev_orders[i];
                tev_orders << "stage " << static_cast<int>(order.stage) << " coord " << static_cast<int>(order.tex_coord) << " map "
                           << static_cast<int>(order.tex_map) << " chan " << static_cast<int>(order.color_channel);
            }

            auto tev_stages = std::ostringstream{};
            for (auto i = std::size_t{}; i < material.tev_stages.size(); ++i) {
                if (i != 0U) {
                    tev_stages << ", ";
                }
                const auto &stage = material.tev_stages[i];
                tev_stages << "stage " << static_cast<int>(stage.stage) << " c(" << static_cast<int>(stage.color_in[0U]) << ','
                           << static_cast<int>(stage.color_in[1U]) << ',' << static_cast<int>(stage.color_in[2U]) << ','
                           << static_cast<int>(stage.color_in[3U]) << ") cop(" << static_cast<int>(stage.color_op) << ','
                           << static_cast<int>(stage.color_bias) << ',' << static_cast<int>(stage.color_scale) << ','
                           << static_cast<int>(stage.color_clamp) << ',' << static_cast<int>(stage.color_out) << ") kc "
                           << static_cast<int>(stage.k_color_sel) << " a(" << static_cast<int>(stage.alpha_in[0U]) << ','
                           << static_cast<int>(stage.alpha_in[1U]) << ',' << static_cast<int>(stage.alpha_in[2U]) << ','
                           << static_cast<int>(stage.alpha_in[3U]) << ") aop(" << static_cast<int>(stage.alpha_op) << ','
                           << static_cast<int>(stage.alpha_bias) << ',' << static_cast<int>(stage.alpha_scale) << ','
                           << static_cast<int>(stage.alpha_clamp) << ',' << static_cast<int>(stage.alpha_out) << ") ka "
                           << static_cast<int>(stage.k_alpha_sel);
            }

            auto alpha_compare = std::ostringstream{};
            if (material.alpha_compare.enabled) {
                alpha_compare << static_cast<int>(material.alpha_compare.comp0) << ',' << static_cast<int>(material.alpha_compare.ref0) << ','
                              << static_cast<int>(material.alpha_compare.op) << ',' << static_cast<int>(material.alpha_compare.comp1) << ','
                              << static_cast<int>(material.alpha_compare.ref1);
            }

            auto blend = std::ostringstream{};
            if (material.blend.enabled) {
                blend << static_cast<int>(material.blend.type) << ',' << static_cast<int>(material.blend.src_factor) << ','
                      << static_cast<int>(material.blend.dst_factor) << ',' << static_cast<int>(material.blend.op);
            }

            auto z_mode = std::ostringstream{};
            if (material.z_mode.enabled) {
                z_mode << static_cast<int>(material.z_mode.compare_enable) << ',' << static_cast<int>(material.z_mode.function) << ','
                       << static_cast<int>(material.z_mode.update_enable);
            }

            const auto &color = material.material_colors[0U];
            const auto &k_color = material.tev_k_colors[0U];
            out << "| " << material.index << " | `" << material.name << "` | " << material.material_id << " | "
                << static_cast<int>(material.material_mode) << " | " << static_cast<int>(material.cull_mode) << " | " << z_mode.str() << " | "
                << static_cast<int>(material.z_comp_loc) << " | " << static_cast<int>(color[0U]) << ',' << static_cast<int>(color[1U]) << ','
                << static_cast<int>(color[2U]) << ',' << static_cast<int>(color[3U]) << " | " << static_cast<int>(k_color[0U]) << ','
                << static_cast<int>(k_color[1U]) << ',' << static_cast<int>(k_color[2U]) << ',' << static_cast<int>(k_color[3U]) << " | "
                << static_cast<int>(material.texgen_count) << " | " << tex_coord_gens.str() << " | " << tex_matrices.str() << " | "
                << static_cast<int>(material.tev_stage_count) << " | " << tev_orders.str() << " | " << tev_stages.str() << " | "
                << alpha_compare.str() << " | " << blend.str() << " | " << textures.str() << " |\n";
        }
        out << '\n';

        auto markdown = smgpc::dump::MarkdownWriter(out);
        markdown.heading(3U, "GX Material State");
        const auto gx_headers = std::vector<std::string_view>{
            "index", "source", "cull", "texgens", "color chans", "chan0 mat", "chan0 amb", "chan0 color ctrl", "chan0 alpha ctrl",
            "loaded lights", "tev stages declared", "textures", "tex coord gens", "tex matrices", "tev orders", "tev stages", "alpha", "blend",
            "z", "z comp loc", "indirect stages", "mdl3 bytes", "parsed", "bp", "xf", "cp", "indexed", "unknown"};
        auto gx_rows = std::vector<std::vector<std::string>>{};
        for (const auto &material : model.materials->materials) {
            const auto &state = material.gx_state;
            const auto &channel = state.color_channels[0U];
            auto loaded_light_mask = std::uint32_t{};
            for (auto light = 0U; light < state.lights.size(); ++light) {
                if (state.lights[light].loaded) {
                    loaded_light_mask |= 1U << light;
                }
            }
            const auto color_text = [](const auto &color) {
                auto text = std::ostringstream{};
                text << static_cast<int>(color[0U]) << ',' << static_cast<int>(color[1U]) << ',' << static_cast<int>(color[2U]) << ','
                     << static_cast<int>(color[3U]);
                return text.str();
            };
            const auto control_text = [](const auto &control) {
                auto text = std::ostringstream{};
                text << "raw " << control.raw << " src " << static_cast<int>(control.material_source) << " light "
                     << (control.lighting_enabled ? "on" : "off") << " mask " << static_cast<int>(control.light_mask) << " attn mode "
                     << static_cast<int>(control.attenuation_mode);
                return text.str();
            };
            auto light_mask = std::ostringstream{};
            light_mask << "0x" << std::hex << loaded_light_mask;
            gx_rows.push_back({
                std::to_string(material.index),
                state.source,
                std::to_string(static_cast<int>(state.cull_mode)),
                std::to_string(static_cast<int>(state.texgen_count)),
                std::to_string(static_cast<int>(state.color_channel_count)),
                color_text(channel.material_color),
                color_text(channel.ambient_color),
                control_text(channel.color_control),
                control_text(channel.alpha_control),
                light_mask.str(),
                std::to_string(static_cast<int>(state.tev_stage_count)),
                std::to_string(state.textures.size()),
                std::to_string(state.tex_coord_gens.size()),
                std::to_string(state.tex_matrices.size()),
                std::to_string(state.tev_orders.size()),
                std::to_string(state.tev_stages.size()),
                state.alpha_compare.enabled ? "on" : "off",
                state.blend.enabled ? "on" : "off",
                state.z_mode.enabled ? "on" : "off",
                std::to_string(static_cast<int>(state.z_comp_loc)),
                std::to_string(static_cast<int>(state.indirect.stage_count)),
                std::to_string(state.mdl3_display_list.size()),
                std::to_string(state.mdl3_stats.parsed_bytes),
                std::to_string(state.mdl3_stats.bp_load_count),
                std::to_string(state.mdl3_stats.xf_load_count),
                std::to_string(state.mdl3_stats.cp_load_count),
                std::to_string(state.mdl3_stats.indexed_load_count),
                std::to_string(state.mdl3_stats.unknown_opcode_count),
            });
        }
        markdown.table(gx_headers, gx_rows);
    }

    void write_shapes(std::ofstream &out, const smgpc::render::J3dModelSummary &model) {
        if (!model.shapes.has_value()) {
            return;
        }

        out << "## SHP1\n\n";
        out << "- shapes: " << model.shapes->shape_count << "\n\n";
        out << "| index | draw order | name | material | joint | mtx type | groups | dl bytes | parsed bytes | primitives | triangles | bounds |\n";
        out << "| ---: | ---: | --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |\n";
        for (const auto &shape : model.shapes->shapes) {
            auto material = std::ostringstream{};
            if (shape.material_index == 0xffffU) {
                material << "none";
            } else {
                material << shape.material_index;
                if (model.materials.has_value() && shape.material_index < model.materials->materials.size()) {
                    material << " (" << model.materials->materials[shape.material_index].name << ')';
                }
            }

            auto joint = std::ostringstream{};
            if (shape.joint_index == 0xffffU) {
                joint << "none";
            } else {
                joint << shape.joint_index;
                if (model.joints.has_value() && shape.joint_index < model.joints->joints.size()) {
                    joint << " (" << model.joints->joints[shape.joint_index].name << ')';
                }
            }

            out << "| " << shape.index << " | " << shape.draw_order << " | `" << shape.name << "` | " << material.str() << " | " << joint.str()
                << " | " << static_cast<int>(shape.matrix_type) << " | " << shape.matrix_group_count << " | " << shape.display_list_bytes << " | "
                << shape.parsed_display_list_bytes << " | " << shape.primitives.size() << " | " << shape.triangle_count << " | " << '['
                << shape.min[0] << ", " << shape.min[1] << ", " << shape.min[2] << "] -> [" << shape.max[0] << ", " << shape.max[1] << ", "
                << shape.max[2] << "] |\n";
        }

        out << "\n### Shape Vertex Descriptors and Primitives\n\n";
        for (const auto &shape : model.shapes->shapes) {
            out << "#### Shape " << shape.index << " `" << shape.name << "`\n\n";
            out << "Vertex descriptors: ";
            for (auto i = std::size_t{}; i < shape.vertex_desc.size(); ++i) {
                if (i != 0U) {
                    out << ", ";
                }
                out << smgpc::render::j3d_vertex_attr_name(shape.vertex_desc[i].attr) << '='
                    << smgpc::render::j3d_vertex_attr_type_name(shape.vertex_desc[i].type);
            }
            out << "\n\n";

            if (!shape.matrix_groups.empty()) {
                out << "Matrix groups:\n\n";
                out << "| group | use matrix | count | first table | table | dl offset | dl bytes | parsed bytes | triangles |\n";
                out << "| ---: | ---: | ---: | ---: | --- | ---: | ---: | ---: | ---: |\n";
                for (const auto &group : shape.matrix_groups) {
                    out << "| " << group.group_index << " | " << group.use_matrix_index << " | " << group.use_matrix_count << " | "
                        << group.first_matrix_table_index << " | ";
                    for (auto matrix = std::size_t{}; matrix < group.matrix_table.size(); ++matrix) {
                        if (matrix != 0U) {
                            out << ',';
                        }
                        out << group.matrix_table[matrix];
                    }
                    out << " | 0x" << std::hex << group.display_list_offset << std::dec << " | " << group.display_list_size << " | "
                        << group.parsed_display_list_bytes << " | " << group.triangle_count << " |\n";
                }
                out << '\n';
            }

            out << "| primitive | vertex format | vertices | triangles |\n";
            out << "| --- | ---: | ---: | ---: |\n";
            for (const auto &primitive : shape.primitives) {
                out << "| " << smgpc::render::j3d_primitive_name(primitive.primitive) << " | " << static_cast<int>(primitive.vertex_format) << " | "
                    << primitive.vertex_count << " | " << primitive.triangle_count << " |\n";
            }
            out << '\n';
        }
    }

    void write_textures(std::ofstream &out, const smgpc::render::J3dModelSummary &model) {
        out << "## TEX1\n\n";
        out << "| index | name | dimensions | format | wrap s | wrap t |\n";
        out << "| ---: | --- | --- | ---: | ---: | ---: |\n";
        for (auto i = std::size_t{}; i < model.textures.size(); ++i) {
            const auto &texture = model.textures[i];
            out << "| " << i << " | `" << texture.name << "` | " << texture.image.width << 'x' << texture.image.height << " | "
                << static_cast<std::uint32_t>(texture.image.format) << " | " << static_cast<int>(texture.wrap_s) << " | "
                << static_cast<int>(texture.wrap_t) << " |\n";
        }
        out << '\n';
    }

    void write_model_probe(const std::filesystem::path &output, std::string_view object_name, const smgpc::resource::RarcEntry &entry,
                           const smgpc::render::J3dModelSummary &model) {
        std::filesystem::create_directories(output.parent_path());

        auto out = std::ofstream(output);
        if (!out) {
            throw std::runtime_error("cannot write J3D model probe " + output.string());
        }

        out << "# J3D Model Probe: " << object_name << " / " << entry.path << "\n\n";
        out << "- magic: `" << smgpc::debug::tag_string(model.magic) << "`\n";
        out << "- model type: `" << smgpc::debug::tag_string(model.model_type) << "`\n";
        out << "- section count: " << model.section_count << "\n\n";
        write_sections(out, model);
        write_info(out, model);
        write_vertices(out, model);
        write_joints(out, model);
        write_matrix_state(out, model);
        write_materials(out, model);
        write_shapes(out, model);
        write_textures(out, model);
    }

}  // namespace

int main(int argc, char **argv) try {
    const auto object_name = argc > 1 ? std::string_view(argv[1]) : std::string_view("CometNearOrbitSky");
    const auto archive_path = smgpc::debug::disc_files_root() / "ObjectData" / (std::string(object_name) + ".arc");
    const auto archive = smgpc::resource::RarcArchive::from_file(archive_path);
    const auto output_root = smgpc::debug::cache_path("j3d-model-probes") / smgpc::debug::sanitize_filename(object_name, "model");

    auto model_count = 0U;
    for (const auto &entry : archive.entries()) {
        if (!smgpc::debug::ends_with_ignore_case(entry.path, ".bdl") && !smgpc::debug::ends_with_ignore_case(entry.path, ".bmd")) {
            continue;
        }

        const auto model = smgpc::render::inspect_j3d_model(archive.file_data(entry));
        const auto output = output_root / (smgpc::debug::sanitize_filename(std::filesystem::path(entry.path).stem().string(), "model") + ".md");
        write_model_probe(output, object_name, entry, model);
        std::cout << output << '\n';
        ++model_count;
    }

    if (model_count == 0U) {
        throw std::runtime_error("object archive contains no J3D model files: " + archive_path.string());
    }

    return 0;
} catch (const std::exception &e) {
    std::cerr << "J3D model probe failed: " << e.what() << '\n';
    return 1;
}
