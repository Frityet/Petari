#include "TraceStore.hpp"

#include <algorithm>
#include <initializer_list>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "Ndjson.hpp"

namespace smgpc::trace {
    namespace {

        using dump::Json;

        struct LayoutImportCounts {
            std::size_t layout_count = 0;
            std::size_t pane_count = 0;
            std::size_t material_count = 0;
            std::size_t texture_count = 0;
        };

        [[nodiscard]] const Json *member(const Json &json, std::string_view key) {
            if (!json.is_object()) {
                return nullptr;
            }

            const auto it = json.find(std::string(key));
            return it == json.end() ? nullptr : &*it;
        }

        [[nodiscard]] const Json *member(const Json &json, std::string_view first, std::string_view second) {
            const auto *first_member = member(json, first);
            return first_member == nullptr ? nullptr : member(*first_member, second);
        }

        [[nodiscard]] std::optional<std::int64_t> json_int(const Json &json) {
            if (json.is_number_integer() || json.is_number_unsigned()) {
                return json.get<std::int64_t>();
            }
            if (json.is_boolean()) {
                return json.get<bool>() ? 1 : 0;
            }
            return std::nullopt;
        }

        [[nodiscard]] std::optional<std::int64_t> json_int(const Json &json, std::string_view key) {
            const auto *value = member(json, key);
            return value == nullptr ? std::nullopt : json_int(*value);
        }

        [[nodiscard]] std::optional<std::int64_t> json_int(const Json &json, std::string_view first, std::string_view second) {
            const auto *value = member(json, first, second);
            return value == nullptr ? std::nullopt : json_int(*value);
        }

        [[nodiscard]] std::optional<std::string> json_text(const Json &json) {
            if (json.is_string()) {
                return json.get<std::string>();
            }
            if (json.is_number() || json.is_boolean()) {
                return json.dump();
            }
            return std::nullopt;
        }

        [[nodiscard]] std::optional<std::string> json_text(const Json &json, std::string_view key) {
            const auto *value = member(json, key);
            return value == nullptr ? std::nullopt : json_text(*value);
        }

        [[nodiscard]] std::string json_raw(const Json &json) {
            return json.dump();
        }

        [[nodiscard]] std::optional<std::string> member_raw(const Json &json, std::string_view key) {
            const auto *value = member(json, key);
            if (value == nullptr || value->is_null()) {
                return std::nullopt;
            }
            return json_raw(*value);
        }

        [[nodiscard]] std::optional<std::int64_t> json_array_size(const Json &json, std::string_view key) {
            const auto *value = member(json, key);
            if (value == nullptr || !value->is_array()) {
                return std::nullopt;
            }
            return static_cast<std::int64_t>(value->size());
        }

        [[nodiscard]] std::optional<std::int64_t> first_int(const Json &json, std::initializer_list<std::string_view> keys) {
            for (const auto key : keys) {
                const auto value = json_int(json, key);
                if (value.has_value()) {
                    return value;
                }
            }
            return std::nullopt;
        }

        [[nodiscard]] std::optional<std::int64_t> packet_texgen_count(const Json &packet) {
            if (const auto value = json_int(packet, "texgen_count"); value.has_value()) {
                return value;
            }
            return json_int(packet, "gen_mode", "texgen_count");
        }

        [[nodiscard]] std::optional<std::int64_t> packet_color_channel_count(const Json &packet) {
            if (const auto value = json_int(packet, "color_channel_count"); value.has_value()) {
                return value;
            }
            if (const auto value = json_int(packet, "gen_mode", "color_channel_count"); value.has_value()) {
                return value;
            }
            const auto *channels = member(packet, "color_channels");
            if (channels != nullptr && channels->is_array()) {
                return static_cast<std::int64_t>(channels->size());
            }
            return std::nullopt;
        }

        [[nodiscard]] std::optional<std::int64_t> packet_tev_stage_count(const Json &packet) {
            if (const auto value = first_int(packet, {"active_tev_stage_count", "declared_tev_stage_count", "tev_stage_count"}); value.has_value()) {
                return value;
            }
            return json_int(packet, "gen_mode", "tev_stage_count");
        }

        [[nodiscard]] std::optional<std::int64_t> packet_indirect_stage_count(const Json &packet) {
            if (const auto value = json_int(packet, "indirect_stage_count"); value.has_value()) {
                return value;
            }
            return json_int(packet, "gen_mode", "indirect_stage_count");
        }

        [[nodiscard]] std::optional<std::string> normalized_cull_mode(std::optional<std::string> value) {
            if (!value.has_value()) {
                return std::nullopt;
            }
            if (*value == "None") {
                return "0";
            }
            if (*value == "Back") {
                return "1";
            }
            if (*value == "Front") {
                return "2";
            }
            if (*value == "FrontAndBack") {
                return "3";
            }
            return value;
        }

        [[nodiscard]] std::optional<std::string> packet_cull_mode(const Json &packet) {
            if (const auto *gen_mode = member(packet, "gen_mode")) {
                if (const auto value = json_text(*gen_mode, "cull_mode"); value.has_value()) {
                    return normalized_cull_mode(value);
                }
            }
            return normalized_cull_mode(json_text(packet, "cull_mode"));
        }

        [[nodiscard]] std::optional<std::int64_t> packet_vertex_count(const Json &packet) {
            return first_int(packet, {"source_vertex_count", "num_vertices", "vertex_count"});
        }

        [[nodiscard]] std::optional<std::string> default_emulator_for_path(const std::filesystem::path &path) {
            const auto filename = path.filename().string();
            if (filename.find("dolphin") != std::string::npos) {
                return "dolphin";
            }
            if (filename.find("pcport") != std::string::npos || filename.find("pc-") != std::string::npos) {
                return "pc-port";
            }
            return std::nullopt;
        }

        [[nodiscard]] std::optional<std::string> trace_emulator(const Json &trace, const std::optional<std::string> &explicit_emulator) {
            if (explicit_emulator.has_value()) {
                return explicit_emulator;
            }
            return json_text(trace, "emulator");
        }

