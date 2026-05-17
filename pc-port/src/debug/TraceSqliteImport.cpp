#include "DebugPaths.hpp"
#include "DumpJson.hpp"

#include <sqlite3.h>

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <initializer_list>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

    using Json = smgpc::dump::Json;

    class SqliteDb {
    public:
        explicit SqliteDb(const std::filesystem::path &path) {
            if (!path.parent_path().empty()) {
                std::filesystem::create_directories(path.parent_path());
            }

            if (sqlite3_open(path.string().c_str(), &mDb) != SQLITE_OK) {
                const auto message = mDb != nullptr ? sqlite3_errmsg(mDb) : "unknown sqlite open failure";
                throw std::runtime_error("could not open SQLite trace database " + path.string() + ": " + message);
            }
        }

        SqliteDb(const SqliteDb &) = delete;
        SqliteDb &operator=(const SqliteDb &) = delete;

        ~SqliteDb() {
            if (mDb != nullptr) {
                sqlite3_close(mDb);
            }
        }

        [[nodiscard]] sqlite3 *get() const {
            return mDb;
        }

        void exec(std::string_view sql) {
            char *error = nullptr;
            if (sqlite3_exec(mDb, std::string(sql).c_str(), nullptr, nullptr, &error) != SQLITE_OK) {
                auto message = std::string(error != nullptr ? error : sqlite3_errmsg(mDb));
                sqlite3_free(error);
                throw std::runtime_error("SQLite exec failed: " + message);
            }
        }

        [[nodiscard]] std::int64_t last_insert_rowid() const {
            return sqlite3_last_insert_rowid(mDb);
        }

    private:
        sqlite3 *mDb = nullptr;
    };

    class Statement {
    public:
        Statement(SqliteDb &db, std::string_view sql) : mDb(db.get()) {
            if (sqlite3_prepare_v2(mDb, std::string(sql).c_str(), -1, &mStatement, nullptr) != SQLITE_OK) {
                throw std::runtime_error("SQLite prepare failed: " + std::string(sqlite3_errmsg(mDb)));
            }
        }

        Statement(const Statement &) = delete;
        Statement &operator=(const Statement &) = delete;

        ~Statement() {
            if (mStatement != nullptr) {
                sqlite3_finalize(mStatement);
            }
        }

        void reset() {
            sqlite3_reset(mStatement);
            sqlite3_clear_bindings(mStatement);
        }

        void bind(int index, std::nullptr_t) {
            check(sqlite3_bind_null(mStatement, index));
        }

        void bind(int index, std::int64_t value) {
            check(sqlite3_bind_int64(mStatement, index, value));
        }

        void bind(int index, std::string_view value) {
            check(sqlite3_bind_text(mStatement, index, value.data(), static_cast<int>(value.size()), SQLITE_TRANSIENT));
        }

        void bind_optional_int(int index, std::optional<std::int64_t> value) {
            if (value.has_value()) {
                bind(index, *value);
            } else {
                bind(index, nullptr);
            }
        }

        void bind_optional_text(int index, std::optional<std::string> value) {
            if (value.has_value()) {
                bind(index, std::string_view(*value));
            } else {
                bind(index, nullptr);
            }
        }

        void step_done() {
            const auto result = sqlite3_step(mStatement);
            if (result != SQLITE_DONE) {
                throw std::runtime_error("SQLite step failed: " + std::string(sqlite3_errmsg(mDb)));
            }
            reset();
        }

    private:
        void check(int result) {
            if (result != SQLITE_OK) {
                throw std::runtime_error("SQLite bind failed: " + std::string(sqlite3_errmsg(mDb)));
            }
        }

        sqlite3 *mDb = nullptr;
        sqlite3_stmt *mStatement = nullptr;
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

    [[nodiscard]] std::optional<std::string> packet_cull_mode(const Json &packet) {
        if (const auto *gen_mode = member(packet, "gen_mode")) {
            if (const auto value = json_text(*gen_mode, "cull_mode"); value.has_value()) {
                return value;
            }
        }
        return json_text(packet, "cull_mode");
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

    void create_schema(SqliteDb &db) {
        db.exec(R"SQL(
            CREATE TABLE IF NOT EXISTS traces (
                id INTEGER PRIMARY KEY,
                path TEXT NOT NULL UNIQUE,
                schema_name TEXT,
                emulator TEXT,
                requested_frame INTEGER,
                raw_json TEXT NOT NULL
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
                raw_json TEXT NOT NULL,
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
                raw_json TEXT NOT NULL,
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
                raw_json TEXT NOT NULL,
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
                raw_json TEXT NOT NULL,
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
                raw_json TEXT NOT NULL,
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
                raw_json TEXT NOT NULL,
                PRIMARY KEY(trace_id, row_index)
            );
            CREATE INDEX IF NOT EXISTS idx_render_packets_material ON render_packets(material_name);
            CREATE INDEX IF NOT EXISTS idx_render_packets_signature ON render_packets(texgen_count, color_channel_count, tev_stage_count, indirect_stage_count, cull_mode, vertex_count);
            CREATE INDEX IF NOT EXISTS idx_texture_bindings_signature ON packet_texture_bindings(slot, format, width, height);
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

    void insert_frame(SqliteDb &db, std::int64_t trace_id, const Json &trace) {
        auto insert = Statement(db, R"SQL(
            INSERT INTO frames(
                trace_id, frame_index, runtime_index, presenter_frame_count, framebuffer_width, framebuffer_height, viewport_json, scissor_json
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        )SQL");
        const auto *frame = member(trace, "frame");
        if (frame == nullptr) {
            return;
        }

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

    void insert_packet_children(SqliteDb &db, std::int64_t trace_id, std::int64_t packet_index, const Json &packet) {
        Statement insert_texture(db, R"SQL(
            INSERT INTO packet_texture_bindings(
                trace_id, packet_index, binding_index, slot, texture_index, name, identity_name, address, width, height, format, format_raw, raw_json
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

        Statement insert_order(db, R"SQL(
            INSERT INTO packet_tev_orders(
                trace_id, packet_index, order_index, stage, tex_coord, tex_map, color_channel, texture_enabled, raw_json
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

        Statement insert_stage(db, R"SQL(
            INSERT INTO packet_tev_stages(
                trace_id, packet_index, stage_index, stage, color_raw, alpha_raw, color_in_json, alpha_in_json, k_color_sel, k_alpha_sel, raw_json
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

        Statement insert_light(db, R"SQL(
            INSERT INTO packet_lights(
                trace_id, packet_index, light_row, light_index, color_json, position_json, direction_json, raw_json
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

    void insert_render_packets(SqliteDb &db, std::int64_t trace_id, const Json &trace) {
        Statement insert(db, R"SQL(
            INSERT INTO render_packets(
                trace_id, packet_index, draw_index, frame_index, presenter_frame_count, model_name, material_name, render_pass, draw_pass, view_id,
                packet_mode, primitive_type, vertex_count, index_count, source_vertex_count, source_triangle_count, texgen_count,
                color_channel_count, tev_stage_count, indirect_stage_count, cull_mode, used_textures_mask, used_texture_slots_json,
                requested_light_mask, loaded_light_mask, raw_json
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        )SQL");

        const auto *packets = member(trace, "render_packets");
        if (packets == nullptr || !packets->is_array()) {
            return;
        }

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
        }
    }

    void insert_copy_events(SqliteDb &db, std::int64_t trace_id, const Json &trace) {
        Statement insert(db, R"SQL(
            INSERT INTO copy_events(
                trace_id, row_index, event_index, presenter_frame_count, kind, source_width, source_height, output_width, output_height,
                target_width, target_height, raw_json
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        )SQL");

        const auto *events = member(trace, "copy_events");
        if (events == nullptr || !events->is_array()) {
            return;
        }

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
        }
    }

    [[nodiscard]] std::int64_t insert_trace(SqliteDb &db, const std::filesystem::path &path) {
        const auto trace = smgpc::dump::load_json_file(path);
        Statement insert(db, R"SQL(
            INSERT INTO traces(path, schema_name, emulator, requested_frame, raw_json)
            VALUES(?, ?, ?, ?, ?)
        )SQL");

        insert.bind(1, std::filesystem::absolute(path).string());
        insert.bind_optional_text(2, json_text(trace, "schema"));
        auto emulator = json_text(trace, "emulator");
        if (!emulator.has_value()) {
            emulator = default_emulator_for_path(path);
        }
        insert.bind_optional_text(3, emulator);
        insert.bind_optional_int(4, json_int(trace, "requested_frame"));
        insert.bind(5, json_raw(trace));
        insert.step_done();

        const auto trace_id = db.last_insert_rowid();
        insert_frame(db, trace_id, trace);
        insert_render_packets(db, trace_id, trace);
        insert_copy_events(db, trace_id, trace);
        return trace_id;
    }

    struct Options {
        std::filesystem::path output;
        bool append = false;
        std::vector<std::filesystem::path> traces;
    };

    void print_usage(std::ostream &out) {
        out << "usage: smg-pc-trace-import-sqlite [--output traces.sqlite] [--append] [trace.json ...]\n";
        out << "If no traces are provided, imports cached frame-1900 Dolphin and PC traces when present.\n";
    }

    [[nodiscard]] Options parse_args(int argc, char **argv) {
        auto options = Options{};
        options.output = smgpc::debug::pc_port_root() / ".cache" / "render-parity" / "traces.sqlite";

        for (auto i = 1; i < argc; ++i) {
            const auto arg = std::string_view(argv[i]);
            if (arg == "--help" || arg == "-h") {
                print_usage(std::cout);
                std::exit(0);
            }
            if (arg == "--append") {
                options.append = true;
                continue;
            }
            if (arg == "--output") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--output requires a path");
                }
                options.output = argv[++i];
                continue;
            }
            options.traces.emplace_back(arg);
        }

        if (options.traces.empty()) {
            const auto cache = smgpc::debug::pc_port_root() / ".cache" / "render-parity";
            const std::filesystem::path defaults[]{
                cache / "dolphin-frame-1900.trace.json",
                cache / "pcport-frame-1900.trace.json",
            };
            for (const auto &path : defaults) {
                if (std::filesystem::is_regular_file(path)) {
                    options.traces.push_back(path);
                }
            }
        }

        if (options.traces.empty()) {
            throw std::runtime_error("no trace files provided and no cached frame-1900 traces found");
        }

        return options;
    }

}  // namespace

int main(int argc, char **argv) try {
    const auto options = parse_args(argc, argv);
    if (!options.append && std::filesystem::exists(options.output)) {
        std::filesystem::remove(options.output);
    }

    auto db = SqliteDb(options.output);
    db.exec("PRAGMA foreign_keys = ON");
    create_schema(db);
    db.exec("BEGIN IMMEDIATE TRANSACTION");
    for (const auto &trace_path : options.traces) {
        const auto trace_id = insert_trace(db, trace_path);
        std::cout << "imported trace_id=" << trace_id << " path=" << trace_path << '\n';
    }
    db.exec("COMMIT");

    std::cout << options.output << '\n';
    return 0;
} catch (const std::exception &e) {
    std::cerr << "trace SQLite import failed: " << e.what() << '\n';
    return 1;
}