        [[nodiscard]] std::optional<std::int64_t> trace_frame_index(const Json &trace) {
            if (const auto value = json_int(trace, "requested_frame"); value.has_value()) {
                return value;
            }
            return json_int(trace, "frame", "index");
        }

        [[nodiscard]] Json base_record(std::string_view record_type, const Json &trace, const std::optional<std::string> &emulator) {
            auto record = Json{
                {"schema", std::string(kTraceNdjsonSchema)},
                {"record_type", std::string(record_type)},
            };
            if (const auto value = trace_emulator(trace, emulator); value.has_value()) {
                record["emulator"] = *value;
            }
            if (const auto value = trace_frame_index(trace); value.has_value()) {
                record["frame_index"] = *value;
            }
            return record;
        }

        [[nodiscard]] Json trace_meta_payload(const Json &trace, const std::optional<std::string> &emulator) {
            auto payload = Json::object();
            if (const auto *schema = member(trace, "schema"); schema != nullptr) {
                payload["source_schema"] = *schema;
            }
            if (const auto value = trace_emulator(trace, emulator); value.has_value()) {
                payload["emulator"] = *value;
            }
            if (const auto value = trace_frame_index(trace); value.has_value()) {
                payload["requested_frame"] = *value;
            }

            if (trace.is_object()) {
                for (const auto &[key, value] : trace.items()) {
                    if (key == "schema" || key == "emulator" || key == "requested_frame" || key == "frame" ||
                        key == "render_packets" || key == "copy_events") {
                        continue;
                    }
                    if (value.is_primitive()) {
                        payload[key] = value;
                    }
                }
            }
            return payload;
        }

        void append_payload_record(std::vector<Json> &records, const Json &trace, const std::optional<std::string> &emulator,
                                   std::string_view record_type, const Json &payload) {
            auto record = base_record(record_type, trace, emulator);
            record["payload"] = payload;
            records.push_back(std::move(record));
        }

        void append_indexed_payload_record(std::vector<Json> &records, const Json &trace, const std::optional<std::string> &emulator,
                                           std::string_view record_type, std::size_t record_index, const Json &payload) {
            auto record = base_record(record_type, trace, emulator);
            record["record_index"] = record_index;
            record["payload"] = payload;
            records.push_back(std::move(record));
        }

        [[nodiscard]] std::string records_to_ndjson_text(std::span<const Json> records) {
            auto out = std::ostringstream{};
            for (const auto &record : records) {
                out << record.dump() << '\n';
            }
            return out.str();
        }

        [[nodiscard]] std::optional<std::string> record_type(const Json &record) {
            if (const auto *value = member(record, "record_type"); value != nullptr && value->is_string()) {
                return value->get<std::string>();
            }
            return std::nullopt;
        }

        [[nodiscard]] const Json &record_payload(const Json &record) {
            const auto *payload = member(record, "payload");
            if (payload == nullptr) {
                throw std::runtime_error("NDJSON trace record is missing payload");
            }
            return *payload;
        }

        void insert_frame(sql::Database &db, std::int64_t trace_id, const Json &trace) {
            const auto *frame = member(trace, "frame");
            if (frame == nullptr) {
                return;
            }

            auto insert = sql::Statement(db, R"SQL(
                INSERT INTO frames(
                    trace_id, frame_index, runtime_index, presenter_frame_count, framebuffer_width, framebuffer_height, viewport_json, scissor_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
            )SQL");
            insert.bind(1, trace_id);
            insert.bind_optional_int(2, json_int(*frame, "index"));
            insert.bind_optional_int(3, json_int(*frame, "runtime_index"));
            insert.bind_optional_int(4, first_int(*frame, {"presenter_frame_count", "dolphin_presenter_frame_count"}));
            insert.bind_optional_int(5, json_int(*frame, "framebuffer", "width"));
            insert.bind_optional_int(6, json_int(*frame, "framebuffer", "height"));
            insert.bind_optional_text(7, member_raw(*frame, "viewport"));
            insert.bind_optional_text(8, member_raw(*frame, "scissor"));
            insert.step_done();
        }

        void insert_packet_children(sql::Database &db, std::int64_t trace_id, std::int64_t packet_index, const Json &packet) {
            auto insert_texture = sql::Statement(db, R"SQL(
                INSERT INTO packet_texture_bindings(
                    trace_id, packet_index, binding_index, slot, texture_index, name, identity_name, address, width, height, format, format_raw, payload_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            )SQL");
            if (const auto *bindings = member(packet, "texture_bindings"); bindings != nullptr && bindings->is_array()) {
                for (auto i = std::size_t{}; i < bindings->size(); ++i) {
                    const auto &binding = (*bindings)[i];
                    if (!binding.is_object()) {
                        continue;
                    }
                    insert_texture.bind(1, trace_id);
                    insert_texture.bind(2, packet_index);
                    insert_texture.bind(3, static_cast<std::int64_t>(i));
                    insert_texture.bind_optional_int(4, json_int(binding, "slot"));
                    insert_texture.bind_optional_int(5, json_int(binding, "texture_index"));
                    insert_texture.bind_optional_text(6, json_text(binding, "name"));
                    insert_texture.bind_optional_text(7, json_text(binding, "identity_name"));
                    insert_texture.bind_optional_int(8, json_int(binding, "address"));
                    insert_texture.bind_optional_int(9, json_int(binding, "width"));
                    insert_texture.bind_optional_int(10, json_int(binding, "height"));
                    insert_texture.bind_optional_text(11, json_text(binding, "format"));
                    insert_texture.bind_optional_int(12, json_int(binding, "format_raw"));
                    insert_texture.bind(13, json_raw(binding));
                    insert_texture.step_done();
                }
            }

            auto insert_order = sql::Statement(db, R"SQL(
                INSERT INTO packet_tev_orders(
                    trace_id, packet_index, order_index, stage, tex_coord, tex_map, color_channel, texture_enabled, payload_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
            )SQL");
            if (const auto *orders = member(packet, "tev_orders"); orders != nullptr && orders->is_array()) {
                for (auto i = std::size_t{}; i < orders->size(); ++i) {
                    const auto &order = (*orders)[i];
                    if (!order.is_object()) {
                        continue;
                    }
                    insert_order.bind(1, trace_id);
                    insert_order.bind(2, packet_index);
                    insert_order.bind(3, static_cast<std::int64_t>(i));
                    insert_order.bind_optional_int(4, json_int(order, "stage"));
                    insert_order.bind_optional_int(5, json_int(order, "tex_coord"));
                    insert_order.bind_optional_int(6, json_int(order, "tex_map"));
                    insert_order.bind_optional_int(7, json_int(order, "color_channel"));
                    insert_order.bind_optional_int(8, json_int(order, "texture_enabled"));
                    insert_order.bind(9, json_raw(order));
                    insert_order.step_done();
                }
            }

            auto insert_stage = sql::Statement(db, R"SQL(
                INSERT INTO packet_tev_stages(
                    trace_id, packet_index, stage_index, stage, color_raw, alpha_raw, color_in_json, alpha_in_json, k_color_sel, k_alpha_sel, payload_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            )SQL");
            if (const auto *stages = member(packet, "tev_stages"); stages != nullptr && stages->is_array()) {
                for (auto i = std::size_t{}; i < stages->size(); ++i) {
                    const auto &stage = (*stages)[i];
                    if (!stage.is_object()) {
                        continue;
                    }
                    insert_stage.bind(1, trace_id);
                    insert_stage.bind(2, packet_index);
                    insert_stage.bind(3, static_cast<std::int64_t>(i));
                    insert_stage.bind_optional_int(4, json_int(stage, "stage"));
                    insert_stage.bind_optional_int(5, json_int(stage, "color_raw"));
                    insert_stage.bind_optional_int(6, json_int(stage, "alpha_raw"));
                    insert_stage.bind_optional_text(7, member_raw(stage, "color_in"));
                    insert_stage.bind_optional_text(8, member_raw(stage, "alpha_in"));
                    insert_stage.bind_optional_int(9, json_int(stage, "k_color_sel"));
                    insert_stage.bind_optional_int(10, json_int(stage, "k_alpha_sel"));
                    insert_stage.bind(11, json_raw(stage));
                    insert_stage.step_done();
                }
            }

            auto insert_light = sql::Statement(db, R"SQL(
                INSERT INTO packet_lights(
                    trace_id, packet_index, light_row, light_index, color_json, position_json, direction_json, payload_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
            )SQL");
            if (const auto *lights = member(packet, "lights"); lights != nullptr && lights->is_array()) {
                for (auto i = std::size_t{}; i < lights->size(); ++i) {
                    const auto &light = (*lights)[i];
                    insert_light.bind(1, trace_id);
                    insert_light.bind(2, packet_index);
                    insert_light.bind(3, static_cast<std::int64_t>(i));
                    if (light.is_object()) {
                        insert_light.bind_optional_int(4, json_int(light, "index"));
                        insert_light.bind_optional_text(5, member_raw(light, "color"));
                        insert_light.bind_optional_text(6, member_raw(light, "position"));
                        insert_light.bind_optional_text(7, member_raw(light, "direction"));
                    } else {
                        insert_light.bind_optional_int(4, json_int(light));
                        insert_light.bind(5, nullptr);
                        insert_light.bind(6, nullptr);
                        insert_light.bind(7, nullptr);
                    }
                    insert_light.bind(8, json_raw(light));
                    insert_light.step_done();
                }
            }
        }

        std::size_t insert_render_packets(sql::Database &db, std::int64_t trace_id, const Json &trace) {
            auto insert = sql::Statement(db, R"SQL(
                INSERT INTO render_packets(
                    trace_id, packet_index, draw_index, frame_index, presenter_frame_count, model_name, material_name, render_pass, draw_pass, view_id,
                    packet_mode, primitive_type, vertex_count, index_count, source_vertex_count, source_triangle_count, texgen_count,
                    color_channel_count, tev_stage_count, indirect_stage_count, cull_mode, used_textures_mask, used_texture_slots_json,
                    requested_light_mask, loaded_light_mask, payload_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            )SQL");

            const auto *packets = member(trace, "render_packets");
            if (packets == nullptr || !packets->is_array()) {
                return 0;
            }

            auto inserted = std::size_t{};
            for (auto i = std::size_t{}; i < packets->size(); ++i) {
                const auto &packet = (*packets)[i];
                if (!packet.is_object()) {
                    continue;
                }

                const auto packet_index = json_int(packet, "index").value_or(static_cast<std::int64_t>(i));
                insert.bind(1, trace_id);
                insert.bind(2, packet_index);
                insert.bind_optional_int(3, json_int(packet, "draw_index"));
                insert.bind_optional_int(4, json_int(packet, "frame_index"));
                insert.bind_optional_int(5, json_int(packet, "presenter_frame_count"));
                insert.bind_optional_text(6, json_text(packet, "model_name"));
                insert.bind_optional_text(7, json_text(packet, "material_name"));
                insert.bind_optional_text(8, json_text(packet, "render_pass"));
                insert.bind_optional_text(9, json_text(packet, "draw_pass"));
                insert.bind_optional_int(10, json_int(packet, "view_id"));
                insert.bind_optional_text(11, json_text(packet, "packet_mode"));
                insert.bind_optional_text(12, json_text(packet, "primitive_type"));
                insert.bind_optional_int(13, packet_vertex_count(packet));
                insert.bind_optional_int(14, json_int(packet, "num_indices"));
                insert.bind_optional_int(15, json_int(packet, "source_vertex_count"));
                insert.bind_optional_int(16, json_int(packet, "source_triangle_count"));
                insert.bind_optional_int(17, packet_texgen_count(packet));
                insert.bind_optional_int(18, packet_color_channel_count(packet));
                insert.bind_optional_int(19, packet_tev_stage_count(packet));
                insert.bind_optional_int(20, packet_indirect_stage_count(packet));
                insert.bind_optional_text(21, packet_cull_mode(packet));
                insert.bind_optional_int(22, json_int(packet, "used_textures_mask"));
                insert.bind_optional_text(23, member_raw(packet, "used_texture_slots"));
                insert.bind_optional_int(24, json_int(packet, "requested_light_mask"));
                insert.bind_optional_int(25, json_int(packet, "loaded_light_mask"));
                insert.bind(26, json_raw(packet));
                insert.step_done();

                insert_packet_children(db, trace_id, packet_index, packet);
                ++inserted;
            }
            return inserted;
        }

        std::size_t insert_copy_events(sql::Database &db, std::int64_t trace_id, const Json &trace) {
            auto insert = sql::Statement(db, R"SQL(
                INSERT INTO copy_events(
                    trace_id, row_index, event_index, presenter_frame_count, kind, source_width, source_height, output_width, output_height,
                    target_width, target_height, payload_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            )SQL");

            const auto *events = member(trace, "copy_events");
            if (events == nullptr || !events->is_array()) {
                return 0;
            }

            auto inserted = std::size_t{};
            for (auto i = std::size_t{}; i < events->size(); ++i) {
                const auto &event = (*events)[i];
                if (!event.is_object()) {
                    continue;
                }
                insert.bind(1, trace_id);
                insert.bind(2, static_cast<std::int64_t>(i));
                insert.bind_optional_int(3, json_int(event, "event_index"));
                insert.bind_optional_int(4, json_int(event, "presenter_frame_count"));
                insert.bind_optional_text(5, json_text(event, "kind"));
                insert.bind_optional_int(6, json_int(event, "source_rect", "width"));
                insert.bind_optional_int(7, json_int(event, "source_rect", "height"));
                insert.bind_optional_int(8, json_int(event, "output_size", "width"));
                insert.bind_optional_int(9, json_int(event, "output_size", "height"));
                insert.bind_optional_int(10, json_int(event, "target_rect", "width"));
                insert.bind_optional_int(11, json_int(event, "target_rect", "height"));
                insert.bind(12, json_raw(event));
                insert.step_done();
                ++inserted;
            }
            return inserted;
        }

        std::size_t insert_semantic_events(sql::Database &db, std::int64_t trace_id, const Json &trace) {
            auto insert = sql::Statement(db, R"SQL(
                INSERT INTO semantic_events(
                    trace_id, row_index, event_index, frame_index, category, name, detail, stage, source, payload_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            )SQL");

            const auto *events = member(trace, "semantic_events");
            if (events == nullptr || !events->is_array()) {
                return 0;
            }

            auto inserted = std::size_t{};
            for (auto i = std::size_t{}; i < events->size(); ++i) {
                const auto &event = (*events)[i];
                if (!event.is_object()) {
                    continue;
                }
                insert.bind(1, trace_id);
                insert.bind(2, static_cast<std::int64_t>(i));
                insert.bind_optional_int(3, json_int(event, "index"));
                insert.bind_optional_int(4, json_int(event, "frame_index"));
                insert.bind_optional_text(5, json_text(event, "category"));
                insert.bind_optional_text(6, json_text(event, "name"));
                insert.bind_optional_text(7, json_text(event, "detail"));
                insert.bind_optional_text(8, json_text(event, "stage"));
                insert.bind_optional_text(9, json_text(event, "source"));
                insert.bind(10, json_raw(event));
                insert.step_done();
                ++inserted;
            }
            return inserted;
        }

        LayoutImportCounts insert_layout_runtime(sql::Database &db, std::int64_t trace_id, const Json &trace) {
            auto counts = LayoutImportCounts{};
            const auto *layouts = member(trace, "layout_runtime");
            if (layouts == nullptr || !layouts->is_array()) {
                return counts;
            }

            auto insert_layout = sql::Statement(db, R"SQL(
                INSERT INTO layout_runtime(
                    trace_id, row_index, layout_index, name, layout_name, archive_path, movement_type, calc_anim_type,
                    draw_type, order_index, dead, suspended, pane_count, picture_count, text_box_count, material_count,
                    texture_count, font_count, animation_count, committed_pane_frame_count, payload_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            )SQL");
            auto insert_pane = sql::Statement(db, R"SQL(
                INSERT INTO layout_runtime_panes(
                    trace_id, layout_row, pane_index, name, parent_index, base_visible, effective_visible, alpha,
                    width, height, content_count, payload_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            )SQL");
            auto insert_content = sql::Statement(db, R"SQL(
                INSERT INTO layout_runtime_pane_contents(
                    trace_id, layout_row, pane_index, content_index, kind, material_index, material_name, texture_name, payload_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
            )SQL");
            auto insert_material = sql::Statement(db, R"SQL(
                INSERT INTO layout_runtime_materials(
                    trace_id, layout_row, material_index, name, texture_count, tex_coord_gen_count, tev_stage_count,
                    alpha_compare_enabled, blend_enabled, payload_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            )SQL");
            auto insert_material_texture = sql::Statement(db, R"SQL(
                INSERT INTO layout_runtime_material_textures(
                    trace_id, layout_row, material_index, texture_slot_index, slot, texture_index, texture_name,
                    wrap_s, wrap_t, min_filter, mag_filter, payload_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            )SQL");
            auto insert_texture = sql::Statement(db, R"SQL(
                INSERT INTO layout_runtime_textures(
                    trace_id, layout_row, texture_index, name, width, height, format, format_raw,
                    uploaded, rgba_byte_count, payload_json
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            )SQL");

            for (auto layout_row = std::size_t{}; layout_row < layouts->size(); ++layout_row) {
                const auto &layout = (*layouts)[layout_row];
                if (!layout.is_object()) {
                    continue;
                }

                insert_layout.bind(1, trace_id);
                insert_layout.bind(2, static_cast<std::int64_t>(layout_row));
                insert_layout.bind_optional_int(3, json_int(layout, "index"));
                insert_layout.bind_optional_text(4, json_text(layout, "name"));
                insert_layout.bind_optional_text(5, json_text(layout, "layout_name"));
                insert_layout.bind_optional_text(6, json_text(layout, "archive_path"));
                insert_layout.bind_optional_int(7, json_int(layout, "movement_type"));
                insert_layout.bind_optional_int(8, json_int(layout, "calc_anim_type"));
                insert_layout.bind_optional_int(9, json_int(layout, "draw_type"));
                insert_layout.bind_optional_int(10, json_int(layout, "order"));
                insert_layout.bind_optional_int(11, json_int(layout, "dead"));
                insert_layout.bind_optional_int(12, json_int(layout, "suspended"));
                insert_layout.bind_optional_int(13, json_int(layout, "pane_count"));
                insert_layout.bind_optional_int(14, json_int(layout, "picture_count"));
                insert_layout.bind_optional_int(15, json_int(layout, "text_box_count"));
                insert_layout.bind_optional_int(16, json_int(layout, "material_count"));
                insert_layout.bind_optional_int(17, json_int(layout, "texture_count"));
                insert_layout.bind_optional_int(18, json_int(layout, "font_count"));
                insert_layout.bind_optional_int(19, json_array_size(layout, "animations"));
                insert_layout.bind_optional_int(20, json_int(layout, "committed_pane_frame_count"));
                insert_layout.bind(21, json_raw(layout));
                insert_layout.step_done();
                ++counts.layout_count;

                if (const auto *panes = member(layout, "panes"); panes != nullptr && panes->is_array()) {
                    for (auto pane_row = std::size_t{}; pane_row < panes->size(); ++pane_row) {
                        const auto &pane = (*panes)[pane_row];
                        if (!pane.is_object()) {
                            continue;
                        }

                        const auto pane_index = json_int(pane, "index").value_or(static_cast<std::int64_t>(pane_row));
                        insert_pane.bind(1, trace_id);
                        insert_pane.bind(2, static_cast<std::int64_t>(layout_row));
                        insert_pane.bind(3, pane_index);
                        insert_pane.bind_optional_text(4, json_text(pane, "name"));
                        insert_pane.bind_optional_int(5, json_int(pane, "parent_index"));
                        insert_pane.bind_optional_int(6, json_int(pane, "base_visible"));
                        insert_pane.bind_optional_int(7, json_int(pane, "effective_visible"));
                        insert_pane.bind_optional_int(8, json_int(pane, "alpha"));
                        insert_pane.bind_optional_int(9, json_int(pane, "width"));
                        insert_pane.bind_optional_int(10, json_int(pane, "height"));
                        insert_pane.bind_optional_int(11, json_array_size(pane, "contents"));
                        insert_pane.bind(12, json_raw(pane));
                        insert_pane.step_done();
                        ++counts.pane_count;

                        if (const auto *contents = member(pane, "contents"); contents != nullptr && contents->is_array()) {
                            for (auto content_row = std::size_t{}; content_row < contents->size(); ++content_row) {
                                const auto &content = (*contents)[content_row];
                                if (!content.is_object()) {
                                    continue;
                                }
                                insert_content.bind(1, trace_id);
                                insert_content.bind(2, static_cast<std::int64_t>(layout_row));
                                insert_content.bind(3, pane_index);
                                insert_content.bind(4, static_cast<std::int64_t>(content_row));
                                insert_content.bind_optional_text(5, json_text(content, "kind"));
                                insert_content.bind_optional_int(6, json_int(content, "material_index"));
                                insert_content.bind_optional_text(7, json_text(content, "material_name"));
                                insert_content.bind_optional_text(8, json_text(content, "texture_name"));
                                insert_content.bind(9, json_raw(content));
                                insert_content.step_done();
                            }
                        }
                    }
                }

                if (const auto *materials = member(layout, "materials"); materials != nullptr && materials->is_array()) {
                    for (auto material_row = std::size_t{}; material_row < materials->size(); ++material_row) {
                        const auto &material = (*materials)[material_row];
                        if (!material.is_object()) {
                            continue;
                        }

                        const auto material_index = json_int(material, "index").value_or(static_cast<std::int64_t>(material_row));
                        insert_material.bind(1, trace_id);
                        insert_material.bind(2, static_cast<std::int64_t>(layout_row));
                        insert_material.bind(3, material_index);
                        insert_material.bind_optional_text(4, json_text(material, "name"));
                        insert_material.bind_optional_int(5, json_int(material, "texture_count"));
                        insert_material.bind_optional_int(6, json_int(material, "tex_coord_gen_count"));
                        insert_material.bind_optional_int(7, json_int(material, "tev_stage_count"));
                        insert_material.bind_optional_int(8, json_int(material, "alpha_compare_enabled"));
                        insert_material.bind_optional_int(9, json_int(material, "blend_enabled"));
                        insert_material.bind(10, json_raw(material));
                        insert_material.step_done();
                        ++counts.material_count;

                        if (const auto *textures = member(material, "textures"); textures != nullptr && textures->is_array()) {
                            for (auto texture_row = std::size_t{}; texture_row < textures->size(); ++texture_row) {
                                const auto &texture = (*textures)[texture_row];
                                if (!texture.is_object()) {
                                    continue;
                                }
                                insert_material_texture.bind(1, trace_id);
                                insert_material_texture.bind(2, static_cast<std::int64_t>(layout_row));
                                insert_material_texture.bind(3, material_index);
                                insert_material_texture.bind(4, static_cast<std::int64_t>(texture_row));
                                insert_material_texture.bind_optional_int(5, json_int(texture, "slot"));
                                insert_material_texture.bind_optional_int(6, json_int(texture, "texture_index"));
                                insert_material_texture.bind_optional_text(7, json_text(texture, "texture_name"));
                                insert_material_texture.bind_optional_int(8, json_int(texture, "wrap_s"));
                                insert_material_texture.bind_optional_int(9, json_int(texture, "wrap_t"));
                                insert_material_texture.bind_optional_int(10, json_int(texture, "min_filter"));
                                insert_material_texture.bind_optional_int(11, json_int(texture, "mag_filter"));
                                insert_material_texture.bind(12, json_raw(texture));
                                insert_material_texture.step_done();
                            }
                        }
                    }
                }

                if (const auto *textures = member(layout, "textures"); textures != nullptr && textures->is_array()) {
                    for (auto texture_row = std::size_t{}; texture_row < textures->size(); ++texture_row) {
                        const auto &texture = (*textures)[texture_row];
                        if (!texture.is_object()) {
                            continue;
                        }

                        insert_texture.bind(1, trace_id);
                        insert_texture.bind(2, static_cast<std::int64_t>(layout_row));
                        insert_texture.bind_optional_int(3, json_int(texture, "index"));
                        insert_texture.bind_optional_text(4, json_text(texture, "name"));
                        insert_texture.bind_optional_int(5, json_int(texture, "width"));
                        insert_texture.bind_optional_int(6, json_int(texture, "height"));
                        insert_texture.bind_optional_text(7, json_text(texture, "format"));
                        insert_texture.bind_optional_int(8, json_int(texture, "format_raw"));
                        insert_texture.bind_optional_int(9, json_int(texture, "uploaded"));
                        insert_texture.bind_optional_int(10, json_int(texture, "rgba_byte_count"));
                        insert_texture.bind(11, json_raw(texture));
                        insert_texture.step_done();
                        ++counts.texture_count;
                    }
                }
            }
            return counts;
        }

    }  // namespace

    std::vector<dump::Json> trace_ndjson_records_from_json(const dump::Json &trace, std::optional<std::string> emulator) {
        auto records = std::vector<Json>{};

        append_payload_record(records, trace, emulator, "trace_meta", trace_meta_payload(trace, emulator));

        if (const auto *frame = member(trace, "frame"); frame != nullptr) {
            append_payload_record(records, trace, emulator, "frame", *frame);
        }

        if (trace.is_object()) {
            for (const auto &[key, value] : trace.items()) {
                if (key == "schema" || key == "emulator" || key == "requested_frame" || key == "frame" ||
                    key == "render_packets" || key == "copy_events" || key == "semantic_events" || value.is_primitive()) {
                    continue;
                }
                auto record = base_record("top_level", trace, emulator);
                record["key"] = key;
                record["payload"] = value;
                records.push_back(std::move(record));
            }
        }

        if (const auto *packets = member(trace, "render_packets"); packets != nullptr && packets->is_array()) {
            for (auto i = std::size_t{}; i < packets->size(); ++i) {
                append_indexed_payload_record(records, trace, emulator, "render_packet", i, (*packets)[i]);
            }
        }

        if (const auto *events = member(trace, "copy_events"); events != nullptr && events->is_array()) {
            for (auto i = std::size_t{}; i < events->size(); ++i) {
                append_indexed_payload_record(records, trace, emulator, "copy_event", i, (*events)[i]);
            }
        }

        if (const auto *events = member(trace, "semantic_events"); events != nullptr && events->is_array()) {
            for (auto i = std::size_t{}; i < events->size(); ++i) {
                append_indexed_payload_record(records, trace, emulator, "semantic_event", i, (*events)[i]);
            }
        }

        return records;
    }

    dump::Json trace_json_from_ndjson_records(std::span<const dump::Json> records) {
        auto trace = Json::object();
        trace["render_packets"] = Json::array();
        trace["copy_events"] = Json::array();
        trace["semantic_events"] = Json::array();

        for (const auto &record : records) {
            const auto schema = json_text(record, "schema");
            if (!schema.has_value() || *schema != kTraceNdjsonSchema) {
                throw std::runtime_error("NDJSON trace record has an unsupported schema");
            }

            const auto type = record_type(record);
            if (!type.has_value()) {
                throw std::runtime_error("NDJSON trace record is missing record_type");
            }

            const auto &payload = record_payload(record);
            if (*type == "trace_meta") {
                if (!payload.is_object()) {
                    throw std::runtime_error("trace_meta payload must be an object");
                }
                for (const auto &[key, value] : payload.items()) {
                    if (key == "source_schema") {
                        trace["schema"] = value;
                    } else {
                        trace[key] = value;
                    }
                }
            } else if (*type == "frame") {
                trace["frame"] = payload;
            } else if (*type == "top_level") {
                const auto key = json_text(record, "key");
                if (!key.has_value()) {
                    throw std::runtime_error("top_level NDJSON trace record is missing key");
                }
                trace[*key] = payload;
            } else if (*type == "render_packet") {
                trace["render_packets"].push_back(payload);
            } else if (*type == "copy_event") {
                trace["copy_events"].push_back(payload);
            } else if (*type == "semantic_event") {
                trace["semantic_events"].push_back(payload);
            } else {
                throw std::runtime_error("NDJSON trace record has unknown record_type " + *type);
            }
        }

        return trace;
    }

    void write_trace_ndjson_file(const std::filesystem::path &path, const dump::Json &trace, std::optional<std::string> emulator) {
        dump::write_ndjson_file(path, trace_ndjson_records_from_json(trace, std::move(emulator)));
    }

    dump::Json load_trace_ndjson_file(const std::filesystem::path &path) {
        const auto records = dump::load_ndjson_file(path);
        return trace_json_from_ndjson_records(records);
    }

    void create_trace_sqlite_schema(sql::Database &db) {
        db.exec(R"SQL(
            CREATE TABLE IF NOT EXISTS traces (
                id INTEGER PRIMARY KEY,
                path TEXT NOT NULL UNIQUE,
                trace_format TEXT NOT NULL,
                schema_name TEXT,
                emulator TEXT,
                requested_frame INTEGER,
                record_count INTEGER NOT NULL,
                raw_ndjson TEXT NOT NULL
            );
            CREATE TABLE IF NOT EXISTS frames (
                trace_id INTEGER PRIMARY KEY REFERENCES traces(id) ON DELETE CASCADE,
                frame_index INTEGER,
                runtime_index INTEGER,
                presenter_frame_count INTEGER,
                framebuffer_width INTEGER,
                framebuffer_height INTEGER,
                viewport_json TEXT,
                scissor_json TEXT
            );
            CREATE TABLE IF NOT EXISTS render_packets (
                trace_id INTEGER NOT NULL REFERENCES traces(id) ON DELETE CASCADE,
                packet_index INTEGER NOT NULL,
                draw_index INTEGER,
                frame_index INTEGER,
                presenter_frame_count INTEGER,
                model_name TEXT,
                material_name TEXT,
                render_pass TEXT,
                draw_pass TEXT,
                view_id INTEGER,
                packet_mode TEXT,
                primitive_type TEXT,
                vertex_count INTEGER,
                index_count INTEGER,
                source_vertex_count INTEGER,
                source_triangle_count INTEGER,
                texgen_count INTEGER,
                color_channel_count INTEGER,
                tev_stage_count INTEGER,
                indirect_stage_count INTEGER,
                cull_mode TEXT,
                used_textures_mask INTEGER,
                used_texture_slots_json TEXT,
                requested_light_mask INTEGER,
                loaded_light_mask INTEGER,
                payload_json TEXT NOT NULL,
                PRIMARY KEY(trace_id, packet_index)
            );
            CREATE TABLE IF NOT EXISTS packet_texture_bindings (
                trace_id INTEGER NOT NULL,
                packet_index INTEGER NOT NULL,
                binding_index INTEGER NOT NULL,
                slot INTEGER,
                texture_index INTEGER,
                name TEXT,
                identity_name TEXT,
                address INTEGER,
                width INTEGER,
                height INTEGER,
                format TEXT,
                format_raw INTEGER,
                payload_json TEXT NOT NULL,
                PRIMARY KEY(trace_id, packet_index, binding_index)
            );
            CREATE TABLE IF NOT EXISTS packet_tev_orders (
                trace_id INTEGER NOT NULL,
                packet_index INTEGER NOT NULL,
                order_index INTEGER NOT NULL,
                stage INTEGER,
                tex_coord INTEGER,
                tex_map INTEGER,
                color_channel INTEGER,
                texture_enabled INTEGER,
                payload_json TEXT NOT NULL,
                PRIMARY KEY(trace_id, packet_index, order_index)
            );
            CREATE TABLE IF NOT EXISTS packet_tev_stages (
                trace_id INTEGER NOT NULL,
                packet_index INTEGER NOT NULL,
                stage_index INTEGER NOT NULL,
                stage INTEGER,
                color_raw INTEGER,
                alpha_raw INTEGER,
                color_in_json TEXT,
                alpha_in_json TEXT,
                k_color_sel INTEGER,
                k_alpha_sel INTEGER,
                payload_json TEXT NOT NULL,
                PRIMARY KEY(trace_id, packet_index, stage_index)
            );
            CREATE TABLE IF NOT EXISTS packet_lights (
                trace_id INTEGER NOT NULL,
                packet_index INTEGER NOT NULL,
                light_row INTEGER NOT NULL,
                light_index INTEGER,
                color_json TEXT,
                position_json TEXT,
                direction_json TEXT,
                payload_json TEXT NOT NULL,
                PRIMARY KEY(trace_id, packet_index, light_row)
            );
            CREATE TABLE IF NOT EXISTS copy_events (
                trace_id INTEGER NOT NULL REFERENCES traces(id) ON DELETE CASCADE,
                row_index INTEGER NOT NULL,
                event_index INTEGER,
                presenter_frame_count INTEGER,
                kind TEXT,
                source_width INTEGER,
                source_height INTEGER,
                output_width INTEGER,
                output_height INTEGER,
                target_width INTEGER,
                target_height INTEGER,
                payload_json TEXT NOT NULL,
                PRIMARY KEY(trace_id, row_index)
            );
            CREATE TABLE IF NOT EXISTS semantic_events (
                trace_id INTEGER NOT NULL REFERENCES traces(id) ON DELETE CASCADE,
                row_index INTEGER NOT NULL,
                event_index INTEGER,
                frame_index INTEGER,
                category TEXT,
                name TEXT,
                detail TEXT,
                stage TEXT,
                source TEXT,
                payload_json TEXT NOT NULL,
                PRIMARY KEY(trace_id, row_index)
            );
            CREATE TABLE IF NOT EXISTS layout_runtime (
                trace_id INTEGER NOT NULL REFERENCES traces(id) ON DELETE CASCADE,
                row_index INTEGER NOT NULL,
                layout_index INTEGER,
                name TEXT,
                layout_name TEXT,
                archive_path TEXT,
                movement_type INTEGER,
                calc_anim_type INTEGER,
                draw_type INTEGER,
                order_index INTEGER,
                dead INTEGER,
                suspended INTEGER,
                pane_count INTEGER,
                picture_count INTEGER,
                text_box_count INTEGER,
                material_count INTEGER,
                texture_count INTEGER,
                font_count INTEGER,
                animation_count INTEGER,
                committed_pane_frame_count INTEGER,
                payload_json TEXT NOT NULL,
                PRIMARY KEY(trace_id, row_index)
            );
            CREATE TABLE IF NOT EXISTS layout_runtime_panes (
                trace_id INTEGER NOT NULL REFERENCES traces(id) ON DELETE CASCADE,
                layout_row INTEGER NOT NULL,
                pane_index INTEGER NOT NULL,
                name TEXT,
                parent_index INTEGER,
                base_visible INTEGER,
                effective_visible INTEGER,
                alpha INTEGER,
                width INTEGER,
                height INTEGER,
                content_count INTEGER,
                payload_json TEXT NOT NULL,
                PRIMARY KEY(trace_id, layout_row, pane_index)
            );
            CREATE TABLE IF NOT EXISTS layout_runtime_pane_contents (
                trace_id INTEGER NOT NULL REFERENCES traces(id) ON DELETE CASCADE,
                layout_row INTEGER NOT NULL,
                pane_index INTEGER NOT NULL,
                content_index INTEGER NOT NULL,
                kind TEXT,
                material_index INTEGER,
                material_name TEXT,
                texture_name TEXT,
                payload_json TEXT NOT NULL,
                PRIMARY KEY(trace_id, layout_row, pane_index, content_index)
            );
            CREATE TABLE IF NOT EXISTS layout_runtime_materials (
                trace_id INTEGER NOT NULL REFERENCES traces(id) ON DELETE CASCADE,
                layout_row INTEGER NOT NULL,
                material_index INTEGER NOT NULL,
                name TEXT,
                texture_count INTEGER,
                tex_coord_gen_count INTEGER,
                tev_stage_count INTEGER,
                alpha_compare_enabled INTEGER,
                blend_enabled INTEGER,
                payload_json TEXT NOT NULL,
                PRIMARY KEY(trace_id, layout_row, material_index)
            );
            CREATE TABLE IF NOT EXISTS layout_runtime_material_textures (
                trace_id INTEGER NOT NULL REFERENCES traces(id) ON DELETE CASCADE,
                layout_row INTEGER NOT NULL,
                material_index INTEGER NOT NULL,
                texture_slot_index INTEGER NOT NULL,
                slot INTEGER,
                texture_index INTEGER,
                texture_name TEXT,
                wrap_s INTEGER,
                wrap_t INTEGER,
                min_filter INTEGER,
                mag_filter INTEGER,
                payload_json TEXT NOT NULL,
                PRIMARY KEY(trace_id, layout_row, material_index, texture_slot_index)
            );
            CREATE TABLE IF NOT EXISTS layout_runtime_textures (
                trace_id INTEGER NOT NULL REFERENCES traces(id) ON DELETE CASCADE,
                layout_row INTEGER NOT NULL,
                texture_index INTEGER NOT NULL,
                name TEXT,
                width INTEGER,
                height INTEGER,
                format TEXT,
                format_raw INTEGER,
                uploaded INTEGER,
                rgba_byte_count INTEGER,
                payload_json TEXT NOT NULL,
                PRIMARY KEY(trace_id, layout_row, texture_index)
            );
            CREATE INDEX IF NOT EXISTS idx_render_packets_material ON render_packets(material_name);
            CREATE INDEX IF NOT EXISTS idx_render_packets_signature ON render_packets(texgen_count, color_channel_count, tev_stage_count, indirect_stage_count, cull_mode, vertex_count);
            CREATE INDEX IF NOT EXISTS idx_texture_bindings_signature ON packet_texture_bindings(slot, format, width, height);
            CREATE INDEX IF NOT EXISTS idx_semantic_events_name ON semantic_events(category, name, frame_index);
            CREATE INDEX IF NOT EXISTS idx_layout_runtime_name ON layout_runtime(name, layout_name, dead, suspended);
            CREATE INDEX IF NOT EXISTS idx_layout_runtime_panes_name ON layout_runtime_panes(name, effective_visible);
            CREATE INDEX IF NOT EXISTS idx_layout_runtime_materials_name ON layout_runtime_materials(name);
            CREATE INDEX IF NOT EXISTS idx_layout_runtime_textures_name ON layout_runtime_textures(name, format, width, height);
            CREATE VIEW IF NOT EXISTS packet_signatures AS
                SELECT
                    rp.trace_id,
                    rp.packet_index,
                    rp.model_name,
                    rp.material_name,
                    rp.render_pass,
                    rp.packet_mode,
                    rp.texgen_count,
                    rp.color_channel_count,
                    rp.tev_stage_count,
                    rp.indirect_stage_count,
                    rp.cull_mode,
                    rp.vertex_count,
                    rp.requested_light_mask,
                    group_concat(ptb.slot || ':' || coalesce(ptb.format, '') || ':' || coalesce(ptb.width, '') || 'x' || coalesce(ptb.height, ''), '|') AS texture_signature
                FROM render_packets rp
                LEFT JOIN packet_texture_bindings ptb ON ptb.trace_id = rp.trace_id AND ptb.packet_index = rp.packet_index
                GROUP BY rp.trace_id, rp.packet_index;
        )SQL");
    }

    ImportResult import_trace_ndjson_file(sql::Database &db, const std::filesystem::path &path) {
        const auto records = dump::load_ndjson_file(path);
        const auto trace = trace_json_from_ndjson_records(records);
        const auto ndjson_text = records_to_ndjson_text(records);

        auto insert = sql::Statement(db, R"SQL(
            INSERT INTO traces(path, trace_format, schema_name, emulator, requested_frame, record_count, raw_ndjson)
            VALUES(?, ?, ?, ?, ?, ?, ?)
        )SQL");

        insert.bind(1, std::filesystem::absolute(path).string());
        insert.bind(2, "ndjson");
        insert.bind_optional_text(3, json_text(trace, "schema"));
        auto emulator = json_text(trace, "emulator");
        if (!emulator.has_value()) {
            emulator = default_emulator_for_path(path);
        }
        insert.bind_optional_text(4, emulator);
        insert.bind_optional_int(5, json_int(trace, "requested_frame"));
        insert.bind(6, static_cast<std::int64_t>(records.size()));
        insert.bind(7, ndjson_text);
        insert.step_done();

        const auto trace_id = db.last_insert_rowid();
        insert_frame(db, trace_id, trace);
        const auto packet_count = insert_render_packets(db, trace_id, trace);
        const auto copy_count = insert_copy_events(db, trace_id, trace);
        const auto semantic_count = insert_semantic_events(db, trace_id, trace);
        const auto layout_counts = insert_layout_runtime(db, trace_id, trace);
        return ImportResult{
            .trace_id = trace_id,
            .record_count = records.size(),
            .render_packet_count = packet_count,
            .copy_event_count = copy_count,
            .semantic_event_count = semantic_count,
            .layout_runtime_count = layout_counts.layout_count,
            .layout_pane_count = layout_counts.pane_count,
            .layout_material_count = layout_counts.material_count,
            .layout_texture_count = layout_counts.texture_count,
        };
    }

}  // namespace smgpc::trace
